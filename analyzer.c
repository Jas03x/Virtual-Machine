#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
    if(argc != 2) {
        puts("Invalid number of command-line arguments.");
        return -1;
    }

    FILE* input = fopen(argv[1], "r");
    if(input == NULL) {
        printf("Error input file [%s] not found.\n", argv[1]);
        return -1;
    }

    while(feof(input) == 0) {
        char in[10];
        scanf("%10s", in);
        char command[10];
        if(sscanf(in, "%s", command) == 0) {
            printf("Bad string error.\n");
            return -1;
        }

        if(strncmp(command, "skip", 4) == 0) {
            int num = 0;
            puts(in);
            if(sscanf(strchr(in, '-') + 1, "%i", &num) != 1) {
                printf("Invalid number entered.\n");
                return -1;
            }
            char buffer[256];
            if(fread(buffer, 1, num, input) != num) {
                printf("Error reading [%i] bytes.\n", num);
                return -1;
            }
        }
        else if(strcmp(command, "byte") == 0) {
            char c = 0;
            fread(&c, sizeof(char), 1, input);
            printf("%i -> [%c]\n", c, c);
        }
        else if(strcmp(command, "short") == 0) {
            short s = 0;
            fread(&s, sizeof(short), 1, input);
            printf("%i\n", s);
        }
        else if(strcmp(command, "int") == 0) {
            int i = 0;
            fread(&i, sizeof(int), 1, input);
            printf("%i\n", i);
        }
        else {
            printf("Unknown command [%s].\n", command);
        }
    }

    return 0;
}

