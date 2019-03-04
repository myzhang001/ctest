#include <stdio.h>
#include <stdint.h>
#include "egi_fbgeom.h"
#include "egi_image.h"
#include "egi_bmpjpg.h"


extern char _binary_buttons_img_start[];

int main(void)
{
   EGI_IMGBUF imgbuf;

   /* --- prepare fb device --- */
   gv_fb_dev.fdfd=-1;
   init_dev(&gv_fb_dev);



  imgbuf.width=240;
  imgbuf.height=320;


  imgbuf.imgbuf=(uint16_t *)_binary_buttons_img_start;

/*
int egi_imgbuf_windisplay(const EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int xp, int yp,
                                int xw, int yw, int winw, int winh)
*/

  egi_imgbuf_windisplay( &imgbuf, &gv_fb_dev, 0, 0, 0, 0, 240, 320 );

   /* close fb dev */
   munmap(gv_fb_dev.map_fb,gv_fb_dev.screensize);
   close(gv_fb_dev.fdfd);

}
