#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <signal.h>

#define PROMPT "> "
#define BUF_SIZE 256

static char input_buffer[BUF_SIZE] = {0};
static size_t input_len = 0;
static struct termios old_tio; // to restore terminal settings

// --- Terminal Mode Control ---
static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}

static void enable_raw_mode(void) {
    struct termios new_tio;

    tcgetattr(STDIN_FILENO, &old_tio);
    atexit(disable_raw_mode);

    new_tio = old_tio;
    new_tio.c_lflag &= ~(ICANON | ECHO); // no line buffering, no echo
    new_tio.c_cc[VMIN] = 1;  // return each char
    new_tio.c_cc[VTIME] = 0; // no timeout

    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

static void handle_sigint(int sig) {
    (void)sig;
    disable_raw_mode();
    printf("\nExiting...\n");
    exit(0);
}

// --- I/O helpers ---
static void make_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void redraw_prompt() {
    printf("\r\x1b[2K" PROMPT "%s", input_buffer);
    fflush(stdout);
}

static void console_print(const char *text) {
    printf("\r\x1b[2K%s\n", text);
    redraw_prompt();
}

int main() {
    signal(SIGINT, handle_sigint);
    enable_raw_mode();

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        return 1;
    }

    make_nonblocking(STDIN_FILENO);

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) < 0) {
        perror("epoll_ctl stdin");
        return 1;
    }

    redraw_prompt();

    int counter = 0; // just for demo background prints

    while (1) {
        struct epoll_event events[16];
        int nfds = epoll_wait(epoll_fd, events, 16, 10); // 200 ms
        if (nfds < 0) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == STDIN_FILENO) {
                char buf[BUF_SIZE];
                ssize_t len = read(STDIN_FILENO, buf, sizeof(buf));
                if (len > 0) {
                    for (ssize_t j = 0; j < len; j++) {
                        char c = buf[j];
                        if (c == '\n' || c == '\r') {
                            input_buffer[input_len] = '\0';
                            console_print(input_buffer);

                            // Commands
                            if (strcmp(input_buffer, "help") == 0) {
                                console_print("help - show this menu");
                                console_print("cls  - clear the screen");
                            } else if (strcmp(input_buffer, "cls") == 0) {
                                printf("\033[2J\033[H");
                            } else if (strlen(input_buffer) > 0) {
                                char msg[300];
                                snprintf(msg, sizeof(msg), "Unknown command: %s", input_buffer);
                                console_print(msg);
                            }

                            input_len = 0;
                            input_buffer[0] = '\0';
                            redraw_prompt();
                        }
                        else if (c == 127 || c == '\b') { // backspace
                            if (input_len > 0) {
                                input_len--;
                                input_buffer[input_len] = '\0';
                                redraw_prompt();
                            }
                        }
                        else if (c >= 32 && c < 127 && input_len < BUF_SIZE - 1) {
                            input_buffer[input_len++] = c;
                            input_buffer[input_len] = '\0';
                            redraw_prompt();
                        }
                    }
                }
            }
        }

        // Background demo output
        if (++counter % 25 == 0) {
            char msg[64];
            snprintf(msg, sizeof(msg), "[BG] tick %d", counter / 25);
            console_print(msg);
        }
    }

    close(epoll_fd);
    return 0;
}
