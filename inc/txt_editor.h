//std lib
#include "ctype.h"

//system (unix/linux)
#include <ncurses.h>
#include <unistd.h>

int txt_editor(char *_txt, size_t _len);
int txt_edit_mode(char *_txt, size_t _len, size_t *_caret);
int is_backspace(int _key);
