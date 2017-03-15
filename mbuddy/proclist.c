/* 
 * Copyright (c) 2015, Justin Oh <0x4f48@gmail.com>
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/stat.h>

#include "proclist.h"

static int is_number_only( char* dir_name )
{
    while( *dir_name != '\0' )
    {
        if ( *dir_name > '9' || *dir_name < '0' )
            return 0;
        dir_name++;
    }

    return 1;
}


static int get_full_cmdline( char* cmdline, char* search_path, char * target_name )
{
    int cmd_fd;
    int rd_cnt = 0;
    char rd_buffer[MAX_CMD_LEN];
    int line_cnt = 0;
    int i = 0;

    cmd_fd = open ( search_path, O_RDONLY );

    if ( cmd_fd < 0 )
        return -1;

    {
        rd_cnt = read( cmd_fd, rd_buffer, MAX_CMD_LEN );

        //printf("rd_cnt: %d, <%s>\n", rd_cnt, rd_buffer );
        if ( rd_cnt <= 0 )
            return -1;

        if ( (line_cnt == 0) && strncmp(rd_buffer, target_name, strlen(target_name)) )
            return -1;

        line_cnt++;
        
        for( i = 0 ; i < rd_cnt; i++)
        {

            if ( rd_buffer[i] == '\0' )
                *(cmdline++) = ' ';
            else
                *(cmdline++) = rd_buffer[i];
        }
        rd_buffer[rd_cnt] = '\0';
    }

    close(cmd_fd);

    return 0;  
}

int get_target_proc_cmdline( struct proc_cmd_info *proc_list, char* target_proc_name )
{
    // list all dirs and filter out non-number-only dirs
    DIR           *d;
    struct dirent *dir;
    int cnt = 0;
    
    d = opendir( "/proc" );
    if ( d )
    {

        while ( (dir = readdir(d)) != NULL )
        {
            if ( is_number_only(dir->d_name) )
            {
                // read process name
                char proc_cmd_line[MAX_CMD_FILE_LEN];
                snprintf( proc_cmd_line, MAX_CMD_FILE_LEN,"/proc/%s/cmdline", dir->d_name );

                // check to see if it has process name we're looking for
                {
                    char full_cmdline[MAX_CMD_LEN];

                    memset( &full_cmdline[0], 0x00, sizeof(full_cmdline) );
                    
                    if ( !get_full_cmdline( full_cmdline, proc_cmd_line, target_proc_name ) )
                    {
                        //printf("[%s] %s\n", dir->d_name, full_cmdline );
                        proc_list[cnt].pid = atoi(dir->d_name);
                        proc_list[cnt].full_cmdline = strdup(full_cmdline);
                        cnt++;
                    }
                }
                
            }
        }
        closedir( d );
    }

    return cnt;
}
