/*
 *  TSD: show what's in a HDF Scientific Data Set (SD),
 *		modeled after 'tsf'
 *
 *      24-dec-94	V1.0  toy model		Peter Teuben
 *                      the MULTI_FILE doesn't work yet
 *	12-feb-95       added verbatim type info	PJT
 *	19-may-95	local symbols - 'fmt' clashed with something on linux
 *      21-may-95       V1.2: added out= format=, coordinate output
 *      27-aug-96       V1.3: header output now using dprintf()
 */

 
#include <stdinc.h>
#include <getparam.h>
#include <vectmath.h>

#include <filestruct.h>
#include <history.h>

#ifdef INC_HDF
#include <hdf.h> 	/* some conflicts with nemo include files */
#endif

string defv[] = {
    "in=???\n			Input file (HDF SD)",
    "out=\n                     ascii dump of the data to this file?",
    "format=%g\n                Format used in dump",
    "coord=f\n                  Add coordinates?",
    "VERSION=1.3\n		27-aug-96 PJT",
    NULL,
};

/* #define MULTI_FILE 1 */

string usage="Scan and optionally ascii dump of an HDF SDS";

#define MAXRANK 10

local int rank, shape[MAXRANK], run[MAXRANK];
local char label[256], unit[256], fmt[256], coordsys[256];


void nemo_main()
{
    scan_sd(getparam("in"));
}

#ifndef MULTI_FILE
scan_sd(string infile)		/* this is the fancy new version */
{
    float **dump, **coord;
    int i, j, k, ret, size, type, nsds;
    char ntype[32];
    string format;
    stream outstr;

    nsds = DFSDndatasets(infile);
    if (nsds<0) 
        error("%s is probably not an HDF scientific dataset",infile);
    dprintf(0,"Found %d scientific data set%s in %s\n",
            nsds,(nsds > 1 ? "s" : ""),infile);
    if (hasvalue("out")) {
        dump = (float **) allocate(nsds*sizeof(float *));   /* all data !! */
        format = getparam("format");
        outstr = stropen(getparam("out"),"w");
    } else
        outstr = NULL;
    
    for (k=0; k<nsds; k++) {
    	ret = DFSDgetdims(infile,&rank, shape, MAXRANK);
    	if (ret < 0) error("Problem getting rank/shape at SDS #%d",k+1);
        if (k==0) {         /* first time around allocate coordinates */
            if (getbparam("coord")) {
                coord = (float **) allocate(rank*sizeof(float *));
            } else {
                coord = NULL;
            }
        }

    	label[0] = unit[0] = fmt[0] = coordsys[0] = 0;
        ret = DFSDgetdatastrs(label, unit, fmt, coordsys);
        ret = DFSDgetNT(&type);
    	dprintf(0,"%d: %s(",k+1,label);
    	for (i=0, size=1; i<rank; i++) {
    	    if (i==rank-1)
                dprintf(0,"%d)",shape[i]);
            else
                dprintf(0,"%d,",shape[i]);
    	    size *= shape[i];

            if (k==0 && coord) {
                coord[i] = (float *) allocate(shape[i] * sizeof(float));
                ret = DFSDgetdimscale(i+1, shape[i],coord[i]);
                if (ret<0) error("getting shape[%d]",i+1);
            }
    	}
    	hdf_type_info(type,ntype);
    	dprintf(0," %s ",unit);
    	dprintf(0," -> [%d elements of type: %d (%s)]\n", size, type, ntype);
        if (outstr) {
            dump[k] = (float *) allocate(size * sizeof(float));
            ret = DFSDgetdata(infile,rank,shape,dump[k]);
        }
    }
    if (outstr) {
        for (i=0; i<rank; i++) run[i] = 0;      /* reset run array */

        for (i=0; i<size; i++) {                /* loop over all data */
            if (coord) {                        /* print coord system ? */
                for (j=rank-1; j>=0; j--) {
                    fprintf(outstr,format,coord[j][run[j]]);  
                    fprintf(outstr," ");
                }
                run[rank-1]++;
                for (j=rank-1; j>=0; j--) {     /* check if axis reset needed */
                    if (run[j] >= shape[j]) {
                        run[j] = 0;
                        if (j>0) run[j-1]++;
                    } else
                        break;
                }
            }
            for (k=0; k<nsds; k++) {            /* loop over all columns */
                fprintf(outstr,format,dump[k][i]);
                fprintf(outstr," ");
            }        
            fprintf(outstr,"\n");
        }
    }
}

#else


     /* NOTE: the above code was substantially changed, the code below not */

#define DFACC_RDONLY 1

scan_sd(string infile)
{
    int id, sds, n, i, j, k, ret, size, type, na, nd;
    char name[256];

    id = SDstart(infile, DFACC_RDONLY);
    if (id<0) error("%s is probably not an HDF SD (scientific dataset)",
                      infile);

    ret = SDfileinfo(id, &nd, &na);
    
    dprintf(0,"Found %d scientific data set%s in %s with %d attributes\n",
            nd,(nd > 1 ? "s" : ""),infile, na);
    for (k=0;;k++) {
        sds = SDselect(id, nd);
        ret = SDgetinfo(sds, name, &rank, shape, &type, &na);
    	label[0] = unit[0] = fmt[0] = coordsys[0] = 0;
        ret =  SDgetdatastrs(sds, label, unit, fmt, coordsys, 256);
    	dprintf(0,"%d: %s(",k,label);
    	for (i=0, size=1; i<rank; i++) {
    	    if (i==rank-1)
                dprintf(0,"%d)",shape[i]);
            else
                dprintf(0,"%d,",shape[i]);
    	    size *= shape[i];
    	}
    	dprintf(0," %s ",unit);
    	dprintf(0," -> [%d elements of type: %d]\n", size, type);
        /* ret = SDendaccess(sds); */
    }   
    SDend(id);
}

#endif
/*

   32-bit float DFNT_FLOAT32         5
   64-bit float DFNT_FLOAT64         6
    8-bit signed int DFNT_INT8      20
    8-bit unsigned int DFNT_UINT8   21
   16-bit signed int DFNT_INT16     22
   16-bit unsigned int DFNT_UINT16  23
   32-bit signed int DFNT_INT32     24
   32-bit unsigned int DFNT_UINT32  25

*/                        
