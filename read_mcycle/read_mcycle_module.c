// 0xB00

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jorim Bechtle");
MODULE_DESCRIPTION("A simple example Linux module.");
MODULE_VERSION("0.01");

static int __init read_mcycle_module_init(void) {
    uint64_t val = 0;
    asm volatile("csrrs %0, 0x5E2, x0": "=r"(val)::);
    printk(KERN_INFO "0x%lx\n", val);
    return 0;
}

static void __exit read_mcycle_module_exit(void) {
    printk(KERN_INFO "Goodbye, World!\n");
}

module_init(read_mcycle_module_init);
module_exit(read_mcycle_module_exit);