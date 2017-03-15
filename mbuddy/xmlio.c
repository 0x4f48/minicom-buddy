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
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "util.h"
#include "common.h"


#define MY_DEBUG            0

#if MY_DEBUG
#define DBG_PRT(fmt, args...)   printf("[XMLIO,%d] "fmt, __LINE__, args)
#else
#define DBG_PRT(fmt, args...)   do {} while (0)
#endif


/* Go through each row and add its data to the xmlDocPtr */
static gboolean save_to_file_foreach(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, xmlNodePtr root)
{
    gchar *title_text, *cmd_text, *pathstr;
    guint type_num;
    xmlNodePtr cur;
    gchar type_text[2] = { 0x00, 0x00};

    /* get the data stored in the model... */
    gtk_tree_model_get(model, iter, 
        COL_0, &type_num, 
        COL_1, &title_text,
        COL_2, &cmd_text,
        -1);

    /* ...and get the path of the current row */
    pathstr = gtk_tree_path_to_string(path);

    /* create a new child of the root node... */
    /* (note that I'm using a (guchar*) cast; this is because it's the same thing as
    * an (xmlChar*) cast, but easier to type) */
    cur = xmlNewChild(root, NULL, (guchar*)"item", NULL);
    /* ...and set some properties */
    type_text[0] = (gchar)(type_num + 0x30);
    xmlSetProp(cur, (guchar*)"type", (guchar*)type_text );
    xmlSetProp(cur, (guchar*)"title", (guchar*)title_text);
    xmlSetProp(cur, (guchar*)"cmd", (guchar*)cmd_text);
    xmlSetProp(cur, (guchar*)"path", (guchar*)pathstr);

    /* free our data we retrieved from the model */
    g_free(title_text);
    g_free(cmd_text);
    g_free(pathstr);

    /* return FALSE to keep iterating */
    return FALSE;
}

/* Function handle saving an xml file; calls save_to_file_foreach */
void xmlio_save_to_file(GtkTreeView *view, gchar *filename)
{
    xmlDocPtr doc;
    xmlNodePtr root;
    GtkTreeModel *model;

    /* try to create a new doc node */
    doc = xmlNewDoc((guchar*)"1.0");
    if(doc == NULL)
      /* handle the error */
      return;

    /* try to create a new root element. You'll want to change "root" to something more specific to your program */
    root = xmlNewDocNode(doc, NULL, (guchar*)"root", NULL);
    if(root == NULL)
    {
      /* handle the error */
      xmlFreeDoc(doc);
      return;
    }
    /* set it as the root element */
    xmlDocSetRootElement(doc, root);

    /* get the tree view's model... */
    model = gtk_tree_view_get_model(view);
    /* ...and go through it with a foreach */
    gtk_tree_model_foreach(model, (GtkTreeModelForeachFunc)save_to_file_foreach, (gpointer)root);

    /* save the actual file */
    //xmlSaveFile(filename, doc);
    xmlSaveFormatFile( filename, doc, 1 );
    /* free the doc node */
    xmlFreeDoc(doc);
}





/* Gets the parent of a path string.
* passing "0:1:2" would return "0:1",
* passing "0:1" would return "0",
* passing "0" would return NULL */
static gchar *gtk_tree_path_string_get_parent(gchar *path)
{
    gchar *colon;

    g_return_val_if_fail(path != NULL, NULL);

    colon = g_strrstr(path, ":");
    if(colon == NULL)
        return NULL;

    return g_strndup(path, colon - path);
}


/* Make sure that path exists within model */
static void gtk_tree_model_generate_path(GtkTreeModel *model, gchar *path)
{
    GtkTreeIter iter, parent;
    gchar *temp;

    while(TRUE)
    {
        /* if this returns TRUE, then this path exists and we're fine */
        if(gtk_tree_model_get_iter_from_string(model, &iter, path))
            break;

        temp = path;
        path = gtk_tree_path_string_get_parent(path);
        /* if there's no parent, then it's toplevel */
        if(path == NULL)
        {
            if(GTK_IS_TREE_STORE(model))
                gtk_tree_store_append(GTK_TREE_STORE(model), &parent, NULL);
            else
                gtk_list_store_append(GTK_LIST_STORE(model), &parent);
            gtk_tree_model_generate_path(model, temp);
            break;
        }
        if(GTK_IS_TREE_STORE(model))
        {
            gtk_tree_model_generate_path(model, path);
            gtk_tree_model_get_iter_from_string(model, &parent, path);
            gtk_tree_store_append(GTK_TREE_STORE(model), &iter, &parent);
        }
    }
}


/* Function to load from an xml file */
void xmlio_load_from_file( GtkTreeView *view, gchar *filename )
{
    xmlDocPtr doc;
    xmlNodePtr cur;

    GtkTreeModel *model;
    GtkTreeIter iter;
    xmlChar *type_text, *title_text, *cmd_text, *path;
    gint type_num;
    
    GdkPixbuf    *group_icon = get_group_icon();
    GdkPixbuf    *cmd_icon   = get_cmd_icon();
    
    type_text  = NULL;
    title_text = NULL;
    cmd_text   = NULL;
    path       = NULL;


    /* load the file */
    doc = xmlParseFile(filename);
    if(doc == NULL)
    {
      /* handle the error */
      goto err_exit;
    }

    /* get the root item */
    cur = xmlDocGetRootElement(doc);
    if(cur == NULL)
    {
        xmlFreeDoc(doc);
        /* handle the error */
        goto free_doc_exit;
    }

    /* get the tree view's model */
    model = gtk_tree_view_get_model(view);

    /* iterate through the root's children items */
    cur = cur->xmlChildrenNode;
    while(cur)
    {

        /* check for the proper element name */
        if(!xmlStrcmp(cur->name, (guchar*)"item"))
        {
            /* get the saved properties */
            type_text = xmlGetProp(cur, (guchar*)"type");
            title_text = xmlGetProp(cur, (guchar*)"title");
            cmd_text = xmlGetProp(cur, (guchar*)"cmd");
            path = xmlGetProp(cur, (guchar*)"path");

            type_num = atoi((char*)type_text);

            /* make sure that the path exists */
            gtk_tree_model_generate_path(model, (gchar*)path);

            /* get an iter to the path */
            gtk_tree_model_get_iter_from_string(model, &iter, (gchar*)path);
             

            /* set the data */
            gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 
                COL_0, type_num, 
                COL_1, title_text,
                COL_2, cmd_text, 
                COL_3, (type_num == 1 ? group_icon : cmd_icon),
                -1);

            /* free the data */
            if ( type_text != NULL )
            {
                xmlFree(type_text);
                type_text = NULL;
            }
            
            if ( title_text != NULL )
            {
                xmlFree(title_text);
                title_text = NULL;
            }
            
            if ( cmd_text != NULL )
            {
                xmlFree(cmd_text);
                cmd_text = NULL;
            }
            
            if ( path != NULL )
            {
                xmlFree(path);
                path = NULL;
            }
        }
        cur = cur->next;
    }

free_doc_exit:
    /* free the doc node */
    xmlFreeDoc(doc);
    
err_exit:
    return;

}
