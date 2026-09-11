#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/shell.h"
#include "../include/input.h"
#include "../include/parser.h"

int main()
{
    char *line;
    char **tokens;
    int i;

    printf("=================================\n");
    printf("%s Version %s\n",SHELL_NAME,VERSION);
    printf("=================================\n");

    while(1)
    {
        printf("myshell> ");
        line = read_line();

        if(strcmp(line,"exit")==0)
        {
            free(line);
            break;
        }

        tokens = parse_line(line);

        if(tokens[0] != NULL)
        {
            printf("\nParsed Tokens\n");
            for(i=0;tokens[i]!=NULL;i++)
            {
                printf("argv[%d] = %s\n",i,tokens[i]);
            }
        }

        free_tokens(tokens);
        free(line);
    }

    printf("Goodbye!\n");
    return 0;
}
