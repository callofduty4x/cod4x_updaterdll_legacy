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
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "q_shared.h"
#include "qcommon.h"

#ifdef _WIN32
#	include <winsock2.h>
#	include <ws2tcpip.h>
/*
#	if WINVER < 0x501
#		ifdef __MINGW32__
			// wspiapi.h isn't available on MinGW, so if it's
			// present it's because the end user has added it
			// and we should look for it in our tree
#			include "wspiapi.h"
#		else
#			include <wspiapi.h>
#		endif
#	else
*/
#		include <ws2spi.h>
//#	endif

typedef int socklen_t;
#	ifdef ADDRESS_FAMILY
#		define sa_family_t	ADDRESS_FAMILY
#	else
typedef unsigned short sa_family_t;
#	endif

#	define EAGAIN					WSAEWOULDBLOCK
#	define EADDRNOTAVAIL	WSAEADDRNOTAVAIL
#	define EAFNOSUPPORT		WSAEAFNOSUPPORT
#	define ECONNRESET			WSAECONNRESET
#	define EINPROGRESS		WSAEINPROGRESS
typedef u_long	ioctlarg_t;
#	define socketError		WSAGetLastError( )

#	define NET_NOSIGNAL 0x0

static WSADATA	winsockdata;
static qboolean	winsockInitialized = qfalse;

#else

#	if MAC_OS_X_VERSION_MIN_REQUIRED == 1020
		// needed for socklen_t on OSX 10.2
#		define _BSD_SOCKLEN_T_
#	endif

#	ifdef MACOS_X
#		define NET_NOSIGNAL SO_NOSIGPIPE
#	else
#		define NET_NOSIGNAL MSG_NOSIGNAL
#	endif

#	include <sys/socket.h>
#	include <errno.h>
#	include <netdb.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <net/if.h>
#	include <sys/ioctl.h>
#	include <sys/types.h>
#	include <sys/time.h>
#	include <unistd.h>
#	if !defined(__sun) && !defined(__sgi)
#		include <ifaddrs.h>
#	endif

#	ifdef __sun
#		include <sys/filio.h>
#	endif

typedef int SOCKET;
#	define INVALID_SOCKET		-1
#	define SOCKET_ERROR			-1
#	define closesocket			close
#	define ioctlsocket			ioctl
typedef int	ioctlarg_t;
#	define socketError			errno

#endif

static int networkingEnabled = 0;

static int	net_enabled;


//=============================================================================


/*
====================
NET_ErrorString
====================
*/
char *NET_ErrorString( void ) {
#ifdef _WIN32
	//FIXME: replace with FormatMessage?
	switch( socketError ) {
		case WSAEINTR: return "WSAEINTR";
		case WSAEBADF: return "WSAEBADF";
		case WSAEACCES: return "WSAEACCES";
		case WSAEDISCON: return "WSAEDISCON";
		case WSAEFAULT: return "WSAEFAULT";
		case WSAEINVAL: return "WSAEINVAL";
		case WSAEMFILE: return "WSAEMFILE";
		case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
		case WSAEINPROGRESS: return "WSAEINPROGRESS";
		case WSAEALREADY: return "WSAEALREADY";
		case WSAENOTSOCK: return "WSAENOTSOCK";
		case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
		case WSAEMSGSIZE: return "WSAEMSGSIZE";
		case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
		case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
		case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
		case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
		case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
		case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
		case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
		case WSAEADDRINUSE: return "WSAEADDRINUSE";
		case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
		case WSAENETDOWN: return "WSAENETDOWN";
		case WSAENETUNREACH: return "WSAENETUNREACH";
		case WSAENETRESET: return "WSAENETRESET";
		case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
		case WSAECONNRESET: return "WSAECONNRESET";
		case WSAENOBUFS: return "WSAENOBUFS";
		case WSAEISCONN: return "WSAEISCONN";
		case WSAENOTCONN: return "WSAENOTCONN";
		case WSAESHUTDOWN: return "WSAESHUTDOWN";
		case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
		case WSAETIMEDOUT: return "WSAETIMEDOUT";
		case WSAECONNREFUSED: return "WSAECONNREFUSED";
		case WSAELOOP: return "WSAELOOP";
		case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
		case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
		case WSASYSNOTREADY: return "WSASYSNOTREADY";
		case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
		case WSANOTINITIALISED: return "WSANOTINITIALISED";
		case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
		case WSATRY_AGAIN: return "WSATRY_AGAIN";
		case WSANO_RECOVERY: return "WSANO_RECOVERY";
		case WSANO_DATA: return "WSANO_DATA";
		default: return "NO ERROR";
	}
#else
	return strerror(socketError);
#endif
}

static void NetadrToSockadr( netadr_t *a, struct sockaddr *s ) {
	if( a->type == NA_BROADCAST ) {
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_port = a->port;
		((struct sockaddr_in *)s)->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if( a->type == NA_IP ) {
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_addr.s_addr = *(int *)&a->ip;
		((struct sockaddr_in *)s)->sin_port = a->port;
	}
	else if( a->type == NA_IP6 ) {
		((struct sockaddr_in6 *)s)->sin6_family = AF_INET6;
		((struct sockaddr_in6 *)s)->sin6_addr = * ((struct in6_addr *) &a->ip6);
		((struct sockaddr_in6 *)s)->sin6_port = a->port;
		((struct sockaddr_in6 *)s)->sin6_scope_id = a->scope_id;
	}
}


static void SockadrToNetadr( struct sockaddr *s, netadr_t *a ) {
	if (s->sa_family == AF_INET) {
		a->type = NA_IP;
		*(int *)&a->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
		a->port = ((struct sockaddr_in *)s)->sin_port;
	}
	else if(s->sa_family == AF_INET6)
	{
		a->type = NA_IP6;
		memcpy(a->ip6, &((struct sockaddr_in6 *)s)->sin6_addr, sizeof(a->ip6));
		a->port = ((struct sockaddr_in6 *)s)->sin6_port;
		a->scope_id = ((struct sockaddr_in6 *)s)->sin6_scope_id;
	}
}


static struct addrinfo *SearchAddrInfo(struct addrinfo *hints, sa_family_t family)
{
	while(hints)
	{
		if(hints->ai_family == family)
			return hints;

		hints = hints->ai_next;
	}
	
	return NULL;
}

/*
=============
Sys_StringToSockaddr
=============
*/
static qboolean Sys_StringToSockaddr(const char *s, struct sockaddr *sadr, int sadr_len, sa_family_t family)
{
	struct addrinfo hints;
	struct addrinfo *res = NULL;
	struct addrinfo *search = NULL;
	struct addrinfo *hintsp;
	int retval;
	
	memset(sadr, '\0', sizeof(*sadr));
	memset(&hints, '\0', sizeof(hints));

	hintsp = &hints;
	hintsp->ai_family = family;
	hintsp->ai_socktype = SOCK_DGRAM;
	
	retval = getaddrinfo(s, NULL, hintsp, &res);

	if(!retval)
	{
		if(family == AF_UNSPEC)
		{
			// Decide here and now which protocol family to use
			if(net_enabled & NET_PRIOV6)
			{
				if(net_enabled & NET_ENABLEV6)
					search = SearchAddrInfo(res, AF_INET6);
				
				if(!search && (net_enabled & NET_ENABLEV4))
					search = SearchAddrInfo(res, AF_INET);
			}
			else
			{
				if(net_enabled & NET_ENABLEV4)
					search = SearchAddrInfo(res, AF_INET);
				
				if(!search && (net_enabled & NET_ENABLEV6))
					search = SearchAddrInfo(res, AF_INET6);
			}
		}
		else
			search = SearchAddrInfo(res, family);

		if(search)
		{
			if(search->ai_addrlen > sadr_len)
				search->ai_addrlen = sadr_len;
				
			memcpy(sadr, search->ai_addr, search->ai_addrlen);
			freeaddrinfo(search);
			
			return qtrue;
		}
		else
			Com_Printf("Sys_StringToSockaddr: Error resolving %s: No address of required type found.\n", s);
	}
	else
		Com_Printf("Sys_StringToSockaddr: Error resolving %s: %s\n", s, gai_strerror(retval));
	
	if(res)
		freeaddrinfo(res);
	
	return qfalse;
}

/*
=============
Sys_SockaddrToString
=============
*/
static void Sys_SockaddrToString(char *dest, int destlen, struct sockaddr *input)
{
	socklen_t inputlen;

	if (input->sa_family == AF_INET6)
		inputlen = sizeof(struct sockaddr_in6);
	else
		inputlen = sizeof(struct sockaddr_in);

	if(getnameinfo(input, inputlen, dest, destlen, NULL, 0, NI_NUMERICHOST) && destlen > 0)
		*dest = '\0';
}

/*
=============
Sys_StringToAdr
=============
*/
qboolean Sys_StringToAdr( const char *s, netadr_t *a, netadrtype_t family ) {
	struct sockaddr_storage sadr;
	sa_family_t fam;
	
	switch(family)
	{
		case NA_IP:
			fam = AF_INET;
		break;
		case NA_IP6:
			fam = AF_INET6;
		break;
		default:
			fam = AF_UNSPEC;
		break;
	}
	if( !Sys_StringToSockaddr(s, (struct sockaddr *) &sadr, sizeof(sadr), fam ) ) {
		return qfalse;
	}
	
	SockadrToNetadr( (struct sockaddr *) &sadr, a );
	return qtrue;
}

/*
===================
NET_CompareBaseAdrMask

Compare without port, and up to the bit number given in netmask.
===================
*/
qboolean NET_CompareBaseAdrMask(netadr_t *a, netadr_t *b, int netmask)
{
	byte cmpmask, *addra, *addrb;
	int curbyte;
	
	if (a->type != b->type)
		return qfalse;

	if (a->type == NA_LOOPBACK)
		return qtrue;

	if(a->type == NA_IP)
	{
		addra = (byte *) &a->ip;
		addrb = (byte *) &b->ip;
		
		if(netmask < 0 || netmask > 32)
			netmask = 32;
	}
	else if(a->type == NA_IP6)
	{
		addra = (byte *) &a->ip6;
		addrb = (byte *) &b->ip6;
		
		if(netmask < 0 || netmask > 128)
			netmask = 128;
	}
	else
	{
		Com_Printf ("NET_CompareBaseAdr: bad address type\n");
		return qfalse;
	}

	curbyte = netmask >> 3;

	if(curbyte && memcmp(addra, addrb, curbyte))
			return qfalse;

	netmask &= 0x07;
	if(netmask)
	{
		cmpmask = (1 << netmask) - 1;
		cmpmask <<= 8 - netmask;

		if((addra[curbyte] & cmpmask) == (addrb[curbyte] & cmpmask))
			return qtrue;
	}
	else
		return qtrue;
	
	return qfalse;
}


/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
qboolean NET_CompareBaseAdr (netadr_t *a, netadr_t *b)
{
	return NET_CompareBaseAdrMask(a, b, -1);
}

const char	*NET_AdrToStringShort (netadr_t* a)
{
	static	char	s[NET_ADDRSTRMAXLEN];
	
	if(a == NULL)
		return "(null)";
		
	if (a->type == NA_LOOPBACK)
		Com_sprintf (s, sizeof(s), "loopback");
	else if (a->type == NA_BOT)
		Com_sprintf (s, sizeof(s), "bot");
	else if (a->type == NA_IP || a->type == NA_IP6)
	{
		struct sockaddr_storage sadr;

		memset(&sadr, 0, sizeof(sadr));
		NetadrToSockadr(a, (struct sockaddr *) &sadr);
		Sys_SockaddrToString(s, sizeof(s), (struct sockaddr *) &sadr);
	}
	return s;
}

const char	*NET_AdrToString(netadr_t* a)
{
	static	char	s[NET_ADDRSTRMAXLEN];
	char		t[NET_ADDRSTRMAXLEN];
	struct 		sockaddr_storage sadr;
	
	if(a == NULL)
		return "(null)";
		
	if (a->type == NA_LOOPBACK){
		Com_sprintf (s, sizeof(s), "loopback");
	}else if (a->type == NA_BOT){
		Com_sprintf (s, sizeof(s), "bot");
	}else if(a->type == NA_IP){

		memset(&sadr, 0, sizeof(sadr));
		NetadrToSockadr(a, (struct sockaddr *) &sadr);
		Sys_SockaddrToString(t, sizeof(t), (struct sockaddr *) &sadr);
		Com_sprintf(s, sizeof(s), "%s:%hu", t, ntohs(a->port));

	}else if(a->type == NA_IP6){

		memset(&sadr, 0, sizeof(sadr));
		NetadrToSockadr(a, (struct sockaddr *) &sadr);
		Sys_SockaddrToString(t, sizeof(t), (struct sockaddr *) &sadr);
		Com_sprintf(s, sizeof(s), "[%s]:%hu", t, ntohs(a->port));
        }
	return s;
}

qboolean	NET_CompareAdr (netadr_t *a, netadr_t *b)
{
	if(!NET_CompareBaseAdr(a, b))
		return qfalse;
	
	if (a->type == NA_IP || a->type == NA_IP6)
	{
		if (a->port == b->port)
			return qtrue;
	}
	else
		return qtrue;
		
	return qfalse;
}


/*
===================
NET_CompareAdrSigned

Compare with port, IPv4 and IPv6 addresses and return if the are greater or smaller.
===================
*/

int NET_CompareAdrSigned(netadr_t *a, netadr_t *b)
{	
	int i;

	if(a->type == NA_IP && b->type == NA_IP)
	{
		for(i = 0; i < 4; i++)
		{
			if(a->ip[i] < b->ip[i])
				return -1;
			else if(a->ip[i] > b->ip[i])
				return 1;
		}
		if(a->port < b->port)
			return -1;
		if(a->port > b->port)
			return 1;
		return 0;
	}
	
	if(a->type == NA_IP6 && b->type == NA_IP6)
	{
		for(i = 0; i < 16; i++)
		{
			if(a->ip6[i] < b->ip6[i])
				return -1;
			else if(a->ip6[i] > b->ip6[i])
				return 1;
		}
		if(a->port < b->port)
			return -1;
		if(a->port > b->port)
			return 1;
		return 0;
	}
	
	/* NA_IP6 is always greater than NA_IP. Port does not matter here */
	if(a->type == NA_IP && b->type == NA_IP6)
	{
		for(i = 0; i < 4; i++)
		{
			if(a->ip[i] < b->ip6[i])
				return -1;
			else if(a->ip[i] > b->ip6[i])
				return 1;
		}
		return -1;
	}
	
	if(a->type == NA_IP6 && b->type == NA_IP)
	{
		for(i = 0; i < 4; i++)
		{
			if(a->ip6[i] < b->ip[i])
				return -1;
			else if(a->ip6[i] > b->ip[i])
				return 1;
		}
		return 1;
	}
	
	Com_PrintError( "NET_CompareAdrSigned: bad address type\n");
    return 0;
}

qboolean	NET_IsLocalAddress( netadr_t adr ) {
	return adr.type == NA_LOOPBACK;
}

//=============================================================================


//=============================================================================


//=============================================================================


/*
====================
NET_IPSocket
====================
*/
int NET_IPSocket( char *net_interface, int port, int *err ) {
	SOCKET				newsocket;
	struct sockaddr_in	address;
	ioctlarg_t			_true = 1;
	int					i;

	*err = 0;

	if( net_interface ) {
		Com_Printf( "Opening IP socket: %s:%i\n", net_interface, port );
	}
	else {
		Com_Printf( "Opening IP socket: 0.0.0.0:%i\n", port );
	}

	if( ( newsocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == INVALID_SOCKET ) {
		*err = socketError;
		Com_Printf( "WARNING: NET_IPSocket: socket: %s\n", NET_ErrorString() );
		return newsocket;
	}
	// make it non-blocking
	if( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR ) {
		Com_Printf( "WARNING: NET_IPSocket: ioctl FIONBIO: %s\n", NET_ErrorString() );
		*err = socketError;
		closesocket(newsocket);
		return INVALID_SOCKET;
	}

	// make it broadcast capable
	i = 1;
	
	if( setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, (char *) &i, sizeof(i) ) == SOCKET_ERROR ) {
		Com_PrintWarning( "NET_IPSocket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString() );
	}
	// set the sendbuffersize to 64Kb
	i = 0x10000;
	
	if( setsockopt( newsocket, SOL_SOCKET, SO_SNDBUF, (char *) &i, sizeof(i) ) == SOCKET_ERROR ) {
		Com_PrintWarning( "NET_IPSocket: setsockopt SO_SNDBUF: %s\n", NET_ErrorString() );
	}
	// set the receivebuffersize to 256Kb
	i = 0x40000;
		
	if( setsockopt( newsocket, SOL_SOCKET, SO_RCVBUF, (char *) &i, sizeof(i) ) == SOCKET_ERROR ) {
		Com_PrintWarning( "NET_IPSocket: setsockopt SO_RCVBUF: %s\n", NET_ErrorString() );
	}
	// All outgoing packets have the QoS EF-Flag set
	i = 0xB8;
	
	if( setsockopt( newsocket, IPPROTO_IP, IP_TOS, (char *) &i, sizeof(i) ) == SOCKET_ERROR ) {
		Com_PrintWarning( "NET_IPSocket: setsockopt IP_TOS: %s\n", NET_ErrorString() );
	}
	
	
	if( !net_interface || !net_interface[0]) {
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		if(!Sys_StringToSockaddr( net_interface, (struct sockaddr *)&address, sizeof(address), AF_INET))
		{
			closesocket(newsocket);
			return INVALID_SOCKET;
		}
	}

	if( port == PORT_ANY ) {
		address.sin_port = 0;
	}
	else {
		address.sin_port = htons( (short)port );
	}

	if( bind( newsocket, (void *)&address, sizeof(address) ) == SOCKET_ERROR ) {
		Com_Printf( "WARNING: NET_IPSocket: bind: %s\n", NET_ErrorString() );
		*err = socketError;
		closesocket( newsocket );
		return INVALID_SOCKET;
	}

	return newsocket;
}

/*
====================
NET_IP6Socket
====================
*/
int NET_IP6Socket( char *net_interface, int port, struct sockaddr_in6 *bindto, int *err ) {
	SOCKET				newsocket;
	struct sockaddr_in6	address;
	ioctlarg_t			_true = 1;

	*err = 0;

	if( net_interface )
	{
		// Print the name in brackets if there is a colon:
		if(Q_CountChar(net_interface, ':'))
			Com_Printf( "Opening IP6 socket: [%s]:%i\n", net_interface, port );
		else
			Com_Printf( "Opening IP6 socket: %s:%i\n", net_interface, port );
	}
	else
		Com_Printf( "Opening IP6 socket: [::]:%i\n", port );

	if( ( newsocket = socket( PF_INET6, SOCK_DGRAM, IPPROTO_UDP ) ) == INVALID_SOCKET ) {
		*err = socketError;
		Com_Printf( "WARNING: NET_IP6Socket: socket: %s\n", NET_ErrorString() );
		return newsocket;
	}

	// make it non-blocking
	if( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR ) {
		Com_Printf( "WARNING: NET_IP6Socket: ioctl FIONBIO: %s\n", NET_ErrorString() );
		*err = socketError;
		closesocket(newsocket);
		return INVALID_SOCKET;
	}

#ifdef IPV6_V6ONLY
	{
		int i = 1;

		// ipv4 addresses should not be allowed to connect via this socket.
		if(setsockopt(newsocket, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &i, sizeof(i)) == SOCKET_ERROR)
		{
			// win32 systems don't seem to support this anyways.
			Com_DPrintf("WARNING: NET_IP6Socket: setsockopt IPV6_V6ONLY: %s\n", NET_ErrorString());
		}
	}
#endif

	if( !net_interface || !net_interface[0]) {
		address.sin6_family = AF_INET6;
		address.sin6_addr = in6addr_any;
	}
	else
	{
		if(!Sys_StringToSockaddr( net_interface, (struct sockaddr *)&address, sizeof(address), AF_INET6))
		{
			closesocket(newsocket);
			return INVALID_SOCKET;
		}
	}

	if( port == PORT_ANY ) {
		address.sin6_port = 0;
	}
	else {
		address.sin6_port = htons( (short)port );
	}

	if( bind( newsocket, (void *)&address, sizeof(address) ) == SOCKET_ERROR ) {
		Com_Printf( "WARNING: NET_IP6Socket: bind: %s\n", NET_ErrorString() );
		*err = socketError;
		closesocket( newsocket );
		return INVALID_SOCKET;
	}
	
	if(bindto)
		*bindto = address;

	return newsocket;
}




/*
====================
NET_Init
====================
*/
void NET_Init( void ) {
#ifdef _WIN32
	int		r;

	r = WSAStartup( MAKEWORD( 1, 1 ), &winsockdata );
	if( r ) {
		Com_Printf( "WARNING: Winsock initialization failed, returned %d\n", r );
		return;
	}
	net_enabled = 3;
	winsockInitialized = qtrue;
	Com_Printf( "Winsock Initialized\n" );
#endif


}


/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown( void ) {
	if ( !networkingEnabled ) {
		return;
	}

#ifdef _WIN32
	WSACleanup();
	winsockInitialized = qfalse;
#endif
}


/*
===============
NET_TcpCloseSocket

This function should be able to close all types of open TCP sockets
===============
*/

void NET_TcpCloseSocket(int socket)
{
	if(socket == INVALID_SOCKET)
		return;

	//Close the socket
	closesocket(socket);

}


/*
====================
NET_TcpClientConnect
====================
*/
int NET_TcpClientConnect( const char *remoteAdr ) {
	SOCKET			newsocket;
	struct sockaddr_in	address;
	netadr_t remoteadr;
	int err = 0;
	int retval;
	fd_set fdr;
	struct timeval timeout;

	Com_Printf( "Connecting to: %s\n", remoteAdr);

	if( ( newsocket = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP ) ) == INVALID_SOCKET ) {
		Com_PrintWarning( "NET_TCPConnect: socket: %s\n", NET_ErrorString() );
		return INVALID_SOCKET;
	}
	// make it non-blocking
	ioctlarg_t	_true = 1;
	if( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR ) {
		Com_PrintWarning( "NET_TCPIPSocket: ioctl FIONBIO: %s\n", NET_ErrorString() );
		closesocket(newsocket);
		return INVALID_SOCKET;
	}

	if(NET_StringToAdr(remoteAdr, &remoteadr, NA_UNSPEC))
	{
		Com_Printf( "Resolved %s to: %s\n", remoteAdr, NET_AdrToString(&remoteadr));
	}else{
		Com_PrintWarning( "Couldn't resolve: %s\n", remoteAdr);
		closesocket( newsocket );
		return INVALID_SOCKET;
	}

	NetadrToSockadr( &remoteadr, (struct sockaddr *)&address);

	if( connect( newsocket, (void *)&address, sizeof(address) ) == SOCKET_ERROR ) {

		err = socketError;
		if(err == EINPROGRESS
#ifdef _WIN32
			|| err == WSAEWOULDBLOCK
#endif		
		){

			FD_ZERO(&fdr);
			FD_SET(newsocket, &fdr);
			timeout.tv_sec = 5;
			timeout.tv_usec = 0;

			retval = select(newsocket +1, NULL, &fdr, NULL, &timeout);

			if(retval < 0){
				Com_PrintWarning("NET_TcpConnect: select() syscall failed: %s\n", NET_ErrorString());
				closesocket( newsocket );
				return INVALID_SOCKET;
			}else if(retval > 0){

				socklen_t so_len = sizeof(err);

				if(getsockopt(newsocket, SOL_SOCKET, SO_ERROR, (char*) &err, &so_len) == 0)
				{
					return newsocket;
				}

			}else{
				Com_PrintWarning("NET_TcpConnect: Connecting to: %s timed out\n", remoteAdr);
				closesocket( newsocket );
				return INVALID_SOCKET;
			}
		}
		Com_PrintWarning( "NET_TCPOpenConnection: connect: %s\n", NET_ErrorString() );
		closesocket( newsocket );
		return INVALID_SOCKET;
	}
	return newsocket;
}

/*
==================
NET_TcpSendData
Only for Stream sockets (TCP)
Return -1 if an fatal error happened on this socket otherwise 0
==================
*/

int NET_TcpSendData( int sock, const void *data, int length ) {

	int state, err, numbytes;
	
	numbytes = length;

	if(sock < 1)
		return -1;

	state = send( sock, data, numbytes, NET_NOSIGNAL); // FIX: flag NOSIGNAL prevents SIGPIPE in case of connection problems

	if(state == SOCKET_ERROR)
	{
			err = SOCKET_ERROR;

			if( err == EAGAIN )
			{
				return 0; 
			}else{
				Com_PrintWarning("NET_SendTCPPacket: Couldn't send data to remote host: %s\n", NET_ErrorString());
			}
			NET_TcpCloseSocket(sock);
			return -1;
	}

	return state;
}

/*
====================
NET_TcpClientGetData
returns -1 if connection got closed
====================
*/

int NET_TcpClientGetData(int sock, void* buf, int* buflen){

	int err;
	int ret;
	int readcount = 0;

	if(sock < 1)
		return -1;

	while(qtrue){

		ret = recv(sock, buf + readcount, *buflen - readcount, 0);

		if(ret == SOCKET_ERROR){

			err = socketError;

			if(err == EAGAIN){
				break; //Nothing more to read left
			}

			if(ret == ECONNRESET)
				Com_Printf("Connection closed by remote host\n");
				//Connection closed
			else
				Com_PrintWarning("NET_TcpGetData recv() syscall failed: %s\n", NET_ErrorString());

			*buflen = readcount;
			NET_TcpCloseSocket(sock);
			return -1;

		}else if(ret == 0){
			if(*buflen == readcount)
			{
				return 0;
			}
			*buflen = readcount;
			NET_TcpCloseSocket(sock);
			Com_Printf("Connection closed by remote host\n");
			return -1;

		}else{

			if( ret > *buflen - readcount) {

				Com_PrintWarning( "Oversize packet on socket %d Remaining bytes are: %d, received bytes are %d\n", sock, *buflen - readcount, ret);
				readcount = *buflen -1;
				break;
			}
			readcount = readcount + ret;
		}
	}
	*buflen = readcount;
	return readcount;
}


/*
===============
NET_TcpReceiveData
Receives a TCP datastream
return values:
2: Got new data + Connection okay
1: Got no data + Connection okay
-1: Got no data + Connection closed
-2: Got new data + Connection closed
================
*/
int NET_TcpReceiveData( int sock, msg_t* msg) {

	int len = msg->maxsize - msg->cursize;
	int ret;

	ret = NET_TcpClientGetData( sock, msg->data + msg->cursize, &len);

	if(ret < 0 && len > 0)
        {
            msg->cursize += len;
            return -2;
        }
	if(ret < 0 && len < 1)
        {
	    return -1;
        }
        if(len > 0)
	{
            msg->cursize += len;
	    return 2;
	}
	return 1;
}

