#include "declarations.h"

int sortProcesses(char *info, char sign)
{
    return snprintf(outputCommand, MAX_COMMAND_SIZE, "%s%s%c%s | %s", DEFAULT_PS_COMMAND, " --sort ", sign, info, DEFAULT_AWK_COMMAND) >= 0;
}

int sortMenu(int *offset, int *lineCount)
{
    char *choices[8] = {"uname", "pid", "etime", "%cpu", "%mem", "comm", "+", "-"};
    int choicesSize = 8;
    int info_index, sign_index;

    do
    {
        info_index = createGeneralMenu(choices, choicesSize, offset, lineCount);
        sign_index = createGeneralMenu(choices, choicesSize, offset, lineCount);
    } while ((info_index < 0 || info_index > 5) || (sign_index < 6 || sign_index > 7)); // Validate user's input.

    int res = sortProcesses(choices[info_index], choices[sign_index][0]);

    return res;
}
