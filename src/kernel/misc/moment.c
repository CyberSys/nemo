/*
 * MOMENTS: accumulate and compute moments - almost OO !
 *
 *  30-oct-93   created         peter teuben
 *   9-nov	reset at ini 
 *   4-mar-94   ansi
 *  12-apr-95   return 0 for mean, sigma etc. when no points accumulated
 *  13-jun-95   added decr_moment, but min/max become invalid
 *  16-sep-95   fixed more bugs if no data accumulated
 *   9-dec-99   fix min/max when data deleted
 *   2-feb-05   added moving moments
 */


#include <stdinc.h>
#include <moment.h>

#define sum0 m->sum[0]
#define sum1 m->sum[1]
#define sum2 m->sum[2]
#define sum3 m->sum[3]
#define sum4 m->sum[4]

void ini_moment(Moment *m, int mom, int ndat)
{
    int i;

    dprintf(1,"ini_moment(%d,%d)\n",mom,ndat);

    m->n = 0;
    m->mom = mom;
    if (mom < 0)
      m->sum = NULL;
    else {
      m->sum = (real *) allocate((mom+1) * sizeof(real));
      for (i=0; i<=mom; i++) m->sum[i] = 0.0;
    }
    m->ndat = ndat;

    if (ndat > 0) {
      m->idat = -1;
      m->dat = (real *) allocate(ndat*sizeof(real));
      m->wgt = (real *) allocate(ndat*sizeof(real));
    } 
}

void accum_moment(Moment *m, real x, real w)
{
    register real sum = w;
    real xx;
    int i;

    if (m->n == 0) {
	m->datamin = m->datamax = x;
    } else {
    	m->datamin = MIN(x, m->datamin);
    	m->datamax = MAX(x, m->datamax);
    }
    m->n++;
    if (m->mom < 0) return;
    for (i=0; i <= m->mom; i++) {
        m->sum[i] += sum;
        sum *= x;
    }
    if (m->ndat > 0) {
      if (m->idat < 0)            /* first time around */
	m->idat=0;
      else if (m->n < m->ndat)    /* buffer not full yet */
	m->idat++;
      else {                      /* buffer full, first remove old */
	i = m->idat;
	if (i==0)  i = m->ndat-1;
	m->idat++;
	if (m->idat == m->ndat) m->idat = 0;
	xx = m->dat[i];
	sum = m->wgt[i];
	for (i=0; i<= m->mom; i++) {
	  m->sum[i] -= sum;
	  sum *= xx;
	}
      }

      m->dat[m->idat] = x;
      m->wgt[m->idat] = w;
    }
}


void decr_moment(Moment *m, real x, real w)
{
    register real sum = w;
    int i;

    if (m->ndat > 0) 
      error("decr_moment: cannot be used in moving moments mode");

    if (m->n == 0) {
	warning("Cannot decrement a moment with no data accumulated");
    } else {
	/* note we cannot correct min/max here ....  client must update */
    }
    m->n--;
    if (m->mom < 0) return;
    for (i=0; i <= m->mom; i++) {
        m->sum[i] -= sum;
        sum *= x;
    }
}

void reset_moment(Moment *m)
{
    int i;
    
    m->n = 0;
    if (m->mom < 0) return;
    for (i=0; i <= m->mom; i++)
        m->sum[i] = 0.0;
}

real show_moment(Moment *m, int mom)
{
    if (m->mom < 0) error("Cannot show moment for mom=%d",m->mom);
    if (mom >= 0 && mom <= m->mom)
        return m->sum[mom];
    else if (mom == -1)
        return mean_moment(m);
    else if (mom == -2)
        return sigma_moment(m);
    else if (mom == -3)
        return skewness_moment(m);
    else if (mom == -4)
        return kurtosis_moment(m);
    else 
        warning("show_moment: out-of-range moment %d %d",mom);
    return 0.0;
}

int n_moment(Moment *m)
{
    return m->n;
}

real sum_moment(Moment *m)
{
    return sum0;   /* BAD BOY: should this not be sum1 ?? */
}

real mean_moment(Moment *m)
{
    if (m->mom < 1)
        error("mean_moment cannot be computed with mom=%d",m->mom);
    if (m->n == 0) return 0;
    return sum1/sum0;
}

real sigma_moment(Moment *m)
{
    real mean, tmp;
    if (m->mom < 2)
        error("sigma_moment cannot be computed with mom=%d",m->mom);
    if (m->n == 0) return 0;
    mean=sum1/sum0;
    if (m->datamin == m->datamax) return 0.0;
    tmp = sum2/sum0 - mean*mean;
    if (tmp <= 0.0) return 0.0;
    return sqrt(tmp);
}

real skewness_moment(Moment *m)
{
    real mean, sigma, tmp;

    if (m->mom < 3)
        error("skewness_moment cannot be computed with mom=%d",m->mom);
    if (m->n == 0) return 0.0;
    if (m->datamin == m->datamax) return 0.0;
    mean = sum1/sum0;
    sigma = sum2/sum0 - mean*mean;
    if (sigma < 0.0) sigma = 0.0;
    sigma = sqrt(sigma);    
    tmp = ((sum3-3*sum2*mean)/sum0 + 2*mean*mean*mean) /
            (sigma*sigma*sigma);
    return tmp;
}

real kurtosis_moment(Moment *m)
{
    real mean, sigma2, tmp;

    if (m->mom < 4)
    error("kurtosis_moment cannot be computed with mom=%d",m->mom);
    if (m->datamin == m->datamax) return 0.0;
    if (m->n == 0) return 0.0;
    mean = sum1/sum0;
    sigma2 = sum2/sum0 - mean*mean;

    tmp = -3.0 +
           ((sum4-4*sum3*mean+6*sum2*mean*mean)/sum0 - 3*mean*mean*mean*mean) /
           (sigma2*sigma2);
    return tmp;
}

real min_moment(Moment *m)
{
    return m->datamin;
}                                        

real max_moment(Moment *m)
{
    return m->datamax;
}                                        

#ifdef TESTBED
#include <getparam.h>

string defv[] = {
    "in=???\n       Table with values in column 1",
    "moment=-1\n    Moment to compute (-4..-1 special, 0,1,2,...)",
    "minmax=f\n     Show datamin & max instead ? ",
    "maxsize=0\n    If > 0, size for moving moments instead\n",
    "VERSION=0.2\n  2-feb-05 PJT",
    NULL,
};

string usage = "(Moving)Moments TESTBED";

void nemo_main(void)
{
    char line[80];
    stream instr = stropen(getparam("in"),"r");
    int mom = getiparam("moment");
    int maxsize = getiparam("maxsize");
    real x;
    Moment m;

    ini_moment(&m,ABS(mom),maxsize);
    while (fgets(line,80,instr) != NULL) {
      x = atof(line);
      accum_moment(&m,x,1.0);
      if (maxsize > 0) {
	if (getbparam("minmax"))
	  printf("%g %g\n",min_moment(&m), max_moment(&m));
	else
	  printf("%g\n",show_moment(&m,mom));
      }
    }
    if (maxsize == 0) {
      if (getbparam("minmax"))
        printf("%g %g\n",min_moment(&m), max_moment(&m));
      else
        printf("%g\n",show_moment(&m,mom));
    }
}

#endif
