/*
 * MKDISK.C: set up a uniform-density test disk in a spherical potential(5)
 *	
 *	original version: xx-jul-87	Peter Teuben
 *		V2.0: 8-feb-89	based on mktestdisk with Potential(5)  PJT
 *		V3.0: 12-may-90  new keywords for potential(5)		PJT
 *              V3.1:  2-nov-90  will not write masses when zero ...    PJT
 *		V3.2: 14-nov-90  NEMO 2.x
 *		V4.1: 11-jun-92  added sign of angular momentum vector  PJT
 *			 	 nbody= is now second parameter	
 *		V4.2b: 24-jul-98 bit more documentation			PJT
 *		** still broken for SINGLEPREC **
 */

#include <stdinc.h>
#include <getparam.h>
#include <vectmath.h>
#include <filestruct.h>
#include <potential.h>

#include <snapshot/snapshot.h>
#include <snapshot/body.h>
#include <snapshot/put_snap.c>

string defv[] = {	/* DEFAULT INPUT PARAMETERS */
    "out=???\n		Output file name (snapshot)",
    "nbody=2048\n	Number of disk particles",
    "potname=plummer\n  Name of potential(5)",
    "potpars=\n         Parameters to potential(5); omega needed but not used",
    "potfile=\n         Optional data file with potential(5)",
    "rmin=0\n		Inner disk radius",
    "rmax=1\n		Outer cutoff radius",
    "mass=0\n		Total mass of disk (0 means no masses supplied)",
    "frac=0\n           Relative vel.disp w.r.t. local rotation speed",
    "seed=0\n		Usual random number seed",
    "sign=1\n           Sign of Z-angular momentum vector of disk",
    "in=\n              If given, these are initial positions",
    "vr=0\n             radial velocity",
    "headline=\n	Text headline for output",
    "VERSION=4.2c\n	10-oct-00 PJT",
    NULL,
};

string usage="set up a uniform-density test disk in a spherical potential";

local real rmin, rmax, mass;
local int  jz_sign;

local int ndisk;
local real frac, vrad;
local Body *disk;

proc potential;

extern double xrandom(double,double), grandom(double,double);

local real took(real);

void nemo_main()
{
    bool Qmass;
    
    if (hasvalue("in")) error("\"in=\" not implemented yet");
    potential = get_potential(getparam("potname"),
                    getparam("potpars"), getparam("potfile"));
    rmin = getdparam("rmin");
    rmax = getdparam("rmax");
    frac = getdparam("frac");
    vrad = getdparam("vrad");
    ndisk = getiparam("nbody");
    jz_sign = getiparam("sign");
    if (ABS(jz_sign) != 1) error("%d: sign must be +1 or -1",jz_sign);
    mass = getdparam("mass") / ndisk;
    if (mass==0.0)  {
	Qmass=FALSE;
	warning("mass=0 -- No masses created in output snapshot");
    } else
        Qmass=TRUE;
    set_xrandom(getiparam("seed"));
    testdisk();
    writegalaxy(getparam("out"), getparam("headline"), Qmass);
}

/*
 * WRITEGALAXY: write galaxy model to output.
 */

writegalaxy(name, headline, Qmass)
string name;
string headline;
bool Qmass;
{
    stream outstr;
    real tsnap = 0.0;
    int bits;

    if (! streq(headline, ""))
	set_headline(headline);
    outstr = stropen(name, "w");
    put_history(outstr);
    if (Qmass)
        bits = MassBit | PhaseSpaceBit;
    else
        bits = PhaseSpaceBit;
    put_snap(outstr, &disk, &ndisk, &tsnap, &bits);
    strclose(outstr);
}

/*
 * TESTDISK: use forces due to a potential to make a uniform
 * density test disk.  
 */

testdisk()
{
    Body *dp;
    real rmin2, rmax2, r_i, theta_i, vcir_i, pot_i, t;
    real  dv_r, dv_t, sint, cost, sigma_i;
    vector acc_i;
    int i, ndim=NDIM;
    double pos_d[NDIM], acc_d[NDIM], pot_d, time_d = 0.0;

    disk = (Body *) allocate(ndisk * sizeof(Body));
    rmin2 = rmin * rmin;
    rmax2 = rmax * rmax;
    t = 0;    /* dummy time ; we do not support variable time yet */
    for (dp=disk, i = 0; i < ndisk; dp++, i++) {	/* loop all stars */
	Mass(dp) = mass;
	r_i = sqrt(rmin2 + i * (rmax2 - rmin2) / (ndisk - 1.0));
	theta_i = xrandom(0.0, TWO_PI);
        cost = cos(theta_i);
        sint = sin(theta_i);
	Pos(dp)[0] = pos_d[0] = r_i * cost;		/* set positions */
	Pos(dp)[1] = pos_d[1] = r_i * sint;
	Pos(dp)[2] = pos_d[2] = 0.0;                      /* it's a DISK */
        (*potential)(&ndim,pos_d,acc_d,&pot_d,&time_d); /* get forces    */
        SETV(acc_i,acc_d);
	vcir_i = sqrt(r_i * absv(acc_i));
        sigma_i = grandom(0.0,frac*vcir_i);
        dv_t = sigma_i;
        dv_r = sigma_i * took(r_i) + vrad;
        cost = cos(theta_i);  
        sint = sin(theta_i);
	Vel(dp)[0] =  -vcir_i * sint * jz_sign;
	Vel(dp)[1] =   vcir_i * cost * jz_sign;
	Vel(dp)[0] += cost*dv_r - sint*dv_t;  /* add dispersions */
	Vel(dp)[1] += sint*dv_r + cost*dv_t;
	Vel(dp)[2] = 0.0;                     /* it's a DISK ! */
    }
}


/*   
 *   2*omega/kappa; from spline interpolation.....
 */

real took(real r)
{
    return  1.0;
}
