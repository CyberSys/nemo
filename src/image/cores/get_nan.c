/*
 *  retrieve the value for NaN; mostly useful for FITS blanking
 *
 */

void get_nanf(float *x)
{
	union {
	  float f;
	  int i;
	  struct {
 	    unsigned :1, e:11;
          } s;
	} z;
#if defined(linux)
	z.i = -1;
#else
        z.s.e = 0x7ff;
#endif
        *x = z.f;
}

void get_nand(double *x)
{
	union {
	  double f;
	  struct {
 	    unsigned :1, e:11;
          } s;
	} z;
        
        z.s.e = 0x7ff;
        *x = z.f;
}

