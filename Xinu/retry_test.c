/*  main.c  - main */

#include <xinu.h>

volatile int debugmem = 0;

process	main(void)
{
    /* initialize memory caching */
    mymeminit();

    /* allocate chunks from memory */

    /* used by caching mechanism */
    char *cache_block = getmem(4096);

    /* 
     * would be returned to the system
     * Note: It is still of lesser size then total avaialable in caching
     */
    char *extra_memory = getmem(3072);

    /* take all memory available, but leave a little bit */
    while(getmem(512) != (char *)SYSERR)
        ;

    freemem(extra_memory, 3072);

    /* split the caching mechanism memory into three parts */
    char *block1 = cache_block;
    char *block2 = cache_block + 1024;
    char *block3 = block2 + 2048;

    /* 
     * load the cache with three entries by calling myfreemem on the three
     * blocks
     */
    myfreemem(block1, 1024);
    myfreemem(block2, 2048);
    myfreemem(block3, 1024);

    /* 
     * Important: request a memory so big that it is only available in the
     * caching mechanism. But the memory cannot be fullfilled as it is broken
     * up into parts in the caching mechanism.
     * Caching mechanism would release all the memory and retry (if working
     * correctly).
     */
    char *check = mygetmem(4096);

    /* 
     * expect the returned block to equal the whole block requested by caching
     * memory earlier
     */
    kprintf("expected block 0x%x\n", cache_block);
    if(check == (char*) SYSERR) {
        kprintf("failed to get new block. Test failed\n");
    }
    else {
        kprintf("actual block: 0x%x\n", check);
        check == cache_block ? kprintf("Test passed\n") : kprintf("Test failed");
    }

	return OK;    
}
