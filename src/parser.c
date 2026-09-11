#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

#define TOKEN_SIZE 64
#define TOKEN_DELIMITERS " \t\r\n\a"

/*
 * Splits a line into tokens and returns a NULL-terminated argv[] array.
 * strtok() writes '\0' into the line itself, so the returned tokens point
 * into that buffer - the caller must keep the line alive until it is done
 * with the tokens, then free the line separately.
 */
char **parse_line(char *line)
{
    int size = TOKEN_SIZE;
    int position = 0;
    char **tokens = malloc(size * sizeof(char*));

    if(tokens == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    char *token = strtok(line, TOKEN_DELIMITERS);

    while(token != NULL)
    {
        tokens[position++] = token;

        if(position >= size)
        {
            size *= 2;
            tokens = realloc(tokens, size * sizeof(char*));
            if(tokens == NULL)
            {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, TOKEN_DELIMITERS);
    }

    tokens[position] = NULL;
    return tokens;
}

/*
 * Frees only the array of pointers. The strings it points at live inside
 * the caller's line buffer and are freed with that buffer.
 */
void free_tokens(char **tokens)
{
    free(tokens);
}
