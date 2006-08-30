// -*- C++ -*-                                                                  
////////////////////////////////////////////////////////////////////////////////
///                                                                             
/// \file   inc/public/forces.cc                                                
///                                                                             
/// \author Walter Dehnen                                                       
/// \date   1999-2006                                                           
///                                                                             
/// \brief  defines all function members of class forces                        
///                                                                             
////////////////////////////////////////////////////////////////////////////////
//                                                                              
// Copyright (C) 1999-2006 Walter Dehnen                                        
//                                                                              
// This program is free software; you can redistribute it and/or modify         
// it under the terms of the GNU General Public License as published by         
// the Free Software Foundation; either version 2 of the License, or (at        
// your option) any later version.                                              
//                                                                              
// This program is distributed in the hope that it will be useful, but          
// WITHOUT ANY WARRANTY; without even the implied warranty of                   
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
// General Public License for more details.                                     
//                                                                              
// You should have received a copy of the GNU General Public License            
// along with this program; if not, write to the Free Software                  
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                    
//                                                                              
////////////////////////////////////////////////////////////////////////////////
#ifndef falcON_included_forces_cc
#  define falcON_included_forces_cc

#ifndef falcON_included_forces_h
#  include <forces.h>
#endif
#ifndef falcON_included_iomanip_h
#  include <iomanip>
#  define falcON_included_iomanip_h
#endif
#ifndef falcON_included_fstream_h
#  include <fstream>
#  define falcON_included_fstream_h
#endif
#ifndef falcON_included_body_h
#  include <body.h>
#endif
#ifndef falcON_included_gravity_h
#  include <public/gravity.h>
#endif
#ifndef falcON_included_partner_h
#  include <public/partner.h>
#endif
#ifdef falcON_SPH
#  ifndef falcON_included_sph_h
#    include <sph/sph.h>
#  endif
#endif

namespace falcON {
  //---------------------------------------------------------------------------+
  //                                                                           |
  // CONSTRUCTORS, DESTRUCTOR AND SUCH                                         |
  //                                                                           |
  //---------------------------------------------------------------------------+
  inline forces::forces(const bodies*b,
			real         e,
			real         th,
			kern_type    k,
#ifdef falcON_INDI
			bool         i_soft,
#endif
			real         g,
			MAC_type     mac,
			const int    gd[4]
#ifdef falcON_SPH
		       ,const int    sd[3]
#endif
			) falcON_THROWING :
    STATS   ( new GravStats() ),
    BODIES  ( b ),
    Ncrit   ( 0 ),
    TREE    ( 0 ),
    GMAC    ( new GravMAC(mac, abs(th), falcON_ORDER) ),
#ifdef falcON_INDI
    GRAV    ( new GravEstimator(TREE,k,STATS,e,g,i_soft,gd) ),
#else
    GRAV    ( new GravEstimator(TREE,k,STATS,e,g,0,gd) ),
#endif
    PAES    ( 0 )
#ifdef falcON_SPH
			  ,
    SPHT    ( new SphEstimator(TREE,sd) )
#endif
  {}
  //----------------------------------------------------------------------------
  inline forces::~forces() falcON_THROWING
  {
    if(TREE) falcON_DEL_O(TREE);
    falcON_DEL_O(GMAC);
    falcON_DEL_O(STATS);
    falcON_DEL_O(GRAV);
    if(PAES) falcON_DEL_O(PAES);
#ifdef falcON_SPH
    falcON_DEL_O(SPHT);
#endif
  }
  //----------------------------------------------------------------------------
#ifdef falcON_INDI
  inline bool const &forces::use_individual_eps() const
  {
    return GRAV->use_indiv_eps();
  }
#endif
  //----------------------------------------------------------------------------
  inline void forces::reset_softening (real e, kern_type k) const
  {
    GRAV->reset_softening(e,k);
  }
  //----------------------------------------------------------------------------
  inline void forces::reset_NewtonsG (real g) const
  {
    GRAV->reset_NewtonsG(g);
  }
  //----------------------------------------------------------------------------
  inline void forces::reset_opening(real th, MAC_type mac) const
  {
    GMAC->reset(mac, abs(th), falcON_ORDER);
  }
  //---------------------------------------------------------------------------+
  //                                                                           |
  // TREE GROWTH AND RE-USE                                                    |
  //                                                                           |
  //---------------------------------------------------------------------------+
  inline void forces::grow(int Ncr, const vect*x0) falcON_THROWING
    // This routines grows a fresh tree or re-grows an existing tree            
  {
    SET_I
      Ncrit = max(Ncr,1);
    if(TREE) {
      TREE->build(Ncrit,x0);
      GRAV->reset();
#ifdef falcON_SPH
      SPHT->reset();
#endif
      SET_T(" time: OctTree::build():               ");
    } else {
      TREE = new OctTree(BODIES,Ncrit,x0);
      GRAV->new_tree(TREE);
#ifdef falcON_SPH
      SPHT->new_tree(TREE);
#endif
      SET_T(" time: OctTree::OctTree():            ");
    }
  }
  //----------------------------------------------------------------------------
  inline void forces::reuse() falcON_THROWING
  {
    // This routine merely updates the leaf's positions                         
    SET_I
      if(TREE == 0) {
	warning("no old tree to be re-used","forces::reuse()");
	return grow();
      }
    TREE->reuse();
    GRAV->reset();
#ifdef falcON_SPH
    SPHT->reset();
#endif
  }
  //---------------------------------------------------------------------------+
  //                                                                           |
  // INFORMATION AND STATISTICS                                                |
  //                                                                           |
  //---------------------------------------------------------------------------+
  inline kern_type const &forces::kernel() const
  {
    return GRAV->kernel();
  }
  //----------------------------------------------------------------------------
  inline const char*  forces::describe_kernel () const
  {
    return describe(GRAV->kernel());
  }
  //----------------------------------------------------------------------------
  inline real const &forces::eps() const
  {
    return GRAV->softening_length();
  }
  //----------------------------------------------------------------------------
  inline real const &forces::softening_length() const
  {
    return GRAV->softening_length();
  }
  //----------------------------------------------------------------------------
  inline real const &forces::NewtonsG() const
  {
    return GRAV->NewtonsG();
  }
  //----------------------------------------------------------------------------
  inline const MAC_type &forces::MAC() const
  {
    return GMAC->method();
  }
  //----------------------------------------------------------------------------
  inline const char* forces::describe_MAC() const
  {
    return GMAC->describe_method();
  }
  //----------------------------------------------------------------------------
  inline unsigned forces::No_bodies() const
  {
    return BODIES->N_bodies();
  }
  //----------------------------------------------------------------------------
  inline unsigned forces::No_bodies_used() const
  {
    return TREE->N_leafs();
  }
  //----------------------------------------------------------------------------
  inline unsigned forces::No_cells_used() const
  {
    return TREE->N_cells();
  }
  //----------------------------------------------------------------------------
  inline void forces::dump_nodes(const char* fcells, const char* fleafs) const
  {
    if(fcells) {
      std::ofstream o(fcells);
      GRAV->dump_cells(o);
    }
    if(fleafs) {
      std::ofstream o(fleafs);
      GRAV->dump_leafs(o);
    }
  }
  //----------------------------------------------------------------------------
  inline void forces::stats(std::ostream& out) const
  {
    out<<"\n state:                ";
    if(TREE) {
      if     (TREE->is_fresh()   ) out<<" tree new\n";
      else if(TREE->is_re_grown()) out<<" tree re-grown\n"; 
      else if(TREE->is_re_used() ) out<<" tree re-used\n";
      out<<" root center:           "<<TREE->root_center()    <<'\n'
	 <<" root radius:           "<<radius(TREE->root())   <<'\n'
	 <<" bodies loaded:         "<<number(TREE->root())   <<'\n';
      if(GRAV->N_coeffs())
	out<<" total mass:            "<<mass  (GRAV->root())<<'\n';
      out<<" N_crit:                "<<Ncrit                  <<'\n'
	 <<" cells used:            "<<TREE->N_cells()        <<'\n';
      if(GRAV->N_coeffs())
	out<<" of which were active   "<<GRAV->N_active_cells()<<'\n';
      out<<" maximum depth:         "<<TREE->depth()          <<'\n'
	 <<" current theta:         "<<GMAC->theta_min()      <<'\n'
	 <<" current MAC:           "<<GMAC->describe_method()<<'\n';
#ifdef falcON_INDI
      if(GRAV->use_indiv_eps())
	out<<" softening:             individual\n";
      else
	out<<" softening:             global\n"
	   <<" softening length:      "<<GRAV->softening_length()<<'\n';
#else
      out<<" softening length:      "<<GRAV->softening_length()<<'\n';
#endif
      out<<" softening kernel:      "<<describe_kernel()      <<'\n';
      if(TREE->is_used_for_grav()) {               // IF just had gravity       
	out
	  <<" Taylor coeffs used:    "<<GRAV->N_coeffs()
	  <<" in "<<GRAV->N_chunks()
	  <<" chunks of "<<GRAV->N_elems_in_chunk()<<'\n';
	STATS->write(out);
      }
    } else out<<" no tree\n";
  }
  //----------------------------------------------------------------------------
  inline unsigned const&forces::No_coeffs_used() const
  {
    return GRAV->N_coeffs();
  }
  //----------------------------------------------------------------------------
  inline unsigned forces::BB_interactions() const
  {
    return STATS->BB_direct_iacts();
  }
  //----------------------------------------------------------------------------
  inline unsigned forces::MB_interactions() const
  {
    return
      STATS->CB_direct_iacts()+
      STATS->CC_direct_iacts()+
      STATS->CX_direct_iacts();
  }
  //----------------------------------------------------------------------------
  inline unsigned forces::CB_interactions() const
  {
    return STATS->CB_approx_iacts();
  }
  //----------------------------------------------------------------------------
  inline unsigned forces::CC_interactions() const
  {
    return STATS->CC_approx_iacts();
  }
  //----------------------------------------------------------------------------
  inline unsigned forces::total_interactions () const
  {
    return 
      BB_interactions()+
      MB_interactions()+
      CB_interactions()+
      CC_interactions();
  }
  //----------------------------------------------------------------------------
  inline vect const&forces::root_center() const
  {
    return TREE->root_center();
  }
  //----------------------------------------------------------------------------
  inline real const&forces::root_radius() const
  {
    return radius(TREE->root());
  }
  //----------------------------------------------------------------------------
  inline unsigned const&forces::root_depth () const
  {
    return TREE->depth();
  }
  //----------------------------------------------------------------------------
  inline int const&forces::root_number() const
  {
    return number(TREE->root());
  }
  //----------------------------------------------------------------------------
  inline real const&forces::root_mass  () const
  {
    return mass(GRAV->root());
  }
  //---------------------------------------------------------------------------+
  //                                                                           |
  // ESTIMATION OF DENSITIES                                                   |
  //                                                                           |
  //---------------------------------------------------------------------------+
  inline void forces::estimate_n(unsigned Nx, bool all) falcON_THROWING
  {
    if(BODIES && !BODIES->have(fieldbit::r))
      error("[forces::estimate_nd()]: nobody has memory for rho");
    GRAV->estimate_nd(all,Nx);
  }
  //----------------------------------------------------------------------------
  inline void forces::estimate_sd(unsigned Nx, bool all) falcON_THROWING
  {
    if(BODIES && !BODIES->have(fieldbit::r))
      error("[forces::estimate_sd()]: nobody has memory for rho");
    GRAV->estimate_sd(all,Nx);
  }
  //----------------------------------------------------------------------------
  inline void forces::estimate_rho(unsigned Nx, bool all) falcON_THROWING
  {
    if(BODIES && !BODIES->have(fieldbit::r))
      error("[forces::estimate_md()]: nobody has memory for rho");
    GRAV->estimate_md(all,Nx);
  }
  //---------------------------------------------------------------------------+
  //                                                                           |
  // APPROXIMATION OF GRAVITY                                                  |
  //                                                                           |
  //---------------------------------------------------------------------------+
  inline void forces::approximate_gravity(bool     split,
					  bool     all
#ifdef falcON_ADAP
					 ,real     Nsoft,
					  unsigned Nref,
					  real     emin,
					  real     efac
#  define ADAP_ARGS ,Nsoft,Nref,emin,efac
#else
#  define ADAP_ARGS
#endif
					  ) falcON_THROWING
  {
    GRAV->approx(GMAC,all,split ADAP_ARGS);
  }
  //----------------------------------------------------------------------------
  inline void forces::exact_gravity(bool     all
#ifdef falcON_ADAP
				   ,real     Nsoft,
				    unsigned Nref,
				    real     emin,
				    real     efac
#endif
				    ) falcON_THROWING
  {
    GRAV->exact(all ADAP_ARGS);
  }
  //---------------------------------------------------------------------------+
  //                                                                           |
  // SPH SUPPORT                                                               |
  //                                                                           |
  //---------------------------------------------------------------------------+
#ifdef falcON_SPH
  inline void forces::adjust_SPH_sizes(real mu, real hm, real dm, bool al,
				       unsigned ix) falcON_THROWING
  {
    SPHT->adjust_sizes(mu,hm,dm,al,ix);
  }
  //----------------------------------------------------------------------------
  inline int forces::SPH_sweep_one(real mu, real dm, real hm, real wf,
				   bool al) falcON_THROWING
  
  {
    return SPHT->sweep_one(mu,dm,hm,wf,al);
  }
  //----------------------------------------------------------------------------
  inline void forces::SPH_sweep_two(const EquationOfState*e,
				    real a) falcON_THROWING
  
  {
    SPHT->sweep_two(e,a);
  }
  //----------------------------------------------------------------------------
  inline unsigned const&forces::N_MuSmall() const {
    return SPHT->N_MuSmall();
  }
  //----------------------------------------------------------------------------
  inline unsigned const&forces::N_MuLarge() const {
    return SPHT->N_MuLarge();
  }
  //----------------------------------------------------------------------------
  inline unsigned const&forces::N_HatMax () const {
    return SPHT->N_HatMax ();
  }
  //----------------------------------------------------------------------------
  inline unsigned forces::N_SPH_active(bool all) const {
    if(BODIES->N_bodies(bodytype::gas) == 0) 
      return 0;
    if(all) 
      return BODIES->N_bodies(bodytype::gas);
    unsigned n=0;
    LoopSPHBodies(BODIES,B) if(is_active(B)) ++n;
    return n;
  }
#endif
  //---------------------------------------------------------------------------+
  //                                                                           |
  // NEIGHBOUR FINDING AND COUNTING                                            |
  //                                                                           |
  //---------------------------------------------------------------------------+
  inline void forces::make_iaction_list(indx_pair *bl,
                                        uint       nl,
                                        uint      &na,
                                        bool       Max,
                                        real       tau,
					bool       count) falcON_THROWING
  {
    if(PAES==0) PAES = new PartnerEstimator(TREE);
    if(tau < zero) PAES->make_sph_list   (bl,nl,na,Max,count);
    else           PAES->make_sticky_list(bl,nl,na,tau,count);
  }
  //---------------------------------------------------------------------------+
  inline void forces::count_sph_partners(bool Max) falcON_THROWING
  {
    if(PAES==0) PAES = new PartnerEstimator(TREE);
    PAES->count_sph_partners(Max);
  }
  //////////////////////////////////////////////////////////////////////////////
} // namespace falcON {
falcON_TRAITS(falcON::forces,"forces");
////////////////////////////////////////////////////////////////////////////////
#endif // falcON_included_forces_cc