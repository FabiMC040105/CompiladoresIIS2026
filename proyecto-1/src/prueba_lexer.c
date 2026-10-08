#include "parser.tab.h"

int yylex(void);

int main(void) {
    int token;

    while ((token = yylex()) != 0) {
        if (token == ERROR_LEXICO)
            return 1;
    }

    return 0;
}
