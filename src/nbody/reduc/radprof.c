/*
 *  RADPROF:  radial profile of an N-body system
 *      output is either a crummy yapp plot, or a table
 *
 *	 8-Apr-87	V1.0 created      		PJT
 *	21-apr-87	V1.1 rotation curve added	PJT
 *	 6-May-87	V1.2 subtle headline bug    	PJT
 *	26-May		V1.3 if no masses, m=1/NOBJ	PJT
 *	25-Mar-88	V1.4 device removed, see yapp   PJT
 *	31-Mar-88	V1.4a read_hist() added		PJT
 *	 1-Jun-88	V2.0 new filestruct, Qtab added	PJT
 *	18-oct-88	V2.0a output bug removed	PJT
 *	20-mar-89	    c      ""
 *	 2-apr-89	V2.1 rough density is now correct  PJT
 *	12-feb-92	    a drange->nemoinp + helpvec	   PJT
 *	22-apr-92	    b usage line
 *	 8-mar-92           c -DMOBJ can be used through Make
 *	16-feb-97           d support for SINGLEPREC	   pjt
 *	23-jul-97	V3.0 Added k= for # neighbors	   pjt
 *      28-jul-97           a check if first particle at 0,0,0  pjt
 */

#include <stdinc.h>
#include <getparam.h>
#include <vectmath.h>
#include <filestruct.h>
#include <snapshot/snapshot.h>
#include <yapp.h>
#include <axis.h>

string defv[] = {		/* DEFAULT INPUT PARAMETERS */
    "in=???\n			  ascii input file name ",
    "center=0,0,0\n               center of the system",
    "mode=density\n		  options: mass, density, rotcur ",
    "rmax=0.0\n			  def: automatic scaling ",
    "dmax=0.0\n			  def: automatic scaling ",
    "mmax=1.0\n			  def: fixed scaling 0->1 ",
    "vmax=0.0\n			  def: automatic scaling ",
    "kmax=1\n			  number of nearest 'radial' neighbors",
    "tab=f\n			  need a table ? ",
    "headline=\n                  random verbiage for plot",
    "VERSION=3.0b\n		  30-jul-97 PJT",
    NULL,
};

string usage = "radial profile of an N-body system";

string headline;		/* random text message */
string iname;			/* input file  name */

local real pos0[NDIM];		/* center */

#ifndef MOBJ
#define MOBJ 8192               /* maximum particles */
#endif

local int  nobj;			/* globals for writesnap */
local real mass[MOBJ];
local real phase[MOBJ][2*NDIM];
local real rad[MOBJ];
local int  irad[MOBJ];
local real dens[MOBJ];		/* in case of space problem */
local real vel[MOBJ];		/* these two can be computed on the fly */

local string mode;                    /* output plot mode */
local real rmax;			/* actual maxima on axes in plot_ routines */
local real dmax;                    /* density */
local real mmax;                    /* mass */
local real vmax;                    /* velocity */
local int  k;

local bool   Qtab;                    /* table output? */
local void   read_snap(string);

local real xtrans(real), ytrans(real);

nemo_main()
{
    setparams();

    read_snap(iname);
    sort_rad();
    if (!Qtab) {    
        plinit("***",0.0,20.0,0.0,20.0);
        if (strcmp(mode,"mass")==0)
            plot_cum_mass(); 
        else if (strcmp(mode,"density")==0)
            plot_density();  
        else if (strcmp(mode,"rotcur")==0 || strcmp(mode,"velocity")==0)
            plot_rotcur();
        else
           warning("not valid mode, try mass, density or rotcur");
        plstop();
    }
}


setparams()
{
    int n;
        
    iname = getparam("in");
    headline = getparam("headline");    
    mode = getparam("mode");

    n = nemoinpr(getparam("center"),pos0,NDIM);
    if (n!=NDIM)
       error("keyword center= must have exactly %d entries",NDIM);
    rmax = getdparam("rmax");
    dmax = getdparam("dmax");
    mmax = getdparam("mmax");
    vmax = getdparam("vmax");
    k = getiparam("kmax");
    Qtab = getbparam("tab");
}

local void read_snap(string name)		
{				
    stream instr;
    int    i;
    
    instr = stropen(name, "r");			/* open input file */

    get_history(instr);

    get_set(instr, SnapShotTag);
      get_set(instr, ParametersTag);
        get_data(instr, NobjTag, IntType, &nobj, 0);
	if (nobj>MOBJ)
		error("read_snap: not enough space to get data MOBJ=%d",MOBJ);
      get_tes(instr,ParametersTag);
      
      get_set(instr, ParticlesTag);
         if (get_tag_ok(instr,MassTag)) {
           get_data(instr, MassTag, RealType, mass, nobj, 0);
         }
         else {
	   dprintf (0,"WARNING: no masses provided, will assume 1/%d\n",nobj);
	   for (i=0; i<nobj; i++)
	      mass[i] = 1/(double)nobj;
	 }
         get_data(instr, PhaseSpaceTag, RealType, phase, nobj, 2, NDIM, 0);
      get_tes(instr,ParticlesTag);
    get_tes(instr,SnapShotTag);

    strclose(instr);
} /* read_snap */

/*	Some lower level plot utilities for YAPP 	*/

local real xplot[2],   yplot[2];		/* ranges in plot variables */
local char   xlabel[80], ylabel[80], plabel[80];	/* labels */

local real xtrans(real x)
{
	return (2.0 + 16.0*(x-xplot[0])/(xplot[1]-xplot[0]) );
}

local real ytrans(real y)
{
	return (2.0 + 16.0*(y-yplot[0])/(yplot[1]-yplot[0]) );
}




#define FUDGE  0.1

sort_rad()
{
    int i,j,l,u;
    real dr, radmax, cum_mass, densmax, velmax, mtot;
    real drmin, sum, radius;
        
    
    for (i=0, radmax=0.0; i<nobj; i++) {     /* build rad[] and find radmax */
        dr = 0.0;
        for (j=0; j<NDIM; j++)                             /* NDIM==3 ?? */
            dr += sqr(phase[i][j]-pos0[j]);
        rad[i] = sqrt(dr);
        if (rad[i]>radmax)
            radmax = rad[i];        /* maximum radius */
    }
    if (rmax<=0.0)                          /* if it was not set yet */
        rmax=radmax;                    /* set global maximum radius */
    if (!Qtab)
       dprintf (1,"maximum radius is %lf, will be set to %lf\n",radmax,rmax);
                                           /*   sort radii */
    sortptr (rad,irad,nobj);       /*   rad[irad[0..nobj-1]] is now sorted */
    
                /* first compute smallest projected interparticle distance */
    drmin = rad[irad[nobj-1]];              /* largest */
    for (i=1; i<nobj; i++) {
        dr = rad[irad[i]] - rad[irad[i-1]];
        if (dr<drmin)
            drmin=dr;
    }
    drmin *= FUDGE;                         /* softened surface density */
    dprintf (1,"fudge (%g) * drmin = %g\n",FUDGE,drmin);
    cum_mass=0.0;
    densmax=0.0;                            /* determine density + maximum */
    velmax=0.0;
    for (i=0; i<nobj; i++) {
        cum_mass += mass[irad[i]];
        if (i==0) {                         /* first particle */
            radius = rad[irad[0]];
            if (radius == 0.0) radius = rad[irad[1]];    /* fake it */
            if (radius == 0.0)              
                dens[0] = 0.0;                           /* can't fake this */
            else
                dens[0] = FRTHRD_PI * cum_mass / qbe(radius);
        } else if (i<nobj-1) {
#if 1
            l = MAX(i-k,0);
            u = MIN(i+k,nobj-1);

            for (j=l, mtot=0.0; j<=u; j++) mtot += mass[irad[j]];
            mtot -= 0.5 * (mass[irad[l]] + mass[irad[u]]);
            dens[i] = mtot/FRTHRD_PI/(qbe(rad[irad[u]])-qbe(rad[irad[l]]));

                
#else                
            dens[i]=(mass[irad[i]]+0.5*(mass[irad[i-1]]+mass[irad[i+1]]))/
                       (FOUR_PI * sqr(rad[irad[i]]) * 
                        (rad[irad[i+1]] - rad[irad[i-1]]));
#endif                        
        } else
            dens[i] = 0;        /* don't know anything better yet */

        vel[i] = sqrt(cum_mass/rad[irad[i]]);
        velmax=MAX(vel[i], velmax);
        densmax=MAX(dens[i], densmax);
        if (Qtab && i<nobj-1) {
            sum = 0.0;              /* add up to surface density */
            radius = rad[irad[i]] + drmin;  /* softened sur.den. */
            for (j=i+1; j<nobj; j++)    /* all stars on outside  */
                   sum += mass[irad[j]] / ( rad[irad[j]] * sqrt(
                      (rad[irad[j]]-radius)*(rad[irad[j]]+radius)));
            sum /= TWO_PI;          /* correct dimension */
            printf ("%g %g %g %g %g %g %g\n",
                     rad[irad[i]],dens[i],vel[i],cum_mass,sum,
                     pow(rad[irad[i]],0.25),
                     -2.5*log10(sum));
        }
    }
    if (dmax<=0.0) dmax=densmax;
    if (vmax<=0.0) vmax=velmax;
    if (!Qtab) {
       dprintf (1,"max density  %lf, reset to %lf\n",densmax,dmax);
       dprintf (1,"max velocity %lf, reset to %lf\n",velmax, vmax);
    }

} /* sort_rad */

/*	Some higher level YAPP interface routines */
plot_cum_mass()
{
	real cum_mass, radius;
	int    i;
	
	xplot[0]=0.0; xplot[1]=rmax; strcpy (xlabel,"radius");
	yplot[0]=0.0; yplot[1]=mmax;  strcpy (ylabel,"cumulative mass");
	xaxis (2.0, 2.0, 16.0, xplot, -7, xtrans, xlabel);
	xaxis (2.0,18.0, 16.0, xplot, -7, xtrans, NULL);
	yaxis (2.0, 2.0, 16.0, yplot, -7, ytrans, ylabel);
	yaxis (18.0,2.0, 16.0, yplot, -7, ytrans, NULL);

	sprintf (plabel,"File: %s",iname);			/* filename */
	pltext (plabel,2.0,18.4, 0.32, 0.0);
	if (*headline!=0) 				  /* identification */
	    pltext (headline,2.0,19.0,0.25,0.0);

	cum_mass = 0.0;
	radius = 0.0;
	plmove (xtrans(radius),ytrans(cum_mass));	
	for (i=0; i<nobj; i++) {
		radius = rad[irad[i]];
		plline (xtrans(radius),ytrans(cum_mass));
		cum_mass += mass[irad[i]];
		plline (xtrans(radius),ytrans(cum_mass));
		if (radius>rmax)
			break;
	}
}	

plot_rotcur()
{
	real velocity, radius, cum_mass;
	real rv, rlen, vlen, vrad;
	int    i, j;
	
	xplot[0]=0.0; xplot[1]=rmax; strcpy (xlabel,"radius");
	yplot[0]=0.0; yplot[1]=vmax; strcpy (ylabel,"velocity");
	xaxis (2.0, 2.0, 16.0, xplot, -7, xtrans, xlabel);
	xaxis (2.0,18.0, 16.0, xplot, -7, xtrans, NULL);
	yaxis (2.0, 2.0, 16.0, yplot, -7, ytrans, ylabel);
	yaxis (18.0,2.0, 16.0, yplot, -7, ytrans, NULL);

	sprintf (plabel,"File: %s",iname);			/* filename */
	pltext (plabel,2.0,18.4, 0.32, 0.0);
	if (*headline!=0) 					/* identification */
		pltext (headline,2.0,19.0,0.25,0.0);

	velocity = 0.0;
	radius = 0.0;
	cum_mass = 0.0;
	plmove (xtrans(radius),ytrans(velocity));	
	for (i=0; i<nobj; i++) {
		radius = rad[irad[i]];
		cum_mass += mass[irad[i]];
		velocity = sqrt(cum_mass/radius);
		plline (xtrans(radius),ytrans(velocity));
		if (radius>rmax)
			break;
	}
#if 1 
	for (i=0; i<nobj; i++) {
			/* DOTVP doesn't work here, manual... */
		rv=rlen=vlen=0.0;
		for (j=0; j<3; j++) {
			rv += phase[i][j]*phase[i][j+3];    /* dot product */
			rlen += sqr(phase[i][j]);	    /* vector length */
			vlen += sqr(phase[i][j+3]);
		}
		rlen = sqrt(rlen);
		vlen = sqrt(vlen);
		vrad = vlen * sqrt(1.0-sqr(rv/(rlen*vlen)));
		plpoint(xtrans(rlen),ytrans(vrad));
	}
#endif
}	
 

plot_density()
{
	real density, radius;
	int    i;

	xplot[0]=0.0; xplot[1]=rmax; strcpy (xlabel,"radius");
	yplot[0]=0.0; yplot[1]=dmax;  strcpy (ylabel,"density");
	xaxis (2.0, 2.0, 16.0, xplot, -7, xtrans, xlabel);
	xaxis (2.0,18.0, 16.0, xplot, -7, xtrans, NULL);
	yaxis (2.0, 2.0, 16.0, yplot, -7, ytrans, ylabel);
	yaxis (18.0,2.0, 16.0, yplot, -7, ytrans, NULL);

	sprintf (plabel,"File: %s",iname);			/* filename */
	pltext (plabel,2.0,18.4, 0.32, 0.0);
	if (headline)					  /* identification */
		pltext (headline,2.0,19.0,0.25,0.0);

	density = dens[0];
	radius = rad[irad[0]];
	plmove (xtrans(radius),ytrans(density));	
	for (i=1; i<nobj; i++) {
		density = dens[i];
		radius = rad[irad[i]];
		plline (xtrans(radius),ytrans(density));
		if (radius>rmax)
			break;
	}
}	

