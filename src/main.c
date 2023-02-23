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
#define STR_CAP 256
#define DEFAULT_FOCUS (25 * MIN)
#define DEFAULT_BREAK_SHORT (5 * MIN)
#define DEFAULT_BREAK_LONG (20 * MIN)
#define DEFAULT_SESSIONS 4

typedef struct State {
    enum {
        STAGE_FOCUS,
        STAGE_BREAK_SHORT,
        STAGE_BREAK_LONG,
        STAGE_COUNT,
    } stage;
    usize session;
} State;
const char *stage_names[STAGE_COUNT] = {
    "Focus",
    "Relax",
    "Break",
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

    args_Flag focus_flag = {
        .name_short = 'f',
        .name_long = "focus",
        .help_text = "specify the length of focus periods, in minutes (default: 25)",
        .type = ARGS_SINGLE_OPT,
        .expects = ARGS_EXPECTS_NUM,
    };
    args_Flag break_short_flag = {
        .name_short = 'b',
        .name_long = "short-break",
        .help_text = "specify the length of short break periods, in minutes (default: 5)",
        .type = ARGS_SINGLE_OPT,
        .expects = ARGS_EXPECTS_NUM,
    };
    args_Flag break_long_flag = {
        .name_short = 'l',
        .name_long = "long-break",
        .help_text = "specify the length of long break periods, in minutes (default: 20)",
        .type = ARGS_SINGLE_OPT,
        .expects = ARGS_EXPECTS_NUM,
    };
    args_Flag notify_flag = {
        .name_short = 'n',
        .name_long = "notify",
        .help_text = "specify a command to run upon timer state change (default: none)\nThe first "
                     "occurrence of `%s` will be substituted with the new timer state",
        .type = ARGS_SINGLE_OPT,
        .expects = ARGS_EXPECTS_STRING,
    };
    args_Flag sessions_flag = {
        .name_short = 's',
        .name_long = "sessions",
        .help_text = "specify the number of work periods before a long break (default: 4)",
        .type = ARGS_SINGLE_OPT,
        .expects = ARGS_EXPECTS_NUM,
    };
    args_Flag *flags[] = {
        &focus_flag,
        &break_short_flag,
        &break_long_flag,
        &sessions_flag,
        &notify_flag,
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

    int flag_check_return = flagIsNum(focus_flag);
    if (flag_check_return != ARGS_RETURN_CONTINUE) return flag_check_return;
    flag_check_return = flagIsNum(break_short_flag);
    if (flag_check_return != ARGS_RETURN_CONTINUE) return flag_check_return;
    flag_check_return = flagIsNum(break_long_flag);
    if (flag_check_return != ARGS_RETURN_CONTINUE) return flag_check_return;
    flag_check_return = flagIsNum(sessions_flag);
    if (flag_check_return != ARGS_RETURN_CONTINUE) return flag_check_return;

    usize durations[STAGE_COUNT] = {
        DEFAULT_FOCUS,
        DEFAULT_BREAK_SHORT,
        DEFAULT_BREAK_LONG,
    };

    durations[STAGE_FOCUS] = focus_flag.is_present ? atoi(focus_flag.opts[0]) * MIN : DEFAULT_FOCUS;
    durations[STAGE_BREAK_SHORT] =
        break_short_flag.is_present ? atoi(break_short_flag.opts[0]) * MIN : DEFAULT_BREAK_SHORT;
    durations[STAGE_BREAK_LONG] =
        break_long_flag.is_present ? atoi(break_long_flag.opts[0]) * MIN : DEFAULT_BREAK_LONG;
    usize sessions = sessions_flag.is_present ? atoi(sessions_flag.opts[0]) * MIN : DEFAULT_SESSIONS;

    time_t end_time = time(NULL) + durations[STAGE_FOCUS];
    usize remaining_time = durations[STAGE_FOCUS];

    State state = {
        .stage = STAGE_FOCUS,
        .session = 1,
    };

    bool should_quit = false;

    char stage_str[STR_CAP];

    int width = 0, height = 0;
    initscr();
    cbreak();
    curs_set(0);
    noecho();
    nodelay(stdscr, TRUE);
    for (;; napms(1), erase(), refresh()) {
        getmaxyx(stdscr, height, width);

        bool paused = false;

        if (remaining_time == 0) {
            if (notify_flag.is_present) {
                usize notify_len = strlen(notify_flag.opts[0]) + STR_CAP;
                char notify_str[notify_len];
                snprintf(notify_str, notify_len, notify_flag.opts[0], "\"%s - %ld minutes\"");
                char notify_str_2[notify_len];
                snprintf(notify_str_2, notify_len, notify_str, stage_str, durations[state.stage] / MIN);
                system(notify_str_2);
            }
            bool completed_set = !(state.session % sessions);
            if (state.stage == STAGE_FOCUS) {
                if (completed_set) {
                    state.stage = STAGE_BREAK_LONG;
                    end_time = time(NULL) + durations[STAGE_BREAK_LONG];
                }
                else {
                    state.stage = STAGE_BREAK_SHORT;
                    end_time = time(NULL) + durations[STAGE_BREAK_SHORT];
                }
            }
            else {
                if (completed_set) state.session = 1;
                else state.session += 1;
                state.stage = STAGE_FOCUS;
                end_time = time(NULL) + durations[STAGE_FOCUS];
            }
        }

        remaining_time = end_time - time(NULL);
        usize remaining_mins = remaining_time / MIN;
        usize remaining_secs = remaining_time % MIN;

        char remaining_time_str[STR_CAP];
        snprintf(remaining_time_str, STR_CAP, "%02ld:%02ld", remaining_mins, remaining_secs);
        mvaddstr(height / 2 - 1, (width - strlen(remaining_time_str)) / 2, remaining_time_str);

        if (state.stage == STAGE_FOCUS) {
            snprintf(stage_str, STR_CAP, "%s (%ld/%ld)", stage_names[state.stage], state.session, sessions);
        }
        else {
            snprintf(stage_str, STR_CAP, "%s", stage_names[state.stage]);
        }
        mvaddstr(height / 2, (width - strlen(stage_str)) / 2, stage_str);

        timeout(1000);

        int c = getch();
        if (c == 'q') break;
        if (c == ' ') paused = true;
        if (paused) {
            time_t pause_time = time(NULL);
            for (;;) {
                napms(1);
                erase();
                getmaxyx(stdscr, height, width);
                const char *pause_text = "[Paused]";
                mvaddstr(height / 2 - 1, (width - strlen(remaining_time_str)) / 2, remaining_time_str);
                mvaddstr(height / 2, (width - strlen(stage_str)) / 2, stage_str);
                mvaddstr(height / 2 + 1, (width - strlen(pause_text)) / 2, pause_text);
                refresh();

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
