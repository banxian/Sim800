#include "NekoDriver.h"
extern "C" {
#include "ANSI/65c02.h"
}
#include <QtCore/QDebug>


#define  BREAKPOINTS     5
#define  COMMANDLINES    5
#define  COMMANDS        39
#define  MAXARGS         40
#define  SOURCELINES     19
#define  STACKLINES      9
#define  WATCHES         5

#define  SCREENSPLIT1    356
#define  SCREENSPLIT2    456

#define  INVALID1        1
#define  INVALID2        2
#define  INVALID3        3
#define  ADDR_IMM        4
#define  ADDR_ABS        5
#define  ADDR_ZPG        6
#define  ADDR_ABSX       7
#define  ADDR_ABSY       8
#define  ADDR_ZPGX       9
#define  ADDR_ZPGY       10
#define  ADDR_REL        11
#define  ADDR_BBREL      12
#define  ADDR_INDX       13
#define  ADDR_ABSIINDX   14
#define  ADDR_INDY       15
#define  ADDR_IZPG       16
#define  ADDR_IABS       17

#define  COLOR_INSTBKG   0
#define  COLOR_INSTTEXT  1
#define  COLOR_INSTBP    2
#define  COLOR_DATABKG   3
#define  COLOR_DATATEXT  4
#define  COLOR_STATIC    5
#define  COLOR_BPDATA    6
#define  COLOR_COMMAND   7
#define  COLORS          8


typedef struct _addrrec {
    TCHAR format[12];
    int   bytes;
} addrrec, *addrptr;


typedef struct _instrec {
    TCHAR mnemonic[5];
    int   addrmode;
} instrec, *instptr;



addrrec addressmode[19]  = { {TEXT("")         ,1},        // (implied)
{TEXT("")         ,1},        // INVALID1
{TEXT("")         ,2},        // INVALID2
{TEXT("")         ,3},        // INVALID3
{TEXT("#$%02X")   ,2},        // ADDR_IMM
{TEXT("%s")       ,3},        // ADDR_ABS
{TEXT("%s")       ,2},        // ADDR_ZPG
{TEXT("%s,X")     ,3},        // ADDR_ABSX
{TEXT("%s,Y")     ,3},        // ADDR_ABSY
{TEXT("%s,X")     ,2},        // ADDR_ZPGX
{TEXT("%s,Y")     ,2},        // ADDR_ZPGY
{TEXT("%s")       ,2},        // ADDR_REL
{TEXT("%s %s")    ,3},        // ADDR_BBREL
{TEXT("(%s,X)")   ,2},        // ADDR_INDX
{TEXT("(%s,X)")   ,3},        // ADDR_ABSIINDX
{TEXT("(%s),Y")   ,2},        // ADDR_INDY
{TEXT("(%s)")     ,2},        // ADDR_IZPG
{TEXT("(%s)")     ,3},        // ADDR_IABS
{TEXT("IO PAGE")  ,1} };      // ADDR_IO PAGE

instrec instruction[257] = { {TEXT("BRK"),ADDR_ABS},              // 00h
{TEXT("ORA"),ADDR_INDX},      // 01h
{TEXT("NOP"),INVALID2},       // 02h
{TEXT("NOP"),INVALID1},       // 03h
{TEXT("TSB"),ADDR_ZPG},       // 04h
{TEXT("ORA"),ADDR_ZPG},       // 05h
{TEXT("ASL"),ADDR_ZPG},       // 06h
{TEXT("RMB0"),ADDR_ZPG},      // 07h
{TEXT("PHP"),0},              // 08h
{TEXT("ORA"),ADDR_IMM},       // 09h
{TEXT("ASL"),0},              // 0Ah
{TEXT("NOP"),INVALID1},       // 0Bh
{TEXT("TSB"),ADDR_ABS},       // 0Ch
{TEXT("ORA"),ADDR_ABS},       // 0Dh
{TEXT("ASL"),ADDR_ABS},       // 0Eh
{TEXT("BBR0"),ADDR_BBREL},    // 0Fh
{TEXT("BPL"),ADDR_REL},       // 10h
{TEXT("ORA"),ADDR_INDY},      // 11h
{TEXT("ORA"),ADDR_IZPG},      // 12h
{TEXT("NOP"),INVALID1},       // 13h
{TEXT("TRB"),ADDR_ZPG},       // 14h
{TEXT("ORA"),ADDR_ZPGX},      // 15h
{TEXT("ASL"),ADDR_ZPGX},      // 16h
{TEXT("RMB1"),ADDR_ZPG},      // 17h
{TEXT("CLC"),0},              // 18h
{TEXT("ORA"),ADDR_ABSY},      // 19h
{TEXT("INA"),0},              // 1Ah
{TEXT("NOP"),INVALID1},       // 1Bh
{TEXT("TRB"),ADDR_ABS},       // 1Ch
{TEXT("ORA"),ADDR_ABSX},      // 1Dh
{TEXT("ASL"),ADDR_ABSX},      // 1Eh
{TEXT("BBR1"),ADDR_BBREL},    // 1Fh
{TEXT("JSR"),ADDR_ABS},       // 20h
{TEXT("AND"),ADDR_INDX},      // 21h
{TEXT("NOP"),INVALID2},       // 22h
{TEXT("NOP"),INVALID1},       // 23h
{TEXT("BIT"),ADDR_ZPG},       // 24h
{TEXT("AND"),ADDR_ZPG},       // 25h
{TEXT("ROL"),ADDR_ZPG},       // 26h
{TEXT("RMB2"),ADDR_ZPG},      // 27h
{TEXT("PLP"),0},              // 28h
{TEXT("AND"),ADDR_IMM},       // 29h
{TEXT("ROL"),0},              // 2Ah
{TEXT("NOP"),INVALID1},       // 2Bh
{TEXT("BIT"),ADDR_ABS},       // 2Ch
{TEXT("AND"),ADDR_ABS},       // 2Dh
{TEXT("ROL"),ADDR_ABS},       // 2Eh
{TEXT("BBR2"),ADDR_BBREL},    // 2Fh
{TEXT("BMI"),ADDR_REL},       // 30h
{TEXT("AND"),ADDR_INDY},      // 31h
{TEXT("AND"),ADDR_IZPG},      // 32h
{TEXT("NOP"),INVALID1},       // 33h
{TEXT("BIT"),ADDR_ZPGX},      // 34h
{TEXT("AND"),ADDR_ZPGX},      // 35h
{TEXT("ROL"),ADDR_ZPGX},      // 36h
{TEXT("RMB3"),ADDR_ZPG},      // 37h
{TEXT("SEC"),0},              // 38h
{TEXT("AND"),ADDR_ABSY},      // 39h
{TEXT("DEA"),0},              // 3Ah
{TEXT("NOP"),INVALID1},       // 3Bh
{TEXT("BIT"),ADDR_ABSX},      // 3Ch
{TEXT("AND"),ADDR_ABSX},      // 3Dh
{TEXT("ROL"),ADDR_ABSX},      // 3Eh
{TEXT("BBR3"),ADDR_BBREL},    // 3Fh
{TEXT("RTI"),0},              // 40h
{TEXT("EOR"),ADDR_INDX},      // 41h
{TEXT("NOP"),INVALID2},       // 42h
{TEXT("NOP"),INVALID1},       // 43h
{TEXT("NOP"),INVALID2},       // 44h
{TEXT("EOR"),ADDR_ZPG},       // 45h
{TEXT("LSR"),ADDR_ZPG},       // 46h
{TEXT("RMB4"),ADDR_ZPG},      // 47h
{TEXT("PHA"),0},              // 48h
{TEXT("EOR"),ADDR_IMM},       // 49h
{TEXT("LSR"),0},              // 4Ah
{TEXT("NOP"),INVALID1},       // 4Bh
{TEXT("JMP"),ADDR_ABS},       // 4Ch
{TEXT("EOR"),ADDR_ABS},       // 4Dh
{TEXT("LSR"),ADDR_ABS},       // 4Eh
{TEXT("BBR4"),ADDR_BBREL},    // 4Fh
{TEXT("BVC"),ADDR_REL},       // 50h
{TEXT("EOR"),ADDR_INDY},      // 51h
{TEXT("EOR"),ADDR_IZPG},      // 52h
{TEXT("NOP"),INVALID1},       // 53h
{TEXT("NOP"),INVALID2},       // 54h
{TEXT("EOR"),ADDR_ZPGX},      // 55h
{TEXT("LSR"),ADDR_ZPGX},      // 56h
{TEXT("RMB5"),ADDR_ZPG},      // 57h
{TEXT("CLI"),0},              // 58h
{TEXT("EOR"),ADDR_ABSY},      // 59h
{TEXT("PHY"),0},              // 5Ah
{TEXT("NOP"),INVALID1},       // 5Bh
{TEXT("NOP"),INVALID3},       // 5Ch
{TEXT("EOR"),ADDR_ABSX},      // 5Dh
{TEXT("LSR"),ADDR_ABSX},      // 5Eh
{TEXT("BBR5"),ADDR_BBREL},    // 5Fh
{TEXT("RTS"),0},              // 60h
{TEXT("ADC"),ADDR_INDX},      // 61h
{TEXT("NOP"),INVALID2},       // 62h
{TEXT("NOP"),INVALID1},       // 63h
{TEXT("STZ"),ADDR_ZPG},       // 64h
{TEXT("ADC"),ADDR_ZPG},       // 65h
{TEXT("ROR"),ADDR_ZPG},       // 66h
{TEXT("RMB6"),ADDR_ZPG},      // 67h
{TEXT("PLA"),0},              // 68h
{TEXT("ADC"),ADDR_IMM},       // 69h
{TEXT("ROR"),0},              // 6Ah
{TEXT("NOP"),INVALID1},       // 6Bh
{TEXT("JMP"),ADDR_IABS},      // 6Ch
{TEXT("ADC"),ADDR_ABS},       // 6Dh
{TEXT("ROR"),ADDR_ABS},       // 6Eh
{TEXT("BBR6"),ADDR_BBREL},    // 6Fh
{TEXT("BVS"),ADDR_REL},       // 70h
{TEXT("ADC"),ADDR_INDY},      // 71h
{TEXT("ADC"),ADDR_IZPG},      // 72h
{TEXT("NOP"),INVALID1},       // 73h
{TEXT("STZ"),ADDR_ZPGX},      // 74h
{TEXT("ADC"),ADDR_ZPGX},      // 75h
{TEXT("ROR"),ADDR_ZPGX},      // 76h
{TEXT("RMB7"),ADDR_ZPG},      // 77h
{TEXT("SEI"),0},              // 78h
{TEXT("ADC"),ADDR_ABSY},      // 79h
{TEXT("PLY"),0},              // 7Ah
{TEXT("NOP"),INVALID1},       // 7Bh
{TEXT("JMP"),ADDR_ABSIINDX},  // 7Ch
{TEXT("ADC"),ADDR_ABSX},      // 7Dh
{TEXT("ROR"),ADDR_ABSX},      // 7Eh
{TEXT("BBR7"),ADDR_BBREL},    // 7Fh
{TEXT("BRA"),ADDR_REL},       // 80h
{TEXT("STA"),ADDR_INDX},      // 81h
{TEXT("NOP"),INVALID2},       // 82h
{TEXT("NOP"),INVALID1},       // 83h
{TEXT("STY"),ADDR_ZPG},       // 84h
{TEXT("STA"),ADDR_ZPG},       // 85h
{TEXT("STX"),ADDR_ZPG},       // 86h
{TEXT("SMB0"),ADDR_ZPG},      // 87h
{TEXT("DEY"),0},              // 88h
{TEXT("BIT"),ADDR_IMM},       // 89h
{TEXT("TXA"),0},              // 8Ah
{TEXT("NOP"),INVALID1},       // 8Bh
{TEXT("STY"),ADDR_ABS},       // 8Ch
{TEXT("STA"),ADDR_ABS},       // 8Dh
{TEXT("STX"),ADDR_ABS},       // 8Eh
{TEXT("BBS0"),ADDR_BBREL},    // 8Fh
{TEXT("BCC"),ADDR_REL},       // 90h
{TEXT("STA"),ADDR_INDY},      // 91h
{TEXT("STA"),ADDR_IZPG},      // 92h
{TEXT("NOP"),INVALID1},       // 93h
{TEXT("STY"),ADDR_ZPGX},      // 94h
{TEXT("STA"),ADDR_ZPGX},      // 95h
{TEXT("STX"),ADDR_ZPGY},      // 96h
{TEXT("SMB1"),ADDR_ZPG},      // 97h
{TEXT("TYA"),0},              // 98h
{TEXT("STA"),ADDR_ABSY},      // 99h
{TEXT("TXS"),0},              // 9Ah
{TEXT("NOP"),INVALID1},       // 9Bh
{TEXT("STZ"),ADDR_ABS},       // 9Ch
{TEXT("STA"),ADDR_ABSX},      // 9Dh
{TEXT("STZ"),ADDR_ABSX},      // 9Eh
{TEXT("BBS1"),ADDR_BBREL},    // 9Fh
{TEXT("LDY"),ADDR_IMM},       // A0h
{TEXT("LDA"),ADDR_INDX},      // A1h
{TEXT("LDX"),ADDR_IMM},       // A2h
{TEXT("NOP"),INVALID1},       // A3h
{TEXT("LDY"),ADDR_ZPG},       // A4h
{TEXT("LDA"),ADDR_ZPG},       // A5h
{TEXT("LDX"),ADDR_ZPG},       // A6h
{TEXT("SMB2"),ADDR_ZPG},      // A7h
{TEXT("TAY"),0},              // A8h
{TEXT("LDA"),ADDR_IMM},       // A9h
{TEXT("TAX"),0},              // AAh
{TEXT("NOP"),INVALID1},       // ABh
{TEXT("LDY"),ADDR_ABS},       // ACh
{TEXT("LDA"),ADDR_ABS},       // ADh
{TEXT("LDX"),ADDR_ABS},       // AEh
{TEXT("BBS2"),ADDR_BBREL},    // AFh
{TEXT("BCS"),ADDR_REL},       // B0h
{TEXT("LDA"),ADDR_INDY},      // B1h
{TEXT("LDA"),ADDR_IZPG},      // B2h
{TEXT("NOP"),INVALID1},       // B3h
{TEXT("LDY"),ADDR_ZPGX},      // B4h
{TEXT("LDA"),ADDR_ZPGX},      // B5h
{TEXT("LDX"),ADDR_ZPGY},      // B6h
{TEXT("SMB3"),ADDR_ZPG},       // B7h
{TEXT("CLV"),0},              // B8h
{TEXT("LDA"),ADDR_ABSY},      // B9h
{TEXT("TSX"),0},              // BAh
{TEXT("NOP"),INVALID1},       // BBh
{TEXT("LDY"),ADDR_ABSX},      // BCh
{TEXT("LDA"),ADDR_ABSX},      // BDh
{TEXT("LDX"),ADDR_ABSY},      // BEh
{TEXT("BBS3"),ADDR_BBREL},    // BFh
{TEXT("CPY"),ADDR_IMM},       // C0h
{TEXT("CMP"),ADDR_INDX},      // C1h
{TEXT("NOP"),INVALID2},       // C2h
{TEXT("NOP"),INVALID1},       // C3h
{TEXT("CPY"),ADDR_ZPG},       // C4h
{TEXT("CMP"),ADDR_ZPG},       // C5h
{TEXT("DEC"),ADDR_ZPG},       // C6h
{TEXT("SMB4"),ADDR_ZPG},      // C7h
{TEXT("INY"),0},              // C8h
{TEXT("CMP"),ADDR_IMM},       // C9h
{TEXT("DEX"),0},              // CAh
{TEXT("WAI"),INVALID1},       // CBh
{TEXT("CPY"),ADDR_ABS},       // CCh
{TEXT("CMP"),ADDR_ABS},       // CDh
{TEXT("DEC"),ADDR_ABS},       // CEh
{TEXT("BBS4"),ADDR_BBREL},    // CFh
{TEXT("BNE"),ADDR_REL},       // D0h
{TEXT("CMP"),ADDR_INDY},      // D1h
{TEXT("CMP"),ADDR_IZPG},      // D2h
{TEXT("NOP"),INVALID1},       // D3h
{TEXT("NOP"),INVALID2},       // D4h
{TEXT("CMP"),ADDR_ZPGX},      // D5h
{TEXT("DEC"),ADDR_ZPGX},      // D6h
{TEXT("SMB5"),ADDR_ZPG},      // D7h
{TEXT("CLD"),0},              // D8h
{TEXT("CMP"),ADDR_ABSY},      // D9h
{TEXT("PHX"),0},              // DAh
{TEXT("STP"),INVALID1},       // DBh
{TEXT("NOP"),INVALID3},       // DCh
{TEXT("CMP"),ADDR_ABSX},      // DDh
{TEXT("DEC"),ADDR_ABSX},      // DEh
{TEXT("BBS5"),ADDR_BBREL},    // DFh
{TEXT("CPX"),ADDR_IMM},       // E0h
{TEXT("SBC"),ADDR_INDX},      // E1h
{TEXT("NOP"),INVALID2},       // E2h
{TEXT("NOP"),INVALID1},       // E3h
{TEXT("CPX"),ADDR_ZPG},       // E4h
{TEXT("SBC"),ADDR_ZPG},       // E5h
{TEXT("INC"),ADDR_ZPG},       // E6h
{TEXT("SMB6"),ADDR_ZPG},      // E7h
{TEXT("INX"),0},              // E8h
{TEXT("SBC"),ADDR_IMM},       // E9h
{TEXT("NOP"),0},              // EAh
{TEXT("NOP"),INVALID1},       // EBh
{TEXT("CPX"),ADDR_ABS},       // ECh
{TEXT("SBC"),ADDR_ABS},       // EDh
{TEXT("INC"),ADDR_ABS},       // EEh
{TEXT("BBS6"),ADDR_BBREL},    // EFh
{TEXT("BEQ"),ADDR_REL},       // F0h
{TEXT("SBC"),ADDR_INDY},      // F1h
{TEXT("SBC"),ADDR_IZPG},      // F2h
{TEXT("NOP"),INVALID1},       // F3h
{TEXT("NOP"),INVALID2},       // F4h
{TEXT("SBC"),ADDR_ZPGX},      // F5h
{TEXT("INC"),ADDR_ZPGX},      // F6h
{TEXT("SMB7"),ADDR_ZPG},      // F7h
{TEXT("SED"),0},              // F8h
{TEXT("SBC"),ADDR_ABSY},      // F9h
{TEXT("PLX"),0},              // FAh
{TEXT("NOP"),INVALID1},       // FBh
{TEXT("NOP"),INVALID3},       // FCh
{TEXT("SBC"),ADDR_ABSX},      // FDh
{TEXT("INC"),ADDR_ABSX},      // FEh
{TEXT("BBS7"),ADDR_BBREL},    // FFh
{TEXT("   "),18} };           // 100h



LPCTSTR GetSymbol (WORD address, int bytes);
void GetTargets (int *intermediate, int *final, BOOL *ind);




WORD LogDisassembly ( WORD offset, LPTSTR text) {
    TCHAR addresstext[40] = TEXT("");
    TCHAR bytestext[10]   = TEXT("");
    TCHAR fulltext[50]    = TEXT("");
    WORD  inst            = GetByte(offset);

    if (offset < iorange /*(offset & 0xFF00) == (iopage & 0xFF00)*/) {
        // Register address can't be executed
        inst = 0x100;
    }

    int   addrmode        = instruction[inst].addrmode;
    WORD  bytes           = addressmode[addrmode].bytes;


    // Build a string containing the target address or symbol
    if (addressmode[addrmode].format[0]) {

        WORD address = GetWord((offset+1) & 0xffff);

        if (bytes == 2)
            address &= 0xFF;

        if (addrmode == ADDR_IMM) {
            WORD address = (GetWord((offset+1) & 0xffff))& 0xFF;
            wsprintf(addresstext,
                addressmode[addrmode].format,
                (unsigned)address);
        }
        if (addrmode == ADDR_REL) {
            address = (offset+2+(int)(signed char)address) & 0xffff;
            wsprintf(addresstext,
                addressmode[addrmode].format,
                (LPCTSTR)GetSymbol(address,bytes));
            //if ((addrmode == ADDR_REL) && (offset == regs.pc) && CheckJump(address))
            //    if (address > offset)
            //        _tcscat(addresstext,TEXT(" \x19")); // up
            //    else
            //        _tcscat(addresstext,TEXT(" \x18")); // down
        }
        if (addrmode == ADDR_BBREL) {
            address &= 0xFF;
            WORD zpaddr = address;
            address = GetWord((offset+2) & 0xffff);
            address &= 0xFF;
            address = (offset+3+(int)(signed char)address) & 0xffff;

            TCHAR zptxt[14];
            wsprintf(zptxt,(LPCTSTR)GetSymbol(zpaddr,2));
            wsprintf(addresstext,
                addressmode[addrmode].format,
                zptxt,
                (LPCTSTR)GetSymbol(address,3));

            //if ((addrmode == ADDR_BBREL) && (offset == regs.pc) && CheckJump(address))
            //    if (address > offset)
            //        _tcscat(addresstext,TEXT(" \x19"));
            //    else
            //        _tcscat(addresstext,TEXT(" \x18"));
        }

        if (addresstext[0] == 0) //else
            wsprintf(addresstext,
            addressmode[addrmode].format,
            (LPCTSTR)GetSymbol(address,bytes));
    }

    // Build a string containing the actual bytes that make up this
    // instruction
    {
        int loop = 0;
        while (loop < bytes)
            wsprintf(bytestext+_tcslen(bytestext),
            TEXT("%02X"),
            GetWord(offset+(loop++)));
        while (_tcslen(bytestext) < 6)
            _tcscat(bytestext,TEXT(" "));
    }



    // Put together all of the different elements that will make up the line
    wsprintf(fulltext,
        TEXT("%04X  %s  %-9s %-4s %s"),
        (unsigned)offset,
        (LPCTSTR)bytestext,
        (LPCTSTR)GetSymbol(offset,0),
        (LPCTSTR)instruction[inst].mnemonic,
        (LPCTSTR)addresstext);
    if (text)
        _tcscpy(text,fulltext);
    qDebug("%ls", fulltext);

    return bytes;
}


LPCTSTR GetSymbol (WORD address, int bytes) {

    // Perform a binary search through the symbol table looking for a value
    // matching this address

    // If there is no symbol for this address, then just return a string
    // containing the address number
    static TCHAR buffer[8];
    switch (bytes) {
    case 2:   wsprintf(buffer,TEXT("$%02X"),(unsigned)address);  break;
    case 3:   wsprintf(buffer,TEXT("$%04X"),(unsigned)address);  break;
    default:  buffer[0] = 0;                                     break;
    }
    return buffer;
}


//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

