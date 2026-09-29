/*
 * Same as dynamic_memory.c, but free(calloc_ptr) is deliberately left out so
 * that valgrind reports a "definitely lost" block. Kept as a separate file so
 * both the clean and the leaking run stay reproducible.
 */
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int i;
    int *malloc_ptr;
    int *calloc_ptr;
    int *temp;

    malloc_ptr = malloc(5 * sizeof(int));
    if (malloc_ptr == NULL)
    {
        printf("malloc() failed\n");
        return 1;
    }
    for (i = 0; i < 5; i++)
        malloc_ptr[i] = (i + 1) * 10;

    calloc_ptr = calloc(5, sizeof(int));
    if (calloc_ptr == NULL)
    {
        printf("calloc() failed\n");
        free(malloc_ptr);
        return 1;
    }

    temp = realloc(malloc_ptr, 10 * sizeof(int));
    if (temp == NULL)
    {
        printf("realloc() failed\n");
        free(malloc_ptr);
        free(calloc_ptr);
        return 1;
    }
    malloc_ptr = temp;
    for (i = 5; i < 10; i++)
        malloc_ptr[i] = (i + 1) * 10;

    free(malloc_ptr);
    malloc_ptr = NULL;

    /* free(calloc_ptr);  <-- deliberately omitted: 20 bytes leak */
    calloc_ptr = NULL;

    printf("Done (calloc block was never freed).\n");
    return 0;
}
