#include "65c02.h"

int      TermHpos		= 0;
int      TermLpos		= 23;
BOOL     TermColor		= 0;
BOOL     TermWrapline	= 0;
BOOL     Displayflag	= 0;
HFONT    Dispfont		= (HFONT)0;
TCHAR    DisplayBuffer[25][81];

// scroll the output window up one line
void TermScroll () {

    int linecount =0;

 	while (linecount < 24) {
	  _tcscpy(DisplayBuffer[linecount], DisplayBuffer[linecount+1]);
	  linecount++;
	}

}

//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

// Get chars from io port and send to the viewport
BYTE __stdcall TerminalOutputWR (BYTE null1, BYTE pchr) {

	if (Capture == 1) {
		Capture = 2;
		DrawStatusArea((HDC)0,0);
	    Capture = 1;
		if (Cfile) fprintf(Cfile,"%c",pchr);
	}



	pchr = pchr & 0x7F;

	if (pchr == 0x0A) {                  //  Line Feed
		TermLpos=23;
		TermScroll();
    }else if (pchr == 0x0D) {            //  Carrage Return
	    TermHpos=0;
		TermLpos=23;
    }else if (pchr == 0x07) {            //  Bell
	    MessageBeep(0xFFFFFFFF);
    }else if (pchr == 0x08) {            //  Backspace - destructable
        TermHpos--;
		if (TermHpos < 0)  {
			TermHpos=0;
			if (TermWrapline) {
				TermHpos=79;
				TermWrapline=0;
				TermLpos=22;
			}
		}
		DisplayBuffer[TermLpos][TermHpos] = 0;

	}else if (pchr == 0x09) {            //  TAB on 8 chr spacings
		while (TermHpos & 0x7) {
			DisplayBuffer[TermLpos][TermHpos] = 0x20;
			TermHpos++;
		}
    }else if (pchr >  0x1F) {            //  printable character
        DisplayBuffer[TermLpos][TermHpos] = pchr;
		TermHpos++;
		DisplayBuffer[TermLpos][TermHpos] = 0;
		if (TermHpos == 80) {
			TermHpos=0;
			if (TermLpos == 23) TermScroll(); else TermLpos = 23;
			TermWrapline=1;
		}
	}
    Displayflag = 1;

return 1;
}

BYTE __stdcall TerminalOutputRD (BYTE null1, BYTE null2) {
// Signal output port ready to receive next character
	// tell system ready for next char to be output
	// always ready so just return a 1
	// target system may not always be ready so use this
	// to emulate it
	return 1;
}

// Redraw the output window
void TerminalDisplay (BOOL drawbackground) {

  TCHAR Prtline[81];

  HDC dc = FrameGetDC();
  SelectObject(dc,Dispfont);
  SetTextAlign(dc,TA_TOP | TA_LEFT);

  // DRAW THE BACKGROUND
  if (drawbackground)   {
		RECT viewportrect;
		viewportrect.left   = 0;
		viewportrect.top    = 45;
		viewportrect.right  = 560;
		viewportrect.bottom = 384;
		SetBkColor(dc,0);                       //black background
		ExtTextOut(dc,0,0,ETO_OPAQUE,&viewportrect,TEXT(""),0,NULL);
  }

  // DRAW OUTPUT BUFFER   80 col x 24 lines
  {
	RECT linerect;
    linerect.left   = 0;
    linerect.top    = 45;
    linerect.right  = 560;
    linerect.bottom = linerect.top+16;


    SetTextColor(dc,TermColor ? 0 : 0xFFFFFF);				 //white text
    SetBkColor(dc,TermColor ? 0XFFFFFF : 0);           //black background

    int linecount =0;

    while (linecount < 24) {
	  _tcscpy(Prtline, DisplayBuffer[linecount]);
	  if (linecount == 0 && mode == MODE_STEPPING)
        _tcscpy(Prtline,TEXT("                                                                ...STEPPING...  "));

	  ExtTextOut(dc,linerect.left,linerect.top,
         ETO_CLIPPED | ETO_OPAQUE,&linerect,
         Prtline,_tcslen(Prtline),NULL);
      linerect.top    += 16;
      linerect.bottom += 16;
	  linecount++;
	}

  }
  FrameReleaseDC(dc);
  Displayflag = 0;
}

void TermInitialize () {

    // CREATE A FONT FOR THE TERMINAL SCREEN
   Dispfont = CreateFont(15,0,0,0,FW_MEDIUM,0,0,0,OEM_CHARSET,
                         OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                         DEFAULT_QUALITY,FIXED_PITCH | 4 | FF_MODERN,
                         TEXT("Courier New"));

   // initialize the line buffers
   _tcscpy(DisplayBuffer[0],TEXT(" "));
   _tcscpy(DisplayBuffer[1],TEXT(" "));
   _tcscpy(DisplayBuffer[2],TEXT(" "));
   _tcscpy(DisplayBuffer[3],TEXT(" "));
   _tcscpy(DisplayBuffer[4],TEXT(" "));
   _tcscpy(DisplayBuffer[5],TEXT(" "));
   _tcscpy(DisplayBuffer[6],TEXT(" "));
   _tcscpy(DisplayBuffer[7],TEXT(" "));
   _tcscpy(DisplayBuffer[8],TEXT(" "));
   _tcscpy(DisplayBuffer[9],TEXT(" "));
   _tcscpy(DisplayBuffer[10],TEXT(" "));
   _tcscpy(DisplayBuffer[11],TEXT(" "));
   _tcscpy(DisplayBuffer[12],TEXT(" "));
   _tcscpy(DisplayBuffer[13],TEXT(" "));
   _tcscpy(DisplayBuffer[14],TEXT(" "));
   _tcscpy(DisplayBuffer[15],TEXT(" "));
   _tcscpy(DisplayBuffer[16],TEXT(" "));
   _tcscpy(DisplayBuffer[17],TEXT(" "));
   _tcscpy(DisplayBuffer[18],TEXT(" "));
   _tcscpy(DisplayBuffer[19],TEXT(" "));
   _tcscpy(DisplayBuffer[20],TEXT(" "));
   _tcscpy(DisplayBuffer[21],TEXT(" "));
   _tcscpy(DisplayBuffer[22],TEXT(" "));
   _tcscpy(DisplayBuffer[23],TEXT(" "));
   _tcscpy(DisplayBuffer[24],TEXT("                                                                                "));

}

