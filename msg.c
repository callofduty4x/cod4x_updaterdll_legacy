/*
===========================================================================
    Copyright (C) 2010-2013  Ninja and TheKelm of the IceOps-Team
    Copyright (C) 1999-2005 Id Software, Inc.

    This file is part of CoD4X17a-Server source code.

    CoD4X17a-Server source code is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    CoD4X17a-Server source code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
===========================================================================
*/

#include "q_shared.h"
#include "qcommon.h"

#include <string.h>
#include <stdint.h>
#include <math.h>

#ifndef	MAX_MSGLEN
#define	MAX_MSGLEN	0x20000		// max length of a message, which may
#endif



#ifndef __HUFFMAN_H__
#pragma message "Function MSG_initHuffman() is undefined"
void MSG_initHuffman(){}
#endif


/*
This part makes msg.c undepended in case no proper qcommon_io.h is included
*/



int pcount[256];

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

int oldsize = 0;


void MSG_Init( msg_t *buf, byte *data, int length ) {

	buf->data = data;
	buf->maxsize = length;
	buf->overflowed = qfalse;
	buf->cursize = 0;
	buf->readonly = qfalse;
	buf->splitdata = NULL;
	buf->splitsize = 0;
	buf->readcount = 0;
	buf->bit = 0;
	buf->lastRefEntity = 0;
}

void MSG_InitReadOnly( msg_t *buf, byte *data, int length ) {

	MSG_Init( buf, data, length);
	buf->data = data;
	buf->cursize = length;
	buf->readonly = qtrue;
	buf->splitdata = NULL;
	buf->maxsize = length;
	buf->splitsize = 0;
	buf->readcount = 0;
}

void MSG_InitReadOnlySplit( msg_t *buf, byte *data, int length, byte* arg4, int arg5 ) {

	buf->data = data;
	buf->cursize = length;
	buf->readonly = qtrue;
	buf->splitdata = arg4;
	buf->maxsize = length + arg5;
	buf->splitsize = arg5;
	buf->readcount = 0;
}


void MSG_Clear( msg_t *buf ) {

	if(buf->readonly == qtrue || buf->splitdata != NULL)
	{
		Com_Error(ERR_FATAL, "MSG_Clear: Can not clear a read only or split msg");
		return;
	}

	buf->cursize = 0;
	buf->overflowed = qfalse;
	buf->bit = 0;					//<- in bits
	buf->readcount = 0;
}


void MSG_BeginReading( msg_t *msg ) {
	msg->overflowed = qfalse;
	msg->readcount = 0;
	msg->bit = 0;
}

void MSG_Copy(msg_t *buf, byte *data, int length, msg_t *src)
{
	if (length < src->cursize) {
		Com_Error( ERR_DROP, "MSG_Copy: can't copy into a smaller msg_t buffer");
	}
	Com_Memcpy(buf, src, sizeof(msg_t));
	buf->data = data;
	Com_Memcpy(buf->data, src->data, src->cursize);
}


//================================================================================

//
// writing functions
//


void MSG_WriteByte( msg_t *msg, int c ) {
#ifdef PARANOID
	if (c < 0 || c > 255)
		Com_Error (ERR_FATAL, "MSG_WriteByte: range error");
#endif
	byte* dst;

	if ( msg->maxsize - msg->cursize < 1 ) {
		msg->overflowed = qtrue;
		return;
	}
	dst = (byte*)&msg->data[msg->cursize];
	*dst = c;
	msg->cursize += sizeof(byte);
}


void MSG_WriteShort( msg_t *msg, int c ) {
#ifdef PARANOID
	if (c < ((short)0x8000) || c > (short)0x7fff)
		Com_Error (ERR_FATAL, "MSG_WriteShort: range error");
#endif
	signed short* dst;

	if ( msg->maxsize - msg->cursize < 2 ) {
		msg->overflowed = qtrue;
		return;
	}
	dst = (short*)&msg->data[msg->cursize];
	*dst = c;
	msg->cursize += sizeof(short);
}

void MSG_WriteLong( msg_t *msg, int c ) {
	int32_t *dst;

	if ( msg->maxsize - msg->cursize < 4 ) {
		msg->overflowed = qtrue;
		return;
	}
	dst = (int32_t*)&msg->data[msg->cursize];
	*dst = c;
	msg->cursize += sizeof(int32_t);
}

void MSG_WriteInt64(msg_t *msg, int64_t c)
{
	int64_t *dst;

	if ( msg->maxsize - msg->cursize < sizeof(int64_t) ) {
		msg->overflowed = qtrue;
		return;
	}
	dst = (int64_t*)&msg->data[msg->cursize];
	*dst = c;
	msg->cursize += sizeof(int64_t);
}

void MSG_WriteData( msg_t *buf, const void *data, int length ) {
	int i;
	for(i=0; i < length; i++){
		MSG_WriteByte(buf, ((byte*)data)[i]);
	}
}

void MSG_WriteString( msg_t *sb, const char *s ) {
	if ( !s ) {
		MSG_WriteData( sb, "", 1 );
	} else {
		int l;
		char string[MAX_STRING_CHARS];

		l = strlen( s );
		if ( l >= MAX_STRING_CHARS ) {
			Com_Printf( "MSG_WriteString: MAX_STRING_CHARS" );
			MSG_WriteData( sb, "", 1 );
			return;
		}
		Q_strncpyz( string, s, sizeof( string ) );

		MSG_WriteData( sb, string, l + 1 );
	}
}

void MSG_WriteBigString( msg_t *sb, const char *s ) {
	if ( !s ) {
		MSG_WriteData( sb, "", 1 );
	} else {
		int l;
		char string[BIG_INFO_STRING];

		l = strlen( s );
		if ( l >= BIG_INFO_STRING ) {
			Com_Printf( "MSG_WriteString: BIG_INFO_STRING" );
			MSG_WriteData( sb, "", 1 );
			return;
		}
		Q_strncpyz( string, s, sizeof( string ) );

		MSG_WriteData( sb, string, l + 1 );
	}
}

void MSG_WriteVector( msg_t *msg, vec3_t c ) {
	vec_t *dst;

	if ( msg->maxsize - msg->cursize < 12 ) {
		msg->overflowed = qtrue;
		return;
	}
	dst = (vec_t*)&msg->data[msg->cursize];
	dst[0] = c[0];
	dst[1] = c[1];
	dst[2] = c[2];
	msg->cursize += sizeof(vec3_t);
}


void MSG_WriteBit0( msg_t* msg )
{
	if(!(msg->bit & 7))
	{
		if(msg->maxsize <= msg->cursize)
		{
			msg->overflowed = qtrue;
			return;
		}
		msg->bit = msg->cursize*8;
		msg->data[msg->cursize] = 0;
		msg->cursize ++;
	}
	msg->bit++;
}

void MSG_WriteBit1(msg_t *msg)
{
	if ( !(msg->bit & 7) )
	{
		if ( msg->cursize >= msg->maxsize )
		{
			msg->overflowed = qtrue;
			return;
		}
		msg->bit = 8 * msg->cursize;
		msg->data[msg->cursize] = 0;
		msg->cursize++;
	}
	msg->data[msg->bit >> 3] |= 1 << (msg->bit & 7);
	msg->bit++;
}


void MSG_WriteBits(msg_t *msg, int bits, int bitcount)
{
    int i;

    if ( msg->maxsize - msg->cursize < 4 )
    {
        msg->overflowed = 1;
        return;
    }

    for (i = 0 ; bitcount != i; i++)
    {

        if ( !(msg->bit & 7) )
        {
			msg->bit = 8 * msg->cursize;
			msg->data[msg->cursize] = 0;
			msg->cursize++;
        }

        if ( bits & 1 )
          msg->data[msg->bit >> 3] |= 1 << (msg->bit & 7);

        msg->bit++;
        bits >>= 1;
    }
}


char *base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void MSG_WriteBase64(msg_t* msg, byte* inbuf, int len)
{
    int bits, i, k, j, shift, offset;
    int mask;
    int b64data;

    i = k = 0;

    while(i < len)
    {
        bits = 0;
        /* Read a base64 3 byte block */
        for(k = 0; k < 3 && i < len; ++k, ++i)
        {
            ((byte*)&bits)[2 - k] = inbuf[i];
        }

        mask = 64 - 1;

        for(j = 0, shift = 0; j < 4; ++j, shift += 6)
        {
            offset = (bits & (mask << shift)) >> shift;

            ((byte*)&b64data)[3 - j] = base64[offset];
        }
        MSG_WriteLong(msg, b64data);
    }

    if(msg->cursize < 3)
    {
        return;
    }

    for(i = 0; k < 3; i++, k++)
    {
        msg->data[msg->cursize - i -1] = '=';
    }
}


//============================================================

//
// reading functions
//

// returns -1 if no more characters are available

int MSG_ReadByte( msg_t *msg ) {
	byte	*c;

	if ( msg->readcount + sizeof(byte) > msg->cursize ) {
		msg->overflowed = 1;
		return -1;
	}
	c = &msg->data[msg->readcount];
		
	msg->readcount += sizeof(byte);
	return *c;
}

int MSG_ReadShort( msg_t *msg ) {
	signed short	*c;

	if ( msg->readcount + sizeof(short) > msg->cursize ) {
		msg->overflowed = 1;
		return -1;
	}
	c = (short*)&msg->data[msg->readcount];

	msg->readcount += sizeof(short);
	return *c;
}

int MSG_ReadLong( msg_t *msg ) {
	int32_t		*c;

	if ( msg->readcount + sizeof(int32_t) > msg->cursize ) {
		msg->overflowed = 1;
		return -1;
	}	
	c = (int32_t*)&msg->data[msg->readcount];

	msg->readcount += sizeof(int32_t);
	return *c;

}


int64_t MSG_ReadInt64( msg_t *msg ) {
	int64_t		*c;

	if ( msg->readcount+sizeof(int64_t) > msg->cursize ) {
		msg->overflowed = 1;
		return -1;
	}	
	c = (int64_t*)&msg->data[msg->readcount];

	msg->readcount += sizeof(int64_t);
	return *c;

}

/*
int MSG_SkipToString( msg_t *msg, const char* string ) {
	byte c;

	do{
		c = MSG_ReadByte( msg );      // use ReadByte so -1 is out of bounds
		if ( c == -1 )
		{
			return qfalse;
		}
		if(c == string[0] && !Q_strncmp(msg->data + msg->readcount, string, msg->cursize - msg->readcount))
		{
			return qtrue;
		}
	}
	return qfalse;
}
*/


char *MSG_ReadString( msg_t *msg, char* bigstring, int len ) {
	int l,c;

	l = 0;
	do {
		c = MSG_ReadByte( msg );      // use ReadByte so -1 is out of bounds
		if ( c == -1 || c == 0 ) {
			break;
		}
		// translate all fmt spec to avoid crash bugs
		if ( c == '%' ) {
			c = '.';
		}
		bigstring[l] = c;
		l++;
	} while ( l < len - 1 );

	if(c != 0 && c != -1)
	{
		Com_PrintError("MSG_ReadString() message has been truncated\n");
		do {
			c = MSG_ReadByte( msg );      // use ReadByte so -1 is out of bounds
		} while ( c != -1 && c != 0 );
	}

	bigstring[l] = 0;
	return bigstring;
}
/*
char *MSG_ReadStringLine( msg_t *msg ) {
	int		l,c;

	l = 0;
	do {
		c = MSG_ReadByte(msg);		// use ReadByte so -1 is out of bounds
		if (c == -1 || c == 0 || c == '\n') {
			break;
		}
		// translate all fmt spec to avoid crash bugs
		if ( c == '%' ) {
			c = '.';
		}
		bigstring[l] = c;
		l++;
	} while (l < sizeof(bigstring)-1);
	
	bigstring[l] = 0;
	
	return bigstring;
}
*/

char *MSG_ReadStringLine( msg_t *msg, char* bigstring, int len ) {
	int		l,c;

	l = 0;
	do {
		c = MSG_ReadByte(msg);		// use ReadByte so -1 is out of bounds
		if (c == -1 || c == 0 || c == '\n') {
			break;
		}
		// translate all fmt spec to avoid crash bugs
		if ( c == '%' ) {
			c = '.';
		}
		bigstring[l] = c;
		l++;
	} while (l < len -1);
	
	bigstring[l] = 0;
	
	return bigstring;
}

void MSG_ReadData( msg_t *msg, void *data, int len ) {
	int		i;

	for (i=0 ; i<len ; i++) {
		((byte *)data)[i] = MSG_ReadByte (msg);
	}
}

void MSG_ClearLastReferencedEntity( msg_t *msg ) {
	msg->lastRefEntity = -1;
}


int MSG_GetUsedBitCount( msg_t *msg ) {

    return ((msg->cursize + msg->splitsize) * 8) - ((8 - msg->bit) & 7);

}

int MSG_ReadBit(msg_t *msg)
{

  int oldbit7, numBytes, bits;

  oldbit7 = msg->bit & 7;
  if ( !oldbit7 )
  {
    if ( msg->readcount >= msg->cursize + msg->splitsize )
    {
      msg->overflowed = 1;
      return -1;
    }
    msg->bit = 8 * msg->readcount;
    msg->readcount++;
  }
  
  numBytes = msg->bit / 8;
  if ( numBytes < msg->cursize )
  {
    bits = msg->data[numBytes] >> oldbit7;
    msg->bit++;
    return bits & 1;
  }
  bits = msg->splitdata[numBytes - msg->cursize] >> oldbit7;
  msg->bit++;
  return bits & 1;
}

int MSG_ReadBits(msg_t *msg, int numBits)
{
  int i;
  signed int var;
  int retval;

  retval = 0;

  if ( numBits > 0 )
  {

    for(i = 0 ;numBits != i; i++)
    {
      if ( !(msg->bit & 7) )
      {
        if ( msg->readcount >= msg->splitsize + msg->cursize )
        {
          msg->overflowed = 1;
          return -1;
        }
        msg->bit = 8 * msg->readcount;
        msg->readcount++;
      }
      if ( ((msg->bit / 8)) >= msg->cursize )
      {

        if(msg->splitdata == NULL)
		{
            return 0;
		}
        var = msg->splitdata[(msg->bit / 8) - msg->cursize];

      }else{
        var = msg->data[msg->bit / 8];
	  }
	  
      retval |= ((var >> (msg->bit & 7)) & 1) << i;
      msg->bit++;
    }
  }
  return retval;
}



void MSG_ReadBase64(msg_t* msg, byte* outbuf, int len)
{
    int databyte;
    int b64data;
    int k, shift;

    int i = 0;

    do
    {
        b64data = 0;
        for(k = 0, shift = 18; k < 4; ++k, shift -= 6)
        {

            databyte = MSG_ReadByte(msg);
            if(databyte >= 'A' && databyte <= 'Z')
            {
                databyte -= 'A';
            }else if(databyte >= 'a' && databyte <= 'z' ){
                databyte -= 'a';
                databyte += 26;
            }else if(databyte >= '0' && databyte <= '9' ){
                databyte -= '0';
                databyte += 52;
            }else if(databyte == '+'){
                databyte = 62;
            }else if(databyte == '/'){
                databyte = 63;
            }else{
                databyte = -1;
                break;
            }

            b64data |= (databyte << shift);

        }

        outbuf[i + 0] = ((char*)&b64data)[2];
        outbuf[i + 1] = ((char*)&b64data)[1];
        outbuf[i + 2] = ((char*)&b64data)[0];

        i += 3;

    }while(databyte != -1 && (i +4) < len);

    outbuf[i] = '\0';

}


void MSG_Discard(msg_t* msg)
{
    msg->cursize = msg->readcount;
    msg->splitsize = 0;
    msg->overflowed = qtrue;
}
