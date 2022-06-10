// 0xB00

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE(“GPL”);
MODULE_AUTHOR(“Jorim Bechtle”);
MODULE_DESCRIPTION(“A simple example Linux module.”);
MODULE_VERSION(“0.01”);

static int __init read_mcycle_module_init(void) {
    printk(KERN_INFO "Hello, World!\n");
    return 0;
}

static void __exit read_mcycle_module_exit(void) {
    printk(KERN_INFO "Goodbye, World!\n");
}

module_init(read_mcycle_module_init);
module_exit(read_mcycle_module_exit);