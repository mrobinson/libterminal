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
        /*
        if((fc >= 32) && (fc <= 126)) {
          //printf("<- appendChar '%c', 0x%x\n", fc, fc);
        }
        // \n \r and \t
        else if((fc == 0xd) || (fc == 0xa)|| (fc == 0x9)) {
          //printf("<- special appendChar 0x%x\n", fc);
        } 
        else {
          printf("<- hrm... appendChar 0x%x\n", fc);
        }
        */
        m_client->appendCharacter(fc);
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

    command = colorChange | resetMode | titleChange | characterMode | ^0x1B@appendChar;

    main := command*;
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
    %%write exec;
    this->cs = cs;
}
