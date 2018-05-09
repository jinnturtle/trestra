#include "Task.h" //header for this implementation file

void print_task(char *_fmt, struct Task *_task)
{
    char est_buf[20];
    char fact_buf[20];
    char parent_indicator[2] = "";

    if(_task->is_parent > 0) {
       strcpy(parent_indicator, "+");
    }

    printw("%u%s: \"%s\" [%s/%s (%0.2lf%)]",
            _task->id, parent_indicator, _task->name,
            format_time_str(_fmt, _task->fact, fact_buf),
            format_time_str(_fmt, _task->estimate, est_buf),
            (100.0d / _task->estimate) * _task->fact);
}

int task_pager(struct Task *_tasks, size_t _n)
{
    if(_tasks == NULL || _n < 1) {
        clear();
        mvprintw(0,0, "*** nothing to display ***");
        getch();
        return 1;
    }

    unsigned ls_top = 0;
    unsigned ls_bot = 0;
    unsigned ls_top_max = _n - 1;
    int max_y = getmaxy(stdscr) - 1;
    
    int cmd = 0;

    while(cmd != 'q') {
        clear();
        unsigned i = 0;
        for(; i < _n && i < max_y && ls_top + i < _n; ++i) {
            move(i,0);
            print_task("hm", &_tasks[ls_top + i]);
        }
        ls_bot = ls_top - 1 + i;

        mvprintw(max_y, 0, "--- tasks %u-%u (%u/%u) ---",
                _tasks[ls_top].id, _tasks[ls_bot].id,
                ls_bot + 1, ls_top_max + 1);

        cmd = getch();
        switch(cmd) {
        case 'j':
            if(ls_top < ls_top_max) { ++ls_top; }
            break;
        case 'k':
            if(ls_top > 0) { --ls_top; }
            break;
        case 'q': continue; break;
        }
    }

    return 0;
}

int task_selector(struct Task *_tasks, size_t _n, int *sel_id_)
{
    if(_tasks == NULL || _n < 1) {
        clear();
        mvprintw(0,0, "*** nothing to display ***");
        getch();
        return 'q';
    }

    unsigned ls_top = 0;
    unsigned ls_bot = 0;
    unsigned ls_top_max = _n - 1;
    unsigned ls_sel = 0;
    int max_y = getmaxy(stdscr) - 1;
    
    int cmd = 0;

    while(cmd != 'q') {
        clear();
        unsigned i = 0;
        for(; i < _n && i < max_y && ls_top + i < _n; ++i) {
            move(i,0);
            if(ls_sel == ls_top + i) { attron(A_REVERSE); }
            print_task("hm", &_tasks[ls_top + i]);
            if(ls_sel == ls_top + i) { attroff(A_REVERSE); }
        }
        ls_bot = ls_top - 1 + i;

        mvprintw(max_y, 0, "--- tasks %u-%u (%u/%u) ---",
                _tasks[ls_top].id, _tasks[ls_bot].id,
                ls_bot + 1, ls_top_max + 1);

        cmd = getch();
        switch(cmd) {
        case 'j':
            if(ls_top < ls_top_max && ls_sel >= ls_bot) { ++ls_top; }
            if(ls_sel < _n && ls_sel < ls_top_max) { ++ls_sel; }
            break;
        case 'k':
            if(ls_top > 0 && ls_sel <= ls_top) { --ls_top; }
            if(ls_sel > 0) { --ls_sel; }
            break;
        case 'l':
            *sel_id_ = _tasks[ls_sel].id;
            if(_tasks[ls_sel].is_parent == 0) { return 's'; }
            else { return cmd; }
            break;
        case 's':
        case 'h':
        case 'q': return cmd; break;
        }
    }

    return 0;
}

//TODO this should probs go to some other source file
int print_from_stmt_short(sqlite3_stmt *_stmt, sqlite3* _db)
{
    char name[64] = { 0 };
    char fact_buf[20] = { 0 };
    char est_buf[20] = { 0 };

    int id = sqlite3_column_int(_stmt, 0);
    strcpy(name, sqlite3_column_text(_stmt, 1));
    int estimate = sqlite3_column_int(_stmt, 2);
    int fact = sqlite3_column_int(_stmt, 3);

    double progress_perc = (100.0d / estimate) * fact;

    char parent_indicator[2] = "";
    if(check_for_children(_db, sqlite3_column_int(_stmt, 0))) {
        strcpy(parent_indicator, "+");
    }

    printw("%d%s: \"%s\" [%s/%s (%0.2lf%)]\n",
            id, parent_indicator, name,
            format_time_str("hm", fact, fact_buf),
            format_time_str("hm", estimate, est_buf),
            progress_perc);
}

void task_init_form_stmt(sqlite3_stmt *_stmt, struct Task *_task)
{
    _task->id = sqlite3_column_int(_stmt, 0);
    _task->parent_id = sqlite3_column_int(_stmt, 1);
    strcpy(_task->name, sqlite3_column_text(_stmt, 2));
    _task->creation_time = sqlite3_column_int(_stmt, 3);
    _task->status_time = sqlite3_column_int(_stmt, 4);
    _task->orig_estimate = sqlite3_column_int(_stmt, 5);
    _task->estimate = sqlite3_column_int(_stmt, 6);
    _task->fact = sqlite3_column_int(_stmt, 7);
    _task->status = sqlite3_column_int(_stmt, 8);
}
