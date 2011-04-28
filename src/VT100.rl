#include "VT100.h"

#include "VT100Client.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>

VT100::VT100(VT100Client* client)
    : m_client(client)
{
}

%%{
    machine terminal;
    write data;
}%%

const char* VT100::executeStateMachine(const char* start, const char* end)
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

const char* VT100::parseBuffer(const char* start, const char* end)
{
    while (start != end) {
        start = executeStateMachine(start, end);
        if (start != end) {
            m_client->appendCharacter(*start);
            start++;
        }
    }
    return start;
}
