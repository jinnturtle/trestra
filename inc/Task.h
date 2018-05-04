#ifndef TASK_H
#define TASK_H

#include <time.h>
#include <string.h>

//system (unix/linux)
#include <ncurses.h>

//homebrew
#include "utils.h"

//3rd party
#include "sqlite3.h"

struct Task {
    time_t creation_time; //date and time when the task was created
    time_t status_time; //when was the status altered
    char name[80];
    unsigned id;
    unsigned parent_id;
    unsigned orig_estimate; //original minutes estimated to complete the task
    unsigned estimate; //minutes estimated to complete the task
    unsigned fact; //how much time is already spent on the task
    int status; //task status (e.g. 0 - NIL, 1 - BACKLOG, 2 - NEXT, etc)
};

int print_from_stmt_short(sqlite3_stmt *_stmt, sqlite3* _db);
void print_task(char *_fmt, struct Task *_task);

#endif //define TASK_H
