SECTION .TEXT
    GLOBAL flush

flush:
    dcache.iall
    ret