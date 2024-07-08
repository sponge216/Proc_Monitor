#include "declarations.h"

int refreshRateMenu(int *offset, int *lineCount)
{
    int scrnHeight = 0;
    int scrnWidth = 0;
    getmaxyx(stdscr, scrnHeight, scrnWidth);
    int height = scrnHeight / 5;

    float refreshRateBuffer = 0;
    int res = 0;

    WINDOW *refreshRateWin = createSubWindow(mainWindow, height, scrnWidth, 0, 0);
    keypad(refreshRateWin, true); // Enable keyboard input.

    mvwprintw(refreshRateWin, 1, 1, "Enter refresh rate per minute");
    echo(); // Allow user input to appear on the window.
    wrefresh(refreshRateWin);

    res = mvwscanw(refreshRateWin, 3, 1, "%f", &refreshRateBuffer); // Ask user for input.
    if (res == ERR)
    {
        noecho();
        werase(refreshRateWin);
        delwin(refreshRateWin);
        refresh();
        return 1;
    }
    if (refreshRateBuffer > 0) // Validate input.
        refreshRate = refreshRateBuffer;

    noecho(); // Disable user input appearing on the window.
    werase(refreshRateWin);
    delwin(refreshRateWin);
    refresh();
    return 0;
}