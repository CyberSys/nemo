/* 
 * CCDMEDIAN: median filtering of an image
 *
 *	29-jan-01	PJT	q&d for Mark Wolfire
 *      30-jan-01       PJT     experimenting with NR's sort routines
 *                      
 */


#include <stdinc.h>
#include <getparam.h>
#include <vectmath.h>
#include <filestruct.h>
#include <image.h>

string defv[] = {
        "in=???\n       Input image file",
	"out=???\n      Output image file",
	"n=5\n		(odd) size of filter",
	"VERSION=0.1\n  29-jan-01 PJT",
	NULL,
};

string usage = "median filter of an image";



#if 0
#define CVI(x,y,z)  CubeValue(iptr,x,y,z)
#define CVO(x,y,z)  CubeValue(optr,x,y,z)
#else
#define CVI(x,y)    MapValue(iptr,x,y)
#define CVO(x,y)    MapValue(optr,x,y)
#endif

real median(int, real *);
void sort1(int, real *);
void sort2(int, real *);
void sort3(int, real *);

#define sort sort1

void nemo_main()
{
    stream  instr, outstr;
    int     nx, ny, nz, mode;
    int     i,j,k, n, n1, i1, j1, m;
    imageptr iptr=NULL, optr;      /* pointer to images */
    real    *vals;

    n = getiparam("n");
    dprintf(0,"Median filter size %d\n",n);
    if (n%2 != 1) error("filter size %d needs to be odd",n);
    n1 = (n-1)/2;
    vals = (real *) allocate (1 + sizeof(real) * n * n);

    instr = stropen(getparam("in"), "r");
    read_image( instr, &iptr);
    nx = Nx(iptr);	
    ny = Ny(iptr);
    nz = Nz(iptr);
    if (nz > 1) error("Cannot do 3D cubes properly; use 2D");

    outstr = stropen(getparam("out"), "w");
    create_cube(&optr,nx,ny,nz);
    Dx(optr) = Dx(iptr);
    Dy(optr) = Dy(iptr);
    Dz(optr) = Dz(iptr);
    Xmin(optr) = Xmin(iptr);
    Ymin(optr) = Ymin(iptr);
    Zmin(optr) = Zmin(iptr);

    for (j=0; j<ny; j++) {
        for (i=0; i<nx; i++) {
            if (j<n || j > ny-n) {
                CVO(i,j) = CVI(i,j);
                continue;
            }
            if (i<n || i>nx-n) {
                CVO(i,j) = CVI(i,j);
                continue;
            }
	    m = 0;
	    for (j1=j-n1; j1<=j+n1; j1++)
	    for (i1=i-n1; i1<=i+n1; i1++)
	      vals[m++] = CVI(i1,j1);
	    CVO(i,j) = median(m,vals);
        }
    }
    write_image(outstr, optr);
}

real median(int n, real *x)
{
#if 0
  real sum = 0.0;

  sum = 0.0;
  for (i=0; i<n; i++)
    sum += x[i];
  return sum/n;
#else
  sort(n, x);
  return x[(n-1)/2];
#endif
}

/* very simply shell sort ; k&r pp108 */

void sort0(int n, real *x)
{
    int   gap, i, j;
    real  temp;
    for (gap=n/2; gap>0; gap /= 2)
        for (i=gap; i<n; i++)
            for (j=i-gap; j>=0; j -= gap) {
                if (x[j] <= x[j+gap])
                    break;
                temp = x[j];
                x[j] = x[j+gap];
                x[j+gap] = temp;
            }
}


/* 
 * Numerical Recipes quick-sort routine ;; still in NR notation index-1 based
 */
#define SWAP(a,b) temp=(a);(a)=(b);(b)=temp;

#define NSTACK 50
#define M      7

static int *istack = NULL;

void sort1(int n, real *arr)
{
  int i,ir=n,j,k,l=1;
  int jstack=0, *istack;
  real a,temp;

  if (istack == NULL)
    istack = (int *) allocate(sizeof(int) * NSTACK);
  
  for (;;) {
    if (ir-l < M) {
      for (j=l+1;j<=ir;j++) {
	a=arr[j];
	for (i=j-1;i>=1;i--) {
	  if (arr[i] <= a) break;
	  arr[i+1]=arr[i];
	}
	arr[i+1]=a;
      }
      if (jstack == 0) break;
      ir=istack[jstack--];
      l=istack[jstack--];
    } else {
      k=(l+ir) >> 1;
      SWAP(arr[k],arr[l+1])
	if (arr[l+1] > arr[ir]) {
	  SWAP(arr[l+1],arr[ir])
	    }
      if (arr[l] > arr[ir]) {
	SWAP(arr[l],arr[ir])
	  }
      if (arr[l+1] > arr[l]) {
	SWAP(arr[l+1],arr[l])
	  }
      i=l+1;
      j=ir;
      a=arr[l];
      for (;;) {
	do i++; while (arr[i] < a);
	do j--; while (arr[j] > a);
	if (j < i) break;
	SWAP(arr[i],arr[j]);
      }
      arr[l]=arr[j];
      arr[j]=a;
      jstack += 2;
      if (jstack > NSTACK) error("NSTACK too small in sort.");
      if (ir-i+1 >= j-l) {
	istack[jstack]=ir;
	istack[jstack-1]=i;
	ir=j-1;
      } else {
	istack[jstack]=j-1;
	istack[jstack-1]=l;
	l=i;
      }
    }
  }
}


/*
 * NR heap sort  ; don't use index-1 based
 */
void sort3(int n, real *ra)
{
  int i,ir,j,l;
  real rra;
  
  if (n < 2) return;
  l=(n >> 1)+1;
  ir=n;
  for (;;) {
    if (l > 1) {
      rra=ra[--l];
    } else {
      rra=ra[ir];
      ra[ir]=ra[1];
      if (--ir == 1) {
	ra[1]=rra;
	break;
      }
    }
    i=l;
    j=l+l;
    while (j <= ir) {
      if (j < ir && ra[j] < ra[j+1]) j++;
      if (rra < ra[j]) {
	ra[i]=ra[j];
	i=j;
	j <<= 1;
      } else j=ir+1;
    }
    ra[i]=rra;
  }
}
