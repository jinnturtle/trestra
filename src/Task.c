#include "Task.h" //header for this implementation file

void print_task(struct Task *_task) {
    char est_buf[20];
    char fact_buf[20];

    printw("%u: \"%s\" [%s/%s (%0.2lf%)]",
            _task->id, _task->name,
            format_time_str("hm", _task->fact, fact_buf),
            format_time_str("hm", _task->estimate, est_buf),
            (100.0d / _task->estimate) * _task->fact);
}

//TODO this should probs go to some other source file
int print_from_stmt_short(sqlite3_stmt *_stmt, sqlite3* _db) {
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
