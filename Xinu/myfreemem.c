/*
 * myfreemem.c - myfreemem
 *
 */

#include <xinu.h>

/*------------------------------------------------------------------------      
 * myfreemem - Cached version of freemem               
 *------------------------------------------------------------------------      
 */
syscall myfreemem(
    char * addr,    /* the client supplied address to free from */ 
    uint32 size)    /* size of the block to free starting from addr */
{
    intmask mask;
    bool8 entry_found = FALSE;  /* whether a matching size list is present */
    uint32 i;
    struct memcache_arr_entry *entry_ptr;

    mask = disable();

    if(size == 0){
        restore(mask);
        return SYSERR;    
    }

    size = (uint32) roundmb(size);  /* round up size to a good value */
    
    entry_ptr = memcache_arr;

    /* search for the size in the arr */
    for(i = 0; i < MEMCACHEMAX; i++) {
        if(entry_ptr->size == size) {
            entry_found = TRUE;
            break;
        }

        entry_ptr++;
    }

    if(entry_found) {
        struct memcache_list_node *b_ptr;
        b_ptr = (struct memcache_list_node *)getmem(
            sizeof(struct memcache_list_node));
        if(b_ptr == (struct memcache_list_node *)SYSERR) {
            restore(mask);
            return SYSERR;
        }

        /* the node's stored memory block is set */
        b_ptr->block = (struct memblk *)addr;

        /* update the list by adding to head */
        b_ptr->next = entry_ptr->list.next;
        entry_ptr->list.next = b_ptr;

        restore(mask);
        return OK;
    }

    else {
        /* if space in arr */
        if(memcache_fill_count < MEMCACHEMAX) {
            memcache_fill_count++;
            struct memcache_list_node *b_ptr = /* block pointer */
                (struct memcache_list_node *)getmem(sizeof(
                    struct memcache_list_node));

            if((uint32)b_ptr == SYSERR){
                restore(mask);
                return SYSERR;
            }

            /* set values for new block */
            b_ptr->next = NULL;
            b_ptr->block = (struct memblk *)addr;

            /* search an available entry */
            entry_ptr = memcache_arr;
            for(i = 0; i < MEMCACHEMAX; i++) {
                if(entry_ptr->size == 0) {
                    break;
                }
                entry_ptr++;
            }

            /* set parameters for the array entry */

            /* add the new block (list node) to the list */
            entry_ptr->list.next = b_ptr;

            entry_ptr->size = size;

            restore(mask);
            return OK;
        }

        else {  /* if no space in arr */
            restore(mask);
            return freemem(addr, size);
        }
    }
}
