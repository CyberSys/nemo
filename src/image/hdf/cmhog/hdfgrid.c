/*
 *  HDFGRID:  re-grid a CMHOG half-plane polar grid to full-plane cartesian
 *            grid.
 *            Output is in image(5NEMO) format, and ccdfits(1NEMO), and
 *            fitstiff(1NEMO) could be used to create 8bit dump files 
 *            for further movie processing
 *            The interpolation scheme is the same as used in cmhog/movie.src
 *            except for handling points near the Y axis to make optimum
 *            use of the (odd/even) symmetry properties.
 *
 *      23-may-95	V1.0  quicky for IAU157 movie			PJT
 *                            Can only do VR(1), VT(2), or DEN(3)
 *	   nov-95	V1.1  added VX and VY gridding			pjt
 *      15-jun-96       V1.1a fixed bug in vx/vy gridding near Y axis   pjt
 *	 1-aug-96       V1.2  add odd/even option
 *	12-dec-99	V1.3b  optional shift the theta (2nd index) array PJT
 *      13-dec-99       V1.4  merged back the old 1.3 change to tag the time from
 *                            TIME= labels                               pjt
 *      19-feb-00       V1.4b fixed header coordinates expression parser pjt
 *      10-sep-00           c fixed bug in the previous fix              pjt
 */

 
#include <stdinc.h>
#include <getparam.h>
#include <vectmath.h>

#include <filestruct.h>
#include <history.h>
#include <image.h>

#ifdef INC_HDF
#include <hdf.h> 	/* some conflicts with nemo include files */
#endif

string defv[] = {
    "in=???\n			Input file (HDF SD)",
    "out=???\n                  Output image file",
    "select=1\n			Select which SDS from the file (1=first)",
    "nx=256\n			Number of pixels in X",
    "ny=256\n			Number of pixels in Y",
    "xrange=-16:16\n		Range in X",
    "yrange=-16:16\n		Range in Y",
    "zvar=\n                    Optional selections: {vr,vt,den,vx,vy}",
    "symmetry=\n		Override symmetry properties (odd|even)",
    "it0=0\n			Shift THETA array (i.e. rotate grid)",
    "VERSION=1.4c\n		10-sep-00 PJT",
    NULL,
};

string usage="Regrid a CMHOG polar HDF SDS image to a cartesian NEMO image";

#define MAXRANK 2


local int rank, shape[MAXRANK], run[MAXRANK];
local char label[256], unit[256], fmt[256], coordsys[256];
local int it0;

local void fshift(int n, float *a, int i0);
local void setrange(real *rval, string rexp, int nr);
local real string_real(string,string);

extern string *burststring(string, string);

void nemo_main()
{
    float **image, **coord, *buffer, *rads, *phis, rad, phi, phi_orig, 
         dmin, dmax, phi1, phi2, rad1, rad2;
    float *buffer1, *buffer2, *buffer3;
    float **image1, **image2;
    int i, j, k, ret, size, type, nsds, isel, nx, ny;
    int ir, ip, jr, jp, nr, np, n1, n2, is;
    char **output, *cbuff;
    char ntype[32];
    string filter, zvar, infile = getparam("in");
    stream outstr;
    real xrange[3], yrange[3], cosp, sinp;
    real x, y, a1, a2, c1,c2,c3,c4, cmin, cmax, dcon, tmp, vr, vt;
    real sds_time = -1.0;
    imageptr iptr;
    bool mirror, first = TRUE, both = FALSE, flip=FALSE;

    it0 = getiparam("it0");
    nsds = DFSDndatasets(infile);
    if (nsds<0) 
        error("%s is probably not an HDF scientific dataset",infile);
    dprintf(1,"Found %d scientific data set%s in %s\n",
            nsds,(nsds > 1 ? "s" : ""),infile);
    if (hasvalue("zvar")) {
        zvar = getparam("zvar");
        dprintf(0,"Polar CMHOG Gridding variable: %s\n",zvar);
        if (streq(zvar,"vr"))
            isel = 1;
        else if (streq(zvar,"vt"))
            isel = 2;
        else if (streq(zvar,"den"))
            isel = 3;
        else if (streq(zvar,"vx") || streq(zvar,"vy")) {
            both = TRUE;
            flip = TRUE;
            isel = 1;   /* !! will need to read 1 + 2 !! */
        } else
            error("Unsupported gridding output variable %s",zvar);
    } else
        isel = getiparam("select");
    if (isel>nsds || isel<1) error("Illegal isel, must be 1..%d",nsds);
    
    for (k=0; k<nsds; k++) {        /* read until we have the right SDS */
    	ret = DFSDgetdims(infile,&rank, shape, MAXRANK);
    	if (ret < 0) error("Problem getting rank/shape at SDS #%d",k+1);

    	label[0] = unit[0] = fmt[0] = coordsys[0] = 0;
        ret = DFSDgetdatastrs(label, unit, fmt, coordsys);
        ret = DFSDgetNT(&type);

        if (sds_time < 0)       /* search for: "AT TIME=" */
            sds_time = string_real(label,"AT TIME=");

    	if (k==0) {				/* first time: allocate */
            coord = (float **) allocate(rank*sizeof(float *));
            for (i=0, size=1; i<rank; i++) {
    	        size *= shape[i];
                coord[i] = (float *) allocate(shape[i] * sizeof(float));
                ret = DFSDgetdimscale(i+1, shape[i],coord[i]);
                if (ret<0) error("getting shape[%d]",i+1);
                dprintf(0,"Dimension %d  Size %d\n",i+1,shape[i]);
            }
            buffer1 = (float *) allocate(size * sizeof(float));
            if (both) buffer2 = (float *) allocate(size * sizeof(float));
        }
        if (k+1 == isel) {          /* if we got the right one, get data now */
            if (both && isel==2)
                buffer = buffer2;
            else
                buffer = buffer1;
            ret = DFSDgetdata(infile,rank,shape,buffer);
            dmin = dmax = buffer[0];
            for (i=1; i<size; i++) {
                dmin = MIN(dmin,buffer[i]);
                dmax = MAX(dmax,buffer[i]);
            }
            dprintf(0,"%d Datamin/max read: %g %g\n",isel,dmin,dmax);
            if (both) {
                isel++;
                if (isel>2) break;
            } else
                break;
        }
    }
    image1 = (float **) allocate(shape[0] *sizeof(float *));
    for (i=0; i<shape[0]; i++) {
    	image1[i] = &buffer1[i*shape[1]];
    }
    if (both) {
        image2 = (float **) allocate(shape[0] *sizeof(float *));
        for (i=0; i<shape[0]; i++) {
            image2[i] = &buffer2[i*shape[1]];
        }
        if (streq(zvar,"vx")) {
            for (i=0; i<shape[0]; i++) {    /* phi */
            	cosp = cos((double)coord[0][i]);
            	sinp = sin((double)coord[0][i]);
	        for (j=0; j<shape[1]; j++) {     /* rad */
                    vr = image1[i][j];
                    vt = image2[i][j];
                    image1[i][j] = vr*cosp-vt*sinp;
                }
            }
        } else if (streq(zvar,"vy")) {
            for (i=0; i<shape[0]; i++) {    /* phi */
            	cosp = cos((double)coord[0][i]);
            	sinp = sin((double)coord[0][i]);
	        for (j=0; j<shape[1]; j++) {     /* rad */
                    vr = image1[i][j];
                    vt = image2[i][j];
                    image1[i][j] = vr*sinp+vt*cosp;
                }
            }
        }
    }
    if (it0) {
        dprintf(0,"New feature: shifting theta by %d/%d pixels\n",it0,shape[0]);
    	/* nt=shape[0]  nr=shape[1] */
        buffer3 = (float *) allocate(sizeof(float)*shape[0]);
    	for (i=0; i<shape[0]; i++) dprintf(2,"before: %d %g\n",i,image1[i][0]);
	for (j=0; j<shape[1]; j++) {
            for (i=0; i<shape[0]; i++) buffer3[i] = image1[i][j];
            fshift(shape[0], buffer3, it0);
            for (i=0; i<shape[0]; i++) image1[i][j] = buffer3[i];
            if (both) {
                for (i=0; i<shape[0]; i++) buffer3[i] = image2[i][j];
                fshift(shape[0], buffer3, it0);
                for (i=0; i<shape[0]; i++) image2[i][j] = buffer3[i];
            }
    	}
    	for (i=0; i<shape[0]; i++) dprintf(2,"after: %d %g\n",i,image1[i][0]);    	
    }
    image = image1;
    /* the image can now be referred to as in:  image[phi][rad]  */

    outstr = stropen(getparam("out"),"w");
    nx = getiparam("nx");
    ny = getiparam("ny");
    setrange(xrange,getparam("xrange"),nx);
    setrange(yrange,getparam("yrange"),ny);
    nr = shape[1];
    np = shape[0];
    rads = coord[1];
    phis = coord[0];
    create_image(&iptr, nx, ny);

    n1 = n2 = 0;
    for (j=0; j<ny; j++) {                  /* loop over all rows */
        y = yrange[0] + (j+0.5)*yrange[2];
        for (i=0; i<nx; i++) {              /* loop over all columns */
            x = xrange[0] + (i+0.5)*xrange[2];
            rad = sqrt(x*x + y*y);
            phi = atan2(y,x);               /* make sure phi in -pi/2 : pi/2  */

            mirror = x < 0;
            if (mirror) {
                phi_orig = phi;
	        if (phi >  HALF_PI) phi -= PI;
	        if (phi < -HALF_PI) phi += PI;
            } 
            if (rad > rads[nr-1] || rad < rads[0]) {
                MapValue(iptr,i,j) = 0.0;
                n1++;
                continue;
            }
            for (ir=0; ir<nr-1; ir++)           /* simple search in Rad */
                if (rads[ir+1] > rad) break;
            jr = ir+1;
            rad1 = rads[ir];
            rad2 = rads[jr];
            
            if(phi<phis[0]) {                   /* special search in Phi */
		flip = TRUE;
                if (mirror) {                   /* 2nd Quadrant */
                    ip = 0;
                    jp = np-1;
                    phi1 = phis[0] + PI;
                    phi2 = phis[np-1];
                    phi = phi_orig;
                } else {                        /* 4th Quadrant */
                    ip = np-1;
                    jp = 0;
                    phi1 = phis[np-1] - PI;
                    phi2 = phis[0];
                }
            } else if (phi>phis[np-1]) {        /* because of half-symmetry */
		flip = TRUE;
                if (mirror) {                   /* 3rd Quadrant */
                    ip = np-1;
                    jp = 0;
                    phi1 = phis[np-1] - PI;
                    phi2 = phis[0];
                    phi = phi_orig;
                } else {                        /* 1st Quadrant */
                    ip = 0;
                    jp = np-1;
                    phi1 = phis[0] + PI;
                    phi2 = phis[np-1];
                }
            } else {                            /* simple in the middle */
		flip = FALSE;
                for (ip=0; ip<np-1; ip++)
                    if (phis[ip+1] > phi) break;
                jp = ip+1;
                phi1 = phis[ip];
                phi2 = phis[jp];
                if (ip>=np-1 || ir>=nr-1) {     /* should never happen */
                    warning("### Odd: %d %d: ip=%d ir=%d",i,j,ip,ir);
                    MapValue(iptr,i,j) = 0.0;
                    n2++;
                    continue;
                }
            }
            dprintf(1,"(x,y)%d %d (r,p) %g %g [@ %d %d] LL: %g %g\n",
                    i,j,rad,phi,ir,ip,rads[ir],phis[ip]);
                
            c1 = (rad-rad2)/(rad1-rad2);
            c2 = (rad-rad1)/(rad2-rad1);
            c3 = (phi-phi2)/(phi1-phi2);
            c4 = (phi-phi1)/(phi2-phi1);
            a1 = image[ip][ir]*c1 + image[ip][jr]*c2;
            a2 = image[jp][ir]*c1 + image[jp][jr]*c2;
            MapValue(iptr,i,j) = a1*c3 + a2*c4;
            if (both) { 
               if (flip) 
                    MapValue(iptr,i,j) = -a1*c3 + a2*c4;
                else if (mirror)
                    MapValue(iptr,i,j) = -a1*c3 - a2*c4;
            } 
            if (first) {
                dmin = dmax = MapValue(iptr,i,j);
                first = FALSE;
            } else {
                dmin = MIN(dmin, MapValue(iptr,i,j));
                dmax = MAX(dmax, MapValue(iptr,i,j));
            }
        } /* i */
    } /* j */
    /* finish off the header */
    Xmin(iptr) = xrange[0];
    Ymin(iptr) = yrange[0];
    Dx(iptr) = xrange[2];
    Dy(iptr) = yrange[2];
    MapMin(iptr) = dmin;
    MapMax(iptr) = dmax;
    Unit(iptr) = unit;
    Time(iptr) = sds_time;

    set_headline(label);
    write_image(outstr,iptr);
    strclose(outstr);

    dprintf(0,"%d points outside radial grid\n",n1,n2);
    dprintf(0,"Datamin/max written: %g %g\n",dmin,dmax);
}

local void setrange(real *rval, string rexp, int nr)
{
    char *cptr, first[80];
    int n;

    cptr = strchr(rexp, ':');
    if (cptr) {
        n = cptr-rexp;
        if(n>79)error("Not enough string space in setrange[80]...");
	strncpy(first,rexp,n);
	first[n] = 0;
        if (nemoinpr(first,&rval[0],1) < 1) error("Parsing first part in %s",rexp);
        if (nemoinpr(cptr+1,&rval[1],1) < 1) error("Parsing second part in %s",rexp);
    } else {
        rval[0] = 0.0;
        if (nemoinpr(rexp,&rval[1],1) < 1) error("Parsing single %s",rexp);
    }
    rval[2] = (rval[1] - rval[0]) / nr;
}

local real string_real(string label, string key)
{
    char *cp = label;
    int klen = strlen(key);

    while (*cp) {
        if (strncmp(cp,key,klen)==0) {
            cp += klen;
            if (*cp == 0) 
                error("string_real conversion: %s %s",label,key);
            return atof(cp);
        }
        cp++;
    }
    warning("string_real conversion: %s %s",label,key);
    return -1.0;
}


/*
 * shift an array of size n by k elements (the slow way)
 */
 
local void fshift(int n, float *a, int k)
{
    int i, j;
    float tmp;

    for(j=0; j<k; j++) {
        tmp = a[n-1];
        for (i=n-1; i>0; i--) {
            a[i] = a[i-1];
        }
        a[0] = tmp;
    }
}
