# termato

`termato` is a simple pomodoro timer for the terminal which can run arbitrary
commands upon state change.

![Screenshot](./screenshot.png)

Licensed under [GPL-3.0](./LICENCE).

### Examples

You can use the `--notify` flag to run a system command upon state change. For
example, if you had `notify-send` installed, you could run
```sh
$ termato --notify "notify-send 'termato: %s'"
```
The `%s` format specifier is replaced with information about the new work or
break period.

<img src="./notification_screenshot.png" width="20%"/>

### Usage
```
Usage: termato [options]

Keys:
space   pause/resume timer
n       skip to the next period
q       quit

Options:
      --focus <minutes> (default 25)
        Specify the duration of focus periods
      --short-break <minutes> (default 5)
        Specify the duration of short break periods
      --long-break <minutes> (default 20)
        Specify the duration of long break periods
      --sessions <num> (default 4)
        Specify the number of work periods before a long break
      --notify <command>
        Specify a command to run upon timer state change.
        The first occurrence of '%s' will be replaced by a string
        indicating the new timer state
  -h, --help
        Print this help and exit
      --version
        Print version information and exit
```

### Building

You will need `ncurses` and a C99 compiler. Build `src/main.c`, passing 
`-lncurses` to link `ncurses`. For example:
```
$ cc -lncurses src/main.c -o termato 
```
