/*
 * SNAPADD: add N-body systems on top of each other.
 *
 *	18-nov-90  2.0 Based on an earlier version of snapadd and snapstack 
 *	21-may-91  2.1 Allow addition of all-massless snapshots
 *	23-may-91  2.1a Be aware of massless snapshots
 *	13-may-92  2.2 Allow concurrent adding; default zerocm=false now
 *	22-nov-92  2.3 options= to be able to erad phi,acc also
 *	mar-94 ansi
 *
 * Some limitations:
 *  snapshots should be not change in size, or else reallocate perhaps
 *      and change nbodytot for output
 *  if masses are on/off later on, this is not treated the best of possible 
 *      ways
 *
 */

#include <stdinc.h>
#include <getparam.h>
#include <vectmath.h>
#include <filestruct.h>
#include <snapshot/snapshot.h>

string defv[] = {		/* DEFAULT INPUT PARAMETERS */
    "in=???\n		input file names (separated by commas)",
    "out=???\n		output file name",
    "zerocm=false\n	zero new center of mass",
    "options=\n         Forced output of: {acc, phi}",
    "headline=\n	random verbiage",
    "VERSION=2.3a\n	8-mar-94 PJT",
    NULL,
};

string usage="add N-body systems on top of each other.";

#if !defined(MAXSNAP)
# define MAXSNAP 32
#endif

stream instr[MAXSNAP], outstr=NULL;
int nbodytot=0, nbody[MAXSNAP];
real *masstot=NULL, *mass[MAXSNAP];
real *phasetot=NULL, *phase[MAXSNAP];
real *phitot=NULL, *phi[MAXSNAP];
real *acctot=NULL, *acc[MAXSNAP];
bool Qmass, Qzerocm, needphi, needacc;
real tsnap;
int nsnap;


void nemo_main() 
{
    int readdata();

    setparams();
    while (readdata()) {
       snapstack();
       writedata();
    }
}

setparams()
{
    string *burststring(), *pp, options;
    int i, offset;
    bool scanopt();

    Qzerocm = getbparam("zerocm");
    options = getparam("options");
    needphi = scanopt(options,"phi");
    needacc = scanopt(options,"acc");

    pp = burststring(getparam("in"),", \t");    /* tokenize in= */
    nsnap = xstrlen(pp,sizeof(string))-1;       /* find how many there are */
    if (nsnap<=0) 
        error("No valid files specified: in=");
    else if (nsnap <2)
        warning("Only one file given: in=");
    else if (nsnap >MAXSNAP) {
        warning("Maximum snapshots MAXSNAP=%d",MAXSNAP);
        nsnap = MAXSNAP;
    }
    for (i=0; i<nsnap; i++)                     /* open all files */
        instr[i] = stropen(pp[i], "r");
}

int readdata()
{
    char *strname();
    int i, offset, n=0;

    Qmass = FALSE;  /* reset mass output flag */

    for (i=0; i<nsnap; i++) {       /* read all headers to get total nbody */
        for(;;) {                   /* loop forever until data found */
            get_history(instr[i]);
            if (!get_tag_ok(instr[i],SnapShotTag)) {
                if (i>0) 
                    warning("Early termination: file %s not enough snapshots",
                        strname(instr[i]));
                return 0;        /* abort !! */
            }
            get_set(instr[i], SnapShotTag);
            get_set(instr[i], ParametersTag);
            get_data(instr[i], NobjTag, IntType, &nbody[i], 0);
            if (i==0) {
                if(get_tag_ok(instr[i],TimeTag))
                    get_data(instr[i], TimeTag, RealType, &tsnap, 0);
                else
                    tsnap=0.0;
            }
            get_tes(instr[i], ParametersTag);
            if (get_tag_ok(instr[i],ParticlesTag)) {
                get_set(instr[i],ParticlesTag);
                break; /* OK, found data !! */
            }
            get_tes(instr[i], SnapShotTag);
        } /* for(;;) */


        if (masstot==NULL) {		/* first time around */
            dprintf(0,"nbody%d = %d\n", i+1,nbody[i]);
            n = nbodytot += nbody[i];
        } else
            n += nbody[i];


    } /* for (i) */

    if (n!=nbodytot) error("Snapshots of different size");

    if (masstot==NULL) {
        masstot = (real *) allocate(sizeof(real) * nbodytot); 
        for (i=0; i<nbodytot; i++)
            masstot[i] = 0.0;
    }
    if (phasetot==NULL) 
        phasetot = (real *) allocate(sizeof(real) * 2*NDIM * nbodytot);
    if (needphi) {
        phitot = (real *) allocate(sizeof(real) * nbodytot);
	dprintf(0,"Also writing potentials\n");
    }
    if (needacc) {
        acctot = (real *) allocate(sizeof(real) * NDIM * nbodytot);
    	dprintf(0,"Also writing accelerations\n");
    }
    offset = 0;
    for (i=0; i<nsnap; i++) {           /* read in data */
        mass[i] = masstot + offset;
        phase[i] = phasetot + 2 * NDIM * offset;
        if(needphi) phi[i] = phitot + offset;
        if(needacc) acc[i] = acctot + NDIM * offset;
        offset += nbody[i];

	if (get_tag_ok(instr[i], MassTag)) {
            get_data_coerced(instr[i], MassTag, RealType, mass[i], 
                             nbody[i], 0);
            Qmass = TRUE;
        }
        get_data_coerced(instr[i], PhaseSpaceTag, RealType, phase[i],
		         nbody[i], 2, NDIM, 0);
        if (needphi) 
            get_data_coerced(instr[i], PotentialTag, RealType, phi[i], 
                             nbody[i], 0);
        if (needacc)
            get_data_coerced(instr[i], AccelerationTag, RealType, acc[i],
		             nbody[i], NDIM, 0);
        get_tes(instr[i], ParticlesTag);
        get_tes(instr[i], SnapShotTag);
    }
    return 1;
}


snapstack()
{
    int i, nzero;

    if (Qzerocm) {
        nzero = 0;
        for (i=0; i<nbodytot; i++)
            if (masstot[i] == 0.0) nzero++;
        if (nzero>0) 
	    warning("A total of %d massless particles used in c.o.m.\n",nzero);
        if (nzero==nbodytot)
            for (i=0; i<nbodytot; i++)  masstot[i] = 1.0;       /* fake */            
	zerocms(phasetot, 2*NDIM, masstot, nbodytot, nbodytot);
    }
}

writedata()
{
    static int cscode = CSCode(Cartesian, NDIM, 2);

    if (outstr==NULL) {         /* do some extras the first time around */
        if (! streq(getparam("headline"), ""))
            set_headline(getparam("headline"));
        outstr = stropen(getparam("out"), "w");
        put_history(outstr);
    }
    put_set(outstr, SnapShotTag);
      put_set(outstr, ParametersTag);
        put_data(outstr, NobjTag, IntType, &nbodytot,0);
        put_data(outstr, TimeTag, RealType, &tsnap,0);
      put_tes(outstr, ParametersTag);
      put_set(outstr, ParticlesTag);
        put_data(outstr, CoordSystemTag, IntType, &cscode,0);
	if (Qmass)
          put_data(outstr, MassTag, RealType, masstot, nbodytot,0);
        put_data(outstr, PhaseSpaceTag, RealType, phasetot, nbodytot,2,NDIM,0);
        if(needphi)
          put_data(outstr, PotentialTag, RealType, phitot, nbodytot,0);
        if(needacc)
          put_data(outstr, AccelerationTag, RealType, acctot, nbodytot,NDIM,0);
      put_tes(outstr, ParticlesTag);
    put_tes(outstr, SnapShotTag);
}
