#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <ncurses.h>

#define ARGS_BINARY_NAME "termato"
#define ARGS_BINARY_VERSION "0.1-dev"
#define ARGS_IMPLEMENTATION
#include "./args.h"
#include "./better_int_types.h"


#define TIME_STR_LEN 16


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

    usize duration = 5;
    time_t start_time = time(NULL);
    time_t end_time = start_time + duration;

    int width = 0, height = 0;
    initscr();
    cbreak();
    curs_set(0);
    noecho();
    nodelay(stdscr, TRUE);
    for (;; napms(1), erase(), refresh()) {
        usize remaining_time = end_time - time(NULL);

        getmaxyx(stdscr, height, width);

        char remaining_time_str[TIME_STR_LEN];
        snprintf(remaining_time_str, TIME_STR_LEN, "%ld", remaining_time);
        mvaddstr(height / 2, (width - strlen(remaining_time_str)) / 2, remaining_time_str);

        int c = getch();
        if (c == 'q' || remaining_time == 0) break;
    }
    endwin();

    return EXIT_SUCCESS;
}
