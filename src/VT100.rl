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

%%{
    machine terminal;
    write data;

    action colorChangeAction { printf(" <-- color change"); printAllNumbers(this->numberStack); printf("\n"); }

    action resetMode { printf(" <-- resetMode"); printAllNumbers(this->numberStack); printf("\n"); }

    action setTitle { printf(" <-- setTitle \n"); }

    action characterMode { printf(" <-- characterMode \n"); }

    action appendChar { 
        m_client->appendCharacter(fc);
    }

    action errorState {
        printf("error state: [%x]\n", fc);
        fhold; fgoto main;
    }

    unsigned_number = digit+
                > { this->unsignedValue = 0; } 
                $ { this->unsignedValue = (this->unsignedValue * 10) + (fc - '0'); }
                % { this->numberStack.push_back(this->unsignedValue); };

    multiple_numeric_parameters = unsigned_number > { this->numberStack.clear(); } (';' unsigned_number )+;

    CSI = 0x1B '[';
    OSC = 0x1B ']';

    colorChange = CSI multiple_numeric_parameters 'm' @colorChangeAction;
    characterMode = CSI unsigned_number? 'm' @characterMode;
    resetMode = CSI multiple_numeric_parameters 'l' @resetMode;
    titleChange = OSC ('0' | '1' | '2' | '3' | '4') ';' any+ :> 0x07 @setTitle;

    command = colorChange | resetMode | titleChange | characterMode 
        | ^0x1B @appendChar;

    main := command* $err(errorState);
}%%

VT100::VT100(VT100Client* client)
    : m_client(client)
{
    int cs;
    %%write init;
    this->cs = cs;
}

void VT100::parseBuffer(const char* start, const char* end)
{
    const char* p = start;
    const char* pe = end;
    int cs = this->cs;
    const char* eof = NULL;
    %%write exec;
    this->cs = cs;
}
