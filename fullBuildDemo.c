#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include "structsAndGlobals.h"

/*
0 - all good.
1 - error.
2 - retry.

make windows adjustable.
*/
int createGeneralMenu(char *[], int);
int exitFunction();
int filterMenu();
int refreshRateMenu();
int sortMenu();
int sortProcesses(char *, char);
int parseFilters(char *, char *);
char *mallocAndCopyString(char *, int);

// procmon
WINDOW *createSubWindow(WINDOW *, int, int, int, int);
ps_line_ptr getNewPsLinePtr();
int freePsLinePtr(ps_line_ptr);
int printLines(ps_line_ptr, WINDOW *, int, int, int);
int freeAllLines(ps_line_ptr);
int getLinesForProc(ps_line_ptr, int *);
int addLinesToProc(WINDOW *, int, int, int *);
void createProcWindow(void *);

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
int exitFunction()
{
    return 0;
}

char *mallocAndCopyString(char *src, int maxLen)
{
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

    char *inputCopy = mallocAndCopyString(input, BUFF_SIZE);
    if (inputCopy == NULL)
        return 1;
    char nameBuffer[NAME_BUFF_SIZE] = {0};
    char *token = strtok(inputCopy, ",");
    char *choices[] = {"uname", "pid", "etime", "cpu", "mem", "comm"};
    int infoIndex = -1;
    if (!token) // check for errors.
        return 1;
    // Parse the input string into filters
    while (token != NULL && filterCount < MAX_FILTERS)
    {
        infoIndex = -1;
        sscanf(token, "%[^:]:%[^:]:%d", nameBuffer, filters[filterCount].sign, &filters[filterCount].value);
        for (int j = 0; j < LINE_ARR_SIZE; j++) // info name
        {
            if (!strncmp(nameBuffer, choices[j], strlen(choices[j]))) // Check if the filter's name matches any of the existing names.
            {
                infoIndex = j;
            }
        }
        if (infoIndex == -1)
            return 1;
        filters[filterCount].infoIndex = infoIndex;
        filterCount++;
        token = strtok(NULL, ","); // Get next token.
    }

    free(inputCopy);

    // Construct the AWK command
    strcpy(awkCmd, "awk '");
    for (int i = 0; i < filterCount; i++)
    {
        if (i > 0)
        {
            strcat(awkCmd, "&& ");
        }
        char filterCmd[100];
        snprintf(filterCmd, sizeof(filterCmd), "$%d %s %d ", filters[i].infoIndex, filters[i].sign, filters[i].value);
        strcat(awkCmd, filterCmd);
    }
    strcat(awkCmd, AWK_PRINT);
    strcat(awkCmd, "'");

    // Ensure the awkCmd is null-terminated
    awkCmd[BUFF_SIZE - 1] = '\0';
    return 0;
}

int filterMenu()
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
    if (res == ERR) // Check if scanning user input was successful.
    {
        noecho();
        werase(filterWin);
        delwin(filterWin);
        refresh();
        return 1;
    }

    res = parseFilters(buffer, awkCmd); // Check if parsing was successful
    if (!res)
    {
        noecho();
        werase(filterWin);
        delwin(filterWin);
        refresh();

        return 1;
    }

    wrefresh(filterWin);
    sprintf(outputCommand, "%s | %s", DEFAULT_PS_COMMAND, awkCmd);
    noecho();
    werase(filterWin);
    delwin(filterWin);
    refresh();
    return 0;
}

int refreshRateMenu()
{
    int scrnHeight;
    int scrnWidth;
    float refreshRateBuffer = 0;
    getmaxyx(stdscr, scrnHeight, scrnWidth);
    int height = scrnHeight / 5;
    WINDOW *refreshRateWin = createSubWindow(mainWindow, height, scrnWidth, 0, 0);
    mvwprintw(refreshRateWin, 1, 1, "Enter refresh rate per minute, max 180");
    keypad(refreshRateWin, true);
    wrefresh(refreshRateWin);
    echo();
    int res = mvwscanw(refreshRateWin, 3, 1, "%f", &refreshRateBuffer);
    if (res == ERR)
    {
        noecho();
        werase(refreshRateWin);
        delwin(refreshRateWin);
        refresh();
        return 1;
    }
    refreshRate = refreshRateBuffer / 60;
    wrefresh(refreshRateWin);

    noecho();
    werase(refreshRateWin);
    delwin(refreshRateWin);
    refresh();
    return 0;
}

int sortMenu()
{ // TODO: add checks
    char *choices[8] = {"uname", "pid", "etime", "%cpu", "%mem", "comm", "+", "-"};
    int choicesSize = 8;
    int info_index = createGeneralMenu(choices, choicesSize);
    int sign_index = createGeneralMenu(choices, choicesSize);
    int res = sortProcesses(choices[info_index], choices[sign_index][0]);

    return res;
}

int createGeneralMenu(char *choices[], int choicesSize)
{
    int scrnHeight;
    int scrnWidth;
    getmaxyx(mainWindow, scrnHeight, scrnWidth);
    int height = scrnHeight / 5;
    WINDOW *menuWin = createSubWindow(mainWindow, height, scrnWidth, 0, 0);
    wrefresh(menuWin);
    keypad(menuWin, true);
    int choice;
    int highlight = 0;
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
        highlight = (highlight < 0) ? (highlight + choicesSize) : (highlight);

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
            attroff(COLOR_PAIR(1));
        }
        y++;
        line = line->next;
        currLine++;
    }
    wrefresh(win);
    return 0;
}

int freeAllLines(ps_line_ptr line)
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
    for (int i = 0; i < LINE_ARR_SIZE; i++)
    {
        line->info_arr[i] = NULL;
    }
    line->next = NULL;
    return line;
}

int getLinesForProc(ps_line_ptr temp, int *lineCount)
{

    FILE *fp = popen(outputCommand, "r");
    if (!fp)
    {
        printf("ERROR, popen failed.");
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
            temp->info_arr[currInfoIndex] = (char *)malloc(sizeof(char) * (len + 1));
            strncpy(temp->info_arr[currInfoIndex], lineBuffer, len);
            temp->info_arr[currInfoIndex][len] = '\0';

            len = 0;
            lineBuffIndex = 0;

            if (currInfoIndex == LINE_ARR_SIZE - 1) // check if it's the end of the line.
            {
                (*lineCount)++;
                currInfoIndex = 0;
                temp->next = getNewPsLinePtr();
                temp = temp->next;
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

int addLinesToProc(WINDOW *pw, int height, int offset, int *lineCount) // returns 1 if failed to add lines 0 if it worked
{

    // do lines
    werase(pw);
    box(pw, 0, 0);

    ps_line_ptr lines = getNewPsLinePtr();
    ps_line_ptr temp = lines;

    getLinesForProc(temp, lineCount);

    int scrnHeight, scrnWidth;
    getmaxyx(mainWindow, scrnHeight, scrnWidth);
    char *infoNamesArr[] = {"UNAME", "PID", "ELAPSED TIME", "CPU", "MEM", "NAME"};
    for (int i = 0; i < LINE_ARR_SIZE; i++)
    {
        mvwprintw(pw, INIT_PRINT_Y - 1, i * (scrnWidth / LINE_ARR_SIZE) + 2, "%s\t", infoNamesArr[i]);
    }
    printLines(lines->next, procWindow, offset, scrnWidth, scrnHeight);

    freeAllLines(lines);

    wrefresh(pw);
    return 0;
}

void createProcWindow(void *data)
{
    pthread_data_ptr procData = (pthread_data_ptr)data;
    pthread_cond_t *inputCond = procData->inputCond;
    pthread_mutex_t *inputLock = procData->inputLock;
    int *run = procData->run;

    int scrnHeight;
    int scrnWidth;
    getmaxyx(stdscr, scrnHeight, scrnWidth);

    int height = scrnHeight / 2;
    int offset = 0;
    int lineCount = 0;
    procWindow = createSubWindow(mainWindow, height, scrnWidth, scrnHeight / 2 - 2, 0);

    if (procWindow == NULL)
    {
        fprintf(stderr, "Failed to create proc window...");
        exit(EXIT_FAILURE);
    }

    nodelay(procWindow, TRUE);
    keypad(procWindow, TRUE);
    // proc window created

    struct timespec remaining, request;
    float interval_in_seconds = 60.0 / refreshRate;

    // Separate into whole seconds and fractional seconds
    request.tv_sec = (time_t)interval_in_seconds;
    request.tv_nsec = (long)((interval_in_seconds - request.tv_sec) * 1e9);
    while (*run && !addLinesToProc(procWindow, height, offset, &lineCount))
    {
        int ch = wgetch(stdscr);
        switch (ch)
        {
        case KEY_UP:
            if (offset > 0)
                offset--;
            break;
        case KEY_DOWN:
            if (offset < lineCount - height)
                offset++;
            break;
        }
        ch = wgetch(stdscr);
        if (ch == KEY_RESIZE)
        {
            werase(procWindow);
            delwin(procWindow);
            getmaxyx(mainWindow, scrnHeight, scrnWidth);
            height = scrnHeight / 2;
            procWindow = createSubWindow(mainWindow, height, scrnWidth, scrnHeight / 2, 0);
            refresh();
        }
        if (waitOnInput)
        {
            pthread_mutex_lock(inputLock);
            keypad(procWindow, FALSE);

            pthread_cond_wait(inputCond, inputLock);

            pthread_mutex_unlock(inputLock);
            keypad(procWindow, TRUE);
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
        exit(EXIT_FAILURE);
    }

    noecho();
    start_color(); /* Start color 			*/
    init_pair(1, COLOR_RED, COLOR_BLACK);
    nodelay(mainWindow, TRUE);
    waitOnInput = 0; // Used to time mainWindow and procWindow's inputs so they dont collide.
    refreshRate = DEFAULT_REFRESH_RATE;
    WINDOW *mainWindow, *procWindow;
    int index = 0;
    int res = 0;
    int run = TRUE;
    char *initialMenuChoices[] = {"RefreshRate", "Sort", "Filter", "Exit"};
    int (*menuFunctionsArr[])() = {refreshRateMenu,
                                   sortMenu,
                                   filterMenu,
                                   exitFunction};
    pthread_cond_t inputCond = PTHREAD_COND_INITIALIZER;

    // declaring mutex
    pthread_mutex_t inputLock = PTHREAD_MUTEX_INITIALIZER;
    pthread_t procsThread;
    pthread_data data = {&inputCond, &inputLock, &run};
    pthread_create(&procsThread, NULL, (void *)createProcWindow, (void *)&data);

    while (run)
    {
        index = createGeneralMenu(initialMenuChoices, INIT_CHOICES_SIZE);
        if (index == -1) // if an error occured, retry.
            continue;

        waitOnInput = 1; // tell procwindow to wait.

        res = menuFunctionsArr[index]();

        waitOnInput = 0;                  // tell procwindow to continue.
        pthread_mutex_lock(&inputLock);   // acquire lock.
        pthread_cond_signal(&inputCond);  // signal procwindow to continue.
        pthread_mutex_unlock(&inputLock); // unacquire lock.

        if (index == INIT_CHOICES_SIZE - 1) // If "Exit" was pressed, exit the program.
            run = 0;

        refresh();
    }
    pthread_join(procsThread, NULL);

    endwin();
    return 0;
}
