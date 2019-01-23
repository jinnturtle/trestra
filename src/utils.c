#include "utils.h" //header for this implementation file

char *format_time_str(char *_fmt, time_t _time, char *str_)
{
    char fmt[4] = { 0 };

    if(_fmt == NULL) { strcpy(fmt, "hms"); }
    else { strcpy(fmt, _fmt); }

    if(strcmp(fmt, "hms") == 0) {
        sprintf(str_, "%luh%lum%lus",
                _time / 3600, (_time % 3600) / 60, _time % 60);
    }
    else if(strcmp(fmt, "hm") == 0) {
        sprintf(str_, "%luh%lum",
                _time / 3600, (_time % 3600) / 60);
    }

    return str_;
}

time_t htime_to_time(char *_str, size_t _n)
{
    size_t n = _n - 1;
    time_t result = 0;
    time_t final_result = 0;

    for(unsigned i = 0; i < n; ++i) {
        if(_str[i] == '\0') { break; }

        if(isdigit(_str[i])) {
            result = (result * 10) + (_str[i] - '0');
        }
        else if(isalpha(_str[i])) {
            switch(_str[i]){
                case 'D': final_result += result * 3600 * 24; break;
                case 'd': final_result += result * 3600 * 8; break;
                case 'h': final_result += result * 3600; break;
                case 'm': final_result += result * 60; break;
                case 's': final_result += result; break;
                default: continue;
            }

            result = 0;
        }
    }

    return final_result;
}

char *time_to_htime(time_t _time, char *str_, size_t _n)
{
    time_t h = _time / 3600;
    time_t m = (_time % 3600) / 60;
    time_t s = _time % 60;
    snprintf(str_, _n, "%luh%lum%lus", h, m, s);

    return str_;
}

void print_mid(int _y, char *_str)
{
    int x = (getmaxx(stdscr) - strlen(_str)) / 2;
    mvprintw(_y, x, _str);
}

int open_db(const char *_path, sqlite3 **_db)
{
    if(sqlite3_open(_path, _db) != SQLITE_OK) {
        mvprintw(0,0, "Error opening database (%s): %s\n",
                _path,
                sqlite3_errmsg(*_db));
        getch();
        sqlite3_close(*_db);
        return 1;
    }

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

int check_for_children(sqlite3 *db, int _parent_id)
{
    char sql_txt[] = "SELECT id "
                     "FROM tasks WHERE parent_id = ? LIMIT 1";

    sqlite3_stmt *stmt;

    if(compile_sql(db, sql_txt, -1, 0, &stmt, NULL) != 0) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, _parent_id);

    int id_found = 0;

    int rc = 0;
    if((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        id_found = sqlite3_column_int(stmt, 0);
    }
    else if(rc != SQLITE_DONE) {
        mvprintw(0,0, "error querying the db: %s\n", sqlite3_errmsg(db));
        printw("retcode = %d\n", rc);
        sqlite3_finalize(stmt);
        getch();
        return -1;
    }

    sqlite3_finalize(stmt);

    if(id_found == 0) { return 0; }
    else { return 1; }
}

int is_backspace(int _key)
{
    if(_key == KEY_BACKSPACE ||
       _key == KEY_DC ||
       _key == 127 //some terminals seem to use this
      )
    {
        return 1;
    }
    else {return 0;}
}
