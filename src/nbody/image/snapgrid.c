/* 
 *  SNAPGRID:  program grids a snapshot into a 3D image-cube
 *             can do amission as well as absorbtion
 *             generalization of snapccd
 *
 *	19-jan-89  V1.0 -- derived from snapccd	PJT
 *      12-mar-89  V1.1 -- allow expression for emmisivity	PJT
 *                         allow taking mean instead of adding up
 *		       a  + warning when emissivity = 0
 *	 2-nov-90  V2.0 allow stacked snapshots
 *      21-oct-91  V3.0 allow moment=-1 or -2 for fast vel/sig gridding
 *                      also think about smoothing
 *	12-jun-92  V3.1 allow selection by time
 *      18-jul-92  V3.2 fixed bug for moment<0 and stack=t
 *                      floor() in gridding functions
 *      30-jul-93  V4.0 write out multiple images from multiple evar's
 *                      and using the new Unit's member of Image's
 *      26-jan-94  adding the new tvar= variable for optical depth (tau)
 *	22-jan-95  V4.2 fixed up the mean=t code
 *                      local variables
 *      11-jul-96  V4.3 finite size particles (svar) or 'variable smoothing'
 *      23-nov-96      a   prototypes, now works on linux
 *	 4-mar-97      b   support for SINGLEPREC
 *      18-jun-98   4.4 added optional overridden {x,y,z}lab's
 *      30-jul-98      a  fixed gridding bug introduced in 4.4
 *
 * Todo: - mean=t may not be correct for nz>1 
 */

#include <stdinc.h>		/* nemo general */
#include <getparam.h>
#include <vectmath.h>
#include <filestruct.h>

#include <snapshot/body.h>      /* snapshot's */
#include <snapshot/snapshot.h>
#include <snapshot/get_snap.c>

#include <image.h>              /* images */

string defv[] = {		/* keywords/default values/help */
	"in=???\n			  input filename (a snapshot)",
	"out=???\n			  output filename (an image)",
	"times=all\n			  standard selection of times",
	"xrange=-2:2\n			  x-range in gridding",
	"yrange=-2:2\n	  	 	  y-range in gridding",
	"zrange=-infinity:infinity\n	  z-range in gridding",
	"xvar=x\n			  x-variable to grid",
	"yvar=y\n			  y-variable to grid",
	"zvar=-vz\n			  z-variable to grid",
        "evar=m\n                         emission variable(s)",
        "tvar=0\n                         absorbtion variable",
        "dvar=z\n                         depth variable w/ tvar=",
        "svar=\n                          smoothing size variable",
	"nx=64\n			  x size of image",
	"ny=64\n			  y size of image",
	"nz=1\n			  	  z size of image",
	"xlab=\n                          Optional X label [xvar]",
	"ylab=\n                          Optional Y label [yvar]",
	"zlab=\n                          Optional Z label [zvar]",
	"moment=0\n			  moment in zvar (-2,-1,0,1,2...)",
	"mean=f\n			  mean (moment=0) or sum per cell",
	"stack=f\n			  Stack all selected snapshots?",
	"VERSION=4.4b\n			  1-apr-01 PJT",
	NULL,
};

string usage="grid a snapshot into a 2/3D image-cube";

#ifndef HUGE
# define HUGE      1.0e20
#endif
#define TIMEFUZZ  0.000001
#define CUTOFF    4.0		/* cutoff of gaussian in terms of sigma */
#define MAXVAR	  16		/* max evar's */

local stream  instr, outstr;				/* file streams */

		/* SNAPSHOT INTERFACE */
local int    nobj, bits, nbody=0, noutxy=0, noutz=0, nzero=0;
local real   tnow;
local Body   *btab = NULL;
local string times;

		/* IMAGE INTERFACE */
local imageptr  iptr=NULL, iptr0=NULL, iptr1=NULL, iptr2=NULL;
local int    nx,ny,nz;			     /* map-size */
local real   xrange[3], yrange[3], zrange[3];      /* range of cube */
local real   xbeam, ybeam, zbeam;                  /* >0 if convolution beams */
local int    xedge, yedge, zedge;                  /* check if infinite edges */

local string xvar, yvar, zvar;  	/* expression for axes */
local string xlab, ylab, zlab;          /* labels for output */
local rproc  xfunc, yfunc, zfunc;	/* bodytrans expression evaluator for axes */
local string *evar;
local rproc  efunc[MAXVAR];
local int    nvar;			/* number of evar's present */
local string dvar, tvar, svar;
local rproc  dfunc, tfunc, sfunc;

local int    moment;	                /* moment to take in velocity */
local real   zsig;			/* positive if convolution in Z used */
local real   zmin, zmax;

local bool   Qmean;			/* adding or taking mean for image ?? */
local bool   Qstack;                  /* stacking snapshots ?? */
local bool   Qdepth;                  /* need dfunc/tfunc for depth integration */
local bool   Qsmooth;                 /* (variable) smoothing */

extern string  *burststring(string,string);
extern rproc   btrtrans(string);


local void setparams(void);
local void compfuncs(void);
local int read_snap(void);
local int allocate_image(void);
local int clear_image(void);
local int bin_data(int ivar);
local int free_snap(void);
local int rescale_data(int ivar);
local int xbox(real x);
local int ybox(real y);
local int zbox(real z);
local real odepth(real tau);
local int setaxis(string rexp, real range[3], int n, int *edge, real *beam);


nemo_main ()
{
    int i;
    
    setparams();                /* set from user interface */
    compfuncs();                /* get expression functions */
    allocate_image();		/* make space for image(s) */
    if (Qstack) clear_image();	/* clear the images */
    if (Qstack && Qdepth)
        warning("memory intensive mode  (stacking and absorbing)");
    while (read_snap())	{                   /* read next N-body snapshot */
        for (i=0; i<nvar; i++) {
            if (!Qstack) {
            	if (nvar>1) dprintf(0,"Gridding evar=%s\n",evar[i]);
		clear_image();
	    }
            bin_data(i);	            /* bin and accumulate */
            if (!Qstack) {                  /* if multiple images: */
                rescale_data(i);            /* rescale */
                write_image(outstr,iptr);   /* and write them out */
                if (i==0) reset_history();  /* clean history */
            }
        }
        free_snap();
    }
    if (Qstack) {
        rescale_data(0);                    /* and rescale before ... */
        write_image (outstr,iptr);	    /* write the image */
    }

    strclose(instr);
    strclose(outstr);
}

void setparams()
{
    char  *cp;

    times = getparam("times");
    Qmean = getbparam("mean");
    Qstack = getbparam("stack");

    nx = getiparam("nx");
    ny = getiparam("ny");
    nz = getiparam("nz");
    if (nx<1 || ny<1 || nz<1) error("Bad grid size (%d %d %d)",nx,ny,nz);

    setaxis(getparam("xrange"), xrange, nx, &xedge, &xbeam);
    if (xbeam > 0.0) error("No convolve in X allowed yet");

    setaxis(getparam("yrange"), yrange, ny, &yedge, &ybeam);
    if (ybeam > 0.0) error("No convolve in Y allowed yet");

    setaxis(getparam("zrange"), zrange, nz, &zedge, &zbeam);
    if (zbeam > 0.0) {                          /* with convolve */
        zsig = zbeam;                           /* beam */
        zmin = zrange[0] - CUTOFF*zsig;         /* make edges wider */
        zmax = zrange[1] + CUTOFF*zsig;
    } else {                                    /* straight gridding */
        zsig = 0.0;                             /* no beam */
        zmin = zrange[0];                       /* exact edges */
        zmax = zrange[1];
    }
    if (zedge && zsig>0.0)
        error("Cannot convolve in Z with an infinite edge");
    if (zedge && nz>1)
        error("Cannot use multiple Z planes with an infinite edge");

    zrange[2] = (zrange[1]-zrange[0])/nz;   /* reset grid spacing */
    dprintf (1,"size of IMAGE cube = %d * %d * %d\n",nx,ny,nz);

    xvar = getparam("xvar");
    yvar = getparam("yvar");
    zvar = getparam("zvar");
    evar = burststring(getparam("evar"),",");
    nvar = xstrlen(evar,sizeof(string)) - 1;
    dvar = getparam("dvar");
    tvar = getparam("tvar");
    Qsmooth = hasvalue("svar");
    if (Qsmooth) svar = getparam("svar");
    if (nvar < 1) error("Need evar=");
    if (nvar > MAXVAR) error("Too many evar's (%d > MAXVAR = %d)",nvar,MAXVAR);
    if (Qstack && nvar>1) error("stack=t with multiple (%d) evar=",nvar);
    xlab = hasvalue("xlab") ? getparam("xlab") : xvar;
    ylab = hasvalue("ylab") ? getparam("ylab") : yvar;
    zlab = hasvalue("zlab") ? getparam("zlab") : zvar;


    moment=getiparam("moment");
    if (moment<-2) error("Illegal moment=%d",moment);
    instr = stropen (getparam ("in"), "r");
    outstr = stropen (getparam("out"),"w");
}

void compfuncs()
{
    int i;
    
    xfunc = btrtrans(xvar);
    yfunc = btrtrans(yvar);
    zfunc = btrtrans(zvar);
    for (i=0; i<nvar; i++)
        efunc[i] = btrtrans(evar[i]);
    Qdepth = !streq(tvar,"0");
    if (Qdepth) {
        dfunc = btrtrans(dvar);
        tfunc = btrtrans(tvar);
    }
    if (Qsmooth)
        sfunc = btrtrans(svar);
}

int read_snap()
{		
    for(;;) {		
        get_history(instr);
        get_snap(instr,&btab,&nobj,&tnow,&bits);
        if (bits==0) 
            break;           /* no snapshot at all */
        if ( (bits&PhaseSpaceBit)==0 )
            continue;       /* skip, no data, probably diagnostics */
        if (streq(times,"all") || within(tnow, times, TIMEFUZZ))
            break;          /* take it, if time in timerange */
    }
    if (bits) {
    	if (Qstack)
	    dprintf(1,"Adding data for times=%g; bits=0x%x\n",tnow,bits);
	else
	    dprintf(1,"New data for times=%g; bits=0x%x\n",tnow,bits);
    }
    return bits;
}

#define CV(i)  (CubeValue(i,ix,iy,iz))

allocate_image()
{
    create_cube (&iptr,nx,ny,nz);
    if (iptr==NULL) error("No memory to allocate first image");

    if (Qmean) {
    	if (moment)
    	    warning("%d: mean=t requires moment=0",moment);
        create_cube(&iptr0,nx,ny,nz);
        if (iptr0==NULL) 
            error("No memory to allocate normalization image - try mean=f");
    }

    if (moment == -1 || moment == -2) {
        create_cube(&iptr1,nx,ny,nz);
        if (iptr1==NULL) 
            error("No memory to allocate second image - try moment>0");
    }

    if (moment == -2) {
        create_cube(&iptr2,nx,ny,nz);
        if (iptr2==NULL) 
            error("No memory to allocate third image - try moment>0");
    }

    Xmin(iptr) = xrange[0] + 0.5*xrange[2];
    Ymin(iptr) = yrange[0] + 0.5*yrange[2];
    Zmin(iptr) = zrange[0] + 0.5*zrange[2];

    Dx(iptr) = xrange[2];
    Dy(iptr) = yrange[2];
    Dz(iptr) = zrange[2];
	
    Namex(iptr) = xlab;
    Namey(iptr) = ylab;
    Namez(iptr) = zlab;

    Beamx(iptr) = 0.0;  /* we don't allow smooth in the image plane for now */
    Beamy(iptr) = 0.0;
    Beamz(iptr) = (zsig>0.0 ? zsig : 0.0);
}

clear_image()
{
    int ix,iy,iz;

    for (ix=0; ix<nx; ix++)		        /* initialize all cubes to 0 */
    for (iy=0; iy<ny; iy++)
    for (iz=0; iz<nz; iz++) {
    	CV(iptr) = 0.0;
        if(iptr0) CV(iptr0) = 0.0;        
        if(iptr1) CV(iptr1) = 0.0;        
        if(iptr2) CV(iptr2) = 0.0;
    }
}


typedef struct point {
    real em, ab, z, depth;          /* emit, absorb, moment var, depth var */
    struct point *next, *last;      /* pointers to aid */
} Point;

#define AddIndex(i,j,em,ab,z)  /**/

local Point **map =NULL;

local int pcomp(Point *a, Point *b);

bin_data(int ivar)
{
    real brightness, cell_factor, x, y, z, z0, t;
    real expfac, fac, sfac, flux, b, emtau, depth;
    real e, emax, twosqs;
    int    i, j, k, ix, iy, iz, n, nneg, ioff;
    int    ix0, iy0, ix1, iy1, m, mmax;
    Body   *bp;
    Point  *pp, *pf,*pl, **ptab;
    bool   done;
    
    if (Qdepth) {
        if (map==NULL)
            map = (Point **) allocate(Nx(iptr)*Ny(iptr)*sizeof(Point *));
        if (map==NULL || !Qstack) {
            for (iy=0; iy<Ny(iptr); iy++)
            for (ix=0; ix<Nx(iptr); ix++)
                map[ix+Nx(iptr)*iy] = NULL;
        }
    }
    expfac = (zsig > 0.0 ? 1.0/(sqrt(TWO_PI)*zsig) : 0.0);
    if (Qmean)
        cell_factor = 1.0;
    else
        cell_factor = 1.0 / ABS(Dx(iptr)*Dy(iptr));
    nbody += nobj;
    if (Qsmooth) 
        mmax = MAX(Nx(iptr),Ny(iptr));
    else
        mmax = 1;
    emax = 10.0;

		/* walk through all particles and accumulate ccd data */
    for (i=0, bp=btab; i<nobj; i++, bp++) {
        x = xfunc(bp,tnow,i);            /* transform */
	y = yfunc(bp,tnow,i);
        z = zfunc(bp,tnow,i);
        flux = efunc[ivar](bp,tnow,i);
        if (Qdepth) {
            emtau = odepth( tfunc(bp,tnow,i) );
            depth = dfunc(bp,tnow,i);
        }
        if (Qsmooth) {
            twosqs = sfunc(bp,tnow,i);
            twosqs = 2.0 * sqr(twosqs);
        }

	ix0 = xbox(x);                  /* direct gridding in X and Y */
	iy0 = ybox(y);


	if (ix0<0 || iy0<0) {           /* outside area (>= nx,ny never occurs */
	    noutxy++;
	    continue;
	}
        if (z<zmin || z>zmax) {         /* initial check in Z */
            noutz++;
            continue;
        }
        if (flux == 0.0) {              /* discard zero flux cases */
            nzero++;
            continue;
        }

      	dprintf(1,"%d @ (%d,%d) from (%g,%g)\n",
      	        i+1,ix0,iy0,x,y);


        for (m=0; m<mmax; m++) {        /* loop over smoothing area */
            done = TRUE;
            for (iy1=-m; iy1<=m; iy1++)
            for (ix1=-m; ix1<=m; ix1++) {       /* current smoothing edge */
                ix = ix0 + ix1;
                iy = iy0 + iy1;
        	if (ix<0 || iy<0 || ix >= Nx(iptr) || iy >= Ny(iptr))
        	    continue;

                if (m>0 && ABS(ix1) != m && ABS(iy1) != m) continue;
                if (m>0)
                    if (twosqs > 0)
                        e = (sqr(ix1*Dx(iptr))+sqr(iy1*Dy(iptr)))/twosqs;
                    else 
                        e = 2 * emax;
                else 
                    e = 0.0;
                if (e < emax) {
                    sfac = exp(-e);
                    done = FALSE;
                } else
                    sfac = 0.0;

                brightness =   sfac * flux * cell_factor;	/* normalize */
                b = brightness;
        	for (k=0; k<ABS(moment); k++) brightness *= z;  /* moments in Z */
                if (brightness == 0.0) continue;

                if (Qdepth) {		/* stack away relevant particle info */
                    pp = (Point *) allocate(sizeof(Point));
                    pp->em = brightness;
                    pp->ab = emtau;
                    pp->z  = z;
                    pp->next = NULL;
                    ioff = ix + Nx(iptr)*iy;
                    pf = map[ioff];
                    if (pf==NULL) {
                        map[ioff] = pp;
                        pp->last = pp;
                    } else {
                        pl = pf->last;
                        pl->next = pp;
                        pf->last = pp;
                    }

                    continue;       /* goto next accumulation now    CHECK */
                }


                if (zsig > 0.0) {           /* with Gaussian convolve in Z */
                    for (iz=0, z0=Zmin(iptr); iz<nz; iz++, z0 += Dz(iptr)) {
        	        fac = (z-z0)/zsig;
	        	if (ABS(fac)>CUTOFF)    /* if too far from gaussian center */
		            continue;           /* no contribution added */
        		fac = expfac*exp(-0.5*fac*fac);
                        CV(iptr) += brightness*fac;     /* moment */
                        if(iptr1) CV(iptr1) += b*fac;   /* moment -1,-2 */
                        if(iptr2) CV(iptr2) += b*z*fac; /* moment -2 */
        	    }
        	} else {
                    iz = zbox(z);
                    CV(iptr) +=   brightness;   /* moment */
                    if(iptr0) CV(iptr0) += 1.0; /* for mean */
                    if(iptr1) CV(iptr1) += b;   /* moment -1,-2 */
                    if(iptr2) CV(iptr2) += b*z; /* moment -2 */
                }

            } /* for (iy1/ix1) */
            if (done) break;
        } /* m */
    }  /*-- end particles loop --*/

    if (Qdepth) {
        /* accumulate */
        for (i=0; i<Nx(iptr)*Ny(iptr); i++) {
	    if (map[i]==NULL) continue;

	    n=1;                    /* count number along line of sight */
	    pp = map[i];
	    while (pp->next) {
	        n++;
	        pp = pp->next;
	    }

            ptab = (Point **) allocate (n * sizeof(Point *));
            
            n=0;
            pp = ptab[0] = map[i];
            while (pp->next) {
                ptab[n++] = pp->next;
                pp = pp->next;
            }
            qsort(ptab, n, sizeof(Point *), pcomp);
        }
        if (!Qstack) {          /* free the chain of visible particles */
            for (i=0; i<Nx(iptr)*Ny(iptr); i++) {
                pf = map[i];
                if (pf==NULL) continue;
                while(pf->next) {
                    pp = pf->next;
                    free(pf);
                    pf = pp;
                }
            }
        }
    }
}

int pcomp(Point *a, Point *b)
{
    return a->depth < b->depth;
}

free_snap()
{
    free(btab);         /* free snapshot */
    btab = NULL;        /* and make sure it can realloc at next get_snap() */
}    

rescale_data(int ivar)
{
    real m_min, m_max, brightness, total, x, y, z, b;
    int    i, j, k, ix, iy, iz, nneg, ndata;

    m_max = -HUGE;
    m_min = HUGE;
    total = 0.0;
    ndata = 0;

    /* Add the variable 4th and 5th dimension coordinates */
    Unit(iptr) = evar[ivar]; /* for Qmean=t should use proper mean cell units */
    Time(iptr) = tnow;
    
    /* handle special cases when mean=t or moment=-1 or moment=-2 */
    if (iptr2) {                    /* moment = -2 : dispersion output */
        nneg=0;
        for (ix=0; ix<nx; ix++)         /* loop over whole cube */
        for (iy=0; iy<ny; iy++)
        for (iz=0; iz<nz; iz++) {
            if(CV(iptr) == 0.0) continue;
            b = CV(iptr)/CV(iptr1)-sqr(CV(iptr2)/CV(iptr1));
            if(b<0.0) {
                nneg++;
                b=0.0;
            }
            CV(iptr) = sqrt(b);
        }
        if(nneg>0) warning("%d pixels with sig^2<0",nneg);
    } else if(iptr1) {
        for (ix=0; ix<nx; ix++)         /* loop over whole cube */
        for (iy=0; iy<ny; iy++)
        for (iz=0; iz<nz; iz++) {
            if(CV(iptr) == 0.0) continue;       /* no emission */
            CV(iptr) /= CV(iptr1);
        }
    } else if(iptr0) {
        for (ix=0; ix<nx; ix++)         /* loop over whole cube */
        for (iy=0; iy<ny; iy++)
        for (iz=0; iz<nz; iz++) {
            if(CV(iptr) == 0.0) continue;       /* no emission */
            CV(iptr) /= CV(iptr0);
        }
        
    }

    for (ix=0; ix<nx; ix++)         	/* determine maximum in picture */
    for (iy=0; iy<ny; iy++)
    for (iz=0; iz<nz; iz++) {
       	  brightness = CV(iptr);
	  total += brightness;
	  m_max =  MAX(m_max,brightness);
	  m_min =  MIN(m_min,brightness);
	  if (brightness!=0.0)
	  	ndata++;
    }
   
    MapMin(iptr) = m_min;               /* min and max of data */
    MapMax(iptr) = m_max;
    BeamType(iptr) = NONE;              /* no smoothing yet */

    dprintf (1,"Total %d particles within grid\n",nbody-noutxy-noutz);
    dprintf (1,"     (%d were outside XY range, %d were not in Z range)\n",
                    noutxy, noutz);
    dprintf (1,"%d cells contain non-zero data,  min and max in map are %f %f\n",
    		ndata, m_min, m_max);
    dprintf (1,"Total mass in map is %f\n",total*Dx(iptr)*Dy(iptr));
    if (nzero)
        warning("There were %d stars with zero emissivity in the grid",nzero);
}

/*	compute integerized coordinates in X Y and Z, return < 0 if
 *	outside the range's - handle Z a little differently, as it
 *	allows edges at infinity
 */

int xbox(real x)
{
    if (xrange[0] < xrange[1]) {
        if (xrange[0]<x && x<xrange[1])
            return (int)floor((x-xrange[0])/xrange[2]);
    } else if (xrange[1] < xrange[0]) {
        if (xrange[1]<x && x<xrange[0])
            return (int)floor((x-xrange[0])/xrange[2]);
    } 
    return -1;
}

int ybox(real y)
{
    if (yrange[0] < yrange[1]) {
        if (yrange[0]<y && y<yrange[1])
            return (int)floor((y-yrange[0])/yrange[2]);
    } else if (yrange[1] < yrange[0]) {
        if (yrange[1]<y && y<yrange[0])
            return (int)floor((y-yrange[0])/yrange[2]);
    } 
    return -1;
}

int zbox(real z)
{
    if (zedge==0x03)			/* Both edges at infinity */
        return 0;                       /* always inside */
    if (zedge==0x01 && z<zrange[1])	/* left edge at infinity */
        return 0;
    if (zedge==0x02 && z>zrange[0])	/* rigth side at infinity */
        return 0;
    if (zedge)
        return -1;                      /* remaining cases outside */
	
    return (int)floor((z-zrange[0])/zrange[2]);    /* simple gridding */
}

real odepth(real tau)
{
    return exp(-tau);
}

/*
 * parse an expression of the form beg:end[,sig] into
 * range[2]:   0 = beg
 *	       1 = end
 *	       2 = (end-beg)/n   if sig not present (end>beg)
 * beam:
 *	       sig          if sig (>0) present
 *             -1           if no beam
 * 
 */
setaxis (string rexp, real range[3], int n, int *edge, real *beam)
{
    char *cp;
    
    *edge = 0;                  /* no infinite edges yet */
    cp = strchr(rexp,':');
    if (cp==NULL)
        error("range=%s must be of form beg:end[,sig]",rexp);
    if (strncmp(rexp,"-inf",4)==0) {
        range[0] = -HUGE;
        *edge |= 0x01;              /* set left edge at inifinity */	
    } else
        range[0] = atof(rexp);
    cp++;
    if (strncmp(cp,"inf",3)==0) {
        range[1] = HUGE;
	*edge |= 0x02;              /* set right edge at infinity */
    } else
        range[1] = atof(cp);
    range[2] = (range[1]-range[0])/(real)n;       /* step */
    cp = strchr(cp,',');
    if (cp)
        *beam = atof(++cp);                  /* convolution beam */
    else
        *beam = -1.0;                        /* any number < 0 */
}
