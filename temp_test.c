#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define CHUNK_SIZE 1024

typedef enum {
    IDENTIFIER,
    NUMBER,
    STRING,
    SYMBOL,
    OPERATOR
} instr_type;
typedef struct instr instr;
struct instr{
    char* value;
    char nblocks;
    instr** blocks;
    instr_type type;
};
typedef struct {
    void** stack;
    int capacity;
    int len;
} _stack;
typedef struct {
    instr** instrs;
    int ninstrs;
} Binstr;

// --- Token reading helper ---
static char* read_token(FILE* f, int first, int (*cond)(int)) {
    size_t cap = 16;
    size_t len = 0;
    char* buf = malloc(cap);
    if (!buf) return NULL;

    buf[len++] = first;

    int c;
    while ((c = fgetc(f)) != EOF && cond(c)) {
        if (len + 1 >= cap) {
            cap *= 2;
            char* tmp = realloc(buf, cap);
            if (!tmp) { free(buf); return NULL; }
            buf = tmp;
        }
        buf[len++] = (char)c;
    }

    buf[len] = '\0';
    if (c != EOF) ungetc(c, f);
    return buf;
}

int cond_identifier(int ch) { return isalnum(ch) || ch == '_'; }
int cond_number(int ch) { return isdigit(ch) || ch == '.'; }
void set_token(Binstr* instrs, char* value, instr_type type) {
    instrs->instrs = realloc(instrs, sizeof(instr*) * (instrs->ninstrs + 1));
    instrs->instrs[instrs->ninstrs] = malloc(sizeof(instr));
    instrs->instrs[instrs->ninstrs]->value = strdup(value);
    instrs->instrs[instrs->ninstrs]->type = type;
    instrs->ninstrs++;
}

// --- Main runner ---
Binstr* load(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror(filename);
        return NULL;
    }

    Binstr* Binstrs = malloc(sizeof(Binstr));
    Binstrs->instrs = malloc(sizeof(instr*));
    Binstrs->ninstrs = 0;

    int c;
    while ((c = fgetc(f)) != EOF) {
        if (isspace(c)) continue;

        // identifiers or keywords
        if (isalpha(c) || c == '_') {
            char* tok = read_token(f, c, cond_identifier);
            set_token(Binstrs, tok, IDENTIFIER);
            // printf("Identifier: %s\n", tok);
            free(tok);
            continue;
        }

        // numbers
        if (isdigit(c)) {
            char* tok = read_token(f, c, cond_number);
            set_token(Binstrs, tok, NUMBER);
            // printf("Number: %s\n", tok);
            free(tok);
            continue;
        }

        // strings
        if (c == '"') {
            size_t cap = 16, len = 0;
            char* str = malloc(cap);
            int closed = 0;

            while ((c = fgetc(f)) != EOF) {
                if (c == '"') { closed = 1; break; }
                if (len + 1 >= cap) { cap *= 2; str = realloc(str, cap); }
                str[len++] = (char)c;
            }
            str[len] = '\0';

            set_token(Binstrs, str, STRING);
            // printf("String: \"%s\"\n", str);
            free(str);
            if (!closed) printf("Warning: unclosed string literal!\n");
            continue;
        }

        // symbols
        set_token(Binstrs, (char[]){c, '\0'}, SYMBOL);
        // printf("Symbol: '%c'\n", c);
    }

    fclose(f);
    return Binstrs;
}
void parse(Binstr* instrs) {

}
int main(void) {
    Binstr* instrs = load("plugin.data");
    for (int i = 0; i < instrs->ninstrs; i++) printf("%s\n",instrs->instrs[i]->value);
    return 0;
}
