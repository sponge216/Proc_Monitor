#include "declarations.h"

// Free all nodes in a ps_line_ptr linked list.
int freeAllPsLines(ps_line_ptr line)
{
    while (line)
    {
        ps_line_ptr t = line->next;
        if (freePsLinePtr(line)) // freeing failed.
            return 1;
        line = t;
    }
    return 0;
}

// Free all data members of a ps_line_ptr struct.
int freePsLinePtr(ps_line_ptr line)
{
    for (int i = 0; i < LINE_ARR_SIZE; i++)
    {
        if (line->info_arr[i])
            free(line->info_arr[i]);
    }
    free(line->info_arr);

    return 0;
}
// Allocate memory and return a new ps_line_ptr.
ps_line_ptr getNewPsLinePtr()
{
    ps_line_ptr line = (ps_line_ptr)malloc(sizeof(ps_line));
    if (line == NULL)
        return NULL;

    for (int i = 0; i < LINE_ARR_SIZE; i++)
    {
        line->info_arr[i] = NULL;
    }
    line->next = NULL;
    return line;
}