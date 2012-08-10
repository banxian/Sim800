/*
This program uses an .ini file instead of the system registry, my personal preference! dr
*/
extern "C" {
#include "65c02.h"
}


TCHAR buffer[32] = TEXT("");

BOOL RegLoadString (LPCTSTR section, LPCTSTR key,
                    LPTSTR buffer, DWORD chars) {
  TCHAR filename[MAX_PATH];
  _tcscpy(filename,progdir);
  _tcscat(filename, TEXT("65c02.ini"));
  return (GetPrivateProfileString(section,
                                  key,
                                  TEXT(""),
                                  buffer,
                                  chars,
                                  filename) != 0);
}

BOOL RegLoadValue (LPCTSTR section, LPCTSTR key, DWORD *value) {
  if (!value)
    return 0;
  buffer[0] = 0;
  if (!RegLoadString(section,key,buffer,32))
    return 0;
  buffer[31] = 0;
  *value = (DWORD)(_tcstoul(buffer,NULL,10));
  return 1;
}

void RegSaveString (LPCTSTR section, LPCTSTR key, LPCTSTR buffer) {
  TCHAR filename[MAX_PATH];
  _tcscpy(filename,progdir);
  _tcscat(filename, TEXT("65c02.ini"));
  WritePrivateProfileString(section,
                              key,
                              buffer,
                              filename);
}

void RegSaveValue (LPCTSTR section, LPCTSTR key, DWORD value) {
  buffer[0] = 0;
  _ultot(value,buffer,10);
  RegSaveString(section,key,buffer);
}

//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//


BOOL LoadReg() {
#define LOAD(a,b,c) if (!RegLoadValue(a,b,c)) return 0;
  DWORD buildnumber = 0;
  DWORD runningonos = 0;

/*  LOAD(TEXT("OS"),TEXT("CurrentBuildNumber"),&buildnumber);
  if (buildnumber != BUILDNUMBER)
    return 0;
*/
  LOAD(TEXT("OS"),TEXT("RunningOnOS"),&runningonos);
  if (runningonos != (GetVersion() & 0x0000FFFF))
    return 0;

  if (RegLoadString(TEXT("Configuration"),TEXT("Boot ROM"),buffer,32))
     _tcscpy(ROMfile,buffer);
  else
	 _tcscpy(ROMfile,TEXT("65c02.rom"));

  buffer[0] =0;
  if (RegLoadString(TEXT("Configuration"),TEXT("IO Page"),buffer,32))
      iopage = ((WORD)(LOWORD(_tcstoul(buffer,NULL,16)))) & 0xFF00;
  else
	  iopage=0x8000;

  autoboot = 0;
  speed = 10;
  LOAD(TEXT("Configuration"),TEXT("Auto Boot"),&autoboot);
  LOAD(TEXT("Configuration"),TEXT("Emulation Speed"),&speed);
  LOAD(TEXT("Configuration"),TEXT("Throttling Enabled"),&throttle);
  LOAD(TEXT("Simulation"),TEXT("IRQ Clock"),&irqclk);
  LOAD(TEXT("Simulation"),TEXT("NMI Clock"),&nmiclk);
  if (throttle) throttle = 1;
#undef LOAD

  return 1;
}

void SaveReg () {

#define SAVE(a,b,c) RegSaveValue(a,b,c);
//   SAVE(TEXT("OS"),TEXT("CurrentBuildNumber"),BUILDNUMBER);
   SAVE(TEXT("OS"),TEXT("RunningOnOS"),(GetVersion() & 0x0000FFFF));
   RegSaveString(TEXT("Configuration"),TEXT("Boot ROM"),ROMfile);
   SAVE(TEXT("Configuration"),TEXT("Auto Boot"),autoboot);
   _stprintf(buffer,TEXT("0x%04X\0"),(iopage & 0xFF00));
   RegSaveString(TEXT("Configuration"),TEXT("IO Page"),buffer);
   SAVE(TEXT("Configuration"),TEXT("Emulation Speed"),speed);
   SAVE(TEXT("Configuration"),TEXT("Throttling Enabled"),(throttle > 0));
   SAVE(TEXT("Simulation"),TEXT("IRQ Clock"),irqclk);
   SAVE(TEXT("Simulation"),TEXT("NMI Clock"),nmiclk);
#undef SAVE
}
