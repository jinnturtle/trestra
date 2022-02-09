#ifndef UTILS_H
#define UTILS_H

//std c lib
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h> //isdigit(), isanum(), etc

//system unix/linux
#include <stdlib.h>
#include <ncurses.h>

//third party
#include "sqlite3.h"

//homebrew
#include "dbg.h"
#include "config.h"

//time string formating to and fro
char *format_time_str(char *_fmt, time_t _time, char *str_);
time_t htime_to_time(char *_str, size_t _n);
char *time_to_htime(time_t _time, char* str_, size_t _n);

//ncurses utils
void print_mid(int _y, char *_str);

//database ops
int open_db(const char *_path, sqlite3 **_db);
int compile_sql(sqlite3 *_db, const char *_txt, int _n, unsigned _flags,
        sqlite3_stmt **_stmt, const char **_tail);
int check_for_children(sqlite3 *db, int _parent_id);

//other
char* get_db_filepath(void);
int is_backspace(int _key);

#endif //#define UTILS_H
