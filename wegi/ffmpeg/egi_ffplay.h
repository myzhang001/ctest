/*----------------------------------------------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Based on:
          FFmpeg examples in code sources.
				       --- FFmpeg ORG
          dranger.com/ffmpeg/tutorialxx.c
 				       ---  by Martin Bohme
	  muroa.org/?q=node/11
				       ---  by Martin Runge
	  www.xuebuyuan.com/1624253.html
				       ---  by niwenxian


Midas Zhou
midaszhou@yahoo.com
-----------------------------------------------------------------------------------------------------------*/
#ifndef __EGI_FFPLAY_H__
#define __EGI_FFPLAY_H__

#include <signal.h>
#include <math.h>
#include <string.h>

#include "egi_common.h"
#include "sound/egi_pcm.h"

/* ffplay control command signal */
enum ffplay_cmd {
        cmd_none=0,
        cmd_play,
        cmd_pause,
        cmd_quit, /* release all resource and quit ffplay */
        cmd_next,
        cmd_prev,
	cmd_mode,

        cmd_exit_display_thread,   /* stop display thread */
        cmd_exit_subtitle_thread    /* stop subtitle tread */
};
enum ffplay_mode
{
        mode_loop_all=0,   /* loop all files in the list */
        mode_repeat_one, /* repeat current file */
        mode_shuffle,    /* pick next file randomly */
};
enum ffplay_status
{
        status_stop=0,
        status_playing,
        status_pausing,
};


/* param: ( enable_seek_loop )
 *   if 1:	loop one single file/stream forever.
 *   if 0:	play one time only.
 *   NOTE: if TRUE, then curretn input seek postin will be ignored.!!! It always start from the
 *     	   very beginning of the file.
 */
/* loop seeking and playing from the start of the same file */
extern bool enable_seekloop;

/* param: ( enable_filesloop )
 *   if 1:	loop playing file(s) in playlist forever, if the file is unrecognizable then skip it
 *		and continue to try next one.
 *   if 0:	play one time for every file in playlist.
 */
extern bool enable_filesloop;

/* param: ( enable_audio )
 *   if 1:	enable audio playback.
 *   if 0:	disable audio playback.
 */
extern bool disable_audio;

/* param: ( play mode )
 *   mode_loop_all:	loop all files in the list
 *   mode_repeat_one:	repeat current file
 *   mode_shuffle:	pick next file randomly
 */
extern enum ffplay_mode playmode;

/* param: subtitle path */
extern char *subpath;


typedef struct FFplay_Context
{
	int	  ftotal;    	/* Total number of media files path in array 'fpath' */
	char     **fpath;   	/* Array of media file paths */

	long start_tmsecs;  	/* start time, default 0 */

	enum ffplay_cmd     ffcmd;      /* 0 default none*/
	enum ffplay_mode    ffmode;	/* 0 default mode_loop_all */
	enum ffplay_status  ffstatus;	/* 0 default mode_stop */
}FFPLAY_CONTEXT;

extern FFPLAY_CONTEXT *FFplay_Ctx;


/*------------------------------------------------------------------
		A Thread Function
Note:
1. Prepare/set  EGI_UI environment before start this thread/

-------------------------------------------------------------------*/
void * egi_thread_ffplay(EGI_PAGE *page);



#endif
