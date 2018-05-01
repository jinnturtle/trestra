//std c lib
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h> //atoi(), etc

//system (unix/linux)
#include <ncurses.h>
#include <unistd.h>

//homebrew
#include "Task.h"

//3rd party
#include "sqlite3.h"

#define PROGRAM_NAME "trestra - time resource tracker"
#define PROGRAM_VERSION "v0.0.1"

int print_tasks(void);
int activate_task(void);
void main_menu(void);
int nc_inp(int _y, int _x, const char *_prompt, char *str_, unsigned _n);
int update_task(struct Task *_task);
int init_nc(void);

/*TODO
 * * [ ] print time format "XXXXXhXXm"
 * * [ ] creating tasks
 * * [ ] nesting tasks (subtasks) via assigning a parent id
 * ** [ ] e.g. show_task menu option to display children
 * ** [ ] indicate tasks that have children when printing
 * ** [ ] add children fact and estimate times to parent when creating/modifying children
 * * [ ] modifying tasks
 */

int main(void)
{
    init_nc();

    main_menu();

    endwin(); //end ncurses mode
    return 0;
}

void main_menu(void)
{
    int cmd = 0;

    while(cmd != 'q') {
        clear();
        mvprintw(0,0, "*** %s %s ***\n", PROGRAM_NAME, PROGRAM_VERSION);
        printw("p - print tasks\n");
        printw("a - activate task\n");
        printw("q - quit\n");

        cmd = getch();
        switch(cmd) {
        case 'p': print_tasks(); break;
        case 'a': activate_task(); break;
        case 'q': continue;
        default: mvprintw(22,0, "invalid selection (%c)", cmd); break;
        }
    }
}

int print_tasks(void)
{
    clear();

    char db_path[] = "dat/db.db";
    sqlite3 *db;

    if(sqlite3_open(db_path, &db) != SQLITE_OK) {
        mvprintw(0,0, "Error opening database: %s\n", sqlite3_errmsg(db));
        getch();
        sqlite3_close(db);
        return 1;
    }

    char sql_txt[] = "select * from tasks";
    sqlite3_stmt *stmt;

    if(sqlite3_prepare_v3(db, sql_txt, strlen(sql_txt), 0, &stmt, NULL)
       != SQLITE_OK)
    {
        mvprintw(0,0, "error while compiling sql: %s\n", sqlite3_errmsg(db));
        getch();
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }

    move(0,0);
    int rc = 0;
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        char name[64];
        strcpy(name, sqlite3_column_text(stmt, 3));
        int estimate = sqlite3_column_int(stmt, 4);
        int fact = sqlite3_column_int(stmt, 5);

        double progress_perc = (100.0d / estimate) * fact;
        printw("id: %d task: \"%s\" progress: %lu/%lu(%0.2lf%)\n",
                id, name, fact, estimate, progress_perc);
    }
    if(rc != SQLITE_DONE) {
        mvprintw(0,0, "error querying the db: %s\n", sqlite3_errmsg(db));
        printw("retcode = %d\n", rc);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    getch();
    return 0;
}

int find_task(int _id, struct Task *task_)
{
    if(task_ == NULL) { return -1; }

    char db_path[] = "dat/db.db";
    sqlite3 *db;

    if(sqlite3_open(db_path, &db) != SQLITE_OK) {
        mvprintw(0,0, "Error opening database: %s\n", sqlite3_errmsg(db));
        getch();
        sqlite3_close(db);
        return 1;
    }

    char sql_txt[] = "select * from tasks where id = ? limit 1";
    sqlite3_stmt *stmt;

    if(sqlite3_prepare_v3(db, sql_txt, strlen(sql_txt), 0, &stmt, NULL)
       != SQLITE_OK)
    {
        mvprintw(0,0, "error while compiling sql: %s\n", sqlite3_errmsg(db));
        getch();
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, _id);

    int rc = 0;
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        task_->id = sqlite3_column_int(stmt, 0);
        strcpy(task_->name, sqlite3_column_text(stmt, 3));
        task_->estimate = sqlite3_column_int(stmt, 4);
        task_->fact = sqlite3_column_int(stmt, 5);
    }
    if(rc != SQLITE_DONE) {
        mvprintw(0,0, "error querying the db: %s\n", sqlite3_errmsg(db));
        printw("retcode = %d\n", rc);
        return -1;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int activate_task(void)
{
    struct Task task = { 0 };
    char strbuf[10] = { 0 };
    nc_inp(10, 10, "task id: ", strbuf, sizeof strbuf);

    find_task(atoi(strbuf), &task);
    if(task.id == 0) {
        clear();
        mvprintw(0,0, "could not find task! (press any key)");
        getch();
        return -1;
    };

    clear();

    mvprintw(0,0, "active task: \"%s\" (%d)", task.name, task.id);

    time_t s_tm = time(NULL); //start time

    halfdelay(1); //setting getch to wait for a limited ammount of time

    time_t elapsed = 0;
    int cmd = 0;
    while(cmd != 'q') {
        elapsed = time(NULL) - s_tm;
        mvprintw(22,0, "session: %ld, total: %ld\n",
                elapsed, task.fact + elapsed);

        cmd = getch();
    }
    cbreak(); //unlimiting getch wait time

    task.fact += elapsed; //add elapsed time
    update_task(&task);

    return 0;
}

int nc_inp(int _y, int _x, const char *_prompt, char *str_, unsigned _n)
{
    if(str_ == NULL) { return -1; }

    int pos = 0;

    int cmd = 0;
    while(cmd != '\n') {
        move(_y, _x);
        if(_prompt != NULL) { printw("%s", _prompt); }

        printw("%s", str_);
        cmd = getch();

        if(strlen(str_) >= _n - 1) {
            mvprintw(_y + 1, _x, "max input length reached!");
            continue;
        }

        switch(cmd) {
        case '\n': break;
        default:
            str_[pos] = cmd;
            ++pos;
        }
    }

    return 0;
}

int update_task(struct Task *_task)
{
    if(_task == NULL) { return -1; }

    char db_path[] = "dat/db.db";
    sqlite3 *db;

    if(sqlite3_open(db_path, &db) != SQLITE_OK) {
        mvprintw(0,0, "Error opening database: %s\n", sqlite3_errmsg(db));
        getch();
        sqlite3_close(db);
        return 1;
    }

    char sql_txt[] = "update tasks "
                     "set time_spent = ? "
                     "where id = ?";
    sqlite3_stmt *stmt;

    if(sqlite3_prepare_v3(db, sql_txt, strlen(sql_txt), 0, &stmt, NULL)
       != SQLITE_OK)
    {
        mvprintw(0,0, "error while compiling sql: %s\n", sqlite3_errmsg(db));
        getch();
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, _task->fact);
    sqlite3_bind_int(stmt, 2, _task->id);

    if(sqlite3_step(stmt) != SQLITE_DONE) {
        mvprintw(0,0, "error updating db: %s\n", sqlite3_errmsg(db));
        getch();
        return -1;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int init_nc(void)
{
    initscr();
    cbreak(); //get raw char input except spec chars like CTRL-C, CTRL-Z, etc.
    noecho(); //do not echo input to screen (to stdout that is i believe)
    keypad(stdscr, true); //enable function keys (like F1, F2, pg_up, etc...)
    curs_set(0); //make the cursor invisible

    return 0;
}
