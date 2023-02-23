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
#define DEFAULT_WORK (25 * 60)
#define DEFAULT_BREAK_SHORT (5 * 60)
#define DEFAULT_BREAK_LONG (20 * 60)
#define DEFAULT_SESSIONS 4


int main(int argc, char **argv) {

    args_Flag work_flag = {
        .name_short = 'w',
        .name_long = "work",
        .help_text = "specify the length of time for work periods, in minutes (default 25)",
        .type = ARGS_SINGLE_OPT,
        .expects = ARGS_EXPECTS_NUM,
    };
    args_Flag break_short_flag = {
        .name_short = 'b',
        .name_long = "short-break",
        .help_text = "specify the length of time for short break periods, in minutes (default 5)",
        .type = ARGS_SINGLE_OPT,
        .expects = ARGS_EXPECTS_NUM,
    };
    args_Flag break_long_flag = {
        .name_short = 'l',
        .name_long = "long-break",
        .help_text = "specify the length of time for long break periods, in minutes (default 20)",
        .type = ARGS_SINGLE_OPT,
        .expects = ARGS_EXPECTS_NUM,
    };
    args_Flag sessions_flag = {
        .name_short = 's',
        .name_long = "sessions",
        .help_text = "specify the number of work periods before a long break (default 4)",
        .type = ARGS_SINGLE_OPT,
        .expects = ARGS_EXPECTS_NUM,
    };
    args_Flag *flags[] = {
        &work_flag,
        &break_short_flag,
        &break_long_flag,
        &sessions_flag,
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

    usize duration_work = work_flag.is_present ? atoi(work_flag.opts[0]) * 60 : DEFAULT_WORK;
    usize duration_break_short =
        break_short_flag.is_present ? atoi(break_short_flag.opts[0]) * 60 : DEFAULT_BREAK_SHORT;
    usize duration_break_long =
        break_long_flag.is_present ? atoi(break_long_flag.opts[0]) * 60 : DEFAULT_BREAK_LONG;
    usize sessions = sessions_flag.is_present ? atoi(sessions_flag.opts[0]) * 60 : DEFAULT_SESSIONS;

    time_t start_time = time(NULL);
    time_t end_time = start_time + duration_work;

    int width = 0, height = 0;
    initscr();
    cbreak();
    curs_set(0);
    noecho();
    nodelay(stdscr, TRUE);
    for (;; napms(1), erase(), refresh()) {
        usize remaining_time = end_time - time(NULL);
        usize remaining_mins = remaining_time / 60;
        usize remaining_secs = remaining_time % 60;

        getmaxyx(stdscr, height, width);

        char remaining_time_str[TIME_STR_LEN];
        snprintf(remaining_time_str, TIME_STR_LEN, "%02ld:%02ld", remaining_mins, remaining_secs);
        mvaddstr(height / 2, (width - strlen(remaining_time_str)) / 2, remaining_time_str);

        timeout(1000);

        int c = getch();
        if (c == 'q') break;
    }
    endwin();

    return EXIT_SUCCESS;
}
