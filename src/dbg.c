#include "dbg.h" //header for this implementation file

static FILE *log_file = NULL;

int dbg_init()
{
    log_file = fopen(DBG_FILE_PATH, "w");
    if(log_file == NULL) {return -1;}

    return 0;
}

void dbg_deinit()
{
    if(log_file != NULL) {fclose(log_file);}
}

//void dbg(int _lvl, const char *_txt)
void dbg(const char *_txt)
{
    if(log_file == NULL) {
        printf("can't use dbg log - file not open\n");
        return;
    }
    //if(_lvl > DEBUG) { return; }

    //generate a time stamp
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    char time_buf[100];
    size_t rc = strftime(time_buf, sizeof time_buf,
            "%Y-%m-%d %T", localtime(&ts.tv_sec));
    snprintf(time_buf + rc, sizeof time_buf - rc, ".%06ld", ts.tv_nsec / 1000);

    //fprintf(stderr, "DBG[%s](L%d): %s", time_buf, _lvl, _txt);
    fprintf(log_file, "DBG[%s]: %s", time_buf, _txt);
}

//void dbgf(int _lvl, const char *_fmt, ...)
void dbgf(const char *_fmt, ...)
{
    //if(_lvl > DEBUG) { return; }

    char buf[1024];

    va_list args;
    va_start(args, _fmt);

    vsnprintf(buf, sizeof buf, _fmt, args);
    va_end(args);

    //dbg(_lvl, buf);
    dbg(buf);
}
