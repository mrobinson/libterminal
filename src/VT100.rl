#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>

%%{
    machine terminal;
    write data;
}%%

const char* execute(const char* start, const char* end)
{
    const char* p = start;
    const char* pe = end;
    int cs;

    %%{
        action consumeChar { printf("%c", *p); }
        action semi { printf(";\n"); }
        action colorChangeAction { printf(" <-- color change \n"); }
        action resetMode { printf(" <-- resetMode \n"); }

        number = digit+ @consumeChar;
        multiple_numeric_parameters = number (';'@semi number)+;

        CSI = 0x1B '[';
        colorChange = CSI multiple_numeric_parameters 'm' @colorChangeAction;
        resetMode = CSI multiple_numeric_parameters 'l' @resetMode;
        command = colorChange | resetMode;
        main := command*;

        write init;
        write exec;
    }%%

    return p;
};

const char* parseBuffer(const char* start, const char* end)
{
    while (start != end) {
        start = execute(start, end);
        printf("did one pass\n");
        if (start != end) {
            printf("Saw other char: %c\n", *start);
            start++;
        }
    }
    return start;
}
