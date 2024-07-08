#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include "structsAndGlobals.h"
#include "./Functions_Dir/declarations.h"
#include "./Functions_Dir/setFunctions.h"
#include "./Functions_Dir/notifsLineFunctions.h"
#include "./Functions_Dir/notificationsFunctions.h"
#include "./Functions_Dir/psLineFunctions.h"
#include "./Functions_Dir/filterFunctions.h"
#include "./Functions_Dir/sortFunctions.h"
#include "./Functions_Dir/refreshRateFunctions.h"

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

int createGeneralMenu(char *choices[], int choicesSize, int *offset, int *lineCount)
{
    int scrnHeight, scrnWidth;
    getmaxyx(mainWindow, scrnHeight, scrnWidth);
    int height = scrnHeight / 5;

    WINDOW *menuWin = createSubWindow(mainWindow, height, scrnWidth, 0, 0); // Create a general purpose window for menus.
    wrefresh(menuWin);
    keypad(menuWin, true); // Enable keyboard input.

    int choice, highlight = 0;
    while (1)
    {
        // Write all options to screen.
        for (int i = 0; i < choicesSize; i++)
        {
            if (i == highlight)              // If the current option should be highlighted.
                wattron(menuWin, A_REVERSE); // Turn on highlight
            mvwprintw(menuWin, 1, 3 + i * (scrnWidth / choicesSize), "%s", choices[i]);
            wattroff(menuWin, A_REVERSE); // Turn off highlight
        }

        wrefresh(menuWin); // Refresh the window so the text is shown.

        choice = wgetch(menuWin); // Get keyboard input.
        switch (choice)
        {
        case KEY_LEFT:
            highlight -= 1;
            break;

        case KEY_RIGHT:
            highlight += 1;
            break;

        case KEY_UP: // Decrease offset by 1 to scroll up the PS lines.
            if (offset[0] > 0)
                (offset[0])--;
            break;

        case KEY_DOWN: // Increase offset by 1 to scroll up the PS lines.
            if (offset[0] < lineCount[0] - height)
                (offset[0])++;
            break;
        case 'w': // Decrease offset by 1 to scroll up the notif lines.
            if (offset[1] > 0)
                (offset[1])--;
            break;
        case 's': // Increase offset by 1 to scroll down the notif lines.
            if (offset[1] < lineCount[1] - height)
                (offset[1])++;
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
        // Handle highlights loop.
        highlight %= choicesSize;
        highlight = (highlight < 0) ? (highlight + choicesSize) : (highlight);

        if (choice == 10) // Check if enter is pressed.
        {
            break;
        }
    }
    // Clear and delete the menu's window.
    werase(menuWin);
    delwin(menuWin);
    refresh();
    // return the last highlighted option
    return highlight;
}

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

    init_pair(3, COLOR_RED, COLOR_BLACK);

    while (line->next && currLine < lastLine)
    {
        ps_line_ptr t = line;
        // Print every part of the process independently, username, PID, ...
        for (int i = 0; i < LINE_ARR_SIZE; i++)
        {
            // Check if is cpu or mem is over certain threshold
            if ((i == 3 || i == 4) && (atof(line->info_arr[i]) >= RED_THRESHOLD))
            {
                wattron(win, COLOR_PAIR(3)); // Color the characters red.
                wrefresh(win);
                mvwprintw(win, y, i * (scrnWidth / LINE_ARR_SIZE) + 2, "%s", line->info_arr[i]);
                wrefresh(win);
                wattroff(win, COLOR_PAIR(3));
            }
            else
            {
                mvwprintw(win, y, i * (scrnWidth / LINE_ARR_SIZE) + 2, "%s", line->info_arr[i]);
            }
            wrefresh(win);
        }
        y++; // Increase the y
        line = line->next;
        currLine++;
    }
    wrefresh(win);
    return 0;
}

int getLinesForProc(ps_line_ptr line, int *lineCount)
{

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
    init_pair(2, COLOR_BLACK, COLOR_GREEN);
    wattron(pw, COLOR_PAIR(2));
    for (int i = 0; i < LINE_ARR_SIZE; i++) // Print the info names.
    {
        mvwprintw(pw, INIT_PRINT_Y - 1, i * (scrnWidth / LINE_ARR_SIZE) + 2, "%s\t", infoNamesArr[i]);
    }
    wattroff(pw, COLOR_PAIR(2));

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
        if (addLinesToProc(procWindow, height, offset, lineCount)) // Create and print the PS lines in procWindow.
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
    start_color();

    waitOnInput = 0; // Used to time mainWindow and procWindow's inputs so they dont collide.
    refreshRate = DEFAULT_REFRESH_RATE;
    WINDOW *mainWindow, *procWindow;
    int index = 0;           // index of choice from menu.
    int res = 0;             // result of operation. Used to check errors.
    int run = 1;             // boolean that is used to terminate the program.
    int countLines[2] = {0}; // array used to keep track of how many lines there are in each screen. 0 - procWindow, 1 - notifWindow.
    int offsets[2] = {0};    //  array used to keep track of the lines offset in each screen. 0 - procWindow, 1 - notifWindow.
    char *initialMenuChoices[] = {"RefreshRate", "Sort", "Filter", "Notifications", "Exit"};
    int (*menuFunctionsArr[])(int *, int *) = {refreshRateMenu,
                                               sortMenu,
                                               filterMenu,
                                               notificationsMenu,
                                               exitFunction};

    pthread_t procsThread;
    pthread_cond_t inputCond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t inputLock = PTHREAD_MUTEX_INITIALIZER;
    thread_data procData = {&inputCond, &inputLock, &run, &offsets[0], &countLines[0]};
    pthread_create(&procsThread, NULL, (void *)createProcWindow, (void *)&procData); // Create a thread for the processes' window.

    pthread_t notifsThread;
    thread_data notifsData = {&inputCond, &inputLock, &run, &offsets[1], &countLines[1]};

    while (run)
    {
        index = createGeneralMenu(initialMenuChoices, INIT_CHOICES_SIZE, offsets, countLines); // Create the initial menu with all the basic options.
        if (index == -1 || index == INIT_CHOICES_SIZE - 1)                                     // If an error occured or the user pressed Exit, close the program.
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
    pthread_mutex_lock(&inputLock);   // acquire lock.
    pthread_cond_signal(&inputCond);  // signal procwindow to continue.
    pthread_mutex_unlock(&inputLock); // unacquire lock.
    pthread_join(procsThread, NULL);  // wait for proc thread to join and then exit safely.

    endwin();
    return 0;
}
