#include "base.c"

#define version_lit "0.5"
const Str8 version_text = str8("termato version " version_lit "\n");

const Str8 help_text = str8(
"termato - pomodoro timer (version " version_lit ")\n"
"\n"
"Usage: termato [options]\n"
"\n"
"Keys:\n"
"space   pause/resume timer\n"
"n       skip to the next period\n"
"q       quit\n"
"\n"
"Options:\n"
"      --focus <minutes> (default 25)\n"
"        Specify the duration of focus periods\n"
"      --short-break <minutes> (default 5)\n"
"        Specify the duration of short break periods\n"
"      --long-break <minutes> (default 20)\n"
"        Specify the duration of long break periods\n"
"      --sessions <num> (default 4)\n"
"        Specify the number of work periods before a long break\n"
"      --notify <command>\n"
"        Specify a command to run upon timer state change.\n"
"        The first occurrence of '%s' will be replaced by a string\n"
"        indicating the new timer state\n"
"  -h, --help\n"
"        Print this help and exit\n"
"      --version\n"
"        Print version information and exit\n"
);

#include "args.c"
#include <ncurses.h>
#include <time.h>

const char *stage_names[3] = { "Focus", "Relax", "Break" };

static error ensure_str_is_numeric(Str8 s) {
    for (usize i = 0; i < s.len; i += 1) {
        if (s.ptr[i] >= '0' && s.ptr[i] <= '9') continue;
        return errf("'%.*s' is not a valid number", s);
    }
    return true;
}

typedef struct {
    Arena arena;
    int argc;
    char **argv;
    enum {
        stage_focus,
        stage_short_break,
        stage_long_break,
    } stage;
    usize session;
} Context;

static error main_wrapper(Context *ctx) {
    try (arena_init(&ctx->arena, 4 * 1024));

    Args_Flag focus_flag = { 
        .name = str8("focus"), 
        .kind = args_kind_single_pos,
    };
    Args_Flag short_break_flag = { 
        .name = str8("short-break"), 
        .kind = args_kind_single_pos,
    };
    Args_Flag long_break_flag = { 
        .name = str8("long-break"), 
        .kind = args_kind_single_pos,
    };
    Args_Flag sessions_flag = { 
        .name = str8("sessions"), 
        .kind = args_kind_single_pos,
    };
    Args_Flag notify_flag = { 
        .name = str8("notify"), 
        .kind = args_kind_single_pos,
    };
    Args_Flag help_flag_short = { .name = str8("h") };
    Args_Flag help_flag_long = { .name = str8("help") };
    Args_Flag version_flag = { .name = str8("version") };
    Args_Flag *flags[] = {
        &focus_flag,
        &short_break_flag,
        &long_break_flag,
        &sessions_flag,
        &notify_flag,
        &help_flag_short, &help_flag_long,
        &version_flag,
    };
    Args_Desc args_desc = { .flags = slice(flags) };
    try (args_parse(&ctx->arena, ctx->argc, ctx->argv, &args_desc));

    if (help_flag_short.is_present || help_flag_long.is_present) {
        printf("%.*s", str8_fmt(help_text));
        return 0;
    }

    if (version_flag.is_present) {
        printf("%.*s", str8_fmt(version_text));
        return 0;
    }

    usize minute = 60;
    usize durations[3] = {
        [stage_focus] = focus_flag.is_present
            ? atoi((char *)focus_flag.single_pos.ptr) * minute
            : 25 * minute,
        [stage_short_break] = short_break_flag.is_present
            ? atoi((char *)short_break_flag.single_pos.ptr) * minute
            : 5 * minute,
        [stage_long_break] = long_break_flag.is_present
            ? atoi((char *)long_break_flag.single_pos.ptr)
            : 20 * minute,
    };

    usize sessions = sessions_flag.is_present
        ? atoi((char *)sessions_flag.single_pos.ptr)
        : 4;

    time_t end_time = time(NULL) + durations[stage_focus];
    usize remaining_time = durations[stage_focus];
    ctx->session = 1;

    bool should_quit = false;

    char stage_str[256]; snprintf(
        stage_str,
        256,
        "%s (%ld/%ld)",
        stage_names[ctx->stage], ctx->session, sessions
    );

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
            ctx->session %= sessions;
            if (ctx->stage == stage_focus) {
                if (ctx->session == 0) ctx->stage = stage_long_break; 
                else ctx->stage = stage_short_break;
                snprintf(stage_str, 256, "%s", stage_names[ctx->stage]);
            } else {
                ctx->session += 1;
                ctx->stage = stage_focus;
                snprintf(
                    stage_str,
                    256,
                    "%s (%ld/%ld)",
                    stage_names[ctx->stage],
                    ctx->session,
                    sessions
                );
            }

            end_time = time(NULL) + durations[ctx->stage];

            if (notify_flag.is_present) {
                char notify_str[256];
                snprintf(
                    notify_str,
                    256,
                    (char *)notify_flag.single_pos.ptr,
                    "%s - %ld minutes"
                );
                char notify_str_2[256];
                snprintf(
                    notify_str_2,
                    256,
                    notify_str,
                    stage_str,
                    durations[ctx->stage] / minute
                );
                (void)system(notify_str_2);
            }
        }

        remaining_time = end_time - time(NULL);
        usize remaining_mins = remaining_time / minute;
        usize remaining_secs = remaining_time % minute;

        char remaining_time_str[256];
        snprintf(
            remaining_time_str,
            256,
            "%02ld:%02ld",
            remaining_mins,
            remaining_secs
        );
        attron(A_BOLD);
        mvaddstr(
            height / 2 - 1,
            (width - strlen(remaining_time_str)) / 2,
            remaining_time_str
        );
        attroff(A_BOLD);
        mvaddstr(height / 2, (width - strlen(stage_str)) / 2, stage_str);

        timeout(1000);

        int c = getch();
        if (c == 'n') remaining_time = 0;
        if (c == 'q') break;
        if (c == ' ') paused = true;

        if (paused) {
            time_t pause_time = time(NULL);

            for (;;) {
                napms(1);
                erase();
                getmaxyx(stdscr, height, width);
                const char *pause_text = "[Paused]";
                mvaddstr(
                    height / 2 - 1,
                    (width - strlen(remaining_time_str)) / 2,
                    remaining_time_str);
                mvaddstr(
                    height / 2, (width - strlen(stage_str)) / 2, stage_str);
                mvaddstr(
                    height / 2 + 1,
                    (width - strlen(pause_text)) / 2,
                    pause_text);
                refresh();

                c = getch();
                if (c == ' ') {
                    end_time += time(NULL) - pause_time;
                    break;
                } else if (c == 'q') {
                    should_quit = true;
                    break;
                }
            }
        }
        if (should_quit) break;
    }

    endwin();

    return 0;
}

int main(int argc, char **argv) {
    Context ctx = { .argc = argc, .argv = argv };
    error e = main_wrapper(&ctx);
    arena_deinit(&ctx.arena);
    return e;
}
