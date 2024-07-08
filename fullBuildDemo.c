#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include "structsAndGlobals.h"
notif_line_ptr getNewNotifsLinePtr();
#include "setFunctions.h"

int createGeneralMenu(char *[], int, int *, int *);
int exitFunction(int *, int *);
int notificationsMenu(int *, int *);
void notificationsWindow(void *);
int filterMenu(int *offset, int *lineCount);
int refreshRateMenu(int *offset, int *lineCount);
int sortMenu(int *offset, int *lineCount);
int sortProcesses(char *, char);
int parseFilters(char *, char *);
char *mallocAndCopyString(char *, int);

// procmon
WINDOW *createSubWindow(WINDOW *, int, int, int, int);
ps_line_ptr getNewPsLinePtr();
int freePsLinePtr(ps_line_ptr);
int printLines(ps_line_ptr, WINDOW *, int, int, int);
int freeAllPsLines(ps_line_ptr);
int getLinesForProc(ps_line_ptr, int *);
int addLinesToProc(WINDOW *, int, int *, int *);
void createProcWindow(void *);

int printNotifLines(WINDOW *, notif_line_ptr, int, int, int);
notif_line_ptr getNewNotifsLinePtr();
int getSetsForNotifs(hash_set_ptr, hash_set_ptr, hash_set_ptr, int *);
int addLinesToNotifications(WINDOW *, hash_set_ptr, notif_line_ptr, int, int *, int *);

char outputCommand[MAX_COMMAND_SIZE] = DEFAULT_COMMAND;
int waitOnInput; // Used to time mainWindow and procWindow's inputs so they dont collide.
float refreshRate;
WINDOW *mainWindow, *procWindow;

WINDOW *createSubWindow(WINDOW *window, int height, int width, int startY, int startX)
{
    WINDOW *win = subwin(window, height, width, startY, startX);
    wclear(win);
    box(win, 0, 0);
    refresh();
    return win;
}
int exitFunction(int *offset, int *lineCount)
{
    return 0;
}
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

    hash_set_ptr newProcsSet;
    hash_set_ptr oldProcsSet;

    int res = getSetsForNotifs(genSet, newProcsSet, oldProcsSet, lineCount); // Get lines to print.
    if (res)                                                                 // check for errors in getting the lines.
    {
        wrefresh(win);
        return 1;
    }

    removeSetFromSet(genSet, oldProcsSet);
    addSetToSet(genSet, newProcsSet);

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
char *mallocAndCopyString(char *src, int maxLen)
{
    if (src == NULL)
        return NULL;

    int len = 0;
    while (len < maxLen && src[len])
    {
        len++;
    }
    if (len >= maxLen || len == 0)
        return NULL;
    char *newCopy = (char *)malloc(len * sizeof(char));
    while (len >= 0)
    {
        newCopy[len] = src[len];
        len--;
    }
    return newCopy;
}
int parseFilters(char *input, char *awkCmd)
{
    awk_filter filters[MAX_FILTERS];
    int filterCount = 0;

    if (input == NULL)
        return 1;

    char *token = strtok(input, ",");
    if (token == NULL) // check for errors.
        return 1;

    char nameBuffer[NAME_BUFF_SIZE] = {0};
    char *choices[] = {"uname", "pid", "etime", "cpu", "mem", "comm"};
    int infoIndex = -1;

    // Parse the input string into filters
    while (token != NULL && filterCount < MAX_FILTERS)
    {
        infoIndex = -1;
        sscanf(token, "%[^:]:%[^:]:%lf", nameBuffer, filters[filterCount].sign, &filters[filterCount].value);
        for (int j = 0; j < LINE_ARR_SIZE; j++) // info name
        {
            if (!strncmp(nameBuffer, choices[j], strlen(choices[j]))) // Check if the filter's name matches any of the existing names.
            {
                infoIndex = j + 1;
            }
        }
        if (infoIndex == -1)
            return 1;
        filters[filterCount].infoIndex = infoIndex;
        filterCount++;
        token = strtok(NULL, ","); // Get next token.
    }

    // Construct the AWK command
    strcpy(awkCmd, "awk '");
    for (int i = 0; i < filterCount; i++)
    {
        if (i > 0)
        {
            strcat(awkCmd, "&& ");
        }
        char filterCmd[100];
        snprintf(filterCmd, sizeof(filterCmd), "$%d %s %lf ", filters[i].infoIndex, filters[i].sign, filters[i].value);
        strcat(awkCmd, filterCmd);
    }
    strcat(awkCmd, AWK_PRINT);
    strcat(awkCmd, "'");

    // Ensure the awkCmd is null-terminated
    awkCmd[BUFF_SIZE - 1] = '\0';
    return 0;
}

int filterMenu(int *offset, int *lineCount)
{
    int scrnHeight;
    int scrnWidth;
    char buffer[BUFF_SIZE] = {0};
    char awkCmd[BUFF_SIZE] = {0};
    int res = 0;
    getmaxyx(stdscr, scrnHeight, scrnWidth);
    int height = scrnHeight / 5;
    WINDOW *filterWin = createSubWindow(mainWindow, height, scrnWidth, 0, 0);
    box(filterWin, 0, 0);
    keypad(filterWin, true);

    mvwprintw(filterWin, 1, 1, "Filter format: name:sign:value. Filters are separated by ','");
    mvwprintw(filterWin, 2, 1, "For example: mem:<:15,cpu:==:3");
    wrefresh(filterWin);
    echo();

    res = mvwscanw(filterWin, 3, 1, "%s", buffer);
    noecho();

    if (res == ERR) // Check if scanning user input was successful.
    {
        werase(filterWin);
        delwin(filterWin);
        refresh();
        return 1;
    }

    res = parseFilters(buffer, awkCmd); // Check if parsing was successful
    if (res)
    {
        werase(filterWin);
        delwin(filterWin);
        refresh();
        return 1;
    }

    snprintf(outputCommand, MAX_COMMAND_SIZE, "%s | %s", DEFAULT_PS_COMMAND, awkCmd);
    mvwprintw(filterWin, 1, 1, "%s", outputCommand);
    wrefresh(filterWin);
    werase(filterWin);
    delwin(filterWin);
    refresh();
    return 0;
}

int refreshRateMenu(int *offset, int *lineCount)
{
    int scrnHeight = 0;
    int scrnWidth = 0;
    getmaxyx(stdscr, scrnHeight, scrnWidth);
    int height = scrnHeight / 5;

    float refreshRateBuffer = 0;
    int res = 0;

    WINDOW *refreshRateWin = createSubWindow(mainWindow, height, scrnWidth, 0, 0);
    keypad(refreshRateWin, true);

    mvwprintw(refreshRateWin, 1, 1, "Enter refresh rate per minute");
    echo();
    wrefresh(refreshRateWin);

    res = mvwscanw(refreshRateWin, 3, 1, "%f", &refreshRateBuffer);
    if (res == ERR)
    {
        noecho();
        werase(refreshRateWin);
        delwin(refreshRateWin);
        refresh();
        return 1;
    }

    refreshRate = refreshRateBuffer;

    noecho();
    werase(refreshRateWin);
    delwin(refreshRateWin);
    refresh();
    return 0;
}

int sortMenu(int *offset, int *lineCount)
{ // TODO: add checks
    char *choices[8] = {"uname", "pid", "etime", "%cpu", "%mem", "comm", "+", "-"};
    int choicesSize = 8;
    int info_index = createGeneralMenu(choices, choicesSize, offset, lineCount);
    int sign_index = createGeneralMenu(choices, choicesSize, offset, lineCount);
    int res = sortProcesses(choices[info_index], choices[sign_index][0]);

    return res;
}

int createGeneralMenu(char *choices[], int choicesSize, int *offset, int *lineCount)
{
    int scrnHeight, scrnWidth;
    getmaxyx(mainWindow, scrnHeight, scrnWidth);
    int height = scrnHeight / 5;

    // create the window at the top left of the screen, with a height of 1/
    WINDOW *menuWin = createSubWindow(mainWindow, height, scrnWidth, 0, 0);
    // make it visible
    wrefresh(menuWin);
    // enable input
    keypad(menuWin, true);

    int choice, highlight = 0;
    while (1)
    {
        for (int i = 0; i < choicesSize; i++)
        {
            if (i == highlight)
                wattron(menuWin, A_REVERSE);
            mvwprintw(menuWin, 1, 3 + i * (scrnWidth / choicesSize), "%s", choices[i]);
            wattroff(menuWin, A_REVERSE);
        }
        wrefresh(menuWin);
        choice = wgetch(menuWin);
        switch (choice)
        {
        case KEY_LEFT:
            highlight -= 1;
            break;

        case KEY_RIGHT:
            highlight += 1;
            break;

        case KEY_UP:
            if (offset[0] > 0)
                (offset[0])--;
            break;

        case KEY_DOWN:
            if (offset[0] < lineCount[0] - height)
                (offset[0])++;
            break;

        case KEY_RESIZE:
            getmaxyx(stdscr, scrnHeight, scrnWidth);
            height = scrnHeight / 5;
            werase(menuWin);
            delwin(menuWin);
            refresh();
            menuWin = createSubWindow(mainWindow, height, scrnWidth, 0, 0);
            break;

        default:
            break;
        }
        highlight %= choicesSize;
        highlight = (highlight < 0) ? (highlight + choicesSize) : (highlight); // If the highlighted index is negative, change it to the end of choices.

        if (choice == 10) // check if enter is pressed
        {
            break;
        }
    }
    werase(menuWin);
    delwin(menuWin);
    refresh();
    return highlight;
}

int sortProcesses(char *info, char sign)
{
    // TODO: check errors
    snprintf(outputCommand, MAX_COMMAND_SIZE, "%s%s%c%s | %s", DEFAULT_PS_COMMAND, " --sort ", sign, info, DEFAULT_AWK_COMMAND);
    return 0;
}

/*
procmon
*/

int printLines(ps_line_ptr line, WINDOW *win, int offset, int scrnWidth, int scrnHeight)
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
        ps_line_ptr t = line;
        for (int i = 0; i < LINE_ARR_SIZE; i++)
        {
            attron(COLOR_PAIR(1));
            mvwprintw(win, y, i * (scrnWidth / LINE_ARR_SIZE) + 2, "%s", line->info_arr[i]);
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

ps_line_ptr getNewPsLinePtr()
{ // TODO: check errors malloc
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

int getLinesForProc(ps_line_ptr line, int *lineCount)
{
    hash_set_ptr genSet = getNewHashSet();
    hash_set_ptr newSet = getNewHashSet();
    if (waitOnInput)
        return 1;

    FILE *fp = popen(outputCommand, "r");
    if (fp == NULL)
    {
        return 1;
    }

    char fileBuffer[BUFF_SIZE];
    char lineBuffer[BUFF_SIZE];
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
            line->info_arr[currInfoIndex] = (char *)malloc(sizeof(char) * (len + 1));
            strncpy(line->info_arr[currInfoIndex], lineBuffer, len);
            line->info_arr[currInfoIndex][len] = '\0';

            len = 0;
            lineBuffIndex = 0;

            if (currInfoIndex == LINE_ARR_SIZE - 1) // check if it's the end of the line.
            {
                (*lineCount)++;
                currInfoIndex = 0;
                line->next = getNewPsLinePtr();
                line = line->next;
                break;
                // TODO: fix it so if a line is bigger than BUFF_SIZE it wont break.
            }
            else
                currInfoIndex++;
            i++;
        }
    }

    pclose(fp);
    return 0;
}

int addLinesToProc(WINDOW *pw, int height, int *offset, int *lineCount)
{

    // do lines
    werase(pw);
    box(pw, 0, 0);

    ps_line_ptr lines = getNewPsLinePtr();
    ps_line_ptr temp = lines;

    int res = getLinesForProc(temp, lineCount); // Get lines to print.
    if (res)                                    // check for errors in getting the lines.
    {
        wrefresh(pw);
        freeAllPsLines(lines); // Free all the allocated lines.
        return 1;
    }

    int scrnHeight, scrnWidth;
    getmaxyx(mainWindow, scrnHeight, scrnWidth);

    char *infoNamesArr[] = {"USER", "PID", "ELAPSED TIME", "%CPU", "%MEM", "NAME"};
    for (int i = 0; i < LINE_ARR_SIZE; i++) // Print the info names.
    {
        mvwprintw(pw, INIT_PRINT_Y - 1, i * (scrnWidth / LINE_ARR_SIZE) + 2, "%s\t", infoNamesArr[i]);
    }

    if (!strcmp(lines->info_arr[0], infoNamesArr[0])) // Check if the headers already exist in the lines.
    {
        temp = lines->next; // If they do, skip the first line.
    }
    else
    {
        temp = lines;
    }

    res = printLines(temp, pw, *offset, scrnWidth, scrnHeight); // Print the lines.
    if (res)                                                    // Check if printing was successful.
    {
        wrefresh(pw);
        freeAllPsLines(lines); // Free all the allocated lines.
        return 1;
    }

    freeAllPsLines(lines); // Free all the allocated lines.
    wrefresh(pw);
    return 0;
}

void createProcWindow(void *data)
{
    thread_data_ptr procData = (thread_data_ptr)data;
    pthread_cond_t *inputCond = procData->inputCond;
    pthread_mutex_t *inputLock = procData->inputLock;

    int *run = procData->run;
    int *offset = procData->offset;
    int *lineCount = procData->lineCount;
    int scrnHeight, scrnWidth;
    getmaxyx(stdscr, scrnHeight, scrnWidth);
    int height = scrnHeight / 2;
    procWindow = createSubWindow(mainWindow, height, scrnWidth, scrnHeight / 2 - 2, 0);

    if (procWindow == NULL)
    {
        fprintf(stderr, "Failed to create proc window...");
        exit(EXIT_FAILURE);
    }

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
        if (addLinesToProc(procWindow, height, offset, lineCount))
        {
            continue;
        }

        nanosleep(&request, &remaining);
    }
    delwin(procWindow);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    mainWindow = initscr();

    if (mainWindow == NULL)
    {
        fprintf(stderr, "Failed to initialize curses...\n");
        exit(1);
    }

    noecho();
    start_color(); /* Start color 			*/
    init_pair(1, COLOR_RED, COLOR_BLACK);

    waitOnInput = 0; // Used to time mainWindow and procWindow's inputs so they dont collide.
    refreshRate = DEFAULT_REFRESH_RATE;
    WINDOW *mainWindow, *procWindow;
    int index = 0;
    int res = 0;
    int run = 1;
    int countLines[2] = {0}; // array used to keep track of how many lines there are in each screen. 0 - procWindow, 1 - notifWindow.
    int offsets[2] = {0};    //  array used to keep track of the linees offset in each screen. 0 - procWindow, 1 - notifWindow.
    char *initialMenuChoices[] = {"RefreshRate", "Sort", "Filter", "Notifications", "Exit"};
    int (*menuFunctionsArr[])(int *, int *) = {refreshRateMenu,
                                               sortMenu,
                                               filterMenu,
                                               notificationsMenu,
                                               exitFunction};

    pthread_t procsThread;
    pthread_cond_t inputCond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t inputLock = PTHREAD_MUTEX_INITIALIZER;
    thread_data data = {&inputCond, &inputLock, &run, &offsets[0], &countLines[0]};
    pthread_create(&procsThread, NULL, (void *)createProcWindow, (void *)&data);

    while (run)
    {
        index = createGeneralMenu(initialMenuChoices, INIT_CHOICES_SIZE, offsets, countLines);
        if (index == -1 || index == INIT_CHOICES_SIZE - 1) // If an error occured or the user pressed Exit, close the program.
        {
            run = 0;
        }

        waitOnInput = 1; // tell procwindow to wait.

        res = menuFunctionsArr[index](offsets, countLines); // Commit the user's requested action.

        pthread_mutex_lock(&inputLock);   // acquire lock.
        pthread_cond_signal(&inputCond);  // signal procwindow to continue.
        pthread_mutex_unlock(&inputLock); // unacquire lock.
        waitOnInput = 0;                  // tell procwindow to continue.

        refresh();
    }
    pthread_join(procsThread, NULL);

    endwin();
    return 0;
}
