#include "task_explorer.h"

struct task_explorer_context {
    char* scene_name;
    struct task_explorer_action* actions;
    size_t actions_n;
    struct Task *tasks;
    size_t tasks_n;
    unsigned ls_top;
    unsigned ls_bot;
    unsigned ls_top_max;
    unsigned ls_sel;
    int max_y;
};

struct task_explorer_action {
    const char* name;
    int key;
    int(*func) (struct task_explorer_context*, int);
};

// select task below in the list
static int action_down(struct task_explorer_context* ctx, int cmd) {
    if(ctx->ls_top < ctx->ls_top_max && ctx->ls_sel >= ctx->ls_bot) {
        ++ctx->ls_top;
    }
    if(ctx->ls_sel < ctx->tasks_n && ctx->ls_sel < ctx->ls_top_max) { ++ctx->ls_sel; }
    return 0;
}

// select task above in the list
static int action_up(struct task_explorer_context* ctx, int cmd) {
    if(ctx->ls_top > 0 && ctx->ls_sel <= ctx->ls_top) { --ctx->ls_top; }
    if(ctx->ls_sel > 0) { --ctx->ls_sel; }
    return 0;
}

// list child tasks, or selects the current task if it has none
static int action_list(struct task_explorer_context* ctx, int cmd) {
    if(ctx->tasks[ctx->ls_sel].is_parent == 0) { return 's'; }
    else { return cmd; }
}

// go back (up a level in the tasks tree)
static int action_back(struct task_explorer_context* ctx, int cmd) {
    // TODO returning a magic number/char is rather ambiguous
    return 'h';
}

static int action_menu(struct task_explorer_context* ctx, int cmd) {
    /* TODO is it realy necessary to let the calling code to handle this,
     * looks unnecessarily obscured?*/
    // TODO returning a magic number/char is rather ambiguous
    return 'm';
}

// shows info about the highlighted task
static int action_info(struct task_explorer_context* ctx, int cmd) {
    /* TODO is it realy necessary to let the calling code to handle this,
     * looks unnecessarily obscured?*/
    // TODO returning a magic number/char is rather ambiguous
    return 'i';
}

static int action_quit(struct task_explorer_context* ctx, int cmd) {
    // TODO returning a magic number/char is rather ambiguous
    return 'q';
}

// open text edit mode for task notes
static int action_txtedit(struct task_explorer_context* ctx, int cmd) {
    txt_editor(
        ctx->tasks[ctx->ls_sel].notes, strlen(ctx->tasks[ctx->ls_sel].notes));
    return 0;
}

// open the hack teminal
static int action_hackterm(struct task_explorer_context* ctx, int cmd) {
    hack_terminal(&ctx->tasks[ctx->ls_sel]);
    return 0;
}

// lists key bindings and maybe other useful info
static int action_help(struct task_explorer_context* ctx, int cmd) {
    clear();
    move(0, 0);
    printw("*** %s - Help ***\n", ctx->scene_name);
    printw("\n--- Key Bindings ---\n");
    for (size_t i = 0; i < ctx->actions_n; ++i) {
        printw("%c - %s\n", ctx->actions[i].key, ctx->actions[i].name);
    }
    printw("\n\npress any key...");
    getch();
    
    return 0;
}

int task_explorer(struct Task *_tasks, size_t _n, int *sel_id_)
{
    if(_tasks == NULL || _n < 1) {
        clear();
        mvprintw(0,0, "*** nothing to display ***");
        getch();
        return 'q';
    }

    struct task_explorer_action actions[] = {
        {
            .name = "help",
            .key = '?',
            .func = action_help
        },
        {
            .name = "down",
            .key = 'j',
            .func = action_down
        },
        {
            .name = "up",
            .key = 'k',
            .func = action_up
        },
        {
            .name = "list/select",
            .key = 'l',
            .func = action_list
        },
        {
            .name = "go back a level",
            .key = 'h',
            .func = action_back
        },
        {
            .name = "open menu",
            .key = 'm',
            .func = action_menu
        },
        {
            .name = "show info about selected task",
            .key = 'i',
            .func = action_info
        },
        {
            .name = "edit notes (BETA)",
            .key = 'X',
            .func = action_txtedit
        },
        {
            .name = "quit",
            .key = 'q',
            .func = action_quit
        },
        {
            .name = "open hacker terminal",
            .key = '`',
            .func = action_hackterm
        }
    };
    size_t actions_n = sizeof(actions)/sizeof(actions[0]);
    
    struct task_explorer_context ctx = {
        .scene_name = "Task Explorer",
        .actions = actions,
        .actions_n = actions_n,
        .tasks = _tasks,
        .tasks_n = _n,
        .ls_top = 0,
        .ls_bot = 0,
        .ls_top_max = _n - 1,
        .max_y = getmaxy(stdscr) - 1
    };

    //selecting highlight position to the correct id if sel_id_ is not 0
    if(*sel_id_ == 0) { *sel_id_ = _tasks[ctx.ls_sel].id; }
    while(_tasks[ctx.ls_sel].id != *sel_id_) {
        if(ctx.ls_sel == _n - 1) {
            ctx.ls_sel = 0;
            break;
        }

        ++ctx.ls_sel;
    }
    
    int cmd = 0;

    while(cmd != 'q') {
        clear();
        unsigned i = 0;
        for(; i < _n && i < ctx.max_y && ctx.ls_top + i < _n; ++i) {
            move(i,0);
            if(_tasks[ctx.ls_top + i].id == *sel_id_) { attron(A_REVERSE); }
            print_task("hm", &_tasks[ctx.ls_top + i]);
            if(_tasks[ctx.ls_top + i].id == *sel_id_) { attroff(A_REVERSE); }
        }
        ctx.ls_bot = ctx.ls_top - 1 + i;

        mvprintw(ctx.max_y, 0, "--- tasks %u-%u (%u/%u) ---",
                _tasks[ctx.ls_top].id, _tasks[ctx.ls_bot].id,
                ctx.ls_bot + 1, ctx.ls_top_max + 1);

        cmd = getch();
        for (size_t i = 0; i < actions_n; ++i) {
            if (cmd == actions[i].key) {
                int rc = actions[i].func(&ctx, cmd);
                // some tasks need to be handled by the calling code (e.g. back)
                if (rc != 0) { return rc; }
            }
        }
        
        *sel_id_ = _tasks[ctx.ls_sel].id;
    }

    return 0;
}
