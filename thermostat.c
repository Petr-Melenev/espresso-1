// This file is part of the ESPResSo distribution (http://www.espresso.mpg.de).
// It is therefore subject to the ESPResSo license agreement which you accepted upon receiving the distribution
// and by which you are legally bound while utilizing this file in any form or way.
// There is NO WARRANTY, not even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// You should have received a copy of that license along with this program;
// if not, refer to http://www.espresso.mpg.de/license.html where its current version can be found, or
// write to Max-Planck-Institute for Polymer Research, Theory Group, PO Box 3148, 55021 Mainz, Germany.
// Copyright (c) 2002-2003; all rights reserved unless otherwise stated.
/** \file thermostat.c
    Implementation of \ref thermostat.h "thermostat.h"
 */
#include "thermostat.h"

/* thermostat switch */
int thermo_switch = THERMO_LANGEVIN;
/** Temperature */
double temperature = -1.0;

/* LANGEVIN THERMOSTAT */
/* Langevin friction coefficient gamma. */
double langevin_gamma = 0.0;
/* Friction coefficient gamma for rotation */
double langevin_gamma_rotation;

/* DPD THERMOSTAT */
/* DPD friction coefficient gamma. */
double dpd_gamma = 0.0;
/* DPD thermostat cutoff */
double dpd_r_cut = 0.0;
/* inverse off DPD thermostat cutoff */
double dpd_r_cut_inv = 0.0;

/* NPT ISOTROPIC THERMOSTAT */
// INSERT COMMENT
double nptiso_gamma0 = 0.0;
// INSERT COMMENT
double nptiso_gammav = 0.0;


/* prefactors for thermostats */
double langevin_pref1;
double langevin_pref2;
double langevin_pref2_rotation;
#ifdef DPD
double dpd_pref1;
double dpd_pref2;
#endif
#ifdef NPT
double nptiso_pref1;
double nptiso_pref2;
double nptiso_pref3;
double nptiso_pref4;
#endif


int temp_callback(Tcl_Interp *interp, void *_data)
{
  double data = *(double *)_data;
  if (data < 0) {
    Tcl_AppendResult(interp, "Temperature must be non-negative.", (char *) NULL);
    return (TCL_ERROR);
  }
  temperature = data;
  mpi_bcast_parameter(FIELD_TEMPERATURE);
  return (TCL_OK);
}

int langevin_gamma_callback(Tcl_Interp *interp, void *_data)
{
  double data = *(double *)_data;
  if (data < 0) {
    Tcl_AppendResult(interp, "Gamma must be non negativ.", (char *) NULL);
    return (TCL_ERROR);
  }
  langevin_gamma = data;
  mpi_bcast_parameter(FIELD_LANGEVIN_GAMMA);
  return (TCL_OK);
}

int dpd_gamma_callback(Tcl_Interp *interp, void *_data)
{
  double data = *(double *)_data;
  if (data < 0) {
    Tcl_AppendResult(interp, "Gamma must be non negativ.", (char *) NULL);
    return (TCL_ERROR);
  }
  dpd_gamma = data;
  mpi_bcast_parameter(FIELD_DPD_GAMMA);
  return (TCL_OK);
}

int dpd_r_cut_callback(Tcl_Interp *interp, void *_data) {
  double data = *(double *)_data;
  if (data < 0) { 
    Tcl_AppendResult(interp, "dpd_r_cut must be non negativ.", (char *) NULL); 
    return (TCL_ERROR); 
  }
  dpd_r_cut = data;
  mpi_bcast_parameter(FIELD_DPD_RCUT);
  return (TCL_OK);
}

int nptiso_g0_callback(Tcl_Interp *interp, void *_data) {
  double data = *(double *)_data;
  if (data < 0) { 
    Tcl_AppendResult(interp, "Gamma_0 must be non negativ.", (char *) NULL); 
    return (TCL_ERROR); }
  nptiso_gamma0 = data;
  mpi_bcast_parameter(FIELD_NPTISO_G0);
  return (TCL_OK);
}

int nptiso_gv_callback(Tcl_Interp *interp, void *_data) {
  double data = *(double *)_data;
  if (data < 0) { 
    Tcl_AppendResult(interp, "Gamma_V must be non negativ.", (char *) NULL); 
    return (TCL_ERROR); 
  }
  nptiso_gammav= data;
  mpi_bcast_parameter(FIELD_NPTISO_GV);
  return (TCL_OK);
}

int thermo_parse_off(Tcl_Interp *interp, int argc, char **argv) 
{
  double zero=0.0;
  /* set temperature to zero */
  temp_callback(interp, &zero);
  /* langevin thermostat */
  langevin_gamma_callback(interp,  &zero);
#ifdef DPD
  /* dpd thermostat */  
  dpd_gamma_callback(interp,  &zero);
  dpd_r_cut_callback(interp,  &zero);
#endif
#ifdef NPT
  /* npt isotropic thermostat */
  nptiso_g0_callback(interp,  &zero);
  nptiso_gv_callback(interp,  &zero);
#endif
  /* switch thermostat off */
  thermo_switch = THERMO_OFF;
  mpi_bcast_parameter(FIELD_THERMO_SWITCH);
  return (TCL_OK);
}


int thermo_parse_langevin(Tcl_Interp *interp, int argc, char **argv) 
{
  double temp, gamma;

  /* check number of arguments */
  if (argc < 4) {
    Tcl_AppendResult(interp, "wrong # args:  should be \n\"",
		     argv[0]," ",argv[1]," <temp> <gamma>\"", (char *)NULL);
    return (TCL_ERROR);
  }

  /* check argument types */
  if ( !ARG_IS_D(2, temp) || !ARG_IS_D(3, gamma)) {
    Tcl_AppendResult(interp, argv[0]," ",argv[1]," needs two DOUBLES", (char *)NULL);
    return (TCL_ERROR);
  }

  /* broadcast parameters */
  if(temp_callback(interp, &temp) == TCL_ERROR) return (TCL_ERROR);
  if(langevin_gamma_callback(interp, &gamma) == TCL_ERROR) return (TCL_ERROR);
  thermo_switch = ( thermo_switch | THERMO_LANGEVIN );
  mpi_bcast_parameter(FIELD_THERMO_SWITCH);
  return (TCL_OK);
}
 
#ifdef DPD
int thermo_parse_dpd(Tcl_Interp *interp, int argc, char **argv) 
{
  double temp, gamma, r_cut;

#ifdef ROTATION
    fprintf(stderr,"WARNING: Do not use DPD with ROTATION compiled in\n");
    fprintf(stderr,"         You should first check if a combiantion of a DPD thermostat\n");
    fprintf(stderr,"         for the translational degrees of freedom and a LANGEVIN thermostat\n");
    fprintf(stderr,"         for the rotational ones yields correct physics!\n");
    fprintf(stderr,"         After this you may remove these lines (thermostat.c::thermo_parse_dpd)!\n");
#endif

  /* check number of arguments */
  if (argc < 5) {
    Tcl_AppendResult(interp, "wrong # args:  should be \n\"",
		     argv[0]," ",argv[1]," <temp> <gamma> <r_cut>\"", (char *)NULL);
    return (TCL_ERROR);
  }

  /* check argument types */
  if ( !ARG_IS_D(2, temp) || !ARG_IS_D(3, gamma) || !ARG_IS_D(4, r_cut)) {
    Tcl_AppendResult(interp, argv[0]," ",argv[1]," needs three DOUBLES", (char *)NULL);
    return (TCL_ERROR);
  }

  /* broadcast parameters */
  if(temp_callback(interp, &temp) == TCL_ERROR) return (TCL_ERROR);
  if(dpd_gamma_callback(interp, &gamma) == TCL_ERROR) return (TCL_ERROR);
  if(dpd_r_cut_callback(interp, &r_cut) == TCL_ERROR) return (TCL_ERROR);
  thermo_switch = ( thermo_switch | THERMO_DPD );
  mpi_bcast_parameter(FIELD_THERMO_SWITCH);

  return (TCL_OK);
}
#endif

#ifdef NPT
int thermo_parse_nptiso(Tcl_Interp *interp, int argc, char **argv) 
{
  double temp, gamma0, gammav;
  /* check number of arguments */
  if (argc < 5) {
    Tcl_AppendResult(interp, "wrong # args:  should be \n\"",
		     argv[0]," ",argv[1]," <temp> <gamma0> <gammav>\"", (char *)NULL);
    return (TCL_ERROR);
  }
  /* check argument types */
  if ( !ARG_IS_D(2, temp) || !ARG_IS_D(3, gamma0) || !ARG_IS_D(4, gammav) ) {
    Tcl_AppendResult(interp, argv[0]," ",argv[1]," needs four DOUBLES", (char *)NULL);
    return (TCL_ERROR);
  }
  /* broadcast parameters */
  if(temp_callback(interp, &temp) == TCL_ERROR) return (TCL_ERROR);
  if(nptiso_g0_callback(interp, &gamma0) == TCL_ERROR) return (TCL_ERROR);
  if(nptiso_gv_callback(interp, &gammav) == TCL_ERROR) return (TCL_ERROR);
  thermo_switch = ( thermo_switch | THERMO_NPT_ISO );
  mpi_bcast_parameter(FIELD_THERMO_SWITCH);
  return (TCL_OK);
}
#endif

int thermo_print(Tcl_Interp *interp)
{
  char buffer[TCL_DOUBLE_SPACE];
  /* thermostat not initialized */
  if(temperature == -1.0) {
    Tcl_AppendResult(interp,"{ not initialized } ", (char *)NULL);
    return (TCL_OK);
  }

  /* no thermostat on */
  if(thermo_switch == THERMO_OFF) {
    Tcl_AppendResult(interp,"{ off } ", (char *)NULL);
    return (TCL_OK);
  }

  /* langevin */
  if(thermo_switch & THERMO_LANGEVIN ) {
    Tcl_PrintDouble(interp, temperature, buffer);
    Tcl_AppendResult(interp,"{ langevin ",buffer, (char *)NULL);
    Tcl_PrintDouble(interp, langevin_gamma, buffer);
    Tcl_AppendResult(interp," ",buffer," } ", (char *)NULL);
  }
    
#ifdef DPD
 /* dpd */
  if(thermo_switch & THERMO_DPD) {
    Tcl_PrintDouble(interp, temperature, buffer);
    Tcl_AppendResult(interp,"{ dpd ",buffer, (char *)NULL);
    Tcl_PrintDouble(interp, dpd_gamma, buffer);
    Tcl_AppendResult(interp," ",buffer, (char *)NULL);
    Tcl_PrintDouble(interp, dpd_r_cut, buffer);
    Tcl_AppendResult(interp," ",buffer, " } ", (char *)NULL);
  }
#endif

#ifdef NPT
  /* npt_isotropic */
  if(thermo_switch & THERMO_NPT_ISO) {
    Tcl_PrintDouble(interp, temperature, buffer);
    Tcl_AppendResult(interp,"{ npt_isotropic ",buffer, (char *)NULL);
    Tcl_PrintDouble(interp, nptiso_gamma0, buffer);
    Tcl_AppendResult(interp," ",buffer, (char *)NULL);
    Tcl_PrintDouble(interp, nptiso_gammav, buffer);
    Tcl_AppendResult(interp," ",buffer, " } ", (char *)NULL);
  }
#endif

  return (TCL_OK);
}

int thermo_usage(Tcl_Interp *interp, int argc, char **argv)
{
  Tcl_AppendResult(interp, "Usage of tcl command thermostat:\n", (char *)NULL);
  Tcl_AppendResult(interp, "\"", argv[0], "\" for status return or \n\"", (char *)NULL);
  Tcl_AppendResult(interp, "\"", argv[0], "off\" or \n\"", (char *)NULL);
  Tcl_AppendResult(interp, "\"", argv[0], "langevin <temp> <gamma>\" or \n", (char *)NULL);
#ifdef DPD
  Tcl_AppendResult(interp, "\"", argv[0], "dpd <temp> <gamma> <r_cut>\" or \n", (char *)NULL);
#endif
#ifdef NPT
  Tcl_AppendResult(interp, "\"", argv[0], "npt_isotropic <temp> <gamma0> <gammav>\"", (char *)NULL);
#endif
  return (TCL_ERROR);
}

int thermostat(ClientData data, Tcl_Interp *interp, int argc, char **argv) 
{
  THERMO_TRACE(fprintf(stderr,"%d: thermostat:\n",this_node));

  /* print thermostat status */
  if(argc == 1) return thermo_print(interp);
  
  if (argc < 2) {
    Tcl_AppendResult(interp, "wrong # args: \n", (char *)NULL);
    return thermo_usage(interp, argc, argv);
  }

  if ( ARG1_IS_S("off") )           return thermo_parse_off(interp, argc, argv);
  if ( ARG1_IS_S("langevin") )      return thermo_parse_langevin(interp, argc, argv);
#ifdef DPD
  if ( ARG1_IS_S("dpd") )           return thermo_parse_dpd(interp, argc, argv);
#endif
#ifdef NPT
  if ( ARG1_IS_S("npt_isotropic") )  return thermo_parse_nptiso(interp, argc, argv);
#endif
  
  Tcl_AppendResult(interp, "Unkwon thermostat:\n", (char *)NULL);
  return thermo_usage(interp, argc, argv);
}



void thermo_init_langevin() 
{
  langevin_pref1 = -langevin_gamma/time_step;
  langevin_pref2 = sqrt(24.0*temperature*langevin_gamma/time_step);

#ifdef ROTATION 
  langevin_gamma_rotation = langevin_gamma/3;
  langevin_pref2_rotation = sqrt(24.0*temperature*langevin_gamma_rotation/time_step);
  THERMO_TRACE(fprintf(stderr,"%d: thermo_init_langevin: langevin_gamma_rotation=%f, langevin_pref2_rotation=%f",langevin_gamma_rotation,langevin_pref2_rotation));
#endif
  THERMO_TRACE(fprintf(stderr,"%d: thermo_init_langevin: langevin_pref1=%f, langevin_pref2=%f",langevin_pref1,langevin_pref2));  
}

#ifdef DPD
void thermo_init_dpd()
{
  /* prefactor friction force */
  dpd_pref1 = dpd_gamma/SQR(time_step);
  /* prefactor random force */
  dpd_pref2 = sqrt(24.0*temperature*dpd_gamma/SQR(time_step));
  dpd_r_cut_inv = 1.0/dpd_r_cut;
  THERMO_TRACE(fprintf(stderr,"%d: thermo_init_dpd: dpd_pref1=%f, dpd_pref2=%f, dpd_r_cut_inv=%f\n",
		       this_node,dpd_pref1,dpd_pref2,dpd_r_cut_inv));
}
#endif

#ifdef NPT
void thermo_init_npt_isotropic()
{
  if (piston != 0.0) {
    nptiso_pref1 = -0.5*nptiso_gamma0;
    nptiso_pref2 = sqrt(12.0*temperature*nptiso_gamma0*time_step);
    nptiso_pref3 = -0.5*nptiso_gammav*(1.0/piston)*0.5*time_step;
    nptiso_pref4 = sqrt(12.0*temperature*nptiso_gammav*time_step);
  }
  THERMO_TRACE(fprintf(stderr,"%d: thermo_init_npt_isotropic: nptiso_pref1=%f, nptiso_pref2=%f, nptiso_pref3=%f, nptiso_pref4=%f",nptiso_pref1,nptiso_pref2,nptiso_pref3,nptiso_pref4));
}
#endif

void thermo_init()
{
  if(thermo_switch == THERMO_OFF)      return;
  if(thermo_switch & THERMO_LANGEVIN ) thermo_init_langevin();
#ifdef DPD
  if(thermo_switch & THERMO_DPD)       thermo_init_dpd();
#endif
#ifdef NPT
  if(thermo_switch & THERMO_NPT_ISO)   thermo_init_npt_isotropic();
#endif
}

void friction_thermo_langevin(Particle *p)
{
  int j;
  for ( j = 0 ; j < 3 ; j++) {
#ifdef EXTERNAL_FORCES
    if (!(p->l.ext_flag & COORD_FIXED(j)))
#endif
      p->f.f[j] = langevin_pref1*p->m.v[j] + langevin_pref2*(d_random()-0.5);
  }

  ONEPART_TRACE(if(p->p.identity==check_id) fprintf(stderr,"%d: OPT: LANG f = (%.3e,%.3e,%.3e)\n",this_node,p->f.f[0],p->f.f[1],p->f.f[2]));
  THERMO_TRACE(fprintf(stderr,"%d: Thermo: P %d: force=(%.3e,%.3e,%.3e)\n",this_node,p->p.identity,p->f.f[0],p->f.f[1],p->f.f[2]));
}

#ifdef ROTATION
void friction_thermo_langevin_rotation(Particle *p)
{
  int j;
#ifdef EXTERNAL_FORCES
  if(!(p->l.ext_flag & COORDS_FIX_MASK))
#endif
    {
      for ( j = 0 ; j < 3 ; j++)
	p->f.torque[j] = -langevin_gamma*p->m.omega[j] + langevin_pref2*(d_random()-0.5);

      ONEPART_TRACE(if(p->p.identity==check_id) fprintf(stderr,"%d: OPT: LANG f = (%.3e,%.3e,%.3e)\n",this_node,p->f.f[0],p->f.f[1],p->f.f[2]));
      THERMO_TRACE(fprintf(stderr,"%d: Thermo: P %d: force=(%.3e,%.3e,%.3e)\n",this_node,p->p.identity,p->f.f[0],p->f.f[1],p->f.f[2]));
    }
}
#endif

#ifdef NPT
double friction_thermo_nptiso(void) {
  return ( nptiso_pref3*p_diff + nptiso_pref4*(d_random()-0.5) );
}
#endif


