SECTION .TEXT
    GLOBAL flush_dcache

flush_dcache:
    dcache.iall
    ret