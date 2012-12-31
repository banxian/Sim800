//
// Copyright (c) 2004 K. Wilkins
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from
// the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//

//////////////////////////////////////////////////////////////////////////////
//                       Handy - An Atari Lynx Emulator                     //
//                          Copyright (c) 1996,1997                         //
//                                 K. Wilkins                               //
//////////////////////////////////////////////////////////////////////////////
// 65C02 Emulation class                                                    //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This class emulates a 65C02 processor. It is interfaced to the rest of   //
// the system via the PEEK/POKE macros and a number of global variables     //
//                                                                          //
//    K. Wilkins                                                            //
// August 1997                                                              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
// Revision History:                                                        //
// -----------------                                                        //
//                                                                          //
// 16Oct2007 AK Moved non-speed-intensive routines to a separate CPP file   //
// 01Aug1997 KW Document header added & class documented.                   //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool C65C02::ContextSave(FILE *fp)
{
  TRACE_CPU0("ContextSave()");

  int mPS;
  mPS=PS();
  if(!fprintf(fp,"C6502::ContextSave")) return 0;
  if(!fwrite(&mA,sizeof(ULONG),1,fp)) return 0;
  if(!fwrite(&mX,sizeof(ULONG),1,fp)) return 0;
  if(!fwrite(&mY,sizeof(ULONG),1,fp)) return 0;
  if(!fwrite(&mSP,sizeof(ULONG),1,fp)) return 0;
  if(!fwrite(&mPS,sizeof(ULONG),1,fp)) return 0;
  if(!fwrite(&mPC,sizeof(ULONG),1,fp)) return 0;
  if(!fwrite(&mIRQActive,sizeof(ULONG),1,fp)) return 0;

  return 1;
}

bool C65C02::ContextLoad(FILE *fp)
{
  //TRACE_CPU0("ContextLoad()");
  //int mPS;
  //char teststr[100]="XXXXXXXXXXXXXXXXXX";
  //if(!lss_read(teststr,sizeof(char),18,fp)) return 0;
  //if(strcmp(teststr,"C6502::ContextSave")!=0) return 0;
  //if(!lss_read(&mA,sizeof(ULONG),1,fp)) return 0;
  //if(!lss_read(&mX,sizeof(ULONG),1,fp)) return 0;
  //if(!lss_read(&mY,sizeof(ULONG),1,fp)) return 0;
  //if(!lss_read(&mSP,sizeof(ULONG),1,fp)) return 0;
  //if(!lss_read(&mPS,sizeof(ULONG),1,fp)) return 0;
  //if(!lss_read(&mPC,sizeof(ULONG),1,fp)) return 0;
  //if(!lss_read(&mIRQActive,sizeof(ULONG),1,fp)) return 0;
  //PS(mPS);
  //return 1;
}
