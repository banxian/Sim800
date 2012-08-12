#include "NekoDriver.h"
#include <windows.h>
extern "C" {
#include "ANSI/65c02.h"
}
#include <QtCore/QFile>


BYTE __stdcall NullRead (BYTE read);
void __stdcall NullWrite (BYTE write, BYTE value);

BYTE __stdcall StartTimer0 (BYTE read);
BYTE __stdcall StopTimer0 (BYTE read);
BYTE __stdcall StartTimer1 (BYTE read);
BYTE __stdcall StopTimer1 (BYTE read);
BYTE __stdcall ReadPort0 (BYTE read);
BYTE __stdcall ReadPort1 (BYTE read);

BYTE __stdcall ReadBank (BYTE read);
void __stdcall SwitchBank (BYTE write, BYTE value);
void __stdcall WriteLCDStartAddr (BYTE write, BYTE value);
void __stdcall WriteTimer01Control (BYTE write, BYTE value);
void __stdcall WritePort0 (BYTE write, BYTE value);
void __stdcall WritePort1 (BYTE write, BYTE value);
void __stdcall ControlPort1 (BYTE write, BYTE value);
void __stdcall WriteZeroPageBankswitch (BYTE write, BYTE value); // $0F
void __stdcall WriteROABBS (BYTE write, BYTE value); // $0A


iofunction1 ioread[0x40]  = {
    ReadBank,       // $00
    NullRead,       // $01
    NullRead,       // $02
    NullRead,       // $03
    StopTimer0,     // $04
    StartTimer0,    // $05
    StopTimer1,     // $06
    StartTimer1,    // $07
    ReadPort0,      // $08
    ReadPort1,      // $09
    NullRead,       // $0A
    NullRead,       // $0B
    NullRead,       // $0C
    NullRead,       // $0D
    NullRead,       // $0E
    NullRead,       // $0F
    NullRead,       // $10
    NullRead,       // $11
    NullRead,       // $12
    NullRead,       // $13
    NullRead,       // $14
    NullRead,       // $15
    NullRead,       // $16
    NullRead,       // $17
    NullRead,       // $18
    NullRead,       // $19
    NullRead,       // $1A
    NullRead,       // $1B
    NullRead,       // $1C
    NullRead,       // $1D
    NullRead,       // $1E
    NullRead,       // $1F
    NullRead,       // $20
    NullRead,       // $21
    NullRead,       // $22
    NullRead,       // $23
    NullRead,       // $24
    NullRead,       // $25
    NullRead,       // $26
    NullRead,       // $27
    NullRead,       // $28
    NullRead,       // $29
    NullRead,       // $2A
    NullRead,       // $2B
    NullRead,       // $2C
    NullRead,       // $2D
    NullRead,       // $2E
    NullRead,       // $2F
    NullRead,       // $30
    NullRead,       // $31
    NullRead,       // $32
    NullRead,       // $33
    NullRead,       // $34
    NullRead,       // $35
    NullRead,       // $36
    NullRead,       // $37
    NullRead,       // $38
    NullRead,       // $39
    NullRead,       // $3A
    NullRead,       // $3B
    NullRead,       // $3C
    NullRead,       // $3D
    NullRead,       // $3E
    NullRead,       // $3F
};

iofunction2 iowrite[0x40] = {
    SwitchBank,     // $00
    NullWrite,      // $01
    NullWrite,      // $02
    NullWrite,      // $03
    NullWrite,      // $04
    NullWrite,      // $05
    WriteLCDStartAddr,          // $06
    NullWrite,      // $07
    WritePort0,                 // $08
    WritePort1,                 // $09
    NullWrite,      // $0A
    NullWrite,      // $0B
    WriteTimer01Control,        // $0C
    NullWrite,      // $0D
    NullWrite,      // $0E
    WriteZeroPageBankswitch,    // $0F
    NullWrite,      // $10
    NullWrite,      // $11
    NullWrite,      // $12
    NullWrite,      // $13
    NullWrite,      // $14
    ControlPort1,      // $15
    NullWrite,      // $16
    NullWrite,      // $17
    NullWrite,      // $18
    NullWrite,      // $19
    NullWrite,      // $1A
    NullWrite,      // $1B
    NullWrite,      // $1C
    NullWrite,      // $1D
    NullWrite,      // $1E
    NullWrite,      // $1F
    NullWrite,      // $20
    NullWrite,      // $21
    NullWrite,      // $22
    NullWrite,      // $23
    NullWrite,      // $24
    NullWrite,      // $25
    NullWrite,      // $26
    NullWrite,      // $27
    NullWrite,      // $28
    NullWrite,      // $29
    NullWrite,      // $2A
    NullWrite,      // $2B
    NullWrite,      // $2C
    NullWrite,      // $2D
    NullWrite,      // $2E
    NullWrite,      // $2F
    NullWrite,      // $30
    NullWrite,      // $31
    NullWrite,      // $32
    NullWrite,      // $33
    NullWrite,      // $34
    NullWrite,      // $35
    NullWrite,      // $36
    NullWrite,      // $37
    NullWrite,      // $38
    NullWrite,      // $39
    NullWrite,      // $3A
    NullWrite,      // $3B
    NullWrite,      // $3C
    NullWrite,      // $3D
    NullWrite,      // $3E
    NullWrite,      // $3F
};

regsrec regs;
// LPBYTE  mem          = NULL;
unsigned char fixedram0000[0x10002]; // just like simulator
unsigned char* pmemmap[8]; // 0000~1FFF ... E000~FFFF
unsigned char* may4000addr;
unsigned char* norbankheader[0x10];
unsigned char* volume0array[0x100];
unsigned char* volume1array[0x100];
unsigned char* bbsbankheader[0x10];

TCHAR   ROMfile[MAX_PATH] = TEXT("65c02.rom");
TCHAR   RAMfile[MAX_PATH] = TEXT("");


void MemDestroy () {

}

void MemInitialize () {
    
    memset(&fixedram0000[0], 0, 0x10002);

    qDebug("RESET will jump to 0x0418 in norflash page1.");

    fixedram0000[0xFFFA] = (BYTE)(0x80);  // NMI Vector LowByte        0x0280
    fixedram0000[0xFFFB] = (BYTE)(0x02);  // NMI Vector HighByte
    fixedram0000[0xFFFC] = (BYTE)(0x18);  // Reset Vector LowByte      0x0300
    fixedram0000[0xFFFD] = (BYTE)(0x40);  // Reset Vector HighByte
    fixedram0000[0xFFFE] = (BYTE)(0x00);  // IRQ Vector LowByte        0x0200
    fixedram0000[0xFFFF] = (BYTE)(0x02);  // IRQ Vector HighByte

    MemReset();
}


void MemReset ()
{
    pmemmap[0] = &fixedram0000[0];
    pmemmap[1] = &fixedram0000[0x2000];
    pmemmap[2] = &fixedram0000[0x4000];
    pmemmap[3] = &fixedram0000[0x6000];
    pmemmap[4] = &fixedram0000[0x8000];
    pmemmap[5] = &fixedram0000[0xA000];
    pmemmap[6] = &fixedram0000[0xC000];
    pmemmap[7] = &fixedram0000[0xE000];

    // Initialize the cpu
    CpuInitialize(); // Read pc from reset vector
}

unsigned char GetByte( unsigned short address )
{
    //unsigned int row = address / 0x2000; // SHR
    //return *(pmemmap[row] + address % 0x2000);
    unsigned int row = address >> 0xD; // 0000 0000 0000 0111 0~7
    return *(pmemmap[row] + (address & 0x1FFF)); // 0001 1111 1111 1111
}

unsigned short GetWord( unsigned short address )
{
    unsigned char low = GetByte(address);
    unsigned char high = GetByte(address == 0xFFFF?0:address + 1);
    return ((high << 8) | low);
}

void SetByte( unsigned short address, unsigned char value )
{
    unsigned int row = address / 0x2000; // SHR
    *(pmemmap[row] + address % 0x2000) = value;
}

BYTE __stdcall NullRead (BYTE address) {
    //qDebug("lee wanna read io, [%04x] -> %02x", address, mem[address]);
    return fixedram0000[address];
}

void __stdcall NullWrite(BYTE address, BYTE value) {
    //qDebug("lee wanna write io, [%04x] (%02x) -> %02x", address, mem[address], value);
    fixedram0000[address] = value;
}

void __stdcall SwitchBank( BYTE write, BYTE value )
{
    fixedram0000[write] = value;
    qDebug("lee wanna switch to bank 0x%02x", value);
    theNekoDriver->SwitchNorBank(value);
}


BYTE __stdcall ReadBank( BYTE read )
{
    byte r = fixedram0000[read];
    qDebug("lee wanna read bank. current bank 0x%02x", r);
    return r;
}

bool timer0started = false;
bool timer1started = false;

BYTE __stdcall StartTimer0( BYTE read )
{
    qDebug("lee wanna start timer0");
    timer0started = true;
    return fixedram0000[read];
}

BYTE __stdcall StopTimer0( BYTE read )
{
    qDebug("lee wanna stop timer0");
    byte r = fixedram0000[read];
    fixedram0000[read] = 0;
    timer0started = false;
    return r;
}

BYTE __stdcall StartTimer1( BYTE read )
{
    qDebug("lee wanna start timer1");
    timer1started = true;
    return fixedram0000[read];
}

BYTE __stdcall StopTimer1( BYTE read )
{
    qDebug("lee wanna stop timer1");
    byte r = fixedram0000[read];
    fixedram0000[read] = 0;
    timer1started = false;
    return r;
}

unsigned short lcdbuffaddr;

void __stdcall WriteLCDStartAddr( BYTE write, BYTE value )
{
    unsigned int t = ((fixedram0000[0x0C] & 0x3) << 12);
    t = t | (value << 4);
    qDebug("lee wanna change lcdbuf address to 0x%04x", t);
    fixedram0000[write] = value;
    lcdbuffaddr = t;
}

void __stdcall WriteTimer01Control( BYTE write, BYTE value )
{
    unsigned int t = ((value & 0x3) << 12);
    t = t | (fixedram0000[0x06] << 4);
    qDebug("lee wanna change lcdbuf address to 0x%04x", t);
    qDebug("lee also wanna change timer settins to 0x%02x.", (value & 0xC));
    fixedram0000[write] = value;
    lcdbuffaddr = t;
}

void __stdcall WriteROABBS( BYTE write, BYTE value )
{

}

unsigned char zp40cache[0x40];

unsigned char* GetZeroPagePointer(unsigned char bank) {
    //.text:0040BFD0 bank            = byte ptr  4
    //.text:0040BFD0
    //.text:0040BFD0                 mov     al, [esp+bank]
    //.text:0040BFD4                 cmp     al, 4
    //.text:0040BFD6                 jnb     short loc_40BFE5 ; if (bank < 4) {
    //.text:0040BFD8                 xor     eax, eax        ; bank == 0,1,2,3
    //.text:0040BFD8                                         ; set bank = 0
    //.text:0040BFDA                 and     eax, 0FFFFh     ; WORD(bank)
    //.text:0040BFDF                 add     eax, offset gFixedRAM0 ; result = &gFixedRAM0[WORD(bank)];
    //.text:0040BFE4                 retn                    ; }
    //.text:0040BFE5 ; ---------------------------------------------------------------------------
    //.text:0040BFE5
    //.text:0040BFE5 loc_40BFE5:                             ; CODE XREF: GetZeroPagePointer+6j
    //.text:0040BFE5                 movzx   ax, al          ; 4,5,6,7
    //.text:0040BFE9                 add     eax, 4          ; bank+=4
    //.text:0040BFEC                 shl     eax, 6          ; bank *= 40;
    //.text:0040BFEF                 and     eax, 0FFFFh     ; WORD(bank)
    //.text:0040BFF4                 add     eax, offset gFixedRAM0
    //.text:0040BFF9                 retn
    unsigned char* result;

    if (bank >= 4) {
        // 4,5,6,7
        // 4 -> 200 5-> 240
        result = &fixedram0000[(bank + 4) << 6];
    } else {
        // 0,1,2,3
        result = &fixedram0000[0];
    }
    return result;
}

void __stdcall WriteZeroPageBankswitch (BYTE write, BYTE value)
{
    //char *oldzpbank; // eax@1
    //unsigned __int8 newzpbank; // dl@1
    //char issamebank; // zf@1
    //signed int k; // ecx@3
    //char *pfixedram040_ya2; // esi@3
    //char *zpptr; // eax@6
    //char *pfixedram040; // ecx@6
    //unsigned int i; // esi@6
    //char *pfixedram040_ya; // ecx@11
    //signed int j; // esi@11

    //LOBYTE(oldzpbank) = mayPrevDestAddrValue & 7; // zpbank is only 3bit
    //newzpbank = tmpAXYValue & 7;
    //issamebank = (mayPrevDestAddrValue & 7) == (tmpAXYValue & 7);
    //mayPrevDestAddrValue &= 7u;                   // remove high P0x Direction
    //tmpAXYValue &= 7u;                            // remove high P0x Direction
    //if ( !issamebank )
    //{
    //    if ( (_BYTE)oldzpbank )
    //    {
    //        // oldzpbank != 0
    //        zpptr = GetZeroPagePointer((unsigned __int8)oldzpbank);
    //        pfixedram040 = gFixedRAM0_b40;
    //        i = 0x40u;
    //        do
    //        {
    //            // copy from fixed 40 to dest
    //            *zpptr++ = *pfixedram040++;
    //            --i;
    //        }
    //        while ( i );                              // for 0 to 40
    //        if ( tmpAXYValue )
    //            // newzpbank != 0
    //            oldzpbank = GetZeroPagePointer(tmpAXYValue);
    //        else
    //            // newzpbank == 0
    //            oldzpbank = (char *)&gzp40Cache;
    //        pfixedram040_ya = gFixedRAM0_b40;
    //        j = 0x40u;
    //        do
    //        {
    //            // copy from cache or other bank to fixed 40
    //            *pfixedram040_ya++ = *oldzpbank++;
    //            --j;
    //        }
    //        while ( j );                              // for 0 to 40
    //    }
    //    else
    //    {
    //        // oldzpbank == 0
    //        memcpy(&gzp40Cache, gFixedRAM0_b40, 0x40u);// backup fixed 40~80 to cache
    //        pfixedram040_ya2 = gFixedRAM0_b40;
    //        oldzpbank = GetZeroPagePointer(newzpbank);
    //        k = 0x40u;
    //        do
    //        {
    //            // copy from other bank to fixed 40
    //            *pfixedram040_ya2++ = *oldzpbank++;
    //            --k;
    //        }
    //        while ( k );
    //    }
    //}
    //return (char)oldzpbank;
    unsigned char oldzpbank = fixedram0000[0x0F] & 7;
    unsigned char newzpbank = value & 7;
    if (oldzpbank != newzpbank) {
        if (oldzpbank != 0) {
            // dangerous if exchange 00 <-> 40
            // oldaddr maybe 0 or 200~300
            unsigned char* oldzpptr = GetZeroPagePointer(oldzpbank);
            memcpy(oldzpptr, &fixedram0000[0x40], 0x40);
            if (newzpbank != 0) {
                unsigned char* newzpptr = GetZeroPagePointer(newzpbank);
                memcpy(&fixedram0000[0x40], newzpptr, 0x40);
            } else {
                memcpy(&fixedram0000[0x40], &zp40cache[0], 0x40); // copy backup to 40
            }
        } else {
            // oldzpbank == 0
            memcpy(&zp40cache[0], &fixedram0000[0x40], 0x40); // backup fixed 40~80 to cache
            unsigned char* newzpptr = GetZeroPagePointer(newzpbank);
            memcpy(&fixedram0000[0x40], newzpptr, 0x40); // copy newbank to 40
        }
    }
}

bool TNekoDriver::SwitchNorBank( int bank )
{
    //memcpy(&fixedram0000[0x4000], &fNorBuffer[bank * 0x8000], 0x8000);
    pmemmap[2] = (unsigned char*)&fNorBuffer[bank * 0x8000]; // 4000
    pmemmap[3] = (unsigned char*)&fNorBuffer[bank * 0x8000 + 0x2000]; // 6000
    pmemmap[4] = (unsigned char*)&fNorBuffer[bank * 0x8000 + 0x4000]; // 8000
    pmemmap[5] = (unsigned char*)&fNorBuffer[bank * 0x8000 + 0x6000]; // A000
    return true;
}

void TNekoDriver::SwitchBank( int bank )
{
    //char result; // al@1
    //int rambank; // eax@6

    //result = tmpAXYValue;                         // tmpAXYValue = new value wanna write into [00]
    //if ( mayPrevDestAddrValue != tmpAXYValue )
    //{
    //    // old[00] != new value
    //    if ( gFixedRAM0[(unsigned __int8)io0A_bios_bsw_roa] & 0x80 )
    //    {
    //        // ROA == 1
    //        // RAM (norflash?!)
    //        rambank = tmpAXYValue & 0xF;              // RAM only have 0~F page
    //        if ( rambank < 0 )
    //            rambank = ((rambank - 1) | 0xFFFFFFF0) + 1;
    //        tmpAXYValue = rambank;
    //        may4000ptr = (DWORD)(&g_pNorBankHeader)[(unsigned __int8)rambank];
    //        result = Switch4000_BFFF(rambank);
    //    }
    //    else
    //    {
    //        // ROA == 0
    //        // BROM
    //        if ( gFixedRAM0[(unsigned __int8)io0D_lcd_segment_volumeID] & 1 )
    //        {
    //            // VolumeID == 1
    //            may4000ptr = gVolume1Array[tmpAXYValue];
    //            result = Switch4000_BFFF(tmpAXYValue);
    //        }
    //        else
    //        {
    //            // VolumeID == 0
    //            may4000ptr = gVolume0Array[tmpAXYValue];
    //            result = Switch4000_BFFF(tmpAXYValue);
    //        }
    //    }
    //}
    //return result;
    if (fixedram0000[0x0A] & 0x80) {
        // ROA == 1
        // RAM (norflash?!)
        char rambank = bank & 0xF; // nor only have 0~F page
        may4000addr = norbankheader[rambank];
        Switch4000toBFFF(rambank);
    } else {
        // ROA == 0
        // BROM
        if (fixedram0000[0x0D] & 1) {
            // VolumeID == 1, 3
            may4000addr = volume1array[bank];
            Switch4000toBFFF(bank);
        } else {
            // VolumeID == 0, 2
            may4000addr = volume0array[bank];
            Switch4000toBFFF(bank);
        }
    }
}

void TNekoDriver::Switch4000toBFFF( unsigned char bank )
{
    //void *addr6000; // ecx@4
    //DWORD result; // eax@7

    //if ( bank || gFixedRAM0[(unsigned __int8)io0A_bios_bsw_roa] & 0x80 )
    //{
    //    // bank != 0 || ROA == RAM
    //    g__pRAM4000 = (void *)may4000ptr;
    //    addr6000 = (void *)(may4000ptr + 0x2000);
    //}
    //else
    //{
    //    // bank == 0 && ROA == ROM
    //    if ( gFixedRAM0[(unsigned __int8)io0D_lcd_segment_volumeID] & 1 )
    //    {
    //        // Volume1
    //        g__pRAM4000 = g_pNorBankHeader;
    //        addr6000 = g_pNorBankHeader + 2048;       // 43BF28 + 2000 = 43DF28 (800*4)
    //    }
    //    else
    //    {
    //        // Volume0
    //        // 6000~7FFF is mirror of 4000~5FFF
    //        addr6000 = &gFixedRAM4000;
    //        g__pRAM4000 = &gFixedRAM4000;
    //    }
    //}
    //result = may4000ptr + 0x6000;
    //g__pRAM6000 = addr6000;
    //g__pRAM8000 = may4000ptr + 0x4000;
    //g__pRAMA000 = may4000ptr + 0x6000;

    if (bank != 0 || fixedram0000[0x0A] & 0x80) {
        // bank != 0 || ROA == RAM
        pmemmap[2] = may4000addr;
        pmemmap[3] = may4000addr + 0x2000;
    } else {
        // bank == 0 && ROA == ROM
        if (fixedram0000[0x0D] & 0x1) {
            // Volume1,3
            // 4000~7FFF is 0 page of Nor.
            // 8000~BFFF is relative to may4000ptr
            pmemmap[2] = norbankheader[0];
            pmemmap[3] = norbankheader[0] + 0x2000;
        } else {
            // Volume0,2
            // 4000~5FFF is RAM
            // 6000~7FFF is mirror of 4000~5FFF
            pmemmap[2] = &fixedram0000[0x4000];
            pmemmap[3] = &fixedram0000[0x4000];
        }
    }
    pmemmap[4] = may4000addr + 0x4000;
    pmemmap[5] = may4000addr + 0x6000;
}

bool TNekoDriver::LoadDemoNor(const QString& filename)
{
    QFile mariofile(filename);
    mariofile.open(QFile::ReadOnly);
    if (fNorBuffer == NULL){
        fNorBuffer = (char*)malloc(16 * 0x8000); // 32K * 16
        for (int i = 0; i < 16; i++) {
            norbankheader[i] = (unsigned char*)fNorBuffer + i * 0x8000;
        }
    }
    int page = 1;
    while (mariofile.atEnd() == false) {
        mariofile.read(fNorBuffer + 0x8000 * page + 0x4000, 0x4000);
        mariofile.read(fNorBuffer + 0x8000 * page, 0x4000);
        page++;
    }

    mariofile.close();
    return true;
}

bool TNekoDriver::LoadBROM( const QString& filename )
{
    QFile romfile(filename);
    romfile.open(QFile::ReadOnly);
    if (fBROMBuffer == NULL){
        fBROMBuffer = (char*)malloc(512 * 0x8000); // 32K * 256 * 2
        for (int i = 0; i < 256; i++) {
            volume0array[i] = (unsigned char*)fBROMBuffer + i * 0x8000;
            volume1array[i] = volume0array[i] + 256 * 0x8000;
        }
    }
    int page = 0;
    while (romfile.atEnd() == false) {
        romfile.read(fBROMBuffer + 0x8000 * page + 0x4000, 0x4000);
        romfile.read(fBROMBuffer + 0x8000 * page, 0x4000);
        page++;
    }
    return true;
}

bool TNekoDriver::LoadFullNorFlash( const QString& filename )
{
    QFile norfile(filename);
    norfile.open(QFile::ReadOnly);
    if (fNorBuffer == NULL){
        fNorBuffer = (char*)malloc(16 * 0x8000); // 32K * 16
        for (int i = 0; i < 16; i++) {
            norbankheader[i] = (unsigned char*)fNorBuffer + i * 0x8000;
        }
    }
    int page = 0;
    while (norfile.atEnd() == false) {
        norfile.read(fNorBuffer + 0x8000 * page + 0x4000, 0x4000);
        norfile.read(fNorBuffer + 0x8000 * page, 0x4000);
        page++;
    }
    return true;
}
