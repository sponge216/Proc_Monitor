#define INIT_CHOICES_SIZE 5                                                                                 // Length of initial menu's choices.
#define DEFAULT_COMMAND "ps -eo uname,pid,etime,%cpu,%mem,comm=NAME | awk '{print $1, $2, $3, $4, $5, $6}'" // Default command used in popen.
#define DEFAULT_PS_COMMAND "ps -eo uname,pid,etime,%cpu,%mem,comm=NAME"
#define DEFAULT_AWK_COMMAND "awk '{print $1, $2, $3, $4, $5, $6}'"
#define DEFAULT_NOTIF_COMMAND "ps -eo pid,comm=NAME | awk '{print $1, $2}'"
#define AWK_PRINT "{print $1, $2, $3, $4, $5, $6}"
#define MAX_COMMAND_SIZE 1024 // Maximum size of a command.
#define BUFF_SIZE 1024        // Size of a general use buffer.
#define LINE_ARR_SIZE 6       // Size of ps_line_t's information array.
#define LINES_ON_SCREEN 20    // Maximum amount of lines on the screen.
#define NAME_BUFF_SIZE 100    // Size of a filter's name buffer.
#define INIT_PRINT_Y 2
#define DEFAULT_REFRESH_RATE 180
#define BUCKET_ARRAY_SIZE 100
#define NOTIF_ARR_SIZE 4
#define RED_THRESHOLD 0.5

char outputCommand[MAX_COMMAND_SIZE] = DEFAULT_COMMAND;
int waitOnInput; // Used to time mainWindow and procWindow's inputs so they dont collide.
float refreshRate;
WINDOW *mainWindow, *procWindow;

/*
Linked list that holds information about a line from the PS command.
*/
typedef struct ps_line_t
{
    char *info_arr[LINE_ARR_SIZE];
    struct ps_line_t *next;
} ps_line, *ps_line_ptr;

/*
Struct that holds information about user's filters.
*/
typedef struct awk_filter_t
{
    int infoIndex;
    char sign[3];
    double value;
} awk_filter, *awk_filter_ptr;

/*
Linked list made to store process notifications.
0 - Status
1 - PID
2 - name
3 - time
*/
typedef struct notif_line_t
{
    char *info_arr[NOTIF_ARR_SIZE];
    struct notif_line_t *next;
} notif_line, *notif_line_ptr;

/*
Struct that's used to hand data over to threads.
*/
typedef struct thread_data_t
{
    pthread_cond_t *inputCond;
    pthread_mutex_t *inputLock;
    int *run;
    int *offset;
    int *lineCount;
} thread_data, *thread_data_ptr;

/*
Node that's used to store processes' information in a hash set.
*/
typedef struct bucket_node_t
{
    unsigned long procNameHashCode;
    long long pid;
    char *procName;
    struct bucket_node_t *next;
} bucket_node, *bucket_node_ptr;
/*
Hash set that's used to store processes' information.
 */
typedef struct hash_set_t
{
    bucket_node_ptr bucketArr[BUCKET_ARRAY_SIZE];

} hash_set, *hash_set_ptr;