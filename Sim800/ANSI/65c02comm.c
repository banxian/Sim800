#include "65c02.h"

DWORD  baudrate       = CBR_19200;
BYTE   bytesize       = 8;
BYTE   commandbyte    = 0x00;
HANDLE commhandle     = INVALID_HANDLE_VALUE;
DWORD  comminactivity = 0;
BYTE   controlbyte    = 0x1F;
BYTE   parity         = NOPARITY;
BYTE   rts			  = RTS_CONTROL_DISABLE;
BYTE   dtr			  = DTR_CONTROL_DISABLE;
BYTE   recvbuffer[9];
DWORD  recvbytes      = 0;
DWORD  serialport     = 0;
BYTE   stopbits       = ONESTOPBIT;
BYTE   txi			  = 0;
BYTE   rxi			  = 0;

void UpdateCommState ();

BOOL CheckComm () {
  comminactivity = 0;
  if ((commhandle == INVALID_HANDLE_VALUE) && serialport) {
    TCHAR portname[8];
    wsprintf(portname,
             TEXT("COM%u"),
             serialport);
    commhandle = CreateFile(portname,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            (LPSECURITY_ATTRIBUTES)NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);
    if (commhandle != INVALID_HANDLE_VALUE) {
      UpdateCommState();
      COMMTIMEOUTS ct;
      ZeroMemory(&ct,sizeof(COMMTIMEOUTS));
      ct.ReadIntervalTimeout = MAXDWORD;
      SetCommTimeouts(commhandle,&ct);
    }
  }
  return (commhandle != INVALID_HANDLE_VALUE);
}

void CheckReceive () {
  if (recvbytes || (commhandle == INVALID_HANDLE_VALUE))
    return;
  ReadFile(commhandle,recvbuffer,8,&recvbytes,NULL);
}

void CloseComm () {
  if (commhandle != INVALID_HANDLE_VALUE)
    CloseHandle(commhandle);
  commhandle     = INVALID_HANDLE_VALUE;
  comminactivity = 0;
}

void UpdateCommState () {
  if (commhandle == INVALID_HANDLE_VALUE)
    return;
  DCB dcb;
  ZeroMemory(&dcb,sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);
  GetCommState(commhandle,&dcb);
  dcb.BaudRate = baudrate;
  dcb.ByteSize = bytesize;
  dcb.Parity   = parity;
  dcb.StopBits = stopbits;
  dcb.fRtsControl = rts;
  dcb.fDtrControl = dtr;
  SetCommState(commhandle,&dcb);
}

//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

BYTE __stdcall CommCommandRD (BYTE null1, BYTE null2) {
  if (!CheckComm())
    return 0x00;
  return commandbyte;
}

BYTE __stdcall CommCommandWR (BYTE write, BYTE value) {
  if (!CheckComm())
    return 0x00;

  commandbyte = value;

  // UPDATE THE PARITY
  if (commandbyte & 0x20)
    switch (commandbyte & 0xC0) {
      case 0x00 : parity = ODDPARITY;    break;
      case 0x40 : parity = EVENPARITY;   break;
      case 0x80 : parity = MARKPARITY;   break;
      case 0xC0 : parity = SPACEPARITY;  break;
    }
  else
   parity = NOPARITY;

  // UPDATE XMT CONTROL
  rts = RTS_CONTROL_DISABLE;
  if (commandbyte & 0x0C)
	rts = RTS_CONTROL_ENABLE;

  // UPDATE RX CONTROL
  dtr = DTR_CONTROL_DISABLE;
  if (commandbyte & 0x01)
	dtr = DTR_CONTROL_ENABLE;

  UpdateCommState();
  return commandbyte;
}

BYTE __stdcall CommControlRD (BYTE null1, BYTE null2) {
  if (!CheckComm())
    return 0;
  return controlbyte;
}


BYTE __stdcall CommControlWR (BYTE write, BYTE value) {
  if (!CheckComm())
    return 0;
  controlbyte = value;

  // UPDATE THE BAUD RATE
  switch (controlbyte & 0x0F) {
    case 0x00: // fall through
    case 0x01: // fall through
    case 0x02: // fall through
    case 0x03: // fall through
    case 0x04: // fall through
    case 0x05: baudrate = CBR_110;     break;
    case 0x06: baudrate = CBR_300;     break;
    case 0x07: baudrate = CBR_600;     break;
    case 0x08: baudrate = CBR_1200;    break;
    case 0x09: // fall through
    case 0x0A: baudrate = CBR_2400;    break;
    case 0x0B: // fall through
    case 0x0C: baudrate = CBR_4800;    break;
    case 0x0D: // fall through
    case 0x0E: baudrate = CBR_9600;    break;
    case 0x0F: baudrate = CBR_19200;   break;
  }

  // UPDATE THE BYTE SIZE
  switch (controlbyte & 0x60) {
    case 0x00: bytesize = 8;  break;
    case 0x20: bytesize = 7;  break;
    case 0x40: bytesize = 6;  break;
    case 0x60: bytesize = 5;  break;
  }

  // UPDATE THE NUMBER OF STOP BITS
  if (controlbyte & 0x80) {
    if ((bytesize == 8) && (parity == NOPARITY))
      stopbits = ONESTOPBIT;
    else if ((bytesize == 5) && (parity == NOPARITY))
      stopbits = ONE5STOPBITS;
    else
      stopbits = TWOSTOPBITS;
  }
  else
    stopbits = ONESTOPBIT;

  UpdateCommState();
  return controlbyte;
}

void CommDestroy () {
  if ((baudrate != CBR_19200) ||
      (bytesize != 8) ||
      (parity   != NOPARITY) ||
      (stopbits != ONESTOPBIT)) {
    CommReset();
    CheckComm();
  }
  CloseComm();
}

void CommSetSerialPort (HWND window, DWORD newserialport) {
  if (commhandle == INVALID_HANDLE_VALUE)
    serialport = newserialport;
  else if (newserialport != serialport)
    MessageBox(window,
               TEXT("You cannot change the serial port while it is ")
               TEXT("in use."),
               TEXT("Configuration"),
               MB_ICONEXCLAMATION);
}

void CommUpdate (DWORD totalcycles) {
  if (commhandle == INVALID_HANDLE_VALUE)
    return;

  if (!recvbytes)
    CheckReceive();

  if (recvbytes) {
    if (commandbyte & 0x01) {
      if (!(commandbyte & 0x02)) {
  	    rxi=1;
		if (irq) irq=0;
	  }
	}else {
	  recvbytes = 0;
    }
  }else {
    rxi=0;
  }


  if ((comminactivity += totalcycles) > 1000000) {
    static DWORD lastcheck = 0;
    if ((comminactivity > 2000000) || (comminactivity-lastcheck > 99950)) {
      DWORD modemstatus = 0;
      GetCommModemStatus(commhandle,&modemstatus);
      if (modemstatus & MS_RLSD_ON) {
         comminactivity = 0;
	  }
    }
    if (comminactivity > 2000000)
      CloseComm();

  }
}

BYTE __stdcall CommReceive (BYTE null1, BYTE null2) {
  if (!CheckComm())
    return 0;
  if (!recvbytes)
    CheckReceive();
  BYTE result = 0;
  if (recvbytes) {
    result = recvbuffer[0];
	if (--recvbytes) {
     //   MoveMemory(recvbuffer,recvbuffer+1,recvbytes);
        recvbuffer[0] = recvbuffer[1];
		recvbuffer[1] = recvbuffer[2];
		recvbuffer[2] = recvbuffer[3];
		recvbuffer[3] = recvbuffer[4];
		recvbuffer[4] = recvbuffer[5];
		recvbuffer[5] = recvbuffer[6];
		recvbuffer[6] = recvbuffer[7];
		recvbuffer[7] = recvbuffer[8];
	}

  }
  return result;
}

void CommReset () {
  CloseComm();
  baudrate    = CBR_19200;
  bytesize    = 8;
  commandbyte = 0x00;
  controlbyte = 0x1F;
  parity      = NOPARITY;
  recvbytes   = 0;
  stopbits    = ONESTOPBIT;
  rts		  = RTS_CONTROL_DISABLE;
  rxi		  =0;
  txi		  =0;
}

BYTE __stdcall CommStatus (BYTE null1, BYTE null2) {
  if (!CheckComm())
    return 0x70;
  if (!recvbytes)
    CheckReceive();
  DWORD modemstatus = 0;
  GetCommModemStatus(commhandle,&modemstatus);
  BYTE irqf = txi | rxi;
  txi=0;
  rxi=0;
  return 0x10 | (recvbytes                  ? 0x08 : 0x00)
              | ((modemstatus & MS_RLSD_ON) ? 0x00 : 0x20)
              | ((modemstatus & MS_DSR_ON)  ? 0x00 : 0x40)
			  | (irqf                       ? 0x80 : 0x00);
}

BYTE __stdcall CommTransmit (BYTE null1, BYTE value) {
  if (!CheckComm())
    return 0;
  DWORD byteswritten;
  WriteFile(commhandle,&value,1,&byteswritten,NULL);
  if ((commandbyte & 0x0C) == 0x04) {
	  txi=1;
	  if (irq) irq=0;
  }

  return 0;
}
