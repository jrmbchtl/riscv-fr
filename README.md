# riscv-fr
This is an example implementation for Flush and Reload on RISC-V, not meant for production.

Facts:
 - The 'rdcycle' instruction has a resolution of 1 cycle, different to newer AMD processors (which have a resolution of 36? cycles).
 - fence.i has issues when used cross thread, gives sub 50% detectioin rate of 1 event on 100 million events.
 - `getconf PAGESIZE` shows that the page size is 4096 bytes, as also commonly used on other systems
 - calling a function does not cause a cache hit with a load instruction, since cache is split into d-cache (used when loading data) and i-cache (used when calling a function)

 Things to try:
 - using evict on data instead of instructions