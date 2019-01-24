#include "hack_terminal.h" //the header for this implementation file

/*TODO after moving find_task() to db_uitls.h,
 * make this function to only accept <int id> as argument*/
/*TODO implement the array used for history as a structure in a separate
 * header+implementation file pair and move/implement the necessary functions
 * there */
void hack_terminal(struct Task *task)
{
    dbg("*** hack terminal accessed ***\n");

    const size_t ln_len = 81;
    const size_t hist_len = 20;
    const int hist_window_h = 20;
    int y_pos = 0;
    char cmd[ln_len];
    cmd[0] = '\0';
    //char hist[hist_len][ln_len + 1];
    char **hist;
    hist = malloc(hist_len * sizeof *hist);
    for(size_t i = 0; i < hist_len; ++i) {
        hist[i] = malloc(ln_len * sizeof *hist[i]);
        hist[i][0] = '\0';
        if(i == hist_len - 1) {dbgf("terminal lines allocated[0 - %ld]\n", i);}
    }
    dbgf("memory allocated for terminal: %ldb\n",
        (hist_len * sizeof *hist) + (hist_len * (ln_len * sizeof *hist[0])));

    clear();

    hack_terminal_adv_history(hist, hist_len);
    strcpy(hist[0], "*** Wellcome to the Hacker's Terminal ***");

    while(strcmp(cmd, "/exit") != 0) {
        if(strcmp(cmd, "") != 0) {
            hack_terminal_adv_history(hist, hist_len);
            strcpy(hist[0], cmd);
        }

        y_pos = 0;

        //print history
        for(int i = 0; i < hist_window_h; ++i) {
            mvprintw(y_pos++, 0, ":%s", hist[hist_window_h - 1 - i]);
            clrtoeol();
        }

        cmd_line(y_pos, 0, cmd, (sizeof cmd) - 1);

        /*TODO [low priority] this could grow into a lot of if statements,
         * implement a key-value map */
        if(strcmp(cmd, "/print_task") == 0) {
            //TODO
            char txt_buf[ln_len];
            task_to_str(task, txt_buf, sizeof txt_buf);
            hack_terminal_adv_history(hist, hist_len);
            strcpy(hist[0], txt_buf);
            strcpy(cmd, "");
        }
    }

    for(size_t i = 0; i < hist_len; ++i) {
        free(hist[i]);
    }
    free(hist);
}

//advance history
void hack_terminal_adv_history(char **hist, size_t hist_len)
{
    size_t head_pos = hist_len - 1;

    for(size_t i = 0; i < head_pos; ++i) {
        /*TODO optimize, no need to copy all bytes if we can simply use
         * pointers to strings*/
        strcpy(hist[head_pos - i], hist[head_pos - i - 1]);
    }
}

char *cmd_line(int y, int x, char *txt, size_t n)
{
    memset(txt, '\0', n);

    size_t caret = 0;
    int inp = 0;
    while(inp != '\n') {
        move(y, x);
        clrtoeol();
        printw(">%s ", txt);

        inp = getch();

        if(inp == '\n') {break;}
        else if(is_backspace(inp) && caret > 0) {
            --caret;
            txt[caret] = '\0';
        }
        else if(isprint(inp) && caret < n) {
            txt[caret] = (char) inp;
            ++caret;
        }
    }

    return txt;
}
