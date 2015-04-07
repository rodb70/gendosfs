/**
 *
 */
/* BSD License
 * Copyright (c) 2011, developer@teamboyce.com
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
#include <stdio.h>
#include <popt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>

#include "ff.h"		

static int showDebug = 0;
static int showVersion = 0;
static int fakeTime = 0;
static int fillValue = 0;
static int blockSize = 0;
static char *rootDir;
static char *startImage;
static char *outputFilename = NULL;
typedef enum 
{
    OUTPUT_IMAGE,
    INPUT_IMAGE,
    NUM_IMAGES
} DISK_ID;

static FATFS fatFs[ NUM_IMAGES ];
char *imageNames[ NUM_IMAGES ];

struct poptOption optionsTable[] =
{
    { "output-filename",'o', POPT_ARG_STRING, &outputFilename,0,"Output filename", 0 },
//    { "starting-image",'x', POPT_ARG_STRING, &startImage, 0, "Starting image", 0 },
    { "root",          'd', POPT_ARG_STRING, &rootDir,    0, "Directory to read files from", 0 },
    { "size-in-block", 'b', POPT_ARG_INT,   &blockSize,   0, "size of disk in blocks", 0 },
    { "fill-value",    'e', POPT_ARG_INT,   &fillValue,   0, "Fill unallocated blocks with value.", 0 },
    { "faketime",      'f', POPT_ARG_NONE,  &fakeTime,    1, "Set filesystem timestamps to 0.", 0 },
    { "version",       'V', POPT_ARG_NONE,  &showVersion, 1, "Display version info", 0 },
    { "verbose",       'v', POPT_ARG_NONE,  &showDebug,   1, "Be verbose", 0 },
    POPT_AUTOHELP
    POPT_TABLEEND
};


static int create_disk_image_backing( char *filename, int num_blocks, int block_size, int fill_value )
{
    int fhandle;
    unsigned char *block = malloc( block_size );
    int i;

    memset( block, fill_value, block_size );
    fhandle = open( filename, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP );
    if( fhandle >= 0 )
    {
        for( i = 0; i < num_blocks; i++ )
        {
            write( fhandle, block, block_size );
        }
        close( fhandle );
        fhandle = 0;
    }

    return fhandle;
}

static int gendosfs_copy_from_dir( char *src, TCHAR *dest )
{
    int sfd;
    FIL dfl;
    int rc = 0;
    int written, sread;
    char buf[ 512 ];

    sfd = open( src, O_RDONLY );
    rc = f_open( &dfl, (TCHAR*)dest, FA_WRITE | FA_CREATE_ALWAYS );

    if( 0 == rc && sfd >= 0 )
    {
        do {
            sread = read( sfd, buf, sizeof( buf ));
            if( sread > 0 )
            {
                rc = f_write( &dfl, buf, sread, (unsigned int *)&written );
                if( sread != written )
                {
                    printf( "image write failed read %d written %d\n", sread, written );
                    exit( -1 );
                }
            }
         } while ( sread > 0 );
        f_close( &dfl );
        close( sfd );
    }

    return 0;
}

static int char_to_tchar( TCHAR *out, char *input, int len )
{
    int i;

    for( i = 0; i < len; i++ )
    {
        *(out + i ) = ff_convert( *(input + i ), 1 );
    }

    return 0;
}

/* static int tchar_to_char( char *out, TCHAR *input, int len )
{
    int i;

    for( i = 0; i < len; i++ )
    {
        *(out + i ) = ff_convert( *(input + i ), 0 );
    }

    return 0;
} */

static int gendosfs_copy_directory( char *dir )
{
    DIR *d;
    struct dirent *dentry;
    int rc;
    char path[ PATH_MAX ];
    char ipath[ PATH_MAX ], *pipath;
    TCHAR opath[ PATH_MAX ];

    d = opendir( dir );
    if( NULL != d )
    {
        while( NULL != (dentry = readdir( d )))
        {
            if(( 0 == strcmp( dentry->d_name, "." )) || ( 0 ==  strcmp( dentry->d_name, ".." )))
            {
                continue;
            }
            memset( path, 0, sizeof( path ));
            memset( ipath, 0, sizeof( ipath ));
            memset( opath, 0, sizeof( opath ));
            (void)snprintf( path, sizeof( path ), "%s/%s", dir, dentry->d_name );
            if( DT_DIR == ( dentry->d_type & DT_DIR ))
            {
                /* Mkdir directory in image */
                pipath = dir + strlen( rootDir ) + 1;
                snprintf( ipath, sizeof( ipath ), "0:/%s/%s", pipath,  dentry->d_name );
                char_to_tchar( opath, ipath, strlen( ipath ));
                rc = f_mkdir( opath );
                printf( "f_mkdir(%s) returned %d\n", ipath, rc );
               /* recurse into directory */
               gendosfs_copy_directory( path );
            }
            else if( DT_REG == ( dentry->d_type & DT_REG ))
            {
                /* Copy file into image */
                pipath = dir + strlen( rootDir ) + 1;
                snprintf( ipath, sizeof( ipath ), "0:/%s/%s", pipath,  dentry->d_name );
                char_to_tchar( opath, ipath, strlen( ipath ));
                (void)gendosfs_copy_from_dir( path, opath );
            }
            else if( DT_LNK == ( dentry->d_type & DT_LNK ))
            {
                /* get file link is pointing to */
                char actualfile[ PATH_MAX ];
                struct stat st;

                readlink( dentry->d_name, actualfile, sizeof( actualfile ));
                stat( actualfile, &st );
                if( 0 != S_ISREG( st.st_mode ))
                {
                    /* copy file to image */
                    pipath = dir + strlen( rootDir ) + 1;
                    snprintf( ipath, sizeof( ipath ), "0:/%s/%s", pipath,  dentry->d_name );
                    char_to_tchar( opath, ipath, strlen( ipath ));
                    (void)gendosfs_copy_from_dir( path, opath );
                }
                else
                {
                    /* ignore if not file */
                    printf( "link %s pointing to %s is not a regular file ignoring\n",  dentry->d_name, actualfile );
                }
            }
            else
            {
                printf( "%s - is not valid on DOS file system\n", dentry->d_name );
            }
        } 
    }

    return 0;
}

/*static int gendosfs_copy_from_image( TCHAR *dirName )
{
    FRESULT rc;
    F_DIR dir;
    FILINFO flInfo;
    TCHAR ipath[ PATH_MAX ];
    TCHAR opath[ PATH_MAX ]; 

    rc = f_opendir( &dir, dirName );
    if( FR_OK == rc )
    {
        
    }
}*/

int main( const int argc, const char *argv[])
{
    int rc;
    poptContext optCon; /* context for parsing command-line options */
    char c; /* used for argument parsing */ 

    optCon = poptGetContext( NULL, argc, argv, optionsTable, 0 );

    if( argc < 2 )
    {
        poptPrintUsage( optCon, stdout, 0 );
        return 1;
    }

    while(( c = poptGetNextOpt( optCon )) >= 0 )
    {
    }

    imageNames[ OUTPUT_IMAGE ] = outputFilename;
    imageNames[ INPUT_IMAGE  ] = startImage;

    printf( "show debug = %d, show version = %d, faketime = %d, file value = 0x%x, block size = %d, root = %s, start image = %s, output image = %s\n",
            showDebug, showVersion, fakeTime, fillValue, blockSize, rootDir, startImage, outputFilename );

    printf( "Create disk image backing store\n" );
    create_disk_image_backing( outputFilename, blockSize, 512, fillValue );

    rc = f_mount( OUTPUT_IMAGE, &fatFs[ OUTPUT_IMAGE ]);
    printf( "f_mount OUTPUT_IMAGE return %d\n", rc );

    if( 0 == rc )
    {
    	rc = f_mkfs( OUTPUT_IMAGE, 1, 512 );
    	printf( "f_mkfs OUTPUT_IMAGE returned %d\n", rc );
    }

/*    if( NULL != startImage )
    {
        rc = f_mount( INPUT_IMAGE, &fatFs[ INPUT_IMAGE ]);
        printf( "f_mount INPUT_IMAGE return %d\n", rc );

        rc = gendosfs_copy_from_image( _T( "1:/" ));
        printf( "gendosfs_copy_from_image( %s ) = %d\n", startImage, rc );
    }
*/
    if( NULL != rootDir && rc == 0 )
    {
        rc = gendosfs_copy_directory( rootDir );
        printf( "gendosfs_copy_directory returned %d\n", rc );
    }

    return rc;
}

DWORD get_fattime (void)
{
    return 0;
}

