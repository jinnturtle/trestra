//c std lib
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define DBG_FILE_PATH "trestra_dbg.log"

void dbg(const char *_txt);
void dbgf(const char *_fmt, ...);
int dbg_init();
void dbg_deinit();
