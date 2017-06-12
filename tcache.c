/*
   Tcache.c
 */

#include <linux/kthread.h>
#include <linux/swap.h>
#include <linux/sched.h>
#include <linux/bio.h>
#include <linux/delay.h>
#include "utmem.h"

#define SSD_NAME "/dev/sda1"

static struct kmem_cache *utmem_pampd_cache;
static struct kmem_cache *utmem_objnode_cache;
static struct kmem_cache *utmem_obj_cache;


static struct bio *get_ssd_bio(gfp_t gfp_flags,
		struct page *page, bio_end_io_t end_io, void *private)
{
	struct bio *bio;

	bio = bio_alloc(gfp_flags, 1);
	if (bio) {
		/*
		   bio->bi_iter.bi_sector = map_swap_page(page, &bio->bi_bdev);
		   bio->bi_iter.bi_sector <<= PAGE_SHIFT - 9; */
		bio->bi_io_vec[0].bv_page = page;
		bio->bi_io_vec[0].bv_len = PAGE_SIZE;
		bio->bi_io_vec[0].bv_offset = 0;
		bio->bi_vcnt = 1;
		bio->bi_iter.bi_size = PAGE_SIZE;
		bio->bi_private = private;
		bio->bi_end_io = end_io;
	}
	return bio;
}

void end_ssd_bio_write(struct bio *bio, int err)
{
	const int uptodate = test_bit(BIO_UPTODATE, &bio->bi_flags);
	struct page *page = bio->bi_io_vec[0].bv_page;
	utmem_pampd *n = bio->bi_private;
	utmemassert(n);

	if (!uptodate) {
		/*
		   We see which sector is not-writable
		   log it for the time being
		 */
		printk(KERN_ALERT "Write-error on ssd-device (%Lu)\n",
				(unsigned long long)bio->bi_iter.bi_sector);
	}
	unlock_page(page);
	__free_page(page);
	//        n->page = bio->bi_iter.bi_sector >> (PAGE_SHIFT - 9);
	n->status = UPTODATE;
	wmb();

	atomic_dec(&n->tmem_obj->pool->client->g->pending_async_writes);

	bio_put(bio);
}

void end_ssd_bio_read(struct bio *bio, int err)
{

	const int uptodate = test_bit(BIO_UPTODATE, &bio->bi_flags);
	struct page *page = bio->bi_io_vec[0].bv_page;

	if (!uptodate) {
		printk(KERN_ALERT "Read-error on ssd-device (%Lu)\n",
				(unsigned long long)bio->bi_iter.bi_sector);
	}

	unlock_page(page);

}
static unsigned long get_ssd_free_block(struct global_info *g)
{
	u8 *bmap_ptr;
	unsigned ins_byte_position;
	u8 ins_bit, val;

	raw_spin_lock(&g->kvm_tmem_lock);  

	ins_byte_position = g->next_byte;
	ins_bit = g->next_bit;	

	bmap_ptr = g->ssd_bmap + ins_byte_position;
	
	val = *bmap_ptr;
	utmemassert(val != 0xff);
	val >>= ins_bit; 
	val = 1 << ins_bit;
	
	utmemassert((*bmap_ptr & val) == 0);
	*bmap_ptr = (*bmap_ptr) ^ val;
	
	g->next_bit = (ins_bit+1) % 8;
	if(g->next_bit==0){
		g->next_byte++;
	}

	bmap_ptr = g->ssd_bmap + g->next_byte;
	
	if(unlikely((void *)bmap_ptr >=  g->ssd_bmap + g->ssd_bmap_size)){
		//printk("\tReached end of map-1\n");
		bmap_ptr = g->ssd_bmap;
		g->next_bit = 0;
	} 
check:
	g->next_byte = (void *)bmap_ptr - g->ssd_bmap;

	if(likely(*bmap_ptr != 0xff))
		goto got_byte;
	
	g->next_bit = 0;	
	bmap_ptr++;
	g->next_byte++;

	/* Checking if current byte has free bits, if not increament */
	while((void *)bmap_ptr < g->ssd_bmap + g->ssd_bmap_size && *bmap_ptr == 0xff){
		//printk("\t%u < %u, VALUE-AT-MAP:%u, @BLOCK:%u \n",  (void *)bmap_ptr, g->ssd_bmap + g->ssd_bmap_size, *bmap_ptr, (g->next_byte<<3) + g->next_bit);
		bmap_ptr++;
		g->next_byte++;
	}
	//printk("\t%u < %u, VALUE-AT-MAP:%u, @BLOCK:%u \n",  (void *)bmap_ptr, g->ssd_bmap + g->ssd_bmap_size, *bmap_ptr, (g->next_byte<<3) + g->next_bit);
	
	/* Reached end of bit map, reinitialize to start */
	if((void *)bmap_ptr >=  g->ssd_bmap + g->ssd_bmap_size){
		//printk("\tReached end of map-2\n");
		bmap_ptr = g->ssd_bmap;
	} 
	goto check; 

got_byte:

	val = *bmap_ptr;
	utmemassert(val != 0xff);
	
	val >>= g->next_bit; 
	
	while(val & 1){
		g->next_bit++;

		if(unlikely(g->next_bit==8)){
			g->next_bit = 0;
			bmap_ptr++;
			goto check;
		}

		val >>= 1;
	}  

	raw_spin_unlock(&g->kvm_tmem_lock); 
	
	//printk("INS-BLOCK:%u, NEXT-BLOCK:%u\n", ((ins_byte_position << 3) + ins_bit), ((g->next_byte << 3) + g->next_bit));
	//printk("Bit:%u, Block:%lu\n", g->next_bit, g->next_byte);

	return ((ins_byte_position << 3) + ins_bit);

}

static void mark_free(struct global_info *g, unsigned long block_no)
{
	u8 *bmap_ptr;
	u8 val = 1;
	unsigned byte = block_no >> 3;
	unsigned bit = block_no % 8;

	bmap_ptr = g->ssd_bmap + byte;
	val <<= bit;

	//printk("FREE: %lu\n", block_no);

	utmemassert(((*bmap_ptr) & val) == val);

	raw_spin_lock(&g->kvm_tmem_lock);  
	*bmap_ptr = *bmap_ptr ^ val;
	raw_spin_unlock(&g->kvm_tmem_lock); 
}

static void free_ssd_block(struct global_info *g, utmem_pampd *n)
{
	while(n->status == IO_IN_PROGRESS){
		asm volatile(
				"pause"
			    );
	}

	utmemassert(n->page <= g->ssd_limit);
	mark_free(g, n->page);
}

static int ssd_alloc_and_write(struct global_info *g, utmem_pampd *n)
{

	struct page *p = pfn_to_page(__pa(n->page) >> PAGE_SHIFT);
	struct bio *bio;
	unsigned block_offset;    

	BUG_ON(!p);

	n->status = IO_IN_PROGRESS;
	n->type = SSD;
	atomic_inc(&g->pending_async_writes);

	lock_page(p);
	bio = get_ssd_bio(GFP_KERNEL, p, end_ssd_bio_write, n);  
	BUG_ON(!bio);

	block_offset = get_ssd_free_block(g);  
	//printk("ALLOC: %lu\n", block_offset); 

	n->page = block_offset; 

	bio->bi_iter.bi_sector = (block_offset) << (PAGE_SHIFT - 9);
	bio->bi_bdev = g->bdev;

	submit_bio(WRITE, bio);

	//printk("PUT-R: %lu \n", n->page); 

	return 0;

}

static int read_and_free_from_ssd(struct global_info *g, struct page *page, utmem_pampd *n)
{
	struct bio *bio;
	int uptodate;

	BUG_ON(!page);

	while(unlikely(n->status == IO_IN_PROGRESS)){
		asm volatile(
				"pause"
			    );
	}

	lock_page(page);
	bio = get_ssd_bio(GFP_KERNEL, page, end_ssd_bio_read, n);  
	BUG_ON(!bio);

	bio->bi_iter.bi_sector = (n->page) << (PAGE_SHIFT - 9);
	bio->bi_bdev = g->bdev;
	submit_bio(READ_SYNC, bio);

	wait_on_page_locked(page); 

	uptodate = test_bit(BIO_UPTODATE, &bio->bi_flags);
	bio_put(bio);
	mark_free(g, n->page);

	if(uptodate)
		return 0;

	return -EIO;
} 


static void *utmem_pampd_create(char *data, size_t size, bool raw, int eph,
		struct tmem_pool *pool, struct tmem_oid *oid,
		uint32_t index,  void* tmem_obj, bool is_ssd)
{
	void *page;
	struct page *spage = (struct page *)data;
	void *src,*dst;

	struct tmem_client *client = pool->client;
	struct eviction_info *ev;

#ifdef GLOBAL_EVICT
	ev = client->eviction_info;
#endif

	utmem_pampd *n = NULL;

	page = (void *)__get_free_page(UTMEM_GFP_MASK);
	if(unlikely(!page))
		goto wakeup_and_failed;

	src = kmap_atomic(spage);
	dst = page;

	memcpy(dst,src,PAGE_SIZE);

	kunmap_atomic(src);    

	n = kmem_cache_alloc(utmem_pampd_cache, UTMEM_GFP_MASK);
	if(unlikely(!n)){
		free_page((unsigned long) page);
		goto wakeup_and_failed;
	}

	n->page = (unsigned long) page;
	n->tmem_obj = tmem_obj;
	n->index = index;

	if(is_ssd){

		ssd_alloc_and_write(client->g, n);

		ev = pool->ssd_eviction_info;	
		spin_lock(&ev->ev_lock); 
		list_add_tail(&n->entry_list, &ev->head); 
		spin_unlock(&ev->ev_lock); 
		
		atomic_inc(&client->ssd_used); 
		atomic_inc(&client->g->ssd_used);
		atomic_inc(&pool->ssd_used);

		pool->ssd_puts++;
	}
	else{

		n->type = MEMORY;
		n->status = UPTODATE;

		ev = pool->mem_eviction_info;
		spin_lock(&ev->ev_lock); 
		list_add_tail(&n->entry_list, &ev->head); 
		spin_unlock(&ev->ev_lock); 

		atomic_inc(&client->mem_used); 
		atomic_inc(&client->g->mem_used);
		atomic_inc(&pool->mem_used);

		pool->mem_puts++;
	}

	return n;

wakeup_and_failed:
	return NULL;
}   

static int utmem_pampd_get_data(char *data, size_t *bufsize, bool raw,
		void *pampd, struct tmem_pool *pool,
		struct tmem_oid *oid, uint32_t index)
{
	/* Should not be called for cleancache BE */
	BUG_ON(1);     
	return -1;

}
static int utmem_pampd_get_data_and_free(char *data, size_t *bufsize, bool raw,
		void *pampd, struct tmem_pool *pool,
		struct tmem_oid *oid, uint32_t index)
{
	void *dst;
	int ret = -1;

	utmem_pampd *n = (utmem_pampd *) pampd;

	struct page *page = (struct page *)data;
	struct tmem_client *client = pool->client;
	struct tmem_obj *tmem;
	struct eviction_info *ev;

	/* TD: GLOBAL EVICT FOR SSD MEM */
#ifdef GLOBAL_EVICT
	ev = client->eviction_info;
#endif
	BUG_ON(!n);

	tmem = (struct tmem_obj *)n->tmem_obj;

	if(n->type == SSD){		
		ret = read_and_free_from_ssd(client->g, page, n);
		
		ev = pool->ssd_eviction_info;
		spin_lock(&ev->ev_lock); 
		list_del(&n->entry_list);
		spin_unlock(&ev->ev_lock);
		
		atomic_dec(&client->ssd_used);
		atomic_dec(&client->g->ssd_used);
		atomic_dec(&pool->ssd_used);

		pool->ssd_gets++;
	}
	else{
		dst = kmap_atomic(page);
		memcpy(dst, (void *)n->page, PAGE_SIZE);
		kunmap_atomic(dst);    
		free_page(n->page);

		ev = pool->mem_eviction_info;
		spin_lock(&ev->ev_lock); 
		list_del(&n->entry_list);
		spin_unlock(&ev->ev_lock);
		
		atomic_dec(&client->mem_used);
		atomic_dec(&client->g->mem_used);
		atomic_dec(&pool->mem_used);

		pool->mem_gets++;

	}
	
	ret = 0;

	kmem_cache_free(utmem_pampd_cache, n);
	return ret;

}

static void utmem_pampd_free(void *pampd, struct tmem_pool *pool,
		struct tmem_oid *oid, uint32_t index)
{
	utmem_pampd *n = (utmem_pampd *) pampd;
	struct tmem_client *client = pool->client;


	struct eviction_info *ev;

	/* TD: GLOBAL EVICT FOR SSD MEM */
#ifdef GLOBAL_EVICT
	ev = client->eviction_info;
#endif
	BUG_ON(!n);

	if(n->type == SSD){		
		free_ssd_block(client->g, n);
		atomic_dec(&client->ssd_used);
		atomic_dec(&client->g->ssd_used);
		atomic_dec(&pool->ssd_used);

		ev = pool->ssd_eviction_info;
		spin_lock(&ev->ev_lock); 
		list_del(&n->entry_list);
		spin_unlock(&ev->ev_lock);
		
		pool->ssd_flushes++;
	}
	else{
		free_page(n->page);
		atomic_dec(&client->mem_used);
		atomic_dec(&client->g->mem_used);
		atomic_dec(&pool->mem_used);

		ev = pool->mem_eviction_info;
		spin_lock(&ev->ev_lock); 
		list_del(&n->entry_list);
		spin_unlock(&ev->ev_lock);

		pool->mem_flushes++;	
	}

	kmem_cache_free(utmem_pampd_cache, n);
	return;
}

static void utmem_pampd_free_obj(struct tmem_pool *pool, struct tmem_obj *obj)
{
	struct tmem_client *client = pool->client;   

	/*
	   struct tmem_extra *acc = obj->extra;
	   utmemassert(client->client_id == client_id);
	   utmemassert(acc);
	 */

	atomic_dec(&client->num_tmem_objs);

	// kmem_cache_free(utmem_tmemex_cache, acc);
	// obj->extra = NULL;

	return;
}

static void utmem_pampd_new_obj(struct tmem_obj *obj)
{
	struct tmem_client *client = obj->pool->client;   

	/*
	   struct tmem_extra *acc;
	   utmemassert(client->client_id == client_id);
	   acc = kmem_cache_alloc(utmem_tmemex_cache, ZCACHE_GFP_MASK);
	   BUG_ON(!acc);
	   memset(acc,0,sizeof(struct tmem_extra));

	   obj->extra = acc;
	 */
	atomic_inc(&client->num_tmem_objs);
}

static int utmem_pampd_replace_in_obj(void *pampd, struct tmem_obj *obj)
{
	return -1;
}

static bool utmem_pampd_is_remote(void *pampd)
{
	return 0;
}
static struct tmem_pamops utmem_pamops = {
	.create = utmem_pampd_create,
	.get_data = utmem_pampd_get_data,
	.get_data_and_free = utmem_pampd_get_data_and_free,
	.free = utmem_pampd_free,
	.free_obj = utmem_pampd_free_obj,
	.new_obj = utmem_pampd_new_obj,
	.replace_in_obj = utmem_pampd_replace_in_obj,
	.is_remote = utmem_pampd_is_remote,
};


/*
   static int shrink_utmem_memory(struct shrinker *shrink,
   struct shrink_control *sc)
   {
   int ret = -1;
   int nr = sc->nr_to_scan;
   gfp_t gfp_mask = sc->gfp_mask;

   if (nr >= 0) {
   if (!(gfp_mask & __GFP_FS))
//does this case really need to be skipped? 
goto out;
ret = utmem_evict_pages(nr, true);
}
out:
return ret;
}

static struct shrinker utmem_shrinker = {
.shrink = shrink_utmem_memory,
.seeks = DEFAULT_SEEKS,
};*/
/*
   int utmem_eviction_kthread(void *data) {
   struct global_info *g = (struct global_info *) data;
   while (!kthread_should_stop()) {
   start_evict = false;
   wait_event_interruptible_timeout(evict_wq,
   (start_evict || kthread_should_stop()), 600*HZ); //run every 5 mins to evict useless pages
   if(kthread_should_stop())
   break;
   if(start_evict && nr_free_pages()  <= FREE_MEM_THRESH ){
   utmem_evict_pages(EVICT_BATCH, true);
   }else if(start_evict && atomic_read(&g->total_used) + LIMIT_THRESH >= g->global_limit){
   utmem_evict_pages(0, false);
   }else if(start_evict){
   utmem_evict_pages(0, false);
   }

   }
   return 0;
   }
 */
/*
 * ut implementation for tmem host ops
 */

static struct tmem_objnode *utmem_objnode_alloc(struct tmem_pool *pool)
{
	struct tmem_objnode *objnode = NULL;
	objnode = kmem_cache_alloc(utmem_objnode_cache,
			UTMEM_GFP_MASK);

	return objnode;
}

static void utmem_objnode_free(struct tmem_objnode *objnode,
		struct tmem_pool *pool)
{
	kmem_cache_free(utmem_objnode_cache, objnode);
}

static struct tmem_obj *utmem_obj_alloc(struct tmem_pool *pool)
{
	struct tmem_obj *obj = NULL;
	obj = kmem_cache_alloc(utmem_obj_cache, UTMEM_GFP_MASK);
	return obj;
}

static void utmem_obj_free(struct tmem_obj *obj, struct tmem_pool *pool)
{
	kmem_cache_free(utmem_obj_cache, obj);
}


int tcache_move_mem_to_ssd(struct tmem_pool *pool, int num_of_pages)
{
	int i;
	struct tmem_hashbucket *hb;	
	utmem_pampd *n = NULL; 	
	struct tmem_client *client = pool->client;
	struct eviction_info *mem_ev = pool->mem_eviction_info;
	struct eviction_info *ssd_ev = pool->ssd_eviction_info;

	for(i=0; i<num_of_pages; i++){

		n = list_first_entry(&mem_ev->head, struct utmem_pampd, entry_list);

		if(unlikely(!n)){				
			goto wakeup_and_failed;
		}
		
		hb = &pool->hashbucket[tmem_oid_hash(&n->tmem_obj->oid)];
		spin_lock(&hb->lock);		
		
		spin_lock(&mem_ev->ev_lock); 
		list_del(&n->entry_list);
		spin_unlock(&mem_ev->ev_lock); 

		ssd_alloc_and_write(client->g, n);

		spin_lock(&ssd_ev->ev_lock); 
		list_add_tail(&n->entry_list, &ssd_ev->head); 
		spin_unlock(&ssd_ev->ev_lock); 
		
		spin_unlock(&hb->lock);		

	}

wakeup_and_failed:

	atomic_sub(i, &client->mem_used);
	atomic_sub(i, &client->g->mem_used);
	atomic_sub(i, &pool->mem_used);

	atomic_add(i, &client->ssd_used); 
	atomic_add(i, &client->g->ssd_used);
	atomic_add(i, &pool->ssd_used);

	pool->move_mem_to_ssd += i;

	return i;
}

int tcache_move_ssd_to_mem(struct tmem_pool *pool, int num_of_pages)
{
	int i=0, ret=-1;

	utmem_pampd *n = NULL; 

	struct page *page;
	struct tmem_hashbucket *hb;	
	struct tmem_client *client = pool->client;
	struct eviction_info *mem_ev = pool->mem_eviction_info;
	struct eviction_info *ssd_ev = pool->ssd_eviction_info;

	for(i=0; i<num_of_pages; i++){

		n = list_first_entry(&ssd_ev->head, struct utmem_pampd, entry_list);

		if(unlikely(!n) || unlikely(n->status == IO_IN_PROGRESS)){
			goto wakeup_and_failed;
		}

		hb = &pool->hashbucket[tmem_oid_hash(&n->tmem_obj->oid)];
		spin_lock(&hb->lock);		

		spin_lock(&ssd_ev->ev_lock);
		list_del(&n->entry_list); 	
		spin_unlock(&ssd_ev->ev_lock); 

		page = alloc_page(UTMEM_GFP_MASK);
		if(unlikely(!page)){	
			spin_lock(&ssd_ev->ev_lock); 
			list_add(&n->entry_list, &ssd_ev->head); 
			spin_unlock(&ssd_ev->ev_lock); 

			goto wakeup_and_failed;
		}

		n->type = MEMORY;

		ret = read_and_free_from_ssd(client->g, page, n);
		n->page = (unsigned long) page_address(page);

		spin_lock(&mem_ev->ev_lock); 
		list_add_tail(&n->entry_list, &mem_ev->head); 
		spin_unlock(&mem_ev->ev_lock);

		spin_unlock(&hb->lock);		
	}

wakeup_and_failed:

	atomic_sub(i, &pool->ssd_used);
	atomic_sub(i, &client->ssd_used); 
	atomic_sub(i, &client->g->ssd_used);

	atomic_add(i, &pool->mem_used);
	atomic_add(i, &client->mem_used);
	atomic_add(i, &client->g->mem_used);

	pool->move_ssd_to_mem += i;

	return i;
}

static struct tmem_hostops utmem_hostops = {
	.obj_alloc = utmem_obj_alloc,
	.obj_free = utmem_obj_free,
	.objnode_alloc = utmem_objnode_alloc,
	.objnode_free = utmem_objnode_free,
};

/*
 *	Initialization for utmem
 */
int init_utmem(struct global_info *g)
{

	utmem_pampd_cache = kmem_cache_create("utmem_node_cache", sizeof(utmem_pampd), 0, 0, NULL);
	utmem_obj_cache = kmem_cache_create("utmem_obj_cache", sizeof(struct tmem_obj),0,0,NULL);
	utmem_objnode_cache = kmem_cache_create("utmem_objnode_cache", sizeof(struct tmem_objnode),0,0,NULL);

	g->mem_limit = totalram_pages - FREE_RAM_THRESH; 

	tmem_register_pamops(&utmem_pamops);
	tmem_register_hostops(&utmem_hostops);

	// register_shrinker(&utmem_shrinker);
	// init_waitqueue_head(&evict_wq);
	// utmem_eviction_thread = kthread_create(utmem_eviction_kthread, g, "utmem_evicter");
	// BUG_ON(IS_ERR(utmem_eviction_thread));
	// wake_up_process(utmem_eviction_thread);

	g->bdev = blkdev_get_by_path(SSD_NAME, FMODE_READ|FMODE_WRITE|FMODE_EXCL, THIS_MODULE);

	if (IS_ERR(g->bdev)){
		printk(KERN_INFO "Block device lookup failed\n");
		return -EINVAL;
	}          
	if(!g->bdev->bd_disk){
		printk(KERN_INFO "Block device lookup failed inside\n");
		return -EINVAL;
	}

	g->ssd_limit = 33554432;  /* NUM BLOCKS */ 
	g->ssd_bmap_size = (33554432 >> 3);
	g->ssd_bmap = vmalloc(g->ssd_bmap_size);
	memset(g->ssd_bmap, 0, g->ssd_bmap_size);

	return 0;
}

/*
 *	Clean up for utmem
 */
int cleanup_utmem(struct global_info *g)
{
	kmem_cache_destroy(utmem_pampd_cache);
	kmem_cache_destroy(utmem_obj_cache);
	kmem_cache_destroy(utmem_objnode_cache);

	if(g->ssd_bmap)
		vfree(g->ssd_bmap);
	if(g->bdev)
		blkdev_put(g->bdev, FMODE_READ|FMODE_WRITE|FMODE_EXCL);

	return 0;
}
