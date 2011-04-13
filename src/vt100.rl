/*
 * Parse a VT100/VT220 stream.
 */

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>

%%{
    machine terminal;
    write data;
}%%

char* execute(char* buffer)
{
    char* p = buffer;
    char* pe = buffer + strlen(buffer);
    int cs;

    std::string param;

    %%{
        CSI = 0x1B '[';

        action appendParameterChar { param += fc; }
        action printNumericParamter { printf("\t%s\n", param.c_str()); param = ""; }
        numeric_parameter = digit+* $appendParameterChar;
        multiple_numeric_parameters = numeric_parameter $printNumericParamter (';' numeric_parameter)*;

        color_change = CSI multiple_numeric_parameters 'm' @{ printf("Color change\n"); };
        reset_mode = CSI multiple_numeric_parameters 'l' @{ printf("Reset mode\n"); };
        command = color_change | reset_mode;

        main := command*;
        write init;
        write exec;
    }%%

    return p;
};

void parseBuffer(char* buffer)
{
    while (*buffer) {
        buffer = execute(buffer);
        if (*buffer) {
            printf("Saw other char: %c\n", *buffer);
            buffer++;
        }
    }
}
