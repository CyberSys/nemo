/*
 *  ellipse:   (de)project ellipses at skewed angles and find the new ellipse
 *
 *   0.2  4-nov-02    original                  PJT
 *   0.3 23-apr-03    added phi_kin to try M51's oval           PJT
 */

#include <stdinc.h>
#include <getparam.h>
#include <math.h>
#include <mathfns.h>

string defv[] = {
  "ba=???\n      Axis ratio of bar",
  "inc=???\n     Inclination of galaxy",
  "phi=\n        Angle between bar and disk in sky plane ",
  "theta=\n      Angle between bar and disk in galax plane",
  "dba=0.0\n     Error term in b/a, if determined",
  "dphi=0.0\n    Error term in phi, if determined",
  "nsim=0\n      Number of monte carlo to perform to compute an error term",
  "seed=0\n      Seed for random number generator",
  "VERSION=0.3\n 23-apr-03 PJT",
  NULL,
};

string usage = "ellipse (de)projection properties";

#define MAXP 1024
#define RPD 0.0174532925199433


void deproject(double ba, double inc, double phi, double *ba1,  double *theta, double *phi_kin) {
  double ai,bi,ci, cos2t, tan2t, ba2, tank;
  double ome2 = ba*ba;   /* 1-e^2 */
  double e2 = 1-ome2;    /* 1-b^2 ;    b := b/a, i.e.   a=1 */
  double cosp = cos(phi*RPD);
  double sinp = sin(phi*RPD);
  double cosi = cos(inc*RPD);
  double z;

  ai =           (1-e2*cosp*cosp)/ome2;
  ci = cosi*cosi*(1-e2*sinp*sinp)/ome2;
  bi = -2*e2*sinp*cosp*cosi/ome2;
  tan2t = bi/(ai-ci);
  cos2t = 1/sqrt(1+tan2t*tan2t);
  ba2 = ((ai+ci)*cos2t + (ai-ci))/((ai+ci)*cos2t - (ai-ci));
  if (ba2<0) error("Bad ba2=%g",ba2);
  z = atan(tan2t)/2/RPD;
  ba2 = sqrt(ba2);
  if (ba2 > 1) {
    ba2 = 1.0/ba2;
    z += 90.0;
  }
  *ba1 = ba2;
  *theta = z;
  tank = bi/(2*ai);
  *phi_kin = atan(tank)/RPD;
}


void  project(double ba, double inc, double theta, double *ba1,  double *phi) {
  error("project-ing not implemented yet");
}


nemo_main()
{
  bool Qtheta = hasvalue("theta");
  bool Qphi = hasvalue("phi");
  bool Qerrba;
  int nsim = getiparam("nsim");
  int seed = init_xrandom(getparam("seed"));
  double ba[MAXP], inc[MAXP], ang[MAXP], x,y,z;
  double dba;
  int nba, ninc, nang;
  int i,j,k;
  string fmt="%g";

  nba = nemoinpd(getparam("ba"),ba,MAXP);
  ninc = nemoinpd(getparam("inc"),inc,MAXP);

  if (Qtheta) {                                    /* theta given, so project it */
    dprintf(0,"Projecting ellipse:\n");
    dprintf(0,"b/a  inc  theta     b/a'  phi\n");
    nang = nemoinpd(getparam("theta"),ang,MAXP);
    for (i=0; i<nang; i++)
      for (j=0; j<ninc; j++)
	for (k=0; k<nba; k++) {
	  project(ba[k],inc[j],ang[i],&x,&y);
	  printf("%g %g %g    %g %g \n",ba[k],inc[j],ang[i],x,y);
	}
  } else if (Qphi) {                               /* phi given, so deproject */
    dprintf(0,"De-projecting ellipse:\n");
    dprintf(0,"b/a  inc  phi    b/a'  theta    phi_kin\n");
    nang = nemoinpd(getparam("phi"),ang,MAXP);
    for (i=0; i<nang; i++)
      for (j=0; j<ninc; j++)
	for (k=0; k<nba; k++) {
	  if (nsim > 0) {
	    double dba = getdparam("dba");
	    double dphi = getdparam("dphi");
	    int l;
	    for (l=0; l<nsim; l++) {
	      deproject(ba[k]+grandom(0.0,dba),inc[j],ang[i]+grandom(0.0,dphi),&x,&y,&z);
	      printf("%g %g %g    %g %g %g\n",ba[k],inc[j],ang[i],x,y,z);
	    }
	  } else {
	    deproject(ba[k],inc[j],ang[i],&x,&y,&z);
	    printf("%g %g %g    %g %g %g\n",ba[k],inc[j],ang[i],x,y,z);
	  }
	}
  } else
    error("Need phi= or theta=");
}

