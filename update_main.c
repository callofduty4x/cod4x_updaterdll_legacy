#define WIN32_LEAN_AND_MEAN

#include "q_shared.h"

#include <windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <direct.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <mmsystem.h>
#include "blake\blake2.h"

void (*Com_Printf)(const char* fmt, ...);
#define PATH_SEP '\\'



/*
=============
Com_Error

Both client and server can use this, and it will
do the appropriate thing.
=============
*/
void Com_Error( const char *fmt, ... ) {
	va_list		argptr;
	char com_errorMessage[2048];

	va_start (argptr,fmt);
	Q_vsnprintf (com_errorMessage, sizeof(com_errorMessage),fmt,argptr);
	va_end (argptr);

	MessageBoxA(NULL, com_errorMessage, "Call of Duty 4 - Update has Failed", MB_OK | MB_ICONERROR);
	exit( -1 );
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

	MessageBoxW(NULL, com_errorMessage, L"Call of Duty 4 - Update has Failed", MB_OK | MB_ICONERROR);
	exit( -1 );
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
		Com_Error( "Short read in FS_Copyfiles()\n" );
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
		Com_Error( "Short write in FS_Copyfiles()\n" );
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
			Com_Error ("FS_ReadOSPath: -1 bytes read");
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
		Com_Error( "FS_ReadFile with empty name\n" );
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

qboolean FS_FileHashUni(const wchar_t* ospath, char* outhash, int lenhash)
{
	byte	buf[0x20000];
	int		len, readlen, i;
	FILE*   h;
	blake2s_state S;
	uint8_t hash[BLAKE2S_OUTBYTES];
	  
	if ( !ospath || !ospath[0] ) {
		Com_Error( "FS_FileHash with empty name\n" );
	}
	
	if ( !outhash || lenhash < 2*BLAKE2S_OUTBYTES +1 ) {
		Com_Error( "FS_FileHash with invalid argument\n" );
	}	
		
	// look for it in the filesystem or pack files
	len = FS_FOpenFileReadOSPathUni( ospath, &h );
	if ( len == -1 ) {
		return qfalse;
	}

    if( blake2s_init( &S, sizeof(hash) ) < 0 )
	{
		Com_Error("blake2s_init() has failed");
	}
	
	do
	{
		readlen = FS_ReadOSPath (buf, sizeof(buf), h);
		blake2s_update( &S, ( uint8_t * )buf, readlen );
	}while(readlen > 0);

	blake2s_final( &S, hash, sizeof(hash) );
	
	FS_FCloseFileOSPath( h );

	for( i = 0; i < BLAKE2S_OUTBYTES; i++)
	{
		sprintf(&outhash[2*i], "%02x", hash[i] );
	}
	outhash[2*i] = '\0';
	
	return qtrue;
}

qboolean FS_FileHash(const char* ospath, char* outhash, int lenhash)
{
	byte	buf[0x20000];
	int		len, readlen, i;
	FILE*   h;
	blake2s_state S;
	uint8_t hash[BLAKE2S_OUTBYTES];
	  
	if ( !ospath || !ospath[0] ) {
		Com_Error( "FS_FileHash with empty name\n" );
	}
	
	if ( !outhash || lenhash < 2*BLAKE2S_OUTBYTES +1 ) {
		Com_Error( "FS_FileHash with invalid argument\n" );
	}	
		
	// look for it in the filesystem or pack files
	len = FS_FOpenFileReadOSPath( ospath, &h );
	if ( len == -1 ) {
		return qfalse;
	}

    if( blake2s_init( &S, sizeof(hash) ) < 0 )
	{
		Com_Error("blake2s_init() has failed");
	}
	
	do
	{
		readlen = FS_ReadOSPath (buf, sizeof(buf), h);
		blake2s_update( &S, ( uint8_t * )buf, readlen );
	}while(readlen > 0);

	blake2s_final( &S, hash, sizeof(hash) );
	
	FS_FCloseFileOSPath( h );

	for( i = 0; i < BLAKE2S_OUTBYTES; i++)
	{
		sprintf(&outhash[2*i], "%02x", hash[i] );
	}
	outhash[2*i] = '\0';
	
	return qtrue;
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
char *Cmd_Argv( int arg ) {
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

BOOL __cdecl CoD4UpdateMain(HMODULE hModule, const char* dllpath, const char* fs_write_base, const wchar_t* updatepath, const char* filelist, void (*sys_printf)(const char* fmt, ...), const char* dlhashs )
{

	int i;
	Com_Printf = sys_printf;
	wchar_t from[1024];
	wchar_t filenameUni[1024];
	char to[1024];
	wchar_t touni[1024];
	char backup[1024];
	char copybase[1024];
	wchar_t copyupd[1024];
	char outhash[256];
	MSG msg;
	
	Q_strncpyz(copybase, fs_write_base, sizeof(copybase));
	Q_strncpyzUni(copyupd, updatepath, sizeof(copyupd));
	
	FS_StripTrailingSeperator( copybase );
	FS_StripTrailingSeperatorUni( copyupd );
	
	SV_Cmd_TokenizeString( dlhashs );
	
	if(SV_Cmd_Argc() & 1)
	{
		Com_Error("File hash list is corrupted\n");	
	}

	
	/* Verify all downloaded files */
	for(i = 0; i < SV_Cmd_Argc(); i += 2)
	{
		if ( GetMessage( &msg, NULL, 0, 0 ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		
		Q_StrToWStr(filenameUni , SV_Cmd_Argv(i), sizeof(filenameUni));
		
		Com_sprintfUni(from, sizeof(from), L"%s%c%s", copyupd, PATH_SEPUNI, filenameUni);
		
		Q_strchrreplUni(from, L'/', PATH_SEPUNI);
		Com_Printf("Verifying file: %s\n", SV_Cmd_Argv(i));
		if( FS_FileHashUni( from, outhash, sizeof(outhash) ) == qfalse)
		{
			Com_ErrorUni(L"Autoupdate has failed. Couldn't read file: %s", from);
		}
		
		if(strcmp( outhash, SV_Cmd_Argv(i+1)))
		{
			Com_Printf("Expected hash: %s, Received hash: %s\n", SV_Cmd_Argv(i+1), outhash);
			Com_ErrorUni( L"Autoupdate has failed. Invalid hash of downloaded file %s", from );
		}
	}
	
	Com_Printf("All files have been verified. Installing update...\n");
	
	
	/* Disable folder virtualization */
    HANDLE Token;
	DWORD disable;
	
    if (OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &Token)){
        SetTokenInformation(Token, (TOKEN_INFORMATION_CLASS)24, &disable, sizeof(disable));
		CloseHandle(Token);
    }
	
	
	for(i = 0; i < SV_Cmd_Argc(); i += 2)
	{
		if ( GetMessage( &msg, NULL, 0, 0 ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		
		Q_StrToWStr(filenameUni , SV_Cmd_Argv(i), sizeof(filenameUni));
		
		Com_sprintfUni(from, sizeof(from), L"%s%c%s", copyupd, PATH_SEPUNI, filenameUni);
		Q_strchrreplUni(from, L'/', PATH_SEPUNI);
		
		if(!strcmp(SV_Cmd_Argv(i), "mss32.dl_")){
			
			Com_sprintf(backup, sizeof( backup ), "%s.old", dllpath);			
			Com_sprintf(to, sizeof(to), "%s", dllpath);
			
		}else{

			Com_sprintf(backup, sizeof( backup ), "%s%c%s.old", copybase, PATH_SEP, SV_Cmd_Argv(i));
			Q_strchrrepl(backup, '/', PATH_SEP);
			Com_sprintf(to, sizeof(to), "%s%c%s", copybase, PATH_SEP, SV_Cmd_Argv(i));		
			Q_strchrrepl(to, '/', PATH_SEP);
		}
		
		Com_Printf( "Installing updatefile %s\n", Cmd_Argv(i) );
		
		FS_RemoveOSPath( backup );
		
		if(FS_FileExistsOSPath( backup ) == qtrue)
		{
			Com_Error("Autoupdate has failed. Couldn't delete file: %s", backup);
			return FALSE;
		}
		
		FS_RenameOSPath( to, backup );
		FS_RemoveOSPath( to );
		
		if(FS_FileExistsOSPath( to ) == qtrue)
		{
			Com_Error("Autoupdate has failed. Couldn't delete file: %s", to);
			return FALSE;
		}
		
		Q_StrToWStr(touni , to, sizeof(touni));

		FS_RenameOSPathUni( from, touni );
		
		if(FS_FileExistsOSPath( to ) == qfalse)
		{
			Com_Error("Autoupdate has failed. Your game installation is maybe corrupted now. Couldn't install file to: %s\nIf you game installation does not work anymore you have to manually install all update files.", to);
			return FALSE;
		}
	}

	Com_Printf("Installation of updates has succeeded.\n");

/*
	unsigned int maxwait;
	
	maxwait = Sys_MillisecondsRaw() + 5000;
	do{
		if ( !GetMessage( &msg, NULL, 0, 0 ) ) {
			break;
		}
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}while( Sys_MillisecondsRaw() < maxwait );
*/



	MessageBoxA(NULL, "The new update has been installed successfully. Please restart Call of Duty 4 - Modern Warfare now.", "Call of Duty 4 - Update Installed", MB_OK | MB_ICONINFORMATION);
	exit( 0 );
	//return TRUE;
}
