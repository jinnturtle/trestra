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

#define PROGRAM_NAME "trestra - Time RESource TRAcker"
#define PROGRAM_VERSION "v0.5.0"

#define DB_PATH "dat/db.db"
#define MAX_ID_LEN 9
#define ROOT_TASK_ID 0

//ncurses platfor specific workaround (temporary solution)
#define BCK_CODE 263

void main_menu(void);
// _n = number of bytes the routine CAN EDIT (usually sizeof char[] - 1)
int nc_inp(int _y, int _x, const char *_prompt, char *str_, unsigned _n);
int open_db(const char *_path, sqlite3 **_db);
int print_tasks(void);
int find_task(int _id, struct Task *task_);
int activate_task(void);
int update_task(struct Task *_task);
int create_task();
int remove_task();
int modify_task();
int compile_sql(sqlite3 *_db, const char *_txt, int _n, unsigned _flags,
        sqlite3_stmt **_stmt, const char **_tail);
int init_nc(void);

/*TODO
 * * [x] print time format "XXXXXhXXm"
 * * [x] creating tasks
 * * [x] removing(moving to dustbin) tasks
 * * [x] modifying tasks
 * * [x] modifying tasks - time addition and substraction
 * * [x] confirmation step when deleting task
 * * [ ] nesting tasks (subtasks) via assigning a parent id
 * ** [ ] e.g. show/list/explore/_task/etc menu option to display children
 * ** [ ] indicate tasks that have children when printing
 * ** [ ] add children fact and estimate times to parent when creating/modifying children
 * * [ ] statuses (show on info and modify screens; lookup table in DB)
 * * [ ] display more tasks/lines than fit on screen (scroll, paging, etc)
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
    bool do_clear = true;

    while(cmd != 'q') {
        if(do_clear) { clear(); }
        else { do_clear = true; }

        mvprintw(0,0, "*** %s %s ***\n", PROGRAM_NAME, PROGRAM_VERSION);
        printw("p - print tasks\n");
        printw("a - activate task\n");
        printw("m - modify task\n");
        printw("n - create a new task\n");
        printw("d - delete task\n");
        printw("q - quit\n");

        cmd = getch();

        switch(cmd) {
        case 'p': print_tasks(); break;
        case 'a': activate_task(); break;
        case 'm': modify_task(); break;
        case 'n': create_task(); break;
        case 'd': remove_task(); break;
        case 'q': continue; break;
        default:
            do_clear = false;
            mvprintw(22,0, "invalid selection \"%c\" (%d)", cmd, cmd);
            break;
        }
    }
}

int nc_inp(int _y, int _x, const char *_prompt, char *str_, unsigned _n)
{
    if(str_ == NULL) { return -1; }

    int pos = 0;
    bool clear_ln = true;

    int cmd = 0;
    while(cmd != '\n') {
        move(_y, _x);
        if(_prompt != NULL) { printw("%s", _prompt); }

        printw("%s", str_);

        cmd = getch();

        switch(cmd) {
        case '\n': continue;
        case BCK_CODE:
            str_[pos] = ' ';
            if(pos > 0) {
                --pos;
                str_[pos] = ' ';
            }
            break;
        default:
            if(pos < _n) { str_[pos] = cmd; }

            if(pos < _n - 1) { ++pos; }
            else { continue; }
        }

        if(clear_ln) {
            move(_y, _x + strlen(_prompt));
            for(unsigned i = 0; i < _n; ++i) { printw(" "); }
            clear_ln = false;
        }
    }

    return 0;
}

int open_db(const char *_path, sqlite3 **_db)
{
    if(sqlite3_open(_path, _db) != SQLITE_OK) {
        mvprintw(0,0, "Error opening database: %s\n", sqlite3_errmsg(*_db));
        getch();
        sqlite3_close(*_db);
        return 1;
    }

    return 0;
}

int print_tasks(void)
{
    clear();

    sqlite3 *db;

    if(open_db(DB_PATH, &db) != 0) { return -1; };

    char sql_txt[] = "select id, name, time_estimated, time_spent "
                     "from tasks";
    sqlite3_stmt *stmt;

    if(compile_sql(db, sql_txt, strlen(sql_txt), 0, &stmt, NULL) != 0) {
        return -1;
    }

    move(0,0);
    int rc = 0;
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        char name[64];
        strcpy(name, sqlite3_column_text(stmt, 1));
        int estimate = sqlite3_column_int(stmt, 2);
        int fact = sqlite3_column_int(stmt, 3);

        double progress_perc = (100.0d / estimate) * fact;
        printw("%d: \"%s\" [%luh%lum/%luh%lum (%0.2lf%)]\n",
                id, name,
                fact / 3600, (fact % 3600) / 60,
                estimate / 3600, (estimate % 3600) / 60,
                progress_perc);
    }
    if(rc != SQLITE_DONE) {
        mvprintw(0,0, "error querying the db: %s\n", sqlite3_errmsg(db));
        printw("retcode = %d\n", rc);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        getch();
        return -1;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    getch();
    return 0;
}

int find_task(int _id, struct Task *task_)
{
    if(task_ == NULL) { return -1; }
    if(_id == ROOT_TASK_ID) { return -1; }

    sqlite3 *db;

    if(open_db(DB_PATH, &db) != 0) { return -1; };

    char sql_txt[] = "SELECT id, parent_id, name, created_time, status_time, "
                     "original_time_estimate, time_estimated, time_spent, "
                     "status "
                     "FROM tasks WHERE id = ? LIMIT 1";
    sqlite3_stmt *stmt;

    if(compile_sql(db, sql_txt, strlen(sql_txt), 0, &stmt, NULL) != 0) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, _id);

    task_->id = 0;

    int rc = 0;
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        task_->id = sqlite3_column_int(stmt, 0);
        task_->parent_id = sqlite3_column_int(stmt, 1);
        strcpy(task_->name, sqlite3_column_text(stmt, 2));
        task_->creation_time = sqlite3_column_int(stmt, 3);
        task_->status_time = sqlite3_column_int(stmt, 4);
        task_->orig_estimate = sqlite3_column_int(stmt, 5);
        task_->estimate = sqlite3_column_int(stmt, 6);
        task_->fact = sqlite3_column_int(stmt, 7);
        task_->status = sqlite3_column_int(stmt, 8);
    }
    if(rc != SQLITE_DONE) {
        mvprintw(0,0, "error querying the db: %s\n", sqlite3_errmsg(db));
        printw("retcode = %d\n", rc);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        getch();
        return -1;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if(task_->id == 0) {
        clear();
        mvprintw(0,0, "could not find task! (press any key)");
        getch();
        return -1;
    }

    return 0;
}

int activate_task(void)
{
    clear();

    struct Task task = { 0 };

    char strbuf[MAX_ID_LEN + 1] = { 0 };
    nc_inp(0, 0, "task id: ", strbuf, MAX_ID_LEN);

    if(find_task(atoi(strbuf), &task) != 0) { return -1; }

    clear();

    mvprintw(0,0, "active task: \"%s\" (%d)", task.name, task.id);

    time_t s_tm = time(NULL); //start time

    halfdelay(1); //setting getch to wait for a limited ammount of time

    time_t elapsed = 0;
    int cmd = 0;
    while(cmd != 'q') {
        elapsed = time(NULL) - s_tm;
        time_t total = task.fact + elapsed;
        int remaining = task.estimate - total;

        mvprintw(20,0, "session: %ldh%ldm%lds\n",
                elapsed / 3600, (elapsed % 3600) / 60, elapsed % 60);

        printw("total: %ldh%ldm%lds / %ldh%ldm%lds\n",
                total / 3600, (total % 3600) / 60, total % 60,
                task.estimate / 3600, (task.estimate % 3600) / 60,
                task.estimate % 60);

        char sign[] = {0, 0};
        if(remaining < 0) {
            remaining = -remaining;
            sign[0] = '-';
        }
        printw("remaining: %s%dh%dm%ds\n",
                sign,
                remaining / 3600, (remaining % 3600) / 60, remaining % 60);

        cmd = getch();
    }
    cbreak(); //unlimiting getch wait time

    task.fact += elapsed; //add elapsed time
    update_task(&task);

    return 0;
}

int update_task(struct Task *_task)
{
    if(_task == NULL) { return -1; }

    sqlite3 *db;

    if(open_db(DB_PATH, &db) != 0) { return -1; };

    char sql_txt[] = "UPDATE tasks "
                     "SET "
                         "parent_id = ?1, "
                         "name = ?2, "
                         "created_time = ?3, "
                         "status_time = ?4, "
                         "original_time_estimate = ?5, "
                         "time_estimated = ?6, "
                         "time_spent = ?7, "
                         "status = ?8 "
                     "WHERE id = ?9";
    sqlite3_stmt *stmt;

    if(compile_sql(db, sql_txt, strlen(sql_txt), 0, &stmt, NULL) != 0) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, _task->parent_id);
    sqlite3_bind_text(stmt, 2, _task->name, -1, NULL);
    sqlite3_bind_int(stmt, 3, _task->creation_time);
    sqlite3_bind_int(stmt, 4, _task->status_time);
    sqlite3_bind_int(stmt, 5, _task->orig_estimate);
    sqlite3_bind_int(stmt, 6, _task->estimate);
    sqlite3_bind_int(stmt, 7, _task->fact);
    sqlite3_bind_int(stmt, 8, _task->status);
    sqlite3_bind_int(stmt, 9, _task->id);

    if(sqlite3_step(stmt) != SQLITE_DONE) {
        mvprintw(0,0, "error updating db: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        getch();
        return -1;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int remove_task()
{
    unsigned id = 0;
    char strbuf[MAX_ID_LEN + 1] = { 0 };

    clear();
    mvprintw(0,0, "*** delete task ***");

    nc_inp(1,0, "task id (0 for none): ", strbuf, MAX_ID_LEN);
    id = atoi(strbuf);

    struct Task task;
    if(find_task(id, &task) != 0) { return -1; };

    mvprintw(2,0, "task: \"%s\"", task.name);

    char confirm[2] = "n";
    nc_inp(3,0, "delete? (y/n): ", confirm, sizeof confirm - 1);
    if(confirm[0] != 'y') { 
        mvprintw(5,0, "canceled (press any key)");
        getch();
        return 0;
    }

    sqlite3 *db;
    if(open_db(DB_PATH, &db) != 0) { return -1; }

    char sql_txt[] = "insert into deleted_tasks "
                     "select * from tasks where id = ?";

    sqlite3_stmt *stmt_cp;
    if(compile_sql(db, sql_txt, strlen(sql_txt), 0, &stmt_cp, NULL) != 0) {
        return -1;
    }

    strcpy(sql_txt, "delete from tasks where id = ?");

    sqlite3_stmt *stmt_del;
    if(compile_sql(db, sql_txt, strlen(sql_txt), 0, &stmt_del, NULL) != 0) {
        return -1;
    }

    sqlite3_bind_int(stmt_cp, 1, id);
    sqlite3_bind_int(stmt_del, 1, id);

    if(sqlite3_step(stmt_cp) != SQLITE_DONE) {
        mvprintw(0,0, "could not copy task to deleted_tasks\n");
        printw("error: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt_cp);
        sqlite3_finalize(stmt_del);
        sqlite3_close(db);
        getch();
        return -1;
    }

    sqlite3_finalize(stmt_cp);

    if(sqlite3_step(stmt_del) != SQLITE_DONE) {
        mvprintw(0,0, "could not delete task from tasks table\n");
        printw("error: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt_del);
        sqlite3_close(db);
        getch();
        return -1;
    }

    sqlite3_finalize(stmt_del);
    sqlite3_close(db);
    return 0;
}

int modify_task()
{
    clear();
    mvprintw(0,0, "*** modify task ***");

    struct Task task = { 0 };

    char strbuf[100] = { 0 };
    nc_inp(1, 0, "task id: ", strbuf, MAX_ID_LEN);

    if(find_task(atoi(strbuf), &task) < 0) { return -1; }

    printw(" \"%s\"", task.name);

    char prompt[40] = "parent id: ";
    mvprintw(2,0, "%s[%d]", prompt, task.parent_id); //printing original value
    memset(&strbuf, '\0', sizeof strbuf); //clearing the buffer for next use
    nc_inp(2,0, prompt, strbuf, sizeof strbuf - 1); //new value input
    if(strbuf[0]) { task.parent_id = atoi(strbuf); }

    strcpy(prompt, "name: ");
    mvprintw(3,0, "%s[%s]", prompt, task.name); //printing original value
    memset(&strbuf, '\0', sizeof strbuf); //clearing the buffer for next use
    nc_inp(3,0, prompt, strbuf, sizeof strbuf - 1); //new value input
    if(strbuf[0]) { strcpy(task.name, strbuf); }

    strcpy(prompt, "original estimate (minutes): ");
    mvprintw(4,0, "%s[%u]", prompt, task.orig_estimate / 60); //printing original value
    memset(&strbuf, '\0', sizeof strbuf); //clearing the buffer for next use
    nc_inp(4,0, prompt, strbuf, sizeof strbuf - 1); //new value input
    if(strbuf[0]) { 
        if(strbuf[0] == '+') { task.orig_estimate += atoi(&strbuf[1]) * 60; }
        else if(strbuf[0] == '-') {task.orig_estimate -= atoi(&strbuf[1]) * 60;}
        else { task.orig_estimate = atoi(strbuf) * 60; }
    }

    strcpy(prompt, "estimate (minutes): ");
    mvprintw(5,0, "%s[%u]", prompt, task.estimate / 60); //printing original value
    memset(&strbuf, '\0', sizeof strbuf); //clearing the buffer for next use
    nc_inp(5,0, prompt, strbuf, sizeof strbuf - 1); //new value input
    if(strbuf[0]) { 
        if(strbuf[0] == '+') { task.estimate += atoi(&strbuf[1]) * 60; }
        else if(strbuf[0] == '-') { task.estimate -= atoi(&strbuf[1]) * 60; }
        else { task.estimate = atoi(strbuf) * 60; }
    }

    strcpy(prompt, "time spent (minutes): ");
    mvprintw(6,0, "%s[%u]", prompt, task.fact / 60); //printing original value
    memset(&strbuf, '\0', sizeof strbuf); //clearing the buffer for next use
    nc_inp(6,0, prompt, strbuf, sizeof strbuf - 1); //new value input
    if(strbuf[0]) { 
        if(strbuf[0] == '+') { task.fact += atoi(&strbuf[1]) * 60; }
        else if(strbuf[0] == '-') { task.fact -= atoi(&strbuf[1]) * 60; }
        else { task.fact = atoi(strbuf) * 60; }
    }

    strcpy(prompt, "status: ");
    mvprintw(7,0, "%s[%d]", prompt, task.status); //printing original value
    memset(&strbuf, '\0', sizeof strbuf); //clearing the buffer for next use
    nc_inp(7,0, prompt, strbuf, sizeof strbuf - 1); //new value input
    int old_status = task.status;
    if(strbuf[0]) { task.status = atoi(strbuf); }

    strcpy(prompt, "submit(y/n)?: ");
    memset(&strbuf, '\0', sizeof strbuf); //clearing the buffer for next use
    strcpy(strbuf, "n"); //writing default value
    nc_inp(9,0, prompt, strbuf, sizeof strbuf - 1);

    bool commit = false;
    char msg[40] = { 0 };
    if(strbuf[0] == 'y') {
        commit = true;
        strcpy(msg, "commiting...");
    }
    else { strcpy(msg, "discarding..."); }
    mvprintw(10,0, "%s", msg);

    if(commit == true) {
        if(task.status != old_status) { task.status_time = time(NULL); }

        if(update_task(&task) != 0) {
            mvprintw(11,0, " FAILURE!");
            getch();
            return -1;
        }
    }

    printw(" done. (press any key)");
    getch();

    return 0;
}

int compile_sql(sqlite3 *_db, const char *_txt, int _n, unsigned _flags,
        sqlite3_stmt **_stmt, const char **_tail)
{
    if(sqlite3_prepare_v3(_db, _txt, _n, _flags, _stmt, _tail) != SQLITE_OK)
    {
        mvprintw(0,0, "error while compiling sql: %s\n", sqlite3_errmsg(_db));
        getch();
        sqlite3_finalize(*_stmt);
        sqlite3_close(_db);
        return -1;
    }

    return 0;
}

int create_task()
{
    //fill a new task structure
    time_t creation_time = time(NULL);
    char strbuf[100] = { 0 };

    clear();
    mvprintw(0,0, "*** create task ***");

    struct Task task = { 0 };
    task.creation_time = creation_time;
    task.status_time = creation_time;
    nc_inp(1, 0, "task name: ", &task.name[0], sizeof task.name - 1);
    nc_inp(2, 0, "parent id (0 for none): ", &strbuf[0], sizeof strbuf - 1);
    task.parent_id = atoi(strbuf);
    memset(&strbuf, '\0', sizeof strbuf); //clearing the buffer for next use
    nc_inp(3, 0, "time estimate (minutes): ", &strbuf[0], sizeof strbuf - 1);
    task.orig_estimate = atoi(strbuf) * 60;
    task.estimate = task.orig_estimate;
    memset(&strbuf, '\0', sizeof strbuf); //clearing the buffer for next use
    nc_inp(4, 0, "time already spent (minutes): ", &strbuf[0], sizeof strbuf-1);
    task.fact = atoi(strbuf) * 60;
    task.status = 0; //TODO LATER statuses are not implemented yet

    sqlite3 *db;
    if(open_db(DB_PATH, &db) != 0) { return -1; };

    char sql_txt[] =
        "insert into tasks "
        "(parent_id, name, created_time, status_time, "
        "original_time_estimate, time_estimated, time_spent, status) "
        "values "
        "(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)";

    sqlite3_stmt *stmt;
    if(compile_sql(db, sql_txt, strlen(sql_txt), 0, &stmt, NULL) != 0) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, task.parent_id);
    sqlite3_bind_text(stmt, 2, task.name, -1, NULL);
    sqlite3_bind_int(stmt, 3, task.creation_time);
    sqlite3_bind_int(stmt, 4, task.status_time);
    sqlite3_bind_int(stmt, 5, task.orig_estimate);
    sqlite3_bind_int(stmt, 6, task.estimate);
    sqlite3_bind_int(stmt, 7, task.fact);
    sqlite3_bind_int(stmt, 8, task.status);

    if(sqlite3_step(stmt) != SQLITE_DONE) {
        mvprintw(0,0, "error creating task! :%s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        getch();
        return -1;
    };

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
