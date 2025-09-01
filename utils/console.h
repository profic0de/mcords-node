#ifndef CONSOLE_H
#define CONSOLE_H

#include <stddef.h>

// Max buffer size for console input
#define CONSOLE_BUF_SIZE 256

// Initialize console (raw mode, epoll registration, etc.)
void console_init(int epoll_fd);

// Process console input when EPOLLIN occurs
void console_handle_input(void);

// Redraw the prompt (used by printf override)
void console_redraw_prompt(void);

// Get the current input buffer (null-terminated)
const char *console_get_input(void);

int my_printf(const char *fmt, ...);

#endif
