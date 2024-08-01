
#include <windows.h>

void Sys_CreateConsole( HINSTANCE hInstance );
void Conbuf_AppendText( const char *pMsg );
void Sys_SetStatusText( const char* statustext, int line );
void Sys_SetProgress(int step);
HWND Sys_GetHWnd();
void DisableExit();