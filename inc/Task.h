#ifndef TASK_H
#define TASK_H

//std lib
#include <time.h>
#include <string.h>

//system (unix/linux)
#include <ncurses.h>

//3rd party
#include "sqlite3.h"

//homebrew
#include "utils.h"
#include "txt_editor.h"

struct Task {
    char name[80];
    time_t creation_time; //date and time when the task was created
    //TODO rework this or add a var to show when the task was last activated
    //TODO add a variable to show when the task was last modified
    time_t status_time; //when was the status altered
    unsigned id;
    unsigned parent_id;
    unsigned orig_estimate; //original minutes estimated to complete the task
    unsigned estimate; //minutes estimated to complete the task
    unsigned fact; //how much time is already spent on the task
    int status; //task status (e.g. 0 - NIL, 1 - BACKLOG, 2 - NEXT, etc)
    char notes[100];//TODO fixed array only for early DEV version, should be a pointer to dynamic array
    int is_parent;
};

void print_task(char *_fmt, struct Task *_task);
char *task_to_str(struct Task *task, char *str, size_t n);
int print_from_stmt_short(sqlite3_stmt *_stmt, sqlite3* _db);
void task_init_form_stmt(sqlite3_stmt *_stmt, struct Task *_task);

#endif //define TASK_H
