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

#define MIN 1
#define STR_CAP 32
#define DEFAULT_WORK (25 * MIN)
#define DEFAULT_BREAK_SHORT (5 * MIN)
#define DEFAULT_BREAK_LONG (20 * MIN)
#define DEFAULT_SESSIONS 4

typedef struct State {
    enum {
        STAGE_WORK,
        STAGE_BREAK_SHORT,
        STAGE_BREAK_LONG,
        STAGE_COUNT,
    } stage;
    usize session;
} State;
const char *stage_names[STAGE_COUNT] = {
    "Focus",
    "Short break",
    "Long break",
};


bool strIsDigits(char *str) {
    for (usize i = 0; i < strlen(str); i++) {
        if (str[i] < '0' || str[i] > '9') return false;
    }
    return true;
}


int flagIsNum(args_Flag flag) {
    if (!flag.is_present) return ARGS_RETURN_CONTINUE;
    if (!strIsDigits(flag.opts[0])) {
        printf("%s: '%s' is not a valid numeric value\n", ARGS_BINARY_NAME, flag.opts[0]);
        args_helpHint();
        return EX_USAGE;
    }
    return ARGS_RETURN_CONTINUE;
}


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

    int flag_check_return = flagIsNum(work_flag);
    if (flag_check_return != ARGS_RETURN_CONTINUE) return flag_check_return;
    flag_check_return = flagIsNum(break_short_flag);
    if (flag_check_return != ARGS_RETURN_CONTINUE) return flag_check_return;
    flag_check_return = flagIsNum(break_long_flag);
    if (flag_check_return != ARGS_RETURN_CONTINUE) return flag_check_return;
    flag_check_return = flagIsNum(sessions_flag);
    if (flag_check_return != ARGS_RETURN_CONTINUE) return flag_check_return;

    usize duration_work = work_flag.is_present ? atoi(work_flag.opts[0]) * MIN : DEFAULT_WORK;
    usize duration_break_short =
        break_short_flag.is_present ? atoi(break_short_flag.opts[0]) * MIN : DEFAULT_BREAK_SHORT;
    usize duration_break_long =
        break_long_flag.is_present ? atoi(break_long_flag.opts[0]) * MIN : DEFAULT_BREAK_LONG;
    usize sessions = sessions_flag.is_present ? atoi(sessions_flag.opts[0]) * MIN : DEFAULT_SESSIONS;

    time_t end_time = time(NULL) + duration_work;
    usize remaining_time = duration_work;

    State state = {
        .stage = STAGE_WORK,
        .session = 1,
    };

    bool should_quit = false;

    int width = 0, height = 0;
    initscr();
    cbreak();
    curs_set(0);
    noecho();
    nodelay(stdscr, TRUE);
    for (;; napms(1), erase(), refresh()) {
        getmaxyx(stdscr, height, width);
        usize target_y = height / 2;
        usize target_x = 0;

        if (remaining_time == 0) {
            bool completed_set = !(state.session % sessions);
            if (state.stage == STAGE_WORK) {
                if (completed_set) {
                    state.stage = STAGE_BREAK_LONG;
                    end_time = time(NULL) + duration_break_long;
                }
                else {
                    state.stage = STAGE_BREAK_SHORT;
                    end_time = time(NULL) + duration_break_short;
                }
            }
            else {
                if (completed_set) state.session = 1;
                else state.session += 1;
                state.stage = STAGE_WORK;
                end_time = time(NULL) + duration_work;
            }
        }

        remaining_time = end_time - time(NULL);
        usize remaining_mins = remaining_time / MIN;
        usize remaining_secs = remaining_time % MIN;

        char remaining_time_str[STR_CAP];
        snprintf(remaining_time_str, STR_CAP, "%02ld:%02ld", remaining_mins, remaining_secs);
        target_x = (width - strlen(remaining_time_str)) / 2;
        mvaddstr(target_y - 1, (width - strlen(remaining_time_str)) / 2, remaining_time_str);

        char stage_str[STR_CAP];
        if (state.stage == STAGE_WORK) {
            snprintf(stage_str, STR_CAP, "%s (%ld/%ld)", stage_names[state.stage], state.session, sessions);
        }
        else {
            snprintf(stage_str, STR_CAP, "%s", stage_names[state.stage]);
        }
        mvaddstr(target_y, (width - strlen(stage_str)) / 2, stage_str);

        timeout(1000);

        int c = getch();
        if (c == 'q') break;
        if (c == ' ') {
            time_t pause_time = time(NULL);
            const char *pause_text = "[Paused]";
            mvaddstr(target_y + 1, (width - strlen(pause_text)) / 2, pause_text);
            refresh();
            for (;;) {
                c = getch();
                if (c == ' ') {
                    end_time += time(NULL) - pause_time;
                    break;
                }
                else if (c == 'q') {
                    should_quit = true;
                    break;
                }
            }
        }
        if (should_quit) break;
    }
    endwin();

    return EXIT_SUCCESS;
}
