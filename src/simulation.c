//////////////////////////////////////////////////////////
// QCADesigner                                          //
// Copyright 2002 Konrad Walus                          //
// All Rights Reserved                                  //
// Author: Konrad Walus                                 //
// Email: walus@atips.ca                                //
// WEB: http://www.atips.ca/projects/qcadesigner/       //
//////////////////////////////////////////////////////////
//******************************************************//
//*********** PLEASE DO NOT REFORMAT THIS CODE *********//
//******************************************************//
// If your editor wraps long lines disable it or don't  //
// save the core files that way.                        //
// Any independent files you generate format as you wish//
//////////////////////////////////////////////////////////
// Please use complete names in variables and fucntions //
// This will reduce ramp up time for new people trying  //
// to contribute to the project.                        //
//////////////////////////////////////////////////////////


#include <stdlib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>

#include "globals.h"
#include "nonlinear_approx.h"
#include "stdqcell.h"
#include "simulation.h"
#include "bistable_simulation.h"
#include "run_dig_sim.h"

extern bistable_OP bistable_options ;
extern nonlinear_approx_OP nonlinear_approx_options ;

// -- this is the main simulation procedure -- //
simulation_data *run_simulation (int sim_engine)
  {
  switch (sim_engine)
    {
    case NONLINEAR_APPROXIMATION:
      return run_nonlinear_approx(first_cell, &nonlinear_approx_options);

    case BISTABLE:
      return run_bistable_simulation(first_cell, &bistable_options);
			
    case DIGITAL_SIM:
      return run_digital_simulation(first_cell);
    }
  return NULL ;
}//run_simualtion

/* Eventually, we should be able to calculate the ground state using
   any of the various different engines, not just the nonlinear
   approximation */
void calculate_ground_state (int sim_engine)
  {
//  simulation_data *sim_data = NULL ;
//  int backup = nonlinear_approx_options.number_of_iterations ;
//  nonlinear_approx_options.number_of_iterations = 1 ;
//  sim_data = run_simulation (NONLINEAR_APPROXIMATION) ;
  /* MEMORY LEAK HERE ! sim_data is not freed ! We need to define a
     consistent way of creating and destroying simulation data */
//  nonlinear_approx_options.number_of_iterations = backup ;
  }
