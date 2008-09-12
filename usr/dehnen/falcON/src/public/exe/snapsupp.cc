// -*- C++ -*-                                                                  
////////////////////////////////////////////////////////////////////////////////
///                                                                             
/// \file   src/public/exe/snapsupp.cc                                          
///                                                                             
/// \author Walter Dehnen                                                       
/// \date   2008                                                                
///                                                                             
////////////////////////////////////////////////////////////////////////////////
//                                                                              
// Copyright (C) 2008 Walter Dehnen                                             
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
//                                                                              
// history:                                                                     
//                                                                              
// v 0.0   02/04/2008  WD created.                                              
// v 0.0.1 10/09/2008  WD created.                                              
////////////////////////////////////////////////////////////////////////////////
#define falcON_VERSION   "0.0"
#define falcON_VERSION_D "02-apr-2008 Walter Dehnen                          "
//-----------------------------------------------------------------------------+
#ifndef falcON_NEMO                                // this is a NEMO program    
#  error You need NEMO to compile "snapsupp"
#endif
#define falcON_RepAction 0                         // no action reporting       
//-----------------------------------------------------------------------------+
#include <body.h>                                  // bodies etc..              
#include <public/io.h>                             // WDs C++ NEMO I/O          
#include <public/bodyfunc.h>                       // body functions            
#include <main.h>                                  // NEMO basics & main        
#include <cstdio>                                  // C std I/O                 
//------------------------------------------------------------------------------
const char*defv[] = {
  "in=???\n         input snapshot file                                ",
  "out=???\n        output snapshot file                               ",
  "times=all\n      times to process                                   ",
  "copy=\n          which data to copy (default: all + prop)           ",
  "prop=???\n       property to set (single char)                      ",
  "value=???\n      bodyfunc: value to be assigned to prop             ", 
  "pars=\n          parameters for value, if any                       ",
  falcON_DEFV, NULL };
//------------------------------------------------------------------------------
const char*usage = "snapsupp -- supplement snapshot";
//------------------------------------------------------------------------------
namespace {
  using namespace falcON;
  using falcON::real;
  fieldbit  PROP;
  Bodyfunc *FUNC;
  snapshot  SHOT;
  template<int BIT> struct SetVal {
    static void act(int const&) {
      if(BIT == value(PROP)) {
	BodyProp<BIT> BP(FUNC, SHOT.time());
	for(bodytype T; T; ++T)
	  if(T.allows(PROP))
	    LoopTypedBodies(&SHOT,B,T)
	      B.datum<BIT>() = BP(B);
      }
    }
  };
}
//------------------------------------------------------------------------------
void falcON::main() falcON_THROWING {
  nemo_in IN(getparam("in"));
  nemo_out OUT;
  PROP = fieldbit(getparam("prop")[0]);
  FUNC = new Bodyfunc(getparam("value"),getparam_z("pars"));
  fieldset COPY(getioparam_a("copy")), NEED(COPY | FUNC->need()), READ;
  COPY |= fieldset(PROP);
  while(IN.has_snapshot()) {
    if(! SHOT.read_nemo(IN,READ,NEED,getparam("times"),0)) continue;
    check_sufficient(READ,FUNC->need());
    SHOT.add_field(PROP);
    LoopAllFields<SetVal>::const_loop(0);
    if(!OUT) OUT.open(getparam("out"));
    SHOT.write_nemo(OUT,COPY);
  }
  falcON_DEL_O(FUNC);
}
