#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <ncurses.h>

#define ARGS_BINARY_NAME "termato"
#define ARGS_BINARY_VERSION "0.1-dev"
#define ARGS_IMPLEMENTATION
#include "./args.h"
#include "./better_int_types.h"


int main(int argc, char **argv) {

    args_Flag *flags[] = {
        &ARGS_HELP_FLAG,
        &ARGS_VERSION_FLAG,
    };
    usize positional_num = 0;
    int args_return = args_proc((args_Proc_Args) {
        argc, argv,
        .flags_count = sizeof(flags) / sizeof(flags[0]),
        flags,
        .positional_num = &positional_num,
        .positional_cap = 0,
        .usage_description = "TUI pomodoro timer",
    });
    if (args_return != ARGS_RETURN_CONTINUE) return args_return;

    int width = 0, height = 0;

    initscr();
    for (getmaxyx(stdscr, height, width); getch() != 'q'; resizeterm(height, width), refresh()) {
        mvprintw(height / 2, (width - 11) / 2, "Hello, World!");
        // if (getch() == 'q') break;
    }
    endwin();

    return EXIT_SUCCESS;
}
