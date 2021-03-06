//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "qcommon.h"
#include "system.h"
#include "system_windows.h"
#include "Common.h"
#include "common_defs.h"
#include "command_buffer.h"
#include "strings.h"
#include "event_queue.h"

#define COPY_ID         1
#define QUIT_ID         2
#define CLEAR_ID        3

#define ERRORBOX_ID     10
#define ERRORTEXT_ID    11

#define EDIT_ID         100
#define INPUT_ID        101

#define SYSCON_DEFAULT_WIDTH    540
#define SYSCON_DEFAULT_HEIGHT   450

#define WIN_COMMAND_HISTORY     64

struct WinConData {
	HWND hWnd;
	HWND hwndBuffer;

	HWND hwndButtonClear;
	HWND hwndButtonCopy;
	HWND hwndButtonQuit;

	HWND hwndErrorBox;
	HWND hwndErrorText;

	HBITMAP hbmLogo;
	HBITMAP hbmClearBitmap;

	HBRUSH hbrEditBackground;
	HBRUSH hbrErrorBackground;

	HFONT hfBufferFont;
	HFONT hfButtonFont;

	HWND hwndInputLine;

	char errorString[ 128 ];

	char consoleText[ 512 ];
	char returnedText[ 512 ];
	int visLevel;
	bool quitOnClose;
	int windowWidth;
	int windowHeight;

	WNDPROC SysInputLineWndProc;
};

static WinConData s_wcd;

static field_t win_consoleField;
static int win_acLength;
static field_t win_historyEditLines[ WIN_COMMAND_HISTORY ];
static int win_nextHistoryLine = 0;
static int win_historyLine = 0;

static LONG WINAPI ConWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	static bool s_timePolarity;

	switch ( uMsg ) {
	case WM_SIZE:
	{
		int cx = LOWORD( lParam );
		int cy = HIWORD( lParam );

//		if (cx < SYSCON_DEFAULT_WIDTH)
//			cx = SYSCON_DEFAULT_WIDTH;
//		if (cy < SYSCON_DEFAULT_HEIGHT)
//			cy = SYSCON_DEFAULT_HEIGHT;

		float sx = ( float )cx / SYSCON_DEFAULT_WIDTH;
		float sy = ( float )cy / SYSCON_DEFAULT_HEIGHT;

		float x = 5;
		float y = 40;
		float w = cx - 15;
		float h = cy - 100;
		SetWindowPos( s_wcd.hwndBuffer, NULL, x, y, w, h, 0 );

		y = y + h + 8;
		h = 20;
		SetWindowPos( s_wcd.hwndInputLine, NULL, x, y, w, h, 0 );

		y = y + h + 4;
		w = 72 * sx;
		h = 24;
		SetWindowPos( s_wcd.hwndButtonCopy, NULL, x, y, w, h, 0 );

		x = x + w + 2;
		SetWindowPos( s_wcd.hwndButtonClear, NULL, x, y, w, h, 0 );

		x = cx - 15 - w;
		SetWindowPos( s_wcd.hwndButtonQuit, NULL, x, y, w, h, 0 );

		s_wcd.windowWidth = cx;
		s_wcd.windowHeight = cy;
	}
	break;

	case WM_ACTIVATE:
		if ( LOWORD( wParam ) != WA_INACTIVE ) {
			SetFocus( s_wcd.hwndInputLine );
		}

		if ( com_viewlog && ( com_dedicated && !com_dedicated->integer ) ) {
			// if the viewlog is open, check to see if it's being minimized
			if ( com_viewlog->integer == 1 ) {
				if ( HIWORD( wParam ) ) {	// minimized flag
					Cvar_Set( "viewlog", "2" );
				}
			} else if ( com_viewlog->integer == 2 ) {
				if ( !HIWORD( wParam ) ) {		// minimized flag
					Cvar_Set( "viewlog", "1" );
				}
			}
		}
		break;

	case WM_CLOSE:
		if ( ( com_dedicated && com_dedicated->integer ) ) {
			char* cmdString = ( char* )Mem_Alloc( 5 );
			String::Cpy( cmdString, "quit" );
			Sys_QueEvent( 0, SE_CONSOLE, 0, 0, String::Length( cmdString ) + 1, cmdString );
		} else if ( s_wcd.quitOnClose ) {
			PostQuitMessage( 0 );
		} else {
			Sys_ShowConsole( 0, false );
			Cvar_Set( "viewlog", "0" );
		}
		return 0;

	case WM_CTLCOLORSTATIC:
		if ( ( HWND )lParam == s_wcd.hwndBuffer ) {
			SetBkColor( ( HDC )wParam, RGB( 0x00, 0x00, 0xB0 ) );
			SetTextColor( ( HDC )wParam, RGB( 0xff, 0xff, 0x00 ) );

#if 0	// this draws a background in the edit box, but there are issues with this
			if ( ( hdcScaled = CreateCompatibleDC( ( HDC )wParam ) ) != 0 ) {
				if ( SelectObject( ( HDC )hdcScaled, s_wcd.hbmLogo ) ) {
					StretchBlt( ( HDC )wParam, 0, 0, 512, 384,
						hdcScaled, 0, 0, 512, 384,
						SRCCOPY );
				}
				DeleteDC( hdcScaled );
			}
#endif
			return ( long )s_wcd.hbrEditBackground;
		} else if ( ( HWND )lParam == s_wcd.hwndErrorBox ) {
			if ( s_timePolarity ) {
				SetBkColor( ( HDC )wParam, RGB( 0x80, 0x80, 0x80 ) );
				SetTextColor( ( HDC )wParam, RGB( 0xff, 0x0, 0x00 ) );
			} else {
				SetBkColor( ( HDC )wParam, RGB( 0x80, 0x80, 0x80 ) );
				SetTextColor( ( HDC )wParam, RGB( 0x00, 0x0, 0x00 ) );
			}
			return ( long )s_wcd.hbrErrorBackground;
		}
		break;

	case WM_COMMAND:
		if ( wParam == COPY_ID ) {
			SendMessage( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
			SendMessage( s_wcd.hwndBuffer, WM_COPY, 0, 0 );
		} else if ( wParam == QUIT_ID ) {
			if ( s_wcd.quitOnClose ) {
				PostQuitMessage( 0 );
			} else {
				char* cmdString = ( char* )Mem_Alloc( 5 );
				String::Cpy( cmdString, "quit" );
				Sys_QueEvent( 0, SE_CONSOLE, 0, 0, String::Length( cmdString ) + 1, cmdString );
			}
		} else if ( wParam == CLEAR_ID ) {
			SendMessage( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
			SendMessage( s_wcd.hwndBuffer, EM_REPLACESEL, FALSE, ( LPARAM )"" );
			UpdateWindow( s_wcd.hwndBuffer );
		}
		break;

	case WM_CREATE:
//		s_wcd.hbmLogo = LoadBitmap(g_wv.hInstance, MAKEINTRESOURCE(IDB_BITMAP1));
//		s_wcd.hbmClearBitmap = LoadBitmap(g_wv.hInstance, MAKEINTRESOURCE(IDB_BITMAP2));
		s_wcd.hbrEditBackground = CreateSolidBrush( RGB( 0x00, 0x00, 0xB0 ) );
		s_wcd.hbrErrorBackground = CreateSolidBrush( RGB( 0x80, 0x80, 0x80 ) );
		SetTimer( hWnd, 1, 1000, NULL );
		break;

	case WM_ERASEBKGND:
#if 0
		HDC hdcScaled;
		HGDIOBJ oldObject;

#if 1	// a single, large image
		hdcScaled = CreateCompatibleDC( ( HDC )wParam );
		assert( hdcScaled != 0 );

		if ( hdcScaled ) {
			oldObject = SelectObject( ( HDC )hdcScaled, s_wcd.hbmLogo );
			assert( oldObject != 0 );
			if ( oldObject ) {
				StretchBlt( ( HDC )wParam, 0, 0, s_wcd.windowWidth, s_wcd.windowHeight,
					hdcScaled, 0, 0, 512, 384,
					SRCCOPY );
			}
			DeleteDC( hdcScaled );
			hdcScaled = 0;
		}
#else	// a repeating brush
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
				FillRect( ( HDC )wParam, &r, hbrClearBrush );
				DeleteObject( hbrClearBrush );
			}
		}
#endif
		return 1;
#endif
		return DefWindowProc( hWnd, uMsg, wParam, lParam );

	case WM_TIMER:
		if ( wParam == 1 ) {
			s_timePolarity = !s_timePolarity;
			if ( s_wcd.hwndErrorBox ) {
				InvalidateRect( s_wcd.hwndErrorBox, NULL, FALSE );
			}
		}
		break;
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

static LONG WINAPI InputLineWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	switch ( uMsg ) {
	case WM_KILLFOCUS:
		if ( ( HWND )wParam == s_wcd.hWnd ||
			 ( HWND )wParam == s_wcd.hwndErrorBox ) {
			SetFocus( hWnd );
			return 0;
		}
		break;

	case WM_KEYDOWN:
		switch ( wParam ) {
		case VK_UP:
			// previous history item
			if ( ( win_nextHistoryLine - win_historyLine < WIN_COMMAND_HISTORY ) && win_historyLine > 0 ) {
				win_historyLine--;
			}
			win_consoleField = win_historyEditLines[ win_historyLine % WIN_COMMAND_HISTORY ];
			SetWindowText( s_wcd.hwndInputLine, win_consoleField.buffer );
			SendMessage( s_wcd.hwndInputLine, EM_SETSEL, win_consoleField.cursor, win_consoleField.cursor );
			win_acLength = 0;
			return 0;

		case VK_DOWN:
			// next history item
			if ( win_historyLine < win_nextHistoryLine ) {
				win_historyLine++;
				win_consoleField = win_historyEditLines[ win_historyLine % WIN_COMMAND_HISTORY ];
				SetWindowText( s_wcd.hwndInputLine, win_consoleField.buffer );
				SendMessage( s_wcd.hwndInputLine, EM_SETSEL, win_consoleField.cursor, win_consoleField.cursor );
			}
			win_acLength = 0;
			return 0;
		}
		break;

	case WM_CHAR:
		GetWindowText( s_wcd.hwndInputLine, win_consoleField.buffer, sizeof ( win_consoleField.buffer ) );
		SendMessage( s_wcd.hwndInputLine, EM_GETSEL, ( WPARAM )NULL, ( LPARAM )&win_consoleField.cursor );
		win_consoleField.widthInChars = String::Length( win_consoleField.buffer );
		win_consoleField.scroll = 0;

		// handle enter key
		if ( wParam == 13 ) {
			String::Cat( s_wcd.consoleText, sizeof ( s_wcd.consoleText ) - 5, win_consoleField.buffer );
			String::Cat( s_wcd.consoleText, sizeof ( s_wcd.consoleText ), "\n" );
			SetWindowText( s_wcd.hwndInputLine, "" );

			Sys_Print( va( "]%s\n", win_consoleField.buffer ) );

			// clear autocomplete length
			win_acLength = 0;

			// copy line to history buffer
			if ( win_consoleField.buffer[ 0 ] != '\0' ) {
				win_historyEditLines[ win_nextHistoryLine % WIN_COMMAND_HISTORY ] = win_consoleField;
				win_nextHistoryLine++;
				win_historyLine = win_nextHistoryLine;
			}

			return 0;
		}
		// handle tab key (commandline completion)
		else if ( wParam == 9 ) {
			Field_CompleteCommand( &win_consoleField, win_acLength );

			SetWindowText( s_wcd.hwndInputLine, win_consoleField.buffer );
			win_consoleField.widthInChars = String::Length( win_consoleField.buffer );
			SendMessage( s_wcd.hwndInputLine, EM_SETSEL, win_consoleField.cursor, win_consoleField.cursor );

			return 0;
		}
		// handle everything else
		else {
			win_acLength = 0;
		}
	}

	return CallWindowProc( s_wcd.SysInputLineWndProc, hWnd, uMsg, wParam, lParam );
}

void Sys_CreateConsole( const char* Title ) {
	const char* DEDCLASS = "jlQuake WinConsole";
	int DEDSTYLE = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX | WS_SIZEBOX;

	WNDCLASS wc;
	Com_Memset( &wc, 0, sizeof ( wc ) );

	wc.style = 0;
	wc.lpfnWndProc = ( WNDPROC )ConWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = global_hInstance;
	wc.hIcon = LoadIcon( global_hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = ( HBRUSH )COLOR_WINDOW;
	wc.lpszMenuName = 0;
	wc.lpszClassName = DEDCLASS;

	if ( !RegisterClass( &wc ) ) {
		return;
	}

	RECT rect;
	rect.left = 0;
	rect.right = SYSCON_DEFAULT_WIDTH;
	rect.top = 0;
	rect.bottom = SYSCON_DEFAULT_HEIGHT;
	AdjustWindowRect( &rect, DEDSTYLE, FALSE );

	HDC hDC = GetDC( GetDesktopWindow() );
	int swidth = GetDeviceCaps( hDC, HORZRES );
	int sheight = GetDeviceCaps( hDC, VERTRES );
	ReleaseDC( GetDesktopWindow(), hDC );

	s_wcd.windowWidth = rect.right - rect.left + 1;
	s_wcd.windowHeight = rect.bottom - rect.top + 1;

	s_wcd.hWnd = CreateWindowEx( 0, DEDCLASS, Title, DEDSTYLE,
		( swidth - 600 ) / 2, ( sheight - 450 ) / 2, rect.right - rect.left + 1, rect.bottom - rect.top + 1,
		NULL, NULL, global_hInstance, NULL );

	if ( s_wcd.hWnd == NULL ) {
		return;
	}

	//
	// create fonts
	//
	hDC = GetDC( s_wcd.hWnd );
	int nHeight = -MulDiv( 8, GetDeviceCaps( hDC, LOGPIXELSY ), 72 );

	s_wcd.hfBufferFont = CreateFont( nHeight, 0, 0, 0, FW_LIGHT, 0, 0, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN | FIXED_PITCH, "Courier New" );

	ReleaseDC( s_wcd.hWnd, hDC );

	//
	// create the input line
	//
	s_wcd.hwndInputLine = CreateWindowEx( WS_EX_CLIENTEDGE, "edit", NULL,
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
		6, 400, 528, 20, s_wcd.hWnd, ( HMENU )INPUT_ID, global_hInstance, NULL );

	//
	// create the buttons
	//
	s_wcd.hwndButtonCopy = CreateWindow( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD,
		5, 425, 72, 24, s_wcd.hWnd, ( HMENU )COPY_ID, global_hInstance, NULL );
	SendMessage( s_wcd.hwndButtonCopy, WM_SETTEXT, 0, ( LPARAM )"Copy" );

	s_wcd.hwndButtonClear = CreateWindow( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD,
		82, 425, 72, 24, s_wcd.hWnd, ( HMENU )CLEAR_ID, global_hInstance, NULL );
	SendMessage( s_wcd.hwndButtonClear, WM_SETTEXT, 0, ( LPARAM )"Clear" );

	s_wcd.hwndButtonQuit = CreateWindow( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD,
		462, 425, 72, 24, s_wcd.hWnd, ( HMENU )QUIT_ID, global_hInstance, NULL );
	SendMessage( s_wcd.hwndButtonQuit, WM_SETTEXT, 0, ( LPARAM )"Quit" );

	//
	// create the scrollbuffer
	//
	s_wcd.hwndBuffer = CreateWindowEx( WS_EX_CLIENTEDGE, "edit", NULL,
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_LEFT | ES_MULTILINE |
		ES_AUTOVSCROLL | ES_READONLY, 6, 40, 526, 354, s_wcd.hWnd,
		( HMENU )EDIT_ID, global_hInstance, NULL );
	SendMessage( s_wcd.hwndBuffer, WM_SETFONT, ( WPARAM )s_wcd.hfBufferFont, 0 );

	s_wcd.SysInputLineWndProc = ( WNDPROC )SetWindowLongPtr( s_wcd.hwndInputLine, GWLP_WNDPROC, ( LONG_PTR )InputLineWndProc );
	SendMessage( s_wcd.hwndInputLine, WM_SETFONT, ( WPARAM )s_wcd.hfBufferFont, 0 );

	ShowWindow( s_wcd.hWnd, SW_SHOWDEFAULT );
	UpdateWindow( s_wcd.hWnd );
	SetForegroundWindow( s_wcd.hWnd );
	SetFocus( s_wcd.hwndInputLine );

	s_wcd.visLevel = 1;
}

void Sys_DestroyConsole() {
	if ( s_wcd.hWnd ) {
		ShowWindow( s_wcd.hWnd, SW_HIDE );
		CloseWindow( s_wcd.hWnd );
		DestroyWindow( s_wcd.hWnd );
		s_wcd.hWnd = 0;
	}
}

void Sys_ShowConsole( int visLevel, bool quitOnClose ) {
	s_wcd.quitOnClose = quitOnClose;

	if ( visLevel == s_wcd.visLevel ) {
		return;
	}

	s_wcd.visLevel = visLevel;

	if ( !s_wcd.hWnd ) {
		return;
	}

	switch ( visLevel ) {
	case 0:
		ShowWindow( s_wcd.hWnd, SW_HIDE );
		break;
	case 1:
		ShowWindow( s_wcd.hWnd, SW_SHOWNORMAL );
		SendMessage( s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff );
		break;
	case 2:
		ShowWindow( s_wcd.hWnd, SW_MINIMIZE );
		break;
	default:
		common->FatalError( "Invalid visLevel %d sent to Sys_ShowConsole\n", visLevel );
		break;
	}
}

char* Sys_ConsoleInput() {
	if ( s_wcd.consoleText[ 0 ] == 0 ) {
		return NULL;
	}

	String::Cpy( s_wcd.returnedText, s_wcd.consoleText );
	s_wcd.consoleText[ 0 ] = 0;

	return s_wcd.returnedText;
}

//	Print text to the dedicated console
void Sys_Print( const char* pMsg ) {
#define CONSOLE_BUFFER_SIZE     16384

	static unsigned long s_totalChars;

	//
	// if the message is REALLY long, use just the last portion of it
	//
	const char* msg;
	if ( String::Length( pMsg ) > CONSOLE_BUFFER_SIZE - 1 ) {
		msg = pMsg + String::Length( pMsg ) - CONSOLE_BUFFER_SIZE + 1;
	} else {
		msg = pMsg;
	}

	//
	// copy into an intermediate buffer
	//
	char buffer[ CONSOLE_BUFFER_SIZE * 2 ];
	char* b = buffer;
	int i = 0;
	while ( msg[ i ] && ( ( b - buffer ) < sizeof ( buffer ) - 1 ) ) {
		if ( msg[ i ] == '\n' && msg[ i + 1 ] == '\r' ) {
			b[ 0 ] = '\r';
			b[ 1 ] = '\n';
			b += 2;
			i++;
		} else if ( msg[ i ] == '\r' ) {
			b[ 0 ] = '\r';
			b[ 1 ] = '\n';
			b += 2;
		} else if ( msg[ i ] == '\n' ) {
			b[ 0 ] = '\r';
			b[ 1 ] = '\n';
			b += 2;
		} else if ( Q_IsColorString( &msg[ i ] ) ) {
			i++;
		} else {
			*b = msg[ i ];
			b++;
		}
		i++;
	}
	*b = 0;
	int bufLen = b - buffer;

	s_totalChars += bufLen;

	//
	// replace selection instead of appending if we're overflowing
	//
	if ( s_totalChars > CONSOLE_BUFFER_SIZE ) {
		SendMessage( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
		s_totalChars = bufLen;
	} else {
		// always append at the bottom of the textbox
		SendMessage( s_wcd.hwndBuffer, EM_SETSEL, 0xFFFF, 0xFFFF );
	}

	//
	// put this text into the windows console
	//
	SendMessage( s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff );
	SendMessage( s_wcd.hwndBuffer, EM_SCROLLCARET, 0, 0 );
	SendMessage( s_wcd.hwndBuffer, EM_REPLACESEL, 0, ( LPARAM )buffer );
}

void Sys_SetErrorText( const char* buf ) {
	String::NCpyZ( s_wcd.errorString, buf, sizeof ( s_wcd.errorString ) );

	if ( !s_wcd.hwndErrorBox ) {
		s_wcd.hwndErrorBox = CreateWindow( "static", NULL, WS_CHILD | WS_VISIBLE | SS_SUNKEN,
			6, 5, 526, 30, s_wcd.hWnd, ( HMENU )ERRORBOX_ID, global_hInstance, NULL );
		SendMessage( s_wcd.hwndErrorBox, WM_SETFONT, ( WPARAM )s_wcd.hfBufferFont, 0 );
		SetWindowText( s_wcd.hwndErrorBox, s_wcd.errorString );

		DestroyWindow( s_wcd.hwndInputLine );
		s_wcd.hwndInputLine = NULL;
	}
}

void Sys_ClearViewlog_f() {
	SendMessage( s_wcd.hwndBuffer, WM_SETTEXT, 0, ( LPARAM )"" );
}
