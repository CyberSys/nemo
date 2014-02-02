/* =================================================================
|  Copyright Jean-Charles LAMBERT - 2005                            
|  e-mail:   Jean-Charles.Lambert@lam.fr                           
|  address:  Centre de donneeS Astrophysique de Marseille (CeSAM)   
|            Laboratoire d'Astrophysique de Marseille               
|            2, place Le Verrier                                    
|            13248 Marseille Cedex 4, France                        
|            CNRS U.M.R 7326                                        
| ==================================================================
|* Initialyze io_nemo data                                          
+----------------------------------------------------------------- */
#ifndef IO_INIT_H
#define IO_INIT_H
#ifdef __cplusplus
extern "C" {
#endif

void init_io_one(int *, bool * , bool *, bool *, char **, int);

void init_flag_io();

#ifdef __cplusplus
}
#endif

#endif /* IO_INIT_H */
/* -----------------------------------------------------------------
|  End of io_init.h                                                 
+----------------------------------------------------------------- */
