#include "NekoDriver.h"
extern "C" {
#include "ANSI/65c02.h"
}
#include "CC800IOName.h"


bool timer0started = false;
bool timer1started = false;

BYTE __stdcall StartTimer0( BYTE ) // 05
{
    qDebug("lee wanna start timer0");
    timer0started = true;
    return fixedram0000[io02_timer0_val];
}

BYTE __stdcall StopTimer0( BYTE ) // 04
{
    qDebug("lee wanna stop timer0");
    byte r = fixedram0000[io02_timer0_val];
    fixedram0000[io02_timer0_val] = 0;
    timer0started = false;
    return r;
}

BYTE __stdcall StartTimer1( BYTE ) // 07
{
    qDebug("lee wanna start timer1");
    timer1started = true;
    return fixedram0000[io03_timer1_val];
}

BYTE __stdcall StopTimer1( BYTE ) // 06
{
    // Stop timer1, and return time1 value
    qDebug("lee wanna stop timer1");
    byte r = fixedram0000[io03_timer1_val];
    fixedram0000[io03_timer1_val] = 0;
    timer1started = false;
    return r;
}

unsigned short lcdbuffaddr;

void __stdcall WriteLCDStartAddr( BYTE write, BYTE value ) // 06
{
    unsigned int t = ((fixedram0000[io0C_lcd_config] & 0x3) << 12);
    t = t | (value << 4);
    qDebug("lee wanna change lcdbuf address to 0x%04x", t);
    fixedram0000[io06_lcd_config] = value;
    lcdbuffaddr = t;
    (void)write;
}

void __stdcall WriteTimer01Control( BYTE write, BYTE value ) // 0C
{
    unsigned int t = ((value & 0x3) << 12); // lc12~lc13
    t = t | (fixedram0000[io06_lcd_config] << 4); // lc4~lc11
    qDebug("lee wanna change lcdbuf address to 0x%04x", t);
    qDebug("lee also wanna change timer settings to 0x%02x.", (value & 0xC));
    fixedram0000[io0C_lcd_config] = value;
    lcdbuffaddr = t;
    (void)write;
}

//////////////////////////////////////////////////////////////////////////
// Keypad registers
//////////////////////////////////////////////////////////////////////////
unsigned char keypadmatrix[8][8] = {0,};

void UpdateKeypadRegisters()
{
    // TODO: 2pass check
    //qDebug("old [0015]:%02x [0009]:%02x [0008]:%02x", mem[0x15], mem[0x9], mem[0x8]);
    //int up = 0, down = 0;
    byte port1control = fixedram0000[io15_port1_dir];
    byte port0control = fixedram0000[io0F_port0_dir] & 0xF0; // b4~b7
    byte port1controlbit = 1; // aka, y control bit
    byte tmpdest0 = 0, tmpdest1 = 0;
    byte port1data = fixedram0000[io09_port1_data], port0data = fixedram0000[io08_port0_data];
    for (int y = 0; y < 8; y++) {
        // y = Port10~Port17
        bool ysend = ((port1control & port1controlbit) != 0);
        byte xbit = 1;
        for (int x = 0; x < 8; x++) {
            // x = Port00~Port07
            byte port0controlbit;
            if (x < 2) {
                // 0, 1 = b4 b5
                port0controlbit = xbit << 4;
            } else if (x < 4) {
                // 2, 3 = b6
                port0controlbit = 0x40;
            } else {
                // 4, 5, 6, 7 = b7
                port0controlbit = 0x80;
            }
            if (keypadmatrix[y][x] != 2) {
                if (ysend) {
                    // port1y-> port0x
                    // port1y is send but only set bit to high when port0 xbit is receive too
                    if ((keypadmatrix[y][x]) && ((port1data & port1controlbit) != 0) && ((port0control & port0controlbit) == 0)) {
                        tmpdest0 |= xbit;
                    }
                } else {
                    // port0x -> port1y
                    // port1y should be receive, only set bit to high when port0 xbit is send
                    if ((keypadmatrix[y][x]) && ((port0data & xbit) != 0) && ((port0control & port0controlbit) != 0)) {
                        tmpdest1 |= xbit;
                    }
                }
            }
            xbit = xbit << 1;
        }
        port1controlbit = port1controlbit << 1;
    }
    if (port1control != 0xFF) {
        // port1 should clean some bits
        // using port1control as port1mask
        // sometimes port10,11 should clean here 
        port1data &= port1control; // pre set receive bits to 0
    }
    if (port1control != 0xF0) {
        // clean port0
        // calculate port0 mask
        // in most case port0 will be set to 0
        byte port0mask = (port0control >> 4) & 0x3; // bit4->0 bit5->1
        if (port0control & 0x40) {
            // bit6->2,3
            port0mask |= 0x0C; // 00001100
        }
        if (port0control & 0x80) {
            // bit7->4,5,6,7
            port0mask |= 0xF0; // 11110000
        }
        port0data &= port0mask;
    }
    port0data |= tmpdest0;
    port1data |= tmpdest1;
    if (fixedram0000[io09_port1_data] != port1data || fixedram0000[io08_port0_data] != port0data) {
        qDebug("old [0015]:%02x [0009]:%02x [0008]:%02x", fixedram0000[io15_port1_dir], fixedram0000[io09_port1_data], fixedram0000[io08_port0_data]);
        qDebug("new [0015]:%02x [0009]:%02x [0008]:%02x", fixedram0000[io15_port1_dir], port1data, port0data);
    }
    fixedram0000[io09_port1_data] = port1data;
    fixedram0000[io08_port0_data] = port0data;
}

BYTE __stdcall ReadPort0( BYTE read )
{
    UpdateKeypadRegisters();
    //qDebug("lee wanna read keypad port0, [%04x] -> %02x", read, mem[read]);
    return fixedram0000[io08_port0_data];
    (void)read;
}

BYTE __stdcall ReadPort1( BYTE read )
{
    UpdateKeypadRegisters();
    //qDebug("lee wanna read keypad port1, [%04x] -> %02x", read, mem[read]);
    return fixedram0000[io09_port1_data];
    (void)read;
}

void __stdcall WritePort0( BYTE write, BYTE value )
{
    //qDebug("lee wanna write keypad port0, [%04x] (%02x) -> %02x", write, mem[write], value);
    fixedram0000[io08_port0_data] = value;
    UpdateKeypadRegisters();
    (void)write;
}

void __stdcall WritePort1( BYTE write, BYTE value )
{
    //qDebug("lee wanna write keypad port1, [%04x] (%02x) -> %02x", write, mem[write], value);
    fixedram0000[io09_port1_data] = value;
    UpdateKeypadRegisters();
    (void)write;
}

void __stdcall ControlPort1( BYTE write, BYTE value )
{
    //qDebug("lee wanna config keypad port1, [%04x] (%02x) -> %02x", write, mem[write], value);
    fixedram0000[io15_port1_dir] = value;
    UpdateKeypadRegisters();
    (void)write;
}
