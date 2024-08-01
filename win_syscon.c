/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// win_syscon.h
#include "q_shared.h"
#include "qcommon.h"
//#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <ctype.h>
#include <windows.h>
#include <commctrl.h>

#define SYSCON_DEFAULT_WIDTH    540
#define SYSCON_DEFAULT_HEIGHT   450

#define COPY_ID         1
#define QUIT_ID         2
#define CLEAR_ID        3

#define ERRORBOX_ID     10
#define ERRORTEXT_ID    11

#define EDIT_ID         100
#define INPUT_ID        101
#define STATUS_ID		102
#define PROGRESS_ID		104
#define STATUS_ID2		103
#ifndef IDI_ICON1
#define IDI_ICON1 108
#endif

#define BUTTON_WIDTH	90
#define BUTTON_HEIGHT	24
#define PROGRESSBAR_XMARGIN 20
// Next default values for new objects
//
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NO_MFC                     1
#define _APS_NEXT_RESOURCE_VALUE        130
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1005
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif

//********************************************************
void Conbuf_AppendTextInternal( const char *pMsg );


typedef struct
{
	HWND hWnd;
	HWND hwndBuffer;
	char consoleBuffer[8192];
	
	HWND hwndButtonClear;
	HWND hwndButtonCopy;
	HWND hwndButtonQuit;
	HWND hwndProgressBar;
	int progressBarPos;
	qboolean progressBarUpdate;
	HWND hwndErrorBox;
	HWND hwndErrorText;
	HWND hwndStatusText;
	char statusText[8192];
	qboolean statusTextUpdate;
	HWND hwndStatusText2;
	char statusText2[8192];
	qboolean statusText2Update;
	HBITMAP hbmLogo;
	HBITMAP hbmClearBitmap;

	HBRUSH hbrEditBackground;
	HBRUSH hbrErrorBackground;

	HFONT hfBufferFont;
	HFONT hfButtonFont;

	HWND hwndInputLine;

	char errorString[80];

	char consoleText[512], returnedText[512];
	int visLevel;
	qboolean quitOnClose;
	int windowWidth, windowHeight;

	WNDPROC SysInputLineWndProc;
	HANDLE threadid;
	qboolean windowCreated;
} WinConData;

static WinConData s_wcd;

void Sys_ProcessPumpedMessages()
{
	char buf[65535];
	int tmp;
	
	if(s_wcd.statusTextUpdate)
	{
		Sys_EnterCriticalSection();
		Q_strncpyz(buf, s_wcd.statusText, sizeof(buf) );
		s_wcd.statusTextUpdate = qfalse;
		Sys_LeaveCriticalSection();
		
		SetWindowText(s_wcd.hwndStatusText, buf);

	}
	if(s_wcd.statusText2Update)
	{	
		Sys_EnterCriticalSection();
		Q_strncpyz(buf, s_wcd.statusText2, sizeof(buf) );
		s_wcd.statusText2Update = qfalse;
		Sys_LeaveCriticalSection();
		
		SetWindowText(s_wcd.hwndStatusText2, buf);
		s_wcd.statusText2[0] = '\0';
	}
	if(s_wcd.progressBarUpdate)
	{
		Sys_EnterCriticalSection();
		tmp = s_wcd.progressBarPos;
		s_wcd.progressBarUpdate = qfalse;
		s_wcd.progressBarPos = 0;
		Sys_LeaveCriticalSection();
		if(s_wcd.hwndProgressBar)
		{
			SendMessageA( s_wcd.hwndProgressBar, PBM_SETPOS, tmp, 0);
		}
	}
	if(s_wcd.consoleBuffer[0])
	{
		Sys_EnterCriticalSection();
		Q_strncpyz(buf, s_wcd.consoleBuffer, sizeof(buf) );
		s_wcd.consoleBuffer[0] = '\0';
		Sys_LeaveCriticalSection();
		
		Conbuf_AppendTextInternal( buf );
		s_wcd.consoleBuffer[0] = '\0';
	}

}


void Sys_EventLoop(){
	MSG msg;
	// pump the message loop
	while ( s_wcd.threadid ) {
		
		if(s_wcd.threadid == 0)
		{
			break;
		}
		
		Sys_ProcessPumpedMessages();
		
		if(!PeekMessageA( &msg, NULL, 0, 0, PM_NOREMOVE ))
		{
			Sleep(5);
			continue;
		}

		if ( !GetMessageA( &msg, NULL, 0, 0 ) ) {
			break;
		}

		TranslateMessage( &msg );
		DispatchMessageA( &msg );
	}
	
	if ( s_wcd.hWnd ) {
		ShowWindow( s_wcd.hWnd, SW_HIDE );
		CloseWindow( s_wcd.hWnd );
		DestroyWindow( s_wcd.hWnd );
		s_wcd.hWnd = 0;
	}
	
}


static LONG WINAPI ConWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	static qboolean s_timePolarity;
	int cx, cy;
	float sx, sy;
	float x, y, w, h;
	int ret;

	switch ( uMsg )
	{
	case WM_SIZE:
		// NERVE - SMF
		cx = LOWORD( lParam );
		cy = HIWORD( lParam );

//		if ( cx < SYSCON_DEFAULT_WIDTH )
//			cx = SYSCON_DEFAULT_WIDTH;
//		if ( cy < SYSCON_DEFAULT_HEIGHT )
//			cy = SYSCON_DEFAULT_HEIGHT;
		
		s_wcd.windowWidth = cx;
		s_wcd.windowHeight = cy;
		
		sx = (float)cx / SYSCON_DEFAULT_WIDTH;
		sy = (float)cy / SYSCON_DEFAULT_HEIGHT;

		x = 8;
		y = 6;
		w = cx - 15;
		h = cy - 150;
		SetWindowPos( s_wcd.hwndBuffer, NULL, x, y, w, h, 0 );

		y = y + h + 15;
		SetWindowPos( s_wcd.hwndButtonCopy, NULL, ((s_wcd.windowWidth - BUTTON_WIDTH) / 2) - 55, y, BUTTON_WIDTH, BUTTON_HEIGHT, 0 );
		SetWindowPos( s_wcd.hwndButtonQuit, NULL, ((s_wcd.windowWidth - BUTTON_WIDTH) / 2) + 55, y, BUTTON_WIDTH, BUTTON_HEIGHT, 0 );
		// -NERVE - SMF

		y += BUTTON_HEIGHT;
		y += 20;
		
		SetWindowPos( s_wcd.hwndStatusText, NULL, PROGRESSBAR_XMARGIN, y, s_wcd.windowWidth - 2*PROGRESSBAR_XMARGIN, BUTTON_HEIGHT, 0 );
		y += 35;
		
		SetWindowPos( s_wcd.hwndStatusText2, NULL, PROGRESSBAR_XMARGIN, y, s_wcd.windowWidth - 2*PROGRESSBAR_XMARGIN, BUTTON_HEIGHT, 0 );
		y += 25;
		if(s_wcd.hwndProgressBar)
		{
			SetWindowPos( s_wcd.hwndProgressBar, NULL, PROGRESSBAR_XMARGIN, y, s_wcd.windowWidth - 2*PROGRESSBAR_XMARGIN, 20, 0 );
		}
		break;
	
		
	case WM_ACTIVATE:
		if ( LOWORD( wParam ) != WA_INACTIVE ) {
			SetFocus( s_wcd.hwndBuffer );
		}
		break;

	case WM_CLOSE:
		if ( s_wcd.quitOnClose )   {
			CoD4UpdateShutdown( 1 );
		} else
		{
			//Sys_ShowConsole( 0, qfalse );
			//Cvar_Set( "viewlog", "0" );
			ShowWindow( s_wcd.hWnd, SW_HIDE );

		}
		return 0;
	case WM_CTLCOLORSTATIC:
		if ( ( HWND ) lParam == s_wcd.hwndBuffer ) {
			SetBkColor( ( HDC ) wParam, RGB( 0x00, 0x00, 0xB0 ) );
			SetTextColor( ( HDC ) wParam, RGB( 0xff, 0xff, 0x00 ) );

#if 0   // this draws a background in the edit box, but there are issues with this
			if ( ( hdcScaled = CreateCompatibleDC( ( HDC ) wParam ) ) != 0 ) {
				if ( SelectObject( ( HDC ) hdcScaled, s_wcd.hbmLogo ) ) {
					StretchBlt( ( HDC ) wParam, 0, 0, 512, 384,
								hdcScaled, 0, 0, 512, 384,
								SRCCOPY );
				}
				DeleteDC( hdcScaled );
			}
#endif
			return ( long ) s_wcd.hbrEditBackground;
		} else if ( ( HWND ) lParam == s_wcd.hwndErrorBox )   {
			if ( s_timePolarity & 1 ) {
				SetBkColor( ( HDC ) wParam, RGB( 0x80, 0x80, 0x80 ) );
				SetTextColor( ( HDC ) wParam, RGB( 0xff, 0x0, 0x00 ) );
			} else
			{
				SetBkColor( ( HDC ) wParam, RGB( 0x80, 0x80, 0x80 ) );
				SetTextColor( ( HDC ) wParam, RGB( 0x00, 0x0, 0x00 ) );
			}
			return ( long ) s_wcd.hbrErrorBackground;
		}
		break;

	case WM_COMMAND:
		if ( wParam == COPY_ID ) {
			SendMessageA( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
			SendMessageA( s_wcd.hwndBuffer, WM_COPY, 0, 0 );
		} else if ( wParam == QUIT_ID )   {
			if ( s_wcd.quitOnClose ) {
				CoD4UpdateShutdown( 1 );
			}
		} else if ( wParam == CLEAR_ID )   {
			SendMessageA( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
			SendMessageA( s_wcd.hwndBuffer, EM_REPLACESEL, FALSE, ( LPARAM ) "" );
			UpdateWindow( s_wcd.hwndBuffer );
		}
		break;
	case WM_CREATE:
//		s_wcd.hbmLogo = LoadBitmap( wc.hInstance, MAKEINTRESOURCE( IDB_BITMAP1 ) );
//		s_wcd.hbmClearBitmap = LoadBitmap( wc.hInstance, MAKEINTRESOURCE( IDB_BITMAP2 ) );
		s_wcd.hbrEditBackground = CreateSolidBrush( RGB( 0x00, 0x00, 0xB0 ) );
		s_wcd.hbrErrorBackground = CreateSolidBrush( RGB( 0x80, 0x80, 0x80 ) );
		SetTimer( hWnd, 1, 1000, NULL );
		break;
	case WM_ERASEBKGND:
#if 0
		HDC hdcScaled;
		HGDIOBJ oldObject;

#if 1   // a single, large image
		hdcScaled = CreateCompatibleDC( ( HDC ) wParam );
		assert( hdcScaled != 0 );

		if ( hdcScaled ) {
			oldObject = SelectObject( ( HDC ) hdcScaled, s_wcd.hbmLogo );
			assert( oldObject != 0 );
			if ( oldObject ) {
				StretchBlt( ( HDC ) wParam, 0, 0, s_wcd.windowWidth, s_wcd.windowHeight,
							hdcScaled, 0, 0, 512, 384,
							SRCCOPY );
			}
			DeleteDC( hdcScaled );
			hdcScaled = 0;
		}
#else   // a repeating brush
		{
			HBRUSH hbrClearBrush;
			RECT r;

			GetWindowRect( hWnd, &r );

			r.bottom = r.bottom - r.top + 1;
			r.right = r.right - r.left + 1;
			r.top = 0;
			r.left = 0;

			hbrClearBrush = CreatePatternBrush( s_wcd.hbmClearBitmap );

			assert( hbrClearBrush != 0 );

			if ( hbrClearBrush ) {
				FillRect( ( HDC ) wParam, &r, hbrClearBrush );
				DeleteObject( hbrClearBrush );
			}
		}
#endif
		return 1;
#endif
		ret = DefWindowProc( hWnd, uMsg, wParam, lParam );
		return ret;
	case WM_TIMER:
		if ( wParam == 1 ) {
			s_timePolarity = !s_timePolarity;
			if ( s_wcd.hwndErrorBox ) {
				InvalidateRect( s_wcd.hwndErrorBox, NULL, FALSE );
			}
		}
		break;
	}
	ret = DefWindowProc( hWnd, uMsg, wParam, lParam );
	return ret;
}

/*
** Conbuf_AppendText
*/

void Conbuf_AppendText( const char *pMsg )
{
	if(!s_wcd.windowCreated)
	{
		return;
	}
	
	while(s_wcd.consoleBuffer[0])
	{
		Sleep(5);
	}
	Sys_EnterCriticalSection();
	Q_strncpyz(s_wcd.consoleBuffer, pMsg, sizeof(s_wcd.consoleBuffer));
	Sys_LeaveCriticalSection();
}

void Conbuf_AppendTextInternal( const char *pMsg ) {
#define CONSOLE_BUFFER_SIZE     16384


	char buffer[CONSOLE_BUFFER_SIZE * 2];
	char *b = buffer;
	const char *msg;
	int bufLen;
	int i = 0;
	static unsigned long s_totalChars;

	//
	// if the message is REALLY long, use just the last portion of it
	//
	if ( strlen( pMsg ) > CONSOLE_BUFFER_SIZE - 1 ) {
		msg = pMsg + strlen( pMsg ) - CONSOLE_BUFFER_SIZE + 1;
	} else
	{
		msg = pMsg;
	}

	//
	// copy into an intermediate buffer
	//
	while ( msg[i] && ( ( b - buffer ) < sizeof( buffer ) - 1 ) )
	{
		if ( msg[i] == '\n' && msg[i + 1] == '\r' ) {
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
			i++;
		} else if ( msg[i] == '\r' )     {
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
		} else if ( msg[i] == '\n' )     {
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
		} else if ( Q_IsColorString( &msg[i] ) )   {
			i++;
		} else
		{
			*b = msg[i];
			b++;
		}
		i++;
	}
	*b = 0;
	bufLen = b - buffer;

	s_totalChars += bufLen;

	//
	// replace selection instead of appending if we're overflowing
	//
	

	if ( s_totalChars > CONSOLE_BUFFER_SIZE ) {
		SendMessageA( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
		s_totalChars = bufLen;
	} else {
		// NERVE - SMF - always append at the bottom of the textbox
		SendMessageA( s_wcd.hwndBuffer, EM_SETSEL, 0xFFFF, 0xFFFF );
	}

	//
	// put this text into the windows console
	//
	SendMessageA( s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff );
	SendMessageA( s_wcd.hwndBuffer, EM_SCROLLCARET, 0, 0 );
	SendMessageA( s_wcd.hwndBuffer, EM_REPLACESEL, 0, (LPARAM) buffer );
	
}


void Sys_ConsoleThread(HINSTANCE hInstance)
{
	
	HDC hDC;
	WNDCLASS wc;
	RECT rect;
	const char *DEDCLASS = LONG_PRODUCT_NAME;
	int y;
#ifdef UPDATE_SERVER        // DHM - Nerve
	const char *WINDOWNAME = "Wolf Update Server";
#else
	const char *WINDOWNAME = CLIENT_WINDOW_TITLE " Console";
#endif
	int nHeight;
	int swidth, sheight;
	int DEDSTYLE = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX | WS_SIZEBOX;
	qboolean show = qtrue;
	
	memset( &wc, 0, sizeof( wc ) );

	wc.style         = 0;
	wc.lpfnWndProc   = (WNDPROC) ConWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon( wc.hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
	wc.hCursor       = LoadCursor( NULL,IDC_ARROW );
	wc.hbrBackground = (void *)COLOR_WINDOW;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = DEDCLASS;

	if ( !RegisterClass( &wc ) ) {
		UnregisterClass( wc.lpszClassName, wc.hInstance );
		if ( !RegisterClass( &wc ) ) {
			return;
		}
	}
	
	
	rect.left = 0;
	rect.right = SYSCON_DEFAULT_WIDTH;
	rect.top = 0;
	rect.bottom = SYSCON_DEFAULT_HEIGHT;
	AdjustWindowRect( &rect, DEDSTYLE, FALSE );

	hDC = GetDC( GetDesktopWindow() );
	swidth = GetDeviceCaps( hDC, HORZRES );
	sheight = GetDeviceCaps( hDC, VERTRES );
	ReleaseDC( GetDesktopWindow(), hDC );

	s_wcd.windowWidth = rect.right - rect.left + 1;
	s_wcd.windowHeight = rect.bottom - rect.top + 1;

	s_wcd.hWnd = CreateWindowExA( 0,
								 DEDCLASS,
								 WINDOWNAME,
								 DEDSTYLE,
								 ( swidth - 600 ) / 2, ( sheight - 450 ) / 2, s_wcd.windowWidth, s_wcd.windowHeight,
								 NULL,
								 NULL,
								 wc.hInstance,
								 NULL );

	if ( s_wcd.hWnd == NULL ) {
		return;
	}

	//
	// create fonts
	//
	hDC = GetDC( s_wcd.hWnd );
	nHeight = -MulDiv( 8, GetDeviceCaps( hDC, LOGPIXELSY ), 72 );

	s_wcd.hfBufferFont = CreateFont( nHeight,
									 0,
									 0,
									 0,
									 FW_LIGHT,
									 0,
									 0,
									 0,
									 DEFAULT_CHARSET,
									 OUT_DEFAULT_PRECIS,
									 CLIP_DEFAULT_PRECIS,
									 DEFAULT_QUALITY,
									 FF_MODERN | FIXED_PITCH,
									 "Courier New" );

	ReleaseDC( s_wcd.hWnd, hDC );

	//
	// create the input line
	//
/*
	s_wcd.hwndInputLine = CreateWindowA( "edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
										ES_LEFT | ES_AUTOHSCROLL,
										6, 400, 528, 20,
										s_wcd.hWnd,
										( HMENU ) INPUT_ID,         // child window ID
										wc.hInstance, NULL );

	
	*/
	//
	// create the buttons
	//


	//
	// create the scrollbuffer
	//
	y = 6;
	s_wcd.hwndBuffer = CreateWindowA( "edit", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
									 ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
									 6, y, 526, 300,
									 s_wcd.hWnd,
									 ( HMENU ) EDIT_ID,             // child window ID
									 wc.hInstance, NULL );
	y += 300;
	SendMessageA( s_wcd.hwndBuffer, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, 0 );	
	
	y += 15;
	
	s_wcd.hwndButtonCopy = CreateWindowA( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
										 ((s_wcd.windowWidth - BUTTON_WIDTH) / 2) - 55, y, BUTTON_WIDTH, BUTTON_HEIGHT,
										 s_wcd.hWnd,
										 ( HMENU ) COPY_ID,         // child window ID
										 wc.hInstance, NULL );
	SendMessageA( s_wcd.hwndButtonCopy, WM_SETTEXT, 0, ( LPARAM ) "copy log" );


	s_wcd.hwndButtonQuit = CreateWindowA( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
										 ((s_wcd.windowWidth - BUTTON_WIDTH) / 2) + 55, y, BUTTON_WIDTH, BUTTON_HEIGHT,
										 s_wcd.hWnd,
										 ( HMENU ) QUIT_ID,         // child window ID
										 wc.hInstance, NULL );
	SendMessageA( s_wcd.hwndButtonQuit, WM_SETTEXT, 0, ( LPARAM ) "abort update" );
	
	y += BUTTON_HEIGHT;
	y += 20;

	s_wcd.hwndStatusText = CreateWindowA("static", NULL, WS_CHILD | WS_VISIBLE,
									  PROGRESSBAR_XMARGIN, y, s_wcd.windowWidth - 2*PROGRESSBAR_XMARGIN, BUTTON_HEIGHT,
									  s_wcd.hWnd, (HMENU)STATUS_ID,
									  wc.hInstance, NULL);
	SetWindowText(s_wcd.hwndStatusText, "");
	
	y += 35;
	
	s_wcd.hwndStatusText2 = CreateWindowA("static", NULL, WS_CHILD | WS_VISIBLE,
									  PROGRESSBAR_XMARGIN, y, s_wcd.windowWidth - 2*PROGRESSBAR_XMARGIN, BUTTON_HEIGHT,
									  s_wcd.hWnd, (HMENU)STATUS_ID2,
									  wc.hInstance, NULL);
	SetWindowText(s_wcd.hwndStatusText2, "");
	
	y += 25;
	if(Sys_IsWindowsVistaAware())
	{
		s_wcd.hwndProgressBar = CreateWindowA(PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE,
										  PROGRESSBAR_XMARGIN, y, s_wcd.windowWidth - 2*PROGRESSBAR_XMARGIN, 20,
										  s_wcd.hWnd, (HMENU)PROGRESS_ID,
										  wc.hInstance, NULL);
		SendMessageA( s_wcd.hwndProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	}
	/*
	s_wcd.SysInputLineWndProc = ( WNDPROC ) SetWindowLong( s_wcd.hwndInputLine, GWL_WNDPROC, ( long ) InputLineWndProc );
	SendMessageA( s_wcd.hwndInputLine, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, 0 );
	*/
	
	if(show == qtrue)
	{
		ShowWindow( s_wcd.hWnd, SW_SHOWDEFAULT );
	}else{
		ShowWindow( s_wcd.hWnd, SW_HIDE );	
	}
	UpdateWindow( s_wcd.hWnd );
	SetForegroundWindow( s_wcd.hWnd );
	SetFocus( s_wcd.hwndBuffer );

	if(show == qtrue)
	{
		s_wcd.visLevel = 1;
	}else{
		s_wcd.visLevel = 0;
	}
	s_wcd.quitOnClose = qtrue;
	s_wcd.windowCreated = qtrue;
	Sys_EventLoop();
	
}


/*
** Sys_CreateConsole
*/
void Sys_CreateConsole( HINSTANCE hInstance ) {

	int i;
	s_wcd.threadid = (void*)-1;
	s_wcd.threadid = CreateThread(NULL, // LPSECURITY_ATTRIBUTES lpsa,
								0, // DWORD cbStack,
								(LPTHREAD_START_ROUTINE)Sys_ConsoleThread, // LPTHREAD_START_ROUTINE lpStartAddr,
								hInstance, // LPVOID lpvThreadParm,
								0, // DWORD fdwCreate,
								NULL);
								
	for(i = 0; i < 100 && !s_wcd.windowCreated; ++i)
	{
		Sleep(30);
	}
		
}

/*
** Sys_DestroyConsole
*/
void Sys_DestroyConsole( void ) {
	
	s_wcd.threadid = 0;

}

void Sys_SetStatusText( const char* statustext, int line )
{

	if(!s_wcd.windowCreated)
	{
		return;
	}
	
	if(line == 0)
	{
		while(s_wcd.statusTextUpdate)
		{
			Sleep(5);
		}
		Sys_EnterCriticalSection();
		Q_strncpyz(s_wcd.statusText, statustext, sizeof(s_wcd.statusText));
		s_wcd.statusTextUpdate = qtrue;
		Sys_LeaveCriticalSection();

	}
	if(line == 1)
	{

		while(s_wcd.statusText2Update)
		{
			Sleep(5);
		}
		Sys_EnterCriticalSection();
		Q_strncpyz(s_wcd.statusText2, statustext, sizeof(s_wcd.statusText2));
		s_wcd.statusText2Update = qtrue;
		Sys_LeaveCriticalSection();

	}

}

void Sys_SetProgress(int step)
{
	
	if(!s_wcd.windowCreated)
	{
		return;
	}

	if(step > 100)
	{
		step = 100;
	}
	if(step < 0)
	{
		step = 0;
	}
	
	Sys_EnterCriticalSection();
	s_wcd.progressBarPos = step;
	s_wcd.progressBarUpdate = qtrue;
	Sys_LeaveCriticalSection();
	
}

HWND Sys_GetHWnd()
{
	return s_wcd.hWnd;
}

void DisableExit()
{
	if(!s_wcd.windowCreated)
	{
		return;
	}
	
	EnableWindow(s_wcd.hwndButtonQuit, qfalse);
	s_wcd.quitOnClose = qfalse;
}