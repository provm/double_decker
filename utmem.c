#include<linux/spinlock.h>
#include "utmem.h"
#include "tmem.h"
#define EVICT_BATCH 4096

#define MAX(a,b) (a) > (b) ? (a) : (b)

static struct global_info *global;

static int readjust_client_entitlements(struct global_info *g);
static int check_and_readjust_allocations(struct tmem_client *client);

static ssize_t utmem_total_objects_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        return sprintf(buf, "%u\n", atomic_read(&global->mem_used));
}

static struct kobj_attribute utmem_total_objects_attribute = __ATTR(mem_used,0444,utmem_total_objects_show,NULL);

static ssize_t utmem_ssd_objects_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        return sprintf(buf, "%u\n", atomic_read(&global->ssd_used));
}

static struct kobj_attribute utmem_ssd_objects_attribute = __ATTR(ssd_used,0444,utmem_ssd_objects_show,NULL);


static ssize_t utmem_global_limit_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        return sprintf(buf, "%u\n",global->mem_limit);
}

static ssize_t utmem_global_limit_set(struct kobject *kobj,
                                   struct kobj_attribute *attr,
                                   const char *buf, size_t count)
{
        int err;
        unsigned long mode;


        err = kstrtoul(buf, 10, &mode);
        if (err)
                return -EINVAL;

        global->mem_limit = mode;
        readjust_client_entitlements(global);
        return count;
}

static struct kobj_attribute utmem_global_limit_attribute = __ATTR(global_limit, 0644, utmem_global_limit_show, utmem_global_limit_set);


static ssize_t utmem_ssd_limit_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        return sprintf(buf, "%u\n",global->ssd_limit);
}

static ssize_t utmem_ssd_limit_set(struct kobject *kobj,
                                   struct kobj_attribute *attr,
                                   const char *buf, size_t count)
{
        int err;
        unsigned long mode;


        err = kstrtoul(buf, 10, &mode);
        if (err)
                return -EINVAL;
        
        if(mode > 33554432)
             return -EINVAL;

        mode = (mode >> 3) << 3;

        global->ssd_limit = mode;
        global->ssd_bmap_size = (mode >> 3);

        return count;
}

static struct kobj_attribute utmem_ssd_limit_attribute = __ATTR(ssd_limit, 0644, utmem_ssd_limit_show, utmem_ssd_limit_set);

static ssize_t utmem_total_gets_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        return sprintf(buf, "%lu\n", global->gets);
}

static struct kobj_attribute utmem_total_gets_attribute = __ATTR(gets,0444,utmem_total_gets_show,NULL);


static ssize_t utmem_total_puts_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        return sprintf(buf, "%lu\n", global->puts);
}

static struct kobj_attribute utmem_total_puts_attribute = __ATTR(puts,0444,utmem_total_puts_show,NULL);

static ssize_t utmem_total_flushes_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        return sprintf(buf, "%lu\n", global->flushes);
}

static struct kobj_attribute utmem_total_flushes_attribute = __ATTR(flushes,0444,utmem_total_flushes_show,NULL);



static struct attribute *utmem_attrs[] = {
        &utmem_total_objects_attribute.attr,
        &utmem_ssd_objects_attribute.attr,
        &utmem_global_limit_attribute.attr,
	&utmem_ssd_limit_attribute.attr,
        &utmem_total_gets_attribute.attr,
        &utmem_total_puts_attribute.attr,
        &utmem_total_flushes_attribute.attr,
        NULL,
};
static struct attribute_group utmem_attr_group = {
        .attrs = utmem_attrs,
};

/*
	Re-adjust per VM entitlements
*/
static int readjust_client_entitlements(struct global_info *g)
{
   int total_weight = 100, i;
   
   for(i=0; i<MAX_VMS && g->tmem_clients[i].id >= 0; ++i){
       struct tmem_client *client = &g->tmem_clients[i];
       utmemassert(client->weight <= 100 && client->weight >= 0);
       total_weight -= client->weight;
   }
   if(total_weight)
        return -1;
   for(i=0; i<MAX_VMS && g->tmem_clients[i].id >= 0; ++i){
           struct tmem_client *client = &g->tmem_clients[i];
           client->mem_entitlement = client->weight * g->mem_limit / 100;
           client->ssd_entitlement = client->weight * g->ssd_limit / 100;
           check_and_readjust_allocations(client);
   }        

   return 0;
  
}

static inline struct tmem_client *find_tmem_client_by_id(struct global_info *g, int client_id)
{
   int i;
   for(i=0; i<MAX_VMS && client_id != g->tmem_clients[i].id; ++i)
   ;
   if(unlikely(i == MAX_VMS))
     return NULL;

   return &g->tmem_clients[i];
   
}
static inline struct tmem_client* get_client_from_kobj(struct kobject *kobj){
   
        int client_id;

        kstrtoint(kobj->name, 10, &client_id);
        if(client_id < 0 || client_id > MAX_VMS)
            return NULL;
        return find_tmem_client_by_id(global, client_id); 
}

static ssize_t client_weight_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        struct tmem_client *client = NULL;
         
        client = get_client_from_kobj(kobj);
        if(!client)
               return -EINVAL;

        return sprintf(buf, "%d\n", client->weight);
}

static ssize_t client_weight_set(struct kobject *kobj,
                                   struct kobj_attribute *attr,
                                   const char *buf, size_t count)
{
        int err;
        int mode;

        struct tmem_client *client = NULL;
         
        client = get_client_from_kobj(kobj);
        if(!client)
               return -EINVAL;

        err = kstrtoint(buf, 10, &mode);
        if (err || mode < 0 || mode > 100)
                return -EINVAL;

        client->weight = mode;
        readjust_client_entitlements(client->g);
        return count;
}


static struct kobj_attribute client_weight_attribute = __ATTR(weight,0644,client_weight_show,client_weight_set);

static ssize_t client_entitlement_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        struct tmem_client *client = NULL;
         
        client = get_client_from_kobj(kobj);
        
        if(!client)
               return -EINVAL;

        return sprintf(buf, "%u\n", client->mem_entitlement);
}

static struct kobj_attribute client_entitlement_attribute = __ATTR(max_mem,0444,client_entitlement_show,NULL);


static ssize_t client_ssd_entitlement_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        struct tmem_client *client = NULL;
         
        client = get_client_from_kobj(kobj);
        
        if(!client)
               return -EINVAL;

        return sprintf(buf, "%u\n", client->ssd_entitlement);
}
static ssize_t client_ssd_entitlement_set(struct kobject *kobj,
                                   struct kobj_attribute *attr,
                                   const char *buf, size_t count)
{
        int err;
        unsigned mode;

        struct tmem_client *client = NULL;

        client = get_client_from_kobj(kobj);
        if(!client)
               return -EINVAL;

        err = kstrtouint(buf, 10, &mode);
        if (err || mode > client->g->ssd_limit)
                return -EINVAL;

        client->ssd_entitlement = mode;
        return count;
}


static struct kobj_attribute client_ssd_entitlement_attribute = __ATTR(max_ssd,0644,client_ssd_entitlement_show, client_ssd_entitlement_set);


static ssize_t client_used_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        struct tmem_client *client = NULL;
         
        client = get_client_from_kobj(kobj);
        
        if(!client)
               return -EINVAL;

        return sprintf(buf, "%u\n", atomic_read(&client->mem_used));
}
static struct kobj_attribute client_used_attribute = __ATTR(mem_used,0444,client_used_show,NULL);


static ssize_t client_ssd_used_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        struct tmem_client *client = NULL;
         
        client = get_client_from_kobj(kobj);
        
        if(!client)
               return -EINVAL;

        return sprintf(buf, "%u\n", atomic_read(&client->ssd_used));
}
static struct kobj_attribute client_ssd_used_attribute = __ATTR(ssd_used,0444,client_ssd_used_show,NULL);


static struct attribute *client_attrs[] = {
        &client_weight_attribute.attr,
        &client_entitlement_attribute.attr,
        &client_ssd_entitlement_attribute.attr,
        &client_used_attribute.attr,
        &client_ssd_used_attribute.attr,
        NULL,
};
static struct attribute_group client_attr_group = {
        .attrs = client_attrs,
};

static inline struct tmem_client* new_tmem_client(struct kvm *kvm, struct global_info *g)
{
     struct eviction_info *ev;
     int i;
     struct tmem_client *client;
     raw_spin_lock(&g->kvm_tmem_lock);
     if(unlikely(g->current_num == MAX_VMS)){
         printk(KERN_INFO "More than 16 VMs running. BUG other wise\n");
         raw_spin_unlock(&g->kvm_tmem_lock);
         return NULL;
     }
    for(i=0; i<MAX_VMS; ++i){
          if(g->tmem_clients[i].guest_mm == NULL)
               break;
    }
    if(unlikely(i == MAX_VMS))
        return NULL;
   
    client = &g->tmem_clients[i];
    memset(client, 0, sizeof(struct tmem_client));
    client->guest_mm = kvm->mm;
    client->id = i;
    client->g = g;
    client->weight = 0;
    atomic_set(&client->evicting, -1);

    ++g->current_num;
    raw_spin_unlock(&g->kvm_tmem_lock);
    
    if(g->utmem_kobj){
             char name[10];
             sprintf(name,"%u",i);
             client->client_kobj = kobject_create_and_add(name, g->utmem_kobj);
             utmemassert(client->client_kobj);
             if(sysfs_create_group(client->client_kobj, &client_attr_group))
                       printk(KERN_INFO "gcache: can't create sysfs\n");
     }
 
    ev = kzalloc(sizeof(struct eviction_info), GFP_KERNEL);
    INIT_LIST_HEAD(&ev->head);
    spin_lock_init(&ev->ev_lock); 
    client->eviction_info = ev;

    readjust_client_entitlements(g);
    return client;
}

/*
 *	Finds the tmem_client corresponding to kvm-VM
 *  	by matching to mm_struct of the tmem-client (VM)
 */
static inline struct tmem_client* find_tmem_client(struct kvm *kvm, struct global_info *g)
{
	int i;
	for(i=0; i<MAX_VMS && kvm->mm != g->tmem_clients[i].guest_mm; ++i);
   
	if(unlikely(i == MAX_VMS))
     	return NULL;

   	return &g->tmem_clients[i];
}


/*
 * 	Destroy tmem-client related state
 */
static inline int destroy_tmem_client(struct kvm *kvm, struct global_info *g)
{
	struct eviction_info *ev;
	struct tmem_client *client;
	
	raw_spin_lock(&g->kvm_tmem_lock);
	
	if((client = find_tmem_client(kvm, g)) == NULL){
		raw_spin_unlock(&g->kvm_tmem_lock);
		return -EINVAL;
	}

	client->guest_mm = NULL;
	client->id = -1;
	client->weight = 0;

	sysfs_remove_group(client->client_kobj, &client_attr_group);
	kobject_del(client->client_kobj);

	g->current_num--;
	raw_spin_unlock(&g->kvm_tmem_lock);

	ev = client->eviction_info;
	utmemassert(ev && list_empty(&ev->head));
	kfree(ev);

	readjust_client_entitlements(g);

  	return 0;
}


static inline struct tmem_pool* get_pool_from_kobj(struct kobject *kobj)
{
        struct tmem_client *client = NULL;
        int pool_id, client_id;

        kstrtoint(kobj->name, 10, &pool_id);
        if(pool_id < 0 || pool_id > MAX_POOLS_PER_CLIENT)
            goto out;
        
        if(!kobj->parent)
               goto out;
        
        kstrtoint(kobj->parent->name, 10, &client_id);
        if(client_id < 0 || client_id > MAX_VMS)
            goto out;
        
        if((client = find_tmem_client_by_id(global, client_id)) == NULL)
             goto out;
        
        return client->pools[pool_id];
         
out:
        return NULL;

}

static ssize_t pool_weight_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        struct tmem_pool *pool = NULL;
         
        pool = get_pool_from_kobj(kobj);
        if(!pool)
               return -EINVAL;

        return sprintf(buf, "%d\n", pool->weight);
}

static struct kobj_attribute pool_weight_attribute = __ATTR(weight,0444,pool_weight_show,NULL);

static ssize_t pool_entitlement_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        struct tmem_pool *pool = NULL;
         
        pool = get_pool_from_kobj(kobj);
        if(!pool)
               return -EINVAL;

        return sprintf(buf, "%u\n", pool->mem_entitlement);
}

static struct kobj_attribute pool_entitlement_attribute = __ATTR(max_mem,0444,pool_entitlement_show,NULL);

static ssize_t pool_ssd_entitlement_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        struct tmem_pool *pool = NULL;
         
        pool = get_pool_from_kobj(kobj);
        if(!pool)
               return -EINVAL;

        return sprintf(buf, "%u\n", pool->ssd_entitlement);
}

static struct kobj_attribute pool_ssd_entitlement_attribute = __ATTR(max_ssd,0444,pool_ssd_entitlement_show,NULL);

static ssize_t pool_used_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        struct tmem_pool *pool = NULL;
         
        pool = get_pool_from_kobj(kobj);
        if(!pool)
               return -EINVAL;

        return sprintf(buf, "%u\n", atomic_read(&pool->used));
}
static struct kobj_attribute pool_used_attribute = __ATTR(mem_used,0444,pool_used_show,NULL);


static ssize_t pool_ssd_used_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        struct tmem_pool *pool = NULL;
         
        pool = get_pool_from_kobj(kobj);
        if(!pool)
               return -EINVAL;

        return sprintf(buf, "%u\n", atomic_read(&pool->ssd_used));
}
static struct kobj_attribute pool_ssd_used_attribute = __ATTR(ssd_used,0444,pool_ssd_used_show,NULL);

static ssize_t pool_stats_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
        struct tmem_pool *pool = NULL;
         
        pool = get_pool_from_kobj(kobj);
        if(!pool)
               return -EINVAL;

        return sprintf(buf, "gets:%u sgets:%u puts:%u flushes:%u evicts:%u\n", pool->total_gets, pool->succ_gets, pool->succ_puts, pool->succ_flushes,pool->evicts);
}
static struct kobj_attribute pool_stats_attribute = __ATTR(stats,0444,pool_stats_show,NULL);

static struct attribute *pool_attrs[] = {
        &pool_weight_attribute.attr,
        &pool_entitlement_attribute.attr,
        &pool_ssd_entitlement_attribute.attr,
        &pool_used_attribute.attr,
        &pool_stats_attribute.attr,
        &pool_ssd_used_attribute.attr,
        NULL,
};
static struct attribute_group pool_attr_group = {
        .attrs = pool_attrs,
};


static int evict_from_pool(struct tmem_pool *pool, int num, bool ssd)
{
   int flushed = 0, ret;
   struct eviction_info *ev = pool->eviction_info;
   atomic_t *used = ssd ? &pool->ssd_used : &pool->used;

   utmemassert(ev);
//   utmemassert(num <= atomic_read(&pool->used));

	while(flushed < num && atomic_read(used)){
        	utmem_pampd *entry;
        	//local_irq_save(flags);
        	if (atomic_read(&pool->obj_count) > 0){
      
			spin_lock(&ev->ev_lock);
			entry = list_first_entry(&ev->head, struct utmem_pampd, entry_list);
			spin_unlock(&ev->ev_lock);

			BUG_ON(!entry); 
			pool->evicts++;
			ret = tmem_flush_page(pool, &entry->tmem_obj->oid, entry->index);
			
			if(!ret)
                            ++flushed; 
        }
      //  local_irq_restore(flags);
  	}

  	if(ssd){
  		printk("Flush occured SSD: %d",flushed);
  	}
   
  	return flushed;     
}


#if 0
static int evict_from_client(struct tmem_client *client, bool ssd)
{
   int poolid, evicted = 0;
   for (poolid = 0; poolid < MAX_POOLS_PER_CLIENT; poolid++){
         struct tmem_pool *pool = client->pools[poolid];
         int pool_used, to_evict = 0;

         if(pool && !ssd && (pool_used = atomic_read(&pool->used)) > pool->mem_entitlement)
              to_evict = MAX(pool_used - pool->mem_entitlement, EVICT_BATCH);
         else if(pool && ssd && (pool_used = atomic_read(&pool->ssd_used)) > pool->ssd_entitlement) 
              to_evict = MAX(pool_used - pool->ssd_entitlement, EVICT_BATCH);
         
         if(to_evict)
               evicted += evict_from_pool(pool, to_evict); 
   }
   return evicted;
}

#endif

/*
	Main controller function for adjusting client allocations 
	TODO: Manipulate this for the algo to accomodate memory and SSD weights together
*/
static int check_and_readjust_allocations(struct tmem_client *client)
{
   int poolid, mem_weight = 100, ssd_weight = 100;
   
   bool adjusted = false;

   for (poolid = 0; poolid < MAX_POOLS_PER_CLIENT; poolid++){
      if(client->pools[poolid] && !client->pools[poolid]->ssd)
           mem_weight -= client->pools[poolid]->weight;
      else if(client->pools[poolid])
            ssd_weight -= client->pools[poolid]->weight; 
           
   }
   
   if(!mem_weight) {
       adjusted = true;
       for (poolid = 0; poolid < MAX_POOLS_PER_CLIENT; poolid++){
           if(client->pools[poolid] && !client->pools[poolid]->ssd)
                   client->pools[poolid]->mem_entitlement = client->pools[poolid]->weight * 
                                                        client->mem_entitlement / 100;
           
      }
   }
   
  if(!ssd_weight) {
       
       adjusted = true;
       for (poolid = 0; poolid < MAX_POOLS_PER_CLIENT; poolid++){
           if(client->pools[poolid] && client->pools[poolid]->ssd)
                   client->pools[poolid]->ssd_entitlement = client->pools[poolid]->weight * 
                                                        client->ssd_entitlement / 100;
           
      }
   }

   return (adjusted ? 0 : -1);
}

#if 0
static inline int pool_overflow(struct tmem_pool *pool)
{
   int pool_used = pool->ssd ? atomic_read(&pool->ssd_used) : atomic_read(&pool->used);
   int entitlement = pool->ssd ? pool->ssd_entitlement : pool->mem_entitlement;

   int to_evict;

   if(entitlement == 0)
          return 0;

   if(pool_used < entitlement)
         return -1;

   to_evict = MAX(pool_used - entitlement, EVICT_BATCH);
   
   if(atomic_inc_and_test(&pool->evicting)){
             evict_from_pool(pool, to_evict);
   }
   atomic_dec(&pool->evicting);
   return 1;
}
#endif

/*
 *	Creation of a new pool
 */
static int utmem_fs_new_pool(struct tmem_client *client, uint32_t flags, int weight)
{
        int poolid = -1;
        struct tmem_pool *pool;
        struct eviction_info *ev;

        pool = kzalloc(sizeof(struct tmem_pool), GFP_ATOMIC);
        if (pool == NULL) {
                utmemdp("utmem: pool creation failed: out of memory\n");
                goto out;
        }

        for (poolid = 0; poolid < MAX_POOLS_PER_CLIENT; poolid++)
                if (client->pools[poolid] == NULL)
                        break;
        
	if (poolid >= MAX_POOLS_PER_CLIENT) {
                utmemdp("utmem: pool creation failed: max exceeded\n");
                kfree(pool);
                poolid = -1;
                goto out;
        }

        atomic_set(&pool->refcount, 0);
        pool->client = client;
        pool->pool_id = poolid;

        pool->weight = weight;
        pool->ssd = false;
        atomic_set(&pool->evicting, -1);
       
        tmem_new_pool(pool, flags);
        client->pools[poolid] = pool;
        check_and_readjust_allocations(client);
        utmemdp("utmem: created %s tmem pool, id=%d, client=%d\n",
                flags & TMEM_POOL_PERSIST ? "persistent" : "ephemeral",
                poolid, client->id);

       	if(likely(client->client_kobj)){
		char name[10];
		sprintf(name,"%u",pool->pool_id);
		pool->pool_kobj = kobject_create_and_add(name, client->client_kobj);  
		utmemassert(pool->pool_kobj); 
		if(sysfs_create_group(pool->pool_kobj, &pool_attr_group))
                	utmemdp("pool: can't create sysfs\n");
	} 
       
	ev = kzalloc(sizeof(struct eviction_info), GFP_KERNEL);
	INIT_LIST_HEAD(&ev->head);
	spin_lock_init(&ev->ev_lock); 
	pool->eviction_info = ev;
out:
	return poolid;
}

static int utmem_fs_destroy_pool(struct tmem_client *client, int pool_id)
{
     struct tmem_pool *pool = NULL;
     int ret = -1;
     struct eviction_info *ev;

     if (pool_id < 0)
                goto out;
     pool = client->pools[pool_id];
     if (pool == NULL)
             goto out;
     /* wait for pool activity on other cpus to quiesce */
     while (atomic_read(&pool->refcount) != 0)
               ;
    
    client->pools[pool_id] = NULL;

    local_bh_disable();
    ret = tmem_destroy_pool(pool);
    local_bh_enable();
    
    sysfs_remove_group(pool->pool_kobj, &pool_attr_group);
    kobject_del(pool->pool_kobj);



     ev = pool->eviction_info;
     utmemassert(ev && list_empty(&ev->head));
     kfree(ev);

    kfree(pool);
    ret = 0;
    check_and_readjust_allocations(client);
    utmemdp("utmem: destroyed pool id=%d, cli_id=%d\n",
                        pool_id, client->id);
out:
        return ret;

}


int utmem_set_pool_weight(struct tmem_client *client, int pool_id, int new_weight, int pool_type)
{
     struct tmem_pool *pool = NULL;
     int ret = -1;
     if (pool_id < 0)
                goto out;
     utmemdp("utmem: change weight pool id=%d, weight=%d\n",
                        pool_id, new_weight);
     pool = client->pools[pool_id];
     if (pool == NULL)
             goto out;
     pool->weight = new_weight;
     
     if(pool_type && !pool->ssd)
             pool->mem_entitlement = 0;
     else if(!pool_type && pool->ssd)
             pool->ssd_entitlement = 0;  

     pool->ssd = pool_type ? true : false;
     ret = check_and_readjust_allocations(client);
     out:
         return ret;
  
}
#if 0
static int client_evict(struct tmem_client *client, int num)
{
   unsigned long flags;
   int flushed = 0, ret;
   struct eviction_info *ev = client->eviction_info;
   

   utmemassert(ev);
   utmemassert(num <= atomic_read(&client->mem_used));

   while(flushed < num && atomic_read(&client->mem_used)){
          utmem_pampd *entry;
          //local_irq_save(flags);

          spin_lock(&ev->ev_lock);
          entry = list_first_entry(&ev->head, struct utmem_pampd, entry_list);
          spin_unlock(&ev->ev_lock);

          BUG_ON(!entry); 
          ret = tmem_flush_page(entry->tmem_obj->pool, &entry->tmem_obj->oid, entry->index);
          if(!ret)
                  ++flushed; 
          //local_irq_restore(flags);
   }   
  return flushed;     
}

static inline int client_overflow(struct tmem_client *client)
{
   int client_mem_used = atomic_read(&client->mem_used);
   int client_ssd_used = atomic_read(&client->ssd_used);
   int to_evict;

   if(client->mem_entitlement == 0 && client->ssd_entitlement == 0)
          return 0;

   if(client_mem_used <= client->mem_entitlement && client_ssd_used <= client->ssd_entitlement)
         return -1;

   if(client_mem_used > client->mem_entitlement){
           to_evict = MAX(client_mem_used - client->mem_entitlement, EVICT_BATCH);
  
           if(atomic_inc_and_test(&client->evicting))
                 client_evict(client, to_evict);
            atomic_dec(&client->evicting);
    }else if(client_ssd_used > client->ssd_entitlement)
           utmemdp("SSD overflow in global mode, not supported\n");

   return 1;
}
        
#endif

static inline int get_overuse(unsigned entitlement, int weight, int used, 
                              unsigned extra, int cuml_weight)
{
    return ((used + EVICT_BATCH) - (entitlement + (extra * weight) / cuml_weight));
}

/*
 *	Evict memory from a targetted client(VM)
 */
static int evict_memory_from_client(struct tmem_client *client)
{
   struct tmem_pool *pools[MAX_POOLS_PER_CLIENT];     
   int cuml_weight = 0;
   int current_max_overuse;
   int count = 0;
   struct tmem_pool *pool;
   unsigned under_utilized_others = 0;
   int i;

   for(i=0; i<MAX_POOLS_PER_CLIENT && client->pools[i]; ++i){
         unsigned used;
         pool = client->pools[i];
         used = atomic_read(&pool->used);

         if(!pool->mem_entitlement && used)
               goto got_pool;
         else if(!pool->mem_entitlement)
                continue;

         if(pool->mem_entitlement < used + EVICT_BATCH){  
             pools[count++] = pool;   // Already over the limits
             cuml_weight += pool->weight;
          }else if(pool->mem_entitlement - used > 2 * EVICT_BATCH){
             under_utilized_others += pool->mem_entitlement - used;
          }
    }
   utmemassert(count);
   utmemassert(cuml_weight);

   if(!count || !cuml_weight)
         return -1;
   
   pool = pools[0];    
   
   if(count > 1)
      current_max_overuse = get_overuse(pool->mem_entitlement, pool->weight, atomic_read(&pool->used), under_utilized_others, cuml_weight);

   for(i=1; i < count; ++i){
         int new_overuse = get_overuse(pools[i]->mem_entitlement, pools[i]->weight,  atomic_read(&pools[i]->used), under_utilized_others, cuml_weight);
         if( new_overuse > current_max_overuse){
             pool = pools[i];     
         }
   }

got_pool:  

  return evict_from_pool(pool, EVICT_BATCH, false);     
}

/*
 *	Routine to choose client for memory eviction
 */
static int utmem_evict_memory(struct global_info *g)
{  
   
    struct tmem_client *cls[MAX_VMS];
    int cuml_weight = 0;
    unsigned current_max_overuse;
    int count = 0;
    struct tmem_client *client;
    unsigned under_utilized_others = 0;
    int i;
     
   
    for(i=0; i<MAX_VMS && g->tmem_clients[i].id >= 0; ++i){
       unsigned used;
       client = &g->tmem_clients[i];
       used = atomic_read(&client->mem_used);

/*This client should not be using any memory*/
       if(!client->mem_entitlement && used)
                goto got_client;     
       else if(!client->mem_entitlement)
                continue;


       if(client->mem_entitlement < used + EVICT_BATCH){  
             cls[count++] = client;   // Already over the limits
             cuml_weight += client->weight;
       }else{
             under_utilized_others += client->mem_entitlement - used;
       }
    }
   
   utmemassert(count && cuml_weight);

   if(!count || !cuml_weight)
         return -1;
   
   client = cls[0];    
   
   if(count > 1)
      current_max_overuse = get_overuse(client->mem_entitlement, client->weight, atomic_read(&client->mem_used), under_utilized_others, cuml_weight);

   for(i=1; i<count; ++i){
         int new_overuse = get_overuse(cls[i]->mem_entitlement, cls[i]->weight,  atomic_read(&cls[i]->mem_used), under_utilized_others, cuml_weight);
         if( new_overuse > current_max_overuse){
             client = cls[i];     
         }
   }

got_client:  

  return evict_memory_from_client(client);     
        
}

/*
 *	Evict SSD-memory from targetted client
 */
static int evict_ssdmem_from_client(struct tmem_client *client)
{
   struct tmem_pool *pools[MAX_POOLS_PER_CLIENT];
   int cuml_weight = 0;
   int current_max_overuse;
   int count = 0;
   struct tmem_pool *pool;
   unsigned under_utilized_others = 0;
   int i;

   for(i=0; i<MAX_POOLS_PER_CLIENT && client->pools[i]; ++i){
         unsigned used;
         pool = client->pools[i];
         used = atomic_read(&pool->ssd_used);

         if(!pool->ssd_entitlement && used)
               goto got_pool;
         else if(!pool->ssd_entitlement)
                continue;

         if(pool->ssd_entitlement < used + EVICT_BATCH){
             pools[count++] = pool;   // Already over the limits
             cuml_weight += pool->weight;
          }else if(pool->ssd_entitlement - used > 2 * EVICT_BATCH){
             under_utilized_others += pool->ssd_entitlement - used;
          }
    }
   utmemassert(count);
   utmemassert(cuml_weight);

   if(!count || !cuml_weight)
         return -1;

   pool = pools[0];

   if(count > 1)
      current_max_overuse = get_overuse(pool->ssd_entitlement, pool->weight, atomic_read(&pool->ssd_used), under_utilized_others, cuml_weight);

   for(i=1; i < count; ++i){
         int new_overuse = get_overuse(pools[i]->ssd_entitlement, pools[i]->weight,  atomic_read(&pools[i]->ssd_used), under_utilized_others, cuml_weight);
         if( new_overuse > current_max_overuse){
             pool = pools[i];
         }
   }

got_pool:
	
   return evict_from_pool(pool, EVICT_BATCH, true);
}


static int utmem_evict_ssd (struct global_info *g)
{
    struct tmem_client *cls[MAX_VMS];
    int cuml_weight = 0;
    unsigned current_max_overuse;
    int count = 0;
    struct tmem_client *client;
    unsigned under_utilized_others = 0;
    int i;
     
   
    for(i=0; i<MAX_VMS && g->tmem_clients[i].id >= 0; ++i){
       unsigned used;
       client = &g->tmem_clients[i];
       used = atomic_read(&client->ssd_used);

/*This client should not be using any memory*/
       if(!client->ssd_entitlement && used)
                goto got_client;     
       else if(!client->ssd_entitlement)
                continue;


       if(client->ssd_entitlement < used + EVICT_BATCH){  
             cls[count++] = client;   // Already over the limits
             cuml_weight += client->weight;
       }else{
             under_utilized_others += client->ssd_entitlement - used;
       }
    }
   
   utmemassert(count && cuml_weight);

   if(!count || !cuml_weight)
         return -1;
   
   client = cls[0];    
   
   if(count > 1)
      current_max_overuse = get_overuse(client->ssd_entitlement, client->weight, atomic_read(&client->ssd_used), under_utilized_others, cuml_weight);

   for(i=1; i<count; ++i){
         int new_overuse = get_overuse(cls[i]->ssd_entitlement, cls[i]->weight,  atomic_read(&cls[i]->ssd_used), under_utilized_others, cuml_weight);
         if( new_overuse > current_max_overuse){
             client = cls[i];     
         }
   }

got_client:  

  return evict_ssdmem_from_client(client);     
   
}

#define THRESH 128

/*
 *	Tmem put entry point
 */
static int utmem_put_page(struct tmem_client *client, int pool_id, struct tmem_oid *oidp, 
				uint32_t index, struct page *page)
{
        struct tmem_pool *pool;
        int ret = -1;
        int entitlement;
        
        client->g->puts++;

        pool = client->pools[pool_id];
        if (unlikely(pool == NULL))
                goto out;
        WARN_ON(client != pool->client);

	/*
		TODO: handle both weights
	*/
        entitlement = pool->ssd ? pool->ssd_entitlement : pool->mem_entitlement;
       
        if(!entitlement)
                goto out;        

	/*
		Work-conserving nature of the tmem-backend that triggers eviction
		only on reaching system limits
	*/
        if(client->g->mem_limit - THRESH < atomic_read(&client->g->mem_used)){
             if(atomic_inc_and_test(&client->g->evicting)){
                       utmem_evict_memory(client->g);
             }
             atomic_dec(&client->g->evicting);
        }
       
       if(pool->ssd && client->g->ssd_limit - THRESH < atomic_read(&client->g->ssd_used)){
               utmem_evict_ssd(client->g);
               goto out;
       }
#if 0
        if(client_overflow(client) == 0)
             goto out;
        if(pool_overflow(pool) == 0)
            goto out;  
#endif

	// local_irq_save(flags);
        ret = tmem_put(pool, oidp, index, (char *)(page),
                                PAGE_SIZE, 0, is_ephemeral(pool));
	// local_irq_restore(flags);
out:
        if(!ret)
            pool->succ_puts++;
        return ret;

}
static int utmem_get_page(struct tmem_client *client, int pool_id, struct tmem_oid *oidp, uint32_t index, struct page *page)
{
        struct tmem_pool *pool;
        int ret = -1;
        size_t size = PAGE_SIZE;
        
        client->g->gets++;
        
        pool = client->pools[pool_id];
        if (unlikely(pool == NULL))
                goto out;
        WARN_ON(client != pool->client);
        pool->total_gets++;

//        local_irq_save(flags);
        if (atomic_read(&pool->obj_count) > 0)
               ret = tmem_get(pool, oidp, index, (char *)(page),
                                        &size, 0, is_ephemeral(pool));
//        local_irq_restore(flags);
out:
       if(!ret)
            pool->succ_gets++;
        return ret;

}

static int utmem_flush_page(struct tmem_client *client, int pool_id,
                                struct tmem_oid *oidp, uint32_t index)
{
        struct tmem_pool *pool;
        int ret = -1;

        client->g->flushes++;
        
        pool = client->pools[pool_id];
        if(!pool)
                 goto out;
        
//        local_irq_save(flags);
        if (atomic_read(&pool->obj_count) > 0){
                        ret = tmem_flush_page(pool, oidp, index);
        }
//        local_irq_restore(flags);
out:
        if(!ret)
            pool->succ_flushes++;
        return ret;
}


/*XXX An object may be present in multiple pools
      So lets check in all
*/
static int utmem_flush_object(struct tmem_client *client, int pool_id,
                                struct tmem_oid *oidp)
{
        struct tmem_pool *pool = NULL;
        int ret = -1, i;

//        local_irq_save(flags);
        if(pool_id < MAX_POOLS_PER_CLIENT)
               pool = client->pools[pool_id];
        else 
               goto nopool;
        
        if(pool && atomic_read(&pool->obj_count) > 0)
                  ret = tmem_flush_object(pool, oidp);
        goto out;
nopool:
        for(i=0; i < MAX_POOLS_PER_CLIENT; ++i){
                pool = client->pools[i];
                if (pool && atomic_read(&pool->obj_count) > 0)
                        ret = tmem_flush_object(pool, oidp);
        }
out:
//        local_irq_restore(flags);
        return ret;
}

#if 0
static int utmem_cg_new_pool(struct tmem_client *client)
{
   return 0;
}

static int utmem_cg_destroy_pool(struct tmem_client *client)
{
   return 0;
}
#endif

static struct page* get_mpage(struct kvm *kvm, unsigned long gmfn)
{
   return gfn_to_page(kvm, gmfn);
}

/*
 *	Hypercall Interface
 */
int utmem_hypercall(struct kvm_tmem_op *op, struct kvm_vcpu *vcpu)
{
	int ret = -1;
	struct tmem_client *client = find_tmem_client(vcpu->kvm, global);

	/* Raises bug only when client is not found and its not a new pool request */
	BUG_ON (!client && op->cmd != TMEM_NEW_POOL && op->cmd != TMEM_NEW_CGPOOL);	 

	if(!client)
	     client = new_tmem_client(vcpu->kvm, global);

	if(!client){
	       printk(KERN_INFO "Client creation error\n");
	       goto out;  
	}
	
	switch(op->cmd){
	
	case TMEM_NEW_POOL:
	case TMEM_NEW_CGPOOL:
			      ret = utmem_fs_new_pool(client, op->u.cnew.flags, op->u.cnew.weight); 
			      break;
			    
	case TMEM_DESTROY_POOL:
	case TMEM_DESTROY_CGPOOL:
			      ret = utmem_fs_destroy_pool(client, op->pool_id); 
			      break;
	case TMEM_SET_POOL_WEIGHT:
			      ret = utmem_set_pool_weight(client, op->pool_id, op->u.cnew.weight, op->u.cnew.flags); 
			      break;
	case TMEM_PUT_PAGE:
			 {
			  struct tmem_oid *oidp = (struct tmem_oid *)(&op->u.gen.oid[0]);
			  struct page *page = NULL;
		          // int64_t start,end;
			  // rdtscll(start);
		          // if(is_invalid_pfn(tmem_op->u.gen.gmfn))
			  // return -EINVAL;
			  page = get_mpage(vcpu->kvm, op->u.gen.gmfn);
			  if (IS_ERR_OR_NULL(page)) {
				printk(KERN_INFO "Invalid page\n");
				return -EINVAL;
			  }

			 
			  WARN_ON(oidp->oid[1]);
			  oidp->oid[1] = (unsigned long) client;
			  ret = utmem_put_page(client, op->pool_id, oidp, op->u.gen.index, page);
			  
			  kvm_release_page_clean(page);
			
			  /*
			  rdtscll(end);
			  if (ret == 0){
				 TPRINT("tmem_sput",end-start);
			  }else{
				TPRINT("tmem_fput",end-start);
			  }
			 */
			  break;
		       }
	case TMEM_GET_PAGE:
			 {
			  struct tmem_oid *oidp = (struct tmem_oid *)(&op->u.gen.oid[0]);
			  struct page *page = NULL;
		      //   int64_t start,end;
		      //   rdtscll(start);
		      //     if(is_invalid_pfn(op->u.gen.gmfn))
		      //          return -EINVAL;
		      //     page = gfn_to_page(vcpu->kvm, tmem_op->u.gen.gmfn);
			  page = get_mpage(vcpu->kvm, op->u.gen.gmfn);
			  if (IS_ERR_OR_NULL(page)){
				printk(KERN_INFO "Invalid page\n");
				return -EINVAL;
			  }
			  if(PageKsm(page)){
			      printk(KERN_INFO "Called on shared page\n");
			      kvm_release_page_clean(page);
			      return -EINVAL;
			  }
			    
			  WARN_ON(oidp->oid[1]);
			  oidp->oid[1] = (unsigned long)client;

			  ret = utmem_get_page(client, op->pool_id, oidp, op->u.gen.index, page);
			  kvm_release_page_dirty(page);
			#if 0
			  rdtscll(end);
			  if (ret == 0){
				 TPRINT("tmem_get",end-start);
			  }else{
				TPRINT("tmem_fget",end-start);
			  }
			#endif
			  break;
			 }
	   case TMEM_FLUSH_PAGE:
			    {
				 struct tmem_oid *oidp = (struct tmem_oid *)(&op->u.gen.oid[0]);
				
			    
				 WARN_ON(oidp->oid[1]);
				 oidp->oid[1] = (unsigned long)client;

				 ret = utmem_flush_page(client, op->pool_id, oidp, op->u.gen.index);
				 break;
			     }
	    case TMEM_FLUSH_OBJECT:
			     {
				 struct tmem_oid *oidp = (struct tmem_oid *)(&op->u.gen.oid[0]);
				
			    
				 WARN_ON(oidp->oid[1]);
				 oidp->oid[1] = (unsigned long)client;

				 ret = utmem_flush_object(client, op->pool_id, oidp);
				 break;
			     }


	default:
		   break;
	}

	out:
    return ret;
}

/*
	Used to destroy VM-State on shutdown
*/
int utmem_vmshutdown(struct kvm *kvm)
{
	struct tmem_pool *pool = NULL;
	int i, ret = 0;
	struct tmem_client *client = find_tmem_client(kvm, global);
	if(!client)
	 return -1;

	for(i=0; i < MAX_POOLS_PER_CLIENT; ++i){
	      pool = client->pools[i];
	      if (pool)
		   ret |= utmem_fs_destroy_pool(client, pool->pool_id);
	}
	
	ret |= destroy_tmem_client(kvm, global);
	return ret;
}

int utmem_init_module(void)
{

        global = alloc_global();

	if(!global)
              return -ENOMEM;
        
        handle_tmem_hcall = utmem_hypercall;       
        handle_vm_destroy = utmem_vmshutdown;

        global->utmem_kobj = kobject_create_and_add("utmem", mm_kobj);
        if(global->utmem_kobj){
              if(sysfs_create_group(global->utmem_kobj, &utmem_attr_group))
                       printk(KERN_INFO "gcache: can't create sysfs\n");
        }else
               printk(KERN_INFO "gcache: can't create sysfs\n");
        
        init_utmem(global);
        return 0;
    }
void utmem_cleanup_module(void)
    {
        handle_tmem_hcall = NULL;       
        handle_vm_destroy = NULL;

        if(global->utmem_kobj){
                  sysfs_remove_group(global->utmem_kobj, &utmem_attr_group);
                  kobject_del(global->utmem_kobj);
        }

        cleanup_utmem(global);

        kfree(global);
    }

module_init(utmem_init_module);
module_exit(utmem_cleanup_module);
MODULE_AUTHOR("debiskms@gmail.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("KSM helper module");

