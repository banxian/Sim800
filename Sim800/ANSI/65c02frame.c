#include "65c02.h"

#define  VIEWPORTCX  560
#define  VIEWPORTCY  384
#define  BUTTONCX    47
#define  BUTTONCY    45
#define  STATUSCY    25
#define  BUTTONS     12
#define  BTN_RUN     0
#define  BTN_IRQ     1
#define  BTN_NMI     2
#define  BTN_RESET   3
#define  BTN_LOAD    4
#define  BTN_SAVE    5
#define  BTN_CAPTURE 7
#define  BTN_DEBUG   8
#define  BTN_SETUP   9
#define  BTN_HELP    10
#define  BTN_QUIT    11

/*
#if (WINVER < 0x0400)
typedef struct tagWNDCLASSEX {
    UINT      cbSize;
    UINT      style;
    WNDPROC   lpfnWndProc;
    int       cbClsExtra;
    int       cbWndExtra;
    HINSTANCE hInstance;
    HICON     hIcon;
    HCURSOR   hCursor;
    HBRUSH    hbrBackground;
    LPCTSTR   lpszMenuName;
    LPCTSTR   lpszClassName;
    HICON     hIconSm;
} WNDCLASSEX;
#endif
*/
typedef struct _devmode40 {
    TCHAR dmDeviceName[32];
    WORD  dmVersions[2];
    WORD  dmSize;
    WORD  dmDriverExtra;
    DWORD dmFields;
    short dmPrintOptions[13];
    TCHAR dmFormName[32];
    WORD  dmLogPixels;
    DWORD dmBitsPerPel;
    DWORD dmPelsWidth;
    DWORD dmPelsHeight;
    DWORD dmDisplayFlags;
    DWORD dmDisplayFrequency;
    DWORD dmMediaInformation[6];
} DEVMODE40, *LPDEVMODE40;

typedef BOOL   (WINAPI *changedisptype)(LPDEVMODE40, DWORD);
typedef BOOL   (WINAPI *enumdisptype  )(LPCTSTR, DWORD, LPDEVMODE40);
typedef void   (WINAPI *initcomctltype)();
typedef HANDLE (WINAPI *loadimagetype )(HINSTANCE, LPCTSTR, UINT, int, int, UINT);
typedef ATOM   (WINAPI *regextype     )(CONST WNDCLASSEX *);


TCHAR   serialchoices[]   = TEXT("None\0")
                            TEXT("COM1\0")
                            TEXT("COM2\0")
                            TEXT("COM3\0")
                            TEXT("COM4\0");

HBITMAP ledbitmap[3];
HBITMAP buttonbitmap[BUTTONS];
HBITMAP logobitmap;

HBRUSH  btnfacebrush    = (HBRUSH)0;
HPEN    btnfacepen      = (HPEN)0;
HPEN    btnhighlightpen = (HPEN)0;
HPEN    btnshadowpen    = (HPEN)0;
int     buttonactive    = -1;
int     buttondown      = -1;
HRGN    clipregion      = (HRGN)0;
HWND    framewindow     = (HWND)0;
HWND	combowindow		= (HWND)0;
BOOL    helpquit        = 0;
HFONT   smallfont       = (HFONT)0;
BOOL    win40           = 0;
TCHAR   hexaddr[7]		= TEXT("0x8000");
DWORD   newboot			= 0;
DWORD   tthrottle       = 0;
TCHAR   irqtclk[9]		= TEXT("1000000");
DWORD   newirqclk		= 0;
TCHAR   nmitclk[9]		= TEXT("0");
DWORD   newnmiclk		= 0;


void    DrawStatusArea (HDC passdc, BOOL drawbackground);
void    EnableTrackbar (HWND window, BOOL enable);
void    FillComboBox (HWND window, int controlid, LPCTSTR choices, int currentchoice);
HBITMAP LoadButtonBitmap (HINSTANCE instance, LPCTSTR bitmapname);
void    ResetMachineState ();
void    VideoDisplayLogo ();
void    ProcessButtonClick (int button);

//*** CONFIG ***************************************
BOOL CALLBACK ConfigDlgProc (HWND   window,
                             UINT   message,
                             WPARAM wparam,
                             LPARAM lparam) {
  static BOOL afterclose = 0;
  TCHAR   dirlist[MAX_PATH];

  switch (message) {

    case WM_COMMAND:
      switch (LOWORD(wparam)) {

        case IDOK:
          {
            TCHAR   newROMfile[MAX_PATH];
			DlgDirSelectComboBoxEx(window,newROMfile,MAX_PATH,105);
			autoboot = (DWORD)IsDlgButtonChecked(window,101);
			GetDlgItemText(window,110,hexaddr,7);
			WORD  newiopage = ((WORD)(LOWORD(strtoul(hexaddr,NULL,16)))) & 0xFF00;
			DWORD newserialport = (DWORD)SendDlgItemMessage(window,104,CB_GETCURSEL,0,0);


            if (_tcscmp(ROMfile,newROMfile)) {
				_tcscpy(ROMfile,newROMfile);
				afterclose = 2;
			}

			if (newiopage != iopage) {
				iopage = newiopage;
				afterclose = 2;
			}

            CommSetSerialPort(window,newserialport);

			if (IsDlgButtonChecked(window,106))
              speed = SPEED_NORMAL;
            else
              speed = SendDlgItemMessage(window,108,TBM_GETPOS,0,0);

			GetDlgItemText(window,131,irqtclk,9);
			newirqclk = (DWORD)(strtoul(irqtclk,NULL,10));
			GetDlgItemText(window,132,nmitclk,9);
			newnmiclk = (DWORD)(strtoul(nmitclk,NULL,10));

            if (irqclk != newirqclk) {
				irqclk= newirqclk;
			}

            if (nmiclk != newnmiclk) {
				nmiclk= newnmiclk;
			}
			tthrottle = (DWORD)IsDlgButtonChecked(window,109);
			if (tthrottle) {
				throttle=11;
			} else {
			    throttle=0;
			}
			SaveReg();

          }
          EndDialog(window,1);
          if (afterclose)
            PostMessage(framewindow,WM_USER+afterclose,0,0);
          break;

        case IDCANCEL:
          EndDialog(window,0);
          break;

  		case 101:
			CheckDlgButton(window,101,((newboot=!newboot)== 1));
          break;

		case 106:
          SendDlgItemMessage(window,108,TBM_SETPOS,1,SPEED_NORMAL);
          EnableTrackbar(window,0);
		  {DWORD temp = SendDlgItemMessage(window,108,TBM_GETPOS,0,0);
		  sprintf(dirlist,"%d.%d MHz\0",(temp/10),(temp - ((temp/10)*10)));
		  SetDlgItemText(window,113,dirlist);}
		  break;

        case 107:
          SetFocus(GetDlgItem(window,108));
          EnableTrackbar(window,1);
		  break;

        case 108:
          CheckRadioButton(window,106,107,107);
          EnableTrackbar(window,1);
		  break;

  		case 109:
			CheckDlgButton(window,109,((tthrottle=!tthrottle)== 1));
          break;


        case 111:
		  benchmark = 2;
          PostMessage(window,WM_COMMAND,IDOK,(LPARAM)GetDlgItem(window,IDOK));
          break;

        case 112:
          if (MessageBox(window,
                         TEXT("Would you like to restart the Simulator now?"),
                         TEXT("Configuration"),
                         MB_ICONQUESTION | MB_YESNO) == IDYES) {
            afterclose = 2;
            PostMessage(window,WM_COMMAND,IDOK,(LPARAM)GetDlgItem(window,IDOK));
          }
          break;
      }
      break;

	  case WM_HSCROLL: {
		CheckRadioButton(window,106,107,107);
		DWORD temp = SendDlgItemMessage(window,108,TBM_GETPOS,0,0);
        sprintf(dirlist,"%d.%d MHz\0",(temp/10),(temp - ((temp/10)*10)));
	    SetDlgItemText(window,113,dirlist);
		}
      break;

    case WM_INITDIALOG:

      newboot=autoboot;
  	  _tcscpy(dirlist,progdir);
	  _tcscat(dirlist,TEXT("*.ROM"));
	  DlgDirListComboBox(window,dirlist,105,0,DDL_READWRITE);
	  SendDlgItemMessage(window,105,CB_SELECTSTRING,-1,(LPARAM)ROMfile);

	  CheckDlgButton(window,101,autoboot);
	  sprintf(hexaddr,"0x%04X\0",iopage);
	  SetDlgItemText(window,110,hexaddr);
	  FillComboBox(window,104,serialchoices,serialport);
      SendDlgItemMessage(window,108,TBM_SETRANGE,1,MAKELONG(1,SPEED_MAX));
      SendDlgItemMessage(window,108,TBM_SETPAGESIZE,0,5);
      SendDlgItemMessage(window,108,TBM_SETTICFREQ,10,0);
      SendDlgItemMessage(window,108,TBM_SETPOS,1,speed);
	  CheckRadioButton(window,106,107,107-(speed == 10));
      SetFocus(GetDlgItem(window,(speed == 10) ? 106 : 108));
      EnableTrackbar(window,1-(speed == 10));
	  sprintf(dirlist,"%d.%d MHz\0",(speed/10),(speed - ((speed/10)*10)));
	  CheckDlgButton(window,109,(throttle>0));
	  tthrottle = (throttle > 0);
	  SetDlgItemText(window,113,dirlist);
	  sprintf(irqtclk,"%d\0",irqclk);
	  SetDlgItemText(window,131,irqtclk);
	  sprintf(nmitclk,"%d\0",nmiclk);
	  SetDlgItemText(window,132,nmitclk);
	  SetDlgItemText(window,198, VERSION);
	  afterclose = 0;
	  return 1;
      break;

    case WM_LBUTTONDOWN:
      {
        POINT pt;
        pt.x = LOWORD(lparam);
        pt.y = HIWORD(lparam);
        ClientToScreen(window,&pt);
        RECT rect;
        GetWindowRect(GetDlgItem(window,108),&rect);
        if ((pt.x >= rect.left) && (pt.x <= rect.right) &&
            (pt.y >= rect.top) && (pt.y <= rect.bottom)) {
          CheckRadioButton(window,106,107,107);
          EnableTrackbar(window,1);
          SetFocus(GetDlgItem(window,108));
          ScreenToClient(GetDlgItem(window,108),&pt);
          PostMessage(GetDlgItem(window,108),WM_LBUTTONDOWN,
                      wparam,MAKELONG(pt.x,pt.y));

        }
      }
      break;

  }
  return 0;
}

//***  CAPTURE ******************************************
BOOL CALLBACK CaptureDlgProc (HWND   window,
                             UINT   message,
                             WPARAM wparam,
                             LPARAM lparam) {

  TCHAR   filename[MAX_PATH];
  TCHAR   fname[MAX_PATH];

  switch (message) {

    case WM_COMMAND:
      switch (LOWORD(wparam)) {

	    case IDOK:
		{
			GetDlgItemText(window,110,fname,MAX_PATH);
			if (fname[0] != 0) {
				_tcscpy(Capfile,fname);
				_tcscpy(filename,progdir);
				_tcscat(filename,Capfile);
				Cfile = fopen(filename,TEXT("a+t"));
				if (!Cfile) {
					Cfile = NULL;
					Capture = 0;
				}
			}
			else {
				Cfile = NULL;
				Capture = 0;
			}
		}
        EndDialog(window,1);
		break;

        case IDCANCEL:
		  Cfile = NULL;
		  Capture = 0;
          EndDialog(window,0);
          break;

      }
      break;

	case WM_INITDIALOG: {
		SetDlgItemText(window,110,Capfile);
    }
	return 1;
    break;

  }
  return 0;
}

//***  LOAD ******************************************
BOOL CALLBACK RamLoadDlgProc (HWND   window,
                             UINT   message,
                             WPARAM wparam,
                             LPARAM lparam) {
 // static BOOL afterclose = 0;
  TCHAR   dirlist[MAX_PATH];

  switch (message) {

    case WM_COMMAND:
      switch (LOWORD(wparam)) {

        case IDOK:
          {
			TCHAR  temp[10] = TEXT("");
			DWORD  startaddr = -1;
			WORD  numbytes  = 0;
			GetDlgItemText(window,102,temp,9);
			if (temp[0] != 0)
				startaddr = ((WORD)(LOWORD(strtoul(temp,NULL,16))));
			GetDlgItemText(window,103,temp,9);
			if (temp[0] != 0)
				numbytes = ((WORD)(LOWORD(strtoul(temp,NULL,16)))) ;
			DlgDirSelectComboBoxEx(window,RAMfile,MAX_PATH,101);
			if (RAMfile[0] != 0) {
            // open file and read into mem
				HANDLE file = CreateFile(RAMfile,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           (LPSECURITY_ATTRIBUTES)NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                           NULL);
				if (file == INVALID_HANDLE_VALUE)
					MessageBox(GetDesktopWindow(),
					TEXT("Unable to open the requested RAM data file."),
					TITLE,
					MB_ICONSTOP | MB_SETFOREGROUND);
				else  {
					DWORD bytesread;
					WORD info[2];
					ReadFile(file,(info),4,&bytesread,NULL);
					if (startaddr == -1) startaddr = info[0];
					if (numbytes == 0) numbytes = info[1];
					if (((WORD)startaddr + numbytes) >= iopage)
						numbytes = iopage - (WORD)startaddr;
					if ((WORD)startaddr < iopage)
						ReadFile(file,(mem+(WORD)startaddr),numbytes,&bytesread,NULL);
					CloseHandle(file);
				}
			}
          }
          EndDialog(window,1);
          break;

        case IDCANCEL:
          EndDialog(window,0);
          break;

		case 101:  {
			TCHAR  temp[10] = TEXT("");
			DWORD  startaddr = 0;
			WORD  numbytes  = 0;

			DlgDirSelectComboBoxEx(window,RAMfile,MAX_PATH,101);
			if (RAMfile[0] != 0) {
					// open file and read into mem
				HANDLE file = CreateFile(RAMfile,
								   GENERIC_READ,
								   FILE_SHARE_READ,
								   (LPSECURITY_ATTRIBUTES)NULL,
								   OPEN_EXISTING,
								   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
								   NULL);
				if (!(file == INVALID_HANDLE_VALUE))  {
					DWORD bytesread;
					WORD info[2];
					ReadFile(file,(info),4,&bytesread,NULL);
					startaddr = info[0];
					numbytes = info[1];
					CloseHandle(file);
				}
			  }
			  sprintf(temp,"0x%04X\0",(WORD)startaddr);
			  SetDlgItemText(window,102,temp);
			  sprintf(temp,"0x%04X\0",numbytes);
			  SetDlgItemText(window,103,temp);
		   }
		   break;

      }
      break;

	case WM_INITDIALOG: {
	  TCHAR  temp[10] = TEXT("");
	  DWORD  startaddr = 0;
	  WORD  numbytes  = 0;

  	  _tcscpy(dirlist,progdir);
	  _tcscat(dirlist,TEXT("*.RAM"));
	  DlgDirListComboBox(window,dirlist,101,0,DDL_READWRITE);
	  SendDlgItemMessage(window,101,CB_SELECTSTRING,-1,(LPARAM)RAMfile);
	  DlgDirSelectComboBoxEx(window,RAMfile,MAX_PATH,101);
	  if (RAMfile[0] != 0) {
      // open file and read into mem
		HANDLE file = CreateFile(RAMfile,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           (LPSECURITY_ATTRIBUTES)NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                           NULL);
		if (!(file == INVALID_HANDLE_VALUE))  {
			DWORD bytesread;
			WORD info[2];
			ReadFile(file,(info),4,&bytesread,NULL);
			startaddr = info[0];
			numbytes = info[1];
			CloseHandle(file);
		}
	  }
	  sprintf(temp,"0x%04X\0",(WORD)startaddr);
	  SetDlgItemText(window,102,temp);
	  sprintf(temp,"0x%04X\0",numbytes);
	  SetDlgItemText(window,103,temp);
	}
	return 1;
    break;

  }
  return 0;
}

//***  SAVE ******************************************
BOOL CALLBACK RamSaveDlgProc (HWND   window,
                             UINT   message,
                             WPARAM wparam,
                             LPARAM lparam) {
  //static BOOL afterclose = 0;
  TCHAR   dirlist[MAX_PATH];

  switch (message) {

    case WM_COMMAND:
      switch (LOWORD(wparam)) {

        case IDOK:
          {

			TCHAR  temp[10] = TEXT("");
			DWORD  startaddr = -1;
			WORD  numbytes  = 0;
			GetDlgItemText(window,102,temp,9);
			if (temp[0] != 0)
				startaddr = ((WORD)(LOWORD(strtoul(temp,NULL,16))));
			if (startaddr == -1) {EndDialog(window,1); break;}
			GetDlgItemText(window,103,temp,9);
			if (temp[0] != 0)
				numbytes = ((WORD)(LOWORD(strtoul(temp,NULL,16)))) ;
			if (numbytes == 0) {EndDialog(window,1); break;}
			if (((WORD)startaddr + numbytes) > iopage)
				numbytes = iopage - (WORD)startaddr;
			GetDlgItemText(window,104,dirlist,12); //check 8.3 file name
			if (dirlist[0] != 0) {
				int x = 0;
				for (x = 0; (dirlist[x]!=0) && dirlist[x] != TEXT('.')  ; x++);
				dirlist[x] = TEXT('.'); dirlist[x+1] = TEXT('r');
				dirlist[x+2] = TEXT('a'); dirlist[x+3] = TEXT('m');
				dirlist[x+4] = 0;
			}
			if ((dirlist[0] == 0) && (IsDlgButtonChecked(window,105) == BST_CHECKED))
				DlgDirSelectComboBoxEx(window,dirlist,MAX_PATH,101);
			if ((dirlist[0] != 0) && (WORD)startaddr < iopage) {

				_tcscpy(RAMfile,dirlist);
				_tcscpy(dirlist,progdir);
				_tcscat(dirlist,RAMfile);

			// open file and write mem
				HANDLE file = CreateFile(RAMfile,
                           GENERIC_WRITE,
                           0,
                           (LPSECURITY_ATTRIBUTES)NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN,
                           NULL);
				if (file == INVALID_HANDLE_VALUE)
					MessageBox(GetDesktopWindow(),
					TEXT("Unable to open the requested RAM data file."),
					TITLE,
					MB_ICONSTOP | MB_SETFOREGROUND);
				else  {
					DWORD byteswritten;
					WriteFile(file,&startaddr,2,&byteswritten,NULL);
					WriteFile(file,&numbytes,2,&byteswritten,NULL);
					WriteFile(file,(mem+(WORD)startaddr),numbytes,&byteswritten,NULL);
					CloseHandle(file);
				}
			}
          }
          EndDialog(window,1);
          break;

        case IDCANCEL:
          EndDialog(window,0);
          break;

		case 101:  {
			TCHAR  temp[10] = TEXT("");
			DWORD  startaddr = 0;
			WORD  numbytes  = 0;

			DlgDirSelectComboBoxEx(window,RAMfile,MAX_PATH,101);
			if (RAMfile[0] != 0) {
					// open file and read into mem
				HANDLE file = CreateFile(RAMfile,
								   GENERIC_READ,
								   FILE_SHARE_READ,
								   (LPSECURITY_ATTRIBUTES)NULL,
								   OPEN_EXISTING,
								   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
								   NULL);
				if (!(file == INVALID_HANDLE_VALUE))  {
					DWORD bytesread;
					WORD info[2];
					ReadFile(file,(info),4,&bytesread,NULL);
					startaddr = info[0];
					numbytes = info[1];
					CloseHandle(file);
				}
			  }
			  sprintf(temp,"0x%04X\0",(WORD)startaddr);
			  SetDlgItemText(window,102,temp);
			  sprintf(temp,"0x%04X\0",numbytes);
			  SetDlgItemText(window,103,temp);
		   }
		   break;

      }
      break;

	case WM_INITDIALOG: {
	  TCHAR  temp[10] = TEXT("");
	  DWORD  startaddr = 0;
	  WORD  numbytes  = 0;

  	  _tcscpy(dirlist,progdir);
	  _tcscat(dirlist,TEXT("*.RAM"));
	  DlgDirListComboBox(window,dirlist,101,0,DDL_READWRITE);
	  SendDlgItemMessage(window,101,CB_SETCURSEL,-1,0);
	  sprintf(temp,"0x%04X\0",0);
	  SetDlgItemText(window,102,temp);
	  SetDlgItemText(window,103,temp);
	  CheckDlgButton(window,105,0);
	}
	return 1;
    break;

  }
  return 0;
}

//***  KB FEEDER ******************************************
BOOL CALLBACK KBFeedDlgProc (HWND   window,
                             UINT   message,
                             WPARAM wparam,
                             LPARAM lparam) {
 // static BOOL afterclose = 0;
  TCHAR   dirlist[MAX_PATH];

  switch (message) {

    case WM_COMMAND:
      switch (LOWORD(wparam)) {

        case IDOK:
          {
			TCHAR  temp[10] = TEXT("");
			DWORD  startaddr = -1;
			WORD  numbytes  = 0;
			DlgDirSelectComboBoxEx(window,RAMfile,MAX_PATH,101);
			if (RAMfile[0] != 0) {
            // open file and read into mem
				HANDLE file = CreateFile(RAMfile,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           (LPSECURITY_ATTRIBUTES)NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                           NULL);
				if (file == INVALID_HANDLE_VALUE)
					MessageBox(GetDesktopWindow(),
					TEXT("Unable to open the requested data file."),
					TITLE,
					MB_ICONSTOP | MB_SETFOREGROUND);
				else  {
					DWORD bytesread;
					WORD info[2];
					ReadFile(file,(info),4,&bytesread,NULL);
					if (startaddr == -1) startaddr = info[0];
					if (numbytes == 0) numbytes = info[1];
					if (((WORD)startaddr + numbytes) >= iopage)
						numbytes = iopage - (WORD)startaddr;
					if ((WORD)startaddr < iopage)
						ReadFile(file,(mem+(WORD)startaddr),numbytes,&bytesread,NULL);
					CloseHandle(file);
				}
			}
          }
          EndDialog(window,1);
          break;

        case IDCANCEL:
          EndDialog(window,0);
          break;

		case 101:  {
			TCHAR  temp[10] = TEXT("");
			DWORD  startaddr = 0;
			WORD  numbytes  = 0;

			DlgDirSelectComboBoxEx(window,RAMfile,MAX_PATH,101);
			if (RAMfile[0] != 0) {
					// open file and read into mem
				HANDLE file = CreateFile(RAMfile,
								   GENERIC_READ,
								   FILE_SHARE_READ,
								   (LPSECURITY_ATTRIBUTES)NULL,
								   OPEN_EXISTING,
								   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
								   NULL);
				if (!(file == INVALID_HANDLE_VALUE))  {
					DWORD bytesread;
					WORD info[2];
					ReadFile(file,(info),4,&bytesread,NULL);
					startaddr = info[0];
					numbytes = info[1];
					CloseHandle(file);
				}
			  }
			  sprintf(temp,"0x%04X\0",(WORD)startaddr);
			  SetDlgItemText(window,102,temp);
			  sprintf(temp,"0x%04X\0",numbytes);
			  SetDlgItemText(window,103,temp);
		   }
		   break;

      }
      break;

	case WM_INITDIALOG: {
	  TCHAR  temp[10] = TEXT("");
	  DWORD  startaddr = 0;
	  WORD  numbytes  = 0;

  	  _tcscpy(dirlist,progdir);
	  _tcscat(dirlist,TEXT("*.txt"));
	  DlgDirListComboBox(window,dirlist,101,0,DDL_READWRITE);
	  SendDlgItemMessage(window,101,CB_SELECTSTRING,-1,(LPARAM)RAMfile);
	  DlgDirSelectComboBoxEx(window,RAMfile,MAX_PATH,101);
	  if (RAMfile[0] != 0) {
      // open file and read into mem
		HANDLE file = CreateFile(RAMfile,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           (LPSECURITY_ATTRIBUTES)NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                           NULL);
		if (!(file == INVALID_HANDLE_VALUE))  {
			DWORD bytesread;
			WORD info[2];
			ReadFile(file,(info),4,&bytesread,NULL);
			startaddr = info[0];
			numbytes = info[1];
			CloseHandle(file);
		}
	  }
	  sprintf(temp,"0x%04X\0",(WORD)startaddr);
	  SetDlgItemText(window,102,temp);
	  sprintf(temp,"0x%04X\0",numbytes);
	  SetDlgItemText(window,103,temp);
	}
	return 1;
    break;

  }
  return 0;
}


void CreateGdiObjects () {
  ZeroMemory(buttonbitmap,BUTTONS*sizeof(HBITMAP));
  buttonbitmap[BTN_HELP  ] = LoadButtonBitmap(instance,TEXT("HELP_BUTTON"));
  buttonbitmap[BTN_RUN   ] = LoadButtonBitmap(instance,TEXT("RUN_BUTTON"));
  buttonbitmap[BTN_DEBUG ] = LoadButtonBitmap(instance,TEXT("DEBUG_BUTTON"));
  buttonbitmap[BTN_SETUP ] = LoadButtonBitmap(instance,TEXT("SETUP_BUTTON"));
  buttonbitmap[BTN_QUIT  ] = LoadButtonBitmap(instance,TEXT("QUIT_BUTTON"));
  buttonbitmap[BTN_NMI   ] = LoadButtonBitmap(instance,TEXT("NMI_BUTTON"));
  buttonbitmap[BTN_IRQ   ] = LoadButtonBitmap(instance,TEXT("IRQ_BUTTON"));
  buttonbitmap[BTN_LOAD  ] = LoadButtonBitmap(instance,TEXT("LOAD_BUTTON"));
  buttonbitmap[BTN_SAVE  ] = LoadButtonBitmap(instance,TEXT("SAVE_BUTTON"));
  buttonbitmap[BTN_RESET ] = LoadButtonBitmap(instance,TEXT("RESET_BUTTON"));
 buttonbitmap[BTN_CAPTURE] = LoadButtonBitmap(instance,TEXT("CAPTURE_BUTTON"));
  ledbitmap[0] = LoadButtonBitmap(instance,TEXT("RND_LED_OFF"));
  ledbitmap[1] = LoadButtonBitmap(instance,TEXT("RND_LED_GRN"));
  ledbitmap[2] = LoadButtonBitmap(instance,TEXT("RND_LED_RED"));
  logobitmap = LoadButtonBitmap(instance,TEXT("B65C02_LOGO"));

  btnfacebrush    = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
  btnfacepen      = CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNFACE));
  btnhighlightpen = CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNHIGHLIGHT));
  btnshadowpen    = CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNSHADOW));

    // CREATE A FONT FOR THE TERMINAL SCREEN
   smallfont = CreateFont(15,0,0,0,FW_MEDIUM,0,0,0,OEM_CHARSET,
                         OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                         DEFAULT_QUALITY,FIXED_PITCH | 4 | FF_MODERN,
                         TEXT("Courier New"));

}

void DeleteGdiObjects () {
  int loop;
  for (loop = 0; loop < BUTTONS; loop++)
    DeleteObject(buttonbitmap[loop]);
  for (loop = 0; loop < 3; loop++)
    DeleteObject(ledbitmap[loop]);
  DeleteObject(btnfacebrush);
  DeleteObject(btnfacepen);
  DeleteObject(btnhighlightpen);
  DeleteObject(btnshadowpen);
  DeleteObject(smallfont);
}

void Draw3dRect (HDC dc, int x1, int y1, int x2, int y2, BOOL out) {
  SelectObject(dc,GetStockObject(NULL_BRUSH));
  SelectObject(dc,out ? btnshadowpen : btnhighlightpen);
  POINT pt[3];
  pt[0].x = x1;    pt[0].y = y2-1;
  pt[1].x = x2-1;  pt[1].y = y2-1;
  pt[2].x = x2-1;  pt[2].y = y1;
  Polyline(dc,(LPPOINT)&pt,3);
  SelectObject(dc,(out == 1) ? btnhighlightpen : btnshadowpen);
  pt[1].x = x1;    pt[1].y = y1;
  pt[2].x = x2;    pt[2].y = y1;
  Polyline(dc,(LPPOINT)&pt,3);
}

void DrawBitmapRect (HDC dc, int x, int y, LPRECT rect, HBITMAP bitmap) {
  HDC    dcmem = CreateCompatibleDC(dc);
  POINT  ptsize, ptorg;
  SelectObject(dcmem,bitmap);
  ptsize.x = rect->right+1-rect->left;
  ptsize.y = rect->bottom+1-rect->top;
  ptorg.x  = rect->left;
  ptorg.y  = rect->top;
  BitBlt(dc,x,y,ptsize.x,ptsize.y,dcmem,ptorg.x,ptorg.y,SRCCOPY);
  DeleteDC(dcmem);
}

void DrawButton (HDC passdc, int number) {
  HDC dc = (passdc ? passdc : GetDC(framewindow));
  int x  = number*BUTTONCX+4;
  int y  = 1;
  SelectObject(dc,GetStockObject(BLACK_PEN));
  MoveToEx(dc,x,y,NULL);
  LineTo(dc,x,y+BUTTONCY-1);
  LineTo(dc,x+BUTTONCX-1,y+BUTTONCY-1);
  if (number == buttondown) {
    int loop = 0;
    while (loop++ < 3)
      Draw3dRect(dc,x+loop,y+loop-1,x+BUTTONCX,y+BUTTONCY-1,0);
    RECT rect = {0,0,39,39};
    DrawBitmapRect(dc,x+4,y+3,&rect,buttonbitmap[number]);
  }
  else {
    Draw3dRect(dc,x+1,y,x+BUTTONCX,y+BUTTONCY-1,1);
    Draw3dRect(dc,x+2,y+1,x+BUTTONCX-1,y+BUTTONCY-2,1);
    RECT rect = {1,1,40,40};
   DrawBitmapRect(dc,x+3,y+2,&rect,buttonbitmap[number]);
  }

  if (!passdc)
    ReleaseDC(framewindow,dc);
}

void DrawFrameWindow (BOOL paint) {
  PAINTSTRUCT ps;
  HDC         dc = (paint ? BeginPaint(framewindow,&ps)
                          : GetDC(framewindow));

  // DRAW THE 3D BORDER AROUND THE EMULATED SCREEN
  Draw3dRect(dc,
             VIEWPORTX-2,VIEWPORTY-2+BUTTONCY,
             VIEWPORTX+VIEWPORTCX+3,VIEWPORTY+BUTTONCY+VIEWPORTCY+2,
             0);
  Draw3dRect(dc,
             VIEWPORTX-3,VIEWPORTY-3+BUTTONCY,
             VIEWPORTX+VIEWPORTCX+4,VIEWPORTY+BUTTONCY+VIEWPORTCY+3,
             0);
  SelectObject(dc,btnfacepen);
  Rectangle(dc,
            VIEWPORTX-4,VIEWPORTY-4+BUTTONCY,
            VIEWPORTX+VIEWPORTCX+5,VIEWPORTY+BUTTONCY+VIEWPORTCY+4);
  Rectangle(dc,
            VIEWPORTX-5,VIEWPORTY-5+BUTTONCY,
            VIEWPORTX+VIEWPORTCX+6,VIEWPORTY+BUTTONCY+VIEWPORTCY+5);

  // FILL IN BUTTON AREA
  SelectObject(dc,GetStockObject(NULL_PEN));
  SelectObject(dc,btnfacebrush);
  Rectangle(dc,0,0,VIEWPORTX+VIEWPORTCX+7,BUTTONCY+1);


  // DRAW THE TOOLBAR BUTTONS
  int loop = BUTTONS;
  while (loop--)
    DrawButton(dc,loop);

  // DRAW THE STATUS AREA
  DrawStatusArea(dc,1);

  if (paint)
    EndPaint(framewindow,&ps);
  else
    ReleaseDC(framewindow,dc);

  // DRAW THE CONTENTS OF THE EMULATED SCREEN
  if (mode == MODE_LOGO)  {

	RECT rect;
	rect.left   = 0;
    rect.top    = 0,
	rect.right  = 560;
	rect.bottom = 384;
	DrawBitmapRect (dc, 5, 50, &rect, logobitmap);


  }
  else if (mode == MODE_DEBUG)
    DebugDisplay(1);
  else
    TerminalDisplay(1);
}


void EnableTrackbar (HWND window, BOOL enable) {
  EnableWindow(GetDlgItem(window,108),enable);
  int loop = 120;
  while (loop++ < 125)
    EnableWindow(GetDlgItem(window,loop),enable);
}

void FillComboBox (HWND window, int controlid, LPCTSTR choices, int currentchoice) {
  HWND combowindow = GetDlgItem(window,controlid);
  SendMessage(combowindow,CB_RESETCONTENT,0,0);
  while (*choices) {
    SendMessage(combowindow,CB_ADDSTRING,0,(LPARAM)(LPCTSTR)choices);
    choices += _tcslen(choices)+1;
  }
  SendMessage(combowindow,CB_SETCURSEL,currentchoice,0);
}

//*** FRAME *********************************************
LRESULT CALLBACK FrameWndProc (HWND   window,
                               UINT   message,
                               WPARAM wparam,
                               LPARAM lparam) {

int omode = 0;

  switch (message) {

    case WM_ACTIVATE:
      break;

    case WM_CLOSE:
	  if (helpquit) {
			helpquit = 0;
			TCHAR filename[MAX_PATH];
			_tcscpy(filename,progdir);
			_tcscat(filename,TEXT("65c02.hlp"));
			WinHelp(window,filename,HELP_QUIT,0);
	  }
      break;

    case WM_CHAR:
      if (mode == MODE_DEBUG)
        DebugProcessChar((TCHAR)wparam);
      break;

    case WM_CREATE:
      framewindow = window;
      CreateGdiObjects();
      PostMessage(window,WM_USER,0,0);
      break;

    case WM_DESTROY:
	  DebugDestroy();
	  CommDestroy();
	  MemDestroy();
	  DeleteGdiObjects();
	  PostQuitMessage(0);
      break;

    case WM_KEYDOWN:
      if ((
		  ((wparam >= VK_F1) && (wparam <= VK_F10))
		  ||
		  ((wparam == VK_F11) && (GetKeyState(VK_SHIFT) < 0))
		  ||
		  ((wparam == VK_F12) && (GetKeyState(VK_SHIFT) < 0))
		  )
		  && (buttondown == -1)) {
        DrawButton((HDC)0,buttondown = wparam-VK_F1);
      }
      else if (wparam == VK_CAPITAL)
        KeybQueueKeypress((int)wparam,0);
      else if (wparam == VK_PAUSE) {
		switch (mode) {
          case MODE_RUNNING:
			  mode = MODE_PAUSED;
			  break;
          case MODE_PAUSED:
			  mode = MODE_RUNNING;
			  ClearCounters();
			  break;
          case MODE_STEPPING:
			  DebugProcessCommand(VK_ESCAPE);
			  break;
        }
	    FrameRefreshStatus ();
        if ((mode != MODE_LOGO) && (mode != MODE_DEBUG))
        TerminalDisplay(1);
      }
      else if ((mode == MODE_RUNNING) || (mode == MODE_LOGO) ||
               ((mode == MODE_STEPPING) && (wparam != VK_ESCAPE))) {
        BOOL autorep  = ((lparam & 0x40000000) != 0);
        BOOL extended = ((lparam & 0x01000000) != 0);

        if (mode != MODE_LOGO)
          KeybQueueKeypress((int)wparam,extended); //***gets keybrd during run
      }
      else if ((mode == MODE_DEBUG) || (mode == MODE_STEPPING))
        DebugProcessCommand(wparam);
      if (wparam == VK_F10) {
        return 0;
      }
      break;

    case WM_KEYUP:
      if ((wparam >= VK_F1) && (wparam <= VK_F12) && (buttondown == (int)wparam-VK_F1)) {
        buttondown = -1;
        DrawButton((HDC)0,wparam-VK_F1);
        ProcessButtonClick(wparam-VK_F1);
      }
      break;

    case WM_LBUTTONDOWN:
      if (buttondown == -1) {
        int x = LOWORD(lparam);
		int y = HIWORD(lparam);
        if ((y < BUTTONCY) && ((x-4) < BUTTONS*BUTTONCX)) {
          DrawButton((HDC)0,buttonactive = buttondown = (x-4)/BUTTONCX);
        }
      }
      break;

    case WM_LBUTTONUP:
      if (buttonactive != -1) {
        if (buttondown == buttonactive) {
          buttondown = -1;
          DrawButton((HDC)0,buttonactive);
          ProcessButtonClick(buttonactive);
        }
        buttonactive = -1;
      }
      break;

    case WM_MOUSEMOVE:  //** add to stop cmd if mouse drug off button???
	{
			int x = LOWORD(lparam);
			int y = HIWORD(lparam);

		if (buttondown != -1) {
				if (y > BUTTONCY || ((x-4)/BUTTONCX) != buttonactive)  {
					buttondown = -1;
					DrawButton((HDC)0,buttonactive);
					buttonactive = -1;
				}
		}
	}
		break;

    case WM_PAINT:
      if (GetUpdateRect(window,NULL,0))
        DrawFrameWindow(1);
      break;

    case WM_QUERYNEWPALETTE:
      return 1;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
      break;

    case WM_SYSCOLORCHANGE:
      DeleteGdiObjects();
      CreateGdiObjects();
      break;

    case WM_SYSKEYDOWN:
      PostMessage(window,WM_KEYDOWN,wparam,lparam);
      if ((wparam == VK_F10) || (wparam == VK_MENU))
        return 0;
      break;

    case WM_SYSKEYUP:
      PostMessage(window,WM_KEYUP,wparam,lparam);
      break;

    case WM_TIMER:
      break;

    case WM_USER:
      SetTimer(window,1,333,NULL);
      if (autoboot) {
        ProcessButtonClick(BTN_RUN);
      }
      break;

    case WM_USER+2:
	  omode = mode;
	  if (mode != MODE_LOGO)  {
          if (mode == MODE_RUNNING) mode = MODE_PAUSED;
		  FrameRefreshStatus ();
	      if (MessageBox(framewindow,
                       TEXT("Restarting the Simulator will reset the state\n")
                       TEXT("of the emulated machine, causing you to \n")
                       TEXT("lose any unsaved work.\n\n")
                       TEXT("Are you sure you want to do this?"),
                       TEXT("Configuration"),
                       MB_ICONQUESTION | MB_YESNO) == IDNO)   {
			  mode = omode;
			  FrameRefreshStatus ();
              break;
		  }
      }
	  restart = 1;
      PostMessage(window,WM_CLOSE,0,0);
      break;

  }
  return DefWindowProc(window,message,wparam,lparam);
}


HBITMAP LoadButtonBitmap (HINSTANCE instance, LPCTSTR bitmapname) {
  HBITMAP bitmap = LoadBitmap(instance,bitmapname);
  if (!bitmap)
    return bitmap;
  BITMAP info;
  GetObject(bitmap,sizeof(BITMAP),&info);
#ifdef WIN31
  info.bmBitsPixel = 0;
#endif
  if (info.bmBitsPixel >= 8) {
    DWORD  bytespixel = info.bmBitsPixel >> 3;
    DWORD  bytestotal = info.bmHeight*info.bmWidthBytes;
    DWORD  pixelmask  = 0xFFFFFFFF >> (32-info.bmBitsPixel);
    LPBYTE data       = (LPBYTE)VirtualAlloc(NULL,bytestotal+4,MEM_COMMIT,PAGE_READWRITE);
    if (!data)
      return bitmap;
    if (pixelmask == 0xFFFFFFFF)
      pixelmask = 0xFFFFFF;
    if (GetBitmapBits(bitmap,bytestotal,data) == (LONG)bytestotal) {
      DWORD origval = *(LPDWORD)data & pixelmask;
      HWND  window  = GetDesktopWindow();
      HDC   dc      = GetDC(window);
      HDC   memdc   = CreateCompatibleDC(dc);
      SelectObject(memdc,bitmap);
      SetPixelV(memdc,0,0,GetSysColor(COLOR_BTNFACE));
      DeleteDC(memdc);
      ReleaseDC(window,dc);
      GetBitmapBits(bitmap,bytestotal,data);
      DWORD newval     = *(LPDWORD)data & pixelmask;
      DWORD offset     = 0;
      DWORD lineoffset = 0;
      while (offset < bytestotal) {
        if ((*(LPDWORD)(data+offset) & pixelmask) == origval)
          *(LPDWORD)(data+offset) = (*(LPDWORD)(data+offset) & ~pixelmask)
                                      | newval;
        offset     += bytespixel;
        lineoffset += bytespixel;
        if (lineoffset+bytespixel > (DWORD)info.bmWidthBytes) {
          offset    += info.bmWidthBytes-lineoffset;
          lineoffset = 0;
        }
      }
      SetBitmapBits(bitmap,bytestotal,data);
    }
    VirtualFree(data,0,MEM_RELEASE);
  }
  else {
    HWND window = GetDesktopWindow();
    HDC  dc     = GetDC(window);
    HDC  memdc  = CreateCompatibleDC(dc);
    SelectObject(memdc,bitmap);
    COLORREF origcol = GetPixel(memdc,0,0);
    COLORREF newcol  = GetSysColor(COLOR_BTNFACE);
    int y = 0;
    do {
      int x = 0;
      do
        if (GetPixel(memdc,x,y) == origcol)
#ifdef WIN31
          SetPixel(memdc,x,y,newcol);
#else
          SetPixelV(memdc,x,y,newcol);
#endif
      while (++x < info.bmWidth);
    } while (++y < info.bmHeight);
    DeleteDC(memdc);
    ReleaseDC(window,dc);
  }
  return bitmap;
}

void ProcessButtonClick (int button) {
  switch (button) {

    case BTN_HELP:
      {
        TCHAR filename[MAX_PATH];
        _tcscpy(filename,progdir);
        _tcscat(filename,TEXT("65c02.hlp"));
        WinHelp(framewindow,filename,HELP_CONTENTS,0);
        helpquit = 1;
      }
      break;

    case BTN_RUN:
      if (mode == MODE_LOGO);
      else if (mode == MODE_RUNNING)
        ResetMachineState();
      if ((mode == MODE_DEBUG) || (mode == MODE_STEPPING))
        DebugEnd();
      mode = MODE_RUNNING;
	  ClearCounters();
	  TerminalDisplay(1);
	  FrameRefreshStatus ();
	  break;

    case BTN_DEBUG:
      if (mode == MODE_LOGO)
        ResetMachineState();
      if (mode == MODE_STEPPING)
        DebugProcessCommand(VK_ESCAPE);
      else if (mode == MODE_DEBUG)
        ProcessButtonClick(BTN_RUN);
      else {
        DebugBegin();
      }
	  FrameRefreshStatus ();
      break;

    case BTN_SETUP: {
      int omode = mode;
	  if (mode == MODE_RUNNING) mode = MODE_PAUSED;
	  FrameRefreshStatus ();
	  {
        HINSTANCE      comctlinstance = LoadLibrary(TEXT("COMCTL32"));
        initcomctltype initcomctl     = (initcomctltype)
                                        GetProcAddress(comctlinstance,
                                                       TEXT("InitCommonControls"));
        if (initcomctl) {
          initcomctl();
          DialogBox(instance,
                    TEXT("CONFIGURATION_DIALOG"),
                    framewindow,
                    (DLGPROC)ConfigDlgProc);
        }
        else
          MessageBox(framewindow,
                     TEXT("Required file ComCtl32.dll not found."),
                     TITLE,
                     MB_ICONEXCLAMATION);
        FreeLibrary(comctlinstance);
      }
	  mode = omode;
	  FrameRefreshStatus ();
	  ClearCounters();
	  }
	  break;

	case BTN_QUIT:   	{
	  KillTimer(framewindow,1);
	  if (helpquit) {
			helpquit = 0;
			TCHAR filename[MAX_PATH];
			_tcscpy(filename,progdir);
			_tcscat(filename,TEXT("65c02.hlp"));
			WinHelp(framewindow,filename,HELP_QUIT,0);
	  }

	  if (Capture) {
		fclose(Cfile);
		Cfile = NULL;
	  }

	  DebugDestroy();
	  CommDestroy();
	  MemDestroy();
	  DeleteGdiObjects();
	  PostQuitMessage(0);
	  }
      break;

	case BTN_IRQ:
	  if (irq) irq=0;
	  break;

	case BTN_NMI:
	  if (nmi) nmi=0;
      break;

    case BTN_LOAD: {
      int omode = mode;
	  if (mode == MODE_RUNNING) mode = MODE_PAUSED;
	  FrameRefreshStatus ();
	  {
        HINSTANCE      comctlinstance = LoadLibrary(TEXT("COMCTL32"));
        initcomctltype initcomctl     = (initcomctltype)
                                        GetProcAddress(comctlinstance,
                                                       TEXT("InitCommonControls"));
        if (initcomctl) {
          initcomctl();
          DialogBox(instance,
                    TEXT("RAM_FILE_LOAD"),
                    framewindow,
                    (DLGPROC)RamLoadDlgProc);
        }
        else
          MessageBox(framewindow,
                     TEXT("Required file ComCtl32.dll not found."),
                     TITLE,
                     MB_ICONEXCLAMATION);
        FreeLibrary(comctlinstance);
      }
	  mode = omode;
	  FrameRefreshStatus ();
      ClearCounters();
	  }
	  break;

    case BTN_SAVE: {
      int omode = mode;
	  if (mode == MODE_RUNNING) mode = MODE_PAUSED;
	  FrameRefreshStatus ();
	  {
        HINSTANCE      comctlinstance = LoadLibrary(TEXT("COMCTL32"));
        initcomctltype initcomctl     = (initcomctltype)
                                        GetProcAddress(comctlinstance,
                                                       TEXT("InitCommonControls"));
        if (initcomctl) {
          initcomctl();
          DialogBox(instance,
                    TEXT("RAM_FILE_SAVE"),
                    framewindow,
                    (DLGPROC)RamSaveDlgProc);
        }
        else
          MessageBox(framewindow,
                     TEXT("Required file ComCtl32.dll not found."),
                     TITLE,
                     MB_ICONEXCLAMATION);
        FreeLibrary(comctlinstance);
      }
	  mode = omode;
	  FrameRefreshStatus ();
      ClearCounters();
	  }
	  break;

      case BTN_RESET: {
	  CpuInitialize();
	  FrameRefreshStatus ();
	  ClearCounters();
	 }
	  break;

	  case BTN_CAPTURE: {

		  Capture = 1 - Capture;

		  if (Capture) {

	  		  {
				HINSTANCE      comctlinstance = LoadLibrary(TEXT("COMCTL32"));
				initcomctltype initcomctl     = (initcomctltype)
												GetProcAddress(comctlinstance,
															   TEXT("InitCommonControls"));
				if (initcomctl) {
				  initcomctl();
				  DialogBox(instance,
							TEXT("CAPTURE"),
							framewindow,
							(DLGPROC)CaptureDlgProc);
				}
				else
				  MessageBox(framewindow,
							 TEXT("Required file ComCtl32.dll not found."),
							 TITLE,
							 MB_ICONEXCLAMATION);
				FreeLibrary(comctlinstance);
			  }

		  }
		  else {
			fclose(Cfile);
			Cfile = NULL;
		  }
	  }
	  break;


  }
}

void ResetMachineState () {
  MemReset();
  CommReset();
}

//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

void DrawStatusArea (HDC passdc, BOOL drawbackground) {
  TCHAR regdat[12] = TEXT("");
  HDC dc = (passdc ? passdc : GetDC(framewindow));
  int x  = 0;
  int y  = (VIEWPORTY<<1)+BUTTONCY+VIEWPORTCY;

  if (drawbackground) {
    SelectObject(dc,GetStockObject(NULL_PEN));
    SelectObject(dc,btnfacebrush);
    Rectangle(dc,x,y,x+VIEWPORTCX+12,y+STATUSCY+1);
    Draw3dRect(dc,x+4,
		          y+2,
		          x+VIEWPORTCX+8,
				  y+STATUSCY-2,0);
	SelectObject(dc,smallfont);
    SetTextAlign(dc,TA_LEFT | TA_TOP);
    SetTextColor(dc,0);
    SetBkMode(dc,TRANSPARENT);
    TextOut(dc,x+7,y+5,TEXT("Run"),3);
    TextOut(dc,x+55,y+5,TEXT("Debug"),5);
	TextOut(dc,x+115,y+5,TEXT("Capture"),7);
	TextOut(dc,x+192,y+5,TEXT("PC="),3);
	TextOut(dc,x+255,y+5,TEXT("A="),2);
	TextOut(dc,x+297,y+5,TEXT("X="),2);
	TextOut(dc,x+339,y+5,TEXT("Y="),2);
	TextOut(dc,x+381,y+5,TEXT("SP="),3);
	TextOut(dc,x+444,y+5,TEXT("PS="),3);

  }

  {
    RECT rect   = {0,0,8,8};
    int  drive1 = (mode == MODE_RUNNING)+ 2*(mode == MODE_PAUSED);
    int  drive2 = (mode == MODE_STEPPING)+ 2*(mode == MODE_DEBUG);
	int  drive3 = Capture;
    DrawBitmapRect(dc,x+34,y+8,&rect,ledbitmap[drive1]);
    DrawBitmapRect(dc,x+95,y+8,&rect,ledbitmap[drive2]);
	DrawBitmapRect(dc,x+170,y+8,&rect,ledbitmap[drive3]);
	SelectObject(dc,smallfont);
    SetTextAlign(dc,TA_LEFT | TA_TOP);
    SetTextColor(dc,0x800000);
	SetBkColor(dc,0xc0c0c0);


    sprintf(regdat,"%04X\0",regs.pc);
	rect.left=x+213; rect.top=y+5; rect.right=x+213+28; rect.bottom=y+5+16;
	ExtTextOut(dc,x+213,y+5,ETO_CLIPPED | ETO_OPAQUE,&rect,regdat,4,NULL);

	sprintf(regdat,"%02X\0",regs.a);
	rect.left=x+269; rect.top=y+5; rect.right=x+269+14; rect.bottom=y+5+16;
	ExtTextOut(dc,x+269,y+5,ETO_CLIPPED | ETO_OPAQUE,&rect,regdat,2,NULL);

	sprintf(regdat,"%02X\0",regs.x);
	rect.left=x+311; rect.top=y+5; rect.right=x+311+14; rect.bottom=y+5+16;
	ExtTextOut(dc,x+311,y+5,ETO_CLIPPED | ETO_OPAQUE,&rect,regdat,2,NULL);

	sprintf(regdat,"%02X\0",regs.y);
	rect.left=x+353; rect.top=y+5; rect.right=x+353+14; rect.bottom=y+5+16;
	ExtTextOut(dc,x+353,y+5,ETO_CLIPPED | ETO_OPAQUE,&rect,regdat,2,NULL);

    sprintf(regdat,"%04X\0",regs.sp);
	rect.left=x+402; rect.top=y+5; rect.right=x+402+28; rect.bottom=y+5+16;
	ExtTextOut(dc,x+402,y+5,ETO_CLIPPED | ETO_OPAQUE,&rect,regdat,4,NULL);

	_tcscpy(regdat,TEXT("  NVRBDIZC"));
	regdat[1]=0;
	int loop = 8;
	BYTE value = regs.ps;
	rect.left=x+465+56; rect.top=y+5; rect.right=x+465+64; rect.bottom=y+5+16;
	while (loop--) {
		regdat[0] = regdat[loop+2];
		SetTextColor(dc,value & 1 ? 0x000080:0x800000);
		ExtTextOut(dc,rect.left,rect.top,
                 ETO_CLIPPED | ETO_OPAQUE,&rect,
                 regdat,1,NULL);
		rect.left  -= 8;
		rect.right -= 8;
//	if (!(value & 1))
//			mnemonic[loop] = TEXT('.');
	value >>= 1;
	}

	SetTextColor(dc,0x800000);
	SetBkColor(dc,0xc0c0c0);
	regdat[0] = 0;
	sprintf(regdat,"%04X\0",throttle);
	rect.left=x+535; rect.top=y+5; rect.right=x+535+28; rect.bottom=y+5+16;
	ExtTextOut(dc,x+535,y+5,ETO_CLIPPED | ETO_OPAQUE,&rect,regdat,4,NULL);


  }

  if (!passdc)
    ReleaseDC(framewindow,dc);
}


void FrameCreateWindow () {
  int width  = VIEWPORTCX + (VIEWPORTX<<1)
                          + (GetSystemMetrics(SM_CXBORDER)<<1)
                          + (win40 ? 5 : 0);
  int height = VIEWPORTCY + (VIEWPORTY<<1)
						  + BUTTONCY
						  + STATUSCY
                          + GetSystemMetrics(SM_CYBORDER)
                          + GetSystemMetrics(SM_CYCAPTION)
                          + (win40 ? 5 : 0);
  framewindow = CreateWindow(TEXT("65C02FRAME"),
                             TITLE,
                             WS_OVERLAPPED
                               | WS_BORDER
							   | WS_CAPTION
                               | WS_SYSMENU
                               | WS_MINIMIZEBOX
                               | WS_VISIBLE,
                             (GetSystemMetrics(SM_CXSCREEN)-width ) >> 1,
                             (GetSystemMetrics(SM_CYSCREEN)-height) >> 1,
                             width,
                             height,
                             HWND_DESKTOP,
                             (HMENU)0,
                             instance,
                             NULL);
//  Ctl3dAutoSubclass((HANDLE)framewindow);
}

HDC FrameGetDC () {

#ifdef CLIPVIEWPORT
  RECT rect;
  rect.left   = VIEWPORTX+1;
  rect.top    = VIEWPORTY+1;
  rect.right  = VIEWPORTX+VIEWPORTCX;
  rect.bottom = VIEWPORTY+VIEWPORTCY;
  InvalidateRect(framewindow,&rect,0);
  ClientToScreen(framewindow,(LPPOINT)&rect.left);
  ClientToScreen(framewindow,(LPPOINT)&rect.right);
  clipregion = CreateRectRgn(rect.left,rect.top,rect.right,rect.bottom);
  HDC dc = GetDCEx(framewindow,clipregion,DCX_INTERSECTRGN | DCX_VALIDATE);
#else
  HDC dc = GetDC(framewindow);
#endif
  SetViewportOrgEx(dc,VIEWPORTX,VIEWPORTY,NULL);
  return dc;
}

void FrameRefreshStatus () {
  DrawStatusArea((HDC)0,0);
}

void FrameRegisterClass () {
#ifndef WIN31
  win40 = ((GetVersion() & 0xFF) >= 4);
#endif
  if (win40) {
    HINSTANCE     userinst   = LoadLibrary(TEXT("USER32"));
    loadimagetype loadimage  = NULL;
    regextype     registerex = NULL;
    if (userinst) {
#ifdef UNICODE
      loadimage  = (loadimagetype)GetProcAddress(userinst,TEXT("LoadImageW"));
      registerex = (regextype)GetProcAddress(userinst,TEXT("RegisterClassExW"));
#else
      loadimage  = (loadimagetype)GetProcAddress(userinst,TEXT("LoadImageA"));
      registerex = (regextype)GetProcAddress(userinst,TEXT("RegisterClassExA"));
#endif
    }
    if (loadimage && registerex) {
      WNDCLASSEX wndclass;
      ZeroMemory(&wndclass,sizeof(WNDCLASSEX));
      wndclass.cbSize        = sizeof(WNDCLASSEX);
      wndclass.style         = CS_OWNDC | CS_BYTEALIGNCLIENT;
      wndclass.lpfnWndProc   = FrameWndProc;
      wndclass.hInstance     = instance;
      wndclass.hIcon         = LoadIcon(instance,TEXT("B65C02_ICON"));
      wndclass.hCursor       = LoadCursor(0,IDC_ARROW);
      wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
      wndclass.lpszClassName = TEXT("65C02FRAME");
      wndclass.hIconSm       = loadimage(instance,TEXT("B65C02_ICON"),
                                         1,16,16,0);
      if (!registerex(&wndclass))
        win40 = 0;
    }
    else
      win40 = 0;
    if (userinst)
      FreeLibrary(userinst);
  }
  if (!win40) {
    WNDCLASS wndclass;
    ZeroMemory(&wndclass,sizeof(WNDCLASS));
    wndclass.style         = CS_OWNDC | CS_BYTEALIGNCLIENT;
    wndclass.lpfnWndProc   = FrameWndProc;
    wndclass.hInstance     = instance;
    wndclass.hIcon         = LoadIcon(instance,TEXT("B65C02_ICON"));
    wndclass.hCursor       = LoadCursor(0,IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndclass.lpszClassName = TEXT("65C02FRAME");
    RegisterClass(&wndclass);
  }
}

void FrameReleaseDC (HDC dc) {
  SetViewportOrgEx(dc,0,0,NULL);
  ReleaseDC(framewindow,dc);
#ifdef CLIPVIEWPORT
  DeleteObject(clipregion);
#endif
}

