/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/


#ifndef __QCOMMON_H__
#define __QCOMMON_H__

#include "q_shared.h"

//
// msg.c
//

typedef struct snapshotInfo_s{
	int clnum;
	struct client_t* cl;
	int var_01;
	qboolean var_02;
	byte var_03;
}snapshotInfo_t;


typedef struct {
	qboolean	overflowed;		//0x00
	qboolean	readonly;		//0x04
	byte		*data;			//0x08
	byte		*splitdata;		//0x0c
	int		maxsize;		//0x10
	int		cursize;		//0x14
	int		splitsize;		//0x18
	int		readcount;		//0x1c
	int		bit;			//0x20	// for bitwise reads and writes
	int		lastRefEntity;		//0x24
} msg_t; //Size: 0x28


struct clientState_s;
struct playerState_s;
struct usercmd_s;


void MSG_Init( msg_t *buf, byte *data, int length );
void MSG_InitReadOnly( msg_t *buf, byte *data, int length );
void MSG_InitReadOnlySplit( msg_t *buf, byte *data, int length, byte*, int );
void MSG_Clear( msg_t *buf ) ;
void MSG_BeginReading( msg_t *msg ) ;
void MSG_Copy(msg_t *buf, byte *data, int length, msg_t *src);
void MSG_WriteByte( msg_t *msg, int c ) ;
void MSG_WriteShort( msg_t *msg, int c ) ;
void MSG_WriteLong( msg_t *msg, int c ) ;
void MSG_WriteData( msg_t *buf, const void *data, int length ) ;
void MSG_WriteString( msg_t *sb, const char *s ) ;
void MSG_WriteBigString( msg_t *sb, const char *s ) ;
int MSG_ReadByte( msg_t *msg ) ;
int MSG_ReadShort( msg_t *msg ) ;
int MSG_ReadLong( msg_t *msg ) ;
char *MSG_ReadString( msg_t *msg, char* bigstring, int len );
char *MSG_ReadStringLine( msg_t *msg, char* bigstring, int len );
void MSG_ReadData( msg_t *msg, void *data, int len ) ;
void MSG_ClearLastReferencedEntity( msg_t *msg ) ;
void MSG_WriteDeltaEntity(struct snapshotInfo_s* snap, msg_t* msg, int time, entityState_t* from, entityState_t* to, int arg_6);
void MSG_WriteBit0( msg_t *msg ) ;
int MSG_WriteBitsNoCompress( int d, byte* src, byte* dst ,int bytecount);
void MSG_WriteVector( msg_t *msg, vec3_t c );
void MSG_Discard(msg_t* msg);


void MSG_WriteBit1(msg_t*);
void MSG_WriteBits(msg_t*, int bits, int bitcount);
int MSG_ReadBits(msg_t *msg, int numBits);
void MSG_WriteReliableCommandToBuffer(const char *source, char *destination, int length);

int __cdecl GetMinBitCount( unsigned int number );
void __cdecl MSG_WriteDeltaClient(struct snapshotInfo_s*, msg_t*, int, struct clientState_s*, struct clientState_s*, int);
void __regparm3 MSG_WriteDeltaField(struct snapshotInfo_s* , msg_t* , int, unsigned const char*, unsigned const char*, const void* netfield, int, unsigned char);
void __cdecl MSG_WriteDeltaPlayerstate(struct snapshotInfo_s* , msg_t* , int , struct playerState_s* , struct playerState_s*);
void __cdecl MSG_WriteEntityIndex(struct snapshotInfo_s*, msg_t*, int, int);
void __cdecl MSG_ReadDeltaUsercmdKey( msg_t *msg, int key, struct usercmd_s *from, struct usercmd_s *to );
/*void __usercall MSG_WriteDeltaUsercmdKey(msg_t *msg<esi>, int a2, usercmd_t *to, usercmd_t *from);*/
void __cdecl MSG_WriteDeltaUsercmdKey(msg_t *msg, int a2, usercmd_t *to, usercmd_t *from);
void MSG_SetDefaultUserCmd( struct playerState_s *ps, struct usercmd_s *ucmd );



/*
==============================================================

NET

==============================================================
*/

#define NET_ENABLEV4            0x01
#define NET_ENABLEV6            0x02
// if this flag is set, always attempt ipv6 connections instead of ipv4 if a v6 address is found.
#define NET_PRIOV6              0x04
// disables ipv6 multicast support if set.
#define NET_DISABLEMCAST        0x08


#define	PACKET_BACKUP	32	// number of old messages that must be kept on client and
							// server for delta comrpession and ping estimation
#define	PACKET_MASK		(PACKET_BACKUP-1)

#define	MAX_PACKET_USERCMDS		32		// max number of usercmd_t in a packet

#define	PORT_ANY			-1

#define	MAX_RELIABLE_COMMANDS	128			// max string commands buffered for restransmit
#define MAX_PACKETLEN           1400        // max size of a network packet

typedef enum {
	NA_BAD = 0,					// an address lookup failed
	NA_BOT = 0,
	NA_LOOPBACK = 2,
	NA_BROADCAST = 3,
	NA_IP = 4,
	NA_IP6 = 5,
	NA_MULTICAST6 = 6,
	NA_UNSPEC = 7,
	NA_DOWN = 8,
} netadrtype_t;

typedef enum {
	NS_CLIENT,
	NS_SERVER
} netsrc_t;

#define NET_ADDRSTRMAXLEN 48	// maximum length of an IPv6 address string including trailing '\0'

typedef struct {
	byte	type;
	byte	scope_id;
	unsigned short	port;
        union{
	    byte	ip[4];
	    byte	ipx[10];
	    byte	ip6[16];
	};
}netadr_t;
/* For legacy stuff conversion this structs */
typedef enum {
	LEGACY_NA_BOT = 0,
	LEGACY_NA_BAD = 1,
	LEGACY_NA_LOOPBACK = 2,
	LEGACY_NA_BROADCAST = 3,
	LEGACY_NA_IP = 4,
	LEGACY_NA_IPX = 5,
	LEGACY_NA_BROADCAST_IPX = 6
}legacy_netadrtype_t;

typedef struct {
	legacy_netadrtype_t	type;
    byte	ip[4];
	unsigned short	port;
    byte	ipx[10];
}legacy_netadr_t;

void		NET_Init( void );
void		NET_Shutdown( void );
void		NET_Restart_f( void );
void		NET_Config( qboolean enableNetworking );
void		NET_FlushPacketQueue(void);
qboolean	NET_SendPacket (netsrc_t sock, int length, const void *data, netadr_t* to);
void		NET_OutOfBandPrint( netsrc_t net_socket, netadr_t *adr, const char *format, ...) __attribute__ ((format (printf, 3, 4)));
void		QDECL NET_OutOfBandData( netsrc_t sock, netadr_t *adr, byte *format, int len );

qboolean	NET_CompareAdr (netadr_t *a, netadr_t *b);
qboolean	NET_CompareBaseAdrMask(netadr_t *a, netadr_t *b, int netmask);
qboolean	NET_CompareBaseAdr (netadr_t *a, netadr_t *b);
int NET_CompareAdrSigned(netadr_t *a, netadr_t *b);
qboolean	NET_IsLocalAddress (netadr_t adr);
const char	*NET_AdrToStringwPort (netadr_t a);
int		NET_StringToAdr ( const char *s, netadr_t *a, netadrtype_t family);
qboolean	NET_GetLoopPacket (netsrc_t sock, netadr_t *net_from, msg_t *net_message);
void		NET_JoinMulticast6(void);
void		NET_LeaveMulticast6(void);
void		NET_Sleep(int msec);
const char*	NET_AdrToStringShort(netadr_t* adr);
const char*	NET_AdrToString(netadr_t* adr);
qboolean	NET_GetPacket(netadr_t *net_from, msg_t *net_message, netadrtype_t type);
qboolean	Sys_SendPacket( int length, const void *data, netadr_t *to );
qboolean	Sys_StringToAdr( const char *s, netadr_t *a, netadrtype_t family );

//Does NOT parse port numbers, only base addresses.
qboolean	Sys_IsLANAddress (netadr_t adr);
void		Sys_ShowIP(void);

/* TCP Client */
int NET_TcpReceiveData( int sock, msg_t* msg);
int NET_TcpSendData( int sock, const void *data, int length );
int NET_TcpClientConnect( const char *remoteAdr );
void NET_TcpCloseSocket(int socket);

/* NET-Chan */

typedef struct{
	char command[MAX_STRING_CHARS];
	int cmdTime;
	int cmdType;
}reliableCmd_t;

typedef struct {
	// sequencing variables
	int			outgoingSequence;
	netsrc_t		sock;
	int			dropped;			// between last packet and previous
	int			incomingSequence;

	//Remote address
	netadr_t	remoteAddress;		// (0x10)
	int			qport;				// qport value to write when transmitting (0x24)

	// incoming fragment assembly buffer
	int			fragmentSequence;
	int			fragmentLength;	
	byte		*fragmentBuffer; // (0x30)
	int			fragmentBufferSize;

	// outgoing fragment buffer
	// we need to space out the sending of large fragmented messages
	qboolean	unsentFragments;
	int			unsentFragmentStart;
	int			unsentLength;
	byte		*unsentBuffer; //(0x44)
	int			unsentBufferSize;
	char		NetProfiling_is_free[1504];
} netchan_t;


void Netchan_Init( int port );
void Netchan_Setup( netsrc_t sock, netchan_t *chan, netadr_t *adr, int qport , byte* unsentBuffer, int unsentBufferSize, byte* fragmentBuffer, int fragmentBufferSize);
qboolean Netchan_Process( netchan_t *chan, msg_t *msg );
qboolean Netchan_TransmitNextFragment( netchan_t *chan );
qboolean Netchan_Transmit( netchan_t *chan, int length, const byte *data );

extern cvar_t *showpackets;
extern cvar_t *net_lanauthorize;
#define MAX_MSGLEN 0x20000

/*
==============================================================

PROTOCOL

==============================================================
*/

#define	LEGACY_PROTOCOL_VERSION	6
#define	PROTOCOL_VERSION	7
// 1.31 - 67

#define MAX_UPDATE_SERVERS  5
#define	UPDATE_SERVER_NAME	"cod4update.iceops.in"
// override on command line, config files etc.
#ifndef MASTER_SERVER_NAME
#define MASTER_SERVER_NAME	"cod4master.activision.com"
#endif

#ifndef AUTHORIZE_SERVER_NAME
  #define	AUTHORIZE_SERVER_NAME	"cod4master.activision.com"
#endif
#ifndef PORT_AUTHORIZE
  #define	PORT_AUTHORIZE		20800
#endif


#define	PORT_MASTER			20810
#define	PORT_UPDATE			27951
#define	PORT_SERVER			28960
#define	NUM_SERVER_PORTS	4		// broadcast scan this many ports after
									// PORT_SERVER so a single machine can
									// run multiple servers


// the svc_strings[] array in cl_parse.c should mirror this
//
// server to client
//
enum svc_ops_e {
	svc_nop,
	svc_gamestate,
	svc_configstring,           // [short] [string] only in gamestate messages
	svc_baseline,               // only in gamestate messages
	svc_serverCommand,          // [string] to be executed by client game module
	svc_download,               // [short] size [size bytes]
	svc_snapshot,
	svc_EOF
};

//
// client to server
//
enum clc_ops_e {
	clc_move,				// [[usercmd_t]
	clc_moveNoDelta,		// [[usercmd_t]
	clc_clientCommand,		// [string] message
	clc_EOF,
	clc_nop,
	
	// clc_extension follows a clc_EOF, followed by another clc_* ...
	//  this keeps legacy servers compatible.
	clc_extension,
	clc_voip   // not wrapped in USE_VOIP, so this value is reserved.
};


#define CMD_BACKUP	128
#define CMD_MASK	( CMD_BACKUP -1 )

/*
==============================================================

Common functions

==============================================================
*/

// centralizing the declarations for cl_cdkey
// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=470
extern char cl_cdkey[34];

// centralized and cleaned, that's the max string you can send to a Com_Printf / Com_DPrintf (above gets truncated)
#define	MAXPRINTMSG	4096


typedef enum {
	// SE_NONE must be zero
	SE_NONE = 0,	// evTime is still valid
	SE_KEY,			// evValue is a key code, evValue2 is the down flag
	SE_CHAR,		// evValue is an ascii char
	SE_CONSOLE		// evPtr is a char*
} sysEventType_t;

typedef struct {
	int				evTime;
	sysEventType_t	evType;
	int				evValue, evValue2;
	int				evPtrLength;	// bytes of data pointed to by evPtr, for journaling
	void			*evPtr;			// this must be manually freed if not NULL
} sysEvent_t;

typedef enum{
    MSG_DEFAULT,
    MSG_NA,	//Not defined
    MSG_WARNING,
    MSG_ERROR,
    MSG_NORDPRINT
}msgtype_t;

#define __QCOMMON_STDIO_H__

void		Com_Init_Try_Block(char* cmdline);
//void		Com_QueueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );
int			Com_EventLoop( void );
sysEvent_t	Com_GetSystemEvent( void );

char		*CopyString( const char *in );
void		Info_Print( const char *s );

void		Com_BeginRedirect (char *buffer, int buffersize, void (*flush)(char *));
void		Com_EndRedirect( void );
void		Com_PrintMessage( int output, char *msg, msgtype_t type);
void 		QDECL Com_Error( int code, const char *fmt, ... ) __attribute__ ((format (printf, 2, 3)));
void		Com_PrintMessage( int output, char *msg, msgtype_t type);
void		Com_GameRestart(int checksumFeed, qboolean clientRestart);

//int			Com_Milliseconds( void );	// will be journaled properly
unsigned	Com_BlockChecksum( const void *buffer, int length );
char		*Com_MD5File(const char *filename, int length, const char *prefix, int prefix_len);
int			Com_Filter(char *filter, char *name, int casesensitive);
int			Com_FilterPath(char *filter, char *name, int casesensitive);
int			Com_RealTime(qtime_t *qtime);
qboolean	Com_SafeMode( void );
void		Com_RunAndTimeServerPacket(netadr_t *evFrom, msg_t *buf);
void 		Com_Quit_f(void);
int			Com_HashKey( char *string, int maxlen );
void		Com_QueueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );
void		Com_Init();
void		Com_Frame();
qboolean	Com_HasPlayerProfile();
qboolean	Com_IsLegacyServer();
void		Com_Restart();
void Com_SetErrorMessage(const char* errormsg);
void Com_SetRecommended(int, int);
void Com_SetDedicatedMode( );
void Com_LoadPlayerProfile(const char *profilename);
qboolean Com_PlayerProfileExists(const char* profilename);
void __cdecl FS_ShutdownReferencedFiles(int *numServerFiles, char **strings);

extern fileHandle_t logfile;
extern cvar_t *com_version;
extern cvar_t *com_legacyProtocol;
extern cvar_t *com_ui_debug;
extern cvar_t *com_dedicated;

#define com_useFastFiles getcvaradr(0x1435d78)
#define com_developer getcvaradr(0x1435d20)
#define com_logfile getcvaradr(0x1435d44)
#define com_sv_running getcvaradr(0x1435d60)
#define com_playerProfile getcvaradr(0x1435d1c)
#define com_animCheck getcvaradr(0x1476EE0)
#define com_maxfps getcvaradr(0x1476EF8)
#define com_fixedtime getcvaradr(0x1435D40)
#define com_timescale getcvaradr(0x1435D4C)
#define sys_lockThreads getcvaradr(0x1476EE8)
#define dev_timescale getcvaradr(0x1476EEC)
#define com_maxFrameTime getcvaradr(0x1435D24)
#define cl_paused getcvaradr(0x1435D48)

//void		Com_StartupVariable( const char *match );
// checks for and removes command line "+set var arg" constructs
// if match is NULL, all set commands will be executed, otherwise
// only a set with the exact name.  Only used during startup.



typedef void *xcommand_t;
int	Cmd_Argc( void );
const char*	Cmd_Argv( int argv );
void Cmd_TokenizeString(const char *line);
void Cmd_EndTokenizedString( );
void    Cmd_AddCommand( const char *cmd_name, xcommand_t function );
void __cdecl Cmd_ExecuteSingleCommand(int, int, const char*);
void    Cmd_AddServerCommand( const char *cmd_name, xcommand_t function );
void Cmd_SetAutoComplete( const char *cmd_name, const char *arg_1, const char *arg_2);
char* Cmd_Argsv( int arg, char * buff, int bufsize );
void Cmd_Shutdown();
void Cmd_RemoveCommand(const char* cmd);
#define Cmd_Stub_f (void*)0x4d74b0
/*
==============================================================

Huffman compressor

==============================================================
*/

#define __HUFFMAN_H__

#define NYT HMAX                    /* NYT = Not Yet Transmitted */
#define INTERNAL_NODE ( HMAX + 1 )

typedef struct nodetype {
	struct  nodetype *left, *right, *parent; /* tree structure */
//	struct  nodetype *next, *prev; /* doubly-linked list */
//	struct  nodetype **head; /* highest ranked node in block */
	int weight;
	int symbol; //0x10
//	struct  nodetype *next, *prev; /* doubly-linked list */
//	struct  nodetype **head; /* highest ranked node in block */

} node_t; //Length: 20

#define HMAX 256 /* Maximum symbol */

typedef struct {
	int blocNode;
	int blocPtrs;

	node_t*     tree;
	node_t*     loc[HMAX + 1];
	node_t**    freelist;

	node_t nodeList[768];
	node_t*     nodePtrs[768];

} huff_t;
/* size 19476*/

int MSG_ReadBitsCompress(const byte* input, byte* outputBuf, int readsize);
int MSG_WriteBitsCompress( char dummy, const byte *datasrc, byte *buffdest, int bytecount);
void MSG_InitHuffman();



/*
==============================================================

FILESYSTEM

No stdio calls should be used by any part of the game, because
we need to deal with all sorts of directory and seperator char
issues.
==============================================================
*/

// referenced flags
// these are in loop specific order so don't change the order
#define FS_GENERAL_REF	0x01
#define FS_UI_REF		0x02
#define FS_CGAME_REF	0x04
#define FS_QAGAME_REF	0x08
// number of id paks that will never be autodownloaded from baseq3/missionpack
#define NUM_ID_PAKS		9
#define NUM_TA_PAKS		4

#define	MAX_FILE_HANDLES	64

#ifdef DEDICATED
#	define Q3CONFIG_CFG "q3config_server.cfg"
#else
#	define Q3CONFIG_CFG "q3config.cfg"
#endif

qboolean FS_Initialized( void );

//void	FS_InitFilesystem ( void );
void	FS_Shutdown( );

qboolean	FS_ConditionalRestart( int checksumFeed );
void	FS_Restart( int checksumFeed );
// shutdown and restart the filesystem so changes to fs_gamedir can take effect

void FS_AddGameDirectory( const char *path, const char *dir );

char	**FS_ListFiles( const char *directory, const char *extension, int *numfiles );
// directory should not have either a leading or trailing /
// if extension is "/", only subdirectories will be returned
// the returned files will not include any directories or /

void	FS_FreeFileList( char **list );

qboolean FS_FileExists( const char *file );

qboolean FS_CreatePath (char *OSPath);

char *FS_FindDll( const char *filename );

char   *FS_BuildOSPath( const char *base, const char *game, const char *qpath );
qboolean FS_CompareZipChecksum(const char *zipfile);

int		FS_LoadStack( void );

int		FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );
int		FS_GetModList(  char *listbuf, int bufsize );

fileHandle_t	FS_FOpenFileWrite( const char *qpath );
fileHandle_t	FS_FOpenFileAppend( const char *filename );
fileHandle_t	FS_FCreateOpenPipeFile( const char *filename );
// will properly create any needed paths and deal with seperater character issues

fileHandle_t FS_SV_FOpenFileWrite( const char *filename );
int		FS_SV_FOpenFileRead( const char *filename, fileHandle_t *fp );
void	FS_SV_Rename( const char *from, const char *to );
int __cdecl FS_FOpenFileRead(const char* filename, fileHandle_t* returnhandle);
qboolean FS_SV_FileExists( const char *filename );
void FS_SV_Remove( const char *filename );
// if uniqueFILE is true, then a new FILE will be fopened even if the file
// is found in an already open pak file.  If uniqueFILE is false, you must call
// FS_FCloseFile instead of fclose, otherwise the pak FILE would be improperly closed
// It is generally safe to always set uniqueFILE to true, because the majority of
// file IO goes through FS_ReadFile, which Does The Right Thing already.

int		FS_FileIsInPAK(const char *filename, int *pChecksum );
// returns 1 if a file is in the PAK file, otherwise -1

int		FS_Write( const void *buffer, int len, fileHandle_t f );

int		FS_Read2( void *buffer, int len, fileHandle_t f );
int		FS_Read( void *buffer, int len, fileHandle_t f );
// properly handles partial reads and reads from other dlls

void	FS_FCloseFile( fileHandle_t f );
// note: you can't just fclose from another DLL, due to MS libc issues

int		FS_ReadFile( const char *qpath, void **buffer );

// returns the length of the file
// a null buffer will just return the file length without loading
// as a quick check for existance. -1 length == not present
// A 0 byte will always be appended at the end, so string ops are safe.
// the buffer should be considered read-only, because it may be cached
// for other uses.

void	FS_ForceFlush( fileHandle_t f );
// forces flush on files we're writing to.

void	FS_FreeFile( void *buffer );
// frees the memory returned by FS_ReadFile

void	FS_WriteFile( const char *qpath, const void *buffer, int size );
// writes a complete file, creating any subdirectories needed

int		FS_filelength( fileHandle_t f );
// doesn't work for files that are opened from a pack file

int		FS_FTell( fileHandle_t f );
// where are we?

void	FS_Flush( fileHandle_t f );

void 	QDECL FS_Printf( fileHandle_t f, const char *fmt, ... ) __attribute__ ((format (printf, 2, 3)));
// like fprintf

int		FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode );
// opens a file for reading, writing, or appending depending on the value of mode

int		FS_Seek( fileHandle_t f, long offset, int origin );
// seek on a file (doesn't work for zip files!!!!!!!!)

qboolean FS_FilenameCompare( const char *s1, const char *s2 );

const char *FS_GamePureChecksum( void );
// Returns the checksum of the pk3 from which the server loaded the qagame.qvm

const char *FS_LoadedPakNames( void );
const char *FS_LoadedPakChecksums( void );
const char *FS_LoadedPakPureChecksums( void );
// Returns a space separated string containing the checksums of all loaded pk3 files.
// Servers with sv_pure set will get this string and pass it to clients.

const char *FS_ReferencedPakNames( void );
const char *FS_ReferencedPakChecksums( void );
const char *FS_ReferencedPakPureChecksums( void );
// Returns a space separated string containing the checksums of all loaded 
// AND referenced pk3 files. Servers with sv_pure set will get this string 
// back from clients for pure validation 

void FS_ClearPakReferences( int flags );
// clears referenced booleans on loaded pk3s

void FS_PureServerSetReferencedPaks( const char *pakSums, const char *pakNames );
void FS_PureServerSetLoadedPaks( const char *pakSums, const char *pakNames );
// If the string is empty, all data sources will be allowed.
// If not empty, only pk3 files that match one of the space
// separated checksums will be checked for files, with the
// sole exception of .cfg files.

qboolean FS_CheckDirTraversal(const char *checkdir);
qboolean FS_idPak(char *pak, char *base, int numPaks);
qboolean FS_ComparePaks( char *neededpaks, qboolean dlstring );
qboolean REGPARM(1) FS_WriteFileToDir( const char* qpath, void* buf, int len );
void FS_Rename( const char *from, const char *to );

void FS_Remove( const char *osPath );
void FS_HomeRemove( const char *homePath );

void	FS_FilenameCompletion( const char *dir, const char *ext,
		qboolean stripExt, void(*callback)(const char *s), qboolean allowNonPureFilesOnDisk );

const char *FS_GetCurrentGameDir(void);

void FS_CopyFile( char *fromOSPath, char *toOSPath );
void FS_DisablePureCheck(int state);
void FS_ShiftStr( const char *string, int shift, char *buf );
void FS_ReplaceSeparators( char *path );
void FS_BuildOSPathForThread(const char *base, const char *game, const char *qpath, char *fs_path, int fs_thread);
void FS_BuildOSPathForThreadUni(const wchar_t *base, const char *game, const char *qpath, wchar_t *fs_path, int fs_thread);
void FS_RenameOSPath( const char *from_ospath, const char *to_ospath );
//void FS_StripTrailingSeperator( char *path );
qboolean FS_FileExistsOSPath( const char *osPath );
void FS_RemoveOSPath( const char *osPath ) ;
qboolean FS_WriteTestOSPath( const char *osPath );
int FS_SV_WriteFileToSavePath( const char *qpath, const void *buffer, int size );
char* FS_GetInstallPath();
wchar_t* FS_GetInstallPathUni(wchar_t* buf, int size);
wchar_t* FS_GetSavePath();
void FS_RenameOSPathUni( const wchar_t *from_ospath, const wchar_t *to_ospath );
qboolean FS_FileExistsOSPathUni( const wchar_t *osPath );
#define fs_gameDirVar getcvaradr(0xcb199a4)
#define fs_homepath getcvaradr(0xCB1DCC0)
//#define fs_basepath getcvaradr(0xcb199a4)

void Cbuf_AddText(const char* );
void Cbuf_ExecuteBuffer(int, int, const char*);
void Cbuf_Execute(  );

void Sys_Print( const char *msg );
char *Sys_ConsoleInput( void );
void Sys_EnterCriticalSection( );
void Sys_LeaveCriticalSection( );
int Sys_Milliseconds(void);
int Sys_TimeGetTime();
const void* Sys_GetValue(int key);

void PbSvAddEvent(int commandtype, int clientNum ,int stringlen, const char* cmdstring);
void PbClAddEvent(int commandtype, int stringlen, const char* cmdstring);
void PbClientConnecting(int arg_1, char* string, int* size);

char* SEH_LocalizeTextMessage(char*, char*, int);

void Scr_UpdateLoadScreen( );

void SND_StopSounds(int);

int SV_Frame(int);
void SV_KillLocalServer();

void Con_Close(int);
void DB_SyncXAssets();
void DB_PostLoadXZone();
void Com_ErrorCleanUp();
void sub_5F78A0();
void CL_StartHunkUsers();
void R_SyncRenderThread();
signed int Int_Parse(char** args, int *);
void Com_WriteConfiguration();
void Win_UpdateThreadLock();
void Ragdoll_Update(int msec);
void StatMon_Frame();
void R_WaitEndTime(unsigned int frameTime);
void SCR_UpdateScreen( );
void Sys_LoadingKeepAlive( );
void Scr_UpdateFrame( );
void R_WaitWorkerCmds(); //0x500740
void SV_Shutdown(char*);
void Com_Close();
void sub_518940();
void Hunk_Clear();
void Sys_Quit( );
void Sys_NormalExit();
void Sys_HideSplashWindow();

#pragma pack(push, 1)
typedef struct
{
  int dword0;
  int dword4;
  int dword8;
  int dwordC;
  int dword10;
  int dword14;
  const char *dword18;
  const char *dword1C;
  const char *dword20;
  const char *dword24;
  const char *dword28;
  const char *dword2C;
}GfxConfiguration_t;
#pragma pack(pop)

void R_MakeDedicated(GfxConfiguration_t*);
byte DB_ModFileExists();
void Sys_DestroySplashWindow();
void Sys_DestroyConsole();

void SV_AddDedicatedCommands();
void IN_Shutdown();
void Key_Shutdown();
void Dvar_Shutdown();
void Sys_SetRestartParams(const char* params);
const char* Sys_ExeFile( void );
void Com_SetErrorEntered();
void Sys_SuspendOtherThreads();
void sub_55A170();
void __cdecl Sys_Error(const char *fmt, ...);
void Sys_Mkdir(const char* dir);

void* __cdecl Z_Malloc(int bytes);
void __cdecl Z_Free(void* ptr);
void Sys_RunUpdater( char* autoupdateFilenameArg );
void Sys_SetupUpdater(  );
const char* Com_GetVersion();
int Sec_Update( );


#define cod4xpem "-----BEGIN PUBLIC KEY-----\n\
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAwu8nEsLD4sTP+Py30Fnq\n\
UOlgZZrGb7aIiQhn8iXAXXuhLC0pKOQ2drq3KWMbHeiNSAaxI2TGRirYCiZETnkX\n\
WCt0NxvrGtbvbsDHBaVju/5X9CiyJBFr+YFhZ8RK/UH8KxMqIAlvN5f3H30rPqwB\n\
QlI+scIXp5ZrFt97zaYw4czpWod4iZVm4O8fNJJAFq9qR2yxVyKaP7DZr3wZEt1+\n\
WJrOmkWPYkNC/YC1qnY35ubDAS7vZPvPtmw4oeJKSsTFwR5ddKMiLvPzRW3KgpT1\n\
B4zHBTO1xOKTYvEQqJqspz1ELUeSPemEYmZEZdakVLDKyzPZ5+a0WR4q3pDtmrZG\n\
KwIDAQAB\n\
-----END PUBLIC KEY-----"


#endif