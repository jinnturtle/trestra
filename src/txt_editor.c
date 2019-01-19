#include "txt_editor.h" //header for this implementation file

int txt_editor(char *_txt, size_t _len)
{
    clear();

    size_t caret = 0;

    int cmd = 0;
    while(cmd != 'q') {
        //TODO display task name
        mvprintw(0,0, "*** Text Editor ***");

        mvprintw(1,0, "%s", _txt);

        //TODO - improve to handle text longer than one line, functionize
        //paint caret marker
        attron(A_REVERSE);
        mvaddch(1,caret, _txt[caret]);
        attroff(A_REVERSE);

        cmd = getch();

        switch(cmd) {
        case 'i': txt_edit_mode(_txt, _len, &caret); break;
        case 'h': if(caret > 0) {--caret;} break;
        case 'l': if(caret < _len - 1) {++caret;} break;
        case 'q': break;
        default: break;
        }
    }

    return 0;
}

//lets one edit text and quit this mode with ESC
int txt_edit_mode(char *_txt, size_t _len, size_t *_caret)
{
    const int esc_key = 27; //also the code for ALT

    clear();

    int cmd = 0;
    while(cmd != esc_key) {
        mvprintw(1,0, "%s", _txt);
        
        cmd = getch();
        if(is_backspace(cmd)) {
            //TODO just a temporary solution below, do proper implementation
            _txt[*_caret] = ' ';
            if(*_caret > 0) {--*_caret;}
        }
        else if(!iscntrl(cmd)) {
            _txt[*_caret] = (char) cmd;
            if(*_caret < _len - 1) {++*_caret;}
        }
    }

    return 0;
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
