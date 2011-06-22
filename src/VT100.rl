#include "VT100.h"

#include "VT100Client.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

static void printAllNumbers(std::vector<int>& numbers)
{
    for (size_t i = 0; i < numbers.size(); i++)
        printf(" %i", numbers[i]);
}

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

    int unsignedValue;
    std::vector<int> numberStack;

    %%{
        action colorChangeAction { printf(" <-- color change"); printAllNumbers(numberStack); printf("\n"); }
        action resetMode { printf(" <-- resetMode"); printAllNumbers(numberStack); printf("\n"); }
        action setTitle { printf(" <-- setTitle \n"); }

        unsigned_number = digit+
                > { unsignedValue = 0; } 
                $ { unsignedValue = (unsignedValue * 10) + (fc - '0'); }
                % { numberStack.push_back(unsignedValue); };
        multiple_numeric_parameters = unsigned_number > { numberStack.clear(); } (';' unsigned_number )+;

        CSI = 0x1B '[';
        OSC = 0x1B ']';

        colorChange = CSI multiple_numeric_parameters 'm' @colorChangeAction;
        resetMode = CSI multiple_numeric_parameters 'l' @resetMode;
        titleChange = OSC ('0' | '1' | '2' | '3' | '4') ';' any+ :> 0x07 @setTitle;

        command = colorChange | resetMode | titleChange;
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
