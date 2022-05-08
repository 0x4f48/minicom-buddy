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
#include <fcntl.h>
#include <string.h>

#include "xmlio.h"
#include "util.h"
#include "mbuddy-gui.h"
#include "proclist.h"
#include "common.h"

#define MY_DEBUG            0

#if MY_DEBUG
#define DBG_PRT(fmt, args...)   printf("[MBUDDY,%d] "fmt, __LINE__, args)
#else
#define DBG_PRT(fmt, args...)   do {} while (0)
#endif

typedef struct main_window_objs
{
    GtkWidget     *main_win;
    GtkWidget     *buddy_dialog;
    GtkWidget     *popup_menu;
    GtkTreeStore  *treeview_store;
    GtkWidget     *command_dialog;
    GtkTreeView   *cmd_treeview;
    
    GtkWidget     *cmd_title;
    GtkWidget     *cmd_typeradio;
    GtkWidget     *cmd_content;
    GtkTextBuffer *cmd_textbuffer;
    GtkWidget     *radio_box;
    GtkWidget     *filename_lable;
    
    GtkImage      *banner;
    GtkWidget     *about_dialog;

    GtkWidget     *pid_combobox;
    GtkTreeStore  *pid_store;
    GtkWidget     *refresh_btn;
    int           uart_number;
    int           conn_status;
      
}MAIN_WINDOW_OBJS;

struct tty_dev_info
{
    int node_number;
    char full_path[64];
};


static gint mbuddy_msg_dialg( GtkWindow* parent, char* message, gint button_type )
{
    GtkWidget *dialog;
    gint response;
    GtkMessageType msg_type;
    
    if ( button_type == GTK_BUTTONS_YES_NO )
    {
        msg_type = GTK_MESSAGE_QUESTION;
    }
    else
    {
        msg_type = GTK_MESSAGE_ERROR;
    }
    
    dialog = gtk_message_dialog_new (parent,
                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                 msg_type,
                                 button_type,
                                 "%s",
                                 message );
                                 
    response = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    
    if ( button_type == GTK_BUTTONS_YES_NO )
    {
        if ( response == GTK_RESPONSE_YES )
            return 1;
        else
            return 0;
            
    }
    return 1;
}


static void send_command( GtkTreePath *path, gpointer user_data )
{
    MAIN_WINDOW_OBJS * main_win_objs = (MAIN_WINDOW_OBJS *)user_data; 
    GtkTreeIter iter;
    unsigned int type;
    char *cmd_text;
    gboolean ok;

    if ( !main_win_objs->conn_status )
        return;        

    /* variable to check return value */
    if ( path != NULL )
    {
        ok = gtk_tree_model_get_iter( (GtkTreeModel*)main_win_objs->treeview_store, &iter, path );
        
        if ( ok == FALSE )
        {
            fprintf( stderr, "ERROR: Fail to get treeview iterator! (%d)\n", __LINE__);
            return; 
        }
    }
    else
    {
        GtkTreeSelection *selected_row;
        gboolean         has_selection;
    
        selected_row =  gtk_tree_view_get_selection (main_win_objs->cmd_treeview);
        has_selection = gtk_tree_selection_get_selected( selected_row, NULL, &iter );
        
        if ( !has_selection )
            return;
    }

    /* if it is group, just return - don't send command */
    gtk_tree_model_get ( (GtkTreeModel*)main_win_objs->treeview_store, 
        &iter,
        COL_0, &type,        
        COL_2, &cmd_text,
        -1 );
    
    if ( type == TYPE_GROUP )
        return;

    /*send text to the target minicom */
    if ( main_win_objs->uart_number >= 0 )
    {
        int uart_fd = 0;
        char uart_dev[MAX_PATH_SIZE];

        snprintf( uart_dev, MAX_PATH_SIZE, "/dev/ttyUSB%d", main_win_objs->uart_number );

        uart_fd = open(uart_dev, O_WRONLY);
        if ( uart_fd > 0 )
        {
            write(uart_fd, cmd_text, strnlen(cmd_text, MAX_FILE_SIZE) );
            // write new line if there is no new line at the end of command
            if ( *(cmd_text+strnlen(cmd_text, MAX_FILE_SIZE)-1) != '\n' )
            {
                write(uart_fd, "\n", 1 );
            }
            close(uart_fd);
        }
        else
        {
            fprintf( stderr, "ERROR: fail to open fifo\n" );
        }
    }

    if ( cmd_text != NULL )
        free ( cmd_text );
}


static void add_command( gpointer user_data )
{
    MAIN_WINDOW_OBJS * main_win_objs = (MAIN_WINDOW_OBJS *)user_data; 
    
    GtkDialog       *dialog          = GTK_DIALOG( main_win_objs->command_dialog );
    GtkEntry        *title_entry     = GTK_ENTRY( main_win_objs->cmd_title );
    GtkTextBuffer   *txt_buffer      = main_win_objs->cmd_textbuffer;   
    GtkToggleButton *toggle_cmdtype  = (GtkToggleButton *)main_win_objs->cmd_typeradio;
    GtkWidget       *radio_box       = main_win_objs->radio_box;
    
    gint result;
    
    /* variables for input dialog */
    const gchar *title_text;
    gchar       *content_text;
    gint        content_len;
    gboolean    is_cmd;
    GdkPixbuf   *icon;
    
    GtkTextIter start_iter;
    GtkTextIter end_iter;
    GtkTreeIter iter;
    
    /* reset title and command text */
    gtk_entry_set_text( title_entry, "" );
    gtk_text_buffer_set_text (txt_buffer, "", 0);
    
    /* enable cmd/group rario */
    gtk_widget_set_sensitive( radio_box, TRUE );
    
    gtk_widget_grab_focus( (GtkWidget*)title_entry );
    
    /* openup dialog */
    result = gtk_dialog_run( dialog );
    
    is_cmd = gtk_toggle_button_get_active(toggle_cmdtype);
    DBG_PRT( "toggle_cmdtype: %p, is_cmd: %s\n", (void*)toggle_cmdtype, is_cmd == TRUE ? "TRUE" : "FALSE" );

    switch (result)
    {
        case GTK_RESPONSE_OK:
        
            if ( is_cmd )
            {
                /* check title and command to see if ther are empty */
                title_text  = gtk_entry_get_text( title_entry ); 
                content_len = gtk_text_buffer_get_char_count( txt_buffer );
                
                if ( title_text == NULL || content_len == 0 )
                {
                    mbuddy_msg_dialg( 0, "Empty input is not allowed!\n", GTK_BUTTONS_CLOSE);
                    break;
                }
                
                DBG_PRT( "Title:  %s\n", title_text );

                /* get text from textbuffer */
                gtk_text_buffer_get_start_iter(txt_buffer, &start_iter );
                gtk_text_buffer_get_end_iter( txt_buffer, &end_iter );
                
                content_text = gtk_text_buffer_get_text( 
                    txt_buffer, &start_iter, &end_iter, FALSE );

                DBG_PRT( "Content: %s\n", content_text );
                
                /* add command */
                {
                    
                    icon = get_cmd_icon();
                    
                    gtk_tree_store_append (main_win_objs->treeview_store, &iter, NULL);

                    gtk_tree_store_set (main_win_objs->treeview_store, &iter, 
                        COL_0, TYPE_COMMAND,
                        COL_1, title_text, 
                        COL_2, content_text,
                        COL_3, icon, 
                        -1 );
                }
            }
            else
            {
                /* user want to create a command group */
                title_text  = gtk_entry_get_text( title_entry ); 
                
                if ( title_text == NULL )
                {
                    mbuddy_msg_dialg( 0, "Empty title is not allowed!\n", GTK_BUTTONS_CLOSE);
                    break;
                }
                
                icon = get_group_icon();
                
                gtk_tree_store_append (main_win_objs->treeview_store, &iter, NULL);
                gtk_tree_store_set (main_win_objs->treeview_store, &iter, 
                    COL_0, TYPE_GROUP,
                    COL_1, title_text, 
                    COL_2, NULL, 
                    COL_3, icon,
                    -1 );
            }
            break;
            
        default:
            //do_nothing_since_dialog_was_cancelled ();
            //DBG_PRT("cancel\n");
            break;
    }
    //gtk_widget_destroy( dialog );
    gtk_widget_hide( (GtkWidget*)dialog );
}


static void edit_command( gpointer user_data )
{
    MAIN_WINDOW_OBJS * main_win_objs = (MAIN_WINDOW_OBJS *)user_data; 
    GtkTreeSelection *selected_row;
    GtkWidget        *radio_box = (GtkWidget *)(main_win_objs->radio_box);
    GtkTreeIter      iter;
    gboolean         has_selection;
    
    
    /* find selected row */
    selected_row =  gtk_tree_view_get_selection (main_win_objs->cmd_treeview);
    
    has_selection = gtk_tree_selection_get_selected( selected_row, NULL, &iter );

    if ( has_selection )
    {
        GtkDialog        *dialog         = GTK_DIALOG( main_win_objs->command_dialog );
        GtkEntry         *title_entry    = GTK_ENTRY( main_win_objs->cmd_title );
        GtkTextBuffer    *txt_buffer     = main_win_objs->cmd_textbuffer;   
        
        GtkTextIter      start_iter;
        GtkTextIter      end_iter;
        
        const gchar      *title_text;
        gchar            *content_text;
        gint             content_len;
        
        char *current_title = NULL;
        char *current_cmd = NULL;
        
        guint     type;
        int       ret;
    
        /* get the key and query title and commadn */
        gtk_tree_model_get ( (GtkTreeModel*)main_win_objs->treeview_store, &iter, 
            COL_0, &type,
            COL_1, &current_title, 
            COL_2, &current_cmd,
            -1 );
        
        /* disable cmd/group rario */
        gtk_widget_set_sensitive( radio_box, FALSE );
        
        /* set title and command text */
        gtk_entry_set_text( title_entry, current_title );
        gtk_text_buffer_set_text (txt_buffer, current_cmd, strlen(current_cmd));
        gtk_widget_grab_focus( (GtkWidget*)title_entry );
        
         /* openup dialog */
        ret = gtk_dialog_run( dialog );

        switch (ret)
        {
            case GTK_RESPONSE_OK:
            
                if ( type == TYPE_GROUP )
                {
                    title_text  = gtk_entry_get_text( title_entry );
                    
                    if ( title_text == NULL )
                    {
                        mbuddy_msg_dialg( NULL , "Empty input, will not be updated", GTK_BUTTONS_CLOSE);
                        break;
                    }
                    
                    gtk_tree_store_set (main_win_objs->treeview_store, 
                        &iter, 
                        COL_1, title_text, 
                        -1 );
                }
                else
                {
                    title_text  = gtk_entry_get_text( title_entry ); 
                    content_len = gtk_text_buffer_get_char_count( txt_buffer );
                    
                    if ( title_text == NULL || content_len == 0 )
                    {
                        // TODO: display warning message box or retry
                        mbuddy_msg_dialg( NULL , "Empty input, will not be updated", GTK_BUTTONS_CLOSE);
                        break;
                    }
                    
                    /* get text from textbuffer */
                    gtk_text_buffer_get_start_iter(txt_buffer, &start_iter );
                    gtk_text_buffer_get_end_iter( txt_buffer, &end_iter );
                
                    content_text = gtk_text_buffer_get_text( 
                        txt_buffer, &start_iter, &end_iter, FALSE );
                        
                    gtk_tree_store_set (main_win_objs->treeview_store, 
                        &iter, 
                        COL_1, title_text, 
                        COL_2, content_text, 
                        -1 );
                }
            
                break;
             
             default:
                break;
        }
        gtk_widget_hide( (GtkWidget*)dialog );
        //gtk_widget_destroy( (GtkWidget*)dialog );
        
        if ( current_title != NULL )
            free( current_title );
        
        if ( current_cmd != NULL )
            free( current_cmd );
    }

}


static void duplicate_command( gpointer user_data )
{
    MAIN_WINDOW_OBJS * main_win_objs = (MAIN_WINDOW_OBJS *)user_data; 
    GtkTreeSelection *selected_row;
    GtkWidget        *radio_box = (GtkWidget *)(main_win_objs->radio_box);
    GtkTreeIter      iter;
    gboolean         has_selection;
    
    
    /* find selected row */
    selected_row =  gtk_tree_view_get_selection (main_win_objs->cmd_treeview);
    
    has_selection = gtk_tree_selection_get_selected( selected_row, NULL, &iter );

    if ( has_selection )
    {
        GtkDialog        *dialog         = GTK_DIALOG( main_win_objs->command_dialog );
        GtkEntry         *title_entry    = GTK_ENTRY( main_win_objs->cmd_title );
        GtkTextBuffer    *txt_buffer     = main_win_objs->cmd_textbuffer;   
        
        GtkTextIter      start_iter;
        GtkTextIter      end_iter;
        
        const gchar      *title_text;
        gchar            *content_text;
        gint             content_len;
        
        char *current_title = NULL;
        char *current_cmd = NULL;
        
        guint     type;
        int       ret;
    
        /* get the key and query title and commadn */
        gtk_tree_model_get ( (GtkTreeModel*)main_win_objs->treeview_store, &iter, 
            COL_0, &type,
            COL_1, &current_title, 
            COL_2, &current_cmd,
            -1 );
        
        if ( type == TYPE_GROUP )
            return;
            
        /* enable cmd/group rario */
        gtk_widget_set_sensitive( radio_box, TRUE );
        
        /* set title and command text */
        gtk_entry_set_text( title_entry, current_title );
        gtk_text_buffer_set_text (txt_buffer, current_cmd, strlen(current_cmd));
        gtk_widget_grab_focus( (GtkWidget*)title_entry );
        
         /* openup dialog */
        ret = gtk_dialog_run( dialog );

        switch (ret)
        {
            case GTK_RESPONSE_OK:
            
                if ( type == TYPE_GROUP )
                {
                    title_text  = gtk_entry_get_text( title_entry );
                    
                    if ( title_text == NULL )
                    {
                        mbuddy_msg_dialg( NULL , "Empty input, will not be updated", GTK_BUTTONS_CLOSE);
                        break;
                    }
                    
                    gtk_tree_store_set (main_win_objs->treeview_store, 
                        &iter, 
                        COL_1, title_text, 
                        -1 );
                }
                else
                {
                    title_text  = gtk_entry_get_text( title_entry ); 
                    content_len = gtk_text_buffer_get_char_count( txt_buffer );
                    
                    if ( title_text == NULL || content_len == 0 )
                    {
                        // TODO: display warning message box or retry
                        mbuddy_msg_dialg( NULL , "Empty input, will not be updated", GTK_BUTTONS_CLOSE);
                        break;
                    }
                    
                    /* get text from textbuffer */
                    gtk_text_buffer_get_start_iter(txt_buffer, &start_iter );
                    gtk_text_buffer_get_end_iter( txt_buffer, &end_iter );
                
                    content_text = gtk_text_buffer_get_text( 
                        txt_buffer, &start_iter, &end_iter, FALSE );
                        
                     /* add command */
                    {
                        GdkPixbuf   *icon;
                        
                        icon = get_cmd_icon();
                        
                        gtk_tree_store_append (main_win_objs->treeview_store, &iter, NULL);

                        gtk_tree_store_set (main_win_objs->treeview_store, &iter, 
                            COL_0, TYPE_COMMAND,
                            COL_1, title_text, 
                            COL_2, content_text,
                            COL_3, icon, 
                            -1 );
                    }
                }
            
                break;
             
             default:
                break;
        }
        gtk_widget_hide( (GtkWidget*)dialog );
        
        if ( current_title != NULL )
            free( current_title );
        
        if ( current_cmd != NULL )
            free( current_cmd );
    }

}


static void del_command( gpointer user_data )
{
    MAIN_WINDOW_OBJS *main_win_objs = (MAIN_WINDOW_OBJS *)user_data; 
    
    GtkTreeSelection *selected_row;
    GtkTreeIter      iter;
    
    guint            type;
    gboolean         has_selection;

    
    /* find selected row */
    selected_row =  gtk_tree_view_get_selection (main_win_objs->cmd_treeview);
    
    has_selection = gtk_tree_selection_get_selected( selected_row, NULL, &iter );

    if ( has_selection )
    {
        /* ask user confirmation */
        if ( mbuddy_msg_dialg( NULL, "Do you want to delete?\n", GTK_BUTTONS_YES_NO ) )
        {
            gtk_tree_model_get ( (GtkTreeModel*)main_win_objs->treeview_store, 
                &iter, COL_0, &type, -1 );
            
            if ( type == TYPE_GROUP )
            {
                GtkTreeIter child_iter;
                DBG_PRT("group delete... %d\n", __LINE__ );
                
                if ( gtk_tree_model_iter_children((GtkTreeModel *)main_win_objs->treeview_store, &child_iter, &iter ) )
                {
                    gboolean succss;
                    do
                    {
                        succss = gtk_tree_store_remove ( main_win_objs->treeview_store, &child_iter );
                    } while( succss );
                }
                gtk_tree_store_remove ( main_win_objs->treeview_store, &iter );
            }
            else
            {
                DBG_PRT("command delete... %d\n", __LINE__ );
                
                /* remove the row from the tree */
                gtk_tree_store_remove ( main_win_objs->treeview_store, &iter );
            }
        }
    }
}


/*
 * handling double click on a row in the treeview 
 */
void on_treeview_cmdlist_row_activated(GtkTreeView *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            userdata)
{
    send_command( path, userdata);
}


/*
 * edit
 */
gboolean on_menuitem_edit_button_release_event( GtkWidget *widget, GdkEvent* event, gpointer data )
{
    edit_command( data );
    return FALSE;
}

/*
 * delete a command entry
 */ 
gboolean on_menuitem_del_button_release_event( GtkWidget *widget, GdkEvent* event, gpointer data )
{
    del_command( data );
    return FALSE;
}

/*
 *  create new command entry
 */
gboolean on_menuitem_new_button_release_event( GtkWidget *widget, GdkEvent* event, gpointer data )
{
    add_command( data );
    return FALSE;
}

/*
 * duplicate selected item
 * 
 */
gboolean on_menuitem_duplicate_button_release_event( GtkWidget *widget, GdkEvent* event, gpointer data )
{
    duplicate_command( data );
    return FALSE;
}


// gtk2.0 doesn't have this API, so make my own version
gboolean gtk_tree_model_iter_previous( GtkTreeModel* tree_model, GtkTreeIter* iter )
{
    GtkTreePath* current_path;
    GtkTreePath* prev_path;
    gboolean has_path;
    
    // iter has current iterator
    current_path = gtk_tree_model_get_path( tree_model, iter ); 
    prev_path = current_path;
    
    has_path = gtk_tree_path_prev( prev_path );
    
    if ( has_path )
    {
        gtk_tree_model_get_iter( tree_model, iter, prev_path);

        gtk_tree_path_free(current_path);
        return TRUE;
    }
    
    gtk_tree_path_free(current_path);
    return FALSE;
}



gboolean on_menuitem_move_up_button_release_event(GtkWidget *widget, GdkEvent* event, gpointer data)
{
    MAIN_WINDOW_OBJS *main_win_objs = (MAIN_WINDOW_OBJS *)data; 
    GtkTreeSelection *selected_row;
    GtkTreeIter      iter;
    gboolean         has_selection;
    
    // find current row and see if there is a previous child node
    /* find selected row */
    selected_row =  gtk_tree_view_get_selection (main_win_objs->cmd_treeview);
    
    has_selection = gtk_tree_selection_get_selected( selected_row, NULL, &iter );
    
    if ( has_selection )
    {
        GtkTreeIter prev_iter = iter;
             
        if ( gtk_tree_model_iter_previous( (GtkTreeModel*)main_win_objs->treeview_store, &prev_iter) )
        {
            gtk_tree_store_move_before( (GtkTreeStore *)main_win_objs->treeview_store,
                                        &iter,
                                        &prev_iter);
        }

    }
 
    return FALSE;
}

gboolean on_menuitem_move_down_button_release_event(GtkWidget *widget, GdkEvent* event, gpointer data)
{

    MAIN_WINDOW_OBJS *main_win_objs = (MAIN_WINDOW_OBJS *)data; 
    GtkTreeSelection *selected_row;
    GtkTreeIter      iter;
    gboolean         has_selection;
    
    // find current row and see if there is a next child node
    selected_row =  gtk_tree_view_get_selection (main_win_objs->cmd_treeview);
    
    has_selection = gtk_tree_selection_get_selected( selected_row, NULL, &iter );
    
    if ( has_selection )
    {
        GtkTreeIter next_iter = iter;
        if ( gtk_tree_model_iter_next( (GtkTreeModel*)main_win_objs->treeview_store, &next_iter) )
        {
            gtk_tree_store_move_after( (GtkTreeStore *)main_win_objs->treeview_store,
                                        &iter,
                                        &next_iter);
        }

    }
    
    return FALSE;
}


void on_radiobtn_cmd_toggled(GtkToggleButton *togglebutton, gpointer user_data )
{
    MAIN_WINDOW_OBJS *main_win_objs = (MAIN_WINDOW_OBJS *)user_data; 
    
    GtkWidget *cmd_textview = main_win_objs->cmd_content;
    gboolean is_cmd;
    
    DBG_PRT("radio toggled .. (%d), togglebutton: %p\n", __LINE__, (void*)togglebutton );
    is_cmd = gtk_toggle_button_get_active (togglebutton);
    
    /* enable/disable command editing window */
    gtk_widget_set_sensitive ( cmd_textview, is_cmd );
    
}

void on_button_new_clicked( GtkButton *button, gpointer user_data ) 
{
    DBG_PRT("button pressed .. (%d)\n", __LINE__ );
    add_command( user_data );
}

void on_button_send_clicked( GtkButton *button, gpointer user_data )
{
    DBG_PRT("button pressed .. (%d)\n", __LINE__ );
    send_command( NULL, user_data );
} 

void on_button_edit_clicked( GtkButton *button, gpointer user_data ) 
{
    DBG_PRT("button pressed .. (%d)\n", __LINE__ );
    edit_command( user_data );
}

void on_button_del_clicked( GtkButton *button, gpointer user_data ) 
{
    DBG_PRT("button pressed .. (%d)\n", __LINE__ );
    del_command( user_data );
}

void on_chkbtn_alwaystop_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    MAIN_WINDOW_OBJS * main_win_objs = (MAIN_WINDOW_OBJS *)user_data; 
    GtkWindow *main_window = (GtkWindow*)main_win_objs->main_win;
    
    gboolean is_toggled;
     
    is_toggled = gtk_toggle_button_get_active(togglebutton);
    
    if ( is_toggled )
    {
        gtk_window_set_keep_above( main_window, TRUE );
    }
    else
    {
        gtk_window_set_keep_above( main_window, FALSE );
    }
}


gboolean on_treeview_cmdlist_button_press_event(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
    if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
    {
        GtkWidget *popup_menu;

        popup_menu = ((MAIN_WINDOW_OBJS*)userdata)->popup_menu;
                
        DBG_PRT ("popup_menu: %p\n", popup_menu);

        /* open up context menu */
        gtk_menu_popup(GTK_MENU(popup_menu), NULL, NULL, NULL, NULL, event->button, event->time);
        
        return TRUE;
    }
    
    return FALSE;
}

gboolean on_treeview_cmdlist_drag_drop ( GtkWidget *widget,
                                         GdkDragContext *context,
                                         gint x,
                                         gint y,
                                         guint time,
                                         gpointer user_data)
{
    MAIN_WINDOW_OBJS * main_win_objs = (MAIN_WINDOW_OBJS *)user_data; 
    
    gboolean                    is_row;
    GtkTreeModel                *tree_model;
    GtkTreePath                 *path = NULL;
    GtkTreeViewDropPosition     pos;
    GtkTreeIter                 iter;
    guint                       src_type   = 0;
    guint                       dest_type  = 0;
    guint                       dest_depth = 0;
    
    tree_model = (GtkTreeModel*)main_win_objs->treeview_store;
    
    is_row = gtk_tree_view_get_dest_row_at_pos ( main_win_objs->cmd_treeview,
                                                 x,
                                                 y,
                                                 &path,
                                                 &pos );

    if ( is_row )
    {
        gtk_tree_model_get_iter( tree_model, &iter, path );
        gtk_tree_model_get ( tree_model, &iter, COL_0, &dest_type, -1 );
        
        dest_depth = gtk_tree_path_get_depth(path);
        
        DBG_PRT("dest_type  : %d\n", dest_type );
        DBG_PRT("path depth : %d\n", dest_depth );
        
        if ( dest_depth < 3 )
        {

            if ( pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE || pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER )            
            {
                /* src dropped on an item */
                if ( (src_type == TYPE_COMMAND && dest_type == TYPE_COMMAND) || 
                     (src_type == TYPE_GROUP   && dest_type == TYPE_GROUP)   || 
                     (src_type == TYPE_GROUP   && dest_type == TYPE_COMMAND) )
                {
                    /* move command onto a command */
                    return TRUE;
                }
            }
                
        }
    }
    else
    {
        return TRUE;
    }
    
    return FALSE;
}


int save_to_file( gboolean is_saveas, gpointer user_data )
{
     MAIN_WINDOW_OBJS *main_win_objs = (MAIN_WINDOW_OBJS *)user_data; 
     gchar *window_title;
     int dlg_ret;
     int ret_code = 1;
    
    /* open up dialog to get the file name */
    GtkWidget *dialog;
    
    if ( is_saveas )
        window_title = strdup("Save as ...");
    else
        window_title = strdup("Save ...");
        
    
    dialog = gtk_file_chooser_dialog_new (window_title,
                          (GtkWindow*)main_win_objs->main_win,
                          GTK_FILE_CHOOSER_ACTION_SAVE,
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                          NULL);
    
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

    
    if (is_saveas || get_current_cmd_file() == NULL )
    {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), get_private_data_path() );
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), DEFAULT_CMD_FILE);
    }
    else
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), get_current_cmd_file() );

    dlg_ret = gtk_dialog_run (GTK_DIALOG (dialog));
    
    if ( dlg_ret == GTK_RESPONSE_ACCEPT )
    {
        gchar *filename;
        
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        
        DBG_PRT("save file: %s\n", filename );
        
        xmlio_save_to_file( main_win_objs->cmd_treeview, filename );
        
        DBG_PRT("set_current_cmd_file: %s\n", filename );
        
        /* update current command file name */
        set_current_cmd_file( filename );
        
        gtk_label_set_text( (GtkLabel*)main_win_objs->filename_lable, get_current_cmd_file() );
        
        g_free (filename);
    }
    else
    {
        ret_code = -1;
    }
    
    free(window_title);
    gtk_widget_destroy (dialog);
    
    return ret_code;
    
}


/*
 * 
 * new cmd file
 * 
 */
void on_filemenu_new_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    MAIN_WINDOW_OBJS *main_win_objs = (MAIN_WINDOW_OBJS *)user_data; 
    GtkTreeStore     *tree_store;
    char             *current_file;
    
    tree_store = (GtkTreeStore*)gtk_tree_view_get_model( main_win_objs->cmd_treeview );
    
    /* save current file, clear items */
    current_file = get_current_cmd_file();
    
    if ( current_file != NULL )
    {
        xmlio_save_to_file( main_win_objs->cmd_treeview, current_file );
    }
    else
    {
        save_to_file( FALSE, user_data );
    }
    
    /* unload tree items */
    gtk_tree_store_clear( tree_store );
    
    /* reset current file */
    set_current_cmd_file( NULL );
    gtk_label_set_text( (GtkLabel*)main_win_objs->filename_lable, "(No file name)" );
}

/*
 * 
 * open a cmd file
 * 
 */
void on_filemenu_open_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    MAIN_WINDOW_OBJS *main_win_objs = (MAIN_WINDOW_OBJS *)user_data; 
    GtkTreeStore     *tree_store;
    GtkWidget        *dialog;
    
    tree_store = (GtkTreeStore*)gtk_tree_view_get_model( main_win_objs->cmd_treeview );

    dialog = gtk_file_chooser_dialog_new ("Open File",
                          (GtkWindow*)main_win_objs->main_win,
                          GTK_FILE_CHOOSER_ACTION_OPEN,
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                          NULL);
                          
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), get_private_data_path() );

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        gchar *filename = NULL;
        
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        
        DBG_PRT("open file name: %s\n", filename );
       
        /* unload tree items */
        gtk_tree_store_clear( tree_store );

        /* load up the file */
        xmlio_load_from_file( main_win_objs->cmd_treeview, filename );
        
        /* save new command file */
        set_current_cmd_file( filename );
        gtk_label_set_text( (GtkLabel*)main_win_objs->filename_lable, get_current_cmd_file() );
        
        g_free (filename);
    }
    
    gtk_widget_destroy (dialog);
    
}

/*
 *  save current command items into the currently opend file
 * 
 */
void on_filemenu_save_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    MAIN_WINDOW_OBJS * main_win_objs = (MAIN_WINDOW_OBJS *)user_data; 
    
    if ( get_current_cmd_file() != NULL )
    {
        DBG_PRT ("save...%s\n", get_current_cmd_file() );
    
        xmlio_save_to_file ( main_win_objs->cmd_treeview, get_current_cmd_file() );
    }
    else
    {
        /* ask new file */
        save_to_file( FALSE, user_data );
    }
}


/* 
 * 
 * save as .. save current file with a new file name
 *  
 */
void on_filemenu_saveas_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    save_to_file( TRUE, user_data );
}

void on_helpmenu_about_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    MAIN_WINDOW_OBJS *main_win_objs = (MAIN_WINDOW_OBJS *)user_data; 
    
    load_aboutdialog_icon( (GtkAboutDialog *)main_win_objs->about_dialog );
    
    gtk_dialog_run ( (GtkDialog *)main_win_objs->about_dialog );
    
    gtk_widget_hide( main_win_objs->about_dialog );
}


/*
 * main window destroy event handler 
 * 
 */
void on_main_window_destroy( GtkWidget *widget, gpointer user_data )
{
    MAIN_WINDOW_OBJS *main_win_objs = (MAIN_WINDOW_OBJS *)user_data; 
    char* current_cmd_file = NULL;
    
    current_cmd_file = get_current_cmd_file();
    
    /* save current tree */
    if ( current_cmd_file != NULL )
    {
        xmlio_save_to_file( main_win_objs->cmd_treeview, current_cmd_file );
        
        /* write currenly opend file to lastfile */
        write_last_cmd_file_path( current_cmd_file );
        free ( current_cmd_file );
    }
    else
    {
        if ( save_to_file( FALSE, user_data ) > 0 )
        {
            current_cmd_file = get_current_cmd_file();
            write_last_cmd_file_path( current_cmd_file );
            free ( current_cmd_file );
        }
    }
    
    if ( get_private_data_path() != NULL )
        free( get_private_data_path() );
    
    /* exit */
    gtk_main_quit ();
}

int get_usb2uart_dev_list(struct tty_dev_info* tty_devs)
{
        // list all dirs and filter out non-number-only dirs
    DIR           *d;
    struct dirent *dir;
    int cnt = 0;

    d = opendir( "/dev" );
    if ( d )
    {
        while ( (dir = readdir(d)) != NULL )
        {
            char* tty_node = 0;
            tty_node = strstr( dir->d_name, "ttyUSB" );
            if ( tty_node != NULL )
            {
                tty_devs[cnt].node_number = atoi(tty_node+strlen("ttyUSB"));
                sprintf(tty_devs[cnt].full_path, "/dev/%s",tty_node);
                //printf("[%d] %s\n", tty_devs[cnt].node_number, tty_devs[cnt].full_path);
                cnt++;
            }
        }
    }
    closedir( d );
    return cnt;
}


void on_button_refresh_clicked( GtkButton *button, gpointer user_data )
{
    MAIN_WINDOW_OBJS *main_win_objs = (MAIN_WINDOW_OBJS *)user_data;
    GtkTreeIter iter;

    struct tty_dev_info dev_info[32];
    int cnt = 0;
    int i = 0;

    /* clear all the items */
    gtk_tree_store_clear(main_win_objs->pid_store);

    /* get minicom pid list */
    cnt = get_usb2uart_dev_list(dev_info);

    for ( i = 0; i < cnt ; i++ )
    {
        gtk_tree_store_append (main_win_objs->pid_store, &iter, NULL);

        gtk_tree_store_set (
            main_win_objs->pid_store, &iter,
            COL_0, dev_info[i].node_number,
            COL_1, dev_info[i].full_path,
            -1 );
    }

    /* set first item as active item */
    if ( cnt > 0 )
        gtk_combo_box_set_active( (GtkComboBox*)main_win_objs->pid_combobox, 0 );
}



void on_button_conn_clicked( GtkButton *button, gpointer user_data )
{
    MAIN_WINDOW_OBJS *main_win_objs = (MAIN_WINDOW_OBJS *)user_data;
    GtkTreeIter iter;
printf("main_win_objs->conn_status: %d\n", main_win_objs->conn_status );
    if ( main_win_objs->conn_status )
    {
        /* current : connected */
        main_win_objs->conn_status = 0;
        main_win_objs->uart_number = 999;

        gtk_button_set_label( button, "Connect" );
        // enable combobox
        gtk_widget_set_sensitive( (GtkWidget*)main_win_objs->pid_combobox, TRUE );
        // enable refresh
        gtk_widget_set_sensitive( (GtkWidget*)main_win_objs->refresh_btn, TRUE );
    }
    else
    {
        /* current : disconnected */
        if( gtk_combo_box_get_active_iter((GtkComboBox*)main_win_objs->pid_combobox, &iter) )
        {
            gtk_tree_model_get ( (GtkTreeModel*)main_win_objs->pid_store,
                                &iter,
                                COL_0, &main_win_objs->uart_number,
                                -1 );

            // set status
            main_win_objs->conn_status = 1;
            // change text
            gtk_button_set_label( button, "Disconnect" );
            // disable combobox
            gtk_widget_set_sensitive( (GtkWidget*)main_win_objs->pid_combobox, FALSE );
            // disable refresh
            gtk_widget_set_sensitive( (GtkWidget*)main_win_objs->refresh_btn, FALSE );
        }
    }
    
}

/*
 * Main entry of the application
 */ 

int main (int argc, char *argv[])
{
    GtkBuilder          *builder; 
    MAIN_WINDOW_OBJS    main_win_objs;

    gtk_init (&argc, &argv);


    builder = gtk_builder_new ();

    gtk_builder_add_from_string (builder, (gchar*)mbuddy_gui_glade, sizeof(mbuddy_gui_glade), NULL );
    
    main_win_objs.main_win = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
    
    /* get objects */
    main_win_objs.buddy_dialog      = GTK_WIDGET (gtk_builder_get_object( builder, "buddy_msg_dialog" ));
    main_win_objs.popup_menu        = GTK_WIDGET (gtk_builder_get_object( builder, "cmd_treeview_popup" ));
    main_win_objs.command_dialog    = GTK_WIDGET (gtk_builder_get_object( builder, "input_dialog" ));
    main_win_objs.cmd_title         = GTK_WIDGET (gtk_builder_get_object( builder, "entry_title" ));
    main_win_objs.cmd_typeradio     = GTK_WIDGET (gtk_builder_get_object( builder, "radiobtn_cmd" ));
    main_win_objs.cmd_content       = GTK_WIDGET (gtk_builder_get_object( builder, "textview_content" ));
    main_win_objs.cmd_treeview      = (GtkTreeView*) (gtk_builder_get_object( builder, "treeview_cmdlist" ));
    main_win_objs.radio_box         = GTK_WIDGET (gtk_builder_get_object( builder, "hbox_radio" ));
    
    main_win_objs.cmd_textbuffer    = (GtkTextBuffer*)(gtk_builder_get_object( builder, "textbuffer_cmd" ));
    main_win_objs.treeview_store    = (GtkTreeStore* )(gtk_builder_get_object( builder, "cmd_treestore" ));
    main_win_objs.banner            = (GtkImage*)(gtk_builder_get_object( builder, "banner_image" ));
    main_win_objs.about_dialog      = GTK_WIDGET (gtk_builder_get_object( builder, "aboutdialog" ));
    main_win_objs.filename_lable    = GTK_WIDGET (gtk_builder_get_object( builder, "label_file_name" ));

    main_win_objs.pid_combobox      = GTK_WIDGET (gtk_builder_get_object( builder, "combobox_minicom_pid" ));
    main_win_objs.pid_store         = (GtkTreeStore* )(gtk_builder_get_object( builder, "pid_treestore" ));
    main_win_objs.refresh_btn       = GTK_WIDGET (gtk_builder_get_object( builder, "button_refresh" ));


    main_win_objs.conn_status = 0;

    /* add buttons on the dialog */
    gtk_dialog_add_buttons( (GtkDialog *)main_win_objs.command_dialog, "Cancel", GTK_RESPONSE_CANCEL,
        "OK", GTK_RESPONSE_OK, NULL );
        
    /* connect signal */
    gtk_builder_connect_signals (builder, &main_win_objs);
    
    gtk_widget_show (main_win_objs.main_win);
    
    /* load file */
    set_private_data_path();
    load_window_icon( (GtkWindow*)main_win_objs.main_win );
    load_banner( main_win_objs.banner );
    load_treeview_icons();
    load_last_cmd_file( main_win_objs.cmd_treeview );
    
    /* set file name label */
    gtk_label_set_text( (GtkLabel*)main_win_objs.filename_lable, get_current_cmd_file() );
 
    gtk_main();

    /* we need to unref the objs when we exit */
    g_object_unref (G_OBJECT (builder));

    return 0;
}
