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

    action resetDevice { printf(" <-- resetDevice \n"); }
    action enableLineWrap { printf(" <-- enableLineWrap \n"); }
    action disableLineWrap { printf(" <-- disableLineWrap \n"); }

    action eraseInLineFromCursorToRight { m_client->eraseFromCursorToEndOfLine(Right); }
    action eraseInLineFromCursorToLeft { m_client->eraseFromCursorToEndOfLine(Left); }
    action eraseEntireLine { printf(" <-- eraseEntireLine \n"); }
    action eraseScreenFromCursorDown { printf(" <-- eraseScreenFromCursorDown \n"); }
    action eraseScreenFromCursorUp { printf(" <-- eraseScreenFromCursorUp \n"); }
    action eraseEntireScreen { printf(" <-- eraseEntireScreen \n"); }

    action errorState {
        const char* i = start;
        printf("error state: [%x]-> %p,%p ->", fc, p, pe);
        if(p < pe) {
            for(i = start; i <= pe; i++) {
                printf("[%x]", *i);
            }
            for(i = start; i <= pe; i++) {
                if((*i >= 0x32) && (*i <= 0x7E)) {
                    printf("{%c}", *i);
                }
                else {
                    switch(*i) {
                        case 0:
                            printf("{NULL}");
                            break;
                        case 0x8:
                            printf("{BackSpace}");
                            break;
                        case 0x1B:
                            printf("{ESC}");
                            break;
                        default:
                            printf("{todo!}");
                            break;
                    }
                }
            }
            printf("\n");
        }
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

    terminalSetup = 0x1B 'c' @resetDevice
        | CSI '7' 'h' @enableLineWrap
        | CSI '7' 'l' @disableLineWrap;

    erase = 0x1B '[' '0'? 'K' @eraseInLineFromCursorToRight
        | 0x1B '[' '1' 'K' @eraseInLineFromCursorToLeft
        | 0x1B '[' '2' 'K' @eraseEntireLine
        | 0x1B '[' '0'? 'J' @eraseScreenFromCursorDown
        | 0x1B '[' '1' 'J' @eraseScreenFromCursorUp
        | 0x1B '[' '2' 'J' @eraseEntireScreen;

    command = colorChange 
        | resetMode 
        | titleChange 
        | characterMode 
        | terminalSetup
        | erase
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
