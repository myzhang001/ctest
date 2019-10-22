/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Midas Zhou
-------------------------------------------------------------------*/
#include "egi_common.h"
#include "egi_FTsymbol.h"
#include "page_avenger.h"
#include "avg_mvobj.h"

#define AVG_FIXED_POINT

/* --------------------------------------------------------------
		Create a new mvobj

@icons:		icon collection
@icon_index:    icon index for the mvobj, as of above icon collection.
@pxy:		initial coordinate
@heading:	heading angle in degree
@speed:		speed in pixel per refresh
@trail_mode:	function to parse the trail

Return:
	Pointer to an AVG_MVOBJ		OK
	NULL				Fails
-----------------------------------------------------------------*/
AVG_MVOBJ* avg_create_mvobj(    EGI_IMGBUF *icons,  int icon_index,
				EGI_POINT pxy, int heading, int speed,
				int (*trail_mode)(AVG_MVOBJ *)
			    )
{
	AVG_MVOBJ   *mvobj=NULL;
	EGI_IMGBUF  *refimg=NULL;
	EGI_IMGBUF  *actimg=NULL;

	/* Check input data */
	if( icons==NULL || icons->subimgs==NULL || icon_index<0 || icon_index > icons->submax ) {
		printf("%s:Input icons is NULL or icon_index invalid!\n",__func__);
		return NULL;
	}

	/* Copy icons->subimg[] to refimg */
	refimg=egi_imgbuf_subImgCopy( icons, icon_index );
	if(refimg==NULL)
		return NULL;

	/* Create actimg according to heading */
	actimg=egi_imgbuf_rotate(refimg, heading);
	if(actimg==NULL) {
		egi_imgbuf_free(refimg);
		return NULL;
	}

	/* Calloc AVG_MVOBJ */
	mvobj=calloc(1, sizeof(AVG_MVOBJ));
	if(mvobj==NULL) {
		printf("%s:Fail to call calloc() !\n",__func__);
		egi_imgbuf_free(refimg);
		egi_imgbuf_free(actimg);
		return NULL;
	}

	/* Assign methods and actions, May re_assign later... */
	mvobj->trail_mode=trail_mode;
	mvobj->hit_effect=avg_effect_exploding;

	/* Assign other memebers */
	mvobj->icons=icons;
	mvobj->icon_index=icon_index;
	mvobj->refimg=refimg;
	mvobj->actimg=actimg;
	mvobj->pxy=pxy;		/* Position: Integer type */

	mvobj->fpx=pxy.x;	/* Position: Float type */
	mvobj->fpy=pxy.y;

	mvobj->fvpx=MAT_FVAL(mvobj->pxy.x); /* Position: Fixed point value */
	mvobj->fvpy=MAT_FVAL(mvobj->pxy.y);

	mvobj->heading=heading;
	mvobj->speed=speed;

	mvobj->vang=0; 		/* Default angular speed is 0 */

	return mvobj;
}



/*----------------------------------------------
Reset mvobj speed to a random value.
-----------------------------------------------*/
inline int avg_random_speed(void)
{
        return egi_random_max(5)+1;
}


/*--------------------------------------
	Destroy a mvobj
---------------------------------------*/
void avg_destroy_mvobj(AVG_MVOBJ **mvobj)
{
	if(*mvobj==NULL)
		return;

	if( (*mvobj)->refimg != NULL) {
		egi_imgbuf_free((*mvobj)->refimg);
		(*mvobj)->refimg=NULL;
	}

	if( (*mvobj)->actimg != NULL) {
		egi_imgbuf_free((*mvobj)->actimg);
		(*mvobj)->actimg=NULL;
	}

	free(*mvobj);
	*mvobj=NULL;
}


/* -------------------------------------
Special effects for an exploding mvobj.
---------------------------------------*/
int avg_effect_exploding(AVG_MVOBJ *mvobj)
{
	if(mvobj==NULL)
		return -1;

	/* !!! NOW !!! Effect image also in mvobj->icons */

	/* End of stage */
	if( mvobj->stage > mvobj->effect_stages-1 )
		return -1;

	/* display image */
     #ifdef AVG_FIXED_POINT  /* use mvobj->fvpx/fvpy */
	egi_subimg_writeFB( mvobj->icons, &gv_fb_dev, mvobj->effect_index + mvobj->stage, -1,
			    (mvobj->fvpx.num>>MATH_DIVEXP) - mvobj->actimg->width/2,
			    (mvobj->fvpy.num>>MATH_DIVEXP) - mvobj->actimg->height/2 );
     #else /* <<<<<  use mvobj->pxy */
	egi_subimg_writeFB( mvobj->icons, &gv_fb_dev, mvobj->effect_index + mvobj->stage,
			    -1, mvobj->pxy.x + mvobj->actimg->width/2, mvobj->pxy.y + mvobj->actimg->height/2);
     #endif


	/* increase stage */
	mvobj->stage++;

	return 0;
}


/*----------------------------------------------
		Renew a plane
Chane param and start trail from starting line.
---------------------------------------------*/
int avg_renew_plane(AVG_MVOBJ *plane)
{
	int endx,endy;

	if(plane==NULL)
		return -1;

	/* Start point,  from to of the screen, screen Y=0  -25  */
	plane->pxy.x=egi_random_max(240+50)-25;
	plane->pxy.y=0-25;

	/* End point,  to the bottom of the screen, screen Y=320 +25  */
	endx=egi_random_max(240+50)-25;
	endy=320+25;

	/* Position: Float type */
	plane->fpx=plane->pxy.x;
	plane->fpy=plane->pxy.y;

	/* Position: Fixed point type */
	plane->fvpx.num=(plane->pxy.x)<<MATH_DIVEXP;
	plane->fvpx.div=MATH_DIVEXP;
	plane->fvpy.num=(plane->pxy.y)<<MATH_DIVEXP;
	plane->fvpy.div=MATH_DIVEXP;

	printf(" plane->fvpx: %d;      plane->fvpy: %d \n",
			(int)(plane->fvpx.num>>MATH_DIVEXP), (int)(plane->fvpy.num>>MATH_DIVEXP) );

	/* Set speed */
	plane->speed=avg_random_speed();

	/* Set angular velocity */
//TO see below:	plane->vang=egi_random_max(5)-3;

	/* Reste effect params */
	plane->is_hit=false;
	plane->stage=0;


#if 1	/* Draw xxx course line */
	switch(egi_random_max(3))
	{
		case 1:
			fbset_color(WEGI_COLOR_WHITE); break;
		case 2:
			fbset_color(WEGI_COLOR_ORANGE); break;
		case 3:
			fbset_color(WEGI_COLOR_RED); break;
		default:
			fbset_color(WEGI_COLOR_WHITE); break;
	}
    //fb_filo_off(&gv_fb_dev);
	#ifdef AVG_FIXED_POINT  /* use plane->fvpx */
	draw_wline(&gv_fb_dev, (plane->fvpx.num)>>MATH_DIVEXP, (plane->fvpy.num)>>MATH_DIVEXP,
								endx, endy, 1+egi_random_max(5));
	#else  /* use plane->pxy instead */
	draw_wline(&gv_fb_dev, plane->pxy.x, plane->pxy.y, endx, endy, 1+egi_random_max(5));
	#endif
    //fb_filo_on(&gv_fb_dev);
#endif

	/* ---------- Set Heading Angle ------------
	 * screenY as heading 0 Degree line.
	 * Counter_Clockwise as positive direction.
	 * ----------------------------------------*/
#ifdef AVG_FIXED_POINT /*  use plane->fvpx/fvpy */
	plane->heading=atan( 1.0*(endx-(plane->fvpx.num>>MATH_DIVEXP))/(endy-(plane->fvpy.num>>MATH_DIVEXP)) )/MATH_PI*180.0;
	printf("---  Pxy=(%d, %d),  Endxy=(%d, %d),  speed=%d  Heading=%d Deg. ---\n",
			(int)(plane->fvpx.num>>MATH_DIVEXP), (int)(plane->fvpy.num>>MATH_DIVEXP), \
			endx, endy, 	plane->speed, plane->heading );
#else 	/* use plane->pxy or instead */
	plane->heading=atan( 1.0*(endx-plane->pxy.x)/(endy-plane->pxy.y) )/MATH_PI*180.0;
	printf("---  Pxy=(%d, %d),  Endxy=(%d, %d),  speed=%d  Heading=%d Deg. ---\n",
			plane->pxy.x, plane->pxy.y, endx, endy, plane->speed, plane->heading);
#endif

	/* Set angular angle according to heading, anti_heading  */
	if( plane->heading>0)
		plane->vang=egi_random_max(2);
	else
		plane->vang=egi_random_max(2)-3;

	/* Change icon, randomly pick an incon in collection */
	egi_imgbuf_free(plane->refimg);
	plane->refimg=egi_imgbuf_subImgCopy( plane->icons, egi_random_max(16)-1 );

        /* Create actimg according to heading */
	egi_imgbuf_free(plane->actimg); plane->actimg=NULL;
       	plane->actimg=egi_imgbuf_rotate(plane->refimg, -plane->heading); /* Here clockwise is positive */


	/* NOTE: Assume that other params need NOT re_assign */

	return 0;
}




/*---------------------------------------------------------
		Renew a bullet
Chane param and start trail from a gun station.

@bullet:	A bullet which is out of sight or exploded.

Return:
	0	OK
	<0	Fails
----------------------------------------------------------*/
int avg_renew_bullet(AVG_MVOBJ *bullet)
{
	if(bullet==NULL || bullet->station==NULL) {
		printf("%s: Input param invalid!\n", __func__);
		return -1;
	}

	/* A gun station to fire the bullet. */
	AVG_MVOBJ *station=bullet->station;

	/* Position: Integer */
	bullet->pxy=station->pxy;

	/* Position: Float type */
	bullet->fpx=station->fpx;
	bullet->fpy=station->fpy;

        /* Position: Fixed type, assing fixed type pos from gun to bullet */
        bullet->fvpx.num=station->fvpx.num;
        bullet->fvpx.div=station->fvpx.div;
        bullet->fvpy.num=station->fvpy.num;
        bullet->fvpy.div=station->fvpy.div;

	/* Set bullet speed */
	bullet->vang=0;
	bullet->speed=-15; /* direction  SCREEN -Y */

	/* Set heading */
	bullet->heading=station->heading;

	/* Reste effect params */
	bullet->is_hit=false;
	bullet->stage=0;

	/* NOTE: Assume that other params need NOT re_assign */

	return 0;
}




/*-----------------------------------------------------------
		A heading line trail

1. A slow mvobj may fly ahead while slipping aside, because
   of pxy+= ... incremental precision, as pxy is an integer.
   Use float type otherwise to improve the precision.

------------------------------------------------------------*/
int fly_trail(AVG_MVOBJ *mvobj)
{
	int ang;	/* heading angle [0 360] */
	int asign;	/* -1 if ang<0,  1 otherwise */

	if(mvobj==NULL)
		return -1;

	/* 1. update heading */
	mvobj->heading += mvobj->vang;	/* degree per refresh */

	/* 2. Check heading and adjust it:
	 * 30 deg. is accumulated by angular speed. it's not default heading angle.
	 */
	if( mvobj->heading > 30 ||  (mvobj->fvpx.num>>MATH_DIVEXP) > 240  ) {
		mvobj->vang=egi_random_max(3)-4;
		mvobj->speed=avg_random_speed();
	}
	else if( mvobj->heading < -30 || (mvobj->fvpx.num>>MATH_DIVEXP) < 0 ) {
		mvobj->vang=egi_random_max(3);
		mvobj->speed=avg_random_speed();
	}

        /* 3. Normalize heading angle to be within [0-360] */
        ang=(mvobj->heading)%360;      /* !!! WARING !!!  The result is depended on the Compiler */
        asign= ang >= 0 ? 1 : -1; /* angle sign for sin */
        ang= ang>=0 ? ang : -ang ;

	/* 4. Update mvobj position */
#ifdef AVG_FIXED_POINT	/* 1.3 Fixed point for sin()/cons() AND mvobj->fvxy */
	mvobj->fvpx.num += mvobj->speed*asign*fp16_sin[ang]>>(16-MATH_DIVEXP);
	mvobj->fvpy.num += mvobj->speed*fp16_cos[ang]>>(16-MATH_DIVEXP);

#else 	/*  Float type for mvobj->fpx AND sin()/cos() */
	mvobj->fpx += mvobj->speed*sin(mvobj->heading/180.0*MATH_PI);
	mvobj->pxy.x = mvobj->fpx;

	mvobj->fpy -= mvobj->speed*cos(mvobj->heading/180.0*MATH_PI);
	mvobj->pxy.y = mvobj->fpy;
#endif

	/* 5. Check if it's out of visible region, then renew it to starting position.
	 *    active zone { (-55,-55), (295,375) }
	 */
#ifdef AVG_FIXED_POINT  /* use mvobj->fvpx/fvpy */
	if( (mvobj->fvpx.num>>MATH_DIVEXP) < 0-100 ||  (mvobj->fvpx.num>>MATH_DIVEXP) > 240+100
			|| (mvobj->fvpy.num>>MATH_DIVEXP) < 0-55 || (mvobj->fvpy.num>>MATH_DIVEXP) > 320 + 55 )
	{
		avg_renew_plane(mvobj);

        }
#else  /* use mvobj->pxy instead */
	if( mvobj->pxy.x < 0-55 || mvobj->pxy.x > 240+55
				|| mvobj->pxy.y < 0-55 || mvobj->pxy.y > 320 + 55 )
	{
		avg_renew_plane(mvobj);

        }
#endif

        /* 6. Update actimg, Create actimg according to heading */
	egi_imgbuf_free(mvobj->actimg); mvobj->actimg=NULL;
       	mvobj->actimg=egi_imgbuf_rotate(mvobj->refimg, -mvobj->heading); /* Here clockwise is positive */

	return 0;
}


/*-----------------------------------------------------------
		A trail for bullet

1. A slow mvobj may fly ahead while slipping aside, because
   of pxy+= ... incremental precision, as pxy is an integer.
   Use float type otherwise to improve the precision.

------------------------------------------------------------*/
int bullet_trail(AVG_MVOBJ *mvobj)
{
	int ang;	/* heading angle [0 360] */
	int asign;	/* -1 if ang<0,  1 otherwise */

	if(mvobj==NULL)
		return -1;

	/* 1. update heading */
	mvobj->heading += mvobj->vang;	/* degree per refresh */

        /* 2. Normalize heading angle to be within [0-360] */
        ang=(mvobj->heading)%360;      /* !!! WARING !!!  The result is depended on the Compiler */
        asign= ang >= 0 ? 1 : -1; /* angle sign for sin */
        ang= ang>=0 ? ang : -ang ;

	/* 3. Update mvobj position */
#ifdef AVG_FIXED_POINT	/* 1.3 Fixed point for sin()/cons() AND mvobj->fvxy */
	mvobj->fvpx.num += mvobj->speed*asign*fp16_sin[ang]>>(16-MATH_DIVEXP);
	mvobj->fvpy.num += mvobj->speed*fp16_cos[ang]>>(16-MATH_DIVEXP);

#else 	/*  Float type for mvobj->fpx AND sin()/cos() */
	mvobj->fpx += mvobj->speed*sin(mvobj->heading/180.0*MATH_PI);
	mvobj->pxy.x = mvobj->fpx;

	mvobj->fpy -= mvobj->speed*cos(mvobj->heading/180.0*MATH_PI);
	mvobj->pxy.y = mvobj->fpy;
#endif

	/* 4. Check if it's out of visible region, then renew it to starting position.
	 *    active zone { (-55,-55), (295,375) }
	 */
#ifdef AVG_FIXED_POINT  /* use mvobj->fvpx/fvpy */
	if( (mvobj->fvpx.num>>MATH_DIVEXP) < 0-100 ||  (mvobj->fvpx.num>>MATH_DIVEXP) > 240+100
			|| (mvobj->fvpy.num>>MATH_DIVEXP) < 0-55 || (mvobj->fvpy.num>>MATH_DIVEXP) > 320 + 55 )
	{
		avg_renew_bullet(mvobj);

        }
#else  /* use mvobj->pxy instead */
	if( mvobj->pxy.x < 0-55 || mvobj->pxy.x > 240+55
				|| mvobj->pxy.y < 0-55 || mvobj->pxy.y > 320 + 55 )
	{
		//avg_renew_bullet(mvobj, NULL);

        }
#endif

        /* 5. Update actimg, Create actimg according to heading */
	egi_imgbuf_free(mvobj->actimg); mvobj->actimg=NULL;
       	mvobj->actimg=egi_imgbuf_rotate(mvobj->refimg, -mvobj->heading); /* Here clockwise is positive */

	return 0;
}



/*----------------------------------------
	Turn around trail
-----------------------------------------*/
int turn_trail(AVG_MVOBJ *mvobj)
{
	int ang;	/* heading angle [0 360] */

	if(mvobj==NULL)
		return -1;

	/* update heading */
	mvobj->heading += mvobj->vang;	/* degree per refresh */

	/* Check heading and adjust it:
	 * 30 deg. is accumulated by angular speed. it's not default heading angle.
	 */
	if( mvobj->heading > 60  ) {
		mvobj->vang=egi_random_max(3)-4;
	}
	else if( mvobj->heading < -60 ) {
		mvobj->vang=egi_random_max(3);
	}

        /* 1. Rotate the object:  update actimg, Create actimg according to heading */
	egi_imgbuf_free(mvobj->actimg); mvobj->actimg=NULL;
       	mvobj->actimg=egi_imgbuf_rotate(mvobj->refimg, -mvobj->heading); /* Here clockwise is positive */

	return 0;
}


/*----------------------------------------------
	Refresh a mvobj image on screen.
----------------------------------------------*/
int refresh_mvobj(AVG_MVOBJ *mvobj)
{
	if(mvobj==NULL || mvobj->icons==NULL)
		return -1;

	/* If hit, show special effect */
	if(mvobj->is_hit) {
		/* Show hit effect stage by stage */
		if(mvobj->hit_effect != NULL)
			mvobj->hit_effect(mvobj);

		/* End of stages, renew the mvobj then */
		if( mvobj->stage > mvobj->effect_stages-1 ) {
			/* TODO: renew method for defferent mvobjs */
			//avg_renew_mvobj(mvobj);
			avg_renew_plane(mvobj);
		}

		return 0;
	}

	/* Else, update trail */
	if(mvobj->trail_mode != NULL)
		mvobj->trail_mode(mvobj);

	/* Refresh active image one the trail */

#ifdef AVG_FIXED_POINT /* Use mvobj->fvpx/fvpy */
        egi_imgbuf_windisplay( 	mvobj->actimg, &gv_fb_dev, -1,           /* img, fb, subcolor */
       	                       	0, 0,					 /* xp, yp */
				(mvobj->fvpx.num>>MATH_DIVEXP) - mvobj->actimg->width/2,   /* xw */
				(mvobj->fvpy.num>>MATH_DIVEXP) - mvobj->actimg->height/2,  /* yw */
                      		mvobj->actimg->width, mvobj->actimg->height );   /* winw, winh */
#else /* Use mvobj->pxy */
        egi_imgbuf_windisplay( 	mvobj->actimg, &gv_fb_dev, -1,           /* img, fb, subcolor */
       	                       	0, 0,					 /* xp, yp */
				mvobj->pxy.x - mvobj->actimg->width/2,		 /* xw */
				mvobj->pxy.y - mvobj->actimg->height/2,          /* yw */
                      		mvobj->actimg->width, mvobj->actimg->height );   /* winw, winh */
#endif

	return 0;
}


/*--------------------------
	Game README
--------------------------*/
void game_readme(void)
{
	const wchar_t *title=L"AVENGER V1.0";

	/* Following will produce 4 RETURE codes */
	const wchar_t *readme=L"    复仇之箭 made by EGI\n \
 运行平台 WIDORA_NEO \n \
   更猛烈的一波攻击来袭! \n \n \
	            READY?!!";

        /*  title  */
        FTsymbol_unicstrings_writeFB(&gv_fb_dev, egi_appfonts.bold,     /* FBdev, fontface */
                                          24, 24, title,                /* fw,fh, pstr */
                                          240, 1,  6,                   /* pixpl, lines, gap */
                                          30, 60,                      /* x0,y0, */
                                          WEGI_COLOR_GRAYC, -1, -1 );     /* fontcolor, stranscolor,opaque */

	/* README */
        FTsymbol_unicstrings_writeFB(&gv_fb_dev, egi_appfonts.bold,   /* FBdev, fontface */
                                          18, 18, readme,                /* fw,fh, pstr */
                                          240, 6,  7,                    /* pixpl, lines, gap */
                                          0, 100,                        /* x0,y0, */
                                          WEGI_COLOR_GRAYC, -1, -1 );      /* fontcolor, stranscolor,opaque */

}


