#include "declarations.h"
/*
This file isn't working as intended and is under development.
*/
notif_line_ptr getNewNotifsLinePtr()
{
    notif_line_ptr line = (notif_line_ptr)malloc(sizeof(notif_line));
    if (line == NULL)
        return NULL;

    for (int i = 0; i < NOTIF_ARR_SIZE; i++)
    {
        line->info_arr[i] = NULL;
    }
    line->next = NULL;
    return line;
}