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
#ifndef SEC_UPDATE_H
#define SEC_UPDATE_H
//#include "sec_init.h"
#include "q_platform.h"
//#include "version.h"


int Sec_Update(  );


typedef struct sec_file_s{
    char name[MAX_OSPATH];
    char path[MAX_OSPATH];
    int size;
    char hash[4096];
    struct sec_file_s *next;
}sec_file_t;


#define SEC_UPDATE_VERSION "1.0"

#define SEC_UPDATE_INITIALBUFFSIZE 10240


//#undef QUOTE
#define SEC_UPDATE_HOST "cod4update.cod4x.me/clupdate"
#define SEC_UPDATE_USER_AGENT "CoD4X AutoUpdater V. " SEC_UPDATE_VERSION
//#define SEC_UPDATE_BOUNDARY "------------------------------------874ryg7v"
#define SEC_UPDATE_PORT 80

#define SEC_UPDATE_DOWNLOAD(baseurl, qpath) "%s%s", baseurl, qpath

#endif
