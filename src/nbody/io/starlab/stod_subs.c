/*
 * aid functions for stod, but in C to avoid too many collisions
 * between NEMO and STARLAB and vice versa.
 *
 * The C++ declarations live in stod_subs.h
 */


#include <stdinc.h>
#include <vectmath.h>
#include <filestruct.h>

#include <snapshot/body.h>
#include <snapshot/snapshot.h>

void check_real(int size)
{
    if (sizeof(real) != size) 
        error("NEMO is using %d-byte real, STARLAB is using %d real",
            sizeof(real), size);
}


int get_snap_c(string fname, real **mass, real **pos, real **vel)
{
    int i, nbody;
    real *mptr, *pptr, *vptr, *phase;
    stream instr;

    instr = stropen(fname,"r");                     /* open file */
    get_history(instr);                             /* add data history */
    if (!get_tag_ok(instr, SnapShotTag)) return 0;
    get_set(instr, SnapShotTag);                    /* and get first dataset */
    get_set(instr, ParametersTag);
    get_data(instr, NobjTag, IntType, &nbody, 0);

    get_tes(instr, ParametersTag);

    if (!get_tag_ok(instr, ParticlesTag)) return 0;
    get_set(instr, ParticlesTag);
    if (get_tag_ok(instr, MassTag)) {
        *mass = mptr  = (real *) allocate(nbody * sizeof(real));
        get_data_coerced(instr, MassTag, RealType, mptr,nbody, 0);
    } else
        *mass = NULL;

    if (get_tag_ok(instr, PhaseSpaceTag)) {
        phase = (real *) allocate(2 * NDIM * nbody * sizeof(real));
        *pos  = pptr  = (real *) allocate(NDIM * nbody * sizeof(real));
        *vel  = vptr  = (real *) allocate(NDIM * nbody * sizeof(real));
        get_data_coerced(instr, PhaseSpaceTag, RealType, phase,nbody,2,NDIM,0);
        for (i=0; i<nbody; i++) {
            *pptr++ = *phase++;
            *pptr++ = *phase++;
            *pptr++ = *phase++;
            *vptr++ = *phase++;       
            *vptr++ = *phase++;       
            *vptr++ = *phase++;
        }       
    }
    return nbody;
}


void put_snap_c(string fname, int nbody, real *mass, real *pos, real *vel)
{
    int i;
    real *mptr, *pptr, *vptr, *ptr, *phase;
    stream instr;

    instr = stropen(fname,"w");                     /* open file */
    put_history(instr);                             /* add data history */

    put_set(instr, SnapShotTag);                    /* and put first dataset */

    put_set(instr, ParametersTag);
    put_data(instr, NobjTag, IntType, &nbody, 0);
    put_tes(instr, ParametersTag);


    put_set(instr, ParticlesTag);
    put_data(instr, MassTag, RealType, mass, nbody, 0);

    ptr = phase = (real *) allocate(2 * NDIM * nbody * sizeof(real));
    pptr = pos;
    vptr = vel;
    for (i=0; i<nbody; i++) {
            *ptr++ = *pptr++;
            *ptr++ = *pptr++;
            *ptr++ = *pptr++;
            *ptr++ = *vptr++;
            *ptr++ = *vptr++;
            *ptr++ = *vptr++;
    }       
    put_data(instr, PhaseSpaceTag, RealType, phase, nbody,2,NDIM,0);
    put_tes(instr, ParticlesTag);
    put_tes(instr, SnapShotTag);
}


