#include "utils/globals.h"
#include "console.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/epoll.h>

#define PROMPT "> "

static char input_buffer[CONSOLE_BUF_SIZE] = {0};
static size_t input_len = 0;
static struct termios old_tio;

// ---------- Internal helpers ----------

static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}

static void enable_raw_mode(void) {
    struct termios new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    atexit(disable_raw_mode);

    new_tio = old_tio;
    new_tio.c_lflag &= ~(ICANON | ECHO);
    new_tio.c_cc[VMIN] = 1;
    new_tio.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

static void make_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// ---------- Public API ----------

void console_redraw_prompt(void) {
    printf("\r\x1b[2K" PROMPT "%s", input_buffer);
    fflush(stdout);
}

const char *console_get_input(void) {
    return input_buffer;
}

#include "utils/globals.h"

void console_init(int epoll_fd) {
    signal(SIGINT, SIG_DFL); // Let main handle SIGINT
    enable_raw_mode();
    make_nonblocking(STDIN_FILENO);

    struct epoll_event ev;
    ev.events = DEFAULT_EPOLL_FLAGS;
    ev.data.fd = STDIN_FILENO;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) < 0) {
        perror("epoll_ctl stdin");
        exit(1);
    }

    console_redraw_prompt();
}

void print_live_memory_usage();
typedef struct AuthBlock AuthBlock;
char *authenticate(char *url, AuthBlock *block);

void console_handle_input(void) {
    char buf[CONSOLE_BUF_SIZE];
    ssize_t len = read(STDIN_FILENO, buf, sizeof(buf));
    if (len <= 0) return;

    for (ssize_t j = 0; j < len; j++) {
        char c = buf[j];

        if (c == '\n' || c == '\r') {
            input_buffer[input_len] = '\0';

            // Example: command handling
            if (strcmp(input_buffer, "help") == 0) {
                printf("help      - show this menu\n");
                printf("cls       - clear the screen\n");
                printf("mem       - show memory usage\n");
                printf("exit      - exit the program\n");
                printf("reg <url> - register account from url\n");
            } else if (!strcmp(input_buffer, "cls")) {
                printf("\033[2J\033[H");
            } else if (!strcmp(input_buffer, "exit")) {
                printf("Exiting...\n");
                exitbool = 1;
            } else if (!strcmp(input_buffer, "mem")) {
                print_live_memory_usage();
            } else if (!strncmp(input_buffer, "reg", 3)) {
                char *url = input_buffer + 4;  // Skip "reg "
                char *r = authenticate(url, NULL);
                if (r) printf("Reg command failed: %s\n", r);
            } else if (strlen(input_buffer) > 0) {
                printf("Unknown command: %s\n", input_buffer);
            }

            input_len = 0;
            input_buffer[0] = '\0';
            console_redraw_prompt();
        }
        else if (c == 127 || c == '\b') {
            if (input_len > 0) {
                input_len--;
                input_buffer[input_len] = '\0';
                console_redraw_prompt();
            }
        }
        else if (c >= 32 && c < 127 && input_len < CONSOLE_BUF_SIZE - 1) {
            input_buffer[input_len++] = c;
            input_buffer[input_len] = '\0';
            console_redraw_prompt();
        }
    }
}

// ---------- printf override ----------

int my_printf(const char *fmt, ...) {
    va_list args;
    int ret;

    // Clear the current line before printing
    fputs("\r\x1b[2K", stdout);

    va_start(args, fmt);
    ret = vfprintf(stdout, fmt, args);
    va_end(args);

    // Check if output ended with newline to decide if we redraw prompt
    size_t len = strlen(fmt);
    if (len > 0 && fmt[len - 1] == '\n') {
        console_redraw_prompt();
    }

    return ret;
}