#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "str.h"

char **split(const char *str, char delim, int *out_count) {
    if (!str) return NULL;

    int count = 1;  // at least one token
    for (const char *s = str; *s; s++) {
        if (*s == delim) count++;
    }

    char **tokens = malloc(count * sizeof(char *));
    if (!tokens) return NULL;

    int token_index = 0;
    const char *start = str;
    const char *end = strchr(start, delim);

    while (end != NULL) {
        int len = end - start;
        tokens[token_index] = malloc(len + 1);
        strncpy(tokens[token_index], start, len);
        tokens[token_index][len] = '\0';
        token_index++;

        start = end + 1;
        end = strchr(start, delim);
    }

    // Last token
    tokens[token_index] = strdup(start);
    token_index++;

    if (out_count) *out_count = token_index;
    return tokens;
}

void free_split(char **tokens, int count) {
    for (int i = 0; i < count; i++) {
        free(tokens[i]);
    }
    free(tokens);
}
