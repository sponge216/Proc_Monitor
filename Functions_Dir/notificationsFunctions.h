#include "declarations.h"
/*
This file isn't working as intended and is under development.
*/

int printNotifLines(WINDOW *win, notif_line_ptr line, int offset, int scrnWidth, int scrnHeight)
{
    if (line == NULL)
        return 1;

    int currLine = 0;
    int lastLine = LINES_ON_SCREEN;

    while (currLine < offset)
    {
        line = line->next;
        currLine++;
    }
    lastLine += currLine;
    int y = INIT_PRINT_Y;
    lastLine -= y;
    while (line->next && currLine < lastLine)
    {
        notif_line_ptr t = line;
        for (int i = 0; i < NOTIF_ARR_SIZE; i++)
        {
            attron(COLOR_PAIR(1));
            mvwprintw(win, y, i * (scrnWidth / NOTIF_ARR_SIZE) + 2, "%s", line->info_arr[i]);
            wrefresh(win);
            attroff(COLOR_PAIR(1));
        }
        y++;
        line = line->next;
        currLine++;
    }
    wrefresh(win);
    return 0;
}

int getSetsForNotifs(hash_set_ptr genSet, hash_set_ptr newProcsSet, hash_set_ptr oldProcsSet, int *lineCount)
{

    FILE *fp = popen(DEFAULT_NOTIF_COMMAND, "r");
    if (fp == NULL)
    {
        return 1;
    }

    hash_set_ptr newSet = getNewHashSet();
    notif_line_ptr line = getNewNotifsLinePtr();

    char fileBuffer[BUFF_SIZE];
    char lineBuffer[BUFF_SIZE];
    char *procInfoArr[2];
    int procInfoArrSize = 2;
    *lineCount = 0;
    int currInfoIndex = 0;
    int lineBuffIndex = 0;
    int len = 0;

    while (fgets(fileBuffer, sizeof(fileBuffer), fp) != NULL)
    {
        int i = 0;
        while (i < BUFF_SIZE)
        {
            int startIndex = i;
            while (i < BUFF_SIZE && fileBuffer[i] != ' ' && fileBuffer[i] != '\n')
            {
                lineBuffer[lineBuffIndex] = fileBuffer[i];
                i++;
                lineBuffIndex++;
            }
            len += i - startIndex;

            if (i >= BUFF_SIZE)
            { // stopped because of end of read.
                break;
            }
            // dump the line's data into a line struct.
            // TODO: check errors malloc
            procInfoArr[currInfoIndex] = (char *)malloc(sizeof(char) * (len + 1));
            strncpy(procInfoArr[currInfoIndex], lineBuffer, len);
            procInfoArr[currInfoIndex][len] = '\0';

            len = 0;
            lineBuffIndex = 0;

            if (currInfoIndex == procInfoArrSize - 1) // check if it's the end of the line.
            {
                addProcToSetByName(newSet, procInfoArr[1], strtoll(procInfoArr[0], NULL, 10)); // Adding the current process' pid and name to newSet.
                (*lineCount)++;
                currInfoIndex = 0;
                for (int i = 0; i < procInfoArrSize; i++) // freeing line's allocated variables.
                {
                    free(procInfoArr[i]);
                }

                break;
                // TODO: fix it so if a line is bigger than BUFF_SIZE it wont break.
            }
            else
                currInfoIndex++;
            i++;
        }
    }
    newProcsSet = compareSets(newSet, genSet);
    oldProcsSet = compareSets(genSet, newSet);

    freeHashSet(newSet);

    pclose(fp);
    return 0;
}
int addLinesToNotifications(WINDOW *win, hash_set_ptr genSet, notif_line_ptr lines, int height, int *offset, int *lineCount)
{

    // do lines
    werase(win);
    box(win, 0, 0);

    hash_set_ptr newProcsSet; // Set used to hold the new processes.
    hash_set_ptr oldProcsSet; // Set used to hold the processes that stopped.

    int res = getSetsForNotifs(genSet, newProcsSet, oldProcsSet, lineCount); // Get lines to print.
    if (res)                                                                 // check for errors in getting the lines.
    {
        wrefresh(win);
        return 1;
    }

    removeSetFromSet(genSet, oldProcsSet); // Remove the processes that stopped.
    addSetToSet(genSet, newProcsSet);      // Add the processes that started.

    notif_line_ptr temp = lines;
    while (temp->next)
        temp = temp->next;

    temp->next = turnSetIntoNotifLines(oldProcsSet, "STOPPED");

    while (temp->next)
        temp = temp->next;

    temp->next = turnSetIntoNotifLines(newProcsSet, "STARTED");

    int scrnHeight, scrnWidth;
    getmaxyx(mainWindow, scrnHeight, scrnWidth);

    char *infoNamesArr[] = {"STATUS", "PID", "NAME", "TIME"};
    for (int i = 0; i < NOTIF_ARR_SIZE; i++) // Print the info names.
    {
        mvwprintw(win, INIT_PRINT_Y - 1, i * (scrnWidth / NOTIF_ARR_SIZE) + 2, "%s\t", infoNamesArr[i]);
    }

    res = printNotifLines(win, lines, *offset, scrnWidth, scrnHeight); // Print the lines.
    if (res)                                                           // Check if printing was successful.
    {
        wrefresh(win);
        return 1;
    }

    freeHashSet(newProcsSet);
    freeHashSet(oldProcsSet);
    wrefresh(win);
    return 0;
}
void notificationsWindow(void *data)
{
    thread_data_ptr procData = (thread_data_ptr)data;
    pthread_cond_t *inputCond = procData->inputCond;
    pthread_mutex_t *inputLock = procData->inputLock;

    int *run = procData->run;
    int *offset = procData->offset;
    int *lineCount = procData->lineCount;
    int scrnHeight;
    int scrnWidth;
    getmaxyx(stdscr, scrnHeight, scrnWidth);
    int height = scrnHeight / 2 - scrnHeight / 5;

    WINDOW *notifWindow = createSubWindow(mainWindow, height, scrnWidth, scrnHeight / 5 + INIT_PRINT_Y, 0);

    if (notifWindow == NULL)
    {
        fprintf(stderr, "Failed to create proc window...");
        exit(EXIT_FAILURE);
    }
    hash_set_ptr genSet = getNewHashSet();
    notif_line_ptr lines = getNewNotifsLinePtr();
    // proc window created
    struct timespec remaining, request;
    while (*run)
    {
        float interval_in_seconds = 60.0 / refreshRate;

        // Separate into whole seconds and fractional seconds
        request.tv_sec = (time_t)interval_in_seconds;
        request.tv_nsec = (long)((interval_in_seconds - request.tv_sec) * 1e9);
        if (waitOnInput)
        {
            pthread_mutex_lock(inputLock);           // Acquire lock.
            pthread_cond_wait(inputCond, inputLock); // Wait for input to be finished.
            pthread_mutex_unlock(inputLock);         // Unacquire lock.
        }
        if (addLinesToNotifications(notifWindow, genSet, lines, height, offset, lineCount))
        {
            continue;
        }

        nanosleep(&request, &remaining);
    }
    delwin(procWindow);
    pthread_exit(NULL);
}
int notificationsMenu(int *offset, int *lineCount)
{
    char *choices[] = {"Enable", "Disable"};
    int choicesSize = 2;
    int choiceIndex = createGeneralMenu(choices, choicesSize, offset, lineCount);
    if (choiceIndex == -1)
        return 1;
    if (choiceIndex == 1)
    {
        // enable
    }
    else if (choiceIndex == 2)
    {
        // disable
    }
    return 0;
}