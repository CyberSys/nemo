/*
 * rotcur.c: potential & forces as defined by a rotation curve
 *           This version can only compute one version; i.e.
 *           on re-entry of inipotential(), old versions are lost
 *
 *	nov-90	created?	PJT
 *    19-jul-92 finished for Bikram     PJT
 *	 oct-93 get_pattern
 *	15-apr-98  oops, made forces now negative
 */

/*CTEX
 *  {\bf potname=rotcur
 *       potpars={\it $\Omega$}
 *	 potfile={\it table(5NEMO)}}
 */
 

#include <stdinc.h>
#include <spline.h>
#include <table.h>

extern double sqr();
 
local double omega = 0.0;           /* just put to zero until implemented */
local double maxrad = -1.0;	    /* maximum radius --  ***not used*** */

local real *rad, *vel, *coef;
local int nrad, nmax;
local int entries=0;

void inipotential (npar, par, name)
int    *npar;
double par[];
char *name;
{
    int i, n, colnr[2];
    real *coldat[2];
    stream instr;

    n = *npar;
    if (n>0) omega = par[0];
    if (n>1) warning("Rotcur potential: only 1 parameter usable");
    
    if (entries>0) {
        warning("Re-entering rotcur potential(5NEMO): removed previous tables");
        free(rad);
        free(vel);
        free(coef);
        entries++;
    }

    dprintf (1,"INIPOTENTIAL Rotcur potential %s\n",name);
    dprintf (1,"  Parameters : Pattern Speed = %f\n",omega);
    dprintf (1,"  Table = %s\n",name);

    nmax = file_lines(name);
    if (nmax<=0) error("file_lines returned %d lines in %s\n",
             nmax,name);
    rad = (real *) allocate(nmax * sizeof(real));
    vel = (real *) allocate(nmax * sizeof(real));
    coldat[0] = rad;        colnr[0] = 1;
    coldat[1] = vel;        colnr[1] = 2;

    instr = stropen(name,"r");
    nrad = get_atable(instr,2,colnr,coldat,nmax);
    strclose(instr);
    if (nrad<=0) error("No lines (%d)read from %s",nrad,name);
    coef = (real *) allocate(nrad*3*sizeof(real));
    spline(coef, rad, vel, nrad);
    par[0] = omega;
        dprintf(2,"rotcur[1]: %g %g\n",rad[0],vel[0]);
        dprintf(2,"rotcur[%d]: %g %g\n",nrad,rad[nrad-1],vel[nrad-1]);
}
    
void potential (ndim,pos,acc,pot,time)
int    *ndim;
double pos[], acc[], *pot, *time;
{
    real r, r2, v, f;
    int    i;

    for (i=0, r2=0.0; i<*ndim; i++)
        r2 += sqr(pos[i]);
    r=sqrt(r2);
    if (r<rad[0] || r>rad[nrad-1]) {
        return;
    } 

    v = seval( r, rad, vel, coef, nrad);     /* interpolate */
    if (r > 0)
	f = sqr(v/r);
    else
    	f = 0;
    dprintf(2,"r=%g v=%g f=%g\n",r,v,f);

    *pot = 0.0;             /* can't compute potentials that easily ... yet */
    acc[0] = -f*pos[0]; 
    acc[1] = -f*pos[1]; 
    acc[2] = 0.0;
}
