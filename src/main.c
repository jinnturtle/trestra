//std c lib
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h> //atoi(), etc

//system (unix/linux)
#include <ncurses.h>
#include <unistd.h>

//3rd party
#include "sqlite3.h"

//homebrew
#include "dbg.h"
#include "Task.h"
#include "utils.h"
#include "hack_terminal.h"

#define PROGRAM_NAME "Time RESource TRAcker"
#define PROGRAM_VERSION "v2.0.0-alpha1"
#ifdef DEBUG
#  define SPEC_VERSION "-DEV"
#else
#  define SPEC_VERSION ""
#endif

#define MAX_ID_LEN 9
#define ROOT_TASK_ID 0

#define STATE_ACTIVE 1
#define STATE_PAUSE 2

void main_menu(int _id);
// _n = number of bytes the routine CAN EDIT (usually sizeof char[] - 1)
int nc_inp(int _y, int _x, const char *_prompt, char *str_, unsigned _n);
// modifies AND returns the str_ buffer */
int get_tasks(struct Task *_tasks, size_t _n, unsigned _parent_id);
int explore_tasks(int _parent_id, int _selected_id);
int find_task(int _id, struct Task *task_);
int activate_task(int _task_id);
int update_parent(sqlite3 *_db, int _id);
int update_task(struct Task *_task, unsigned _old_parent_id);
int create_task(int _parent_id);
int remove_task_interact(unsigned _id);
int remove_task(sqlite3* _db, unsigned _id);
int remove_children(sqlite3 *_db, int _parent_id);
int show_task_info(int _id);
int modify_task(int _id);
int menu_create_db(char* _filepath);
int create_db(const char *_path);
int task_explorer(struct Task *_tasks, size_t _n, int *sel_id_);
int init_nc(void);

int main(void)
{
    if(dbg_init() != 0) {
        printf("error initializing dbg logging\n");
        return -1;
    }

    dbg("*** NEW SESSION ***");

    init_nc();

    //if there is no database, prompt to create a new one
    char* db_filepath = get_db_filepath();
    if (!db_filepath) {
        dbg("error getting db_filepath");
        return -1;
    }
    dbgf("db_filepath == %s", db_filepath);

    if(access(db_filepath, F_OK) == -1) {
        if(menu_create_db(db_filepath) == -1) { return -1; }
    }

    main_menu(0);

    endwin(); //end ncurses mode
    free(db_filepath);
    dbg_deinit();
    return 0;
}

void main_menu(int _id)
{
    int cmd = 0;
    bool do_clear = true;
    unsigned max_entry_len = 22;
    int mid_x = (getmaxx(stdscr) - max_entry_len) / 2;
    char header[60];

    snprintf(header, sizeof header, "*** %s %s%s ***",
             PROGRAM_NAME, PROGRAM_VERSION, SPEC_VERSION);

    while(cmd != 'q') {
        int pos_y = 0;
        if(do_clear) { clear(); }
        else { do_clear = true; }

        //mvprintw(0,0, "*** %s %s ***\n", PROGRAM_NAME, PROGRAM_VERSION);
        print_mid(pos_y, header);
        mvprintw(++pos_y,mid_x, "e - explore tasks\n");
        if(_id > 0) {
            mvprintw(++pos_y,mid_x, "a - activate task\n");
            mvprintw(++pos_y,mid_x, "i - task info\n");
            mvprintw(++pos_y,mid_x, "m - modify task\n");
            mvprintw(++pos_y,mid_x, "d - delete task\n");
        }
        mvprintw(++pos_y,mid_x, "n - create a new task\n");
        mvprintw(pos_y += 2,mid_x,   "q - quit\n");

        cmd = getch();

        switch(cmd) {
        case 'e':
            explore_tasks(_id, 0); break;
        case 'a':
            if(_id > 0) {activate_task(_id);}
            break;
        case 'i':
            if(_id > 0) {show_task_info(_id);}
            break;
        case 'm':
            if(_id > 0) {modify_task(_id);}
            break;
        case 'd':
            if(_id > 0) {remove_task_interact(_id);}
            break;
        case 'c':
            //same as 'n'
        case 'n':
            create_task(_id); break;
        case 'q':
            continue; break;
        default:
            do_clear = false;
            mvprintw(getmaxy(stdscr) -1,0, "invalid selection \"%c\" (%d)     ",
                     cmd, cmd);
            break;
        }
    }
}

int nc_inp(int _y, int _x, const char *_prompt, char *str_, unsigned _n)
{
    if(str_ == NULL) { return -1; }

    int pos = 0;
    bool clear_ln = true;
    bool preview = (str_[0] == '\0')? false : true;

    int cmd = 0;
    while(cmd != '\n') {
        move(_y, _x);
        if(_prompt != NULL) { printw("%s", _prompt); }

        if(preview) {
            printw("[%s]", str_);
            preview = false;
        }
        else { printw("%s", str_); }

        cmd = getch();

        switch(cmd) {
        case '\n': continue;
        //various possible backspace codes
        case KEY_BACKSPACE:
        case KEY_DC:
        case 127: //some terminals seem to use this
        //case BCK_CODE:
            str_[pos] = '\0';
            if(pos > 0) {
                --pos;
                str_[pos] = '\0';
            }

            clear_ln = true;
            break;
        default:
            if(pos < _n) { str_[pos] = cmd; }
            str_[pos + 1] = '\0';//making sure the string ends right after caret

            if(pos < _n - 1) { ++pos; }
        }

        if(clear_ln) {
            move(_y, _x + strlen(_prompt));
            // _n + 2 to account for the preview brackets
            for(unsigned i = 0; i < _n + 2; ++i) { printw(" "); }
            clear_ln = false;
        }
    }

    return 0;
}

int get_tasks(struct Task *_tasks, size_t _n, unsigned _parent_id)
{
    //get link to db and compile sql statement

    sqlite3 *db;

    char* db_fpath = get_db_filepath();
    if(open_db(db_fpath, &db) != 0) { return -1; };
    free(db_fpath);

    char sql_txt[] = "SELECT id, parent_id, name, created_time, status_time, "
                     "original_time_estimate, time_estimated, time_spent, "
                     "status, notes "
                     "FROM tasks WHERE parent_id = ?";
    sqlite3_stmt *stmt;

    if(compile_sql(db, sql_txt, strlen(sql_txt), 0, &stmt, NULL) != 0) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, _parent_id);

    //fill task buffer

    unsigned fill_pos = 0;

    int rc = 0;
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        task_init_form_stmt(stmt, &_tasks[fill_pos]);

        _tasks[fill_pos].is_parent = check_for_children(db,_tasks[fill_pos].id);

        if(fill_pos <= _n) { ++fill_pos; }
        else { break; }
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

    return fill_pos;
}

/* FIXME if db was not created, this errors but creates and empty file
 * probably open_db() in get_tasks() is to blame, but not sure
 */
int explore_tasks(int _parent_id, int _selected_id)
{
    clear();

    /*TODO think of a fast way to determine the actual space needed for the
     * buffer instead of just hardcoding a magic number*/
    size_t max_buf_size = 1000;
    struct Task *tasks = NULL;
    size_t mem_req = max_buf_size * sizeof tasks[0];
    tasks = malloc(mem_req);
    dbgf("task buffer size: %lub", mem_req);

    int tasks_buffered = get_tasks(tasks, max_buf_size, _parent_id);

    if(tasks_buffered < 0) {
        free(tasks);
        return -1;
    }

    int opt = 0;
    while(opt != 'q') {
        opt = task_explorer(tasks, tasks_buffered, &_selected_id);

        switch(opt) {
        case 'l': explore_tasks(_selected_id, 0); break;
        case 's': activate_task(_selected_id); break;
        case 'h': opt = 'q'; break;
        case 'm': main_menu(_selected_id); break;
        case 'i': show_task_info(_selected_id); break;
        }
    }

    free(tasks);
    return 0;
}

//TODO move tasks like these to db_utils.h
int find_task(int _id, struct Task *task_)
{
    if(task_ == NULL) { return -1; }
    if(_id == ROOT_TASK_ID) { return -1; }

    sqlite3 *db;

    char* db_fpath = get_db_filepath();
    if(open_db(db_fpath, &db) != 0) { return -1; };
    free(db_fpath);

    char sql_txt[] = "SELECT id, parent_id, name, created_time, status_time, "
                     "original_time_estimate, time_estimated, time_spent, "
                     "status, notes "
                     "FROM tasks WHERE id = ? LIMIT 1";
    sqlite3_stmt *stmt;

    if(compile_sql(db, sql_txt, -1, 0, &stmt, NULL) != 0) { return -1; }

    sqlite3_bind_int(stmt, 1, _id);

    task_->id = 0;

    /* TODO (clean up code) - feels like the below loop is using functionality
     * extremely similar to Task.c->task_init_form_stmt(),
     * why copy similar logic here if the function can be used here?*/
    int rc = 0;
    const char *txt_buf = NULL;
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        task_->id = sqlite3_column_int(stmt, 0);
        task_->parent_id = sqlite3_column_int(stmt, 1);
        txt_buf = (const char*)sqlite3_column_text(stmt, 2);
        if(txt_buf) {strcpy(task_->name, txt_buf);}
        txt_buf = NULL;
        task_->creation_time = sqlite3_column_int(stmt, 3);
        task_->status_time = sqlite3_column_int(stmt, 4);
        task_->orig_estimate = sqlite3_column_int(stmt, 5);
        task_->estimate = sqlite3_column_int(stmt, 6);
        task_->fact = sqlite3_column_int(stmt, 7);
        task_->status = sqlite3_column_int(stmt, 8);
        txt_buf = (const char*)sqlite3_column_text(stmt, 9);
        if(txt_buf) {strcpy(task_->notes, txt_buf);}
        txt_buf = NULL;
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

int activate_task(int _task_id)
{
    char strbuf[MAX_ID_LEN + 1] = { 0 };
    char est_buf[20] = { 0 };
    char elp_buf[20] = { 0 };

    clear();

    //find and check

    struct Task task = { 0 };

    unsigned task_id = 0;
    if(_task_id != 0) { task_id = _task_id; }
    else {
        nc_inp(0, 0, "task id: ", strbuf, MAX_ID_LEN);
        task_id = atoi(strbuf);
    }

    if(find_task(task_id, &task) != 0) { return -1; }

    mvprintw(1, 0, "task: ");
    print_task("hms", &task);

    memset(strbuf, '\0', sizeof strbuf);
    strbuf[0] = 'n';
    nc_inp(2, 0, "select task? (y/n): ", strbuf, 1);

    if(strbuf[0] != 'y') { return 1; }

    //the main part (print time counters, then update the db at the end)

    clear();

    mvprintw(0,0, "active task: \"%s\" (%d)", task.name, task.id);

    time_t s_tm = time(NULL); //start time

    halfdelay(1); //setting getch to wait for a limited ammount of time

    time_t elapsed = 0;
    time_t paused_time = 0;
    time_t pause_start = 0;
    time_t total_paused_tm = 0;
    int state = STATE_ACTIVE;
    char state_txt[7] = "ACTIVE";

    int cmd = 0;
    while(cmd != 'q') {
        switch(state) {
        case STATE_ACTIVE:
            attron(A_REVERSE);
            mvprintw(getmaxy(stdscr) - 4, 0, state_txt);
            attroff(A_REVERSE);

            break;
        case STATE_PAUSE:
            attron(A_BLINK);
            mvprintw(getmaxy(stdscr) - 4, 0, state_txt);
            attroff(A_BLINK);

            paused_time = time(NULL) - pause_start;

            break;
        }

        elapsed = time(NULL) - s_tm - paused_time - total_paused_tm;
        time_t total = task.fact + elapsed;
        int remaining = task.estimate - total;

        char tot_buf[20] = {0};
        char rem_buf[20] = {0};

        mvprintw(getmaxy(stdscr) - 3, 0,
                    "session: %s\n", format_time_str("hms", elapsed, elp_buf));
        printw("total: %s / %s\n",
                format_time_str("hms", total, tot_buf),
                format_time_str("hms", task.estimate, est_buf));
        char sign[2] = {0};
        if(remaining < 0) {
            remaining = -remaining;
            sign[0] = '-';
        }
        printw("remaining: %s%s\n",
                sign,
                format_time_str("hms", remaining, rem_buf));

        cmd = getch();
        switch(cmd) {
        case 'p':
            if(state == STATE_ACTIVE) {
                state = STATE_PAUSE;
                strcpy(state_txt, "PAUSED");
                total_paused_tm += paused_time;
                paused_time = 0;
                pause_start = time(NULL);
            }
            else {
                state = STATE_ACTIVE;
                strcpy(state_txt, "ACTIVE");
            }
        }
    }
    cbreak(); //unlimiting getch wait time

    task.fact += elapsed; //add elapsed time
    if(update_task(&task, 0) != 0) { return -1; }

    return 0;
}

int get_children_time(sqlite3 *_db, int _parent_id, struct Task *totals_)
{
    totals_->orig_estimate = 0;
    totals_->estimate = 0;
    totals_->fact = 0;

    char sql_txt[] = "SELECT "
                     "original_time_estimate, time_estimated, time_spent "
                     "FROM tasks WHERE parent_id = ?";
    sqlite3_stmt *stmt;

    if(compile_sql(_db, sql_txt, -1, 0, &stmt, NULL) != 0) { return -1; }

    sqlite3_bind_int(stmt, 1, _parent_id);

    int rc = 0;
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        totals_->orig_estimate += sqlite3_column_int(stmt, 0);
        totals_->estimate += sqlite3_column_int(stmt, 1);
        totals_->fact += sqlite3_column_int(stmt, 2);
    }
    if(rc != SQLITE_DONE) {
        mvprintw(0,0, "error querying the db: %s\n", sqlite3_errmsg(_db));
        printw("retcode = %d\n", rc);
        sqlite3_finalize(stmt);
        sqlite3_close(_db);
        getch();
        return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int update_parent(sqlite3 *_db, int _id)
{
    if(_id == 0) { return 1; }

    struct Task task = { 0 };
    if(find_task(_id, &task) != 0) { return -1; }

    struct Task totals = { 0 };
    get_children_time(_db, task.id, &totals);

    task.orig_estimate = totals.orig_estimate;
    task.estimate = totals.estimate;
    task.fact = totals.fact;

    if(update_task(&task, 0) != 0) { return -1; }

    return 0;
}

int update_task(struct Task *_task, unsigned _old_parent_id)
{
    if(_task == NULL) { return -1; }

    sqlite3 *db;

    char* db_fpath = get_db_filepath();
    if(open_db(db_fpath, &db) != 0) { return -1; };
    free(db_fpath);

    char sql_txt[] = "UPDATE tasks "
                     "SET "
                         "parent_id = ?1, "
                         "name = ?2, "
                         "created_time = ?3, "
                         "status_time = ?4, "
                         "original_time_estimate = ?5, "
                         "time_estimated = ?6, "
                         "time_spent = ?7, "
                         "status = ?8, "
                         "notes = ?9 "
                     "WHERE id = ?10";
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
    sqlite3_bind_text(stmt, 9, _task->notes, -1, NULL);
    sqlite3_bind_int(stmt, 10, _task->id);

    if(sqlite3_step(stmt) != SQLITE_DONE) {
        clear();
        mvprintw(0,0, "error updating db: %s\n", sqlite3_errmsg(db));
        mvprintw(1,0, "failed to update task %u%s\"\"", _task->id, _task->name);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        getch();
        return -1;
    }

    if(_task->parent_id != 0) {
        if(update_parent(db, _task->parent_id) < 0) { return -2; }
        if(_task->parent_id != _old_parent_id) {
            if(update_parent(db, _old_parent_id) < 0) {
                clear();
                mvprintw(0,0, "Warning, task updated successfully, but failed");
                mvprintw(1,0, "to update the previous parent of said task.");
                getch();
                return -3;
            }
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int remove_task_interact(unsigned _id)
{
    unsigned id = _id;
    char strbuf[MAX_ID_LEN + 1] = { 0 };

    clear();
    mvprintw(0,0, "*** delete task ***");

    if(id == 0) {
        nc_inp(1,0, "task id (0 for none): ", strbuf, MAX_ID_LEN);
        id = atoi(strbuf);
    }

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

    remove_task(NULL, id);

    return 0;
}

int remove_task(sqlite3 *_db, unsigned _id)
{
    if(_id == 0) { return 1; }

    sqlite3 *db;
    if(_db == NULL) {
        char* db_fpath = get_db_filepath();
        if(open_db(db_fpath, &db) != 0) { return -1; }
        free(db_fpath);
    }
    else { db = _db; }


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

    sqlite3_bind_int(stmt_cp, 1, _id);
    sqlite3_bind_int(stmt_del, 1, _id);

    if(check_for_children(db, _id)) { remove_children(db, _id); }

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
    if(_db == NULL) { sqlite3_close(db); }
    return 0;
}

int remove_children(sqlite3 *_db, int _parent_id)
{
    char sql_txt[] = "SELECT id FROM tasks "
                     "WHERE parent_id = ?";

    sqlite3_stmt *stmt;
    if(compile_sql(_db, sql_txt, -1, 0, &stmt, NULL) != 0 ) { return -1; }

    sqlite3_bind_int(stmt, 1, _parent_id);

    int delete_me = 0;

    int rc = 0;
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        ++delete_me;
        remove_task(_db, sqlite3_column_int(stmt, 0));
    }
    if(rc != SQLITE_DONE) {
        clear();
        mvprintw(0,0, "error querying the db: %s\n", sqlite3_errmsg(_db));
        printw("retcode = %d\n", rc);
        sqlite3_finalize(stmt);
        getch();
        return -1;
    }

    clear();
    mvprintw(0,0, "deleted %d children", delete_me);


    sqlite3_finalize(stmt);
    getch();
    return 0;
}

int show_task_info(int _id)
{
    char txtbuf[100] = { 0 };
    struct Task task = { 0 };
    int id = _id;
    int caret_y = 0;
    const size_t txt_preview_len = 40;

    //sanity checks
    //is it the root task? don't want to display that
    if(id == 0) {return 1;}
    //does the task exist?
    if(find_task(id, &task) < 0) {return -1;}

    //so far so good - let's go
    clear();
    mvprintw(caret_y,0, "*** Task Info ***");
    mvprintw(++caret_y,0, "name: %s", task.name);
    //TODO show date of creation and date of last change
    mvprintw(++caret_y,0, "created: %s", "[placeholder]");
    mvprintw(++caret_y,0, "last change: %s", "[placeholder]");
    mvprintw(++caret_y,0, "id: %u", task.id);
    mvprintw(++caret_y,0, "parent id: %d", task.parent_id);
    time_to_htime(task.orig_estimate, txtbuf, sizeof txtbuf - 1);
    mvprintw(++caret_y,0, "orig. estimate: %s", txtbuf);
    time_to_htime(task.estimate, txtbuf, sizeof txtbuf - 1);
    mvprintw(++caret_y,0, "estimate: %s", txtbuf);
    time_to_htime(task.fact, txtbuf, sizeof txtbuf - 1);
    mvprintw(++caret_y,0, "time spent: %s", txtbuf);
    mvprintw(++caret_y,0, "status: %d", task.status);
    //only taking part of the task.notes text for this preview
    strncpy(txtbuf, task.notes, txt_preview_len);
    if(strlen(task.notes) > txt_preview_len) {
        strcpy(&(txtbuf[txt_preview_len - 3]), "[...]");
    }
    mvprintw(++caret_y,0, "notes: %s", txtbuf);

    getch();

    return 0;
}

int modify_task(int _id)
{
    char strbuf[100] = { 0 };
    char strbuf2[100] = { 0 };
    struct Task task = { 0 };
    int id = _id;
    int caret_y = 0; //which line to print at, should be incremented accordingly

    clear();
    mvprintw(caret_y,0, "*** modify task ***");

    if(id == 0) {
        nc_inp(1, 0, "task id: ", strbuf, MAX_ID_LEN);
        id = atoi(strbuf);
    }

    if(find_task(id, &task) < 0) { return -1; }

    //need this in order to update the old parent, if we move task
    unsigned old_parent_id = task.parent_id;

    move(++caret_y, 0);
    print_task("hms", &task);
    //printw(" \"%s\"", task.name);

    char prompt[40] = "parent id: ";
    snprintf(strbuf, MAX_ID_LEN, "%u", task.parent_id); //filling default value
    nc_inp(++caret_y,0, prompt, strbuf, MAX_ID_LEN); //new value input
    if(strbuf[0]) { task.parent_id = atoi(strbuf); }

    strcpy(prompt, "name: ");
    snprintf(strbuf, sizeof strbuf, "%s", task.name);
    nc_inp(++caret_y,0, prompt, strbuf, sizeof strbuf - 1); //new value input
    if(strbuf[0]) { strcpy(task.name, strbuf); }

    strcpy(prompt, "original estimate: ");
    snprintf(strbuf, sizeof strbuf, "%s", //filling default value
            time_to_htime(task.orig_estimate, strbuf2, sizeof strbuf2));
    nc_inp(++caret_y,0, prompt, strbuf, sizeof strbuf - 1); //new value input
    if(strbuf[0]) { 
        if(strbuf[0] == '+') {
            task.orig_estimate += htime_to_time(&strbuf[1], sizeof strbuf - 1);
        }
        else if(strbuf[0] == '-') {
           task.orig_estimate -= htime_to_time(&strbuf[1], sizeof strbuf - 1);
        }
        else { task.orig_estimate = htime_to_time(strbuf, sizeof strbuf); }
    }

    strcpy(prompt, "estimate: ");
    snprintf(strbuf, sizeof strbuf, "%s", //filling default value
            time_to_htime(task.estimate, strbuf2, sizeof strbuf2));
    nc_inp(++caret_y,0, prompt, strbuf, sizeof strbuf - 1); //new value input
    if(strbuf[0]) { 
        if(strbuf[0] == '+') {
            task.estimate += htime_to_time(&strbuf[1], sizeof strbuf - 1);
        }
        else if(strbuf[0] == '-') {
           task.estimate -= htime_to_time(&strbuf[1], sizeof strbuf - 1);
        }
        else { task.estimate = htime_to_time(strbuf, sizeof strbuf); }
    }

    strcpy(prompt, "time spent: ");
    snprintf(strbuf, sizeof strbuf, "%s", //filling default value
            time_to_htime(task.fact, strbuf2, sizeof strbuf2));
    nc_inp(++caret_y,0, prompt, strbuf, sizeof strbuf - 1); //new value input
    if(strbuf[0]) { 
        if(strbuf[0] == '+') {
            task.fact += htime_to_time(&strbuf[1], sizeof strbuf - 1);
        }
        else if(strbuf[0] == '-') {
           task.fact -= htime_to_time(&strbuf[1], sizeof strbuf - 1);
        }
        else { task.fact = htime_to_time(strbuf, sizeof strbuf); }
    }

    strcpy(prompt, "status: ");
    snprintf(strbuf, sizeof strbuf, "%d", task.status);//filling default value
    nc_inp(++caret_y,0, prompt, strbuf, sizeof strbuf - 1); //new value input
    int old_status = task.status;
    if(strbuf[0]) { task.status = atoi(strbuf); }

    strcpy(prompt, "notes: ");
    snprintf(strbuf, sizeof strbuf, "%s", task.notes);//filling default value
    nc_inp(++caret_y,0, prompt, strbuf, sizeof strbuf - 1); //new value input
    if(strbuf[0]) { strcpy(task.notes, strbuf); }

    strcpy(prompt, "submit(y/n)?: ");
    strcpy(strbuf, "n"); //writing default value
    nc_inp(caret_y += 2,0, prompt, strbuf, 1);

    bool commit = false;
    char msg[40] = { 0 };
    if(strbuf[0] == 'y') {
        commit = true;
        strcpy(msg, "commiting...");
    }
    else { strcpy(msg, "discarding..."); }
    mvprintw(++caret_y, 0, "%s", msg);

    if(commit == true) {
        if(task.status != old_status) { task.status_time = time(NULL); }

        if(update_task(&task, old_parent_id) != 0) { return -1; }
    }

    printw(" done. (press any key)");
    getch();

    return 0;
}

int create_task(int _parent_id)
{
    char strbuf[100] = { 0 };
    char default_parent_id[MAX_ID_LEN + 1] = "0";
    char default_est[] = "8h";
    char default_fact[] = "0";

    //writing default parent id
    snprintf(default_parent_id, sizeof default_parent_id, "%d", _parent_id);

    clear();
    mvprintw(0,0, "*** create task ***");

    struct Task task = { 0 };
    nc_inp(1, 0, "task name: ", &task.name[0], sizeof task.name - 1);
    strcpy(strbuf, default_parent_id); //filling default value
    nc_inp(2, 0, "parent id (0 for none): ", &strbuf[0], MAX_ID_LEN);
    task.parent_id = atoi(strbuf);
    strcpy(strbuf, default_est); //filling default value
    nc_inp(3, 0, "time estimate: ", &strbuf[0], sizeof strbuf - 1);
    task.estimate = htime_to_time(strbuf, sizeof strbuf);
    strcpy(strbuf, default_fact); //filling default value
    nc_inp(4, 0, "time already spent: ", &strbuf[0], sizeof strbuf-1);
    task.fact = htime_to_time(strbuf, sizeof strbuf);

    task.status = 0; //TODO LATER statuses are not implemented yet

    task.orig_estimate = task.estimate;
    task.creation_time = time(NULL);
    task.status_time = task.creation_time;

    memset(&strbuf, '\0', sizeof strbuf); //clearing the buffer for next use
    strbuf[0] = 'n';
    nc_inp(6, 0, "confirm? (y/n): ", strbuf, 1);
    if(strbuf[0] != 'y') {
        mvprintw(7,0, "cancelled (press any key)");
        getch();
        return 0;
    }

    mvprintw(7,0, "creating task...");

    sqlite3 *db;
    char* db_fpath = get_db_filepath();
    if(open_db(db_fpath, &db) != 0) { return -1; };
    free(db_fpath);

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

    if(task.parent_id != 0) { update_parent(db, task.parent_id); }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int menu_create_db(char* _filepath)
{
    char menu_name[] = "*** create new database ***";

    int cmd = 0;
    while(cmd != 'q') {
        print_mid(0, menu_name);
        print_mid(2, "new path:");
        print_mid(3, _filepath);
        print_mid(5, "n - create new database");
        print_mid(6, "q - exit this menu");

        cmd = getch();

        switch(cmd) {
        case 'n':
            create_db(_filepath);
            cmd = 'q'; //in order to quit after this command executes
            break;
        default: break;
        }
    }

    return 0;
}

int create_db(const char *_path)
{
    sqlite3 *db;

    if(open_db(_path, &db) != 0) {return -1;}

    char sql_txt[1024*2] =
        "CREATE TABLE \"tasks\""
        "(`id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,"
        "`parent_id` INTEGER,"
        "`name` TEXT NOT NULL,"
        "`created_time` INTEGER,"
        "`status_time` INTEGER,"
        "`original_time_estimate` INTEGER,"
        "`time_estimated` INTEGER,"
        "`time_spent` INTEGER,"
        "`status` INTEGER,"
        "`notes` TEXT )";

    sqlite3_stmt *stmt;

    if(compile_sql(db, sql_txt, -1, 0, &stmt, NULL) != 0) {
        return -1;
    }

    //TODO should create reusable function for db error reporting like below
    int rc = 0;
    if((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        mvprintw(0,0, "error creating table \"tasks\": %s\n",
                sqlite3_errmsg(db));
        printw("retcode = %d\n", rc);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        getch();
        return -1;
    }

    sqlite3_finalize(stmt);

    strcpy(sql_txt, 
        "CREATE TABLE \"archive\""
        "(`id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,"
        "`parent_id` INTEGER,"
        "`name` TEXT NOT NULL,"
        "`created_time` INTEGER,"
        "`status_time` INTEGER,"
        "`original_time_estimate` INTEGER,"
        "`time_estimated` INTEGER,"
        "`time_spent` INTEGER,"
        "`status` INTEGER,"
        "`notes` TEXT )");

    if(compile_sql(db, sql_txt, -1, 0, &stmt, NULL) != 0) {
        return -1;
    }

    //TODO should create reusable function for db error reporting like below
    rc = 0;
    if((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        mvprintw(0,0, "error creating table \"archive\": %s\n",
                sqlite3_errmsg(db));
        printw("retcode = %d\n", rc);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
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
