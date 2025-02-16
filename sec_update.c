/*
===========================================================================
    Copyright (C) 2010-2013  Ninja and TheKelm of the IceOps-Team

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
#include "sec_update.h"
#include "sec_crypto.h"
#include "sec_common.h"
#include "sec_sign.h"
#include "httpftp.h"
#include "windows_inc.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <aclapi.h>
#include <stdbool.h>


#define BUFSIZE 10240


char *Sec_StrTok(char *str,char *tokens,int id){
    static char *mem[100] = {NULL};
    char *ptr,*ptr2;
    if(id<0||id>99||tokens==NULL)
	return NULL;
    if(str==NULL){
		if(mem[id]==NULL){
			return NULL;
		}
		
		for(ptr=mem[id];*ptr != 0;++ptr){
			//printf("---%c\n",*ptr);
			for(ptr2=tokens;*ptr2!=0;++ptr2){
				//printf("------%c\n",*ptr2);
				if(*ptr == *ptr2){
					//printf("DEBUG!!!!!!!! %p:\"%s\", %p:\"%s\",%p:\"%s\".\n\n",ptr,ptr,ptr2,ptr2,mem[id],mem[id]);
					*ptr=0;
					ptr2=mem[id];
					mem[id]=ptr+1;
					//printf("DEBUG!!!!!!!! %p:\"%s\", %p:\"%s\".\n\n",ptr,ptr,ptr2,ptr2);
					//__asm__("int $3");
					return ptr2;
				}
			}
		}
		if(ptr!=mem[id]){
			ptr = mem[id];
		}
		else
			ptr = NULL;
		mem[id]=NULL;
		return ptr;
    
	}else{
    
		//printf("Debugging: \"%s\"\n",str);
		//mem[id]=str;
		for(ptr=str;*ptr != 0 && mem[id]==NULL;++ptr){
			for(ptr2=tokens;*ptr2!=0 && mem[id]==NULL;++ptr2){
			if(*ptr != *ptr2){
				mem[id]=ptr;
			}
			}
		}
		if(mem[id] == NULL) return NULL;
		return Sec_StrTok(NULL,tokens,id); // BECAUSE I CAN.
    }	
}

void Sec_FreeFileStruct(sec_file_t *file){
    if(file->next != NULL)
	Sec_FreeFileStruct(file->next);
    Sec_Free(file);
}


int Sec_DownloadFile(const char* baseurl, sec_file_t *currFile)
{
		ftRequest_t* curfileobj;
		int transret;
		char updatepathbuf[1024];
	    int len;

	    char buff[SEC_UPDATE_INITIALBUFFSIZE];

		Com_sprintf(buff, sizeof(buff), SEC_UPDATE_DOWNLOAD(baseurl, currFile->path));

		curfileobj = FileDownloadRequest(buff);
		if(curfileobj == NULL)
		{
			return -1;	
		}

		Com_PrintStatus("Downloading file: \"%s\"\n", currFile->name);

		do {
			transret = FileDownloadSendReceive( curfileobj );
			FileDownloadGenerateProgress( curfileobj );
			usleep(100000);
		} while (transret == 0);
		
		if(transret < 0)
		{
			FileDownloadFreeRequest(curfileobj);
			return -1;
		}

		if(curfileobj->code != 200){
			Com_PrintError("Downloading has failed! Error code: %d. Update aborted.\n", curfileobj->code);
			FileDownloadFreeRequest(curfileobj);
			return -1;
		}

		Com_sprintf(updatepathbuf, sizeof(updatepathbuf), "updates/%s", currFile->name);

		len = FS_SV_WriteFileToSavePath( updatepathbuf, curfileobj->recvmsg.data + curfileobj->headerLength, curfileobj->contentLength );
		if(len != curfileobj->contentLength){

			Com_PrintError("Opening \"%s\" for writing! Update aborted.\n",currFile->name);
			FileDownloadFreeRequest(curfileobj);
			return -1;
		}

		if(Sec_VerifyMemory(currFile->hash, curfileobj->recvmsg.data + curfileobj->headerLength, curfileobj->contentLength) == qfalse)
		{
			FileDownloadFreeRequest(curfileobj);
			Com_PrintError("File \"%s\" is corrupt!\nUpdate aborted.\n",currFile->name);
			return -1;
		}
		FileDownloadFreeRequest(curfileobj);
		return len;
}



sec_file_t* Sec_ParseLine(const char* line)
{
	
	char* ptr, *ptr2;
	
	ptr = (char*)line;
	
	sec_file_t *currFile;
	
	currFile = Sec_GMalloc(sec_file_t,1);
	
	if(currFile == NULL)
	{
		return currFile;
	}
	
	Com_Memset(currFile,0,sizeof(sec_file_t));
	
	ptr2 = strchr(ptr,' ');
	
	if(ptr2 == NULL){
		Com_PrintWarning("Sec_Update: Corrupt data from update server. Update aborted.\nDebug:\"%s\"\n",ptr);
		Sec_FreeFileStruct(currFile);
		return NULL;
	}
	*ptr2++ = 0;
	
	Q_strncpyz(currFile->path,ptr,sizeof(currFile->path));
	
	ptr = ptr2;
	
	ptr2 = strchr(ptr,' ');
	
	if(ptr2 == NULL){
		Com_PrintWarning("Sec_Update: Corrupt data from update server. Update aborted.\nDebug:\"%s\"\n",ptr);
		Sec_FreeFileStruct(currFile);
		return NULL;
	}
	
	*ptr2++ = 0;
	
	if(!isInteger(ptr, 0)){
		Com_PrintWarning("Sec_Update: Corrupt data from update server - size is not a number. Update aborted.\nDebug:\"%s\"\n",ptr);
		Sec_FreeFileStruct(currFile);
		return NULL;
	}
	
	currFile->size = atoi(ptr);
	
	Q_strncpyz(currFile->hash,ptr2,sizeof(currFile->hash));
	Q_strncpyz(currFile->name,currFile->path, sizeof(currFile->name));
	
	return currFile;
}

int Sec_GetUpdateInfo(char *buff, int bufflen)
{
	int transret;
	ftRequest_t* filetransferobj;
	char url[2048];
	int len;

	Com_PrintStatus("Getting info about update...\n");	

    Com_sprintf(url, sizeof(url), "http://" SEC_UPDATE_HOST "/?mode=1&ver=%s&os=%s", Com_GetVersion(), OS_STRING);
	
	filetransferobj = FileDownloadRequest( url );

    if(filetransferobj == NULL){
		return -1;
    }

	do {
		transret = FileDownloadSendReceive( filetransferobj );
		usleep(20000);
	} while (transret == 0);

    if(transret < 0)
	{
		FileDownloadFreeRequest(filetransferobj);
		return -1;
    }
	
/*  Need to catch errors */
//  FS_WriteFile("tmp.txt", va("%d", status), 1);
//  TODO: Do something with the status?
//  FS_WriteFile("tmp2.txt", packet.header, packet.headerLength);
//  FS_WriteFile("tmp3.txt", packet.content, packet.contentLength);

    if(filetransferobj->code <= 0){
		Com_PrintError("Receiving data. Error code: %d.\n", filetransferobj->code);
		FileDownloadFreeRequest(filetransferobj);
		return -1;
    }
    if(filetransferobj->code == 204){
		Com_Printf("Game files are up to date.\n\n");
		FileDownloadFreeRequest(filetransferobj);
		return -1;
    }
    else if(filetransferobj->code != 200){
		Com_PrintWarning("The update server's malfunction.\nStatus code: %d.\n", filetransferobj->code);
		FileDownloadFreeRequest(filetransferobj);
		return -1;
    }

	if(filetransferobj->contentLength >= sizeof(buff))
	{
		len = bufflen -1;
	}else{
		len = filetransferobj->contentLength;
	}
	memcpy(buff, filetransferobj->recvmsg.data + filetransferobj->headerLength, len);
	buff[len] = '\0';
	FileDownloadFreeRequest(filetransferobj);
	return len;
}

//Returns true if file will be in cod4root otherwise returns false when in Localappdata
bool Sec_BuildDestinationFilePath(const char* currFileName, wchar_t *toospath)
{
	wchar_t bufuni[1024];
	if(Q_strncmp(currFileName, "fs_savepath", 11) == 0)
	{
		int startpath = 11;
		if(currFileName[11])
		{
			startpath = 12;
		}
		wchar_t realsavepath[4096];
		Q_strncpyzUni(realsavepath, FS_GetSavePath(), sizeof(realsavepath));
	
		FS_BuildOSPathForThreadUni(realsavepath, currFileName + startpath, "", toospath, 0);		
		return false;
	}
	FS_BuildOSPathForThreadUni(FS_GetInstallPathUni(bufuni, sizeof(bufuni)), currFileName, "", toospath, 0);
	return true;
}



int Sec_Update( )
{
    char buff[SEC_UPDATE_INITIALBUFFSIZE];
    char *ptr;
    char baseurl[1024];
    char name1[256],name2[256];
    sec_file_t files, *currFile = &files;
    int len;
	wchar_t fromospath[MAX_OSPATH];
	wchar_t toospath[MAX_OSPATH];
	wchar_t binkwospath[MAX_OSPATH];
	wchar_t bufuni[1024];

    if(!Sec_Initialized()){
		return -1;
    }
    
    Com_Printf("\n-----------------------------\n");
    Com_Printf(" CoD4 X Auto Update\n");
    Com_Printf(" Current version: %s\n", Com_GetVersion());
    Com_Printf("-----------------------------\n\n");

	len = Sec_GetUpdateInfo(buff, sizeof(buff));
	if(len < 1)
	{
		return len;
	}


    /* We need to parse filenames etc */
    ptr = Sec_StrTok(buff,"\n",42); // Yes, 42.
    if(ptr == NULL || Q_stricmpn("baseurl: ", ptr, 9))
    {
	    Com_PrintWarning("Sec_Update: Corrupt data from update server. Update aborted.\n");
		return -1;
    }
    Q_strncpyz(baseurl, ptr +9, sizeof(baseurl));

    Com_Memset(&files, 0, sizeof(files));

	bool needElevation = false;

	while(qtrue)
	{
		ptr = Sec_StrTok(NULL,"\n",42); // Yes, 42 again.
		
		if(ptr == NULL)
		{
			break;
		}
		currFile->next = Sec_ParseLine(ptr);
		currFile = currFile->next;

		if(currFile == NULL)
		{
			Sec_FreeFileStruct(files.next);
			Com_PrintError("Update has failed with: Parser error\n");
			return -1;
		}

		bool inroot = Sec_BuildDestinationFilePath(currFile->name, toospath);
		if(Sec_VerifyFile(currFile->hash, toospath))
		{
			currFile->skipped = qtrue;
			continue;
		}
		if(inroot)
		{
			needElevation = true;
		}
	}

	if(needElevation && Sys_TestPermissions() == false)
	{
		Com_PrintStatus("Updater has no write access to some files...\n");	

		//Need elevation to proceed
		Sys_SetRestartParams("+nosplash +set installupdatefiles \"1\"");
		Sec_FreeFileStruct(files.next);
		return 0;
	}

	Com_Printf("\n");
	Com_PrintStatus("Downloading new update files...\n");	

	currFile = files.next;
    do{
		//printf("DEBUG: File to download: link: \"%s\", name: \"%s\", size: %d, hash: \"%s\"\n\n",file.path,file.name,file.size,file.hash);
		if(currFile->skipped == qfalse)
		{
			if((len = Sec_DownloadFile(baseurl, currFile)) < 1)
			{
				Sec_FreeFileStruct(files.next);
				return len;
			}
		}
		currFile = currFile->next;
	}while(currFile != NULL);

	Com_Printf("\n");
	
    Com_PrintStatus("All files downloaded successfully. Applying update files...\n");
	
	DisableExit();
    
	currFile = files.next;
    do{
		if(currFile->skipped)
		{
			Com_PrintStatus("Skipping over file %s\n", currFile->name);
			currFile = currFile->next;
			continue;
		}
		Com_PrintStatus("Updating file %s...\n", currFile->name);

		Q_strncpyz(name1, currFile->name, sizeof(name1));

		Q_strcat(name1, sizeof(name1), ".old");

		Q_strncpyz(name2, currFile->name, sizeof(name2));

		Q_strcat(name2, sizeof(name2), ".new");
		
		//Back up and/or remove old files
		if(FS_SV_FileExists(name1))
		{// Old backup file exists, delete it
			FS_SV_Remove( name1 );
			if(FS_SV_FileExists(name1))
			{
				Com_PrintWarning("Couldn't remove backup file: %s\n", name1);
			}
		}
		// Check if an old file exists with this name
		if(FS_SV_FileExists(currFile->name))
		{ // Old file exists, back it up
			FS_SV_Rename(currFile->name, name1);
		}
		// We couldn't back it up. Now we try to just delete it.
		if(FS_SV_FileExists(currFile->name))
		{
			FS_SV_Remove( currFile->name );
			if(FS_SV_FileExists(currFile->name))
			{
				Com_PrintError("Couldn't remove file: %s\n", currFile->name);
				Sec_FreeFileStruct(files.next);
				return -1;
			}
		}

		//Copy the new file over
		FS_SV_Rename(name2, currFile->name);
		

		FS_BuildOSPathForThreadUni(FS_GetSavePath(), "updates", currFile->name, fromospath, 0 );

		if(Sec_BuildDestinationFilePath(currFile->name, toospath) == false)
		{
			FS_RenameOSPathUni( fromospath, toospath );
		}else{
			//Grepping security information of file binkw32.dll
			PACL pDACL = NULL;
			PSECURITY_DESCRIPTOR pSD = NULL;

			FS_BuildOSPathForThreadUni(FS_GetInstallPathUni(bufuni, sizeof(bufuni)), "binkw32.dll", "", binkwospath, 0);
			if(GetNamedSecurityInfoW(binkwospath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pDACL, NULL, &pSD) != ERROR_SUCCESS)
			{
				Com_PrintError("Failed get security information for file %s. If game does work you need to fix access rights for %s manually.\n", "miles32.dll", currFile->name);
			}
			//Rename file for cod4 root and place it there
			FS_RenameOSPathUni( fromospath, toospath );

			//Apply security information from binkw32.dll to new created file
			if(pSD != NULL)
			{
				if(SetNamedSecurityInfoW(toospath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pDACL, NULL) != ERROR_SUCCESS)
				{
					Com_PrintError("Failed set security information for file %s. If game does work you need to fix access rights for %s manually.\n", currFile->name, currFile->name);

				}
            	LocalFree((HLOCAL) pSD);
			}
		}

		if(!FS_FileExistsOSPathUni(toospath))
		{
			Com_PrintError("Failed to rename file %s to %s\n", name2,currFile->name);
			Com_PrintError("Update has failed!\n");
			Sec_FreeFileStruct(files.next);
			return -1;
		}
		Com_PrintStatus("Update on file %s successfully applied.\n",currFile->name);

		currFile = currFile->next;

    }while(currFile != NULL);

    Sec_FreeFileStruct(files.next);
    Com_PrintStatus("Update finished\n");

	return 0;
}

