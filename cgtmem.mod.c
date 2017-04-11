#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x3e2842d7, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xec795900, __VMLINUX_SYMBOL_STR(kmem_cache_destroy) },
	{ 0x2244ef90, __VMLINUX_SYMBOL_STR(fs_bio_set) },
	{ 0x342ddc45, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x30bef6d8, __VMLINUX_SYMBOL_STR(bio_alloc_bioset) },
	{ 0xda3e43d1, __VMLINUX_SYMBOL_STR(_raw_spin_unlock) },
	{ 0x5c8795c3, __VMLINUX_SYMBOL_STR(kvm_release_page_dirty) },
	{ 0xd6ee688f, __VMLINUX_SYMBOL_STR(vmalloc) },
	{ 0x4ece956d, __VMLINUX_SYMBOL_STR(__lock_page) },
	{ 0xa4e10b4c, __VMLINUX_SYMBOL_STR(handle_vm_destroy) },
	{ 0xe0cb0001, __VMLINUX_SYMBOL_STR(kobject_del) },
	{ 0x999e8297, __VMLINUX_SYMBOL_STR(vfree) },
	{ 0x4629334c, __VMLINUX_SYMBOL_STR(__preempt_count) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x76ce14fb, __VMLINUX_SYMBOL_STR(sysfs_remove_group) },
	{ 0xe754075f, __VMLINUX_SYMBOL_STR(kobject_create_and_add) },
	{ 0xde9360ba, __VMLINUX_SYMBOL_STR(totalram_pages) },
	{ 0xd1260bbd, __VMLINUX_SYMBOL_STR(kvm_release_page_clean) },
	{ 0xece784c2, __VMLINUX_SYMBOL_STR(rb_first) },
	{ 0x3c80c06c, __VMLINUX_SYMBOL_STR(kstrtoull) },
	{ 0xfb578fc5, __VMLINUX_SYMBOL_STR(memset) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xe15f42bb, __VMLINUX_SYMBOL_STR(_raw_spin_trylock) },
	{ 0x3c3fce39, __VMLINUX_SYMBOL_STR(__local_bh_enable_ip) },
	{ 0xcbffb12d, __VMLINUX_SYMBOL_STR(sysfs_create_group) },
	{ 0x4c9d28b0, __VMLINUX_SYMBOL_STR(phys_base) },
	{ 0xa1c76e0a, __VMLINUX_SYMBOL_STR(_cond_resched) },
	{ 0x4d9b652b, __VMLINUX_SYMBOL_STR(rb_erase) },
	{ 0xb085a1b1, __VMLINUX_SYMBOL_STR(blkdev_get_by_path) },
	{ 0x59a9adb2, __VMLINUX_SYMBOL_STR(kmem_cache_free) },
	{ 0x16305289, __VMLINUX_SYMBOL_STR(warn_slowpath_null) },
	{ 0x2276db98, __VMLINUX_SYMBOL_STR(kstrtoint) },
	{ 0x24f48558, __VMLINUX_SYMBOL_STR(wait_on_page_bit) },
	{ 0xb9911ace, __VMLINUX_SYMBOL_STR(unlock_page) },
	{ 0xc5b6c961, __VMLINUX_SYMBOL_STR(bio_put) },
	{ 0xe3ed17cb, __VMLINUX_SYMBOL_STR(submit_bio) },
	{ 0x4921e0bc, __VMLINUX_SYMBOL_STR(kmem_cache_alloc) },
	{ 0x28b562fb, __VMLINUX_SYMBOL_STR(__free_pages) },
	{ 0x97f93634, __VMLINUX_SYMBOL_STR(blkdev_put) },
	{ 0x721f9ab5, __VMLINUX_SYMBOL_STR(handle_tmem_hcall) },
	{ 0x93fca811, __VMLINUX_SYMBOL_STR(__get_free_pages) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x63db7751, __VMLINUX_SYMBOL_STR(pv_cpu_ops) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
	{ 0xc56dd881, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0xd52bf1ce, __VMLINUX_SYMBOL_STR(_raw_spin_lock) },
	{ 0xa5526619, __VMLINUX_SYMBOL_STR(rb_insert_color) },
	{ 0x21bdc1b1, __VMLINUX_SYMBOL_STR(kmem_cache_create) },
	{ 0x4302d0eb, __VMLINUX_SYMBOL_STR(free_pages) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x69ad2f20, __VMLINUX_SYMBOL_STR(kstrtouint) },
	{ 0xe8a63b8c, __VMLINUX_SYMBOL_STR(gfn_to_page) },
	{ 0xca9360b5, __VMLINUX_SYMBOL_STR(rb_next) },
	{ 0x76d9fbdf, __VMLINUX_SYMBOL_STR(mm_kobj) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=kvm";


MODULE_INFO(srcversion, "A4E6F72770BBB32302D8743");
