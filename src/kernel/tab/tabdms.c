/*
 * TABDMS: convert column from/to HMS/DMS
 *
 *      24-jul-94  V0.1 created	PJT
 *	28-aug-98   0.2 ?
 *      13-sep-99   0.3 added hms PJT  - but not finished
 *      24-jan-00   0.4 added hms/dms with sign for decimal stuff
 *
   %a, %A     Sun/Mon/...  or Sunday/...
   %b, %B     Jan/...	      January/...
   %d         day of month (01..31)
   %D         mm/dd/yy
   %e         day of month, blank padded (1..31)
   %H         hour (00..23)
   %j         day of year (001..366)
   %k         hour (0..23)
   %m         month (01..12)
   %M         minute (00..59)
   %S         second (00..60)
   %T         time (hh:mm:ss)
   %y         last two digits of year (00..99)
   %Y         year (1970..)
 */

#include <stdinc.h>
#include <getparam.h>
#include <ctype.h>

string defv[] = {                /* DEFAULT INPUT PARAMETERS */
    "in=???\n           input file name(s)",
    "out=???\n          output file name",
    "todms=\n           list of columns (1..) to convert to dms (0=none)",
    "tohms=\n           list of columns (1..) to convert to hms (0=none)",
    "separator=:\n      separator between output D-M-S.S",
    "format=\n          date/time format",
    "VERSION=0.5\n      14-jun-00 PJT",
    NULL
};

string usage = "Convert to HMS/DMS tables";



stream instr, outstr;			/* file streams */

string sep;                             /* separator */

#define MAXCOL          256             /* MAXIMUM number of columns */
#define MLINELEN       8196		/* max linelength of a line */

bool   keepc[MAXCOL+1];                 /* columns to keep (t/f) in old fmt */
int    colmode[MAXCOL+1];		/* 0, +/-24, +/- 360 */

extern string *burststring(string, string);


nemo_main()
{
    setparams();

    instr = stropen(getparam("in"),"r");
    outstr = stropen (getparam("out"),"w");

    convert(instr,outstr);
}

setparams()
{
    int    col[MAXCOL];                    /* columns to skip for output */
    int    ncol, i, j;

    for (i=0; i<MAXCOL; i++)
        colmode[i+1] = 0;

    if (hasvalue("todms")) {
    	ncol = nemoinpi(getparam("todms"),col,MAXCOL);
        if (ncol < 0) error("Error parsing todms=%s",getparam("todms"));
        for (i=0; i<ncol; i++) {
            j = ABS(col[i]);
            if (j>MAXCOL) error("Column %d too large, MAXCOL=%d",j,MAXCOL);
            if (colmode[j] != 0) error("Column %d already specified",j);
            colmode[j] = ( col[i] > 0 ? 360 : -360);
        }
    }
    if (hasvalue("tohms")) {
    	ncol = nemoinpi(getparam("tohms"),col,MAXCOL);
        if (ncol < 0) error("Error parsing tohms=%s",getparam("tohms"));
        for (i=0; i<ncol; i++) {
            j = ABS(col[i]);
            if (j>MAXCOL) error("Column %d too large, MAXCOL=%d",j,MAXCOL);
            if (colmode[j] != 0) error("Column %d already specified",j);
            colmode[j] = ( col[i] > 0 ? 24 : -24);
        }
    }

    sep = getparam("separator");
}



convert(stream instr, stream outstr)
{
    char   line[MLINELEN];          /* input linelength */
    double dval;        
    int    nval, i, nlines, sign, decimalval;
    string *outv;                   /* pointer to vector of strings to write */
    string seps=", \t";             /* column separators  */
    real   dd, mm, ss;
        
    nlines=0;               /* count lines read so far */
    for(;;) {                       /* loop over all lines in file(s) */

        if (!get_line(instr, line))
            return 0;
        dprintf(3,"LINE: (%s)\n",line);
	if(line[0] == '#') continue;		/* don't use comment lines */
        nlines++;


        outv = burststring(line,seps);
        i=0;
        while (outv[i]) {
            if (colmode[i+1] == 0) {        /* no special mode: just copy */
                fputs(outv[i],outstr);
                fputs(" ",outstr);
            } else {
                if (nemoinpd(outv[i],&dval,1)<0) 
                    error("syntax error decoding %s",outv[i]);
		if (colmode[i+1] < 0) {     
                    sign = SGN(dval);
		    dval = ABS(dval);       /* do we allow < 0 ??? */
                    decimalval = (int)floor(dval);
                    fprintf(outstr,"%d",decimalval*sign);
                    fprintf(outstr,"%s",sep);
                    dval -= decimalval;
                    dval *= ABS(colmode[i+1]);
		}
                sign = SGN(dval);
                dval = ABS(dval);
                dd = floor(dval);
                dval = (dval-dd)*60.0;
                mm = floor(dval);
                ss = (dval-mm)*60.0;
                fprintf(outstr,"%02d",(int)dd*sign);
                fprintf(outstr,"%s",sep);
                fprintf(outstr,"%02d",(int)mm);
                fprintf(outstr,"%s",sep);
                fprintf(outstr,"%06.3f ",ss);

            }
            i++;
        }
        fputs("\n",outstr);
    } /* for(;;) */
}
