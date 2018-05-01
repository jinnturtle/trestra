//std c lib
#include <stdio.h>
#include <string.h>
#include <time.h>

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
int get_tasks(struct Task *_tasks);
int init_nc(void);

int main(void)
{
    init_nc();

    struct Task tasks[100];
    get_tasks(&tasks[0]);

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
        printw("id: %d task: \"%s\" progress: %0.2lf\n", id, name, progress_perc);
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

int activate_task(void)
{
    //TODO for now this only counts some seconds and does nothing else

    clear();

    time_t s_tm = time(NULL);

    halfdelay(1); //setting getch to wait for a limited ammount of time

    int cmd = 0;
    while(cmd != 'q') {
        mvprintw(22,0, "time spent: %ld\n", time(NULL) - s_tm);

        cmd = getch();
    }

    cbreak(); //unlimiting getch wait time

    return 0;
}

int get_tasks(struct Task *_tasks) {
    //TODO functionality

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
