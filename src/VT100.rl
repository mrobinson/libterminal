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

    unsigned_number = digit+
        > { this->unsignedValue = 0; }
        $ { this->unsignedValue = (this->unsignedValue * 10) + (fc - '0'); }
        % { this->numberStack.push_back(this->unsignedValue); };

    # TODO allow ESC [  ;  ;  m -> ESC [ -1 ; -1 ; -1 m
    multiple_numeric_parameters = unsigned_number? (';' unsigned_number)* ';'?
        > { this->numberStack.clear(); }
        % {
            if (this->numberStack.size() == 0)
                this->numberStack.push_back(-1);
        };

# Standard Escape Sequences

    # CPR - Cursor Position Report - CSI [ Pline ; Pcolumn R - default value 1

    #  The CPR sequence reports the active position by means of the
    #  parameters. This sequence has two parameter values, the first
    #  specifying the line and the second specifying the column. The default
    #  condition with no parameters present, or parameters of 0, is
    #  equivalent to a cursor at home position.

    #  The numbering of lines depends on the state of the Origin Mode (DECOM).

    #  This control sequence is solicited by a device status report (DSR)
    #  sent from the host.

    action CPR { 
        printActionWithNumbers("cursorPositionReport", this->numberStack); 
    }

    cursorPositionReport = CSI multiple_numeric_parameters 'R' @CPR;

    # CUB - Cursor Backward - CSI Pn D - default value: 1

    #  The CUB sequence moves the active position to the left. The distance
    #  moved is determined by the parameter. If the parameter value is zero
    #  or one, the active position is moved one position to the left. If the
    #  parameter value is n, the active position is moved n positions to the
    #  left. If an attempt is made to move the cursor to the left of the left
    #  margin, the cursor stops at the left margin. Editor Function

    action CUB {
        printActionWithNumbers("cursorBackward", this->numberStack); 
    }

    cursorBackward = CSI multiple_numeric_parameters 'D' @CUB;

    # CUD - Cursor Down - ESC [ Pn B - default value: 1

    #  The CUD sequence moves the active position downward without altering
    #  the column position. The number of lines moved is determined by the
    #  parameter. If the parameter value is zero or one, the active position
    #  is moved one line downward. If the parameter value is n, the active
    #  position is moved n lines downward. In an attempt is made to move the
    #  cursor below the bottom margin, the cursor stops at the bottom
    #  margin. Editor Function

    action CUD {
        printActionWithNumbers("cursorDown", this->numberStack); 
    }

    cursorDown = CSI multiple_numeric_parameters 'B' @CUD;

    # CUF - Cursor Forward - ESC [ Pn C - default value: 1

    #  The CUF sequence moves the active position to the right. The distance
    #  moved is determined by the parameter. A parameter value of zero or one
    #  moves the active position one position to the right. A parameter value
    #  of n moves the active position n positions to the right. If an attempt
    #  is made to move the cursor to the right of the right margin, the
    #  cursor stops at the right margin. Editor Function

    action CUF {
        printActionWithNumbers("cursorForward", this->numberStack); 
    }

    cursorForward = CSI multiple_numeric_parameters 'C' @CUF;

    # CUP - Cursor Position - ESC [ Pn ; Pn H - default value: 1

    #  The CUP sequence moves the active position to the position specified
    #  by the parameters. This sequence has two parameter values, the first
    #  specifying the line position and the second specifying the column
    #  position. A parameter value of zero or one for the first or second
    #  parameter moves the active position to the first line or column in the
    #  display, respectively. The default condition with no parameters
    #  present is equivalent to a cursor to home action. In the VT100, this
    #  control behaves identically with its format effector counterpart,
    #  HVP. Editor Function

    #  The numbering of lines depends on the state of the Origin Mode (DECOM).

    action CUP {
        printActionWithNumbers("cursorPosition", this->numberStack); 
    }

    cursorPosition = CSI multiple_numeric_parameters 'H' @CUP;

    # CUU - Cursor Up - ESC [ Pn A - default value: 1

    #  Moves the active position upward without altering the column
    #  position. The number of lines moved is determined by the parameter. A
    #  parameter value of zero or one moves the active position one line
    #  upward. A parameter value of n moves the active position n lines
    #  upward. If an attempt is made to move the cursor above the top margin,
    #  the cursor stops at the top margin. Editor Function

    action CUU {
        printActionWithNumbers("cursorUp", this->numberStack); 
    }

    cursorUp = CSI multiple_numeric_parameters 'A' @CUU;

    # DA - Device Attributes - ESC [ Pn c - default value: 0

    #  1. The host requests the VT100 to send a device attributes (DA)
    #     control sequence to identify itself by sending the DA control
    #     sequence with either no parameter or a parameter of 0.

    #  2. Response to the request described above (VT100 to host) is
    #     generated by the VT100 as a DA control sequence with the numeric
    #     parameters as follows:

    #  Option Present               Sequence Sent
    #  --------------               -------------
    #  No options                   ESC [?1;0c
    #  Processor option (STP)       ESC [?1;1c
    #  Advanced video option (AVO)  ESC [?1;2c
    #  AVO and STP                  ESC [?1;3c
    #  Graphics option (GPO)        ESC [?1;4c
    #  GPO and STP                  ESC [?1;5c
    #  GPO and AVO                  ESC [?1;6c
    #  GPO, STP and AVO             ESC [?1;7c

    action DA {
        printActionWithNumbers("deviceAttributes", this->numberStack); 
    }

    deviceAttributes = CSI multiple_numeric_parameters 'c' @DA;
    
    # DECALN - Screen Alignment Display (DEC Private) - ESC # 8  

    #  This command fills the entire screen area with uppercase Es for screen
    #  focus and alignment. This command is used by DEC manufacturing and
    #  Field Service personnel.

    action DECALN {
        printAction("screenAlignmentDisplay"); 
    }

    screenAlignmentDisplay = ESC '\#' '8' @DECALN;

    # DECANM -- ANSI/VT52 Mode (DEC Private)

    #  This is a private parameter applicable to set mode (SM) and reset mode
    #  (RM) control sequences. The reset state causes only VT52 compatible
    #  escape sequences to be interpreted and executed. The set state causes
    #  only ANSI "compatible" escape and control sequences to be interpreted
    #  and executed.

# TODO!!!!
# TODO!!!! what is the sequence
# TODO!!!!

    # DECARM -- Auto Repeat Mode (DEC Private)

    #  This is a private parameter applicable to set mode (SM) and reset mode
    #  (RM) control sequences. The reset state causes no keyboard keys to
    #  auto-repeat. The set state causes certain keyboard keys to
    #  auto-repeat.

# TODO!!!!
# TODO!!!! what is the sequence
# TODO!!!!

    # DECAWM -- Autowrap Mode (DEC Private)

    #  This is a private parameter applicable to set mode (SM) and reset mode
    #  (RM) control sequences. The reset state causes any displayable
    #  characters received when the cursor is at the right margin to replace
    #  any previous characters there. The set state causes these characters
    #  to advance to the start of the next line, doing a scroll up if
    #  required and permitted.

# TODO!!!!
# TODO!!!! what is the sequence
# TODO!!!!

    # DECCKM -- Cursor Keys Mode (DEC Private)

    #  This is a private parameter applicable to set mode (SM) and reset mode
    #  (RM) control sequences. This mode is only effective when the terminal
    #  is in keypad application mode (see DECKPAM) and the ANSI/VT52 mode
    #  (DECANM) is set (see DECANM). Under these conditions, if the cursor
    #  key mode is reset, the four cursor function keys will send ANSI cursor
    #  control commands. If cursor key mode is set, the four cursor function
    #  keys will send application functions.

# TODO!!!!
# TODO!!!! what is the sequence
# TODO!!!!

    # DECCOLM -- Column Mode (DEC Private)

    #  This is a private parameter applicable to set mode (SM) and reset mode
    #  (RM) control sequences. The reset state causes a maximum of 80 columns
    #  on the screen. The set state causes a maximum of 132 columns on the
    #  screen.

# TODO!!!!
# TODO!!!! what is the sequence
# TODO!!!!

    # DECDHL -- Double Height Line (DEC Private)

    #  Top Half: ESC # 3        
    #  Bottom Half: ESC # 4
         
    #  These sequences cause the line containing the active position to
    #  become the top or bottom half of a double-height double-width
    #  line. The sequences must be used in pairs on adjacent lines and the
    #  same character output must be sent to both lines to form full
    #  double-height characters. If the line was single-width single-height,
    #  all characters to the right of the center of the screen are lost. The
    #  cursor remains over the same character position unless it would be to
    #  the right of the right margin, in which case it is moved to the right
    #  margin.

    #  NOTE: The use of double-width characters reduces the number of
    #  characters per line by half.

    action DECDHL {
        printAction("doubleHeightLine");
    }

    doubleHeightLine = ESC '\#' '3' @DECDHL;

    # DECDWL - Double-Width Line (DEC Private) - ESC # 6  

    #  This causes the line that contains the active position to become
    #  double-width single-height. If the line was single-width
    #  single-height, all characters to the right of the screen are lost. The
    #  cursor remains over the same character position unless it would be to
    #  the right of the right margin, in which case, it is moved to the right
    #  margin.

    #  NOTE: The use of double-width characters reduces the number of
    #  characters per line by half.

    action DECDWL {
        printAction("doubleWidthLine");
    }

    doubleWidthLine = ESC '\#' '6' @DECDWL;

    # DECID - Identify Terminal (DEC Private) - ESC Z

    #  This sequence causes the same response as the ANSI device attributes
    #  (DA). This sequence will not be supported in future DEC terminals,
    #  therefore, DA should be used by any new software.

    action DECID {
        printAction("identifyTerminal");
    }

    identifyTerminal = ESC 'Z' @DECID;

    # DECINLM - Interlace Mode (DEC Private)

    #  This is a private parameter applicable to set mode (SM) and reset mode
    #  (RM) control sequences. The reset state (non-interlace) causes the
    #  video processor to display 240 scan lines per frame. The set state
    #  (interlace) causes the video processor to display 480 scan lines per
    #  frame. There is no increase in character resolution.

# TODO!!!!
# TODO!!!! what is the sequence
# TODO!!!!

    # DECKPAM - Keypad Application Mode (DEC Private) - ESC =    

    #  The auxiliary keypad keys will transmit control sequences as defined
    #  in Tables 3-7 and 3-8.

    #  3-7
    #  KNM - Keypad Numeric Mode
    #  KAMK - Keypad Application Mode	Key
    #  Key      Keypad Numeric Mode   Keypad Application Mode
    #   0                0                    ESC ? p
    #   1                1                    ESC ? q
    #   2                2                    ESC ? r
    #   3                3                    ESC ? s
    #   4                4                    ESC ? t
    #   5                5                    ESC ? u
    #   6                6                    ESC ? v
    #   7                7                    ESC ? w
    #   8                8                    ESC ? x
    #   9                9                    ESC ? y

    #  * The last character of this escape sequence is a lowercase L (1548).

    action DECKPAM {
        printAction("keypadApplicationMode"); 
    }

    keypadApplicationMode = ESC '=' @DECKPAM;

    # DECKPNM - Keypad Numeric Mode (DEC Private) - ESC >    

    #  The auxiliary keypad keys will send ASCII codes corresponding to the
    #  characters engraved on the keys.

    action DECKPNM {
        printAction("keypadNumericMode"); 
    }

    keypadNumericMode = ESC '>' @DECKPNM;

    # DECLL - Load LEDS (DEC Private) - ESC [ Ps q - default value: 0

    #  Load the four programmable LEDs on the keyboard according to the
    #  parameter(s).

    #  Parameter  Parameter Meaning
    #  ---------  -----------------
    #  0          Clear LEDs L1 through L4
    #  1          Light L1
    #  2          Light L2
    #  3          Light L3
    #  4          Light L4

    #  LED numbers are indicated on the keyboard.

    action DECLL {
        printActionWithNumbers("loadLEDs", this->numberStack); 
    }

    loadLEDs = CSI multiple_numeric_parameters 'q' @DECLL;

    # DECOM - Origin Mode (DEC Private)

    #  This is a private parameter applicable to set mode (SM) and reset mode
    #  (RM) control sequences. The reset state causes the origin to be at the
    #  upper-left character position on the screen. Line and column numbers
    #  are, therefore, independent of current margin settings. The cursor may
    #  be positioned outside the margins with a cursor position (CUP) or
    #  horizontal and vertical position (HVP) control.

    #  The set state causes the origin to be at the upper-left character
    #  position within the margins. Line and column numbers are therefore
    #  relative to the current margin settings. The cursor is not allowed to
    #  be positioned outside the margins.

    #  The cursor is moved to the new home position when this mode is set or
    #  reset.

    #  Lines and columns are numbered consecutively, with the origin being
    #  line 1, column 1.

# TODO
# TODO
# TODO

    # DECRC - Restore Cursor (DEC Private) - ESC 8

    #  This sequence causes the previously saved cursor position, graphic
    #  rendition, and character set to be restored.

    action DECRC {
        printAction("restoreCursor"); 
    }

    restoreCursor = ESC '8' @DECRC;

    # DECREPTPARM - Report Terminal Parameters
    #  - ESC [ <sol>; <par>; <nbits>; <xspeed>; <rspeed>; <clkmul>; <flags> x     

    #  These sequence parameters are explained below in the DECREQTPARM sequence.

    action DECREPTPARM {
        printActionWithNumbers("requestOrReportTerminalParameters", 
            this->numberStack); 
    }

    requestOrReportTerminalParameters = CSI multiple_numeric_parameters 
        'x' @DECREPTPARM;

    # DECREQTPARM - Request Terminal Parameters - ESC [ <sol> x    

    #  The sequence DECREPTPARM is sent by the terminal controller to notify
    #  the host of the status of selected terminal parameters. The status
    #  sequence may be sent when requested by the host or at the terminal's
    #  discretion. DECREPTPARM is sent upon receipt of a DECREQTPARM. On
    #  power-up or reset, the VT100 is inhibited from sending unsolicited
    #  reports.

    #  The meanings of the sequence parameters are:

    #  Parameter           Value      Meaning
    #
    #  <sol>               0 or none  This message is a request (DECREQTPARM) and 
    #                                 the terminal will be allowed to send 
    #                                 unsolicited reports. (Unsolicited reports 
    #                                 are sent when the 
    #                                 terminal exits the SET-UP mode).
    #                      1          This message is a request; from now on the 
    #                                 terminal may only report in response to a 
    #                                 request.
    #                      2          This message is a report (DECREPTPARM).
    #                      3          This message is a report and the terminal is 
    #                                 only reporting on request.
    #  <par>               1          No parity set
    #                      4          Parity is set and odd
    #                      5          Parity is set and even
    #  <nbits>             1          8 bits per character
    #                      2          7 bits per character
    #  <xspeed>, <rspeed>  0  50      Bits per second
    #                      8  75
    #                      16 110
    #                      24 134.5
    #                      32 150
    #                      40 200
    #                      48 300
    #                      56 600
    #                      64 1200
    #                      72 1800
    #                      80 2000
    #                      88 2400
    #                      96 3600
    #                      104 4800
    #                      112 9600
    #                      120 19200
    #  <clkmul>            1          The bit rate multiplier is 16.
    #  <flags>             0-15       This value communicates the four switch 
    #                                 values in block 5 of SET UP B, which are 
    #                                 only visible to the user when an STP option 
    #                                 is installed. These bits may be assigned for
    #                                 an STP device. The four bits are a 
    #                                 decimal-encoded binary number.

    # handled just above via: requestOrReportTerminalParameters


    # DECSC - Save Cursor (DEC Private) - ESC 7

    #  This sequence causes the cursor position, graphic rendition, and
    #  character set to be saved. (See DECRC).

    action DECSC {
        printAction("saveCursor"); 
    }

    saveCursor = ESC '7' @DECSC;

    # DECSCLM - Scrolling Mode (DEC Private)

    #  This is a private parameter applicable to set mode (SM) and reset mode
    #  (RM) control sequences. The reset state causes scrolls to "jump"
    #  instantaneously. The set state causes scrolls to be "smooth" at a
    #  maximum rate of six lines per second.

# TODO
# TODO
# TODO

    # DECSCNM -- Screen Mode (DEC Private)

    #  This is a private parameter applicable to set mode (SM) and reset mode
    #  (RM) control sequences. The reset state causes the screen to be black
    #  with white characters. The set state causes the screen to be white
    #  with black characters.

# TODO
# TODO
# TODO

    # DECSTBM -- Set Top and Bottom Margins (DEC Private)
    #   - ESC [ Pn; Pn r - default values: see below

    #  This sequence sets the top and bottom margins to define the scrolling
    #  region. The first parameter is the line number of the first line in
    #  the scrolling region; the second parameter is the line number of the
    #  bottom line in the scrolling region. Default is the entire screen (no
    #  margins). The minimum size of the scrolling region allowed is two
    #  lines, i.e., the top margin must be less than the bottom margin. The
    #  cursor is placed in the home position (see Origin Mode DECOM).

    action DECSTBM {
        printActionWithNumbers("setTopAndBottomMargins", this->numberStack); 
    }

    setTopAndBottomMargins = CSI multiple_numeric_parameters 'r' @DECSTBM;

    # DECSWL - Single-width Line (DEC Private) - ESC # 5  

    #  This causes the line which contains the active position to become
    #  single-width single-height. The cursor remains on the same character
    #  position. This is the default condition for all new lines on the
    #  screen.

    action DECSWL {
        printAction("singleWidthLine"); 
    }

    singleWidthLine = ESC '\#' '5' @DECSWL;

    # DECTST - Invoke Confidence Test - ESC [ 2 ; Ps y   

    #  Ps is the parameter indicating the test to be done. Ps is computed by
    #  taking the weight indicated for each desired test and adding them
    #  together. If Ps is 0, no test is performed but the VT100 is reset.

    #  Test -> Weight
    #  Power up self-test(ROM check sum, RAM, NVR keyboard & AVO if installed) -> 1
    #  Data Loop Back -> 2 (loop back connector required)
    #  EIA modem control test -> 4 (loop back connector required)
    #  Repeat Selected Test(s) indefinitely (until failure or power off) -> 8

    action DECTST {
        printActionWithNumbers("invokeConfidenceTest", this->numberStack); 
    }

    invokeConfidenceTest = CSI multiple_numeric_parameters 'y' @DECTST;

    # DSR - Device Status Report - ESC [ Ps n - default value: 0

    #  Requests and reports the general status of the VT100 according to the
    #  following parameter(s).

    #  Parameter  Parameter Meaning
    #  ---------  -----------------
    #  0          Response from VT100 -- Ready, No malfunctions detected (default)
    #  3          Response from VT100 -- Malfunction -- retry
    #  5          Command from host -- Please report status (using a DSR control 
    #             sequence)
    #  6          Command from host -- Please report active position (using a CPR 
    #             control sequence)

    #  DSR with a parameter value of 0 or 3 is always sent as a response to a
    #  requesting DSR with a parameter value of 5.

    action DSR {
        printActionWithNumbers("deviceStatusReport", this->numberStack); 
    }

    deviceStatusReport = CSI multiple_numeric_parameters 'n' @DSR;

    # ED - Erase In Display - ESC [ Ps J - default value: 0

    #  This sequence erases some or all of the characters in the display
    #  according to the parameter. Any complete line erased by this sequence
    #  will return that line to single width mode. Editor Function

    #  Parameter  Parameter Meaning
    #  ---------  -----------------
    #  0          Erase from the active position to the end of the screen, 
    #             inclusive (default)
    #  1          Erase from start of the screen to the active position, inclusive
    #  2          Erase all of the display -- all lines are erased, changed to 
    #             single-width, and the cursor does not move.

    action ED {
        printActionWithNumbers("eraseInDisplay", this->numberStack); 
    }

    eraseInDisplay = CSI multiple_numeric_parameters 'J' @ED;

    # EL - Erase In Line - ESC [ Ps K -  default value: 0

    #  Erases some or all characters in the active line according to the
    #  parameter. Editor Function

    #  Parameter  Parameter Meaning
    #  ---------  -----------------
    #  0          Erase from the active pos to the end of the line, inclusive 
    #             (default)
    #  1          Erase from the start of the screen to the active pos, inclusive
    #  2          Erase all of the line, inclusive

    action eraseInLine {
        printActionWithNumbers("eraseInLine", this->numberStack); 
    }

    eraseInLine = CSI multiple_numeric_parameters 'K' @eraseInLine;

    # HTS - Horizontal Tabulation Set - ESC H    

    #  Set one horizontal stop at the active position. Format Effector

    action HTS {
        printAction("horizontalTabulationSet"); 
    }

    horizontalTabulationSet = ESC 'H' @HTS;

    # HVP - Horizontal and Vertical Position - ESC [ Pn ; Pn f - default value: 1

    #  Moves the active position to the position specified by the
    #  parameters. This sequence has two parameter values, the first
    #  specifying the line position and the second specifying the column. A
    #  parameter value of either zero or one causes the active position to
    #  move to the first line or column in the display, respectively. The
    #  default condition with no parameters present moves the active position
    #  to the home position. In the VT100, this control behaves identically
    #  with its editor function counterpart, CUP. The numbering of lines and
    #  columns depends on the reset or set state of the origin mode
    #  (DECOM). Format Effector

    action HVP {
        printActionWithNumbers("horizontalAndVerticalPosition", 
            this->numberStack); 
    }

    horizontalAndVerticalPosition = CSI multiple_numeric_parameters 'f' @HVP;

    # IND - Index - ESC D    

    #  This sequence causes the active position to move downward one line
    #  without changing the column position. If the active position is at the
    #  bottom margin, a scroll up is performed. Format Effector

    action IND {
        printAction("index");
    }

    index = ESC 'D' @IND;

    # LNM - Line Feed/New Line Mode

    #  This is a parameter applicable to set mode (SM) and reset mode (RM)
    #  control sequences. The reset state causes the interpretation of the
    #  line feed (LF), defined in ANSI Standard X3.4-1977, to imply only
    #  vertical movement of the active position and causes the RETURN key
    #  (CR) to send the single code CR. The set state causes the LF to imply
    #  movement to the first position of the following line and causes the
    #  RETURN key to send the two codes (CR, LF). This is the New Line (NL)
    #  option.

    #  This mode does not affect the index (IND), or next line (NEL) format 
    #  effectors.

# TODO
# TODO
# TODO

    # NEL - Next Line - ESC E    

    #  This sequence causes the active position to move to the first position
    #  on the next line downward. If the active position is at the bottom
    #  margin, a scroll up is performed. Format Effector

    action NEL {
        printAction("nextLine"); 
    }

    nextLine = ESC 'E' @NEL;

    # RI - Reverse Index - ESC M    

    #  Move the active position to the same horizontal position on the
    #  preceding line. If the active position is at the top margin, a scroll
    #  down is performed. Format Effector

    action RI {
        printAction("reverseIndex"); 
    }

    reverseIndex = ESC 'M' @RI;

    # RIS - Reset To Initial State - ESC c    

    #  Reset the VT100 to its initial state, i.e., the state it has after it
    #  is powered on. This also causes the execution of the power-up
    #  self-test and signal INIT H to be asserted briefly.

    action RIS {
        printAction("resetToInitialState"); 
    }

    resetToInitialState = ESC 'c' @RIS;

    # RM - Reset Mode - ESC [ Ps ; Ps ; . . . ; Ps l - default value: none

    #  Resets one or more VT100 modes as specified by each selective
    #  parameter in the parameter string. Each mode to be reset is specified
    #  by a separate parameter. [See Set Mode (SM) control sequence]. (See
    #  Modes following this section).

    action RM {
        printActionWithNumbers("resetMode", this->numberStack); 
    }

    resetMode = CSI multiple_numeric_parameters 'l' @RM;

    # SCS - Select Character Set

    #  The appropriate G0 and G1 character sets are designated from one of
    #  the five possible character sets. The G0 and G1 sets are invoked by
    #  the codes SI and SO (shift in and shift out) respectively.

    #  G0 Sets Sequence  G1 Sets Sequence  Meaning
    #  ----------------  ----------------  -------
    #  ESC ( A           ESC ) A           United Kingdom Set
    #  ESC ( B           ESC ) B           ASCII Set
    #  ESC ( 0           ESC ) 0           Special Graphics
    #  ESC ( 1           ESC ) 1           Alt Character ROM Standard Char Set
    #  ESC ( 2           ESC ) 2           Alt Character ROM Special Graphics

    #  The United Kingdom and ASCII sets conform to the "ISO international
    #  register of character sets to be used with escape sequences". The
    #  other sets are private character sets. Special graphics means that the
    #  graphic characters for the codes 1378 to 1768 are replaced with other
    #  characters. The specified character set will be used until another SCS
    #  is received.

    #  NOTE: Additional information concerning the SCS escape sequence may be
    #  obtained in ANSI standard X3.41-1974.

# TODO
# TODO
# TODO

    # SGR - Select Graphic Rendition - ESC [ Ps ; . . . ; Ps m - default value: 0

    #  Invoke the graphic rendition specified by the parameter(s). All
    #  following characters transmitted to the VT100 are rendered according
    #  to the parameter(s) until the next occurrence of SGR. Format Effector

    #  Parameter  Parameter Meaning
    #  ---------  -----------------
    #  0          Attributes off
    #  1          Bold or increased intensity
    #  4          Underscore
    #  5          Blink
    #  7          Negative (reverse) image
    #  All other parameter values are ignored.

    #  With the Advanced Video Option, only one type of character attribute
    #  is possible as determined by the cursor selection; in that case
    #  specifying either the underscore or the reverse attribute will
    #  activate the currently selected attribute. (See cursor selection in
    #  Chapter 1).

    action SGR {
        printActionWithNumbers("selectGraphicRendition", this->numberStack); 
    }

    selectGraphicRendition = CSI multiple_numeric_parameters 'm' @SGR;

    # SM - Set Mode - ESC [ Ps ; . . . ; Ps h - default value: none

    #  Causes one or more modes to be set within the VT100 as specified by
    #  each selective parameter in the parameter string. Each mode to be set
    #  is specified by a separate parameter. A mode is considered set until
    #  it is reset by a reset mode (RM) control sequence.

    action SM {
        printActionWithNumbers("setMode", this->numberStack); 
    }

    setMode = CSI multiple_numeric_parameters 'h' @SM;

    # TBC - Tabulation Clear - ESC [ Ps g - default value: 0

    #  Parameter  Parameter Meaning
    #  ---------  -----------------
    #  0          Clear the horizontal tab stop at the active position (the default
    #             case).
    #  3          Clear all horizontal tab stops.

    #  Any other parameter values are ignored. Format Effector

    action TBC { 
        printActionWithNumbers("tabulationClear", this->numberStack); 
    }

    tabulationClear = CSI multiple_numeric_parameters 'g' @TBC;

    vt100 = cursorPositionReport 
        | cursorBackward
        | cursorDown
        | cursorForward
        | cursorPosition
        | cursorUp
        | deviceAttributes 
        | screenAlignmentDisplay
        | doubleHeightLine
        | doubleWidthLine
        | identifyTerminal
        | keypadApplicationMode
        | keypadNumericMode
        | loadLEDs
        | restoreCursor
        | requestOrReportTerminalParameters
        | saveCursor
        | setTopAndBottomMargins
        | singleWidthLine
        | invokeConfidenceTest
        | deviceStatusReport
        | eraseInDisplay
        | eraseInLine
        | horizontalTabulationSet
        | horizontalAndVerticalPosition
        | index
        | nextLine
        | reverseIndex
        | resetToInitialState
        | resetMode
        | selectGraphicRendition
        | setMode
        | tabulationClear;

    action setTitle { printAction("setTitle"); }

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
            for(i = fpc - 4; i <= pe; i++) {
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

    titleChange = OSC ('0' | '1' | '2' | '3' | '4') ';' any+ :> 0x07 @setTitle;

    terminalSetup = CSI '7' 'h' @enableLineWrap
        | CSI '7' 'l' @disableLineWrap;

    erase = CSI '0'? 'K' @eraseInLineFromCursorToRight
        | CSI '1' 'K' @eraseInLineFromCursorToLeft
        | CSI '2' 'K' @eraseEntireLine
        | CSI '0'? 'J' @eraseScreenFromCursorDown
        | CSI '1' 'J' @eraseScreenFromCursorUp
        | CSI '2' 'J' @eraseEntireScreen;

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
        | terminalSetup
        | erase
        | unknown
        | vt100
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
