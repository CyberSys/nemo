/*WIP: adding mode=plane */
/*TODO:adding mode=poly*/
/*BUG: large tables will fail in old table input method */
/*
 * TABLSQFIT: a general (linear) fitting program for tabular data 
 *
 *       5-aug-87  created using NumRec in double C     Peter Teuben
 *      10-nov-87  V1.2 need new 'drange' version skipping blanks
 *      11-Mar-88  V1.3 dprintf() output now used
 *      17-May-88  V1.4 optional output of data+residuals
 *      31-May-88  V2.0 for NumRec beta-release (float instead of double)
 *                      fixed small buf for interactive input (see also tabmath)
 *      30-aug-90  V2.1 fitting ellipses (still unfinished)
 *      13-nov-90  V2.2 drange->nemoinp; using lsq() not NumRec
 *       9-apr-91  V2.3 optional output with deleting high residuals data
 *      15-nov-91  V2.4 fixed ellipse fit routine (PJT)
 *	21-nov-91       optional plot ellipse and or residuals - hyperbola
 *	12-mar-92  V2.5 added the imageshift fit (complex y=ax+b)
 *	13-apr-92  V2.5a  fixed bad nemoinpi("x...   calls
 *	 6-nov-93  V2.6 added nmax= to allow dynamic input
 *		       a  handle no-data cases
 *      12-jun-94  V2.7 general overhaul to use a 'column' data structure
 *                      to make code extendible for different fit modes
 *	 8-jun-95  V2.8 added fit option peak
 *
 *	30-jan-97     a resurrected the xrange selection for nxcol==1 !!
 *	26-jan-98  V2.9 added gamma functions and goodness of fit for fit=line
 *       4-feb-98     a compute 'r' (corr.coeff.) for fit=line
 *      14-feb-98     c fixed bug in poly fitting column requirement
 *	17-apr-00     d fixed edge finding in do_peak()
 *      14-aug-00  V3.0 added tab=t|f for tabular short output
 *      23-mar-01     a fixed xrange=  for singular column files
 */

/*
 * literature:
 *
   W.Gander, G.H.Golub and RStrebel, "Least-Square Fitting of Circles and Ellipses",
    BIT, No,43 pp.558-578, 1998
or
    A.Fitzbibbon, M. Pilu and R.B.Fisher, "Direct Least Square Fitting of Ellipse",
    IEEE Tran. Pattern Analysis and Maschine Intelligence, Vol.21, No.5, pp.476-480, May 1999

 */

#include <stdinc.h>  
#include <getparam.h>

string defv[] = {
    "in=???\n           input (table) file name",
    "xcol=1\n           column(s) for x",
    "ycol=2\n           column(s) for y",
    "dycol=\n           column for sigma-y, if used",
    "xrange=\n          in case restricted range is used (1D only)",
    "fit=line\n         fitmode (line, ellipse, imageshift, plane, poly, peak)",
    "order=0\n		Order of plane/poly fit",
    "out=\n             optional output file for some fit modes",
    "nsigma=-1\n        delete points more than nsigma away?",
    "estimate=\n        optional estimates (e.g. for ellipse center)",
    "nmax=10000\n       Default max allocation",
    "tab=f\n            short one-line output?",
    "VERSION=3.0a\n     23-mar-01 PJT",
    NULL
};

string usage="a linear least square fitting program";

/**************** SOME GLOBAL VARIABLES ************************/

#if !defined(HUGE)
#define HUGE 1e20
#endif

#define MAXCOL 10

typedef struct column {
    int maxdat;     /* allocated length of data */          /* not used */
    int ndat;       /* actual length of data */             /* not used */
    real *dat;      /* pointer to data */
    int colnr;      /* column number this data came from */ /* not used */
} column;

int nxcol, nycol, xcolnr[MAXCOL], ycolnr[MAXCOL], dycolnr; 
column            xcol[MAXCOL],   ycol[MAXCOL],   dycol;

real xrange[MAXCOL*2]; /* ??? */

string method;
stream instr, outstr;                            /* input file */


int    nmax;                         /* allocated space */
int    npt;                          /* actual number of points from table */
real   nsigma;                       /* fractional sigma removal */

real  a,b;                           /* fit parameters in: y=ax+b  */
int order;

bool Qtab;

extern bool scanopt();


/****************************** START OF PROGRAM **********************/

nemo_main()
{

    setparams();
    read_data();

    if (scanopt(method,"line")) {
        do_line();
    } else if (scanopt(method,"ellipse")) {
        do_ellipse();
    } else if (scanopt(method,"imageshift")) {
        do_imageshift();
    } else if (scanopt(method,"plane")) {
    	do_plane();
    } else if (scanopt(method,"poly")) {
    	do_poly();
    } else if (scanopt(method,"peak")) {
    	do_peak();
    } else
        error("fit=%s invalid; try [line,ellipse,imageshift,plane,poly,peak]",
	      getparam("fit"));

    if (outstr) strclose(outstr);
}

setparams()
{
    string inname = getparam("in");
    nmax = file_lines(inname,getiparam("nmax"));
    if (nmax<0) error("Error opening %s",inname);
    if (nmax==0) error("No data?");
    instr = stropen (inname,"r");

    if (hasvalue("out"))
        outstr=stropen(getparam("out"),"w");
    else
        outstr=NULL;

    nxcol = nemoinpi(getparam("xcol"),xcolnr,MAXCOL);
    if (nxcol<0) error("Illegal xcol= nxcol=%d",nxcol);
    nycol = nemoinpi(getparam("ycol"),ycolnr,MAXCOL);
    if (nycol<0) error("Illegal ycol= nycol=%d",nycol);
    if (hasvalue("dycol"))
        dycolnr = getiparam("dycol");
    else
        dycolnr = 0;

    if (hasvalue("xrange"))
        setrange(xrange,getparam("xrange"));
    else {
        xrange[0] = -HUGE;
        xrange[1] = HUGE;
    } 
    
    method = getparam("fit");
    nsigma = getdparam("nsigma");
    order = getiparam("order");
    if (order<0) error("order=%d of %s cannot be negative",order,method);
    Qtab = getbparam("tab");
}

setrange(rval, rexp)
real rval[];
string rexp;
{
    char *cptr;

    cptr = strchr(rexp, ':');
    if (cptr != NULL) {
        rval[0] = atof(rexp);
        rval[1] = atof(cptr+1);
    } else {
        rval[0] = 0.0;
        rval[1] = atof(rexp);
    	warning("Range taken from 0 - %g",rval[1]);
    }
}

read_data()
{
    real *coldat[2*MAXCOL+1];
    int colnr[2*MAXCOL+1], ncols = 0, i, j;

    for (i=0; i<nxcol; i++) {
        coldat[ncols] = xcol[i].dat = (real *) allocate(nmax * sizeof(real));
        colnr[ncols] = xcolnr[i];        
        ncols++;
    }
    for (i=0; i<nycol; i++) {
        coldat[ncols] = ycol[i].dat = (real *) allocate(nmax * sizeof(real));
        colnr[ncols] = ycolnr[i];        
        ncols++;
    }
    if (dycolnr>0) {
        coldat[ncols] = dycol.dat = (real *) allocate(nmax * sizeof(real));
        colnr[ncols] = dycolnr;
        ncols++;
    }
    
    npt = get_atable(instr,ncols,colnr,coldat,nmax);
    if (npt < 0) {
        npt = -npt;
       	warning("Could only read %d data",npt);
    }


    /* special case for nxcol=1  ... what to do for nxcol > 1 ??? */
    /* should also handle nycol > 1  but does not yet             */

    if (nxcol == 1 && nycol == 1) {
        for(i=0, j=0; i<npt; i++) {
          if(xrange[0] < xcol[0].dat[i] && xcol[0].dat[i] < xrange[1]) {    /* sub-select on X */
              xcol[0].dat[j] = xcol[0].dat[i];
              ycol[0].dat[j] = ycol[0].dat[i];
              if (dycolnr>0) dycol.dat[j] = dycol.dat[i];
              j++;
           }
        }
        dprintf(1,"Copied over %d/%d data within xrange's\n",j,npt);
	npt = j;
    }
       
    if (npt==0) error("No data");
}


write_data(outstr)              /* only for straight line fit */
stream outstr;
{
#if 0    
    int i;
    real d;

    for (i=0; i<npt; i++) {
        d = y[i] - a*x[i] - b;
        fprintf (outstr,"%f %f %f\n",x[i],y[i],d);
    }
#endif    
}

do_line()
{
    real *x, *y, *dy;
    int i,j, mwt;
    real chi2,q,siga,sigb, sigma, d;
    real r, prob, z;
    void fit(), pearsn();

    if (nxcol < 1) error("nxcol=%d",nxcol);
    if (nycol < 1) error("nycol=%d",nycol);
    x = xcol[0].dat;
    y = ycol[0].dat;
    dy = (dycolnr>0 ? dycol.dat : NULL);

    for(mwt=0;mwt<=1;mwt++) {

#if 1
                if ( (mwt>0) && (dycolnr==0) )
                        continue;               /* break off */
                fit(x,y,npt,dy,mwt,&b,&a,&sigb,&siga,&chi2,&q);
#else
		if (mwt==0)
                    fit(x,y,npt,dy,mwt,&b,&a,&sigb,&siga,&chi2,&q);
                else
                  myfit(x,y,npt,dy,mwt,&b,&a,&sigb,&siga,&chi2,&q);
#endif
                dprintf(2,"\n");
                dprintf (1,"Fit:   y=ax+b\n");
                if (mwt == 0)
                        dprintf(1,"Ignoring standard deviations\n");
                else
                        dprintf(1,"Including standard deviation\n");
                printf("%12s %9.6f %18s %9.6f \n",
                        "a  =  ",a,"uncertainty:",siga);
                printf("%12s %9.6f %18s %9.6f \n",
                        "b  =  ",b,"uncertainty:",sigb);
                printf("%19s %14.6f \n","chi-squared: ",chi2);
                printf("%23s %10.6f %s\n","goodness-of-fit: ",q,
                	q==1 ? "(no Y errors supplied [dycol=])" : "");

                pearsn(x, y, npt, &r, &prob, &z);
                
                printf("%9s %g\n","r: ",r);
                printf("%12s %g\n","prob: ",prob);
                printf("%9s %g\n","z: ",z);
                
            if (nsigma>0) {                
                sigma = 0.0;
                for(i=0; i<npt; i++)
                    sigma += sqr(y[i] - a*x[i] - b);
                sigma /= (real) npt;
                sigma = nsigma * sqrt(sigma);   /* critical distance */
    
                for(i=0, j=0; i<npt; i++) {
                    d = ABS(y[i] - a*x[i] - b);
                    if (d > sigma) continue;
                    x[j] = x[i];
                    y[j] = y[i];
                    if (dy) dy[j] = dy[i];
                    j++;
                }
                dprintf(0,"%d/%d points outside %g*sigma (%g)\n",
                        npt-j,npt,nsigma,sigma);
                npt = j;
            }

    } /* mwt */
    
    if (outstr) write_data(outstr);
}

char name[10] = "ABCDE";

do_ellipse()
{
    real *xdat, *ydat;
    real mat[5*5], vec[5], sol[5], a[6], cnt, xmean, ymean, x, y;
    real aa,bb,cc,dd,ee,pa,pp,cospp,sinpp,cospa,sinpa,ecc,al,r,ab,
         s1,s2,s3,y1,y2,y3,x0,y0, sum0, sum1, sum2, dx, dy, rr,
	 radmean, radsig, dr;
    real aaa, bbb, xp, yp, delta;
    int i, j;

    if (nxcol < 1) error("nxcol=%d",nxcol);
    if (nycol < 1) error("nycol=%d",nycol);
    xdat = xcol[0].dat;
    ydat = ycol[0].dat;

    if (npt < 5) {
        warning("Got %d; need minimum 5 points for an ellipse",npt);
        return 0;
    }
    xmean = ymean = 0.0;
    for (i=0; i<npt; i++) {             /* get mean of (x,y) as first guess */
      xmean += xdat[i];                 /* of center of ellips */
      ymean += ydat[i];                 /* to prevent all too much roundoff */
      dprintf(2,"Data: %f %f\n",xdat[i], ydat[i]);
    } 
    xmean /= npt;
    ymean /= npt;
    dprintf(1,"Estimate for center of ellips using %d points: %f %f\n",
					npt,xmean,ymean);
    if (hasvalue("estimate")) {
        if (nemoinpr(getparam("estimate"),vec,2) != 2) 
            error("estimate=%s needs two values for center of ellipse",
                   getparam("estimate"));
        xmean = vec[0];
        ymean = vec[1];
        dprintf(1,"Reset center of ellips: %f %f\n", xmean,ymean);
    }

    lsq_zero(5,mat,vec);

    for (i=0; i<npt; i++) {       /* gather all the stuff in matrix */
        x = xdat[i]-xmean;          /* treat (x,y) w.r.t. the mean center */
        y = ydat[i]-ymean;          /* of all points to prevent rounding err */
        a[0] = sqr(x);
        a[1] = 2*x*y;
        a[2] = sqr(y);
        a[3] = x;
        a[4] = y;
        a[5] = 1.0;
        lsq_accum(5,mat,vec,a,1.0); /* spread a[] into mat[] and vec[] */
    }

    for (i=0; i<5; i++)		/* print input matrix */
      dprintf(1,"( %9.3e %9.3e %9.3e %9.3e %9.3e  ) * ( %c ) = ( %9.3e )\n",
        mat[i*5+0], mat[i*5+1], mat[i*5+2], mat[i*5+3], mat[i*5+4],
        name[i], vec[i]);

    lsq_solve(5,mat,vec,sol);

    for (i=0; i<5; i++)		/* print inverse matrix & solution */
      dprintf(1,"( %9.3e %9.3e %9.3e %9.3e %9.3e  ) ; %c = %9.3e\n",
        mat[i*5+0], mat[i*5+1], mat[i*5+2], mat[i*5+3], mat[i*5+4],
        name[i], sol[i]);

    /* Now that we have the coefficient a,b,c,d,e in:
     *      a.x^2 + 2bxy + cy^2 + dx + ey = 1
     * They have to be converted to human readable ones
     */

    aa = sol[0]; bb = sol[1]; cc = sol[2]; dd = sol[3]; ee = sol[4];
    delta = aa*cc-bb*bb;
    if (delta < 0)
        warning("You seem to have an hyperbola, not an ellipse");
    pp = atan(2.0*bb/(aa-cc));
    pa = 0.5*pp;        /* P.A. of undetermined axis */
    cospp = cos(pp);
    cospa = cos(pa);
    sinpp = sin(pp);
    sinpa = sin(pa);
    r = (sinpp*(aa+cc) + 2*bb)/(sinpp*(aa+cc) - 2*bb);
    r = sqrt(ABS(r));           /* r is now axis ratio b/a or a/b for ell/hyp */
    s1 = bb*ee-cc*dd;
    s2 = bb*dd-aa*ee;
    s3 = aa*cc-bb*bb;
    x0 = 0.5*s1/s3;
    y0 = 0.5*s2/s3;

    lsq_zero(3,mat,vec);
    vec[0] = aa*sqr(x0)+1; 
    vec[1] = bb*sqr(x0);
    vec[2] = cc*sqr(x0);
    lsq_cfill(3,mat,0,vec);
    vec[0] = 2*aa*x0*y0; 
    vec[1] = 2*bb*x0*y0+1;
    vec[2] = 2*cc*x0*y0; 
    lsq_cfill(3,mat,1,vec);
    vec[0] = aa*sqr(y0);
    vec[1] = bb*sqr(y0);
    vec[2] = cc*sqr(y0)+1;
    lsq_cfill(3,mat,2,vec);
    vec[0] = aa;
    vec[1] = bb;
    vec[2] = cc;
    lsq_solve(3,mat,vec,sol);
    dprintf(1,"Real A,B,C = %f %f %f \n",sol[0],sol[1],sol[2]);
    if (ABS(cospp) > ABS(sinpp))    /* two ways to find ab: which is a^2 now */
        ab = (2/(sol[0]+sol[2] + (sol[0]-sol[2])/cospp));
    else                            /* use the biggest divisor */
        ab = (2/(sol[0]+sol[2] + 2*sol[1]/sinpp));

    pa *= 180/PI;           /* PA is now in degrees */
    dprintf(1,"Before re-arrangeing: ab, r, pa = %f %f %f\n",ab,r,pa);
    if (delta > 0) {            /* ELLIPSE */
        if (r>1.0) { /* ellipse with r>1 means we've got a & b mixed up */
            r = 1/r;            /* so make r < 1 */
            ab = sqrt(ab)/r;    /* and ab is now the true semi major axis */
            pa -= 90.0;         /* and change PA by 90 degrees */
       }
       ecc = sqrt(1-r*r);       /* eccentricity of the ellipse ecc=sqrt(a^2-b^2)/a */
    } else {                    /* HYPERBOLA */
        if (ab < 0) {             /* if ab<0 means we've got a and b mixed up */
            r = 1/r;              /* so make r < 1 */
            ab = sqrt(ABS(ab))/r; /* so take abs value as semi major real axis */
            pa -= 90.0;           /* and change PA by 90 degrees */
        } else {
            ab = sqrt(ab); 
        }
        ecc = sqrt(1+r*r);      /* eccentricity of the hyperbola ecc=sqrt(a^2+b^2)/a */
    }

    x0 += xmean;        /* and finally correct to the real center */
    y0 += ymean;    

    if (nsigma<0) {         /* if no rejection of 'bad' points */
        if (delta>0) {
	  if(Qtab) {
	    printf("%f %f %f %f %f %f\n",ab,ecc,r,x0,y0,pa+90);
	  } else {
            printf("Ellips fit: \n");
            printf(" semi major axis: %g\n",ab);
            printf(" eccentricity:    %g axis ratio: %g\n",ecc,r);
            printf(" x-center:        %g\n",x0);
            printf(" y-center:        %g\n",y0);
            printf(" P.A.:            %g\n",pa+90);
	  }
        } else {
            printf("Hyperbole fit: \n");
            printf(" semi real axis: %g\n",ab);
            printf(" eccentricity:   %g\n",ecc);
            printf(" x-center:       %g\n",x0);
            printf(" y-center:       %g\n",y0);
            printf(" P.A.:           %g\n",pa+90);
        }
    } else {
        dprintf(1,"Ellfit[%d]: a,b/a,x,y,pa=%g %g %g %g %g\n",npt,ab,r,x0,y0,pa+90);
			
        dprintf(0,"Now looking to delete points > %f sigma\n",nsigma);
        sum0 = sum1 = sum2 = 0.0;
        pa *= PI/180;           /* pa now in radians */
        for (i=0; i<npt; i++) {
            x = xdat[i] - x0;
            y = ydat[i] - y0;
            pp = atan2(y,x)-pa;
            rr = sqrt((sqr(x)+sqr(y))*(sqr(cos(pp))+sqr(sin(pp)/r)));
            sum0 += 1.0;
            sum1 += rr;
            sum2 += sqr(rr);
        }
        radmean = sum1/sum0;
        radsig = sqrt(sum2/sum0 - sqr(radmean));
	dprintf(0,"Deprojected radii:  mean: %f  sigma: %f\n",
			radmean, radsig);
	
        for (i=0; i<npt; i++) {
            x = xdat[i] - x0;
            y = ydat[i] - y0;
            pp = atan2(y,x)-pa;
            rr = sqrt((sqr(x)+sqr(y))*(sqr(cos(pp))+sqr(sin(pp)/r)));
            dr = (rr - radmean)/radsig;
            if (ABS(dr) < nsigma)
                printf("%g %g %g\n",xdat[i],ydat[i],dr);
            else
                dprintf(0,"Data Point %d deviates %f sigma from mean\n",
			i,dr);
        }
#if 0
        for (i=0; i<npt; i++) {
            pp = atan2(ydat[i]-y0,xdat[i]-x0)*180.0/PI;
            x = xdat[i] - xmean;
            y = ydat[i] - ymean;
            rr = aa*x*x + 2*bb*x*y + cc*y*y + dd*x + ee*y;
            printf("%d %g %g\n",i+1,(pp<0?pp+360:pp),rr);
        }
#endif    
    }

    if (outstr) {
       if (delta<0) error("Can't compute output hyperbola table yet");
       bbb = r*ab;        /* b = semi minor axis  */
       aaa = 1-r*r;            /* eps^2 */
       for (i=0; i<=360; i++) {
           cospp = cos(PI*i/180.0);
           rr = bbb/sqrt(1-aaa*cospp*cospp);
           xp = x0 + rr*cos(PI*(i+pa)/180.0);
           yp = y0 + rr*sin(PI*(i+pa)/180.0);
           fprintf(outstr,"%d %g %g\n", i+1, xp, yp);
       }
    }
}

do_imageshift()
{
    real *x, *y, *u, *v;
    /* this code is on 3b1 ... which we've lot by now */
}

/*
 * PLANE:       y = b_0 + b_1 * x_1 + b_2 * x_2 + ... + b_n * x_n
 *
 *      used:   n = dimensionality of space in which hyper plane is fit
 */
 
do_plane()
{
    my_poly(FALSE);
}
/*
 * POLYNOMIAL:  y = b_0 + b_1 * x^1 + b_2 * x^2 + ... + b_n * x^n
 *
 *      used:   n = order of polynomial
 */
 
do_poly()
{
    my_poly(TRUE);
}

my_poly(bool Qpoly)
{
    real mat[(MAXCOL+1)*(MAXCOL+1)], vec[MAXCOL+1], sol[MAXCOL+1], a[MAXCOL+2];
    int i, j;

    if (nycol<1) error("Need 1 value for ycol=");
    if (nxcol<order && !Qpoly) error("Need %d value(s) for xcol=",order);

    lsq_zero(order+1, mat, vec);
    for (i=0; i<npt; i++) {
        a[0] = 1.0;
        for (j=0; j<order; j++) {
            if (Qpoly)
                a[j+1] = a[j] * xcol[0].dat[i];     /* polynomial */
            else
                a[j+1] = xcol[j].dat[i];            /* plane */
        }
        a[order+1] = ycol[0].dat[i];
        lsq_accum(order+1,mat,vec,a,1.0);
    }
    if (order==0) printf("TEST = %g %g\n",mat[0], vec[0]);
    lsq_solve(order+1,mat,vec,sol);
    printf("%s fit of order %d:\n", Qpoly ? "Polynomial" : "Planar" , order);
    for (j=0; j<=order; j++) printf("%g ",sol[j]);
    printf("\n");
}


/* fit a peak, 
 * for now this is the my_poly() code, though forced with order=2 
 * first find the peak, then take the two points on either side
 * to find an exact solution
 */

do_peak()
{
    real mat[(MAXCOL+1)*(MAXCOL+1)], vec[MAXCOL+1], sol[MAXCOL+1], a[MAXCOL+2];
    real *x, *y, ymax;
    int i, j, k, range=1;

    order = 2;

    if (nycol<1) error("Need 1 value for ycol=");

    x = xcol[0].dat;
    y = ycol[0].dat;
    for (i=1, j=0; i<npt; i++)			/* find the peak, at j */
        if (y[j] < y[i]) j = i;
    if (j==0 || j==npt-1) {			/* handle edge cases */
        warning("Peak at the edge");
        printf("%g %g\n",x[j],y[j]);
        return;
    }
    if (range==2) {
    	if (j==1 || j==npt-2) {
    	    warning("Peak too close to edge");
    	    return;
    	}
    }

    lsq_zero(order+1, mat, vec);
    for (i=j-range; i<=j+range; i++) {
        a[0] = 1.0;
        for (k=0; k<order; k++) {
            a[k+1] = a[k] * (x[i]-x[j]);
        }
        a[order+1] = y[i];
        lsq_accum(order+1,mat,vec,a,1.0);
    }
    lsq_solve(order+1,mat,vec,sol);
    dprintf(1,"Poly2 fit near j=%d (%g,%g)\n",j+1,x[j],y[j]);
    printf("Peak:x,y= %g %g\n",
            x[j] - sol[1]/(2*sol[2]),
            sol[0]+sqr(sol[1])/(2*sol[2]));
}


myfit(x,y,npt,dy,mwt,b,a,sigb,siga,chi2,q)    /* NumRec emulator - no chi^2 */
real *x, *y, *dy;
int npt, mwt;
real *b, *a, *siga, *sigb, *chi2, *q;
{
    real mat[4], vec[2], sol[2], aa[3];
    int i;

    dprintf(0,"local fit\n");
    lsq_zero(2,mat,vec);
    for (i=0; i<npt; i++) {
        aa[0] = 1.0;       /* mat */
        aa[1] = x[i];
        aa[2] = y[i];       /* rsh */
        lsq_accum(2, mat,  vec, aa, 1.0);    /* 1.0 -> dy */
    }
    dprintf(0,"fit: mat = %f %f %f %f\n", mat[0], mat[1], mat[2], mat[3]);
    dprintf(0,"fit: vec = %f %f\n", vec[0], vec[1]);
    lsq_solve(2, mat, vec, sol);
    *a = sol[1];
    *b = sol[0];
}

/* NR version of fit(), using gamma functions */

extern real gammq(real a, real x);

void fit(x,y,ndata,sig,mwt,a,b,siga,sigb,chi2,q)
real x[],y[],sig[],*a,*b,*siga,*sigb,*chi2,*q;
int ndata,mwt;
{
        int i;
        real wt,t,sxoss,sx=0.0,sy=0.0,st2=0.0,ss,sigdat;

        *b=0.0;
        if (mwt) {
                ss=0.0;
                for (i=0;i<ndata;i++) {
                        wt=1.0/sqr(sig[i]);
                        ss += wt;
                        sx += x[i]*wt;
                        sy += y[i]*wt;
                }
        } else {
                for (i=0;i<ndata;i++) {
                        sx += x[i];
                        sy += y[i];
                }
                ss=ndata;
        }
        sxoss=sx/ss;
        if (mwt) {
                for (i=0;i<ndata;i++) {
                        t=(x[i]-sxoss)/sig[i];
                        st2 += t*t;
                        *b += t*y[i]/sig[i];
                }
        } else {
                for (i=0;i<ndata;i++) {
                        t=x[i]-sxoss;
                        st2 += t*t;
                        *b += t*y[i];
                }
        }
        *b /= st2;
        *a=(sy-sx*(*b))/ss;
        *siga=sqrt((1.0+sx*sx/(ss*st2))/ss);
        *sigb=sqrt(1.0/st2);
        *chi2=0.0;
        if (mwt == 0) {
                for (i=0;i<ndata;i++)
                        *chi2 += sqr(y[i]-(*a)-(*b)*x[i]);
                *q=1.0;
                sigdat=sqrt((*chi2)/(ndata-2));
                *siga *= sigdat;
                *sigb *= sigdat;
        } else {
                for (i=0;i<ndata;i++)
                        *chi2 += sqr((y[i]-(*a)-(*b)*x[i])/sig[i]);
                *q=gammq(0.5*(ndata-2),0.5*(*chi2));	
        }
}

