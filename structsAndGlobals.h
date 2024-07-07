#define INIT_CHOICES_SIZE 4
#define DEFAULT_COMMAND "ps -eo uname,pid,etime,%cpu,%mem,comm=NAME | awk '{print $1, $2, $3, $4, $5, $6}'"
#define DEFAULT_PS_COMMAND "ps -eo uname,pid,etime,%cpu,%mem,comm=NAME"
#define DEFAULT_AWK_COMMAND "awk '{print $1, $2, $3, $4, $5, $6}'"
#define AWK_PRINT "{print $1, $2, $3, $4, $5, $6}"
#define MAX_COMMAND_SIZE 1024
#define BUFF_SIZE 1024
#define LINE_ARR_SIZE 6
#define LINES_ON_SCREEN 20
#define MAX_FILTERS 100
#define NAME_BUFF_SIZE 100
#define INIT_PRINT_Y 2
#define DEFAULT_REFRESH_RATE 60

typedef struct ps_line_t
{
    char *info_arr[LINE_ARR_SIZE];
    struct ps_line_t *next;
} ps_line, *ps_line_ptr;

typedef struct awk_filter_t
{
    int infoIndex;
    char sign[3];
    int value;
} awk_filter, *awk_filter_ptr;

typedef struct pthread_data_t
{
    pthread_cond_t *inputCond;
    pthread_mutex_t *inputLock;
    int *run;
} pthread_data, *pthread_data_ptr;