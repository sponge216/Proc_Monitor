#define INIT_CHOICES_SIZE 4
#define DEFAULT_COMMAND "ps -eo uname,pid,etime,%cpu,%mem,comm=NAME | awk '{print $1, $2, $3, $4, $5, $6}'"
#define DEFAULT_PS_COMMAND "ps -eo uname,pid,etime,%cpu,%mem,comm=NAME"
#define DEFAULT_AWK_COMMAND "awk '{print $1, $2, $3, $4, $5, $6}'"
#define MAX_COMMAND_SIZE 1024
#define BUFF_SIZE 1024
#define LINE_ARR_SIZE 6
#define LINES_ON_SCREEN 20

typedef struct ps_line_t
{
    char *info_arr[LINE_ARR_SIZE];
    struct ps_line_t *next;
} ps_line, *ps_line_ptr;
