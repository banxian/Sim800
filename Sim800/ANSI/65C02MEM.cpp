extern "C" {
#include "65c02.h"
}

BYTE __stdcall NullIo (BYTE write, BYTE value);

iofunction ioread[0x100]  = {KeybReadData,       // $00
                             KeybReadFlag,       // $01
                             NullIo,             // $02
                             NullIo,             // $03
                             NullIo,             // $04
                             NullIo,             // $05
                             NullIo,             // $06
                             NullIo,             // $07
                             NullIo,             // $08
                             NullIo,             // $09
                             NullIo,             // $0A
                             NullIo,             // $0B
                             NullIo,             // $0C
                             NullIo,             // $0D
                             NullIo,             // $0E
                             NullIo,             // $0F
                             TerminalOutputRD,	 // $10
                             NullIo,             // $11
                             NullIo,             // $12
                             NullIo,             // $13
                             NullIo,             // $14
                             NullIo,             // $15
                             NullIo,             // $16
                             NullIo,             // $17
                             NullIo,             // $18
                             NullIo,             // $19
                             NullIo,             // $1A
                             NullIo,             // $1B
                             NullIo,             // $1C
                             NullIo,             // $1D
                             NullIo,             // $1E
                             NullIo,             // $1F
                             SpkrToggle,		 // $20
                             NullIo,             // $21
                             NullIo,             // $22
                             NullIo,             // $23
                             NullIo,             // $24
                             NullIo,             // $25
                             NullIo,             // $26
                             NullIo,             // $27
                             NullIo,             // $28
                             NullIo,             // $29
                             NullIo,             // $2A
                             NullIo,             // $2B
                             NullIo,             // $2C
                             NullIo,             // $2D
                             NullIo,             // $2E
                             NullIo,             // $2F
                             CommReceive,        // $30
                             CommStatus,         // $31
                             CommCommandRD,      // $32
                             CommControlRD,      // $33
                             NullIo,             // $34
                             NullIo,             // $35
                             NullIo,             // $36
                             NullIo,             // $37
                             NullIo,             // $38
                             NullIo,             // $39
                             NullIo,             // $3A
                             NullIo,             // $3B
                             NullIo,             // $3C
                             NullIo,             // $3D
                             NullIo,             // $3E
                             NullIo,             // $3F
                             LTPDataPort,        // $40
                             LTPStatusPort,      // $41
                             LTPCommandPort,     // $42
                             NullIo,             // $43
                             NullIo,             // $44
                             NullIo,             // $45
                             NullIo,             // $46
                             NullIo,             // $47
                             NullIo,             // $48
                             NullIo,             // $49
                             NullIo,             // $4A
                             NullIo,             // $4B
                             NullIo,             // $4C
                             NullIo,             // $4D
                             NullIo,             // $4E
                             NullIo,             // $4F
                             NullIo,			 // $50
                             NullIo,             // $51
                             NullIo,             // $52
                             NullIo,             // $53
                             NullIo,             // $54
                             NullIo,             // $55
                             NullIo,             // $56
                             NullIo,             // $57
                             NullIo,             // $58
                             NullIo,             // $59
                             NullIo,             // $5A
                             NullIo,             // $5B
                             NullIo,             // $5C
                             NullIo,             // $5D
                             NullIo,             // $5E
                             NullIo,             // $5F
                             NullIo,             // $60
                             NullIo,             // $61
                             NullIo,             // $62
                             NullIo,             // $63
                             NullIo,             // $64
                             NullIo,             // $65
                             NullIo,             // $66
                             NullIo,             // $67
                             NullIo,             // $68
                             NullIo,             // $69
                             NullIo,             // $6A
                             NullIo,             // $6B
                             NullIo,             // $6C
                             NullIo,             // $6D
                             NullIo,             // $6E
                             NullIo,             // $6F
                             NullIo,             // $70
                             NullIo,             // $71
                             NullIo,             // $72
                             NullIo,             // $73
                             NullIo,             // $74
                             NullIo,             // $75
                             NullIo,             // $76
                             NullIo,             // $77
                             NullIo,             // $78
                             NullIo,             // $79
                             NullIo,             // $7A
                             NullIo,             // $7B
                             NullIo,             // $7C
                             NullIo,             // $7D
                             NullIo,             // $7E
                             NullIo,             // $7F
							 NullIo,             // $80
                             NullIo,             // $81
                             NullIo,             // $82
                             NullIo,             // $83
                             NullIo,             // $84
                             NullIo,             // $85
                             NullIo,             // $86
                             NullIo,             // $87
                             NullIo,             // $88
                             NullIo,             // $89
                             NullIo,             // $8A
                             NullIo,             // $8B
                             NullIo,             // $8C
                             NullIo,             // $8D
                             NullIo,             // $8E
                             NullIo,             // $8F
                             NullIo,             // $90
                             NullIo,             // $91
                             NullIo,             // $92
                             NullIo,             // $93
                             NullIo,             // $94
                             NullIo,             // $95
                             NullIo,             // $96
                             NullIo,             // $97
                             NullIo,             // $98
                             NullIo,             // $99
                             NullIo,             // $9A
                             NullIo,             // $9B
                             NullIo,             // $9C
                             NullIo,             // $9D
                             NullIo,             // $9E
                             NullIo,             // $9F
                             NullIo,             // $A0
                             NullIo,             // $A1
                             NullIo,             // $A2
                             NullIo,             // $A3
                             NullIo,             // $A4
                             NullIo,             // $A5
                             NullIo,             // $A6
                             NullIo,             // $A7
							 NullIo,             // $A8
                             NullIo,             // $A9
                             NullIo,             // $AA
                             NullIo,             // $AB
                             NullIo,             // $AC
                             NullIo,             // $AD
                             NullIo,             // $AE
                             NullIo,             // $AF
                             NullIo,             // $B0
                             NullIo,             // $B1
                             NullIo,             // $B2
                             NullIo,             // $B3
                             NullIo,             // $B4
                             NullIo,             // $B5
                             NullIo,             // $B6
                             NullIo,             // $B7
                             NullIo,             // $B8
                             NullIo,             // $B9
                             NullIo,             // $BA
                             NullIo,             // $BB
                             NullIo,             // $BC
                             NullIo,             // $BD
                             NullIo,             // $BE
                             NullIo,             // $BF
                             NullIo,             // $C0
                             NullIo,             // $C1
                             NullIo,             // $C2
                             NullIo,             // $C3
                             NullIo,             // $C4
                             NullIo,             // $C5
                             NullIo,             // $C6
                             NullIo,             // $C7
                             NullIo,             // $C8
                             NullIo,             // $C9
                             NullIo,             // $CA
                             NullIo,             // $CB
                             NullIo,             // $CC
                             NullIo,             // $CD
                             NullIo,             // $CE
                             NullIo,             // $CF
                             NullIo,             // $D0
                             NullIo,             // $D1
                             NullIo,             // $D2
                             NullIo,             // $D3
                             NullIo,             // $D4
                             NullIo,             // $D5
                             NullIo,             // $D6
                             NullIo,             // $D7
                             NullIo,             // $D8
                             NullIo,             // $D9
                             NullIo,             // $DA
                             NullIo,             // $DB
                             NullIo,             // $DC
                             NullIo,             // $DD
                             NullIo,             // $DE
                             NullIo,             // $DF
                             NullIo,             // $E0
                             NullIo,             // $E1
                             NullIo,             // $E2
                             NullIo,             // $E3
                             NullIo,             // $E4
                             NullIo,             // $E5
                             NullIo,             // $E6
                             NullIo,             // $E7
                             NullIo,             // $E8
                             NullIo,             // $E9
                             NullIo,             // $EA
                             NullIo,             // $EB
                             NullIo,             // $EC
                             NullIo,             // $ED
                             NullIo,             // $EE
                             NullIo,             // $EF
                             NullIo,             // $F0
                             NullIo,             // $F1
                             NullIo,             // $F2
                             NullIo,             // $F3
                             NullIo,             // $F4
                             NullIo,             // $F5
                             NullIo,             // $F6
                             NullIo,             // $F7
                             NullIo,             // $F8
                             NullIo,             // $F9
                             NullIo,             // $FA
                             NullIo,             // $FB
                             NullIo,             // $FC
                             NullIo,             // $FD
                             NullIo,             // $FE
                             NullIo};            // $FF

iofunction iowrite[0x100] = {NullIo,             // $00
                             KeybReadFlag,       // $01
                             NullIo,             // $02
                             NullIo,             // $03
                             NullIo,             // $04
                             NullIo,             // $05
                             NullIo,             // $06
                             NullIo,             // $07
                             NullIo,             // $08
                             NullIo,             // $09
                             NullIo,             // $0A
                             NullIo,             // $0B
                             NullIo,             // $0C
                             NullIo,             // $0D
                             NullIo,             // $0E
                             NullIo,             // $0F
                             TerminalOutputWR,   // $10
                             NullIo,             // $11
                             NullIo,             // $12
                             NullIo,             // $13
                             NullIo,             // $14
                             NullIo,             // $15
                             NullIo,             // $16
                             NullIo,             // $17
                             NullIo,             // $18
                             NullIo,             // $19
                             NullIo,             // $1A
                             NullIo,             // $1B
                             NullIo,             // $1C
                             NullIo,             // $1D
                             NullIo,             // $1E
                             NullIo,             // $1F
                             SpkrToggle,         // $20
                             NullIo,             // $21
                             NullIo,             // $22
                             NullIo,             // $23
                             NullIo,             // $24
                             NullIo,             // $25
                             NullIo,             // $26
                             NullIo,             // $27
                             NullIo,             // $28
                             NullIo,             // $29
                             NullIo,             // $2A
                             NullIo,             // $2B
                             NullIo,             // $2C
                             NullIo,             // $2D
                             NullIo,             // $2E
                             NullIo,             // $2F
                             CommTransmit,       // $30
                             CommStatus,         // $31
                             CommCommandWR,      // $32
                             CommControlWR,      // $33
                             NullIo,             // $34
                             NullIo,             // $35
                             NullIo,             // $36
                             NullIo,             // $37
                             NullIo,             // $38
                             NullIo,             // $39
                             NullIo,             // $3A
                             NullIo,             // $3B
                             NullIo,             // $3C
                             NullIo,             // $3D
                             NullIo,             // $3E
                             NullIo,             // $3F
                             LTPDataPort,        // $40   LTP1:
                             LTPStatusPort,      // $41
                             LTPCommandPort,     // $42
                             NullIo,             // $43
                             NullIo,             // $44
                             NullIo,             // $45
                             NullIo,             // $46
                             NullIo,             // $47
                             NullIo,             // $48
                             NullIo,             // $49
                             NullIo,             // $4A
                             NullIo,             // $4B
                             NullIo,             // $4C
                             NullIo,             // $4D
                             NullIo,             // $4E
                             NullIo,             // $4F
                             NullIo,             // $50
                             NullIo,             // $51
                             NullIo,             // $52
                             NullIo,             // $53
                             NullIo,             // $54
                             NullIo,             // $55
                             NullIo,             // $56
                             NullIo,             // $57
                             NullIo,             // $58
                             NullIo,             // $59
                             NullIo,             // $5A
                             NullIo,             // $5B
                             NullIo,             // $5C
                             NullIo,             // $5D
                             NullIo,             // $5E
                             NullIo,             // $5F
                             NullIo,             // $60
                             NullIo,             // $61
                             NullIo,             // $62
                             NullIo,             // $63
                             NullIo,             // $64
                             NullIo,             // $65
                             NullIo,             // $66
                             NullIo,             // $67
                             NullIo,             // $68
                             NullIo,             // $69
                             NullIo,             // $6A
                             NullIo,             // $6B
                             NullIo,             // $6C
                             NullIo,             // $6D
                             NullIo,             // $6E
                             NullIo,             // $6F
                             NullIo,             // $70
                             NullIo,             // $71
                             NullIo,             // $72
                             NullIo,             // $73
                             NullIo,             // $74
                             NullIo,             // $75
                             NullIo,             // $76
                             NullIo,             // $77
                             NullIo,             // $78
                             NullIo,             // $79
                             NullIo,             // $7A
                             NullIo,             // $7B
                             NullIo,             // $7C
                             NullIo,             // $7D
                             NullIo,             // $7E
                             NullIo,             // $7F
                             NullIo,             // $80
                             NullIo,             // $81
                             NullIo,             // $82
                             NullIo,             // $83
                             NullIo,             // $84
                             NullIo,             // $85
                             NullIo,             // $86
                             NullIo,             // $87
                             NullIo,             // $88
                             NullIo,             // $89
                             NullIo,             // $8A
                             NullIo,             // $8B
                             NullIo,             // $8C
                             NullIo,             // $8D
                             NullIo,             // $8E
                             NullIo,             // $8F
                             NullIo,             // $90
                             NullIo,             // $91
                             NullIo,             // $92
                             NullIo,             // $93
                             NullIo,             // $94
                             NullIo,             // $95
                             NullIo,             // $96
                             NullIo,             // $97
                             NullIo,             // $98
                             NullIo,             // $99
                             NullIo,             // $9A
                             NullIo,             // $9B
                             NullIo,             // $9C
                             NullIo,             // $9D
                             NullIo,             // $9E
                             NullIo,             // $9F
                             NullIo,             // $A0
                             NullIo,             // $A1
                             NullIo,             // $A2
                             NullIo,             // $A3
                             NullIo,             // $A4
                             NullIo,             // $A5
                             NullIo,             // $A6
                             NullIo,             // $A7
                             NullIo,             // $A8
                             NullIo,             // $A9
                             NullIo,             // $AA
                             NullIo,             // $AB
                             NullIo,             // $AC
                             NullIo,             // $AD
                             NullIo,             // $AE
                             NullIo,             // $AF
                             NullIo,             // $B0
                             NullIo,             // $B1
                             NullIo,             // $B2
                             NullIo,             // $B3
                             NullIo,             // $B4
                             NullIo,             // $B5
                             NullIo,             // $B6
                             NullIo,             // $B7
                             NullIo,             // $B8
                             NullIo,             // $B9
                             NullIo,             // $BA
                             NullIo,             // $BB
                             NullIo,             // $BC
                             NullIo,             // $BD
                             NullIo,             // $BE
                             NullIo,             // $BF
                             NullIo,             // $C0
                             NullIo,             // $C1
                             NullIo,             // $C2
                             NullIo,             // $C3
                             NullIo,             // $C4
                             NullIo,             // $C5
                             NullIo,             // $C6
                             NullIo,             // $C7
                             NullIo,             // $C8
                             NullIo,             // $C9
                             NullIo,             // $CA
                             NullIo,             // $CB
                             NullIo,             // $CC
                             NullIo,             // $CD
                             NullIo,             // $CE
                             NullIo,             // $CF
                             NullIo,             // $D0
                             NullIo,             // $D1
                             NullIo,             // $D2
                             NullIo,             // $D3
                             NullIo,             // $D4
                             NullIo,             // $D5
                             NullIo,             // $D6
                             NullIo,             // $D7
                             NullIo,             // $D8
                             NullIo,             // $D9
                             NullIo,             // $DA
                             NullIo,             // $DB
                             NullIo,             // $DC
                             NullIo,             // $DD
                             NullIo,             // $DE
                             NullIo,             // $DF
                             NullIo,             // $E0
                             NullIo,             // $E1
                             NullIo,             // $E2
                             NullIo,             // $E3
                             NullIo,             // $E4
                             NullIo,             // $E5
                             NullIo,             // $E6
                             NullIo,             // $E7
                             NullIo,             // $E8
                             NullIo,             // $E9
                             NullIo,             // $EA
                             NullIo,             // $EB
                             NullIo,             // $EC
                             NullIo,             // $ED
                             NullIo,             // $EE
                             NullIo,             // $EF
                             NullIo,             // $F0
                             NullIo,             // $F1
                             NullIo,             // $F2
                             NullIo,             // $F3
                             NullIo,             // $F4
                             NullIo,             // $F5
                             NullIo,             // $F6
                             NullIo,             // $F7
                             NullIo,             // $F8
                             NullIo,             // $F9
                             NullIo,             // $FA
                             NullIo,             // $FB
                             NullIo,             // $FC
                             NullIo,             // $FD
                             NullIo,             // $FE
                             NullIo};            // $FF

regsrec regs;
LPBYTE  mem          = NULL;
TCHAR   ROMfile[MAX_PATH] = TEXT("65c02.rom");
TCHAR   RAMfile[MAX_PATH] = TEXT("");



BYTE __stdcall NullIo (BYTE address, BYTE null1) {
	return address;
}

void MemDestroy () {
  VirtualFree(mem,0,MEM_RELEASE);
  mem      = NULL;
}

void MemInitialize () {

  mem = (LPBYTE)VirtualAlloc(NULL,0x10001,MEM_COMMIT,PAGE_READWRITE);

  if (!mem)	{
    MessageBox(GetDesktopWindow(),
               TEXT("The Simulator was unable to allocate the memory it ")
               TEXT("requires.  Further execution is not possible."),
               TITLE,
               MB_ICONSTOP | MB_SETFOREGROUND);
    ExitProcess(1);
  }

  //pre-fill rom area with 0xFF
  DWORD address = (0xFFFF - iopage);
  do *(mem + iopage + address) = (BYTE)(0xFF);
  while (address--);

  // READ THE FIRMWARE ROM INTO THE ROM IMAGE
  TCHAR filename[MAX_PATH];
  _tcscpy(filename,progdir);
  _tcscat(filename,ROMfile);

  HANDLE file = CreateFile(filename,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           (LPSECURITY_ATTRIBUTES)NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                           NULL);
  if (file == INVALID_HANDLE_VALUE) {
    MessageBox(GetDesktopWindow(),
		       TEXT("Unable to open the required firmware ROM data file.")
			   TEXT("  Building an empty image in ROM filled with 0xFF's.")
               TEXT("  RESET will jump to 0x0300, IRQ to 0x0200, &")
		       TEXT(" NMI to 0x0280.  A '????????.ROM' file is needed."),
               TITLE,
               MB_ICONSTOP | MB_SETFOREGROUND);

	*(mem + 0xFFFA) = (BYTE)(0x80);  // NMI Vector LowByte        0x0280
	*(mem + 0xFFFB) = (BYTE)(0x02);  // NMI Vector HighByte
	*(mem + 0xFFFC) = (BYTE)(0x00);  // Reset Vector LowByte      0x0300
	*(mem + 0xFFFD) = (BYTE)(0x03);  // Reset Vector HighByte
	*(mem + 0xFFFE) = (BYTE)(0x00);  // IRQ Vector LowByte        0x0200
	*(mem + 0xFFFF) = (BYTE)(0x02);  // IRQ Vector HighByte
  }
  DWORD bytesread;
  ReadFile(file,(mem+0x8000),0x8000,&bytesread,NULL);  //always read 32k
  CloseHandle(file);    							   //memreset will fix ram

  MemReset();
}


void MemReset () {

  ZeroMemory(mem,iopage);

  // INITIALIZE THE CPU
  CpuInitialize();


}
