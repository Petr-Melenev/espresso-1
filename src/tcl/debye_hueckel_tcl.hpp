/*
  Copyright (C) 2010,2011,2012,2013 The ESPResSo project
  Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009,2010 
    Max-Planck-Institute for Polymer Research, Theory Group
  
  This file is part of ESPResSo.
  
  ESPResSo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  ESPResSo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>. 
*/
#ifndef DEBYE_HUECKEL_TCL_H
#define DEBYE_HUECKEL_TCL_H
/** \file debye_hueckel_tcl.hpp
 * Tcl interface for \ref debye_hueckel.h
 */

#include "parser.hpp"

#ifdef ELECTROSTATICS

///
int tclprint_to_result_dh(Tcl_Interp *interp);
///
int tclcommand_inter_coulomb_parse_dh(Tcl_Interp * interp, int argc, char ** argv);

#endif

#endif
