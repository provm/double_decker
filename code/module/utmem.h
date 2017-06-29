#ifndef __UTMEM_H_
#define __UTMEM_H_

#include <linux/module.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/highmem.h>
#include <linux/sched.h>
#include <linux/hugetlb.h>
#include <linux/rmap.h>
#include <linux/kvm_host.h>
#include <linux/tmem_interface.h>
#include <linux/list.h>

#include "tmem.h"

#define MAX_VMS 16
#define MAX_POOLS_PER_CLIENT 32
//#define GLOBAL_EVICT

#define FREE_RAM_THRESH 200 * 256 /* 200MB in number of pages */

#define DBG
#ifdef DBG
#define utmemdp(x...) printk(KERN_INFO x);
#define utmemassert(x) do{ \
	if(unlikely(!(x))) \
	printk(KERN_INFO "Assert failure %s %s:%d\n",__func__, __FILE__, __LINE__); \
}while(0);
#else
#define utmemassert(x) 
#define utmemdp(x...)
#endif

#define UTMEM_GFP_MASK  (__GFP_FS | __GFP_NORETRY | __GFP_NOWARN | __GFP_NOMEMALLOC)

struct global_info;

enum{
	UPTODATE,
	IO_IN_PROGRESS,
};

enum{
	UPDATED,
	MOVE_IN_PROGRESS,
	GET_IN_PROGRESS,
	FLUSH_IN_PROGRESS	
};

enum{
	MEMORY,
	SSD
};

typedef struct utmem_pampd{
	unsigned long page;
	struct tmem_obj *tmem_obj;  
	struct list_head entry_list;
	u32 index;
	volatile short move_status;
	volatile u32 status:2, type;
}utmem_pampd;


struct tmem_client{
	int id;
	int mem_weight;
	int ssd_weight;

	struct global_info *g;
	void *eviction_info;

	struct mm_struct *guest_mm;
	struct tmem_pool *pools[MAX_POOLS_PER_CLIENT];

	unsigned mem_entitlement;
	unsigned ssd_entitlement;

	unsigned long create_time;
	struct kobject *client_kobj;

	long evictions;

	atomic_t num_tmem_objs;
	atomic_t evicting;
	atomic_t ssd_used;
	atomic_t mem_used;
	//atomic_t ssd_uptodate;
};

struct tmem_pool_stats {

        int mem_weight;
        int ssd_weight;

        unsigned mem_entitlement;
        unsigned ssd_entitlement;

        unsigned mem_used;
        unsigned ssd_used;

        unsigned get_requests;

        unsigned mem_puts;
        unsigned ssd_puts;

        unsigned mem_gets;
        unsigned ssd_gets;

        unsigned mem_flushes;
        unsigned ssd_flushes;

        unsigned mem_evicts;
        unsigned ssd_evicts;

        unsigned move_mem_to_ssd;
        unsigned move_ssd_to_mem;
};

struct global_info{

	raw_spinlock_t kvm_tmem_lock;

	int current_num;
	struct tmem_client tmem_clients[MAX_VMS];
	struct kobject *utmem_kobj;

	atomic_t mem_used;
	atomic_t ssd_used;
	atomic_t evicting;

	unsigned mem_limit;
	unsigned ssd_limit;

	unsigned long gets;
	unsigned long puts;
	unsigned long flushes;
	//unsigned long mem_sgets;

	atomic_t pending_async_writes;

	struct block_device *bdev;
	unsigned ssd_bmap_size;
	void *ssd_bmap;

	unsigned next_byte;
	u8 next_bit;
};


struct eviction_info{
	spinlock_t ev_lock;
	struct list_head head;
};

static inline struct global_info * alloc_global(void)
{
	struct global_info *g = kzalloc(sizeof(struct global_info),GFP_KERNEL);
	if(!g)
		return g;
	g->kvm_tmem_lock = __RAW_SPIN_LOCK_UNLOCKED(g->kvm_tmem_lock);
	atomic_set(&g->evicting, -1);
	return g;
}

extern int init_utmem(struct global_info *);
extern int cleanup_utmem(struct global_info *);

#endif
