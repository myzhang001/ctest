/*----------------------- egi_txt.c ------------------------------
egi txt tyep ebox functions

Midas Zhou
----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
//#include <signal.h>
#include <string.h>
#include "egi_color.h"
#include "egi.h"
#include "egi_txt.h"
#include "egi_debug.h"
//#include "egi_timer.h"
#include "egi_symbol.h"
#include "bmpjpg.h"


/*-------------------------------------
      txt ebox self_defined methods
--------------------------------------*/
static EGI_METHOD txtbox_method=
{
        .activate=egi_txtbox_activate,
        .refresh=egi_txtbox_refresh,
        .decorate=egi_txtbox_decorate,
        .sleep=egi_txtbox_sleep,
        .free=NULL, //egi_ebox_free,
};


/*-----------------------------------------------------------------------------
Dynamically create txt_data struct

return:
	poiter 		OK
Y	NULL		fail
-----------------------------------------------------------------------------*/
EGI_DATA_TXT *egi_txtdata_new(int offx, int offy,
	int nl,
	int llen,
	struct symbol_page *font,
	uint16_t color
)

{
	int i,j;

	/* malloc a egi_data_txt struct */
	egi_pdebug(DBG_TXT,"egi_txtdata_new(): malloc data_txt ...\n");
	EGI_DATA_TXT *data_txt=malloc(sizeof(EGI_DATA_TXT));
	if(data_txt==NULL)
	{
		printf("egi_txtdata_new(): fail to malloc egi_data_txt.\n");
		return NULL;
	}
	/* clear up data */
	memset(data_txt,0,sizeof(EGI_DATA_TXT));

	/* assign parameters */
        data_txt->nl=nl;
        data_txt->llen=llen;
        data_txt->font=font;
        data_txt->color=color;
        data_txt->offx=offx;
        data_txt->offy=offy;

        /*  malloc data->txt  */
	egi_pdebug(DBG_TXT,"egi_txtdata_new(): malloc data_txt->txt ...\n");
        data_txt->txt=malloc(nl*sizeof(char *));
        if(data_txt->txt == NULL) /* malloc **txt */
        {
                printf("egi_txtdata_new(): fail to malloc egi_data_txt->txt!\n");
		free(data_txt);
		data_txt=NULL; /* :( */
                return NULL;
        }
	memset(data_txt->txt,0,nl*sizeof(char *));

	/* malloc data->txt[] */
        for(i=0;i<nl;i++)
        {
		egi_pdebug(DBG_TXT,"egi_txtdata_new(): start to malloc data_txt->txt[%d]...\n",i);
                data_txt->txt[i]=malloc(llen*sizeof(char));
                if(data_txt->txt[i] == NULL) /* malloc **txt */
                {
                        printf("egi_txtdata_new(): fail to malloc egi_data_txt->txt[]!\n");
			/* roll back to free malloced */
                        for(j=0;j<i;j++)
                        {
                                free(data_txt->txt[j]);
                                data_txt->txt[j]=NULL; /* :< */
                        }
                        free(data_txt->txt);
                        data_txt->txt=NULL; /* :) */
			free(data_txt);
			data_txt=NULL; /* :))) ...JUST TO EMPHOSIZE THE IMPORTANCE !!!  */

                        return NULL;
                }

                /* clear up data */
                memset(data_txt->txt[i],0,llen*sizeof(char));
        }

	/* finally! */
	return data_txt;
}


/*-----------------------------------------------------------------------------
Dynamically create a new txtbox object

return:
	poiter 		OK
	NULL		fail
-----------------------------------------------------------------------------*/
EGI_EBOX * egi_txtbox_new( char *tag,
	/* parameters for concept ebox */
	EGI_DATA_TXT *egi_data,
	bool movable,
	int x0, int y0,
	int width, int height,
	int frame,
	int prmcolor
)

{
	EGI_EBOX *ebox;

	/* 0. check egi_data */
	if(egi_data==NULL)
	{
		printf("egi_txtbox_new(): egi_data is NULL. \n");
		return NULL;
	}

	/* 1. create a new common ebox */
	egi_pdebug(DBG_TXT,"egi_txtbox_new(): start to egi_ebox_new(type_txt)...\n");
	ebox=egi_ebox_new(type_txt);// egi_data NOT allocated in egi_ebox_new()!!! , (void *)egi_data);
	if(ebox==NULL)
	{
		printf("egi_txtbox_new(): fail to execute egi_ebox_new(type_txt). \n");
		return NULL;
	}

	/* 2. default method assigned in egi_ebox_new() */

	/* 3. txt ebox object method */
	egi_pdebug(DBG_TXT,"egi_txtbox_new(): assign defined mehtod ebox->method=methd...\n");
	ebox->method=txtbox_method;

	/* 4. fill in elements  */
	egi_ebox_settag(ebox,tag);
	//strncpy(ebox->tag,tag,EGI_TAG_LENGTH); /* addtion EGI_TAG_LENGTH+1 for end token here */
	ebox->egi_data=egi_data; /* ----- assign egi data here !!!!! */
	ebox->movable=movable;
	ebox->x0=x0;	ebox->y0=y0;
	ebox->width=width;	ebox->height=height;
	ebox->frame=frame;	ebox->prmcolor=prmcolor;

	/* 5. pointer default */
	egi_pdebug(DBG_TXT,"egi_txtbox_new(): assign ebox->bkimg=NULL ...\n");
	ebox->bkimg=NULL;

	egi_pdebug(DBG_TXT,"egi_txtbox_new(): finish.\n");
	return ebox;
}


/*--------------------------------------------------------------------------------
initialize EGI_DATA_TXT according

offx,offy:			offset from prime ebox
int nl:   			number of txt lines
int llen:  			in byte, data length for each line,
			!!!!! -	llen deciding howmany symbols it may hold.
struct symbol_page *font:	txt font
uint16_t color:     		txt color
char **txt:       		multiline txt data

Return:
        non-null        OK
        NULL            fails
---------------------------------------------------------------------------------*/
EGI_DATA_TXT *egi_init_data_txt(EGI_DATA_TXT *data_txt,
			int offx, int offy, int nl, int llen, struct symbol_page *font, uint16_t color)
{
	int i,j;

	/* check data first */
	if(data_txt == NULL)
	{
		printf("egi_init_data_txt(): data_txt is NULL!\n");
		return NULL;
	}
	if(data_txt->txt != NULL) /* if data_txt defined statically, txt may NOT be NULL !!! */
	{
		printf("egi_init_data_txt(): ---WARNING!!!--- data_txt->txt is NOT NULL!\n");
		return NULL;
	}
	if( nl<1 || llen<1 )
	{
		printf("egi_init_data_txt(): data_txt->nl or llen is 0!\n");
		return NULL;
	}
	if( font == NULL )
	{
		printf("egi_init_data_txt(): data_txt->font is NULL!\n");
		return NULL;
	}

	/* --- assign var ---- */
	data_txt->nl=nl;
	data_txt->llen=llen;
	data_txt->font=font;
	data_txt->color=color;
	data_txt->offx=offx;
	data_txt->offy=offy;

	/* --- malloc data --- */
	data_txt->txt=malloc(nl*sizeof(char *));
	if(data_txt->txt == NULL) /* malloc **txt */
	{
		printf("egi_init_ebox(): fail to malloc egi_data_txt->txt!\n");
		return NULL;
	}
	for(i=0;i<nl;i++) /* malloc *txt */
	{
		data_txt->txt[i]=malloc(llen*sizeof(char));
		if(data_txt->txt[i] == NULL) /* malloc **txt */
		{
			printf("egi_init_ebox(): fail to malloc egi_data_txt->txt[]!\n");
			for(j=0;j<i;j++)
			{
				free(data_txt->txt[j]);
				data_txt->txt[j]=NULL;
			}
			free(data_txt->txt);
			data_txt->txt=NULL;

			return NULL;
		}
		/* clear up data */
		memset(data_txt->txt[i],0,llen*sizeof(char));
	}

	return data_txt;
}


/*-------------------------------------------------------------------------------------
activate a txt ebox:
	0. if ebox is in a sleep_status, just refresh it, and reset txt file pos offset.
	1. adjust ebox height and width according to its font line set
 	2. store back image covering txtbox frame range.
	3. refresh the ebox.
	4. change status token to active,

TODO:
	if ebox_size is re-sizable dynamically, bkimg mem size MUST adjusted.

Return:
	0	OK
	<0	fails!

----------------------------------------------------------------------------------*/
int egi_txtbox_activate(EGI_EBOX *ebox)
{
//	int i,j;
	int ret;
	int x0=ebox->x0;
	int y0=ebox->y0;
	int height=ebox->height;
	int width=ebox->width;
	EGI_DATA_TXT *data_txt=(EGI_DATA_TXT *)(ebox->egi_data);

	/* 0. check data */
	if(data_txt == NULL)
	{
                printf("egi_txtbox_activate(): data_txt is NULL in ebox '%s'!\n",ebox->tag);
                return -1;
	}
	if(height==0 || width==0)
	{
                printf("egi_txtbox_activate(): height or width is 0 in ebox '%s'!\n",ebox->tag);
                return -1;
	}

	int nl=data_txt->nl;
//	int llen=data_txt->llen;
//	int offx=data_txt->offx;
	int offy=data_txt->offy;
//	char **txt=data_txt->txt;
	int font_height=data_txt->font->symheight;

	/* 1. confirm ebox type */
        if(ebox->type != type_txt)
        {
                printf("egi_txtbox_activate(): '%s' is not a txt type ebox!\n",ebox->tag);
                return -2;
        }


        egi_pdebug(DBG_TXT,"egi_txtbox_activate(): start to activate '%s' txt type ebox!\n",ebox->tag);
	/* 2. activate(or wake up) a sleeping ebox
		not necessary to adjust ebox size and allocate bkimg memory for a slpeeping ebox
	*/
	if(ebox->status==status_sleep)
	{
		((EGI_DATA_TXT *)(ebox->egi_data))->foff=0; /* reset affliated file position */
		ebox->status=status_active; /* reset status */
		if(egi_txtbox_refresh(ebox)!=0) /* refresh the graphic display */
		{
			ebox->status=status_sleep; /* reset status */
			printf("egi_txtbox_activate():fail to wake up sleeping ebox '%s'!\n",ebox->tag);
			return -3;
		}

		printf("egi_txtbox_activate(): wake up a sleeping '%s' ebox.\n",ebox->tag);
		return 0;
	}

        /* 3. check ebox height and font lines, then adjust the height */
        height= (font_height*nl+offy)>height ? (font_height*nl+offy) : height;
        ebox->height=height;

	//TODO: malloc more mem in case ebox size is enlarged later????? //
	/* 4. malloc exbo->bkimg for bk image storing */
   if( ebox->movable || (ebox->prmcolor<0) ) /* only if ebox is movale or it's transparent */
   {
	egi_pdebug(DBG_TXT,"egi_txtbox_activate(): start to egi_alloc_bkimg() for '%s' ebox. height=%d, width=%d \n",
											ebox->tag,height,width);
		/* egi_alloc_bkimg() will check width and height */
		if( egi_alloc_bkimg(ebox, width, height)==NULL )
        	{
               	 	printf("egi_txtbox_activate(): fail to egi_alloc_bkimg() for '%s' ebox!\n",ebox->tag);
                	return -4;
        	}
		egi_pdebug(DBG_TXT,"egi_txtbox_activate(): finish egi_alloc_bkimg() for '%s' ebox.\n",ebox->tag);


	/* 5. store bk image which will be restored when this ebox position/size changes */
	/* define bkimg box */
	ebox->bkbox.startxy.x=x0;
	ebox->bkbox.startxy.y=y0;
	ebox->bkbox.endxy.x=x0+width-1;
	ebox->bkbox.endxy.y=y0+height-1;
#if 0 /* DEBUG */
	egi_pdebug(DBG_TXT,"txt activate... fb_cpyto_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x, ebox->bkbox.endxy.y);
#endif

	egi_pdebug(DBG_TXT,"egi_txtbox_activate(): start fb_cpyto_buf() for '%s' ebox.\n",ebox->tag);
	if(fb_cpyto_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
				ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) < 0 )
		return -5;
  } /* ebox->movable codes end */

	/* 6. change its status, if not, you can not refresh.  */
	ebox->status=status_active;

	/* 7. reset offset for txt file if fpath applys */
	//???? NOT activate ????? ((EGI_DATA_TXT *)(ebox->egi_data))->foff=0;

	/* set for refresh */
	ebox->need_refresh=true;
	/* 8. refresh displaying the ebox */
	ret=egi_txtbox_refresh(ebox);
	if(ret != 0)
	{
		printf("egi_txtbox_activate(): WARNING!! egi_txtbox_refresh(ebox) return with %d !=0.\n", ret);
		return -6;
	}
	egi_pdebug(DBG_TXT,"egi_txtbox_activate(): a '%s' ebox is activated.\n",ebox->tag);
	return 0;
}


/*-------------------------------------------------------------------------------
refresh a txt ebox.
	1.refresh ebox image according to following parameter updates:
		---txt(offx,offy,nl,llen)
		---size(height,width)
		---positon(x0,y0)
		---ebox color
	----NOPE!!! affect bkimg size !!!----  2. adjust ebox height and width according to its font line set
	3.restore backgroud from bkimg and store new position backgroud to bkimg.
	4.refresh ebox color if ebox->prmcolor >0, and draw frame.
 	5.update txt, or read txt file to it.
	6.write txt to FB and display it.

Return
	2	fail to read txt file.
	1	need_refresh=false
	0	OK
	<0	fail
-------------------------------------------------------------------------------*/
int egi_txtbox_refresh(EGI_EBOX *ebox)
{
	int i;
	int ret=0;



	/* 1. check data */
	if(ebox->type != type_txt)
	{
		printf("egi_txtbox_refresh(): Not txt type ebox!\n");
		return -1;
	}

	/* 2. check the ebox status */
	if( ebox->status != status_active )
	{
		egi_pdebug(DBG_TXT,"egi_txtbox_refresh(): This '%s' ebox is not active! refresh action is ignored! \n",ebox->tag);
		return -2;
	}

	/* only if need_refresh=true */
	if(!ebox->need_refresh)
	{
		egi_pdebug(DBG_TXT,"egi_txtbox_refresh(): need_refresh is false!\n");
		return 1;
	}


	/* 4. get updated ebox parameters */
	int x0=ebox->x0;
	int y0=ebox->y0;
	int height=ebox->height;
	int width=ebox->width;
	egi_pdebug(DBG_TXT,"egi_txtbox_refresh():start to assign data_txt=(EGI_DATA_TXT *)(ebox->egi_data)\n");
	EGI_DATA_TXT *data_txt=(EGI_DATA_TXT *)(ebox->egi_data);
	int nl=data_txt->nl;
//	int llen=data_txt->llen;
	int offx=data_txt->offx;
	int offy=data_txt->offy;
	char **txt=data_txt->txt;
	int font_height=data_txt->font->symheight;

#if 0
	/* test WARNING!!!! fonts only !!!--------------   print out box txt content */
	for(i=0;i<nl;i++)
		printf("egi_txtbox_refresh(): txt[%d]:%s\n",i,txt[i]);
#endif

        /* redefine bkimg box range, in case it changes
	 check ebox height and font lines, then adjust the height */
	height= (font_height*nl+offy)>height ? (font_height*nl+offy) : height;
	ebox->height=height;


   /* ------ restore bkimg and buffer new bkimg
   ONLY IF:
	1. the txt ebox is movable,
	   and ebox size and position is changed.
	2. or data_txt->.prmcolor <0 , it's transparent. EGI_NOPRIM_COLOR
   */
   if ( ( ebox->movable && ( (ebox->bkbox.startxy.x!=x0) || (ebox->bkbox.startxy.y!=y0)
			|| ( ebox->bkbox.endxy.x!=x0+width-1) || (ebox->bkbox.endxy.y!=y0+height-1) ) )
           || (ebox->prmcolor<0)  )
   {
#if 0 /* DEBUG */
	egi_pdebug(DBG_TXT,"txt refresh... fb_cpyfrom_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x,ebox->bkbox.endxy.y);
#endif
	/* restore bk image before refresh */
	egi_pdebug(DBG_TXT,"egi_txtbox_refresh(): fb_cpyfrom_buf() before refresh...\n");
        if(fb_cpyfrom_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
                               ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) <0 )
		return -3;

	/* store new coordinates of the ebox
	    updata bkimg->bkbox according */
        ebox->bkbox.startxy.x=x0;
        ebox->bkbox.startxy.y=y0;
        ebox->bkbox.endxy.x=x0+width-1;
        ebox->bkbox.endxy.y=y0+height-1;

#if 1 /* DEBUG */
	egi_pdebug(DBG_TXT,"refresh() fb_cpyto_buf: startxy(%d,%d)   endxy(%d,%d)\n",ebox->bkbox.startxy.x,ebox->bkbox.startxy.y,
			ebox->bkbox.endxy.x,ebox->bkbox.endxy.y);
#endif
        /* ---- 6. store bk image which will be restored when this ebox position/size changes */
	egi_pdebug(DBG_TXT,"egi_txtbox_refresh(): fb_cpyto_buf() before refresh...\n");
        if( fb_cpyto_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
                                ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) < 0)
		return -4;

   } /* end of movable code */

	/* ---- 7. refresh prime color under the symbol  before updating txt.  */
	if(ebox->prmcolor >= 0)
	{
		/* set color and draw filled rectangle */
		fbset_color(ebox->prmcolor);
		draw_filled_rect(&gv_fb_dev,x0,y0,x0+width-1,y0+height-1);
	}

	/* --- 8. draw frame according to its type  --- */
	if(ebox->frame >= 0) /* 0: simple type */
	{
		fbset_color(0); /* use black as frame color  */
		draw_rect(&gv_fb_dev,x0,y0,x0+width-1,y0+height-1);

		if(ebox->frame >0) /* draw double line */
			draw_rect(&gv_fb_dev,x0+1,y0+1,x0+width-2,y0+height-2);
	}
	/* TODO: other type of frame .....*/

	/* ---- 9. if data_txt->fpath !=NULL, then re-read txt file to txt[][] */
	if(data_txt->fpath)
	{
		egi_pdebug(DBG_TXT,"egi_txtbox_refresh():  data_txt->fpath is NOT null, re-read txt file now...\n");
		if(egi_txtbox_readfile(ebox,data_txt->fpath)<0) /* not = 0 */
		{
			printf("egi_txtbox_refresh(): fail to read txt file: %s \n",data_txt->fpath);
			ret=2;
		}
	}

	/* ---- 10. refresh TXT, write txt line to FB */
	egi_pdebug(DBG_TXT,"egi_txtbox_refresh(): start symbol_string_writeFB(), font color=%d ...\n", data_txt->color);
	for(i=0;i<nl;i++)
	{
		egi_pdebug(DBG_TXT,"egi_txtbox_refresh(): txt[%d]='%s' \n",i,txt[i]);
		/*  (fb_dev,font, font_color,transpcolor, x0,y0, char*)...
					1, for font/icon symbol: tranpcolor is its img symbol bkcolor!!! */
		symbol_string_writeFB(&gv_fb_dev, data_txt->font, data_txt->color, 1, x0+offx, \
						 y0+offy+font_height*i, txt[i]);

	}

	/* ---- 4. reset need_refresh */
	ebox->need_refresh=false;

	return ret;
}


/*-----------------------------------------------------
put a txt ebox to sleep
1. restore bkimg
2. reset status

return
	0 	OK
	<0 	fail

------------------------------------------------------*/
int egi_txtbox_sleep(EGI_EBOX *ebox)
{
   	if(ebox->movable) /* only for movable ebox */
   	{
		/* restore bkimg */
       		if(fb_cpyfrom_buf(&gv_fb_dev, ebox->bkbox.startxy.x, ebox->bkbox.startxy.y,
                               ebox->bkbox.endxy.x, ebox->bkbox.endxy.y, ebox->bkimg) <0 )
		{
			printf("egi_txtbox_sleep(): fail to restor bkimg.\n");
                	return -1;
		}
   	}

	/* reset status */
	ebox->status=status_sleep;

	egi_pdebug(DBG_TXT,"egi_txtbox_sleep(): a '%s' ebox is put to sleep.\n",ebox->tag);
	return 0;
}


/*-----------------------------------------------------------------
Note:
1. Read a txt file and try to push it to egi_data_txt->txt[]
2. If it reaches the end of file, then reset offset and roll back.
3. llen=data_txt->llen-1  one byte for string end /0.
4. conditions for line return:
	4.1 Max. char number per line   =  llen;
	4.2 Max. pixel number per line  =  bxwidth


Return:
	>0	number of chars read and stored to txt[].
	0	get end of file.

	<0	fail
---------------------------------------------------------------*/
int egi_txtbox_readfile(EGI_EBOX *ebox, char *path)
{
	FILE *fil;
	int i;
	char buf[32]={0};
	int nread=0;
	int ret=0;
	EGI_DATA_TXT *data_txt=(EGI_DATA_TXT *)(ebox->egi_data);
	int bxwidth=ebox->width; /* in pixel, ebox width for txt  */
	int bxheight=ebox->height;
	char **txt=data_txt->txt;
	int nt=0;/* index, txt[][nt] */
	int nl=data_txt->nl; /* from 0, number of txt line */
	int nlw=0; /* current written line of txt */
	int llen=data_txt->llen -1; /*in bytes(chars), length for each line, one byte for /0 */
	int ncount=0; /*in pixel, counter for used pixels per line, MAX=bxwidth.*/
	int *symwidth=data_txt->font->symwidth;/* width list for each char code */
	int symheight=data_txt->font->symheight;
	int maxnum=data_txt->font->maxnum;
	long foff=data_txt->foff; /* current file position */

	/* WOWOO,NOPE!  bxheight already enlarged to fit for nl in egi_txtbox_activate()...
	 reset llen here, to consider ebox height
	 nl = nl < (bxheight/symheight) ? nl : (bxheight/symheight);  */

	/* check ebox data here */
	if( txt==NULL )
	{
		printf("egi_txtbox_readfile(): data_txt->txt is NULL!\n");
		return -1;
	}

	/* reset txt buf */
	for(i=0;i<nl;i++)
		memset(txt[i],0,data_txt->llen); /* dont use llen, here llen=data_txt->llen-1 */

	/* open txt file */
	fil=fopen(path,"rb");
	if(fil==NULL)
	{
		perror("egi_txtbox_readfile()");
		return -2;
	}
	printf("egi_txtbox_readfile(): succeed to open %s, current offset=%ld \n",path,foff);

	/* restore file position */
	fseek(fil,foff,SEEK_SET);

#if 0   /* WARNING:  feof() tests the end-of-file indicator for the stream pointed  to  by  stream,  returning
       		nonzero if it is set. !!!!!! and only AFTER fread() operation can you get real stream buffer,
		then you can use feof().
	*/
	if(feof(fil))
	{
		printf("egi_txtbox_readfile(): already reaches the end of the file '%s', offset=%ld, \
			roll back now!\n", path,ftell(fil) );
		rewind(fil); /* rewind the file offset */
		//return 0;
	}
#endif

	while( !feof(fil) )
	{
		memset(buf,0,sizeof(buf));/* clear buf */

		nread=fread(buf,1,sizeof(buf),fil);
		//egi_pdebug(DBG_TXT,"nread=%d\n",nread);
		if(nread <= 0) /* error or end of file */
			break;

//		ret+=nread;
		printf("-------- finish fread():  nread=fread()=%d ------- \n",nread);

		/* here put char to egi_data_txt->txt */
		for(i=0;i<nread;i++)
		{
			//egi_pdebug(DBG_TXT,"buf[%d]='%c' ascii=%d\n",i,buf[i],buf[i]);
			printf("i=%d of nread=%d\n",i,nread);
			/*  ------ 1. if it's a return code */
			/* TODO: substitue buf[i] with space ..... */
			if( buf[i]==10 )
			{
				//egi_pdebug(DBG_TXT," ------get a return \n");
				/* if return code is the first char of a line, ignore it then. */
			/*	if(nt != 0)
					nlw +=1; //new line
			*/
				nlw += 1; /* return to next line anyway */
				nt=0;ncount=0; /*reset one line char counter and pixel counter*/
#if 0 /* move to the last block of for() loop */
				/* MAX. number of lines = nl */
				if(nlw>nl-1) /* no more line for txt ebox */
					break;//return ret; /* abort the job */
				continue;
#endif
			}

			/* ----- 2. if symbol code out of range */
			else if( (uint8_t)buf[i] > maxnum )
			{
				printf("egi_txtbox_readfile():symbol/font/assic code number out of range.\n");
				continue;
			}

			/* ----- 3. check available pixel space for current line
			   Max. pixel number per line = bxwidth 	*/
			else if( symwidth[ (uint8_t)buf[i] ] > bxwidth-ncount )
			{
				nlw +=1; /* new line */
				nt=0;ncount=0; /*reset line char counter and pixel counter*/
				/*else, retry then with the new empty line */
				i--;
#if 0 /* move to the last block of for() loop */
				/* MAX. number of lines = nl */
				if(nlw>nl-1) /* no more line for txt ebox */
					break;//return ret; /* abort the job */
				continue;
#endif
			}

			/* ----- 4. OK, now push a char to txt[][] */
			else
			{
				ncount+=symwidth[ (uint8_t)buf[i] ]; /*increase total bumber of pixels for current txt line*/
				//egi_pdebug(DBG_TXT,"one line pixel counter: ncount=%d\n",ncount);
				txt[nlw][nt]=buf[i];
				nt++;

				/* ----- 5. check remained space
				check Max. char number per line =llen
					*/
				if( nt > llen-1 ) /* txt buf end */
				{
					nlw +=1; /* new line */
					nt=0;ncount=0; /*reset one line char counter and pixel counter*/
				}

			}

			/* -----  5. check number of lines used  ------- */
			if(nlw>nl-1) /* no more line for txt ebox */
			{
				//ret+=i+1; /* count total chars, plus pushed in this for() */
				//printf("last push txt i=%d of one read nread=%d, total ret=%d \n",i,nread,ret);
				break; /* end for() session */
			}
			/* if else, go on for() to push next char */



		}/* END for() */

		/* check if txt line is used up, end this file-read session */
		if(nlw>nl-1)
		{
			ret+=i+1;
			break; /* end while() of fread */
		}
		else
		/* or if just used up */
		{
			ret+=nread;
		}

	} /* END while() of fread */


	/* DEBUG, print out all txt in txt data buf */
#if 1
	for(i=0;i<nl;i++)
		printf("%s\n",txt[i]);
#endif

	/* save current file position */
	if(feof(fil))
	{
		((EGI_DATA_TXT *)(ebox->egi_data))->foff=0; /* reset offset,if end of file */
		ret=0; /* end of file */
	}
	else
	{
		//egi_pdebug(DBG_TXT,"ftell(fil)=%ld\n",ftell(fil));
		((EGI_DATA_TXT *)(ebox->egi_data))->foff +=ret; //ftell(fil);
	}

	fclose(fil);

	/* set need_refresh */
	ebox->need_refresh=true;

	return ret;
}


/*--------------------------------------------------
	release EGI_DATA_TXT
---------------------------------------------------*/
void egi_free_data_txt(EGI_DATA_TXT *data_txt)
{
	if(data_txt == NULL)
		return;

	int i;
	int nl=data_txt->nl;

	if( data_txt->txt != NULL)
	{
		for(i=0;i<nl;i++)
		{
			if(data_txt->txt[i] != NULL)
			{
				free(data_txt->txt[i]);
				data_txt->txt[i]=NULL;
			}
		}
		free(data_txt->txt);
		data_txt->txt=NULL;

		free(data_txt);
		data_txt=NULL;
	}

}


/*--------------------------------------------------
txtbox decorationg function

---------------------------------------------------*/
int egi_txtbox_decorate(EGI_EBOX *ebox)
{
	int ret=0;

	egi_pdebug(DBG_TXT,"egi_txtbox_decorate(): start to show_jpg()...\n");
	ret=show_jpg("/tmp/openwrt.jpg", &gv_fb_dev, SHOW_BLACK_NOTRANSP, ebox->x0+2, ebox->y0+2); /* blackoff<0, show black */

	return ret;
}



/*------------------------------------------------
 set string for first line of data_txt->txt[0]
-------------------------------------------------*/
void egi_txtbox_settitle(EGI_EBOX *ebox, char *title)
{
        /* 1. check data */
        if( ebox==NULL || ebox->type != type_txt )
        {
                printf("egi_txtbox_settitle(): ebox=NULL or not a txt type ebox! fail to set title!\n");
                return;
        }

	EGI_DATA_TXT *data_txt=(EGI_DATA_TXT *)(ebox->egi_data);

	if( data_txt == NULL || data_txt->txt[0]==NULL)
        {
                printf("egi_txtbox_settitle(): data_txt=NULL or data_txt->txt[0]=NULL! fail to set title!\n");
                return;
        }

	/* 2. clear data */
	int llen=data_txt->llen;
	memset(data_txt->txt[0],0,llen);

	/* 3. set title string */
	strncpy(data_txt->txt[0],title,llen-1);

	/* 4. set need_refresh */
	ebox->need_refresh=true;
}