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
WINDOW *createWindow(int, int, int, int);
ps_line_ptr getNewPsLinePtr();
int freePsLinePtr(ps_line_ptr);
int printLines(ps_line_ptr, WINDOW *, int, int, int);
int freeAllLines(ps_line_ptr);

char outputCommand[MAX_COMMAND_SIZE] = DEFAULT_COMMAND;

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
    int y = 2;
    while (line->next && currLine < lastLine)
    {
        ps_line_ptr t = line;
        for (int i = 0; i < LINE_ARR_SIZE; i++)
        {
            mvwprintw(win, y, i * (scrnWidth / LINE_ARR_SIZE) + 2, "%s", line->info_arr[i]);
        }
        wprintw(win, "\n");
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

WINDOW *createWindow(int height, int width, int startY, int startX)
{
    WINDOW *win = newwin(height, width, startY, startX);
    wclear(win);
    box(win, 0, 0);
    refresh();
    wrefresh(win);
    return win;
}

int main(int argc, char *argv[])
{
    initscr();
    noecho();
    cbreak();
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    int run = 1;
    int scrnHeight;
    int scrnWidth;
    char *infoNamesArr[] = {"UNAME", "PID", "ELAPSED TIME", "CPU", "MEM", "NAME"};

    getmaxyx(stdscr, scrnHeight, scrnWidth);
    int height = scrnHeight / 2;
    WINDOW *procWin = createWindow(height, scrnWidth, 0, 0);
    scrollok(procWin, true);
    nodelay(procWin, TRUE);
    keypad(procWin, TRUE); // Enable special keys input
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

    FILE *fp;
    char fileBuffer[BUFF_SIZE];
    char lineBuffer[BUFF_SIZE];
    int offset = 0;
    while (run)
    {
        wclear(procWin);
        box(procWin, 0, 0);
        wrefresh(procWin);

        fp = popen(outputCommand, "r");
        if (!fp)
        {
            break;
            printf("ERROR, popen failed.");
        }
        ps_line_ptr lines = getNewPsLinePtr();
        // TODO: check for errors on getnewptr
        ps_line_ptr temp = lines;
        int lineCount = 0;
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
                    lineCount++;
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
        int ch = wgetch(procWin);
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
        case KEY_RESIZE:
            getmaxyx(stdscr, scrnHeight, scrnWidth);
            height = scrnHeight / 2;
            delwin(procWin);
            procWin = createWindow(height, scrnWidth, 0, 0);
            break;
        }

        printLines(lines->next, procWin, offset, scrnWidth, scrnHeight);
        freeAllLines(lines);
        for (int i = 0; i < LINE_ARR_SIZE; i++)
        {
            mvwprintw(procWin, 2, i * (scrnWidth / LINE_ARR_SIZE), "%s\t", infoNamesArr[i]);
        }
        pclose(fp);
        struct timespec remaining, request;
        request.tv_nsec = 100000000;
        request.tv_sec = 0;
        nanosleep(&request, &remaining);
    }
    delwin(procWin);
}