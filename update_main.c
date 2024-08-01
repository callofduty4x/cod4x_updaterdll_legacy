#define WIN32_LEAN_AND_MEAN

#include "q_shared.h"
#include "qcommon.h"
#include "sec_common.h"

#include <windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <direct.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <mmsystem.h>
#include <Shellapi.h>
#include "blake\blake2.h"
#include "windows_inc.h"


typedef struct{
	HINSTANCE reflib_library;           // Handle to refresh DLL
	qboolean reflib_active;
	HWND hWnd;							//0xcc1b6fc
	HINSTANCE hInstance;				//0xcc1b700
	qboolean activeApp;
	qboolean isMinimized;				//0xcc1b708
	OSVERSIONINFO osversion;
	// when we get a windows message, we store the time off so keyboard processing
	// can know the exact time of an event
	unsigned sysMsgTime;				//0xcc1b710
} WinVars_t;


#define PATH_SEP '\\'

wchar_t* FS_GetSavePath();

/*
================
Sys_Milliseconds
================
*/

int Sys_Milliseconds( void ) {
	int sys_curtime;
	static int sys_timeBase;
	static qboolean initialized = qfalse;

	if ( !initialized ) {
		sys_timeBase = timeGetTime();
		initialized = qtrue;
	}
	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
}


/*
=============
Com_Error

Both client and server can use this, and it will
do the appropriate thing.
=============
*/
void Com_Error( int null, const char *fmt, ... ) {
	va_list		argptr;
	char com_errorMessage[2048];

	va_start (argptr,fmt);
	Q_vsnprintf (com_errorMessage, sizeof(com_errorMessage),fmt,argptr);
	va_end (argptr);

	MessageBoxA(NULL, com_errorMessage, "Call of Duty 4 - Update has Failed", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
	
	CoD4UpdateShutdown(1);
	
}

/*
=============
Com_ErrorUni

Both client and server can use this, and it will
do the appropriate thing.
=============
*/
void Com_ErrorUni( const wchar_t *fmt, ... ) {
	va_list		argptr;
	wchar_t com_errorMessage[2048];
	int numchar = sizeof(com_errorMessage) / sizeof(wchar_t);
	
	va_start (argptr,fmt);
	vsnwprintf (com_errorMessage, numchar, fmt, argptr );
	va_end (argptr);

	MessageBoxW(NULL, com_errorMessage, L"Call of Duty 4 - Update has Failed", MB_OK | MB_ICONERROR | MB_APPLMODAL);
	CoD4UpdateShutdown(1);;
}

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
void QDECL Com_Printf( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

    Conbuf_AppendText( msg );
}

/*
=============
Com_PrintStatus

Both client and server can use this, and it will output
to the apropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
void QDECL Com_PrintStatus( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

    Sys_SetStatusText( msg, 0 );
	Conbuf_AppendText( msg );
}

/*
====================
FS_StripTrailingSeperator

Fix things up differently for win/unix/mac
====================
*/
static void FS_StripTrailingSeperator( char *path ) {

	int len = strlen(path);

	if(path[len -1] == PATH_SEP)
	{
		path[len -1] = '\0';
	}
}


/*
====================
FS_StripTrailingSeperatorUni

Fix things up differently for win/unix/mac
====================
*/
static void FS_StripTrailingSeperatorUni( wchar_t *path ) {

	int len = wcslen(path);

	if(path[len -1] == PATH_SEPUNI)
	{
		path[len -1] = L'\0';
	}
}


/*
====================
FS_ReplaceSeparators

Fix things up differently for win/unix/mac
====================
*/
void FS_ReplaceSeparators( char *path ) {
	char	*s;

	for ( s = path ; *s ; s++ ) {
		if ( *s == '/' || *s == '\\' ) {
			*s = PATH_SEP;
		}
	}
}

/*
====================
FS_ReplaceSeparatorsUni

Fix things up differently for win/unix/mac
====================
*/
void FS_ReplaceSeparatorsUni( wchar_t *path ) {
	wchar_t	*s;

	for ( s = path ; *s ; s++ ) {
		if ( *s == L'/' || *s == L'\\' ) {
			*s = PATH_SEPUNI;
		}
	}
}

void Sys_Mkdir(const char* dir)
{
	_mkdir(dir);
}

void Sys_MkdirUni(const wchar_t* dir)
{
	_wmkdir(dir);
}

/*
===========
FS_RemoveOSPath

===========
*/
void FS_RemoveOSPath( const char *osPath ) {
	remove( osPath );
}

/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
qboolean FS_CreatePath (char *OSPath) {
	char	*ofs;

	// make absolutely sure that it can't back up the path
	// FIXME: is c: allowed???
	if ( strstr( OSPath, ".." ) || strstr( OSPath, "::" ) ) {
		Com_Printf( "WARNING: refusing to create relative path \"%s\"\n", OSPath );
		return qtrue;
	}

	for (ofs = OSPath+1 ; *ofs ; ofs++) {
		if (*ofs == PATH_SEP) {	
			// create the directory
			*ofs = 0;
			Sys_Mkdir (OSPath);
			*ofs = PATH_SEP;
		}
	}
	return qfalse;
}



/*
=================
FS_CopyFile

Copy a fully specified file from one place to another
=================
*/
void FS_CopyFile( char *fromOSPath, char *toOSPath ) {
	FILE    *f;
	int len;
	byte    *buf;

	f = fopen( fromOSPath, "rb" );
	if ( !f ) {
		return;
	}
	fseek( f, 0, SEEK_END );
	len = ftell( f );
	fseek( f, 0, SEEK_SET );

	// we are using direct malloc instead of Z_Malloc here, so it
	// probably won't work on a mac... Its only for developers anyway...
	buf = malloc( len );
	if ( fread( buf, 1, len, f ) != len ) {
		Com_Error(0, "Short read in FS_Copyfiles()\n" );
	}
	fclose( f );

	if ( FS_CreatePath( toOSPath ) ) {
		return;
	}

	f = fopen( toOSPath, "wb" );
	if ( !f ) {
		free( buf );    //DAJ free as well
		return;
	}
	if ( fwrite( buf, 1, len, f ) != len ) {
		Com_Error(0, "Short write in FS_Copyfiles()\n" );
	}
	fclose( f );
	free( buf );


}

/*
============
FS_CreatePathUni

Creates any directories needed to store the given filename
============
*/
qboolean FS_CreatePathUni (wchar_t *OSPath) {
	wchar_t	*ofs;

	// make absolutely sure that it can't back up the path
	// FIXME: is c: allowed???
	if ( wcsstr( OSPath, L".." ) || wcsstr( OSPath, L"::" ) ) {
		Com_Printf( "WARNING: refusing to create relative unicode path\n" );
		return qtrue;
	}

	if(OSPath[0] == L'\0')
	{
		return qtrue;
	}
	
	for (ofs = OSPath+1 ; *ofs ; ofs++) {
		if (*ofs == PATH_SEPUNI) {	
			// create the directory
			*ofs = 0;
			Sys_MkdirUni(OSPath);
			*ofs = PATH_SEPUNI;
		}
	}
	return qfalse;
}


qboolean Sys_CopyFileUni(wchar_t* fromOSPath, wchar_t* toOSPath)
{

	if(CopyFileW( fromOSPath, toOSPath, FALSE) == 0)
	{
		return qfalse;
	}
	return qtrue;
}

/*
=================
FS_CopyFileUni

Copy a fully specified file from one place to another
=================
*/
qboolean FS_CopyFileUni( wchar_t *fromOSPath, wchar_t *toOSPath )
{
	qboolean suc;

	if ( FS_CreatePathUni( toOSPath ) ) {
		return qfalse;
	}

	suc = Sys_CopyFileUni(fromOSPath, toOSPath);
	
	return suc;
}

/*
===========
FS_RemoveOSPathUni

===========
*/
void FS_RemoveOSPathUni( const wchar_t *osPath ) {
	_wremove( osPath );
}


/*
===========
FS_RenameOSPath

===========
*/
void FS_RenameOSPath( const char *from_ospath, const char *to_ospath ) {

	if (rename( from_ospath, to_ospath )) {
		// Failed, try copying it and deleting the original
		FS_CopyFile ( (char*)from_ospath, (char*)to_ospath );
		FS_RemoveOSPath ( from_ospath );
	}
}

/*
===========
FS_RenameOSPathUni

===========
*/
void FS_RenameOSPathUni( const wchar_t *from_ospath, const wchar_t *to_ospath ) {

	if (_wrename( from_ospath, to_ospath )) {
		// Failed, try copying it and deleting the original
		FS_CopyFileUni ( (wchar_t*)from_ospath, (wchar_t*)to_ospath );
		FS_RemoveOSPathUni ( from_ospath );
	}
}

/*
===========
FS_FileExistsOSPath

===========
*/
qboolean FS_FileExistsOSPath( const char *osPath ) {
	
	FILE* fh = fopen( osPath, "rb" );
	
	if(fh == NULL)
		return qfalse;
	
	fclose( fh );
	return qtrue;
	
}

/*
===========
FS_FileExistsOSPathUni

===========
*/
qboolean FS_FileExistsOSPathUni( const wchar_t *osPath ) {
	
	FILE* fh = _wfopen( osPath, L"rb" );
	
	if(fh == NULL)
		return qfalse;
	
	fclose( fh );
	return qtrue;
	
}

/*
================
FS_filelengthOSPath

If this is called on a non-unique FILE (from a pak file),
it will return the size of the pak file, not the expected
size of the file.
================
*/
int FS_filelengthOSPath( FILE* h ) {
	int		pos;
	int		end;

	pos = ftell (h);
	fseek (h, 0, SEEK_END);
	end = ftell (h);
	fseek (h, pos, SEEK_SET);

	return end;
}

/*
===========
FS_FOpenFileReadOSPathUni
search for a file somewhere below the home path, base path or cd path
we search in that order, matching FS_SV_FOpenFileRead order
===========
*/
int FS_FOpenFileReadOSPathUni( const wchar_t *filename, FILE **fp ) {
	wchar_t ospath[MAX_OSPATH];
	FILE* fh;
	
	Q_strncpyzUni( ospath, filename, sizeof( ospath ) );

	fh = _wfopen( ospath, L"rb" );

	if ( !fh ){
		*fp = NULL;
		return -1;
	}

	*fp = fh;

	return FS_filelengthOSPath(fh);
}

/*
===========
FS_FOpenFileReadOSPath
search for a file somewhere below the home path, base path or cd path
we search in that order, matching FS_SV_FOpenFileRead order
===========
*/
int FS_FOpenFileReadOSPath( const char *filename, FILE **fp ) {
	char ospath[MAX_OSPATH];
	FILE* fh;
	
	Q_strncpyz( ospath, filename, sizeof( ospath ) );

	fh = fopen( ospath, "rb" );

	if ( !fh ){
		*fp = NULL;
		return -1;
	}

	*fp = fh;

	return FS_filelengthOSPath(fh);
}

/*
==============
FS_FCloseFileOSPath

==============
*/
qboolean FS_FCloseFileOSPath( FILE* f ) {

	if (f) {
	    fclose (f);
	    return qtrue;
	}
	return qfalse;
}

/*
=================
FS_ReadOSPath

Properly handles partial reads
=================
*/
int FS_ReadOSPath( void *buffer, int len, FILE* f ) {
	int		block, remaining;
	int		read;
	byte	*buf;

	if ( !f ) {
		return 0;
	}

	buf = (byte *)buffer;

	remaining = len;
	while (remaining) {
		block = remaining;
		read = fread (buf, 1, block, f);
		if (read == 0)
		{
			return len-remaining;	//Com_Error (ERR_FATAL, "FS_Read: 0 bytes read");
		}

		if (read == -1) {
			Com_Error (0,"FS_ReadOSPath: -1 bytes read");
		}

		remaining -= read;
		buf += read;
	}
	return len;

}

/*
============
FS_ReadFileOSPath

Filename are relative to the quake search path
a null buffer will just return the file length without loading
============
*/
int FS_ReadFileOSPath( const char *ospath, void **buffer ) {
	byte*	buf;
	int		len;
	FILE*   h;
	
	
	if ( !ospath || !ospath[0] ) {
		Com_Error(0, "FS_ReadFile with empty name\n" );
	}

	buf = NULL;	// quiet compiler warning

	// look for it in the filesystem or pack files
	len = FS_FOpenFileReadOSPath( ospath, &h );
	if ( len == -1 ) {
		if ( buffer ) {
			*buffer = NULL;
		}
		return -1;
	}
	
	if ( !buffer ) {
		FS_FCloseFileOSPath( h );
		return len;
	}

	buf = malloc(len+1);
	*buffer = buf;

	FS_ReadOSPath (buf, len, h);

	// guarantee that it will have a trailing 0 for string operations
	buf[len] = 0;
	FS_FCloseFileOSPath( h );
	return len;
}



void FS_BuildOSPathForThreadUni(const wchar_t *base, const char *game, const char *qpath, wchar_t *fs_path, int fs_thread)
{
  wchar_t basename[MAX_OSPATH];
  wchar_t gamename[MAX_OSPATH];
  wchar_t qpathname[MAX_QPATH];
  int len;

  if ( !game || !*game )
    game = "";

  Q_strncpyzUni(basename, base, sizeof(basename));
  Q_StrToWStr(gamename, game, sizeof(gamename));
  Q_StrToWStr(qpathname, qpath, sizeof(qpathname));
  
  len = wcslen(basename);
  if(len > 0 && (basename[len -1] == L'/' || basename[len -1] == L'\\'))
  {
        basename[len -1] = L'\0';
  }

  len = wcslen(gamename);
  if(len > 0 && (gamename[len -1] == L'/' || gamename[len -1] == L'\\'))
  {
        gamename[len -1] = L'\0';
  }
  
  if( Com_sprintfUni(fs_path, sizeof(wchar_t) * MAX_OSPATH, L"%s/%s/%s", basename, gamename, qpathname) >= MAX_OSPATH )
  {
    if ( fs_thread )
    {
        fs_path[0] = 0;
        return;
    }
    Com_Error(ERR_FATAL, "FS_BuildOSPath: os path length exceeded");
  }
  FS_ReplaceSeparatorsUni(fs_path);
  FS_StripTrailingSeperatorUni(fs_path);
}

void FS_BuildOSPathForThread(const char *base, const char *game, const char *qpath, char *fs_path, int fs_thread)
{
  char basename[MAX_OSPATH];
  char gamename[MAX_OSPATH];

  int len;

  if ( !game || !*game )
    game = "";

  Q_strncpyz(basename, base, sizeof(basename));
  Q_strncpyz(gamename, game, sizeof(gamename));

  len = strlen(basename);
  if(len > 0 && (basename[len -1] == '/' || basename[len -1] == '\\'))
  {
        basename[len -1] = '\0';
  }

  len = strlen(gamename);
  if(len > 0 && (gamename[len -1] == '/' || gamename[len -1] == '\\'))
  {
        gamename[len -1] = '\0';
  }
  if ( Com_sprintf(fs_path, MAX_OSPATH, "%s/%s/%s", basename, gamename, qpath) >= MAX_OSPATH )
  {
    if ( fs_thread )
    {
        fs_path[0] = 0;
        return;
    }
    Com_Error(ERR_FATAL, "FS_BuildOSPath: os path length exceeded");
  }
  FS_ReplaceSeparators(fs_path);
  FS_StripTrailingSeperator(fs_path);

}


/*
===========
FS_SV_FOpenFileWriteSavePath
===========
*/
FILE * FS_SV_FOpenFileWriteSavePath( const char *filename ) {
	wchar_t ospath[MAX_OSPATH];
	FILE* fh;
	
	FS_BuildOSPathForThreadUni( FS_GetSavePath(), filename, "", ospath, 0 );

	if ( FS_CreatePathUni( ospath ) ) {
		return NULL;
	}
	// enabling the following line causes a recursive function call loop
	// when running with +set logfile 1 +set developer 1
	//Com_DPrintf( "writing to: %s\n", ospath );
	fh = _wfopen( ospath, L"wb" );
	
	if ( !fh ){
		return NULL;
	}

	return fh;
}

/*
============
FS_SV_WriteFileToSavePath
 
Filename are reletive to the quake search path
============
*/
int FS_SV_WriteFileToSavePath( const char *qpath, const void *buffer, int size ) {
  FILE* f;

  
  if ( !qpath || !buffer ) {
    Com_Error( ERR_FATAL, "FS_WriteFile: NULL parameter" );
  }

  f = FS_SV_FOpenFileWriteSavePath( qpath );
  if ( !f ) {
    Com_Printf( "Failed to open %s\n", qpath );
    return 0;
  }

  if ( fwrite( buffer, 1, size, f ) != size ) {
	Com_PrintError("Short write in FS_SV_WriteFileToSavePath()\n" );
	size = 0;
  }

  fclose( f );
  
  return size;
}

qboolean FS_SV_FileExists(const char *filename)
{
	char ospath[1024];
	FS_BuildOSPathForThread(FS_GetInstallPath(), filename, "", ospath, 0);
	return FS_FileExistsOSPath( ospath );
}

void FS_SV_Rename(const char* from, const char* to)
{
	char from_ospath[1024];
	char to_ospath[1024];
	
	FS_BuildOSPathForThread(FS_GetInstallPath(), from, "", from_ospath, 0);	
	FS_BuildOSPathForThread(FS_GetInstallPath(), to, "", to_ospath, 0);	
	
	FS_RenameOSPath( from_ospath, to_ospath );
}

void FS_SV_Remove(const char* filename)
{
	char ospath[1024];
	FS_BuildOSPathForThread(FS_GetInstallPath(), filename, "", ospath, 0);
	FS_RemoveOSPath(ospath);
	
}

static int cmd_argc;
static char *cmd_argv[MAX_STRING_TOKENS]; // points into cmd_tokenized
static char cmd_tokenized[BIG_INFO_STRING + MAX_STRING_TOKENS]; // will have 0 bytes inserted

static int sv_cmd_argc;
static char *sv_cmd_argv[MAX_STRING_TOKENS]; // points into cmd_tokenized
static char sv_cmd_tokenized[BIG_INFO_STRING + MAX_STRING_TOKENS]; // will have 0 bytes inserted

/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
The text is copied to a seperate buffer and 0 characters
are inserted in the apropriate place, The argv array
will point into this temporary buffer.
============
*/
void Cmd_TokenizeString( const char *text_in ) {
	const char *text;
	char *textOut;

	// clear previous args
	cmd_argc = 0;

	if ( !text_in ) {
		return;
	}

	text = text_in;
	textOut = cmd_tokenized;

	while ( 1 ) {
		if ( cmd_argc == MAX_STRING_TOKENS ) {
			return; // this is usually something malicious
		}

		while ( 1 ) {
			// skip whitespace
			while ( *text && *text <= ' ' ) {
				text++;
			}
			if ( !*text ) {
				return; // all tokens parsed
			}

			// skip // comments
			if ( text[0] == '/' && text[1] == '/' ) {
				return; // all tokens parsed
			}

			// skip /* */ comments
			if ( text[0] == '/' && text[1] == '*' ) {
				while ( *text && ( text[0] != '*' || text[1] != '/' ) ) {
					text++;
				}
				if ( !*text ) {
					return; // all tokens parsed
				}
				text += 2;
			} else {
				break; // we are ready to parse a token
			}
		}

		// handle quoted strings
		if ( *text == '"' ) {
			cmd_argv[cmd_argc] = textOut;
			cmd_argc++;
			text++;
			while ( *text && *text != '"' ) {
				*textOut++ = *text++;
			}
			*textOut++ = 0;
			if ( !*text ) {
				return; // all tokens parsed
			}
			text++;
			continue;
		}

		// regular token
		cmd_argv[cmd_argc] = textOut;
		cmd_argc++;

		// skip until whitespace, quote, or command
		while ( *text > ' ' ) {
			if ( text[0] == '"' ) {
				break;
			}

			if ( text[0] == '/' && text[1] == '/' ) {
				break;
			}

			// skip /* */ comments
			if ( text[0] == '/' && text[1] == '*' ) {
				break;
			}

			*textOut++ = *text++;
		}

		*textOut++ = 0;

		if ( !*text ) {
			return; // all tokens parsed
		}
	}

}


/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
The text is copied to a seperate buffer and 0 characters
are inserted in the apropriate place, The argv array
will point into this temporary buffer.
============
*/
void SV_Cmd_TokenizeString( const char *text_in ) {
	const char *text;
	char *textOut;

	// clear previous args
	sv_cmd_argc = 0;

	if ( !text_in ) {
		return;
	}

	text = text_in;
	textOut = sv_cmd_tokenized;

	while ( 1 ) {
		if ( sv_cmd_argc == MAX_STRING_TOKENS ) {
			return; // this is usually something malicious
		}

		while ( 1 ) {
			// skip whitespace
			while ( *text && *text <= ' ' ) {
				text++;
			}
			if ( !*text ) {
				return; // all tokens parsed
			}

			// skip // comments
			if ( text[0] == '/' && text[1] == '/' ) {
				return; // all tokens parsed
			}

			// skip /* */ comments
			if ( text[0] == '/' && text[1] == '*' ) {
				while ( *text && ( text[0] != '*' || text[1] != '/' ) ) {
					text++;
				}
				if ( !*text ) {
					return; // all tokens parsed
				}
				text += 2;
			} else {
				break; // we are ready to parse a token
			}
		}

		// handle quoted strings
		if ( *text == '"' ) {
			sv_cmd_argv[sv_cmd_argc] = textOut;
			sv_cmd_argc++;
			text++;
			while ( *text && *text != '"' ) {
				*textOut++ = *text++;
			}
			*textOut++ = 0;
			if ( !*text ) {
				return; // all tokens parsed
			}
			text++;
			continue;
		}

		// regular token
		sv_cmd_argv[sv_cmd_argc] = textOut;
		sv_cmd_argc++;

		// skip until whitespace, quote, or command
		while ( *text > ' ' ) {
			if ( text[0] == '"' ) {
				break;
			}

			if ( text[0] == '/' && text[1] == '/' ) {
				break;
			}

			// skip /* */ comments
			if ( text[0] == '/' && text[1] == '*' ) {
				break;
			}

			*textOut++ = *text++;
		}

		*textOut++ = 0;

		if ( !*text ) {
			return; // all tokens parsed
		}
	}

}

/*
============
Cmd_Argc
============
*/
int Cmd_Argc( void ) {
	return cmd_argc;
}

/*
============
Cmd_Argc
============
*/
int SV_Cmd_Argc( void ) {
	return sv_cmd_argc;
}

/*
============
Cmd_Argv
============
*/
const char *Cmd_Argv( int arg ) {
	if ( (unsigned)arg >= cmd_argc ) {
		return "";
	}
	return cmd_argv[arg];
}

/*
============
Cmd_Argv
============
*/
char *SV_Cmd_Argv( int arg ) {
	if ( (unsigned)arg >= sv_cmd_argc ) {
		return "";
	}
	return sv_cmd_argv[arg];
}


unsigned int Sys_MillisecondsRaw()
{
	return timeGetTime();
}



/*
=============
NET_StringToAdr

Traps "localhost" for loopback, passes everything else to system
return 0 on address not found, 1 on address found with port, 2 on address found without port.
=============
*/
int NET_StringToAdr( const char *s, netadr_t *a, netadrtype_t family )
{
	char	base[MAX_STRING_CHARS], *search;
	char	*port = NULL;

	if (!strcmp (s, "localhost")) {
		Com_Memset (a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
// as NA_LOOPBACK doesn't require ports report port was given.
		return 1;
	}

	Q_strncpyz( base, s, sizeof( base ) );
	
	if(*base == '[' || Q_CountChar(base, ':') > 1)
	{
		// This is an ipv6 address, handle it specially.
		search = strchr(base, ']');
		if(search)
		{
			*search = '\0';
			search++;

			if(*search == ':')
				port = search + 1;
		}
		
		if(*base == '[')
			search = base + 1;
		else
			search = base;
	}
	else
	{
		// look for a port number
		port = strchr( base, ':' );
		
		if ( port ) {
			*port = '\0';
			port++;
		}
		
		search = base;
	}

	if(!Sys_StringToAdr(search, a, family))
	{
		a->type = NA_BAD;
		return 0;
	}

	if(port)
	{
		a->port = BigShort((short) atoi(port));
		return 1;
	}
	else
	{
		a->port = BigShort(PORT_SERVER);
		return 2;
	}
}

CRITICAL_SECTION sys_xCriticalSection;

void Sys_InitializeCriticalSection()
{
	InitializeCriticalSection(&sys_xCriticalSection);
}

void Sys_EnterCriticalSection()
{
	EnterCriticalSection(&sys_xCriticalSection);
}

void Sys_LeaveCriticalSection()
{
	LeaveCriticalSection(&sys_xCriticalSection);
}

void Sys_GetLastErrorAsString(char* errbuf, int len)
{
	int lastError = GetLastError();
	if(lastError != 0)
	{
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lastError, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR)errbuf, len -1, NULL);
	}else{
		Q_strncpyz(errbuf, "Unknown Error", len);
	}
}

int Sys_SortVersionDirs(const void* cmp1, const void* cmp2)
{
	const char* stri1 = *(const char**)cmp1;
	const char* stri2 = *(const char**)cmp2;

	return Q_stricmp(stri1, stri2);
}



int Sys_ListDirectories(const wchar_t* dir, char** list, int limit)
{
  int count, i;
  HANDLE dhandle;
  struct _WIN32_FIND_DATAW FindFileData;
  char *ansifilename;
  wchar_t searchdir[MAX_OSPATH];
  char errorString[1024];

  list[0] = NULL;

  FS_BuildOSPathForThreadUni( dir, "*", "", searchdir, 0 );

  dhandle = FindFirstFileW(searchdir, &FindFileData);
  if ( dhandle == (HANDLE)-1 )
  {
	Sys_GetLastErrorAsString(errorString, sizeof(errorString));
	Com_DPrintf("Sys_ListDirectories: %s\n", errorString);
    return 0;
  }
  count = 0;

  do
  {

    if ( !FindFileData.cFileName[0])
    {
		continue;
		}
		if(FindFileData.cFileName[0] == L'.' && (FindFileData.cFileName[1] == L'.' || !FindFileData.cFileName[1]))
		{
			continue;
		}

		/* is directory ? */
	  if ( FindFileData.dwFileAttributes & 0x10)
		{

			if(Q_WIsAnsiString(FindFileData.cFileName) == qfalse)
			{

				//We won't support non ANSI chars
				continue;
			}

			list[count] = malloc(wcslen(FindFileData.cFileName) +1);

			if(list[count] == NULL)
			{
				break;
			}

			ansifilename = list[count];

			/* String copy as ANSI string */
			for(i = 0; FindFileData.cFileName[i]; ++i)
			{
				ansifilename[i] = FindFileData.cFileName[i];
			}
			ansifilename[i] = '\0';
			count++;
		}

  }
  while ( FindNextFileW(dhandle, &FindFileData) != 0 && count < limit -1);

  FindClose(dhandle);

  list[count] = NULL;

  return count;
}

/*
==============
Sys_FreeFileList
==============
*/
void Sys_FreeFileList( char **list )
{
	int i;

	if ( !list ) {
		return;
	}

	for ( i = 0 ; list[i] ; i++ ) {
		free( list[i] );
	}

}



wchar_t fs_savepathbuf[1024];
char fs_installpathbuf[1024];
qboolean windowsVistaAware;
wchar_t* FS_GetSavePath()
{
	return fs_savepathbuf;
}

void FS_SetupSavePath(const wchar_t *updatepath)
{
	Q_strncpyzUni(fs_savepathbuf, updatepath, sizeof(fs_savepathbuf));
	FS_StripTrailingSeperatorUni( fs_savepathbuf );
			
	if(wcsncmp(&fs_savepathbuf[wcslen(fs_savepathbuf) - 7], L"updates", 7) == 0)
	{
		fs_savepathbuf[wcslen(fs_savepathbuf) - 8] = '\0';
	}
	FS_StripTrailingSeperatorUni( fs_savepathbuf );

	FS_ReplaceSeparatorsUni(fs_savepathbuf);
}

void FS_SetupInstallPath(const char* installpath)
{
	Q_strncpyz(fs_installpathbuf, installpath, sizeof(fs_installpathbuf));
	FS_StripTrailingSeperator( fs_installpathbuf );
	FS_ReplaceSeparators(fs_installpathbuf);
}


wchar_t* FS_GetInstallPathUni(wchar_t* buf, int size)
{
	Q_StrToWStr(buf, FS_GetInstallPath(), size);
	return buf;
}

char* FS_GetInstallPath()
{
	return fs_installpathbuf;	
}


char versionstring[1024];

const char* Com_GetVersion()
{
	return versionstring;
}

void Sys_CheckOS()
{

	OSVERSIONINFO osversion;
	
	osversion.dwOSVersionInfoSize = sizeof( osversion );

	if ( !GetVersionEx( &osversion ) ) {
		windowsVistaAware = qfalse;
	}
		
	if(osversion.dwMajorVersion >= 6)
	{
		windowsVistaAware = qtrue;
	}else{
		windowsVistaAware = qfalse;
	}
}

qboolean Sys_IsWindowsVistaAware()
{
	return windowsVistaAware;
}

/*
===========
FS_WriteTestOSPath

===========
*/
//returns true if writetest was successful
qboolean FS_WriteTestOSPath( const char *osPath ) {
	
	FILE* fh = fopen( osPath, "wb" );
	char testbuf[] = "MZ_test";
	
	if(fh == NULL)
		return qfalse;
	
	fwrite(testbuf, 1, sizeof(testbuf), fh);
	fclose( fh );
	
	fh = fopen( osPath, "rb" );
	
	if(fh == NULL)
		return qfalse;
	fclose( fh );
	
	remove( osPath );
	
	return qtrue;
	
}

//Returns false if write access is denied
qboolean Sys_TestPermissions()
{
	char testpath[MAX_STRING_CHARS];
	char exepath[MAX_STRING_CHARS];
	unsigned short rannum;


	GetModuleFileNameA(NULL, exepath, sizeof(exepath));

	if(strlen(exepath) < 9)
	{
		return qfalse;
	}

	rannum = GetTickCount();
	Com_sprintf( testpath, sizeof(testpath), "%s.%u_test.exe", exepath, rannum);

	if(FS_WriteTestOSPath( testpath ) == qtrue)
	{
		return qtrue;
	}
	return qfalse;
}

char sys_restartCmdLine[4096];

void Sys_SetRestartParams(const char* params)
{
	Q_strncpyz(sys_restartCmdLine, params, sizeof(sys_restartCmdLine));
}

void Sys_RestartProcess()
{
	void* HWND = NULL;
	char displayMessageBuf[1024];
	char exefile[1024];
	const char* method;
    SHELLEXECUTEINFO sei;
	int pid;


	GetModuleFileNameA(NULL, exefile, sizeof(exefile));

	if(strlen(exefile) < 9)
	{
		Com_Error(0, "Can not restart with elevated permissions. GetModuleFileNameA(NULL) fails");
		return;
	}


	method = "runas";
	MessageBoxA(HWND, "Note: Installation of this update for Call of Duty 4 will require extended permissions" , "Call of Duty 4 - Autoupdate", MB_OK | MB_ICONEXCLAMATION);

	// Launch itself as administrator.
	memset(&sei, 0, sizeof(sei));
	sei.cbSize = sizeof(sei);
    sei.lpVerb = method;
    sei.lpFile = exefile;
	sei.lpParameters = sys_restartCmdLine;
    sei.hwnd = HWND;
    sei.nShow = SW_NORMAL;
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;

	if (!ShellExecuteExA(&sei))
	{
		FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						0, GetLastError(), 0x400, displayMessageBuf, sizeof(displayMessageBuf), 0);

		MessageBoxA(HWND, va("ShellExec of commandline: %s %s has failed.\nError: %s\n" , exefile, sys_restartCmdLine, displayMessageBuf), "CoD4 X Update - Error", MB_OK | MB_ICONERROR);
		return;
	}
	if(sei.hProcess == NULL)
	{
		return;
	}
	pid = GetProcessId(sei.hProcess);
    AllowSetForegroundWindow(pid);
}


void CoD4UpdateShutdown(int exitcode)
{
	NET_Shutdown();
	Sys_DestroyConsole();
	
	Sleep(500);
	
	if(exitcode){
		exit(exitcode);
	}
		
}

BOOL __cdecl CoD4UpdateMain(HMODULE hModule, const char* fs_write_base, const wchar_t* updatepath, const char* nullver, int numargs, WinVars_t* g_wvptr )
{

	Sys_InitializeCriticalSection();

	Sys_CheckOS();
	
	if(numargs == 1)
	{
		Sys_CreateConsole(g_wvptr->hInstance);
	}
	
	SleepEx(2500, FALSE);

	NET_Init();
	Sec_Init();
	
	FS_SetupSavePath(updatepath);
	FS_SetupInstallPath(fs_write_base);
	
	wchar_t versiondir[1024];
	char* list[1024];
	char* filteredList[1024];
	const char* filestring;
	int i, j;

	FS_BuildOSPathForThreadUni( FS_GetSavePath(), "bin", "", versiondir, 0);

	int count = Sys_ListDirectories(versiondir, list, sizeof(list) / sizeof(list[0]));

	for(i = 0, j = 0; i < count; ++i)
	{
		if(Q_stricmpn(list[i], "cod4x_", 6) == 0)
		{
			filteredList[j] = list[i];
			++j;
		}
	}
	filteredList[j] = NULL;

	qsort(filteredList, j, sizeof(filteredList[0]), Sys_SortVersionDirs);

	if(j > 0)
	{
		filestring = filteredList[j -1];
		int version = atoi(filestring + 6);
		Com_sprintf(versionstring, sizeof(versionstring), "%d.x", version);
	}else{
		Com_sprintf(versionstring, sizeof(versionstring), "1.0");
	}



	Sys_FreeFileList( list );


	//Disable folder virtualization 
    HANDLE Token;
	DWORD disable = 0;
	
    if (OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &Token)){
        SetTokenInformation(Token, (TOKEN_INFORMATION_CLASS)24, &disable, sizeof(disable));
		CloseHandle(Token);
    }
	
	if(Sec_Update( ) != 0)
	{
		Com_Printf("Update has failed\n");
		MessageBoxA(NULL, "The new update failed to install.", "Call of Duty 4 - Update Failed", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
		CoD4UpdateShutdown( 1 );
	}


	if(sys_restartCmdLine[0] == 0)
	{
		Com_Printf("Installation of updates has succeeded.\n");
		MessageBoxA(NULL, "The new update has been installed successfully. Please restart Call of Duty 4 - Modern Warfare now.", "Call of Duty 4 - Update Installed", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
		CoD4UpdateShutdown(0);

	}else{
		CoD4UpdateShutdown(0);
		Sys_RestartProcess();
	}
	return 0;
}
