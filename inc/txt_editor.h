#ifndef TXT_EDITOR_H
#define TXT_EDITOR_H

//std lib
#include <ctype.h>

//system (unix/linux)
#include <ncurses.h>
#include <unistd.h>

//homebrew
#include "utils.h"

int txt_editor(char *_txt, size_t _len);
int txt_edit_mode(char *_txt, size_t _len, size_t *_caret);

#endif //define TXT_EDITOR_H
