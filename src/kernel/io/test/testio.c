/*
 * Test of Nemo's i/o stuff
 *	3-oct-90    Created for new 'micro (nu)nemo'	PJT
 *     12-oct-90    Read or Write big files...          PJT
 *     13-oct-90    Random access                       PJT
 *     27-feb-94    ansi
 */

#include <stdinc.h>
#include <getparam.h>
#include <filestruct.h>

string defv[] = {
    "in=\n         Input file (if provided)",
    "out=\n        output file (if provided)",
    "real=1.234\n  Real number to write (if out=)",
    "count=1\n     Number of reals to write (if out=)",
    "random=1\n    Number of times to reverse (random) write count",
    "VERSION=1.2a\n 27-feb-94 PJT",
    NULL,
};
string usage = "testing NEMO's I/O";

nemo_main()
{
    real x,y = 0;
    int count, random;
    string name;

    x = getdparam("real");    
    count = getiparam("count");
    random = getiparam("random");
    
    name = getparam("in");
    if (*name)
        get_number(name,&y);
    else
        dprintf(1,"No input file specified\n");
    
    name = getparam("out");
    if (*name)
        put_number(name,y+x,count,random);
    else
        dprintf(1,"No output file specified\n");
}

get_number(name,x)
string name;
real *x;
{
    stream str;
    real *buf;
    int  n;

    str = stropen(name,"r");
    get_history(str);

    if (!get_tag_ok(str,"n"))
        error("%s: missing \"n\", Not a valid TESTIO input file",name);
    get_data(str,"n",IntType,&n,0);
    buf = (real *) allocate(n*sizeof(real));

    if (!get_tag_ok(str,"x"))
        error("%s: missing \"x\", Not a valid TESTIO input file",name);
    get_data(str,"x",RealType,buf,n,0);
    strclose(str);

    *x = *buf;
    dprintf(1,"Read number %f from file %s\n",*x,name);
    free( (char *) buf);
    return(n);
}

put_number(name,x,n,r)
string name;
real x;
int n,r;
{
    stream str;
    real *buf;
    int nout,i;

    str = stropen(name,"w");
    put_history(str);
    nout = n*r;
    put_data(str,"n",IntType,&nout,0);
    buf = (real *) allocate(n*sizeof(real));
    buf[0] = x;   /* only set first and third number */
    if (n>2) buf[2] = x;
    if (r==1)
        put_data(str,"x",RealType,buf,n,0);
    else {
        put_data_set(str,"x",RealType,nout,0);
        for (i=r;i>0;i--) {
            buf[1] = -1.0 * i;
            if (n>3) get_nand(&buf[3]);
	    if (n>4) get_nanf(&buf[4]);
            put_data_ran(str,"x",buf,(i-1)*n,n);
        }
        put_data_tes(str,"x");
    }
    strclose(str);
    dprintf(1,"Wrote number %f to file %s\n",x,name);
    free( (char *) buf);
}
