//A command-line style utility for debugging, experimenting, etc
#ifndef HACK_TERMINAL_H
#define HACK_TERMINAL_H

//std lib
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

//homebrew
#include "utils.h"
#include "Task.h"
#include "dbg.h"

//void hack_terminal(struct Task *task);
void hack_terminal();
char *cmd_line(int y, int x, char *txt, size_t n);
void hack_terminal_adv_history(char **hist, size_t hist_len);

#endif //define HACK_TERMINAL_H
