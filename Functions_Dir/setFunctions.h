#include "declarations.h"

hash_set_ptr getNewHashSet()
{
    hash_set_ptr set = (hash_set_ptr)malloc(sizeof(hash_set));
    if (set == NULL)
        return NULL;

    for (int i = 0; i < BUCKET_ARRAY_SIZE; i++)
    {
        set->bucketArr[i] = NULL;
    }
    return set;
}

/*
Frees all nodes in a hash set.
*/
int freeHashSet(hash_set_ptr set)
{
    for (int i = 0; i < BUCKET_ARRAY_SIZE; i++)
    {
        if (set->bucketArr[i] != NULL)
        {
            bucket_node_ptr behind = set->bucketArr[i];
            bucket_node_ptr head = set->bucketArr[i]->next;
            while (head)
            {
                free(behind);
                behind = head;
                head = head->next;
            }
            free(behind);
        }
    }
    return 1;
}

unsigned long hashProcName(char *procName) // Hashes a process' name.
{
    unsigned long hash = 5381;
    int c;
    int i = 0;
    while ((c = procName[i]))
    {
        hash = ((hash << 5) + hash) + c; 
        i++;
    }

    return hash;
}

int setContainsByName(hash_set_ptr set, char *procName, long long pid)
{
    unsigned long hash = hashProcName(procName);
    int index = hash % BUCKET_ARRAY_SIZE;
    if (set->bucketArr[index] == NULL)
        return 0; // bucket is empty, return false.

    bucket_node_ptr temp = set->bucketArr[index];
    while (temp)
    {
        if (temp->procNameHashCode == hash && temp->pid == pid) // if both hash and pid match, the set contains the process.
            return 1;
        temp = temp->next;
    }
    return 0;
}

int addProcToSetByName(hash_set_ptr set, char *procName, long long pid)
{
    if (setContainsByName(set, procName, pid))
        return 0;

    unsigned long hash = hashProcName(procName);
    int index = hash % BUCKET_ARRAY_SIZE;
    bucket_node_ptr temp = set->bucketArr[index];
    if (temp == NULL) // bucket is empty, create a new one.
    {
        set->bucketArr[index] = (bucket_node_ptr)malloc(sizeof(bucket_node));
        set->bucketArr[index]->next = NULL;
        set->bucketArr[index]->pid = pid;
        set->bucketArr[index]->procNameHashCode = hash;
        strcpy(set->bucketArr[index]->procName, procName);
        return 1;
    }

    temp = (bucket_node_ptr)malloc(sizeof(bucket_node)); // create a new node and add it to the front of the list.
    temp->next = set->bucketArr[index];
    temp->pid = pid;
    temp->procNameHashCode = hash;
    strncpy(temp->procName, procName, BUFF_SIZE);
    set->bucketArr[index] = temp;
    return 1;
}
/*
Removes and deallocates from a hash set.
*/
int removeProcFromSetByName(hash_set_ptr set, char *procName, long long pid)
{
    if (setContainsByName(set, procName, pid))
        return 0;

    unsigned long hash = hashProcName(procName);
    int index = hash % BUCKET_ARRAY_SIZE;
    if (set->bucketArr[index]->procNameHashCode == hash && set->bucketArr[index]->pid == pid) // in case the first node is the wanted process.
    {
        bucket_node_ptr temp = set->bucketArr[index];
        set->bucketArr[index] = set->bucketArr[index]->next;
        free(temp);
        return 1;
    }
    bucket_node_ptr behind = set->bucketArr[index];
    bucket_node_ptr head = set->bucketArr[index]->next;

    while (head)
    {
        if (head->procNameHashCode == hash && head->pid == pid) // if both names and pid match, remove the process.
        {
            behind->next = head->next;
            free(head->procName);
            free(head);
            return 1;
        }
        head = head->next;
        behind = behind->next;
    }
    return 0;
}

/* Returns a new set that contains all the members in set1 that are not in set2.
 */
hash_set_ptr compareSets(hash_set_ptr set1, hash_set_ptr set2)
{
    int procsCount = 0;
    hash_set_ptr resultSet = getNewHashSet();
    if (resultSet == NULL)
        return NULL;

    for (int i = 0; i < BUCKET_ARRAY_SIZE; i++)
    {
        if (set1->bucketArr[i] == NULL)
            continue;

        bucket_node_ptr temp = set1->bucketArr[i];
        while (temp)
        {
            if (!setContainsByName(set2, temp->procName, temp->pid))
            {
                addProcToSetByName(resultSet, temp->procName, temp->pid);
            }
            temp = temp->next;
        }
    }
    return resultSet;
}

/*
Adds all of set2 to set1.
*/
int addSetToSet(hash_set_ptr set1, hash_set_ptr set2)
{
    for (int i = 0; i < BUCKET_ARRAY_SIZE; i++)
    {
        if (set2->bucketArr[i] == NULL)
            continue;
        bucket_node_ptr temp = set2->bucketArr[i];
        while (temp)
        {
            addProcToSetByName(set1, temp->procName, temp->pid);
            temp = temp->next;
        }
    }
    return 0;
}

/*
Removes all of set2's mutual members from set1.
*/
int removeSetFromSet(hash_set_ptr set1, hash_set_ptr set2)
{
    for (int i = 0; i < BUCKET_ARRAY_SIZE; i++)
    {
        if (set2->bucketArr[i] == NULL)
            continue;
        bucket_node_ptr temp = set2->bucketArr[i];
        while (temp)
        {
            removeProcFromSetByName(set1, temp->procName, temp->pid);
            temp = temp->next;
        }
    }
    return 0;
}

/*
Converts a hash set into a notif_line linked list and returns it.
*/
notif_line_ptr turnSetIntoNotifLines(hash_set_ptr set, char *status)
{
    notif_line_ptr line = getNewNotifsLinePtr();
    if (line == NULL)
        return NULL;

    time_t rawtime;
    struct tm *timeinfo;
    for (int i = 0; i < BUCKET_ARRAY_SIZE; i++)
    {
        if (set->bucketArr[i] == NULL)
            continue;

        bucket_node_ptr temp = set->bucketArr[i];
        while (temp)
        {
            strcpy(line->info_arr[0], status);
            sprintf(line->info_arr[1], "%lld", temp->pid);
            strcpy(line->info_arr[2], temp->procName);

            time(&rawtime);
            timeinfo = localtime(&rawtime);
            sprintf(line->info_arr[3], "%s", asctime(timeinfo));
        }
        temp = temp->next;
    }
    return line;
}