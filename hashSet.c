#include <stdlib.h>
#include "structsAndGlobals.h"
#define BUCKET_ARRAY_SIZE 100
typedef struct bucket_node_t
{
    unsigned long procNameHashCode;
    int pid;
    struct bucket_node_t *next;
} bucket_node, *bucket_node_ptr;

typedef struct hash_set_t
{
    bucket_node_ptr bucketArr[BUCKET_ARRAY_SIZE];

} hash_set, *hash_set_ptr;

int initHashSet(hash_set_ptr set)
{
    for (int i = 0; i < BUCKET_ARRAY_SIZE; i++)
    {
        set->bucketArr[i] = NULL;
    }
}

unsigned long hashProcName(char *procName) // Hashes a process' name.
{
    unsigned long hash = 5381;
    int c;
    int i = 0;
    while (c = procName[i])
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        i++;
    }

    return hash;
}

int contains(hash_set_ptr set, char *procName, int pid)
{
    unsigned long hash = hashProcName(procName);
    int index = hash % BUCKET_ARRAY_SIZE;
    if (!set->bucketArr[index])
        return 0; // bucket is empty, return false.

    bucket_node_ptr temp = set->bucketArr[index];
    while (temp)
    {
        if (temp->procNameHashCode == hash && temp->pid == pid) // if both names and pid match, the set contains the process.
            return 1;
        temp = temp->next;
    }
    return 0;
}

int addProcToHashSet(hash_set_ptr set, char *procName, int pid)
{
    if (contains(set, procName, pid))
        return 0;

    unsigned long hash = hashProcName(procName);
    int index = hash % BUCKET_ARRAY_SIZE;
    bucket_node_ptr temp = set->bucketArr[index];
    if (!temp) // bucket is empty, create a new one.
    {
        set->bucketArr[index] = (bucket_node_ptr)malloc(sizeof(bucket_node));
        set->bucketArr[index]->next = NULL;
        set->bucketArr[index]->pid = pid;
        set->bucketArr[index]->procNameHashCode = hash;
        return 1;
    }
    while (temp->next) // find the last node in the list.
    {
        temp = temp->next;
    }
    temp->next = (bucket_node_ptr)malloc(sizeof(bucket_node)); // create new node.
    temp = temp->next;
    temp->next = NULL;
    temp->pid = pid;
    temp->procNameHashCode = hash;
    return 1;
}

int removeProcFromHashSet(hash_set_ptr set, char *procName, int pid)
{
    if (contains(set, procName, pid))
        return 0;

    unsigned long hash = hashProcName(procName);
    int index = hash % BUCKET_ARRAY_SIZE;
    bucket_node_ptr temp = set->bucketArr[index];
    bucket_node_ptr behind = temp;

    while (temp)
    {
        if (temp->procNameHashCode == hash && temp->pid == pid) // if both names and pid match, remove the process.
            return 1;
        temp = temp->next;
    }
}
