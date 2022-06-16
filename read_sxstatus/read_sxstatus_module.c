#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jorim Bechtle");
MODULE_DESCRIPTION("A simple RISC-V module reading sxtatus.");
MODULE_VERSION("0.01");


static int __init read_sxstatus_module_init(void) {
    asm volatile(".word 0x0030000b");

    return 0;
}

static void __exit read_sxstatus_module_exit(void) {
    printk(KERN_INFO "Goodbye, World!\n");
}

module_init(read_sxstatus_module_init);
module_exit(read_sxstatus_module_exit);