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
 
#ifndef __COMMON_H__
#define __COMMON_H__

#define ICON_GROUP          "group_icon.png"
#define ICON_COMMAND        "cmd_icon.png"
#define ICON_MAINWIN        "mbuddy_128x128.png"
#define ICON_ABOUT          "mbuddy_64x64.png"
#define IMAGE_BANNER        "truthisoutthere.png"

#define TYPE_GROUP          0x01
#define TYPE_COMMAND        0x02

#define MAX_FILE_SIZE       4096
#define MAX_PATH_SIZE       256
#define PRIVATE_FOLDER      ".mbuddy"
#define DEFAULT_CMD_FILE    "default.mbd"
#define FIFO_PATH_FORMAT    "/tmp/_minicom_mbuddy_%d_"


enum
{
   COL_0 = 0,   // type, int
   COL_1,       // title, gchararray
   COL_2,       // command, gchararray
   COL_3,       // icon buffer, GdkPixBuff
   N_COLS,
};

#endif
