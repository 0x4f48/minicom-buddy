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
 
#include <gtk/gtk.h>
#include <stdlib.h>
#include <dirent.h> 
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "common.h"
#include "xmlio.h"

#define MY_DEBUG            0

#if MY_DEBUG
#define DBG_PRT(fmt, args...)   printf("[UTIL,%d] "fmt, __LINE__, args)
#else
#define DBG_PRT(fmt, args...)   do {} while (0)
#endif


static gchar        *current_cmd_file;
static gchar        *private_data_path;
static GdkPixbuf    *group_icon;
static GdkPixbuf    *cmd_icon;

/*
 * 
 * this function reads the "lastfile" to retrieve last command file path
 * 
 */
static int read_last_cmd_file_path( char** file_path )
{
    int  fd, read_size;
    char last_file_path[MAX_FILE_SIZE];
    char file_buffer[MAX_FILE_SIZE];
        
    sprintf( last_file_path, "%s/lastfile", private_data_path );
    
    DBG_PRT("last file: %s\n", last_file_path );
    
    fd = open( last_file_path, O_RDONLY );
    
    if ( fd < 0 )
        return -1;
    
    memset( file_buffer, 0x00, sizeof(file_buffer) );  
    read_size = read( fd, file_buffer, MAX_FILE_SIZE );
    
    DBG_PRT("read size: %d\n", read_size );
    DBG_PRT("file_buffer: %s\n", file_buffer );
    
    close( fd );
    
    if ( read_size > 0 )
        *file_path = strdup( file_buffer );
    else
        return -1;
    
    return 1;
}

/*
 * 
 * this function writes last command file into the "lastfile"
 * 
 */
int write_last_cmd_file_path( char* data )
{
    FILE *wr_file;
    char last_file_path[MAX_FILE_SIZE];
    
    sprintf( last_file_path, "%s/lastfile", private_data_path );
    
    DBG_PRT( "last file: %s\n", last_file_path );
    
    wr_file = fopen( last_file_path, "w");
    
    if ( !wr_file )
        return -1;
      
    fprintf( wr_file, "%s", data );
    
    fclose(wr_file);
    
    return 1;
}

/*
 * 
 * this function finds the last command file and load it
 * 
 */
void load_last_cmd_file( GtkTreeView *tree_view )
{
    DIR *d;
        
    if ( !private_data_path )
        return;
        
    d = opendir(private_data_path);
    
    if( d ) 
    {
        char last_file_path[4096];
        sprintf( last_file_path, "%s/lastfile", private_data_path );
        
        DBG_PRT( "last_file_path: %s\n", last_file_path );
        
        if( access( last_file_path, F_OK ) != -1 ) 
        {
            /* file exists */
            char *last_cmd_file_path;
            
            if ( read_last_cmd_file_path( &last_cmd_file_path ) > 0 )
            {
                current_cmd_file = strdup( last_cmd_file_path );
                xmlio_load_from_file( tree_view, current_cmd_file );
            }
        }
    }
    else
    {
        DBG_PRT( "Fail to open... %s\n", strerror(errno) );

        if ( errno == ENOENT)
        {
            /* create .mbuddy folder */
            mkdir(private_data_path, 0700);
        }
    }
    
}

/*
 * 
 * sets application private path 
 * 
 */
void set_private_data_path()
{
    struct passwd   *pw      = getpwuid(getuid());
    const char      *homedir = pw->pw_dir;
    

    DBG_PRT( "Home dir: %s\n", homedir );
    
    private_data_path = (char*)malloc( strlen(homedir) + strlen(PRIVATE_FOLDER) + 1 );
    
    if ( !private_data_path ) 
    {
        fprintf( stderr, "fail to alloc private data path\n");
        return;
    }
        
    sprintf( private_data_path, "%s/"PRIVATE_FOLDER, homedir );

}

/*
 * 
 * sets window's icon
 * 
 */
void load_window_icon( GtkWindow* window )
{
    GError          *error = NULL;
    char            win_icon_path[MAX_FILE_SIZE];
    
    if ( !private_data_path )
    {
        fprintf( stderr, "private path is not set!\n");
        return;
    }
    
    sprintf( win_icon_path, "%s/images/"ICON_MAINWIN, private_data_path );
    
    /* set window icon */
    gtk_window_set_icon_from_file ( window, win_icon_path, &error );
}

/*
 * 
 * sets icon of about dialog
 * 
 */
void load_aboutdialog_icon( GtkAboutDialog *aboutdlg )
{
    char            icon_file_path[MAX_FILE_SIZE];
    GError          *error = NULL;
    GdkPixbuf       *icon;
    
    if ( !private_data_path )
    {
        fprintf( stderr, "private path is not set!\n");
        return;
    }
    
    sprintf( icon_file_path, "%s/images/"ICON_ABOUT, private_data_path );
    
    icon = gdk_pixbuf_new_from_file( icon_file_path, &error );

    gtk_about_dialog_set_logo ( aboutdlg, icon);
}


/*
 * 
 *  load icon of treeview icons
 * 
 */    
void load_treeview_icons()
{
    char            icon_grp_file_path[MAX_FILE_SIZE];
    char            icon_cmd_file_path[MAX_FILE_SIZE];
    GError          *error = NULL;
    
    if ( !private_data_path )
    {
        fprintf( stderr, "private path is not set!\n");
        return;
    }
    
    sprintf( icon_grp_file_path, "%s/images/"ICON_GROUP, private_data_path );
    sprintf( icon_cmd_file_path, "%s/images/"ICON_COMMAND, private_data_path );

    error = NULL;
    group_icon  = gdk_pixbuf_new_from_file( icon_grp_file_path, &error );
    error = NULL;
    cmd_icon    = gdk_pixbuf_new_from_file( icon_cmd_file_path, &error );
    
    

}

void load_banner( GtkImage* image )
{
    char            banner_path[MAX_FILE_SIZE];
    
    if ( !private_data_path )
    {
        fprintf( stderr, "private path is not set!\n");
        return;
    }
    
    sprintf( banner_path, "%s/images/"IMAGE_BANNER, private_data_path );
    
    gtk_image_set_from_file (image, banner_path );
}

char* get_current_cmd_file()
{
    return (char*)current_cmd_file;
}

void set_current_cmd_file( char* file_name )
{
    DBG_PRT("[%d] current_cmd_file : %p\n", __LINE__, current_cmd_file );
    
    if ( current_cmd_file != NULL )
        free(current_cmd_file);
    
    if( file_name == NULL)
        current_cmd_file = NULL;
    else
        current_cmd_file =  strdup(file_name);
        
    DBG_PRT("[%d] current_cmd_file : %p, %s\n", __LINE__, current_cmd_file, current_cmd_file );
}

char* get_private_data_path()
{
    return (char*)private_data_path;
}

GdkPixbuf* get_group_icon()
{
    return group_icon;
}

GdkPixbuf* get_cmd_icon()
{
    return cmd_icon;
}

//------------------------------------------------------------------


