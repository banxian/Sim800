#include "65c02.h"

BYTE __stdcall SpkrToggle (BYTE null1, BYTE null2) {

	int d=0;
	int v=0;
	d=_inp(0x61);
	v = d & 2;
	if (v==0) {
	  d = d + 2;
	} else {
	  d = d - 2;
	}
	_outp(0x61,d);

//   _asm(" push  %eax");
//	 _asm(" in    al,$97");
//	 _asm(" xor   al,$2");
//	 _asm(" out   $97,al");
//	 _asm(" pop   %eax ");

  return 0xff;
}

