/*
 * SNAPSORT: sort particles according to a user-specified ranking.
 *
 *    22-jan-89	  V1.1   some changes	JEB
 *     9-dec-90   V1.2   helpvec	PJT
 *    16-jun-92   V1.3   usage; fixed bug with sandwiched history   PJT
 *    21-dec-92   V1.4   selectable sort routine                    PJT
 *    10-jun-95       a  declaration fix (linux) n                  pjt
 */

#include <stdinc.h>
#include <getparam.h>
#include <vectmath.h>
#include <filestruct.h>

#include <snapshot/body.h>
#include <snapshot/snapshot.h>
#include <snapshot/get_snap.c>
#include <snapshot/put_snap.c>

string defv[] = {		/* DEFAULT INPUT PARAMETERS */
    "in=???\n		Input file name (snapshot)",
    "out=???\n		Output file name (snapshot)",
    "rank=etot\n	Value used in ranking particles",
    "times=all\n        Range of times to process ",
    "sort=qsort\n       Sort mode {qsort;...}",
    "VERSION=1.4\n      10-jun-95 PJT ",
    NULL,
};

string usage="sort particles according to a user-specified ranking";

/* #define FLOGGER 1       /* merge in the cute flogger test routines */

nemo_main()
{
    stream instr, outstr;
    string times;
    rproc btrtrans(), rank;
    iproc mysort, getsort();
    Body *btab = NULL;
    int nbody, bits;
    real tsnap;

    instr = stropen(getparam("in"), "r");
    get_history(instr);
    outstr = stropen(getparam("out"), "w");
    put_history(outstr);
    rank = btrtrans(getparam("rank"));
    times = getparam("times");
    mysort = getsort(getparam("sort"));
    do {
        get_history(instr);  /*  get history that is not written */
	get_snap_by_t(instr, &btab, &nbody, &tsnap, &bits, times);
	if (bits & PhaseSpaceBit) {
	    snapsort(btab, nbody, tsnap, rank, mysort);
	    put_snap(outstr, &btab, &nbody, &tsnap, &bits);
	}
	btab=NULL;	/* 'free' the snapshot */
    } while (bits != 0);
    strclose(outstr);
}

snapsort(btab, nbody, tsnap, rank, mysort)
Body *btab;
int nbody;
real tsnap;
rproc rank;
iproc mysort;
{
    int i, rank_aux();
    Body *b;

    for (i = 0, b = btab; i < nbody; i++, b++)
	Aux(b) = (rank)(b, tsnap, i);
    (mysort)(btab, nbody, sizeof(Body), rank_aux);
}

int rank_aux(a, b)
Body *a, *b;
{
    return (Aux(a) < Aux(b) ? -1 : Aux(a) > Aux(b) ? 1 : 0);
}


/*
 *  Tabulate the valid sort names, plus their associated external
 *  routines. The accompanying getsort() routine returns the 
 *  appropriate sort routine
 */

typedef struct sortmode {
    string name;
    iproc   fie;
} sortmode;

#define SortName(x)  x->name
#define SortProc(x)  x->fie


/* List of externally available sort routines */

/* extern int qsort();         /* Standard Unix : stdlib.h */

extern int bubble_sort();   /* Flogger library routines */
extern int heap_sort();
extern int insertion_sort();
extern int merge_sort();
extern int quick_sort();
extern int shell_sort();


local sortmode smode[] = {
#ifdef FLOGGER
    "bubble",   bubble_sort,        /* cute flogger routines */
    "heap",     heap_sort,
    "insert",   insertion_sort,
    "merge",    merge_sort,
    "quick",    quick_sort,
    "shell",    shell_sort,
#endif
    "qsort",    qsort,              /* standard Unix qsort() */
    NULL, NULL,
};

iproc getsort(name)
string name;
{
    sortmode *s;

    for (s=smode; SortName(s); s++)  {
        dprintf(1,"GETSORT: Trying %s\n",SortName(s));
        if (streq(SortName(s),name))
            return SortProc(s);
    }
    error("No valid sortname");
    return NULL;
}
