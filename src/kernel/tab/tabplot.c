/*
 * TABPLOT: a simple general table plotter
 *			for ascii data in tabular format
 *
 *	25-nov-88  V1.0 :  created by P.J.Teuben
 *	 5-jul-89      a:  nemoinp to nemoinp(d/i)
 *	13-nov-90  V1.1 :  helpvec, get_atable()
 *      26-mar-92  V1.2 :  added nmax=
 *       8-apr-92  V1.3 :  skip plotting of no points found, usage;
 *			   no more 'nskip'; error= -> errors=
 *	24-apr-92  V1.4 :  repaired xbin=  - PJT
 *	 5-nov-93      a:  get_atable: warn about partial pipe
 *	24-jan-95  V1.5 :  support for reading multiple columns (ycol=)
 *                     a:  added xcoord= and ycoord=
 *      13-feb-96  V1.6 :  added cursor= keyword to get positions back
 *                         into this file (only works with PGPLOT)
 *			   and added layout= keyword
 *      16-aug-96      b : increased max # Y-columns from 16 to 64
 *	14-sep-96      c : bug in reporting ycol
 *	16-feb-97      d : support for SINGLEPREC
 *	20-feb-97  V1.7  : removed never used skip= keyword, MAXYCOL now 256
 *       1-apr-97      a : options pl_cursor compilation
 *	10-oct-97      b : minor debug output change
 *	 7-may-98  V1.8  : added median option for binning
 *	13-jul-98      a : printf->dprintf for fixed binning
 *      25-jul-98      b : allow line= with histogram-type (ltype<0)
 *      31-mar-99  V2.1  : allow autoscaling with a range set in the
 *                         other coordinate
 *	28-jul-99  V2.2  : add color=
 *      21-jul-00  V2.3  : allow autoscaling on half (min OR max) an axis
 *
 */

/**************** INCLUDE FILES ********************************/ 

#include <stdinc.h>	
#include <getparam.h>
#include <yapp.h>
#include <axis.h>
#include <layout.h>

                    /* undefined values trick !!!  MACHINE DEP  !!! */
#ifdef SINGLEPREC
#define NaN 0x7FFF
#else
#define NaN 0x7FFFFFFF
#endif

#define MAXYCOL 256
#define MAXCOORD 16

/**************** COMMAND LINE PARAMETERS **********************/

string defv[] = {                /* DEFAULT INPUT PARAMETERS */
    "in=???\n            Input file name (table)",
    "xcol=1\n		 x coordinate column",
    "ycol=2\n		 y coordinate column",
    "xmin=\n		 X-min value if not autoscaled",
    "xmax=\n		 X-max value if not autoscaled",
    "ymin=\n		 Y-min value if not autoscaled",
    "ymax=\n		 Y-max value if not autoscaled",
    "xbin=\n             binning data in X?",
    "point=1,0.1\n       point type and size pairs for each Y column (yapp)",
    "line=0,0\n          line width and style pairs for each Y column (yapp)",
    "color=\n		 colors for the symbols/lines for each Y column",
    "errors=\n           X-Y error bars ?",
    "xlab=\n		 Label along X-axis",
    "ylab=\n		 Label along Y-axis",
    "xcoord=\n		 Draw additional vertical coordinate lines along these X values",
    "ycoord=\n		 Draw additional horizontal coordinate lines along these Y values",
    "nxticks=7\n         Number of ticks on X axis",
    "nyticks=7\n         Number of ticks on Y axis",
    "nmax=100000\n       Hardcoded allocation space, if needed for piped data",
    "median=f\n          Use median in computing binning averages?",
    "headline=\n	 headline in graph on top right",
    "tab=f\n             Table output also if binning is used?",
    "fullscale=f\n       Use full autoscale in one axis if other autoscaled?",
    "cursor=\n           Optional output file to retrieve cursor coordinates",
    "layout=\n           Optional input layout file",
    "VERSION=2.3\n	 21-jul-00 PJT",
    NULL
};

string usage = "general table plotter";

/**************** GLOBAL VARIABLES ************************/

local string input;				/* filename */
local stream instr;				/* input file */

local int xcol, ycol[MAXYCOL], nycol;		/* column numbers */
local real xrange[2], yrange[2];		/* range of values */

local real  *x, *y[MAXYCOL]; 			/* data from file */
local real *xp, *yp, *xps, *yps;              /* rebinned data (if needed) */
local int    npt;				/* actual number of data points */
local int    np;                              /* actual number of rebinned data */

local real xmin, xmax;			/* actual min and max as computed */
local real ymin, ymax;
local bool   Qautox0, Qautoy0, Qfull;		/* autoscale ? */
local bool   Qautox1, Qautoy1;
local bool   Qsigx, Qsigy;                    /* show error bars on data points ? */
local bool   Qtab;				/* table output ? */
local bool   Qmedian;
local real *xbin;                             /* boundaries for binned data */
local int    nbin;                            /* number of boundaries " */

local int    nmax;				/* lines to allocate */

local string headline;			/* text string above plot */
local string xlab, ylab;			/* text string along axes */
local real xplot[2],yplot[2];		        /* borders of plot */
local int    yapp_line[2*MAXYCOL];            /* thickness and pattern of line */
local int    yapp_color[MAXYCOL];	      /* color per column */
local real yapp_point[2*MAXYCOL];             /* what sort of point to plot */
local string errorbars;                       /* x or y or both (xy) */
local real xcoord[MAXCOORD], ycoord[MAXCOORD];/* coordinate lines */
local int nxcoord, nycoord;
local int nxticks, nyticks;                   /*& number of tickmarks */
local int ncolors;

local plcommand *layout;

void setparams(), plot_data();

extern real median(int, real *);

local real  xtrans(real),  ytrans(real);    /* WC -> cmXY */
local real ixtrans(real), iytrans(real);    /* cmXY -> WC */

/****************************** START OF PROGRAM **********************/

nemo_main()
{
    setparams();

    instr = stropen (input,"r");
    read_data();
    plot_data();
}

void setparams()
{
    char *smin,*smax;
    int  i, j;
   
    input = getparam("in");             /* input table file */
    xcol = getiparam("xcol");           /* x column */
    nycol = nemoinpi(getparam("ycol"),ycol,MAXYCOL);
    if (nycol < 1) error("Error parsing ycol=%s",getparam("ycol"));

    nmax = file_lines(input,getiparam("nmax"));
    dprintf(0,"Allocated %d lines for table\n",nmax);
    x = (real *) allocate(sizeof(real) * (nmax+1));          /* X data */
    for (j=0; j<nycol; j++)
        y[j] = (real *) allocate(sizeof(real) * (nmax+1));   /* Y data */

    Qsigx = Qsigy = FALSE;
    if (hasvalue("xbin")) {
	smin = getparam("xbin");            /* binning of data */
        Qsigx = Qsigy = TRUE;           /* binning of data */
        xbin = (real *) allocate(nmax*sizeof(real));
        nbin = nemoinpr(smin,xbin,nmax);   /* get binning boundaries */
        if (nbin==1) {              /*  fixed amount of datapoints to bin */
            (void) nemoinpi(smin,&nbin,1);
            dprintf(0,"Binning with fixed number (%d) of points\n",nbin);
            np = nmax / nbin + 1;
            nbin = -nbin;       /* make it <0 to trigger rebin_data */
        } else if (nbin>1) {
            np = nbin - 1;
            if (np==1)
                warning("plotting one averaged datapoint");
        } else
            error("Error %d in parsing xbin=%s",nbin,smin);

        xp =  (real *) allocate(np*sizeof(real));    /* X data */
        yp =  (real *) allocate(np*sizeof(real));    /* Y data */
        xps = (real *) allocate(np*sizeof(real));   /* sig-X data */
        yps = (real *) allocate(np*sizeof(real));   /* sig-Y data */

        for (i=0; i<np; i++)
            xp[i] = xps[i] = yp[i] = yps[i] = NaN;      /* init empty */
    } else
        np = 0;
    
    if (hasvalue("xmin") && hasvalue("xmax")) {
	xrange[0] = getdparam("xmin");
	xrange[1] = getdparam("xmax");
	Qautox0=Qautox1=FALSE;
	dprintf (2,"fixed plotrange in X: %f : %f\n",xrange[0],xrange[1]);
    } else if (hasvalue("xmin")) {
	xrange[0] = getdparam("xmin");
	Qautox0=FALSE;
	Qautox1=TRUE;
	dprintf (2,"fixed minimum in X: %f\n",xrange[0]);
    } else if (hasvalue("xmax")) {
	xrange[1] = getdparam("xmax");
	Qautox0=TRUE;
	Qautox1=FALSE;
	dprintf (2,"fixed maximum in X: %f\n",xrange[1]);
    } else {
    	xrange[0] = -HUGE;
    	xrange[1] = HUGE;
	Qautox0=Qautox1=TRUE;
	dprintf (2,"auto plotscaling\n");
    }

    if (hasvalue("ymin") && hasvalue("ymax")) {
	yrange[0] = getdparam("ymin");
	yrange[1] = getdparam("ymax");
	Qautoy0=Qautoy1=FALSE;
	dprintf (2,"fixed plotrange in Y: %f : %f\n",yrange[0],yrange[1]);
    } else if (hasvalue("ymin")) {
	yrange[0] = getdparam("ymin");
	Qautoy0=FALSE;
	Qautoy1=TRUE;
	dprintf (2,"fixed minimum in Y: %f\n",yrange[0]);
    } else if (hasvalue("ymax")) {
	yrange[1] = getdparam("ymax");
	Qautoy0=TRUE;
	Qautoy1=FALSE;
	dprintf (2,"fixed maximum in Y: %f\n",yrange[1]);
    } else {
    	yrange[0] = -HUGE;
    	yrange[1] = HUGE;
	Qautoy0=Qautoy1=TRUE;
	dprintf (2,"auto plotscaling\n");
    }

    Qfull = getbparam("fullscale");
    parse_pairsi("line", yapp_line, nycol);
    parse_pairsr("point",yapp_point,nycol);
    ncolors = nemoinpi(getparam("color"),yapp_color,nycol);
    if (ncolors > 0) {
    	for (i=ncolors; i<nycol; i++)
    	   yapp_color[i] = yapp_color[i-1];
    }
       
    smin = getparam("errors");               /* force error bars in graph ? */
    if (*smin) {
        Qsigx = (strchr(smin,'x') != NULL);	/* error bar in X needed ?*/
        Qsigy = (strchr(smin,'y') != NULL);	/* error bar in Y needed ?*/
    }
    Qtab = getbparam("tab");
    Qmedian = getbparam("median");
    xlab=getparam("xlab");
    ylab=getparam("ylab");
    headline = getparam("headline");
    nxcoord = nemoinpr(getparam("xcoord"),xcoord,MAXCOORD);
    nycoord = nemoinpr(getparam("ycoord"),ycoord,MAXCOORD);
    nxticks = getiparam("nxticks");
    nyticks = getiparam("nyticks");
    if (hasvalue("layout"))
        layout = pl_fread(getparam("layout"));
    else
        layout = NULL;
    
}

#define MVAL 		 64
#define MLINELEN	512

read_data()
{
    real *coldat[1+MAXYCOL];
    int i, j, colnr[1+MAXYCOL];
		
    dprintf (2,"Reading datafile, xcol,ycol=%d,%d,...\n",xcol,ycol[0]);
    colnr[0] = xcol;
    coldat[0] = x;
    for (j=0; j<nycol; j++) {
        colnr[1+j] = ycol[j];
        coldat[1+j] = y[j];
    }
    npt = get_atable(instr,1+nycol,colnr,coldat,nmax);    /* get data */
    if (npt < 0) {
    	npt = -npt;
    	warning("Could only read first set of %d data",npt);
    }

    xmin = ymin =  HUGE;
    xmax = ymax = -HUGE;
    for (i=0; i<npt; i++) {     /* find global min and max in X and all Y */
        xmax=MAX(x[i],xmax);
        xmin=MIN(x[i],xmin);
        for (j=0; j<nycol; j++) {
	    ymax=MAX(y[j][i],ymax);
	    ymin=MIN(y[j][i],ymin);
        }
    }
}


void plot_data()
{
    int    i, j, k;
    real xcur, ycur, edge;
    char c;
    stream cstr;
    bool first;

    if (npt<1) {
        warning("Nothing to plot, npt=%d",npt);
        return;
    }
    dprintf (0,"read %d points\n",npt);
    dprintf (0,"min and max value in xcolumn  %d: [%f : %f]\n",xcol,xmin,xmax);
    dprintf (0,"min and max value in ycolumns %s: [%f : %f]\n",
			getparam("ycol"),ymin,ymax);
    if (Qautox0 && Qautox1) {
	    edge = (xmax-xmin) * 0.05;        /* add 5% to the edges */
	    xmin -= edge;
	    xmax += edge;
    } else if (Qautox0) {
	    xmax = xrange[1];
            edge = (xmax-xmin) * 0.05;        /* add 5% to the edge */
            xmin -= edge;
    } else if (Qautox1) {
	    xmin = xrange[0];
            edge = (xmax-xmin) * 0.05;        /* add 5% to the edge */
            xmax += edge;
    } else {
	    xmin = xrange[0];
	    xmax = xrange[1];
    }
    if (Qautox0 || Qautox1) 
        dprintf(0,"X:min and max value reset to : [%f : %f]\n",xmin,xmax);

    if (Qautoy0 && Qautoy1) {
	    edge = (ymax-ymin) * 0.05;        /* add 5% to the edges */
	    ymin -= edge;
	    ymax += edge;
    } else if (Qautoy0) {
	    ymax = yrange[1];
            edge = (ymax-ymin) * 0.05;        /* add 5% to the edge */
            ymin -= edge;
    } else if (Qautoy1) {
	    ymin = yrange[0];
            edge = (ymax-ymin) * 0.05;        /* add 5% to the edge */
            ymax += edge;
    } else {
	    ymin = yrange[0];
	    ymax = yrange[1];
    }
    if (Qautoy0 || Qautoy1)
        dprintf(0,"Y:min and max value reset to : [%f : %f]\n",ymin,ymax);

    /*
     *  if scale in X fully fixed, and some autoscaling in Y is done
     *  we can recompute the Ymin and/or Ymax based on fixed Xmin/Xmax
     */

    if (!Qfull && (Qautoy0 || Qautoy1) && !Qautox0 && !Qautox1) {
        first = TRUE;
        for (i=0; i<npt; i++) {     /* go through data, find Y min and max again */
	    if (xmin < xmax && (x[i] < xmin || x[i] > xmax)) continue;
	    if (xmin > xmax && (x[i] > xmin || x[i] < xmax)) continue;
            if (first) {
                if (Qautoy0) ymin = y[0][i];
                if (Qautoy1) ymax = y[0][i];
                first = FALSE;
            }
            for (j=0; j<nycol; j++) {
	        if (Qautoy0) ymin=MIN(y[j][i],ymin);
    	        if (Qautoy1) ymax=MAX(y[j][i],ymax);
            }
        }
        dprintf(0,"Y:min and max value re-reset to : [%f : %f]\n",ymin,ymax);
    }

    /*
     * now the same, but reversed axes...
     */

    if (!Qfull && (Qautox0 || Qautox1) && !Qautoy0 && !Qautoy1) {
        first = TRUE;
        for (i=0; i<npt; i++) {     /* go through data, find X min and max again */
            for (j=0; j<nycol; j++) {
                if (ymin < ymax && (y[j][i]<ymin || y[j][i]>ymax)) break;
                if (ymin > ymax && (y[j][i]>ymin || y[j][i]<ymax)) break;
            }
            if (j<nycol) continue;

            if (first) {
                if (Qautox0) xmin = x[i];
                if (Qautox1) xmax = x[i];
                first = FALSE;
            }
            if (Qautox0) xmin=MIN(x[i],xmin);
            if (Qautox1) xmax=MAX(x[i],xmax);
        }
        dprintf(0,"X:min and max value re-reset to : [%f : %f]\n",xmin,xmax);
    }
    
	/*	PLOTTING */	
    plinit("***",0.0,20.0,0.0,20.0);

    xplot[0] = xmin;        /* set scales for xtrans() */
    xplot[1] = xmax;
    yplot[0] = ymin;        /* set scales for ytrans() */
    yplot[1] = ymax;
    xaxis ( 2.0,  2.0, 16.0, xplot, -nxticks, xtrans, xlab);
    xaxis ( 2.0, 18.0, 16.0, xplot, -nxticks, xtrans, NULL);
    yaxis ( 2.0,  2.0, 16.0, yplot, -nyticks, ytrans, ylab);
    yaxis (18.0,  2.0, 16.0, yplot, -nyticks, ytrans, NULL);
    for (i=0; i<nxcoord; i++) {
        plmove(xtrans(xcoord[i]),ytrans(yplot[0]));
        plline(xtrans(xcoord[i]),ytrans(yplot[1]));
    }
    for (i=0; i<nycoord; i++) {
        plmove(xtrans(xplot[0]),ytrans(ycoord[i]));
        plline(xtrans(xplot[1]),ytrans(ycoord[i]));
    }
    

    pljust(-1);     /* set to left just */
    pltext(input,2.0,18.2,0.32,0.0);             /* filename */
    pljust(1);
    pltext(headline,18.0,18.2,0.24,0.0);         /* headline */
    pljust(-1);     /* return to left just */

    for (j=0; j<nycol; j++) {
        if (np) rebin_data (npt,x,y[j], nbin, xbin, np, xp, yp,  xps, yps);

        plot_points( npt, x, y[j], xps, yps,
            yapp_point[2*j], yapp_point[2*j+1],
            yapp_line[2*j], yapp_line[2*j+1],
            ncolors > 0 ? yapp_color[j] : -1,
            Qsigx & (Qsigy << 1) );
    }

    if (hasvalue("cursor")) {
        dprintf(0,"Interactive mode: enter coordinates with the cursor\n");
        cstr = NULL;
        while (pl_cursor(&xcur, &ycur, &c)) {
            dprintf(0,"%c: %g %g cm (%g %g)\n",
                c,xcur,ycur,ixtrans(xcur),iytrans(ycur));
            if (c=='X') break;
            if (c=='A') {
                if (cstr == NULL) cstr = stropen(getparam("cursor"),"w");
                fprintf(cstr,"%g %g\n",ixtrans(xcur),iytrans(ycur));
            }
        }
        if (cstr) strclose(cstr);
    }
    if (layout) pl_exec(layout);

    plstop();
}

/*
 *  REBIN_DATA:
 *
 *      nbin > 0:   array xbin[0..nbin-1] countaints bin boundaries for x
 *      nbin < 0:   bin x data in groups of (-nbin) points into (xp,yp)
 *  
 *      The arrays (xp,yp) contain resulting data, (xps,yps) the dispersion,
 *      which can be used for plotting error bars
 *
 *      It is assumed that the X-data are sorted
 */
 
rebin_data (n,x,y, nbin,xbin, np, xp, yp,  xps, yps)
int n, nbin, np;
real *x, *y, *xbin, *xp, *yp, *xps, *yps;
{
    int    i, j, ip, iold=0, zbin=0;
    real x0, x1, x2, y1, y2;

    dprintf(0,"Rebinning...n=%d nbin=%d np=%d\n",n,nbin,np);
    
    for (i=0; i<np; i++)
        xp[i] = xps[i] = yp[i] = yps[i] = NaN;      /* init (again) */
   
    for (i=0, ip=0; i<n-1; i++)		/* loop over all points */
        if (x[i] < x[i-1])
            ip++;              /* count unsorted data in 'ip' */
    if (ip>0)
        warning("%d out of %d datapoints are not sorted\n", ip, n);

    i = 0;                     /* point to original data (x,y) to be rebinned */
    iold = 0;
    for (ip=0; ip<nbin-1; ip++) {		/* for each bin, accumulate */
        x0 = x1 = x2 = y1 = y2 = 0.0;       /* reset */
        while (i<n && x[i] >= xbin[ip] && x[i] < xbin[ip+1]) {
            x0 += 1.0;
            x1 += x[i];
            x2 += sqr(x[i]);
            y1 += y[i];
            y2 += sqr(y[i]);
            i++;
        }
        if (x0==1.0) {
            xp[ip] = x1;  xps[ip] = 0.0;
            yp[ip] = y1;  yps[ip] = 0.0;
        } else if (x0 > 1.0) {
            xp[ip] = x1/x0;
            yp[ip] = y1/x0;
            xps[ip] = sqrt(x2/x0 - sqr(xp[ip]));
            yps[ip] = sqrt(y2/x0 - sqr(yp[ip]));
            if (Qmedian) {
                xp[ip] = median(i-iold, &x[iold]);
                yp[ip] = median(i-iold, &y[iold]);
            }
        } else
            zbin++;     /* count bins with no data */
	if(Qtab && x0>0)
            printf("%g %g %g %g %d\n",xp[ip],yp[ip],xps[ip],yps[ip],i-iold);
        iold = i;
    } /* for(ip) */
    if(zbin)warning("There were %d bins with no data",zbin);
    if (nbin>0)
        return(0);      /* done with variable bins */

    nbin = -nbin;       /* make it positive for fixed binning */
    for (i=0, ip=0; i<n;ip++) {       /* loop over all points */
        x0 = x1 = x2 = y1 = y2 = 0.0;       /* reset accums */
        for(j=0; j<nbin && i<n; j++, i++) {   /* accum the new bin */
            x0 += 1.0;
            x1 += x[i];
            x2 += sqr(x[i]);
            y1 += y[i];
            y2 += sqr(y[i]);
        }
        xp[ip] = x1/x0;
        yp[ip] = y1/x0;
        xps[ip] = sqrt(x2/x0 - sqr(xp[ip]));
        yps[ip] = sqrt(y2/x0 - sqr(yp[ip]));
	if(Qtab)
            printf("%g %g %g %g %d\n",xp[ip],yp[ip],xps[ip],yps[ip],i-iold);
        iold = i;
    }
}

/* PLOT_POINTS
 *
 *      Plot datapoints, optionally connecting them with lines,
 *      adding errorbars in X as well as Y
 *
 *      X,Y Points with value NaN are skipped, as well as their errorbars
 */
 
plot_points (np, xp, yp, xps, yps, pstyle, psize, lwidth, lstyle, color, errorbars)
int np, lwidth, lstyle, color, errorbars;
real xp[], yp[], xps[], yps[], pstyle, psize;
{
    int i, ipstyle;
    real p1, p2;

    if (color >= 0) {
        plcolor(color);
    }
    
    ipstyle = pstyle+0.1;
    if (ipstyle != 0)
        for (i=0; i<np; i++)
            if (xp[i] != NaN && yp[i] != NaN)
                switch (ipstyle) {
                case 1:
                    plpoint (xtrans(xp[i]), ytrans(yp[i]));
                    break;
                case 2:
                    plcircle (xtrans(xp[i]), ytrans(yp[i]),psize);
                    break;
                case 3:
                    plcross (xtrans(xp[i]), ytrans(yp[i]),psize);
                    break;
                case 4:
                    plbox (xtrans(xp[i]), ytrans(yp[i]),psize);
                    break;
                default:
                    error ("Invalid pstyle = %d\n",pstyle);
                }


    if (lwidth > 0) {
        plltype(lwidth,ABS(lstyle));
        i=0;
        while (xp[i] == NaN || yp[i] == NaN)
            i++;                             /* skip first undefined */
	if (lstyle > 0) {       /* connect the dots */
            plmove (xtrans(xp[i]), ytrans(yp[i]));      /* move to point */
            while (++i < np)
                if (xp[i] != NaN)
                    plline (xtrans(xp[i]), ytrans(yp[i]));  /* draw line */
        } else {                /* histogram approach */
            plmove (xtrans(xp[i]), ytrans(yp[i]));      /* move to 1st point */
            while (++i < np) {
                plline(xtrans(0.5*(xp[i-1]+xp[i])),ytrans(yp[i-1]));
                plline(xtrans(0.5*(xp[i-1]+xp[i])),ytrans(yp[i]));
                plline(xtrans(xp[i]),ytrans(yp[i]));
            }            
        }
        plltype(1,1);
    }
    
    if (errorbars && 0x0001) {
        for (i=0; i<np; i++) {
            if (xps[i] == NaN || xp[i] == NaN)
                continue;
            p1 = xp[i] - xps[i];
            p2 = xp[i] + xps[i];
            plmove (xtrans(p1), ytrans(yp[i]));
            plline (xtrans(p2), ytrans(yp[i]));
        }
    }
    if (errorbars && 0x0002) {
        for (i=0; i<np; i++) {
            if (yps[i] == NaN || yp[i] == NaN)
                continue;
            p1 = yp[i] - yps[i];
            p2 = yp[i] + yps[i];
            plmove (xtrans(xp[i]), ytrans(p1));
            plline (xtrans(xp[i]), ytrans(p2));
        }
    }
}

local real xtrans(real x)
{
    return (2.0 + 16.0*(x-xplot[0])/(xplot[1]-xplot[0]));
}

local real ytrans(real y)
{
    return (2.0 + 16.0*(y-yplot[0])/(yplot[1]-yplot[0]));
}

local real ixtrans(real x)
{
        return xplot[0] + (x-2.0)*(xplot[1]-xplot[0])/16.0;
}
 
local real iytrans(real y)
{
        return yplot[0] + (y-2.0)*(yplot[1]-yplot[0])/16.0;
        
}       



parse_pairsi(key, pairs, nycol)
string key;
int *pairs;
int nycol;
{
    int n, i;

    n = nemoinpi(getparam(key),pairs,2*nycol);
    if (n>=0 && n<2) error("%s= needs at least 2 values",key);
    if (n<0) error("Parsing error %s=%s",key,getparam(key));
    for (i=n; i<2*nycol; i++) pairs[i] = pairs[i-2];

}

parse_pairsr(key, pairs, nycol)
string key;
real *pairs;
int nycol;
{
    int n, i;
    real r1, r2;

    n = nemoinpr(getparam(key),pairs,2*nycol);
    if (n>=0 && n<2) error("%s= needs at least 2 values",key);
    if (n<0) error("Parsing error %s=%s",key,getparam(key));
    for (i=n; i<2*nycol; i++) pairs[i] = pairs[i-2];
}


/*
 *  YAPP routine for PGPLOT
 */
#if 0
    
int pl_cursor(real *x, real *y, char *c)
{
    char inf[8], ans[8];
    int len, inf_len, ans_len;
    permanent float xsave, ysave;
    
    strcpy(inf,"CURSOR");
    inf_len = strlen(inf);
    ans_len = 1;
    pgqinf_(inf, ans, &len, inf_len, ans_len);
    if (ans[0]=='n' || ans[0]=='N') return 0; 
    pgcurs_(&xsave, &ysave, c, 1);
    *x = xsave;
    *y = ysave;
    return 1;
}
#endif  

