/*
 * YAPP: Yet Another Plotting Package.
 *
 *	This implements a simple ascii meta-file; can be edited
 *	Note that the units are in cm, and only accurate to 0.1mm
 *	Sometimes useful for debugging.
 */

#include <stdinc.h>

extern string yapp_string;

local stream yappstr = NULL;
local string yapp_dev;

plinit(string pltdev, real xmin, real xmax, real ymin, real ymax)
{
    yappstr = stropen(yapp_string,"w");
    fprintf(yappstr,"plinit %5.2f %5.2f %5.2f %5.2f %s\n",
            xmin,xmax,ymin,ymax,yapp_string);
}

plswap() 
{
    fprintf(yappstr,"plswap\n");
}

real plxscale(real x, real y)
{
    fprintf(yappstr,"plxscale %5.2f %5.2f\n",x,y);
}

real plyscale(real x, real y)
{
    fprintf(yappstr,"plyscale %5.2f %5.2f\n",x,y);
}

plltype(int lwid, int lpat)
{
    fprintf(yappstr,"plltype %d %d\n",lwid,lpat);
}

plline(real x, real y)
{
    fprintf(yappstr,"plline %5.2f %5.2f\n",x,y);
}

plmove(real x, real y)
{
    fprintf(yappstr,"pldraw %5.2f %5.2f\n",x,y);
}

plpoint(real x, real y)
{
    fprintf(yappstr,"plpoint %5.2f %5.2f\n",x,y);
}

plcircle(real x, real y, real r)
{
    fprintf(yappstr,"plcircle %5.2f %5.2f %5.2f\n",x,y,r);    
}

plcross(real x, real y, real s)
{
    fprintf(yappstr,"plcross %5.2f %5.2f %5.2f\n",x,y,s);
}

plbox(real x, real y, real s)
{
    fprintf(yappstr,"plbox %5.2f %5.2f %5.2f\n",x,y,s);
}

pljust(int jus)
{
    fprintf(yappstr,"pljust %d\n",jus);
}

pltext(string msg, real x, real y, real hgt, real ang)
{
    fprintf(yappstr,"pltext %5.2f %5.2f %5.2f %5.2f %s\n",x,y,hgt,ang,msg);
}

plflush() 
{
    fprintf(yappstr,"plflush\n");
}

plframe()
{
    fprintf(yappstr,"plframe\n");
}

plstop()
{
    fprintf(yappstr,"plstop\n");
    strclose(yappstr);
    yappstr=NULL;
}

pl_matrix(frame,nx,ny,xmin,ymin,cell,fmin,fmax,findex)
real *frame, xmin, ymin, cell, fmin, fmax, findex;
int nx, ny;
{
    fprintf(yappstr,"pl_matrix\n");
}

pl_screendump(string fname)
{
    fprintf(yappstr,"pl_screendump\n");
}

pl_getpoly(float *x, float *y, int n)
{
    fprintf(yappstr,"pl_getpoly\n");
    return 0;
}

pl_interp(string cmd)
{
    fprintf(yappstr,"pl_interp\n");
}
