#include "VT100.h"

#include "VT100Client.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

static void printDebuggingCharacter(char character);

static void printAction(const char* message);
static void printActionWithNumbers(const char* message, std::vector<int>& numbers);

static void printAllNumbers(std::vector<int>& numbers)
{
    for (size_t i = 0; i < numbers.size(); i++)
        printf(" %i", numbers[i]);
}

%%{
    machine terminal;
    write data;

    ESC = 0x1B;
    CSI = ESC '[';
    OSC = ESC ']';

# Standard Escape Sequence Parameters.
# The following are equivalent:
#   1. ESC [ ; 4 ; 5 m
#   2. ESC [ m
#      ESC [ 4 m
#      ESC [ 5 m
#   3. ESC [ 0 ; 04; 005 m

    # clear the stack at the beginning of a new escape sequence that takes
    # parameters
    action clearStack {
        this->numberStack.clear(); 
    }

    # if the escape sequence that takes paramters lacks a parameter, give it
    # a parameter of zero which is generally a reset op.
    action addNullOpAsNeeded {
        if(this->numberStack.size() == 0) { 
            this->numberStack.push_back(-1); 
        }
    }

    unsigned_number = digit+
        > { this->unsignedValue = 0; }
        $ { this->unsignedValue = (this->unsignedValue * 10) + (fc - '0'); }
        % { this->numberStack.push_back(this->unsignedValue); };

    multiple_numeric_parameters = unsigned_number? >clearStack %addNullOpAsNeeded (';' unsigned_number)* ';'?;

# Standard Escape Sequences


    # CPR - Cursor Position Report - CSI [ Pline ; Pcolumn R 
    # default value 1

    action resetMode { printf(" <-- resetMode"); printAllNumbers(this->numberStack); printf("\n"); }

    action setTitle { printAction("setTitle"); }

    action characterMode { printActionWithNumbers("characterMode", this->numberStack); }

    action handleChar {
        //printf("appending 0x%x ", fc);
        //printDebuggingCharacter(fc);
        //printf("\n");
        switch (fc) {
        case '\a':
            m_client->bell();
            break;
        default:
            m_client->appendCharacter(fc);
        }
    }

    action resetDevice { printAction("resetDevice"); }
    action enableLineWrap { printAction("enableLineWrap"); }
    action disableLineWrap { printAction("disableLineWrap"); }

    action eraseInLineFromCursorToRight { m_client->eraseFromCursorToEndOfLine(Right); }
    action eraseInLineFromCursorToLeft { m_client->eraseFromCursorToEndOfLine(Left); }
    action eraseEntireLine { printAction("eraseEntireLine"); }
    action eraseScreenFromCursorDown { printAction("eraseScreenFromCursorDown"); }
    action eraseScreenFromCursorUp { printAction("eraseScreenFromCursorUp"); }
    action eraseEntireScreen { printAction("eraseEntireScreen"); }

    action moveCursorUpNLines { printf("<-- moveCursorUpNLines"); printAllNumbers(this->numberStack); printf("\n"); }
    action moveCursorDownNLines { printf("<-- moveCursorDownNLines"); printAllNumbers(this->numberStack); printf("\n"); }
    action moveCursorRightNLines { printf("<-- moveCursorRightNLines"); printAllNumbers(this->numberStack); printf("\n"); }
    action moveCursorLeftNLines { printf("<-- moveCursorLeftNLines"); printAllNumbers(this->numberStack); printf("\n"); }
    action moveCursorToUpperLeftCorner { /* printf("<-- moveCursorToUpperLeftCorner\n"); */ }
    action moveCursor { /* printf("<-- moveCursor "); printAllNumbers(this->numberStack); printf("\n"); */ }
    action moveUpOneLine { printAction("moveUpOneLine"); }
    action moveDownOneLine { printAction("moveDownOneLine"); }
    action moveToNextLine { printAction("moveToNextLine"); }
    action saveCursorPositionAndAttributes { printf("<-- saveCursorPositionAndAttributes\n"); }
    action restoreCursorPositionAndAttributes { printf("<-- restoreCursorPositionAndAttributes\n"); }
    action unknown { printAction("unknown sequence"); }
    action unknownSet { 
        /* ESC ) A United Kingdom Set
           ESC ) B ASCII Set
           ESC ) 0 Special Graphics
           ESC ) 1 Alternate Character ROM Standard Character Set
           ESC ) 2 Alternate Character ROM Special Graphics */
        printAction("unknown set"); 
    }

    action errorState {
        const char* i = start;
        printf("ERROR STATE: [%x]-> %p,%p ->\n hex->", fc, fpc, pe);
        if(fpc < pe) {
            for(i = fpc; i <= pe; i++) {
                printf("[%x]", *i);
            }
			printf("\n rep->");
            for(i = fpc; i <= pe; i++) {
                printDebuggingCharacter(*i);
            }
            printf("\n");
        }
        fhold; fgoto main;
    }


    characterMode = CSI multiple_numeric_parameters 'm' @characterMode;
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
        | CSI multiple_numeric_parameters ('H'|'f') @moveCursor
        | ESC 'D' @moveUpOneLine
        | ESC 'M' @moveDownOneLine
        | ESC 'E' @moveToNextLine
        | ESC '7' @saveCursorPositionAndAttributes
        | ESC '8' @restoreCursorPositionAndAttributes;

    unknown = ESC '(' 'A' @unknownSet
        | ESC '(' 'B' @unknownSet
        | ESC '(' '0' @unknownSet
        | ESC '(' '1' @unknownSet
        | ESC '(' '2' @unknownSet
        | CSI '?' unsigned_number 'l' @unknown
        | CSI unsigned_number ';' 'l' @unknown
        | CSI unsigned_number 'd' @unknown;

    command = resetMode
        | titleChange
        | characterMode
        | terminalSetup
        | erase
        | cursor
        | unknown
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

static void printAction(const char* message) {
    printf("<-- %s\n", message);
}

static void printActionWithNumbers(const char* message, std::vector<int>& numbers) {
    printf("<-- %s", message);
    printAllNumbers(numbers);
    printf("\n");
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
