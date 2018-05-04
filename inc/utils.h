#ifndef UTILS_H
#define UTILS_H

//std c lib
#include <stdio.h>
#include <time.h>
#include <string.h>

//system unix/linux
#include <ncurses.h>

//third party
#include "sqlite3.h"

char *format_time_str(char *_fmt, time_t _time, char *str_);

int open_db(const char *_path, sqlite3 **_db);
int compile_sql(sqlite3 *_db, const char *_txt, int _n, unsigned _flags,
        sqlite3_stmt **_stmt, const char **_tail);
int check_for_children(sqlite3 *db, int _parent_id);

#endif //#define UTILS_H
