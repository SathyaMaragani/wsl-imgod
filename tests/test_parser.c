/*
 * Unit tests for parse_line().  Build and run with: make test
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"

static int checks = 0;

/* expected is a NULL-terminated list of expected tokens */
static void expect(const char *input, const char **expected)
{
    char line[256];
    char **tokens;
    int i;

    strncpy(line, input, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';

    tokens = parse_line(line);

    for(i = 0; expected[i] != NULL; i++)
    {
        assert(tokens[i] != NULL);
        assert(strcmp(tokens[i], expected[i]) == 0);
    }
    assert(tokens[i] == NULL);   /* array must be NULL-terminated for execvp */

    free_tokens(tokens);
    checks++;
}

int main(void)
{
    const char *ls[]      = {"ls", NULL};
    const char *lsl[]     = {"ls", "-l", "/home", NULL};
    const char *gcc_[]    = {"gcc", "main.c", NULL};
    const char *cat_[]    = {"cat", "sample.txt", NULL};
    const char *none[]    = {NULL};

    expect("ls", ls);                        /* simple command          */
    expect("ls -l /home", lsl);              /* command with arguments  */
    expect("gcc    main.c", gcc_);           /* repeated spaces         */
    expect("  ls  ", ls);                    /* leading/trailing spaces */
    expect("cat\tsample.txt", cat_);         /* tab delimiter           */
    expect("ls -l /home\n", lsl);            /* trailing newline        */
    expect("", none);                        /* empty line              */
    expect("   \t ", none);                  /* whitespace only         */

    /* more arguments than the initial 64-slot array, to exercise realloc() */
    {
        char big[1024] = "cmd";
        char **tokens;
        int i;
        for(i = 0; i < 99; i++)
            strcat(big, " a");
        tokens = parse_line(big);
        for(i = 0; i < 100; i++)
            assert(tokens[i] != NULL);
        assert(tokens[100] == NULL);
        free_tokens(tokens);
        checks++;
    }

    printf("parse_line(): %d checks passed\n", checks);
    return 0;
}
