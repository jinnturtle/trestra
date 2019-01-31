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
#include "txt_editor.h"

void hack_terminal(struct Task *task);
char *cmd_line(int y, int x, char *txt, size_t n);
void hack_terminal_adv_history(char **hist, size_t hist_len);
void hack_terminal_add_ln(char **hist, size_t hist_len, char *ln, size_t n);

#endif //define HACK_TERMINAL_H
