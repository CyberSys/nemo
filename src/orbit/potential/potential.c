/* potential.c - proc get_potential, local load_potential */
/*               double get_pattern                       */

/*------------------------------------------------------------------------------
 *  POTENTIAL:  procedures for finding fixed potential, initially written 
 *		for newton0
 *
 *      July 1987  -  Peter Teuben  @ Inst. f. Adv. Study, 
 *						  Princeton, NJ 08540, USA
 *	April 1988 -  V2.0 added dataname parameter to get_potential
 *	March 1989 -  V2.1 added dataname length in case fortran was called 
 *			   (only BSD interface fortran-C)
 *	Febr. 1990 -  V3.0 added time as extra parameter in potential() call 
 *			   although no extra code needed here...
 *			   drange() replaced by nemoinpd()		 - PJT
 *	14-mar-90     V3.1 made GCC happy - increased MAXPAR for Stefano - PJT
 *      25-oct-90     V3.2 allow multiple get_potential() calls		   PJT
 *	 6-nov-90     V3.2a force link of spline(3NEMO) routines	   PJT
 *	20-nov-90         b NEMOPATH->NEMO and few other cosmetics         PJT
 *      14-oct-91     V3.3  add image I/O to make potfft a potential(5)    PJT
 *	 7-mar-92     V3.3a happy gcc2.0                                   PJT
 *	24-may-92     V3.4 changed names of local variables to accomodate
 *			   the new 'potential' data type.		   PJT
 *      11-oct-93     V5.0 get_pattern                                     PJT
 *	20-jan-94     V5.1 using mapsys(); needed for solaris		   pjt
 *	22-feb-94     V5.1a ansi
 *	23-mar-95         b prototypes	- fixed 0-strlen bug in f77 I/O    pjt
 *	17-apr-95           use -DNO_IMAGE				   pjt
 *      13-dec-95     V5.2  Now using ldso to access shared objects        pjt 
 *                          fixed small CFLAGS bug
 *	 1-apr-01     V5.3  converted for NEMO V3 with .so files           pjt
 *------------------------------------------------------------------------------
 */

#include  <stdinc.h>
#include  <getparam.h>
#include  <loadobj.h>
#include  <filefn.h>
#include  <potential.h>

#define MAXPAR 64

local double local_par[MAXPAR]; /* NOTE: first par reserved for pattern speed*/
local int  local_npar=0;        /* actual used number of par's               */
local real local_omega=0;	/* pattern speed                             */
local proc l_potential=NULL;    /* actual storage of pointer to exter worker */
local proc l_inipotential=NULL; /* actual storage of pointer to exter inits  */
local bool Qfortran = FALSE;    /* was a fortran routine used ? -- a hack -- */
local bool first = TRUE;        /* see if first time called for mysymbols()  */

/* forward declarations */

local proc load_potential(string, string, string); /* load by name    */

/*-----------------------------------------------------------------------------
 *  get_potential --  returns the pointer ptr to the function which carries out
 *                the calculation of potential and accelerations.
 *-----------------------------------------------------------------------------
 */
proc  get_potential(string potname, string potpars, string potfile)
{
    if (potname == NULL || *potname == 0)	/* if no name provided */
        return NULL;				/* return no potential */
    l_potential = load_potential(potname, potpars, potfile);
    return l_potential;
}

/*-----------------------------------------------------------------------------
 *  get_inipotential --  returns the pointer ptr to the last inipotential
 *          function which initializes the potential
 *-----------------------------------------------------------------------------
 */
proc get_inipotential()
{
    if (first) 
        warning("get_inipotential: get_potential not called yet");
    return l_inipotential;
}

/*-----------------------------------------------------------------------------
 *  get_pattern --  return the last set pattern speed
 *		    note that a 0.0 parameter does *not* reset it !!!
 *-----------------------------------------------------------------------------
 */

real get_pattern()
{
    if (first) error("get_pattern: get_potential not called yet");
    return local_omega;
}

/*-----------------------------------------------------------------------------
 *  load_potential -- load the potential from an object file
 *       This routine depends heavily on the object-loader (loadobj(3NEMO))
 *	BUG: if dataname is NULL, the routines dies when trying f_c interface 
 *		since it can't take strlen(NULL)
 *-----------------------------------------------------------------------------
 */
local proc load_potential(string fname,string parameters, string dataname)
{
    char  name[256], cmd[256], path[256], pname[32];
    char  *fullname, *nemopath, *potpath, *cflags;
    proc  pot, ini_pot;

    if (parameters!=NULL && *parameters!=0) {              /* get parameters */
	local_npar = nemoinpd(parameters,local_par,MAXPAR);
        if (local_npar>MAXPAR)
            error ("get_potential: potential has too many parameters (%d)",
                            local_npar);
        if (local_npar<0)
            warning("get_potential: parsing error in: %s",parameters);
    } else
        local_npar=0;
    if (local_npar > 0 && local_par[0] != 0.0) { /* aid multiple potentials */
        local_omega = local_par[0];
        dprintf(1,"get_potential: setting local_omega = %g\n",local_omega);
    }

    if (first) {
        mysymbols(getparam("argv0"));      /* get symbols for this program */
        first = FALSE;			   /* and tell it we've initialized */
    }
    potpath = getenv("POTPATH");	     /* is there customized path ? */
    if (potpath==NULL) {			/* use default path */
       potpath = path;
       strcpy (path,".:");
       nemopath = getenv("NEMO");
       strcat (path,nemopath);
       strcat (path,"/obj/potential");		/* ".:$NEMO/obj/potential" */
    }
    strcpy (name,fname);
#if 1
    strcat (name,".so");
#else
    strcat (name,".o");
#endif

    fullname = pathfind (potpath, name);
    if (fullname!=NULL) {			/* .o found !! */
        dprintf (2,"Attempt to load potential from %s\n",name);
    	loadobj(fullname);
    } else {					/* no .o found */
	strcpy (path,".");	/* just look in current directory */
	strcpy (name,fname);
	strcat (name,".c");
	if (pathfind(".",name)==NULL)
	    error("get_potential: no potential %s found",name);
	dprintf (0,"[Compiling potential %s]\n",name);	
	cflags = getenv("CFLAGS");
#if 1
	sprintf (cmd,"make -f $NEMOLIB/Makefile.lib %s.so",name);
#else
	sprintf (cmd,"cc %s -c %s",cflags ? cflags : "",name);
#endif
	dprintf (1,"%s\n",cmd);
	if (system(cmd)!=0)
	    error ("Error in compiling potential file");
#if 1
	strcpy (name,fname);
	strcat (name,".so");
#else
	sprintf(cmd,"ldso %s",fname);
        dprintf(1,"%s\n",cmd);
	if (system(cmd)!=0)
		error("Error in making shared object");
	strcpy (name,fname);
	strcat (name,".o");
#endif
	fullname = name;	/* or use: findpath ? */
	loadobj (name);
    }

    dprintf (1,"[Potential loaded from object file %s]\n",fullname);
    strcpy(pname,"potential");
    mapsys(pname);
    pot = (proc) findfn (pname);             /* try C-routine */
    if (pot==NULL) {                          
        Qfortran = TRUE;		      /* must be F77 then... */		
        strcat(pname,"_");
        pot = (proc) findfn (pname);          /* try F77-routine */
        if (pot==NULL)                        /* blasted programmer? */
            error ("getpotential: %s() not present",pname);
    }
    strcpy(pname,"inipotential");
    mapsys(pname);
    ini_pot = (proc) findfn (pname);              		/* C */
    if (ini_pot==NULL) {
        strcpy(pname,"ini_potential");
        mapsys(pname);
        ini_pot = (proc) findfn (pname);	        	/* C */
        if (ini_pot==NULL) {
            strcpy(pname,"inipotential_");
            mapsys(pname);
            ini_pot = (proc) findfn (pname);		        /* F77 */
            if (ini_pot==NULL) {
                strcpy(pname,"ini_potential_");
                mapsys(pname);
                ini_pot = (proc) findfn (pname);        	/* F77 */
            }
        }
    }
    if (ini_pot)
        if (!Qfortran)
            (*ini_pot)(&local_npar,local_par,dataname); 	/* C */
        else {
            if (dataname==NULL)
                (*ini_pot)(&local_npar,local_par,dataname,0);   /* F77 */
            else
                (*ini_pot)(&local_npar,local_par,dataname,strlen(dataname)); /* F77 */

        }
    else {
        printf ("Warning: inipotential(_) not present in %s", fname);
        printf (",default taken\n");
    }
    l_potential = pot;            /* save these two for later references */
    l_inipotential = ini_pot;
    if (local_npar > 0 && local_par[0] != local_omega) {
    	local_omega = local_par[0];
    	dprintf(1,"get_potential: modified omega=%g\n",local_omega);
    }
    return pot;
}
/* endof: potential.c */
 
/******   now some junk needed to force linker to load some extra ******/

#include <mathlinker.h>		/* extra math */
#include <fslinker.h>		/* extra routines from filestruct() */

void local dummy_for_c()
{
    double a,b,c;
    int spline();
    double bessi0(), bessk0(), bessi1(), bessk1();
    void get_atable();
    void read_image();

    error("Cannot call dummy_for_c - included to fool the linker");
    (void) spline();
    (void) bessi0();
    (void) bessk0();
    (void) bessi1();
    (void) bessk1();
    stropen("/dev/null","w");
#ifndef NO_IMAGE
    read_image();
#endif    
    get_atable();
}

#if defined(FORTRAN)
void local dummy_for_fortran()
{
	zzzzzz_();			/* force loading of Fortran I/O */
}
#endif

void local dummy_for_fortran_math()
{
	fmath_();
}
