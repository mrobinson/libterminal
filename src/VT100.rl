#include "VT100.h"

#include "VT100Client.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

static void printDebuggingCharacter(char character);

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

    action handleChar {
        printf("appending 0x%x ", fc);
        printDebuggingCharacter(fc);
        printf("\n");
        switch (fc) {
        case '\a':
            m_client->bell();
            break;
        default:
            m_client->appendCharacter(fc);
        }
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

    action moveCursorUpNLines { printf("<-- moveCursorUpNLines"); printAllNumbers(this->numberStack); printf("\n"); }
    action moveCursorDownNLines { printf("<-- moveCursorDownNLines"); printAllNumbers(this->numberStack); printf("\n"); }
    action moveCursorRightNLines { printf("<-- moveCursorRightNLines"); printAllNumbers(this->numberStack); printf("\n"); }
    action moveCursorLeftNLines { printf("<-- moveCursorLeftNLines"); printAllNumbers(this->numberStack); printf("\n"); }
    action moveCursorToUpperLeftCorner { printf("<-- moveCursorToUpperLeftCorner\n"); }
    action moveCursor { printf("<-- moveCursor "); printAllNumbers(this->numberStack); printf("\n"); }
    action moveUpOneLine { printf("<-- moveUpOneLine\n"); }
    action moveDownOneLine { printf("<-- moveDownOneLine\n"); }
    action moveToNextLine { printf("<-- moveToNextLine\n"); }
    action saveCursorPositionAndAttributes { printf("<-- saveCursorPositionAndAttributes\n"); }
    action restoreCursorPositionAndAttributes { printf("<-- restoreCursorPositionAndAttributes\n"); }

    action errorState {
        const char* i = start;
        printf("error state: [%x]-> %p,%p ->", fc, p, pe);
        if(p < pe) {
            for(i = start; i <= pe; i++) {
                printf("[%x]", *i);
            }
            for(i = start; i <= pe; i++) {
                printDebuggingCharacter(*i);
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

    ESC = 0x1B;
    CSI = ESC '[';
    OSC = ESC ']';

    colorChange = CSI multiple_numeric_parameters 'm' @colorChangeAction;
    characterMode = CSI unsigned_number? 'm' @characterMode;
    resetMode = CSI multiple_numeric_parameters 'l' @resetMode;
    titleChange = OSC ('0' | '1' | '2' | '3' | '4') ';' any+ :> 0x07 @setTitle;

    terminalSetup = ESC 'c' @resetDevice
        | CSI '7' 'h' @enableLineWrap
        | CSI '7' 'l' @disableLineWrap;

    erase = CSI '0'? 'K' @eraseInLineFromCursorToRight
        | CSI '1' 'K' @eraseInLineFromCursorToLeft
        | CSI '2' 'K' @eraseEntireLine
        | CSI '0'? 'J' @eraseScreenFromCursorDown
        | CSI '1' 'J' @eraseScreenFromCursorUp
        | CSI '2' 'J' @eraseEntireScreen;

    cursor = CSI unsigned_number 'A' @moveCursorUpNLines
        | CSI unsigned_number 'B' @moveCursorDownNLines
        | CSI unsigned_number 'C' @moveCursorRightNLines
        | CSI unsigned_number 'D' @moveCursorLeftNLines
        | CSI 'H' @moveCursorToUpperLeftCorner
        | CSI ';' 'H' @moveCursorToUpperLeftCorner
        | CSI multiple_numeric_parameters 'H' @moveCursor
        | CSI 'f' @moveCursorToUpperLeftCorner
        | CSI ';' 'f' @moveCursorToUpperLeftCorner
        | CSI multiple_numeric_parameters 'f' @moveCursor
        | ESC 'D' @moveUpOneLine
        | ESC 'M' @moveDownOneLine
        | ESC 'E' @moveToNextLine
        | ESC '7' @saveCursorPositionAndAttributes
        | ESC '8' @restoreCursorPositionAndAttributes;

    command = colorChange
        | resetMode
        | titleChange
        | characterMode
        | terminalSetup
        | erase
        | cursor
        | ^0x1B @handleChar;

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

static void printDebuggingCharacter(char character) {
    if((character >= 0x20) && (character <= 0x7E)) {
        printf("{%c}", character);
        return;
    }

    switch(character) {
    case 0x0:  printf("{NULL}"); break;
    case 0x1:  printf("{StartOfHeading}"); break;
    case 0x2:  printf("{StartOfText}"); break;
    case 0x3:  printf("{EndOfText}"); break;
    case 0x4:  printf("{EndOfTransmission}"); break;
    case 0x5:  printf("{Enquiry}"); break;
    case 0x6:  printf("{Acknowledgement}"); break;
    case 0x7:  printf("{Bell}"); break;
    case 0x8:  printf("{BackSpace}"); break;
    case 0x9:  printf("{HorizontalTab}"); break;
    case 0xA:  printf("{LineFeed}"); break;
    case 0xB:  printf("{VerticalTab}"); break;
    case 0xC:  printf("{FromFeed}"); break;
    case 0xD:  printf("{CarriageReturn}"); break;
    case 0xE:  printf("{ShiftOutXOn}"); break;
    case 0xF:  printf("{ShiftInXOff}"); break;
    case 0x10: printf("{DateLineEscape}"); break;
    case 0x11: printf("{DeviceControl1}"); break;
    case 0x12: printf("{DeviceControl2}"); break;
    case 0x13: printf("{DeviceControl3}"); break;
    case 0x14: printf("{DeviceControl4}"); break;
    case 0x15: printf("{NegativeAcknowledgement}"); break;
    case 0x16: printf("{SynchronousIdle}"); break;
    case 0x17: printf("{EndOfTransmitBlock}"); break;
    case 0x18: printf("{Cancel}"); break;
    case 0x19: printf("{EndOfMedium}"); break;
    case 0x1A: printf("{Substitute}"); break;
    case 0x1B: printf("{ESC}"); break;
    case 0x1C: printf("{FileSeparator}"); break;
    case 0x1D: printf("{GroupSeparator}"); break;
    case 0x1E: printf("{RecordSeparator}"); break;
    case 0x1F: printf("{UnitSeparator}"); break;
    case 0x7F: printf("{Delete}"); break;
    default:
        printf("{todo!}");
        break;
    }
}
