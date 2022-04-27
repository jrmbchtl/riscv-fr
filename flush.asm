section .TEXT
    GLOBAL flush

flush:
    dcache.iall
    ret