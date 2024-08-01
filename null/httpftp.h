/*
 *  httpftp.h
 *  CoD4X17a_testing
 *
 *  Created by Florian on 4/10/14.
 *  Copyright 2014 Dorg. All rights reserved.
 *
 */
#ifndef _HTTPFTP_H_
#define _HTTPFTP_H_

#include "q_shared.h"
#include "qcommon.h"

typedef enum
{
	FT_PROTO_HTTP,
	FT_PROTO_FTP
}ftprotocols_t;

typedef struct
{
	qboolean lock;
	qboolean active;
	qboolean transferactive;
	int transferStartTime;
	int socket;
	int transfersocket;
	int sentBytes;
	int finallen;
	int totalreceivedbytes;
	int transfertotalreceivedbytes;	
	msg_t *extrecvmsg;
	msg_t *extsendmsg;
	msg_t sendmsg;	
	msg_t recvmsg;
	msg_t transfermsg;
	qboolean complete;
	int code;
	int version;
	char status[32];
	char url[MAX_STRING_CHARS];
	char address[MAX_STRING_CHARS];
	char username[256];
	char password[256];
	int mode;
	int headerLength;
	int contentLength;
	int startTime;
	int stage;
	ftprotocols_t protocol;
}ftRequest_t;

typedef enum
{
	HTTP_GET,
	HTTP_POST,
	HTTP_HEAD
}httpMethod_t;


void FileDownloadFreeRequest(ftRequest_t* request);
ftRequest_t* FileDownloadRequest( const char* url);
int FileDownloadSendReceive( ftRequest_t* request );
const char* FileDownloadGenerateProgress( ftRequest_t* request );
void HTTPServer_Init();


#endif
