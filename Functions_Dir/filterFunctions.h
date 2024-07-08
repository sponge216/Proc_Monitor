#include "declarations.h"

#define MAX_FILTERS 100

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
            strcat(awkCmd, "&& "); // Add AND between filters.
        }
        char filterCmd[100];
        snprintf(filterCmd, sizeof(filterCmd), "$%d %s %lf ", filters[i].infoIndex, filters[i].sign, filters[i].value);
        strcat(awkCmd, filterCmd); // Add filter to awk command.
    }
    strcat(awkCmd, AWK_PRINT);
    strcat(awkCmd, "'");

    // Ensure that awkCmd is null-terminated.
    awkCmd[BUFF_SIZE - 1] = '\0';
    return 0;
}

int filterMenu(int *offset, int *lineCount)
{
    int scrnHeight;
    int scrnWidth;
    char buffer[BUFF_SIZE] = {0}; // Buffer used to store user's input.
    char awkCmd[BUFF_SIZE] = {0}; // Buffer used to construct the filter command.
    int res = 0;
    getmaxyx(stdscr, scrnHeight, scrnWidth);
    int height = scrnHeight / 5;
    WINDOW *filterWin = createSubWindow(mainWindow, height, scrnWidth, 0, 0);
    box(filterWin, 0, 0);
    keypad(filterWin, true); // Enable keyboard input.

    mvwprintw(filterWin, 1, 1, "Filter format: name:sign:value. Filters are separated by ','");
    mvwprintw(filterWin, 2, 1, "For example: mem:<:15,cpu:==:3");
    wrefresh(filterWin);
    echo();

    res = mvwscanw(filterWin, 3, 1, "%s", buffer); // Get user input.
    noecho();

    if (res == ERR) // Check if scanning user input was successful.
    {
        werase(filterWin);
        delwin(filterWin);
        refresh();
        return 1;
    }

    res = parseFilters(buffer, awkCmd); // Check if parsing was successful.
    if (res)
    {
        werase(filterWin);
        delwin(filterWin);
        refresh();
        return 1;
    }

    snprintf(outputCommand, MAX_COMMAND_SIZE, "%s | %s", DEFAULT_PS_COMMAND, awkCmd); // Construct the filtered command.
    werase(filterWin);
    delwin(filterWin);
    refresh();
    return 0;
}
