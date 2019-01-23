#include "hack_terminal.h" //the header for this implementation file

//void hack_terminal(struct Task *task)
void hack_terminal()
{
    const size_t ln_len = 80;
    const size_t hist_len = 20;
    const int hist_window_y = 1;
    const int hist_window_h = 20;
    char cmd[ln_len + 1];
    cmd[0] = '\0';
    //char hist[hist_len][ln_len + 1];
    char **hist;
    hist = malloc(hist_len * sizeof *hist);
    for(size_t i = 0; i < hist_len; ++i) {
        dbgf("allocating hist[%ld]\n", i);
        hist[i] = malloc(ln_len * sizeof *hist[i]);
        hist[i][0] = '\0';
    }
    dbgf("mem for terminal: %ldb\n",
        (hist_len * sizeof *hist) + (hist_len * (ln_len * sizeof *hist[0])));

    clear();

    while(strcmp(cmd, "/exit") != 0) {
        mvprintw(0, 0, "*** Wellcome to the Hacker's Terminal ***");

        strcpy(hist[0], cmd);

        hack_terminal_adv_history(hist, hist_len);
        //hack_terminal_adv_history(hist, 2);

        //print history
        for(int i = 0; i < hist_window_h; ++i) {
            mvprintw(hist_window_h - i, 0, ":%s", hist[i]);
            clrtoeol();
        }

        memset(cmd, '\0', sizeof cmd);
        cmd_line(23, 0, cmd, (sizeof cmd) - 1);

        /*TODO [low priority] this could grow into a lot of if statements,
         * implement a key-value map */
        if(strcmp(cmd, "/print_task") == 0) {
            //TODO
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
