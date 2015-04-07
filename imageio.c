/**
 * Disk image IO functions for gendosfs
 */
/* BSD License
 * Copyright (c) 2012, developer@teamboyce.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of the Team Boyce Limited nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2012        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "diskio.h"		/* FatFs lower layer API */


static inline int image_open( int drv )
{
    int fd;
    int flags = 0;
   
    switch( drv )
    {
        case OUTPUT_IMAGE : flags = O_RDWR ; break;
        case INPUT_IMAGE : flags = O_RDONLY; break;
    }

    fd = open( imageNames[ drv ], flags );
    if( fd < 0 )
    {
        perror( "image open fail" );
        exit( -1 );
    }

    return fd;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE drv				/* Physical drive nmuber (0..) */
)
{
    DSTATUS stat = STA_NOINIT;

    if( NULL == imageNames[ drv ])
    {
        stat = RES_NOTRDY;
    }
    else
    {
        switch ( drv )
        {
            case OUTPUT_IMAGE :
                stat = RES_OK;
            break;
            case INPUT_IMAGE :
                stat = RES_OK;
            break;
        }
    }

    return stat;
}



/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{
        drv = 0;
	return drv;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..128) */
)
{
    int fd = image_open( drv );
    
    lseek( fd, sector * 512, SEEK_SET );
    read( fd, buff, count * 512 );
    close( fd );

    return 0;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..128) */
)
{
    int fd = image_open( drv );
    lseek( fd, sector * 512, SEEK_SET );
    write( fd, buff, count * 512 );
    close( fd );

    return RES_OK;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
    struct stat stat;
    DRESULT res = RES_PARERR;
    int fd;

    if( CTRL_SYNC == ctrl )
    {
        return RES_OK;
    }
    if( GET_SECTOR_SIZE == ctrl )
    {
        *(WORD*)buff = 512;
        return RES_OK;
    }
    fd = image_open( drv );
    fstat( fd, &stat );
    close( fd );
    if( GET_SECTOR_COUNT == ctrl )
    {
        *(DWORD*)buff = stat.st_blocks;
        return RES_OK;
    }
    if( GET_BLOCK_SIZE == ctrl )
    {
        *(DWORD*)buff = 1;
         return RES_OK;
    }

    return res;
}
#endif
