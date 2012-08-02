#include "65c02.h"

BYTE __stdcall LTPDataPort (BYTE write, BYTE value) {

  if (write)
	  return _outp(0x378,value);
  else
	  return _inp(0x378);

}

BYTE __stdcall LTPStatusPort (BYTE write, BYTE value) {

  if (write)
	  return 0;  //_outp(0x379, value);
  else
	  return _inp(0x379);

}


BYTE __stdcall LTPCommandPort (BYTE write, BYTE value) {

  if (write)
	  return _outp(0x37A,value);
  else
	  return _inp(0x37A);

}

