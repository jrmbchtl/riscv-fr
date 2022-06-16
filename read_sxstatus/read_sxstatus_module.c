#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jorim Bechtle");
MODULE_DESCRIPTION("A simple RISC-V module reading sxtatus.");
MODULE_VERSION("0.01");

static inline void flush() {
    asm volatile(".word 0x0030000b");
}

static inline uint64_t rdtsc()
{
    uint64_t val;
    asm volatile("rdcycle %0\n"
                 : "=r"(val)::);
    return val;
}

static inline void maccess(void *p)
{
    uint64_t val;
    asm volatile("ld %0, %1\n"
                 : "=r"(val)
                 : "m"(p)
                 :);
}

static inline uint64_t timed_load(void *p)
{
    uint64_t start, end;
    start = rdtsc();
    maccess(p);
    end = rdtsc();
    return end - start;
}

static int __init read_sxstatus_module_init(void) {
    char __attribute__((aligned(4096))) data[4096 * 4];
    memset(data, 0, 4096 * 4);
    uint64_t timing = 0;

    timing = timed_load(&data[0]);
    printk(KERN_INFO "timing: %lu\n", timing);
    flush();
    timing = timed_load(&data[0]);
    printk(KERN_INFO "timing: %lu\n", timing);
    flush();
    timing = timed_load(&data[0]);
    printk(KERN_INFO "timing: %lu\n", timing);
    timing = timed_load(&data[0]);
    printk(KERN_INFO "timing: %lu\n", timing);

    return 0;
}

static void __exit read_sxstatus_module_exit(void) {
    printk(KERN_INFO "Goodbye, World!\n");
}

module_init(read_sxstatus_module_init);
module_exit(read_sxstatus_module_exit);