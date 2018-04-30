//std c lib
#include <stdio.h>
#include <string.h>
#include <time.h>

//system (unix/linux)
#include <unistd.h>

//3rd party
#include "sqlite3.h"

#define PROGRAM_NAME "trestra - time resource tracker"
#define PROGRAM_VERSION "v0.0.1"

int print_tasks();
int timer_test();

int main(void)
{
    printf("*** %s %s ***\n", PROGRAM_NAME, PROGRAM_VERSION);

    char cmd[20];

    print_tasks();
    timer_test();

    return 0;
}

int print_tasks()
{
    char db_path[] = "dat/db.db";
    sqlite3 *db;

    if(sqlite3_open(db_path, &db) != SQLITE_OK) {
        fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    char sql_txt[] = "select * from tasks";
    sqlite3_stmt *stmt;

    if(sqlite3_prepare_v3(db, sql_txt, strlen(sql_txt), 0, &stmt, NULL)
       != SQLITE_OK)
    {
        fprintf(stderr, "error while compiling sql: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }

    int rc = 0;
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        char name[64];
        strcpy(name, sqlite3_column_text(stmt, 3));
        int estimate = sqlite3_column_int(stmt, 4);
        int fact = sqlite3_column_int(stmt, 5);

        double progress_perc = (100.0d / estimate) * fact;
        printf("task: \"%s\" progress: %0.2lf\n", name, progress_perc);
    }
    if(rc != SQLITE_DONE) {
        fprintf(stderr, "error querying the db: %s\n", sqlite3_errmsg(db));
        fprintf(stderr, "retcode = %d\n", rc);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int timer_test()
{
    printf("--- time test ----------------------------------------\n");

    time_t s_tm = time(NULL);
    char cmd[80];

    while(strcmp(cmd, "quit") != 0) {
        printf("time spent: %ld\n", time(NULL) - s_tm);

        printf("trestra> ");
        scanf("%s", &cmd[0]);
    }

    return 0;
}
