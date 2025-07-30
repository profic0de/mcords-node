#ifndef UTILS_STRING
#define UTILS_STRING

char **split(const char *str, char delim, int *out_count);
void free_split(char **tokens, int count);

#endif