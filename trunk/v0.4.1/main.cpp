/*
   VERSION 0.4.1

   STATUS : [ ~ done , - Partially , * TODO ]
   
        ~ Spotify on Lock & PAUSE
        ~ BSPlayer on Lock & PAUSE
        ~ iTunes bug Fixed
        ~ Foobar2000 pause support & bug fixed
        ~ XMPlay with PAUSE key
        
                  
        * WMP 11 bug   
        * i18n Support
        * MSN , SKYPE Support
        * Fix Memory leaks
        * Separate Lite , Full Version.
     
   Copyright 2009 Umakanthan Chandran (cumakt@gmail.com)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions
   and limitations under the License.
   */

#define _UNICODE
#include <windows.h>
#include <wininet.h>
#include <string>

using namespace std;

#include "tray.h"
#include "iTunesCOMInterface.h"
#include "addons.h"

#include "iTunesCOMInterface_i.c"
#include "sound_control.cpp"

#define WM_WTSSESSION_CHANGE 0x02B1
#define WTS_SESSION_LOGON                  0x5
#define WTS_SESSION_LOGOFF                 0x6
#define WTS_SESSION_LOCK                   0x7
#define WTS_SESSION_UNLOCK                 0x8

#define ID_MON 1
#define ID_SCR 2
#define ID_DISABLE 3
#define ID_INTIME 5
#define ID_TITLE 6
#define ID_INTIME_CHECK 7
#define ID_IPADDRESS 8

#define OPTION_BNCLICK 9
#define OPTION_EXITCLICK 10
#define OPTION_HELPCLICK 11
#define MAX_X 245
#define MAX_Y 145
#define START_X 20

#define ON_HOTKEY 670
#define OFF_HOTKEY 671
#define PAUSE_HOTKEY 672
#define AWAY_MESSAGE "Away"

#define WM_BSP_CMD WM_USER+2
#define BSP_Play 20
#define BSP_Pause 21

HINSTANCE g_hinst;
HWND o1,o2,o3,b1,g1,intime,hwndButton,hwndExitButton,hwndHelpButton;
HDC hdc;
HBRUSH hbr;

int ID_GLOBAL_FLAG = 0;
int ID_WEEKEND_FLAG = 0;
int AUTO_START_FLAG = 0;
int MEDIA_CHECK_FLAG = 0;
int MESSENGER_CHECK_FLAG = 0;
int MUTE_CHECK_FLAG = 0;
int ID_CHECK_DISABLE = 0;
int MINIMIZE_CHECK_FLAG = 0;
int PAUSE_CHECK_FLAG = 0;
int YAHOO_USER_FLAG = 0;
int AWAY_MESSAGE_FLAG = 0;
int WMP_CLASSIC_RUN = 0;
int FOOBAR_RUN = 0;

bool flagFirstTime = false;
bool pauseKey = true;
char *LocalTimeHHMM;
char *ipAddress;
SYSTEMTIME st;
HFONT font,font2;
char sBuffer[1000];
DWORD ret;

bool isITunesPause = false;
bool GetLocalTimeHHMM(char *LocalTimeHHMM);
void XMLHTTPRequest(char *requesturl);

/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

/*  Make the class name into a global variable  */
char szClassName[ ] = "MonitorES";
char szTitleText[ ] = "Monitor ES 0.4.1";

char * originalMessage;
char WinName[60];
char YahooName[32];
char AwayMessage[128];

int (__stdcall * WorkStationStatus)();


void controlVLC()
{
   HINTERNET Internet = InternetOpen("URL", INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);
   HWND statusText;
   int status = 0;
   if(!Internet) { }   
   HINTERNET Url = InternetOpenUrl(Internet, "http://localhost:8080/requests/status.xml?command=pl_status", 0, 0, 0, 0);
   if(!Url){ }
   char Buffer[64*1024];  
   DWORD ReadLen = 0, CurrentPos = 0;  
   do
   {
      InternetReadFile(Url, (void *)(Buffer + CurrentPos), sizeof(Buffer) - CurrentPos, &ReadLen);
      CurrentPos += ReadLen;
   }  
   while(ReadLen);   
   Buffer[CurrentPos] = 0;
   
   std::string strText = Buffer;
   size_t pause,play;
   pause = strText.find("paused");
   if (pause !=string::npos) {
         status = 1;                   
   }
   play = strText.find("playing");
   if(play != string::npos) {
         status = 2;   
   }
   switch(status) {
    case 1:
          XMLHTTPRequest("http://localhost:8080/requests/status.xml?command=pl_play");
          break;
    case 2:
          XMLHTTPRequest("http://localhost:8080/requests/status.xml?command=pl_pause");
          break; 
   }
   
}        
    
void XMLHTTPRequest(char *requesturl)
{
   HINTERNET Internet = InternetOpen("URL", INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);
   HWND statusText;
   if(!Internet) { }   
   HINTERNET Url = InternetOpenUrl(Internet, requesturl, 0, 0, 0, 0);
   if(!Url){ }
   char Buffer[64*1024];  
   DWORD ReadLen = 0, CurrentPos = 0;  
   do
   {
      InternetReadFile(Url, (void *)(Buffer + CurrentPos), sizeof(Buffer) - CurrentPos, &ReadLen);
      CurrentPos += ReadLen;
   }  
   while(ReadLen);   
   Buffer[CurrentPos] = 0;   
}   

void controlSound(bool flagMute)
{
   if(MUTE_CHECK_FLAG) 
   {
    Mixer mixer;
    if( mixer.init() && mixer.GetMuteControl() )
     {   
       if(flagMute) mixer.SetMute(true);
       else mixer.SetMute(false);
     }      
    mixer.close();
   }
}


/* Main Program call */
void ControlTunes(int flag)
{
 HRESULT hRes;
 CoInitialize(0);
 BSTR bstrURL = 0;
 IiTunes *iITunes = 0;  
 hRes = ::CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (PVOID *)&iITunes);
		if(hRes == S_OK && iITunes)
		{
            ITPlayerState iIPlayerState;
            iITunes->get_PlayerState(&iIPlayerState);
            if(flag == 0)
            {
             switch(iIPlayerState)
			 {
			   case ITPlayerStatePlaying:
                     iITunes->Pause();
                     break;
               case ITPlayerStateStopped:
                     iITunes->Play();  
                     break;
			   default:
                     break;
             }	
            }
            else if ( flag == 1) 
            {
              iITunes->Pause();
            }
            else if( flag == 2) 
            {
              iITunes->Play();     
            }
            iITunes->Release();
        }      
  CoUninitialize();
}

HWND GetRunningWindow() {
    // Check if exists an application with the same class name as this application
    HWND hWnd = FindWindow(szClassName, NULL);
    if (IsWindow(hWnd)) {
        HWND hWndPopup = GetLastActivePopup(hWnd);
        if (IsWindow(hWndPopup))
            hWnd = hWndPopup; // Previous instance exists
    }
    else hWnd = NULL; // Previous instance doesnt exist
    return hWnd;
}

void MediaRunning()
{
        //winamp & aimp , MediaMonkey control
        HWND hwndWinamp = FindWindow("Winamp v1.x",NULL); //Finding window
        if (hwndWinamp != NULL) {
            int ret = SendMessage(hwndWinamp, WM_USER, 0, 104);
            switch(ret)
            {
               case 1: // its playing 
                    SendMessage(hwndWinamp,WM_COMMAND, 40046, 1); //pause
                    break;
               case 3: // its paused
                    SendMessage(hwndWinamp,WM_COMMAND, 40045, 1); //play
                    break;
               default:
                    break;        
            }                       
             // Pause/play
        }   
             
        //windows media player control        
        HWND hwndWMP = FindWindow("WMPlayerApp",NULL);
        if (hwndWMP != NULL) { 
            SendMessage(hwndWMP,WM_COMMAND, 0x00004978, 1); // Pause/play
        }
        
        //iTunes control using COM 
        HWND hwndTunes = FindWindow("iTunes",NULL);
        if(hwndTunes != NULL ) {
              ControlTunes(0);         
        }
        
        //1by1 Player
        HWND hwndBY1= FindWindow("1by1WndClass",NULL);
        if(hwndBY1 != NULL)
         {
            SendMessage(hwndBY1,  WM_COMMAND, 0x000001FF, 0x000904A8); // Pause/play   
         }
         
         //Spotify Player
         
         /*
         Copyright (c) 2009, Bj�rge N�ss(PYTIFY)
         
         CMD_PLAYPAUSE   = 917504
	     CMD_MUTE        = 524288
	     CMD_VOLUMEDOWN  = 589824
	     CMD_VOLUMEUP    = 655360
	     CMD_STOP        = 851968
	     CMD_PREVIOUS    = 786432
	     CMD_NEXT        = 720896 
         */
        HWND hwndSP= FindWindow("SpotifyMainWindow",NULL);
         if(hwndSP != NULL)
         {
            SendMessage(hwndSP,  0x0319, 0, 917504);
         } 

        //VLC Player
        HWND hwndVLC = FindWindow("QWidget",NULL);
        if (hwndVLC != NULL) {
            ShowWindow(hwndVLC, SW_RESTORE);
            SetFocus(hwndVLC);
            SetActiveWindow(hwndVLC);
            SendMessage(hwndVLC, WM_KEYDOWN, VK_SPACE, 0);
            SendMessage(hwndVLC, WM_KEYUP, VK_SPACE, 0);
            ShowWindow(hwndVLC, SW_SHOWNOACTIVATE);           
        }
        
        //BSPlayer         
        HWND hwndBS = FindWindow("BSPlayer",NULL);
        if(hwndBS != NULL)
         {
            SendMessage(hwndBS,WM_BSP_CMD,BSP_Pause ,0);
         }
         
       //QuickTime Player
        HWND hwndQTP = FindWindow("QuickTimePlayerMain",NULL);
        if (hwndQTP != NULL) {
            ShowWindow(hwndQTP, SW_RESTORE);
            SetFocus(hwndQTP);
            SetActiveWindow(hwndQTP);
            SendMessage(hwndQTP, WM_KEYDOWN, VK_SPACE, 0);
            SendMessage(hwndQTP, WM_KEYUP, VK_SPACE, 0);
            ShowWindow(hwndQTP, SW_SHOWNOACTIVATE);           
        }
        
        //GOMPlayer
        HWND hwndGOM= FindWindow("GomPlayer1.x",NULL);
        if(hwndGOM != NULL)
         {
            SendMessage(hwndGOM, WM_COMMAND, 0x0001800C, 0); // Pause/play   
         }
        
        //ALShow Player
        HWND hwndALS= FindWindow("ALShowMainWindow",NULL);
        if(hwndALS != NULL)
         {
            SendMessage(hwndALS, WM_COMMAND, 0x00018154, 0); // Pause/play   
         }
          
        //Media Player Classic Player
        HWND hwndWMPC = FindWindow("MediaPlayerClassicW",NULL);
        if (hwndWMPC != NULL) {
            if(WMP_CLASSIC_RUN == 0)                
            {
               SendMessage(hwndWMPC,  WM_COMMAND, 0x0000009B, 0x0004030E); //Play 
               WMP_CLASSIC_RUN = 1;
            }  
            else    
            {
               SendMessage(hwndWMPC,  WM_COMMAND, 0x0000009C, 0x0004030E); //Pause                                             
               WMP_CLASSIC_RUN = 0;
            } 
        }
        
        //SongBird Player
        HWND hwndSB= FindWindow(NULL, "Songbird");
        if (hwndSB != NULL) {
            ShowWindow(hwndSB, SW_RESTORE);
            SetFocus(hwndSB);
            SetActiveWindow(hwndSB);
            SendMessage(hwndSB, WM_KEYDOWN, VK_SPACE, 0);
            SendMessage(hwndSB, WM_KEYUP, VK_SPACE, 0);
            ShowWindow(hwndSB, SW_SHOWNOACTIVATE);           
        }
        
        controlFooBar();
        
        //XMplay
        HWND hwndXMP = FindWindow("XMPLAY-MAIN",NULL);
        if (hwndXMP != NULL) {
            ShowWindow(hwndXMP, SW_RESTORE);
            SetFocus(hwndXMP);
            SetActiveWindow(hwndXMP);
            SendMessage(hwndXMP, WM_KEYDOWN, 0x50, 0);
            SendMessage(hwndXMP, WM_KEYUP, 0x50, 0);
            ShowWindow(hwndXMP, SW_SHOWNOACTIVATE);           
        }
          
}

void pauseMediaRunning()
{
     if (MEDIA_CHECK_FLAG && ID_GLOBAL_FLAG != 2) {
        //winamp & aimp , MediaMonkey control
        HWND hwndWinamp = FindWindow("Winamp v1.x",NULL); //Finding window
        if (hwndWinamp != NULL) {
            int ret = SendMessage(hwndWinamp, WM_USER, 0, 104);
            switch(ret)
            {
               case 1: // its playing 
                    SendMessage(hwndWinamp,WM_COMMAND, 40046, 1); //pause
                    break;
               default:
                    break;        
            }                       
             // Pause/play
        }        
        //windows media player control        
        HWND hwndWMP = FindWindow("WMPlayerApp",NULL);
        if (hwndWMP != NULL) { 
            SendMessage(hwndWMP,WM_COMMAND, 0x00004978, 1); // Pause/play
        }
        //iTunes control using COM 
        HWND hwndTunes = FindWindow("iTunes",NULL);
        if(hwndTunes != NULL ) {
              ControlTunes(1);         
        }

        //Foobar Control
        controlFooBar();  
        
        //1by1 Player
        HWND hwndBY1= FindWindow("1by1WndClass",NULL);
        if(hwndBY1 != NULL)
         {
            SendMessage(hwndBY1,  WM_COMMAND, 0x000001FF, 0x000904A8); // Pause/play   
         }
            
        //WMP Classic
        HWND hwndWMPC = FindWindow("MediaPlayerClassicW",NULL);
         if (hwndWMPC != NULL) {
            SendMessage(hwndWMPC,  WM_COMMAND, 0x0000009C, 0x0004030E); 
         }    
         
         //GOMPlayer
        HWND hwndGOM= FindWindow("GomPlayer1.x",NULL);
        if(hwndGOM != NULL)
         {
            SendMessage(hwndGOM, WM_COMMAND, 0x0001800C, 0); // Pause/play   
         } 
        
        //ALShow Player
        HWND hwndALS= FindWindow("ALShowMainWindow",NULL);
        if(hwndALS != NULL)
         {
            SendMessage(hwndALS, WM_COMMAND, 0x00018154, 0); // Pause/play   
         }
         
         //Spotify
         HWND hwndSP= FindWindow("SpotifyMainWindow",NULL);
         if(hwndSP != NULL)
         {
            SendMessage(hwndSP,  0x0319, 0, 917504);
         } 
         
         //BSPlayer         
        HWND hwndBS = FindWindow("BSPlayer",NULL);
        if(hwndBS != NULL)
         {
            SendMessage(hwndBS,WM_BSP_CMD,BSP_Pause ,0);
         }   
         
        //vlc 
        HWND hwndVLC = FindWindow("QWidget",NULL);
        if (hwndVLC != NULL) {      
             controlVLC();
        }
    }  
}

void resetMediaRunning() {
  
    if (MEDIA_CHECK_FLAG && ID_GLOBAL_FLAG != 2) {
        //winamp & aimp control
        HWND hwndWinamp = FindWindow("Winamp v1.x",NULL); //Finding window
        if (hwndWinamp != NULL) {
            int ret = SendMessage(hwndWinamp, WM_USER, 0, 104);
            switch(ret)
            {
               case 3: // its paused
                    SendMessage(hwndWinamp,WM_COMMAND, 40045, 1); //play
                    break;
               default:
                    break;        
            }                       
             // Pause/play
        }
        
        //windows media player control        
        HWND hwndWMP = FindWindow("WMPlayerApp",NULL);
        if (hwndWMP != NULL) {
            SendMessage(hwndWMP,WM_COMMAND, 0x00004978, 1); // Pause/play
        }
        //iTunes control using COM 
        HWND hwndTunes = FindWindow("iTunes",NULL);
        if(hwndTunes != NULL ) {
              ControlTunes(2);         
        }
      
        //Foobar Control
        controlFooBar(); 
         
        //1by1 player          
        HWND hwndBY1= FindWindow("1by1WndClass",NULL);
        if(hwndBY1 != NULL)
         {
            SendMessage(hwndBY1,  WM_COMMAND, 0x000001FF, 0x000904A8); // Pause/play   
         }
         
         //WMP Classic
        HWND hwndWMPC = FindWindow("MediaPlayerClassicW",NULL);
         if (hwndWMPC != NULL) {
             SendMessage(hwndWMPC,  WM_COMMAND, 0x0000009B, 0x0004030E); //Play 
         } 
         
         //GOMPlayer
        HWND hwndGOM= FindWindow("GomPlayer1.x",NULL);
        if(hwndGOM != NULL)
         {
            SendMessage(hwndGOM, WM_COMMAND, 0x0001800C, 0); // Pause/play   
         }
 
        //ALShow Player
        HWND hwndALS= FindWindow("ALShowMainWindow",NULL);
        if(hwndALS != NULL)
         {
            SendMessage(hwndALS, WM_COMMAND, 0x00018154, 0); // Pause/play   
         }
         
        //Spotify
         HWND hwndSP= FindWindow("SpotifyMainWindow",NULL);
         if(hwndSP != NULL)
         {
            SendMessage(hwndSP,  0x0319, 0, 917504);
         }  
         
         //BSPlayer         
        HWND hwndBS = FindWindow("BSPlayer",NULL);
        if(hwndBS != NULL)
         {
            SendMessage(hwndBS,WM_BSP_CMD,BSP_Pause ,0);
         }
         
        //vlc 
        HWND hwndVLC = FindWindow("QWidget",NULL);
        if (hwndVLC != NULL) {      
             controlVLC();
        }
    }
}

//Copyright Viktor Brange AKA Vikke
//You are allowed to change the code :P
//And don't forget to link to CustomizeTalk.com
//Thanks to Wumpus who made ChangeStatus for VB.

long SendMouseEvent(HWND hwnd, long Msg, int X, int Y) {
    LPARAM lParam =
        MAKELPARAM(X * GetDeviceCaps(GetDC(NULL), LOGPIXELSX) / 1440,
                   Y * GetDeviceCaps(GetDC(NULL), LOGPIXELSY) / 1440);

    return SendMessage(hwnd, Msg, 0, lParam);
}



char * GetCaption(HWND hwnd) {
    long Length = SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
    char* Caption = new char[Length + 1];
    long CharCount = SendMessage(hwnd, WM_GETTEXT, Length, (LPARAM)Caption);
    Caption[CharCount] = '\0';
    char * ret(Caption);
    delete [] Caption;
    return ret;
}



void GTalkChangeStatus(char * strText, bool Polygamy) {
    HWND hwndMain = NULL;
    HWND hwndMainView = NULL;
    HWND hwndStatusView2 = NULL;
    HWND hwndRichEdit = NULL;
    HWND hwndSidebar = NULL;
    HWND hwndPluginHost = NULL;
    HWND hwndATL = NULL;

    do {

        // Find Google Talk window
        hwndMain = FindWindowEx(NULL, hwndMain,
                                TEXT("Google Talk - Google Xmpp Client GUI Window"), NULL);

        if (hwndMain == NULL) {

            //Check for sidebar
            hwndSidebar = FindWindowEx(NULL, hwndSidebar, TEXT("_GD_Sidebar"), NULL);
            if (hwndSidebar == NULL)
                break;
            hwndPluginHost = FindWindowEx(hwndSidebar, NULL, TEXT("PluginHost"), NULL);
            if (hwndPluginHost == NULL)
                break;

            hwndATL = FindWindowEx(hwndPluginHost, NULL, TEXT("ATL:017DC0C0"), NULL);
            if (hwndATL == NULL)
                break;

            hwndMain = FindWindowEx(hwndATL, NULL,
                                    TEXT("Google Talk - Google Xmpp Client GUI Window"), NULL);
            if (hwndMain == NULL)
                break;
        }

        //Find status field
        hwndMainView = FindWindowEx(hwndMain, NULL, TEXT("Main View"), NULL);
        if (hwndMainView == NULL)
            break;

        hwndStatusView2 = FindWindowEx(hwndMainView, NULL, TEXT("Status View 2"), NULL);
        if (hwndStatusView2 == NULL)
            break;

        hwndRichEdit = FindWindowEx(hwndStatusView2, NULL, TEXT("RichEdit20W"), NULL);
        if (hwndRichEdit == NULL)
            break;

        originalMessage =  GetCaption(hwndRichEdit);
        //Change status if not empty
        if (originalMessage != strText) {
            SendMouseEvent(hwndStatusView2, WM_LBUTTONDOWN, 350, 400);
            SendMouseEvent(hwndStatusView2, WM_LBUTTONUP, 350, 400);
            SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) strText);
            SendMessage(hwndStatusView2, WM_IME_KEYDOWN, VK_RETURN, 0);
        }

        //Application->ProcessMessages();

        //Do this multiple times
        if ( Polygamy == false)
            break;
    }
    while (true);
}

BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam)
{
     HWND hParent = (HWND)lParam;
     int txtLen;
     int len = GetWindowTextLength(hwndChild);
     if(len > 0)
            {
             char* buf;
             buf = (char*)GlobalAlloc(GPTR, len + 1);
             GetWindowText(hwndChild, buf, len + 1);
             
             if (strlen(buf) > 0) {
             //int index = SendDlgItemMessage(hParent, IDC_MAIN_LIST, LB_ADDSTRING,0, (LPARAM)buf);
             }
      GlobalFree((HANDLE)buf);
      }   
    return TRUE;    
}

void YahooStatusSet(LPCTSTR szMessage , LPTSTR szProfile) {
     
    //Copyright (C) GPL by Khoa Nguyen <instcode (at) gmail.com>
    
    HKEY hRootKey;       
    TCHAR lpszKeyPath[256];
    wsprintf(lpszKeyPath, "Software\\yahoo\\pager\\profiles\\%s\\Custom Msgs\\", szProfile);
    RegOpenKeyEx(HKEY_CURRENT_USER, lpszKeyPath, 0, KEY_READ | KEY_SET_VALUE, &hRootKey); 
    if (hRootKey != NULL) {
		// YM 8.1.0.xxx(-)
		RegSetValueEx(hRootKey, "1", 0, REG_SZ, (LPBYTE) szMessage, strlen(szMessage) + 1);
		// YM 9.0.0.xxx(+)
		RegSetValueEx(hRootKey, "1_W", 0, REG_SZ, (LPBYTE) szMessage, strlen(szMessage) + 1);
		// Update the combo box..
		HWND hWnd = ::FindWindow(TEXT("YahooBuddyMain"), NULL);
		if (hWnd != NULL) {
			/***************************************
			 Known WM_COMMAND, WPARAM (MS.Spy++, http://yayak.wordpress.com/2007/05/27/yahoo-messenger-idle-status-by-delphi/)
				376 : Available[N]
				---
				377 : Be Right Back[B]
				378 : Busy[B]
				379 : Not at Home[B] - Hidden
				380 : Not at My Desk[B]
				381 : Not in the Office[B] - Hidden
				382 : On the Phone[B]
				383 : On Vacation[B] - Hidden
				384 : Out To Lunch[B] - Hidden
				385 : Stepped Out[B]
				---
				388 : custom_mesg_1
				389 : custom_mesg_2
				390 : custom_mesg_3
				391 : custom_mesg_4
				392 : custom_mesg_5
			****************************************/
			::SendMessage(hWnd, WM_COMMAND, 388, 0);
		}
		RegCloseKey(hRootKey);
	}
     
}

void controlMessengers() {
    if (MESSENGER_CHECK_FLAG) {
    switch(AWAY_MESSAGE_FLAG)
     {
      case 0:
           if(YAHOO_USER_FLAG == 1)                            
             YahooStatusSet(AWAY_MESSAGE, TEXT(YahooName));                
           GTalkChangeStatus(AWAY_MESSAGE, true);                 
           break;
      case 1:
           if(YAHOO_USER_FLAG == 1)                            
             YahooStatusSet(TEXT(AwayMessage), TEXT(YahooName));
           GTalkChangeStatus(TEXT(AwayMessage), true);                            
           break;                   
      }     
   }
}

void restoreMessengers() {
    if (MESSENGER_CHECK_FLAG) {
         //YAHOO        
         if(YAHOO_USER_FLAG == 1)
           YahooStatusSet("", TEXT(YahooName)); 
         // GOOGLE TALK
         GTalkChangeStatus("",true);
    }
}

void YahooMessengerSettings()
{
    HKEY hKey; 
    DWORD size1 = sizeof(YahooName);
    DWORD Type; 
    
    RegOpenKey(HKEY_CURRENT_USER,"Software\\MonitorES",&hKey);
    if (ERROR_SUCCESS == RegQueryValueEx( hKey, TEXT("YUserName"), NULL, &Type, (LPBYTE)YahooName,&size1)) {
        if(StringLength(YahooName) != 0 && StringLength(YahooName) > 1)
         {
            YAHOO_USER_FLAG = 1;                          
         }                          
    }
    
    RegCloseKey(hKey);     
}

void AwayMessageSettings()
{
    HKEY hKey; 
    DWORD size1 = sizeof(AwayMessage);
    DWORD Type; 
    
    RegOpenKey(HKEY_CURRENT_USER,"Software\\MonitorES",&hKey);
    if (ERROR_SUCCESS == RegQueryValueEx( hKey, TEXT("AwayMsg"), NULL, &Type, (LPBYTE)AwayMessage,&size1)) {
                      
        if(StringLength(AwayMessage) != 0 && StringLength(AwayMessage) > 1)
         {
            AWAY_MESSAGE_FLAG = 1;                          
         }                          
    }
    
    RegCloseKey(hKey);     
}

void LoadRegistrySettings() {
    HKEY hKey;
    char rgValue[2];
    DWORD size1 = sizeof(rgValue);
    DWORD Type;
    char pathValue[MAX_PATH];
    DWORD size2 = sizeof(pathValue);
    
    RegOpenKey(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Run",&hKey);
    if (ERROR_SUCCESS == RegQueryValueEx( hKey, TEXT("MonitorES"), NULL, &Type, (LPBYTE)pathValue,&size2)) {
       AUTO_START_FLAG = 1;
    }
        
    RegOpenKey(HKEY_CURRENT_USER,"Software\\MonitorES",&hKey);
    if (ERROR_SUCCESS == RegQueryValueEx( hKey, TEXT("MediaCheck"), NULL, &Type, (LPBYTE)rgValue,&size1)) {
        if (rgValue[0] == 49) {
            MEDIA_CHECK_FLAG = 1;
        }
    }
    
    RegOpenKey(HKEY_CURRENT_USER,"Software\\MonitorES",&hKey);
    if (ERROR_SUCCESS == RegQueryValueEx( hKey, TEXT("MessengerCheck"), NULL, &Type, (LPBYTE)rgValue,&size1)) {
        if (rgValue[0] == 49) {
            MESSENGER_CHECK_FLAG = 1;
        }
    }
    
    RegOpenKey(HKEY_CURRENT_USER,"Software\\MonitorES",&hKey);
    if (ERROR_SUCCESS == RegQueryValueEx( hKey, TEXT("SoundCheck"), NULL, &Type, (LPBYTE)rgValue,&size1)) {
        if (rgValue[0] == 49) {
            MUTE_CHECK_FLAG = 1;
        }
    }
    
    RegOpenKey(HKEY_CURRENT_USER,"Software\\MonitorES",&hKey);
    if (ERROR_SUCCESS == RegQueryValueEx( hKey, TEXT("MinimizeCheck"), NULL, &Type, (LPBYTE)rgValue,&size1)) {
        if (rgValue[0] == 49) {
            MINIMIZE_CHECK_FLAG = 1;
        }
    }
    
    RegOpenKey(HKEY_CURRENT_USER,"Software\\MonitorES",&hKey);
    if (ERROR_SUCCESS == RegQueryValueEx( hKey, TEXT("PauseHotKeyCheck"), NULL, &Type, (LPBYTE)rgValue,&size1)) {
        if (rgValue[0] == 49) {
            PAUSE_CHECK_FLAG = 1;
        }
    }    
        
    RegCloseKey(hKey);
}


void SaveRegistrySettings(HWND hwnd) {
    HKEY hKey;
    LPCTSTR sk = TEXT("SOFTWARE\\MonitorES");
    LONG setRes;
    LPCTSTR value;
    LPCTSTR data;
    
    LONG openRes = RegCreateKeyEx(
                       HKEY_CURRENT_USER,
                       sk,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE,
                       NULL,
                       &hKey,
                       NULL);
    
    
                   
    if (openRes==ERROR_SUCCESS) {                                
       
        value = TEXT("MediaCheck");
        data = TEXT("0");    
        if (MEDIA_CHECK_FLAG) data = TEXT("1");
        setRes = RegSetValueEx (hKey, value, 0, REG_SZ, (LPBYTE)data, strlen(data)+1);

        value = TEXT("MessengerCheck");
        data = TEXT("0");
        if (MESSENGER_CHECK_FLAG) data = TEXT("1");
        setRes = RegSetValueEx (hKey, value, 0, REG_SZ, (LPBYTE)data, strlen(data)+1);
        
        value = TEXT("SoundCheck");
        data = TEXT("0");
        if (MUTE_CHECK_FLAG) data = TEXT("1");
        setRes = RegSetValueEx (hKey, value, 0, REG_SZ, (LPBYTE)data, strlen(data)+1);
        
        value = TEXT("MinimizeCheck");
        data = TEXT("0");
        if (MINIMIZE_CHECK_FLAG) data = TEXT("1");
        setRes = RegSetValueEx (hKey, value, 0, REG_SZ, (LPBYTE)data, strlen(data)+1);

        value = TEXT("PauseHotKeyCheck");
        data = TEXT("0");
        if (PAUSE_CHECK_FLAG) data = TEXT("1");
        setRes = RegSetValueEx (hKey, value, 0, REG_SZ, (LPBYTE)data, strlen(data)+1);        
        
    }
    if(PAUSE_CHECK_FLAG == 0)  
    {
       UnregisterHotKey(NULL, PAUSE_HOTKEY);              
    } 
    else 
    {
       RegisterHotKey(NULL,PAUSE_HOTKEY, 0 ,0x13); 
    }
    
    //saving yahoo messenger name
    char user[32];
    GetDlgItemText(hwnd, ID_YAHOO_USERNAME, user, 64);
    int size = StringLength(user);
    if(size != 0) 
     {
       value = TEXT("YUserName");
       data = TEXT(user);
       setRes = RegSetValueEx (hKey, value, 0, REG_SZ, (LPBYTE)data, strlen(data)+1);     
     }
     else {
       value = TEXT("YUserName");
       data = TEXT("0");
       setRes = RegSetValueEx (hKey, value, 0, REG_SZ, (LPBYTE)data, strlen(data)+1); 
     }    
     
    char awaymsg[128];
    GetDlgItemText(hwnd, ID_CUSTOM_MESSAGE, awaymsg, 128);
    size = StringLength(awaymsg);
    if(size != 0) 
     {
       value = TEXT("AwayMsg");
       data = TEXT(awaymsg);
       strcpy(AwayMessage,awaymsg);
       AWAY_MESSAGE_FLAG = 1;
       setRes = RegSetValueEx (hKey, value, 0, REG_SZ, (LPBYTE)data, strlen(data)+1);     
     }
     else {
       value = TEXT("AwayMsg");
       data = TEXT("0");
       AwayMessage[0] = ' ';
       AWAY_MESSAGE_FLAG = 0;       
       setRes = RegSetValueEx (hKey, value, 0, REG_SZ, (LPBYTE)data, strlen(data)+1); 
     }     
         
   RegCloseKey(hKey); 
}

void auto_start(bool val) {
    try {
        if (val) {
            TCHAR szPath[MAX_PATH];
            GetModuleFileName(NULL,szPath,MAX_PATH);
            HKEY newValue;
            RegOpenKey(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Run",&newValue);
            RegSetValueEx(newValue,"MonitorES",0,REG_SZ,(LPBYTE)szPath,sizeof(szPath));
            RegCloseKey(newValue);
            throw 1;
        }
        else {
            HKEY newValue;
            RegOpenKey(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Run",&newValue);
            RegDeleteValue(newValue,"MonitorES");
        }
    }
    catch (int i) {}

}

//MAIN 
void SaveEnergyShort() {
    Sleep(1000); // Eliminate user's interaction for 500 ms
    switch(ID_GLOBAL_FLAG)
    {
     case 0: 
            SendMessage(HWND_BROADCAST, WM_SYSCOMMAND,SC_MONITORPOWER, (LPARAM) 2);
            ID_CHECK_DISABLE = 0;
            break;
     case 1:
            SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_SCREENSAVE, (LPARAM) 0);
            ID_CHECK_DISABLE = 0;
            break;
     case 2: ID_CHECK_DISABLE = 1; break;
     default: break;              
    }     
}

//main function
int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nFunsterStil)

{

    HWND hwnd,hCheckWnd;        /* This is the handle for our window */
    MSG messages;        /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */
    
    hCheckWnd = GetRunningWindow();
    
    if (hCheckWnd) { // hCheckWnd != NULL -> Previous instance exists
        MessageBox(hCheckWnd, TEXT("The program is already running"), TEXT("Info"), MB_OK);
        SetForegroundWindow(hCheckWnd); // Activate it &
        if (IsIconic(hCheckWnd))
             ShowWindow(hCheckWnd, SW_SHOW); // restore it
        return FALSE;       // Exit program
    }
    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;       /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;        /* No menu */
    wincl.cbClsExtra = 0;        /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;        /* structure or the window instance */
    /* Use Windows's default color as the background of the window */
    //wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
    wincl.hbrBackground = GetSysColorBrush(COLOR_3DFACE);

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 0;

    int desktopwidth=GetSystemMetrics(SM_CXSCREEN);
    int desktopheight=GetSystemMetrics(SM_CYSCREEN);


    /* The class is registered, let's create the program*/
    hwnd = CreateWindowEx (
               0,         /* Extended possibilites for variation */
               szClassName,        /* Classname */
               szTitleText,        /* Title Text */
               WS_OVERLAPPEDWINDOW , /* default window */
               CW_USEDEFAULT,       /* Windows decides the position */
               CW_USEDEFAULT,       /* where the window ends up on the screen */
               MAX_X,        /* The programs width */
               MAX_Y,        /* and height in pixels */
               HWND_DESKTOP,        /* The window is a child-window to desktop */
               NULL,         /* No menu */
               hThisInstance,       /* Program Instance handler */
               NULL         /* No Window Creation data */
           );

    HMENU hmenu= GetSystemMenu(hwnd,FALSE);

    DeleteMenu(hmenu,SC_MAXIMIZE,MF_BYCOMMAND );

    ShowWindow (hwnd, nFunsterStil);

    RegisterSession(hwnd);

    RegisterHotKey(NULL,OFF_HOTKEY,MOD_CONTROL,0x71);
    
    RegisterHotKey(NULL,ON_HOTKEY,MOD_CONTROL,0x72);

    if(PAUSE_CHECK_FLAG == 1)
    {
      RegisterHotKey(NULL,PAUSE_HOTKEY, 0 ,0x13);
    }  
    
    bool gRet;

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while ((gRet = GetMessage (&messages, NULL, 0, 0)) != 0 ) {
        if (gRet == -1) {
            UnregisterSession(hwnd);
            OnDestroyTray();
            PostQuitMessage (0);
        }
        else {
           /* Translate virtual-key messages into character messages */
            TranslateMessage(&messages);
            /* Send message to WindowProcedure */
            DispatchMessage(&messages);            
            //-----------------------------------------------------------------
            if (messages.message == WM_HOTKEY && 
               (LOWORD(messages.lParam)==(MOD_CONTROL)) 
               && (HIWORD(messages.lParam)== 0x71)) {           
                  SaveEnergyShort();
            } 
            else if(messages.message == WM_HOTKEY && 
               (LOWORD(messages.lParam)==(MOD_CONTROL)) 
               && (HIWORD(messages.lParam)== 0x72)) {                                                       
                  SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, (LPARAM) -1);  
            }    
            else if(messages.message == WM_HOTKEY && (HIWORD(messages.lParam)== 0x13))
             { 
                  MediaRunning();                  
             }                                 
        }
    }
    UnregisterHotKey(NULL, OFF_HOTKEY);
    UnregisterHotKey(NULL, ON_HOTKEY);    
    UnregisterHotKey(NULL, PAUSE_HOTKEY);

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}

BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    switch (Message) {
    case WM_INITDIALOG:
        if (AUTO_START_FLAG) { 
            SendDlgItemMessage(hwnd, IDCHECK_START, BM_SETCHECK, BST_CHECKED, 0);
         }                   
        else {
            SendDlgItemMessage(hwnd, IDCHECK_START, BM_SETCHECK, BST_UNCHECKED, 0);
        }   
        if (MEDIA_CHECK_FLAG) {
            SendDlgItemMessage(hwnd, IDMEDIA_DISABLE, BM_SETCHECK, BST_CHECKED, 0);
        }
        else {
            SendDlgItemMessage(hwnd, IDMEDIA_DISABLE, BM_SETCHECK, BST_UNCHECKED, 0);
        }
        if (MESSENGER_CHECK_FLAG) {
            SendDlgItemMessage(hwnd, IDMESSENGER, BM_SETCHECK, BST_CHECKED, 0);
        }
        else {
            SendDlgItemMessage(hwnd, IDMESSENGER, BM_SETCHECK, BST_UNCHECKED, 0);
        }
        if(!IsWindowsVista())
        {
          if(MUTE_CHECK_FLAG) {
            SendDlgItemMessage(hwnd, IDSOUND_CONTROL, BM_SETCHECK, BST_CHECKED, 0);            
         }
          else {
            SendDlgItemMessage(hwnd, IDSOUND_CONTROL, BM_SETCHECK, BST_UNCHECKED, 0);
          }
        } 
        else {
            EnableWindow(GetDlgItem(hwnd, IDSOUND_CONTROL), FALSE);
        }
        if(MINIMIZE_CHECK_FLAG) {
            SendDlgItemMessage(hwnd, IDCHECK_MINIMIZE, BM_SETCHECK, BST_CHECKED, 0);            
        }
        else {
            SendDlgItemMessage(hwnd, IDCHECK_MINIMIZE, BM_SETCHECK, BST_UNCHECKED, 0);
        }
        if(PAUSE_CHECK_FLAG) {
            SendDlgItemMessage(hwnd, ID_PAUSE_HOT, BM_SETCHECK, BST_CHECKED, 0);            
        }
        else {
            SendDlgItemMessage(hwnd, ID_PAUSE_HOT, BM_SETCHECK, BST_UNCHECKED, 0);
        }
        
        if(YAHOO_USER_FLAG == 1)
         {
           SetDlgItemText(hwnd, ID_YAHOO_USERNAME ,TEXT(YahooName));  
         }  
        
         if(AWAY_MESSAGE_FLAG == 1)
         {
           SetDlgItemText(hwnd, ID_CUSTOM_MESSAGE ,TEXT(AwayMessage));  
         }  
           
              
        return TRUE;
        
    case WM_DESTROY:
        EndDialog(hwnd, 0);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCHECK_START:
            if (SendDlgItemMessage(hwnd, IDCHECK_START, BM_GETCHECK, 0, 0)== BST_UNCHECKED) {
                SendDlgItemMessage(hwnd, IDCHECK_START, BM_SETCHECK, BST_CHECKED, 0);
                AUTO_START_FLAG = 1;
            }
            else {
                SendDlgItemMessage(hwnd, IDCHECK_START, BM_SETCHECK, BST_UNCHECKED, 0);
                AUTO_START_FLAG = 0;
            }
            break;
        case IDMEDIA_DISABLE:
            if (SendDlgItemMessage(hwnd, IDMEDIA_DISABLE, BM_GETCHECK, 0, 0)== BST_UNCHECKED) {
                SendDlgItemMessage(hwnd, IDMEDIA_DISABLE, BM_SETCHECK, BST_CHECKED, 0);
                MEDIA_CHECK_FLAG = 1;
            }
            else {
                SendDlgItemMessage(hwnd, IDMEDIA_DISABLE, BM_SETCHECK, BST_UNCHECKED, 0);
                MEDIA_CHECK_FLAG = 0;
            }
            break;
        case IDMESSENGER:
            if (SendDlgItemMessage(hwnd, IDMESSENGER, BM_GETCHECK, 0, 0)== BST_UNCHECKED) {
                SendDlgItemMessage(hwnd, IDMESSENGER, BM_SETCHECK, BST_CHECKED, 0);
                MESSENGER_CHECK_FLAG = 1;
            }
            else {
                SendDlgItemMessage(hwnd, IDMESSENGER, BM_SETCHECK, BST_UNCHECKED, 0);
                MESSENGER_CHECK_FLAG = 0;
            }
            break;
            
        case  IDSOUND_CONTROL:
            if (SendDlgItemMessage(hwnd, IDSOUND_CONTROL, BM_GETCHECK, 0, 0)== BST_UNCHECKED) {
                SendDlgItemMessage(hwnd, IDSOUND_CONTROL, BM_SETCHECK, BST_CHECKED, 0);
                MUTE_CHECK_FLAG = 1;
            }
            else {
                SendDlgItemMessage(hwnd, IDSOUND_CONTROL, BM_SETCHECK, BST_UNCHECKED, 0);
                MUTE_CHECK_FLAG = 0;
            }
            break;  
        case  IDCHECK_MINIMIZE:
            if (SendDlgItemMessage(hwnd, IDCHECK_MINIMIZE, BM_GETCHECK, 0, 0)== BST_UNCHECKED) {
                SendDlgItemMessage(hwnd, IDCHECK_MINIMIZE, BM_SETCHECK, BST_CHECKED, 0);
                MINIMIZE_CHECK_FLAG = 1;
            }
            else {
                SendDlgItemMessage(hwnd, IDCHECK_MINIMIZE, BM_SETCHECK, BST_UNCHECKED, 0);
                MINIMIZE_CHECK_FLAG = 0;
            }
            break;    
        case  ID_PAUSE_HOT:
            if (SendDlgItemMessage(hwnd, ID_PAUSE_HOT, BM_GETCHECK, 0, 0)== BST_UNCHECKED) {
                SendDlgItemMessage(hwnd, ID_PAUSE_HOT, BM_SETCHECK, BST_CHECKED, 0);
                PAUSE_CHECK_FLAG = 1;
            }
            else {
                SendDlgItemMessage(hwnd, ID_PAUSE_HOT, BM_SETCHECK, BST_UNCHECKED, 0);
                PAUSE_CHECK_FLAG = 0;
            }
            break;      
        case ID_DIALOGOK:
            if (AUTO_START_FLAG) auto_start(true);
            else auto_start(false);
            SaveRegistrySettings(hwnd);
            EndDialog(hwnd, ID_DIALOGOK);
            break;
        case ID_DIALOGCANCEL:
            EndDialog(hwnd, ID_DIALOGCANCEL);
            break;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}
/*  This function is called by the Windows function DispatchMessage()  */


LRESULT CALLBACK WindowProcedure (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LPMINMAXINFO lpmmi;
    switch (message) {     /* handle the messages */
    case WM_WTSSESSION_CHANGE:
        switch ( wParam ) {
        case WTS_SESSION_LOCK:
           // if(isNotScreenSaverRunning())  
           //  {
                SaveEnergyShort();
           //  }  
            if(ID_CHECK_DISABLE == 0) 
            { 
                pauseMediaRunning();
                controlMessengers();
                controlSound(true);            
            }  
            break;
        case WTS_SESSION_UNLOCK:
            if(ID_CHECK_DISABLE ==0 ) 
            {           
                resetMediaRunning();
                restoreMessengers();
                controlSound(false); 
            }
            break;
        default:
            break;
        }
        break;
    case WM_CREATE: {

        TRAY_Init(hWnd);

        OnTray(wParam);
        
        o1 = CreateWindow(TEXT("button"), TEXT("Monitor Off - Power Saving"),
                          WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON ,
                          START_X, 5, 190, 30, hWnd, (HMENU)ID_MON , g_hinst, NULL);

        o2 = CreateWindow(TEXT("button"), TEXT("Screen Saver On"),
                          WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON ,
                          START_X, 30, 190, 30, hWnd, (HMENU)ID_SCR , g_hinst, NULL);
        
        o3 = CreateWindow(TEXT("button"), TEXT("No Thanks"),
                          WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON ,
                          START_X, 55, 190, 30, hWnd, (HMENU)ID_DISABLE , g_hinst, NULL);                          

        hwndButton = CreateWindow (TEXT("BUTTON"),TEXT("Option"),
                                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_TEXT,
                                   START_X,85,64,24,hWnd,(HMENU)OPTION_BNCLICK, g_hinst,NULL) ;

         //FIX ME: SET Image to Button
         //SendDlgItemMessage(hWnd, OPTION_BNCLICK, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)LoadIcon(g_hinst, MAKEINTRESOURCE(MES_EXIT)));
                                              
        hwndHelpButton = CreateWindow (TEXT("BUTTON"),TEXT("Help"),
                                   WS_CHILD |WS_VISIBLE | BS_PUSHBUTTON | BS_TEXT,
                                   160,85,32,24,hWnd,(HMENU)OPTION_HELPCLICK, g_hinst,NULL) ;
       
        hwndExitButton = CreateWindow (TEXT("BUTTON"),TEXT("Exit"),
                                   WS_CHILD |WS_VISIBLE | BS_PUSHBUTTON | BS_TEXT,
                                   195,85,32,24,hWnd,(HMENU)OPTION_EXITCLICK, g_hinst,NULL) ;
        
        font = CreateFont(15, 0, 0, 0, 550, 0, 0, 0, 0, 0, 0, 0, 0, "Verdana");
        font2 = CreateFont(12, 0, 0, 0, 550, 0, 0, 0, 0, 0, 0, 0, 0, "Verdana");
        
        SendDlgItemMessage(hWnd, ID_MON, WM_SETFONT, (WPARAM)font, TRUE);
        SendDlgItemMessage(hWnd, ID_SCR, WM_SETFONT, (WPARAM)font, TRUE);
        SendDlgItemMessage(hWnd, ID_DISABLE, WM_SETFONT, (WPARAM)font, TRUE);
        SendDlgItemMessage(hWnd, ID_INTIME, WM_SETFONT, (WPARAM)font, TRUE);
        SendDlgItemMessage(hWnd, ID_IPADDRESS, WM_SETFONT, (WPARAM)font, TRUE);

        SendDlgItemMessage(hWnd, OPTION_BNCLICK, WM_SETFONT, (WPARAM)font2, TRUE);
        SendDlgItemMessage(hWnd, OPTION_HELPCLICK, WM_SETFONT, (WPARAM)font2, TRUE);
        SendDlgItemMessage(hWnd, OPTION_EXITCLICK, WM_SETFONT, (WPARAM)font2, TRUE);
        
        CheckDlgButton(hWnd, ID_MON, BST_CHECKED);

        LoadRegistrySettings();
        
        YahooMessengerSettings();    
        
        AwayMessageSettings();       
    }
    break;
    case WM_KEYDOWN :
        if (wParam == VK_ESCAPE) {
            ShowWindow(hWnd, SW_SHOWMINIMIZED);            
        }
        break;
    case WM_SIZE:
        OnSizeTray(wParam,MINIMIZE_CHECK_FLAG); // Minimize/Restore to/from tray
        break;
    case WM_NOTIFYICONTRAY:
        OnNotifyTray(lParam); // Manages message from tray
        return TRUE;
    case WM_COMMAND: {
        if (OnCommandTrayMenu(wParam)) break;
        if (HIWORD(wParam) == BN_CLICKED) {
            switch (LOWORD(wParam)) {
            case ID_MON:
                ID_GLOBAL_FLAG = 0;
                break;
            case ID_SCR:
                ID_GLOBAL_FLAG = 1;
                break;
            case ID_DISABLE:
                 ID_GLOBAL_FLAG = 2;
                 break;    
            case OPTION_BNCLICK:
                DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(ABOUTDLG), hWnd, AboutDlgProc);
                break;
             case OPTION_EXITCLICK:
                  UnregisterSession(hWnd);
                  OnDestroyTray();
                  PostQuitMessage (0);
                  break; 
             case OPTION_HELPCLICK:
                  MessageBox(hWnd,APP_ABOUT,TEXT(" About MonitorES "), MB_ICONINFORMATION);
                  break;                    
            }
        }
    }
    break;
    case WM_RBUTTONDOWN: {
        TRAY_Menu_Show();//load POPUP menu in main window (why?)
    }
    break;
    /*case WM_CLOSE:
        ShowWindow(hWnd, SW_SHOWMINIMIZED); // Minimize/Restore to/from tray
    break;*/
    case WM_DESTROY:
        UnregisterSession(hWnd);
        OnDestroyTray();
        PostQuitMessage (0);  /* send a WM_QUIT to the message queue */
        break;
    case WM_GETMINMAXINFO:
        lpmmi = (LPMINMAXINFO) lParam;
        lpmmi->ptMaxSize.x = MAX_X;
        lpmmi->ptMaxSize.y = MAX_Y;
        lpmmi->ptMaxTrackSize.x = MAX_X;
        lpmmi->ptMaxTrackSize.y = MAX_Y;
        lpmmi->ptMinTrackSize.x = MAX_X;
        lpmmi->ptMinTrackSize.y = MAX_Y;
        return 0;
        break;
    case WM_INITDIALOG:
        hbr = CreateSolidBrush(RGB(0, 0, 0));
    case WM_CTLCOLORSTATIC:
        hdc = (HDC) wParam;
        SetBkMode ( hdc, TRANSPARENT );
        SetBkColor( hdc , (COLORREF)COLOR_BACKGROUND);
        hbr = (HBRUSH)GetStockObject( NULL_BRUSH );
        return (LRESULT)hbr;
        break;
    default: /* for messages that we don't deal with */
        return DefWindowProc (hWnd, message, wParam, lParam);
    }

    return 0;
}
