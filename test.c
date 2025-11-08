#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    IDENTIFIER,
    NUMBER,
    STRING,
    SYMBOL,
    OPERATOR,
    BRACKETS
} instr_type;

typedef struct instr instr;
typedef struct {
    instr** instrs;
    int capacity;
    int len;
} Binstr;

struct instr {
    char* value;
    Binstr* blocks;
    instr_type type;
};

typedef struct {
    Binstr** items;
    int capacity;
    int len;
} Stack;

#define INITIAL_CAP 4

Binstr* Binstr_init(void) {
    Binstr* b = malloc(sizeof(Binstr));
    if (!b) return NULL;
    b->capacity = INITIAL_CAP;
    b->len = 0;
    b->instrs = malloc(sizeof(instr*) * b->capacity);
    return b;
}

instr* instr_init(void) {
    instr* i = malloc(sizeof(instr));
    if (!i) return NULL;
    i->value = NULL;
    i->blocks = NULL;
    i->type = IDENTIFIER;
    return i;
}

void Binstr_ensure_capacity(Binstr* b) {
    if (b->len >= b->capacity) {
        b->capacity *= 2;
        b->instrs = realloc(b->instrs, sizeof(instr*) * b->capacity);
    }
}

void Binstr_append(Binstr* b, instr* i) {
    Binstr_ensure_capacity(b);
    b->instrs[b->len++] = i;
}

Stack* Stack_init(void) {
    Stack* s = malloc(sizeof(Stack));
    s->capacity = INITIAL_CAP;
    s->len = 0;
    s->items = malloc(sizeof(Binstr*) * s->capacity);
    return s;
}

void Stack_ensure_capacity(Stack* s) {
    if (s->len >= s->capacity) {
        s->capacity *= 2;
        s->items = realloc(s->items, sizeof(Binstr*) * s->capacity);
    }
}

void Stack_push(Stack* s, Binstr* b) {
    Stack_ensure_capacity(s);
    s->items[s->len++] = b;
}

Binstr* Stack_top(Stack* s) {
    if (s->len == 0) return NULL;
    return s->items[s->len - 1];
}

Binstr* Stack_pop(Stack* s) {
    if (s->len == 0) return NULL;
    return s->items[--s->len];
}

static char* read_token(FILE* f, int first, int (*cond)(int)) {
    size_t cap = 16;
    size_t len = 0;
    char* buf = malloc(cap);
    if (!buf) return NULL;

    buf[len++] = (char)first;

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

Binstr* parse(Binstr* flat);
Binstr* load(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror(filename);
        return NULL;
    }

    Binstr* top = Binstr_init();
    if (!top) { fclose(f); return NULL; }

    int c;
    while ((c = fgetc(f)) != EOF) {
        if (isspace(c)) continue;
        // if (c!='\n'&&isspace(c)) continue;

        if (isalpha(c) || c == '_') {
            char* tok = read_token(f, c, cond_identifier);
            instr* i = instr_init();
            i->value = strdup(tok);
            i->type = IDENTIFIER;
            Binstr_append(top, i);
            free(tok);
            continue;
        }

        if (isdigit(c)) {
            char* tok = read_token(f, c, cond_number);
            instr* i = instr_init();
            i->value = strdup(tok);
            i->type = NUMBER;
            Binstr_append(top, i);
            free(tok);
            continue;
        }

        if (c == '"') {
            size_t cap = 16, len = 0;
            char* str = malloc(cap);
            int closed = 0;
            int ch;
            while ((ch = fgetc(f)) != EOF) {
                if (ch == '"') { closed = 1; break; }
                if (len + 1 >= cap) { cap *= 2; str = realloc(str, cap); }
                str[len++] = (char)ch;
            }
            str[len] = '\0';
            instr* i = instr_init();
            i->value = strdup(str);
            i->type = STRING;
            Binstr_append(top, i);
            free(str);
            if (!closed) fprintf(stderr, "Warning: unclosed string literal!\n");
            continue;
        }

        // everything else as SYMBOL (single-char)
        char s[2] = { (char)c, '\0' };
        instr* i = instr_init();
        i->value = strdup(s);
        i->type = SYMBOL;
        Binstr_append(top, i);
    }

    fclose(f);

    if (!top) return NULL;
    Binstr* nested = parse(top);

    return nested;
}

/* ---------- Parser to create nested structure using stack ----------

   Strategy:
   - Start with a root Binstr (result).
   - Keep a stack of current Binstr*, top = where new tokens are appended.
   - For a normal token: append it to top.
   - For '(' or '{': create an instr (value "(" or "{"), create a child Binstr,
     assign instr->blocks = child, append instr to top, then push child as new top.
   - For ')' or '}': pop the stack (but never pop the root).
*/

Binstr* parse(Binstr* flat) {
    if (!flat) return NULL;

    Binstr* root = Binstr_init();
    Stack* st = Stack_init();
    Stack_push(st, root);

    for (int i = 0; i < flat->len; ++i) {
        instr* token = flat->instrs[i];
        if (!token || !token->value) continue;

        char ch = token->value[0];

        if (ch == '(' || ch == '{') {
            // create a bracket instr with a new child block
            instr* bracket = instr_init();
            char s[2] = { ch, '\0' };
            bracket->value = strdup(s);
            bracket->type = BRACKETS;
            bracket->blocks = Binstr_init(); // child content
            Binstr_append(Stack_top(st), bracket);
            // now descend into child block
            Stack_push(st, bracket->blocks);
            continue;
        }

        if (ch == ')' || ch == '}') {
            // when closing, pop but do not pop root
            if (st->len > 1) Stack_pop(st);
            else {
                // unbalanced closing bracket: ignore or warn
                fprintf(stderr, "Warning: unmatched closing bracket '%c'\n", ch);
            }
            // we may optionally append the closing bracket token as well;
            // current code does not append the closing bracket itself.
            continue;
        }

        // if (ch == '=') {
        if (ch=='='||ch=='<'||ch=='>') {
            instr* t = flat->instrs[i+1];
            if (!t || !t->value) goto skip;
            char c = t->value[0];
            if (c=='='||c=='<'||c=='>') {
                strcat(token->value, t->value);
                i++;
            }
            token->type=OPERATOR;
        }
        skip:

        // normal token: clone and append to current top
        instr* copy = instr_init();
        copy->value = strdup(token->value);
        copy->type = token->type;
        Binstr_append(Stack_top(st), copy);
    }

    // cleanup stack
    free(st->items);
    free(st);

    return root;
}

/* ---------- Printing with indentation ---------- */

void print_instrs_rec(Binstr* b, int indent) {
    if (!b) return;
    for (int i = 0; i < b->len; ++i) {
        instr* it = b->instrs[i];
        for (int s = 0; s < indent; ++s) putchar(' ');
        if (it->type == BRACKETS) {
            printf("%s {\n", it->value ? it->value : "(null)");
            print_instrs_rec(it->blocks, indent + 2);
            for (int s = 0; s < indent; ++s) putchar(' ');
            printf("}\n");
        } else {
            printf("%s\n", it->value ? it->value : "(null)");
        }
    }
}

void print_instrs(Binstr* b) {
    print_instrs_rec(b, 0);
}

/* ---------- main ---------- */

int main(void) {
    Binstr* nested = load("plugin.data");
    if (!nested) {
        fprintf(stderr, "Failed to load plugin.data\n");
        return 1;
    }
    // Binstr* nested = parse(flat);

    printf("=== Nested tokens ===\n");
    print_instrs(nested);

    return 0;
}
