//////////////////////////////////////////////////////////
// QCADesigner                                          //
// Copyright 2002 Konrad Walus                          //
// All Rights Reserved                                  //
// Author: Konrad Walus                                 //
// Email: qcadesigner@gmail.com                         //
// WEB: http://qcadesigner.ca/                          //
//////////////////////////////////////////////////////////
//******************************************************//
//*********** PLEASE DO NOT REFORMAT THIS CODE *********//
//******************************************************//
// If your editor wraps long lines disable it or don't  //
// save the core files that way. Any independent files  //
// you generate format as you wish.                     //
//////////////////////////////////////////////////////////
// Please use complete names in variables and fucntions //
// This will reduce ramp up time for new people trying  //
// to contribute to the project.                        //
//////////////////////////////////////////////////////////
// Contents:                                            //
//                                                      //
// The coherence vector time-dependent simulation       //
// engine.                                              //
//                                                      //
//////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "objects/QCADCell.h"
#include "simulation.h"
#include "coherence_vector.h"
#include "custom_widgets.h"
#include "global_consts.h"
#ifdef GTK_GUI
#include "callback_helpers.h"
#endif /* def GTK_GUI */
#include "intl.h"

// Calculates the magnitude of the 3D energy vector
#define magnitude_energy_vector(P,G) (hypot(2*(G), (P)) * over_hbar)
//(sqrt((4.0*(G)*(G) + (P)*(P))*over_hbar_sqr))
// Calculates the temperature ratio
#define temp_ratio(P,G,T) (hypot((G),(P)*0.5)/((T) * kB))

#ifndef max
#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
#define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

//!Options for the coherence simulation engine
//Added by Marco default values for phase shift (0,0,0,0)
//Added by Faizal: wave_numbers (defaults kx=0, ky=0)
coherence_OP coherence_options = {300, 1.11e-16, 1.11e-16, 1.11e-12, 9.43e-19, 1.41e-20, 0.0, 2.0, 1.5, 1, 1.15, TRUE, FALSE,0,0,0,0,0,0,10,FOUR_PHASE_CLOCKING} ;

typedef struct
  {
  int number_of_neighbours;
  QCADCell **neighbours;
  int *neighbour_layer;
  int *neighbour_index;
  double *Ek;
  double lambda_x;
  double lambda_y;
  double lambda_z;
  double lambda_x_old;
  double lambda_y_old;
  double lambda_z_old;
  double *Ek_array;
  int distDriver;
  int *Driver;
  int ss_neighbours;
  int num_inv;
  } coherence_model;

#ifdef GTK_GUI
extern int STOP_SIMULATION;
#else
static int STOP_SIMULATION = 0 ;
#endif /* def GTK_GUI */

// some often used variables that can be precalculated
typedef struct
  {
  double clock_prefactor;
  double clock_shift;
  double four_pi_over_number_samples;
  double two_pi_over_number_samples;
  double hbar_over_kBT;
	  double over_kBT;
  } coherence_optimizations;

GdkColor clrCorr = {0, 42405,10794,10794} ;
GdkColor clrYellowBckUp = {0, 0xFFFF, 0xFFFF, 0x0000} ;
static GdkColor clrClockBckUp[4] =
{
{0, 0x0000, 0xFFFF, 0x0000},
{0, 0xFFFF, 0x0000, 0xFFFF},
{0, 0x0000, 0xFFFF, 0xFFFF},
{0, 0xFFFF, 0xFFFF, 0xFFFF},
} ;

// instance of the optimization options;
static coherence_optimizations optimization_options;

static double coherence_determine_Ek (QCADCell *cell1, QCADCell *cell2, int layer_separation, coherence_OP *options);
static void coherence_refresh_all_Ek (int number_of_cell_layers, int *number_of_cells_in_layer, QCADCell ***sorted_cells, coherence_OP *options);

//static void run_coherence_iteration (int sample_number, int number_of_cell_layers, int *number_of_cells_in_layer, QCADCell ***sorted_cells, int total_number_of_inputs, unsigned long int number_samples, const coherence_OP *options, simulation_data *sim_data, int SIMULATION_TYPE, VectorTable *pvt, double **Kxz, double **Kzz, double **Kxz_copy, double **Kzz_copy, double **a, double *b, double *c, int num_cells, double Ek_max, double Ek_min);
static void run_coherence_iteration (QCADCell ***sorted_cells, int total_number_of_inputs, unsigned long int number_samples, const coherence_OP *options, int SIMULATION_TYPE, VectorTable *pvt, double **a, double *b, double *c, int num_cells, double Ek_max, double Ek_min, double **k, double *cv, double t);
static void run_coherence_iteration_hf (QCADCell ***sorted_cells, int total_number_of_inputs, unsigned long int number_samples, const coherence_OP *options, int SIMULATION_TYPE, VectorTable *pvt, double **a, double *b, double *c, int num_cells, double Ek_max, double Ek_min, double **k, double *cv, double t);

static inline double calculate_clock_value (unsigned int clock_num, double sample, unsigned long int number_samples, int total_number_of_inputs, const coherence_OP *options, int SIMULATION_TYPE, VectorTable *pvt);
static inline double calculate_clock_value_cc (QCADCell *cell, double sample, unsigned long int number_samples, int total_number_of_inputs, const coherence_OP *options, int SIMULATION_TYPE, VectorTable *pvt);
static inline double lambda_ss_x (double t, double PEk, double Gamma, const coherence_OP *options);
static inline double lambda_ss_y (double t, double PEk, double Gamma, const coherence_OP *options);
static inline double lambda_ss_z (double t, double PEk, double Gamma, const coherence_OP *options);
static inline double get_max_array(double *array, int num_elements);
static inline void matrix_copy(double **Mat1, int dimx, int dimy, double **Mat0);

static void lambda_next (QCADCell ***sorted_cells, double t, int num_cells, const coherence_OP *options, unsigned long int number_samples, int total_number_of_inputs, int SIMULATION_TYPE, VectorTable *pvtdouble, double Ek_max, double Ek_min, double *cv, double *cv_old);
static void lambda_next_hf (QCADCell ***sorted_cells, double t, int num_cells, const coherence_OP *options, unsigned long int number_samples, int total_number_of_inputs, int SIMULATION_TYPE, VectorTable *pvtdouble, double Ek_max, double Ek_min, double *cv, double *cv_old);

static inline double lambda_x_next (double t, double PEk, double Gamma, double lambda_x, double lambda_y, double lambda_z, const coherence_OP *options);
static inline double lambda_y_next (double t, double PEk, double Gamma, double lambda_x, double lambda_y, double lambda_z, const coherence_OP *options);
static inline double lambda_z_next (double t, double PEk, double Gamma, double lambda_x, double lambda_y, double lambda_z, const coherence_OP *options);

static inline double slope_x (double t, double PEk, double Gamma, double lambda_x, double lambda_y, double lambda_z, const coherence_OP *options);
static inline double slope_y (double t, double PEk, double Gamma, double lambda_x, double lambda_y, double lambda_z, const coherence_OP *options);
static inline double slope_z (double t, double PEk, double Gamma, double lambda_x, double lambda_y, double lambda_z, const coherence_OP *options);
static int compareCoherenceQCells (const void *p1, const void *p2) ;

static inline void get_steadystate (int *number_of_cells_in_layer, QCADCell ***sorted_cells, double **Ek_matrix, int total_number_of_inputs, const coherence_OP *options);
static inline int find_path (double **Ek_matrix, int driver, int *fork, double Ek_max, double Ek_min, int num_cells, int distance, QCADCell ***sorted_cells, int num_seen);

static inline int search_matrix_row(double **A, double cmp, int row, int num_elements);
static inline int find_matrix_row(double **A, double cmp, int row, int num_elements, int select);
static inline int search_array(double *A, double cmp, int num_elements );
static inline int search_array_int(int *A, double cmp, int num_elements );
static inline int search_array_thresh(double *A, double cmp, int num_elements );
static inline int find_array(double *A, double cmp, int num_elements);
static inline int find_in_array(double *A, double cmp, int num_elements, int select);
static inline double get_max(double **A, int num_elements);
static inline double get_min(double **A, int num_elements);
static inline void compare_array(double *A, double *B, int length, double *Out);
static inline void array_copy(double *Arr1, double *Arr0, int length);
static inline void add_array(double *Arr1, double *Arr2, int length, double *Arr0);
static inline void mult_array_by_constant(double *Arr1, double constant, int length);
static inline void matrix_mult(double **A, double **B, double **C, int numrA, int numcB, int numcA);
static inline void matrix_array_mult(double *A, double **B, double *C, int numcB, int numcA);

//-------------------------------------------------------------------//
// -- this is the main simulation procedure -- //
//-------------------------------------------------------------------//
simulation_data *run_coherence_simulation (int SIMULATION_TYPE, DESIGN *design, coherence_OP *options, VectorTable *pvt)
  {
  
#ifdef GTK_GUI
	  GdkColormap *clrmap = gdk_colormap_get_system () ;
#endif /* def GTK_GUI */
	  
#ifdef GTK_GUI
	  if (0 == clrCorr.pixel)
		  gdk_colormap_alloc_color (clrmap, &clrCorr, FALSE, TRUE) ;
	  if (0 == clrYellowBckUp.pixel)
		  gdk_colormap_alloc_color (clrmap, &clrYellowBckUp, FALSE, TRUE) ;
	  if (0 == clrClockBckUp[0].pixel)
		  gdk_colormap_alloc_color (clrmap, &clrClockBckUp[0], FALSE, TRUE) ;
	  if (0 == clrClockBckUp[1].pixel)
		  gdk_colormap_alloc_color (clrmap, &clrClockBckUp[1], FALSE, TRUE) ;
	  if (0 == clrClockBckUp[2].pixel)
		  gdk_colormap_alloc_color (clrmap, &clrClockBckUp[2], FALSE, TRUE) ;
	  if (0 == clrClockBckUp[3].pixel)
		  gdk_colormap_alloc_color (clrmap, &clrClockBckUp[3], FALSE, TRUE) ;
#endif /* def GTK_GUI */
	  
  int i,j, k, l, q, number_of_cell_layers, *number_of_cells_in_layer;
  QCADCell ***sorted_cells = NULL ;
  int total_number_of_inputs = design->bus_layout->inputs->icUsed;
  unsigned long int number_samples;
  //number of points to record in simulation results //
  //simulations can have millions of points and there is no need to plot them all //
  unsigned long int number_recorded_samples = 3000;
  unsigned long int record_interval;
  double PEk = 0;
  double Eks = 0;
  gboolean stable;
  double old_lambda_x;
  double old_lambda_y;
  double old_lambda_z;
  double average_power=0;
  time_t start_time, end_time;
  simulation_data *sim_data = NULL ;
  // for randomization
  int Nix, Nix1, idxCell1, idxCell2 ;
  QCADCell *swap = NULL ;
  BUS_LAYOUT_ITER bli ;
  double dPolarization = 2.0 ;
  int idxMasterBitOrder = -1.0 ;
	  double **a = NULL;
	  //double *b4 = NULL;
	  double *b = NULL;
	  double *c = NULL;
	  
      int num_cells = 0;	
	
	  double **Ek_matrix = NULL;
      double **k_corr = NULL;
      double **k_hf = NULL;
      double *cv_corr = NULL;
	  double *cv_hf = NULL;
      
  STOP_SIMULATION = FALSE;

  // -- get the starting time for the simulation -- //
  if ((start_time = time (NULL)) < 0)
    fprintf (stderr, "Could not get start time\n");

  // determine the number of samples from the user options //
  number_samples = (unsigned long int)(ceil (options->duration/options->time_step));

  // if the number of samples is larger then the number of recorded samples then change the
  // time step to ensure only number_recorded_samples is used //
  if (number_recorded_samples >= number_samples)
    {
    number_recorded_samples = number_samples;
    record_interval = 1;
    }
  else
    record_interval = (unsigned long int)ceil ((double)(number_samples - 1) / (double)(number_recorded_samples));

  //fill in some of the optimizations
  optimization_options.clock_prefactor = (options->clock_high - options->clock_low) * options->clock_amplitude_factor;
  optimization_options.clock_shift = (options->clock_high + options->clock_low) * 0.5;
  optimization_options.four_pi_over_number_samples = FOUR_PI / (double)number_samples;
  optimization_options.two_pi_over_number_samples = TWO_PI / (double)number_samples;
  optimization_options.hbar_over_kBT = hbar / (kB * options->T);
  optimization_options.over_kBT = 1 / (kB * options->T);

  // -- spit out some messages for possible debugging -- //
  command_history_message ("About to start the coherence vector simulation with %d samples\n", number_samples);
  command_history_message ("%d samples will be recorded for graphing.\n", number_recorded_samples);
  set_progress_bar_visible (TRUE) ;
  set_progress_bar_label ("Coherence vector simulation:") ;
  set_progress_bar_fraction (0.0) ;

  // Fill in the cell arrays necessary for conducting the simulation
  simulation_inproc_data_new (design, &number_of_cell_layers, &number_of_cells_in_layer, &sorted_cells) ;

      if (number_of_cell_layers > 1) {
          STOP_SIMULATION = TRUE;
          command_history_message ("Cannot have multiple cell layers!\n");
          command_history_message ("Simulation was terminated.\n");
          return sim_data;
      }
      
  // determine which cells are inputs and which are outputs //
  for(i = 0; i < number_of_cell_layers; i++)
    for(j = 0; j < number_of_cells_in_layer[i]; j++)
      {
	  num_cells = num_cells+1;
      // attach the model parameters to each of the simulation cells //
      sorted_cells[i][j]->cell_model = g_malloc0 (sizeof(coherence_model));
		  		  
      // -- Clear the model pointers so they are not dangling -- //
      ((coherence_model *)sorted_cells[i][j]->cell_model)->neighbours = NULL;
      ((coherence_model *)sorted_cells[i][j]->cell_model)->Ek = NULL;
	  ((coherence_model *)sorted_cells[i][j]->cell_model)->Ek_array = NULL;
	  
      }

  // if we are performing a vector table simulation we consider only the activated inputs //
  if (VECTOR_TABLE == SIMULATION_TYPE)
    for (total_number_of_inputs = 0, Nix = 0 ; Nix < pvt->inputs->icUsed ; Nix++)
      {
      if (exp_array_index_1d (pvt->inputs, VT_INPUT, Nix).active_flag)
        total_number_of_inputs++ ;
      else
        // Kill the input flag for inactive inputs, so they may be correctly simulated
        exp_array_index_1d (pvt->inputs, VT_INPUT, Nix).input->cell_function = QCAD_CELL_NORMAL ;
      }

  // write message to the command history window //
  command_history_message ("Simulation found %d inputs %d outputs\n", total_number_of_inputs, design->bus_layout->outputs->icUsed) ;

  // -- Allocate memory to hold the simulation data -- //
  sim_data = g_malloc0 (sizeof(simulation_data)) ;

  // -- Initialize the simualtion data structure -- //
  sim_data->number_of_traces = design->bus_layout->inputs->icUsed + design->bus_layout->outputs->icUsed;

  // set the number of simulation samples to be the desired number of recorded samples //
  sim_data->number_samples = number_recorded_samples;

  // allocate the memory for each trace //
  sim_data->trace = g_malloc0 (sizeof (struct TRACEDATA) * sim_data->number_of_traces);

  // create and initialize the inputs into the sim data structure //
  for (i = 0; i < design->bus_layout->inputs->icUsed; i++)
    {
    sim_data->trace[i].data_labels = g_strdup (qcad_cell_get_label (exp_array_index_1d (design->bus_layout->inputs, BUS_LAYOUT_CELL, i).cell));
    sim_data->trace[i].drawtrace = TRUE;
    sim_data->trace[i].trace_function = QCAD_CELL_INPUT;
    sim_data->trace[i].data = g_malloc0 (sim_data->number_samples * sizeof (double));
    }

  // create and initialize the outputs into the sim data structure //
  for (i = 0; i < design->bus_layout->outputs->icUsed; i++)
    {
    sim_data->trace[i + total_number_of_inputs].data_labels = g_strdup (qcad_cell_get_label (exp_array_index_1d (design->bus_layout->outputs, BUS_LAYOUT_CELL, i).cell));
    sim_data->trace[i + total_number_of_inputs].drawtrace = TRUE;
    sim_data->trace[i + total_number_of_inputs].trace_function = QCAD_CELL_OUTPUT;
    sim_data->trace[i + total_number_of_inputs].data = g_malloc0 (sim_data->number_samples * sizeof (double));
    }

//  if (options->clocking == four_phase_clocking) {
  // create and initialize the clock data //
  sim_data->clock_data = g_malloc0 (sizeof (struct TRACEDATA) * 4);

  for (i = 0; i < 4; i++)
    {
    sim_data->clock_data[i].data_labels = g_strdup_printf ("CLOCK %d", i);
    sim_data->clock_data[i].drawtrace = 1;
    sim_data->clock_data[i].trace_function = QCAD_CELL_FIXED;
    if (NULL == (sim_data->clock_data[i].data = g_malloc0 (sim_data->number_samples * sizeof (double))))
      printf("Could not allocate memory for clock data\n");

    // fill in the clock data for the simulation results //
    for (j = 0; j<sim_data->number_samples; j++)
      //printf("j=%d, j*record_interval = %d\n",j,j*record_interval);
      sim_data->clock_data[i].data[j] = calculate_clock_value(i, j * record_interval, number_samples, total_number_of_inputs, options, SIMULATION_TYPE, pvt);
 //   }
  }
  // -- refresh all the kink energies and neighbours-- //
  coherence_refresh_all_Ek (number_of_cell_layers, number_of_cells_in_layer, sorted_cells, options);
  
      int num_entries = 3*num_cells + 2*num_cells*num_cells;
      
      cv_corr = (double*)malloc(num_entries*sizeof(double));
      cv_hf = (double*)malloc(3*num_cells*sizeof(double));
      
      k_corr = (double**)malloc(7*sizeof(double*));
      k_hf = (double**)malloc(7*sizeof(double*));
      
      
      for (i = 0; i < 7; i++) {
          k_corr[i] = (double*)malloc(num_entries*sizeof(double));
          k_hf[i] = (double*)malloc(3*num_cells*sizeof(double));
      }
      
      for (i = 0; i < 7; i++) {
          for (j = 0; j < num_entries; j++) {
              k_corr[i][j] = 0;
          }
      }
      
      for (i = 0; i < 7; i++) {
          for (j = 0; j < 3*num_cells; j++) {
              k_hf[i][j] = 0;
          }
      }
      
  // -- sort the cells with respect to the neighbour count -- //
  // -- this is done so that majority gates are evalulated last -- //
  // -- to ensure that all the signals have arrived first -- //
  // -- kept getting wrong answers without this -- //

  // The following line causes a segfault when the design consists of a single cell
//  printf("The Ek to the first cells neighbour is %e [eV]\n",((coherence_model *)sorted_cells[0][0]->cell_model)->Ek[0]/1.602e-19);

	  
  //initialize Butcher Tableau
	  a = (double**)malloc(7*sizeof(double*));
	  //b4 = (double*)malloc(7*sizeof(double));
	  b = (double*)malloc(7*sizeof(double));
	  c = (double*)malloc(7*sizeof(double));
	  
	  for (i = 0; i < 7; i++) {
		  a[i] = (double*)malloc(7*sizeof(double));
	  }
	  
	  for (i = 0; i < 7; i++) {
		  for (j = 0; j < 7; j++) {
			  a[i][j] = 0;
		  }
	  }
	  
      a[1][0] = 0.2;
	  a[2][0] = 0.075;
	  a[2][1] = 0.225;
	  a[3][0] = 44.0/45.0;
	  a[3][1] = -56.0/15.0;
	  a[3][2] = 32.0/9.0;
	  a[4][0] = 19372.0/6561.0;
	  a[4][1] = -25360.0/2187.0;
	  a[4][2] = 64448.0/6561.0;
	  a[4][3] = -212.0/729.0;
	  a[5][0] = 9017.0/3168.0;
	  a[5][1] = -355.0/33.0;
	  a[5][2] = 46732.0/5247.0;
	  a[5][3] = 49.0/176.0;
	  a[5][4] = -5103.0/18656.0;
	  a[6][0] = 35.0/384.0;
	  a[6][1] = 0;
	  a[6][2] = 500.0/1113.0;
	  a[6][3] = 125.0/192.0;
	  a[6][4] = -2187.0/6784.0;
	  a[6][5] = 11.0/84.0; 
	  
	  /*
	  b4[0] = 5179.0/57600.0;
	  b4[1] = 0;
	  b4[2] = 7571.0/16695.0;
	  b4[3] = 393.0/640.0;
	  b4[4] = -92097.0/339200.0;
	  b4[5] = 187.0/2100.0;
	  b4[6] = 1.0/40.0;
	  */
	   
	  b[0] = 35.0/384.0;
	  b[1] = 0;
	  b[2] = 500.0/1113.0;
	  b[3] = 125.0/192.0;
	  b[4] = -2187.0/6784.0;
	  b[5] = 11.0/84.0;
	  b[6] = 0;
	  
	  c[0] = 0;
	  c[1] = 0.2;
	  c[2] = 0.3;
	  c[3] = 0.8;
	  c[4] = 8.0/9.0;
	  c[5] = 1;
	  c[6] = 1;
	  
	  //End initialize Butcher Tableau
	  
	  
	  
  if (EXHAUSTIVE_VERIFICATION == SIMULATION_TYPE)
    for (design_bus_layout_iter_first (design->bus_layout, &bli, QCAD_CELL_INPUT, &i) ; i > -1 ; design_bus_layout_iter_next (&bli, &i))
      qcad_cell_set_polarization (exp_array_index_1d (design->bus_layout->inputs, BUS_LAYOUT_CELL, i).cell, 
        sim_data->trace[i].data[0] = -1) ;
  else
//  if (VECTOR_TABLE == SIMULATION_TYPE)
    for (design_bus_layout_iter_first (design->bus_layout, &bli, QCAD_CELL_INPUT, &i) ; i > -1 ; design_bus_layout_iter_next (&bli, &i))
      if (exp_array_index_1d (pvt->inputs, VT_INPUT, i).active_flag)
        qcad_cell_set_polarization (exp_array_index_1d (pvt->inputs, VT_INPUT, i).input,
          sim_data->trace[i].data[0] = exp_array_index_2d (pvt->vectors, gboolean, 0, i) ? 1 : -1) ;
	  
	  //Create Ek_matrix
	  Ek_matrix = (double**)malloc(number_of_cells_in_layer[0]*sizeof(double*));
	  for (i = 0; i < number_of_cells_in_layer[0]; i++) {
		  Ek_matrix[i] = (double*)malloc(number_of_cells_in_layer[0]*sizeof(double));
	  }
	  
	  for(i = 0; i < number_of_cells_in_layer[0]; i++) {		  
		  ((coherence_model *)sorted_cells[0][i]->cell_model)->Ek_array = (double*)malloc(number_of_cells_in_layer[0]*sizeof(double));
		  for(j = 0; j < number_of_cells_in_layer[0]; j++) {
			  if (i == j) {
				  Ek_matrix[i][j] = 0;
				  ((coherence_model *)sorted_cells[0][i]->cell_model)->Ek_array[j] = 0;
			  }
			  else {
				  Ek_matrix[i][j] = coherence_determine_Ek (sorted_cells[0][i], sorted_cells[0][j], 0, options);
				  ((coherence_model *)sorted_cells[0][i]->cell_model)->Ek_array[j] = Ek_matrix[i][j];
                  //printf("Ek = %e\n",Ek_matrix[i][j]);
			  }
		  }
	  }
	  
      double Ek_max = get_max(Ek_matrix, num_cells);
      double Ek_min = get_min(Ek_matrix, num_cells);
      if (Ek_min >= 0) {
          Ek_min = -Ek_max;
      }

  // Converge the steady state coherence vector for each cell so that the simulation starts without any transients //
  stable = FALSE;
  k = 0;
	  int sflag = 0;
  while (!stable)
    {
    stable = TRUE;
		sflag = 0;	
		
    for (i = 0; i < number_of_cell_layers; i++)
      for (j = 0; j < number_of_cells_in_layer[i]; j++)
        {
        if (((QCAD_CELL_INPUT == sorted_cells[i][j]->cell_function)||
             (QCAD_CELL_FIXED == sorted_cells[i][j]->cell_function)))
          {
          continue;
          }

        PEk = 0;
        // Calculate the sum of neighboring polarizations * the kink energy between them//
			for (q = 0; q < ((coherence_model *)sorted_cells[i][j]->cell_model)->number_of_neighbours; q++)
				PEk += (qcad_cell_calculate_polarization (((coherence_model *)sorted_cells[i][j]->cell_model)->neighbours[q])) * ((coherence_model *)sorted_cells[i][j]->cell_model)->Ek[q];
    
			
        old_lambda_x = ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_x;
        old_lambda_y = ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_y;
        old_lambda_z = ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_z;

        ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_x = lambda_ss_x(0, PEk, sim_data->clock_data[sorted_cells[i][j]->cell_options.clock].data[0], options);
        ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_y = lambda_ss_y(0, PEk, sim_data->clock_data[sorted_cells[i][j]->cell_options.clock].data[0], options);
        ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_z = lambda_ss_z(0, PEk, sim_data->clock_data[sorted_cells[i][j]->cell_options.clock].data[0], options);

		((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_x_old = ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_x; 	
		((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_y_old = ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_y;
		((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_z_old = ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_z;	
            
        cv_corr[3*j] = ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_x; 
        cv_corr[3*j+1] = ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_y; 
        cv_corr[3*j+2] = ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_z; 
		
            cv_hf[3*j] = ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_x; 
            cv_hf[3*j+1] = ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_y; 
            cv_hf[3*j+2] = ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_z; 
			
        qcad_cell_set_polarization(sorted_cells[i][j], ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_z);

			if (sflag == 0) {
			
        // if the lambda values are different by more then the tolerance then they have not converged //
        stable =
          !(fabs (((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_x - old_lambda_x) > 1e-7 ||
            fabs (((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_y - old_lambda_y) > 1e-7 ||
            fabs (((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_z - old_lambda_z) > 1e-7) ;
        
				if (!stable) {
					sflag = 1;
				}
			}
		}
		
    k++;
    }
	command_history_message ("It took %d iterations to converge the initial steady state polarization\n", k);
	  
	  for (i = 0; i < number_of_cell_layers; i++)
		  for (j = 0; j < number_of_cells_in_layer[i]; j++)
		  {
			  
			  if (((QCAD_CELL_INPUT == sorted_cells[i][j]->cell_function)||
				   (QCAD_CELL_FIXED == sorted_cells[i][j]->cell_function))) {
				  ((coherence_model *)sorted_cells[i][j]->cell_model)->distDriver = 1e5;
				  continue;
			  } 
              ((coherence_model *)sorted_cells[i][j]->cell_model)->distDriver = 1e5;
			  for (k = 0; k < number_of_cells_in_layer[i]; k++)
			  {
				  if (j == k) {
					  cv_corr[(3+j)*num_cells+k] = 0; //kxz(j,k)
                      cv_corr[(3+j+num_cells)*num_cells+k] = 0; //kzz(j,k)
					  continue;
				  }
				  
				  if (((QCAD_CELL_INPUT == sorted_cells[i][k]->cell_function)||
					   (QCAD_CELL_FIXED == sorted_cells[i][k]->cell_function)))
				  {
					  cv_corr[(3+j)*num_cells+k] = 0; //kxz(j,k)
                      cv_corr[(3+j+num_cells)*num_cells+k] = 0; //kzz(j,k)
					  continue;
				  }
				  cv_corr[(3+j)*num_cells+k] = ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_x * ((coherence_model *)sorted_cells[i][k]->cell_model)->lambda_z; 
				  cv_corr[(3+j+num_cells)*num_cells+k] = ((coherence_model *)sorted_cells[i][j]->cell_model)->lambda_z * ((coherence_model *)sorted_cells[i][k]->cell_model)->lambda_z;
			  }
		  }
	  
	  /*
	  int *driver_ind = NULL;
	  driver_ind = (int*)malloc(num_cells*sizeof(int));
	  determine_corr (number_of_cell_layers, number_of_cells_in_layer, sorted_cells, Ek_matrix, num_cells, driver_ind, 0);
	  */
	  
		  
	  		  	  
	  /*
	  for (i = 0 ; i < number_of_cell_layers ; i++) {
		  for (j = 0 ; j < number_of_cells_in_layer[i] ; j++)
		  {
			  if (((coherence_model *)sorted_cells[i][j]->cell_model)->distDriver >= options->dist_thresh) {
				  sorted_cells[i][j]->cell_options.mode = QCAD_CELL_MODE_CLUSTER;
				  memcpy (&(QCAD_DESIGN_OBJECT (sorted_cells[i][j])->clr), &clrCorr, sizeof (GdkColor)) ;
			  }
		  }
	  }
	  */
	  
#ifdef DESIGNER
		  redraw_async(NULL);
		  gdk_flush () ;
#endif /* def DESIGNER */
	  
	  int progress_bar = min(floor(number_samples/100),10000);
	  
      //for power calculations
      double clock_value_k;
      double clock_value_l;
      double lambda_x_k;
      double lambda_y_k;
      double lambda_z_k;
      double lambda_x_l;
      double lambda_y_l;
      double lambda_z_l;
      double Ek_temp_p;
      double Ekd_p, EkP;
      double power_corr;
      double Pol;
      int ss_neighbours;
      int distDriver, num_inv, num_neighbours;
      int fk;
      double lss_z;
      //end for power calculations
      
      get_steadystate (number_of_cells_in_layer[0], sorted_cells, Ek_matrix, total_number_of_inputs, options);
      
      
	  for (j = 0; j < number_samples; j++) {
          power_corr = 0;
		  
		  if (0 == j % progress_bar || j == number_samples - 1)
		  {
			  // Update the progress bar
			  set_progress_bar_fraction ((float) j / (float) number_samples) ;
			  // redraw the design if the user wants it to appear animated or if this is the last sample //
#ifdef DESIGNER
			  if(options->animate_simulation || j == number_samples - 1)
			  {
				  redraw_async(NULL);
				  gdk_flush () ;
			  }
#endif /* def DESIGNER */
		  }
		  // -- for each of the inputs -- //
		  
		  if (EXHAUSTIVE_VERIFICATION == SIMULATION_TYPE) {
			  for (idxMasterBitOrder = 0, design_bus_layout_iter_first (design->bus_layout, &bli, QCAD_CELL_INPUT, &i) ; i > -1 ; design_bus_layout_iter_next (&bli, &i), idxMasterBitOrder++)
			  {
				  qcad_cell_set_polarization (exp_array_index_1d (design->bus_layout->inputs, BUS_LAYOUT_CELL, i).cell,
											  dPolarization = (-sin (((double) (1 << idxMasterBitOrder)) * (double) j * optimization_options.four_pi_over_number_samples)) > 0 ? 1 : -1) ;
				  if (0 == j % record_interval)
					  sim_data->trace[i].data[j/record_interval] = dPolarization ;
			  }
		  }
		  else
			  //    if (VECTOR_TABLE == SIMULATION_TYPE)
			  for (design_bus_layout_iter_first (design->bus_layout, &bli, QCAD_CELL_INPUT, &i) ; i > -1 ; design_bus_layout_iter_next (&bli, &i)) {
				  if (exp_array_index_1d (pvt->inputs, VT_INPUT, i).active_flag)
				  {
					  qcad_cell_set_polarization (exp_array_index_1d (pvt->inputs, VT_INPUT, i).input,
												  dPolarization = exp_array_index_2d (pvt->vectors, gboolean, (j*pvt->vectors->icUsed) / number_samples, i) ? 1 : -1) ;
					  if (0 == j % record_interval)
						  sim_data->trace[i].data[j/record_interval] = dPolarization ;
				  }
			  }
		  
           
		  if (options->include_correlations) {
              run_coherence_iteration (sorted_cells, total_number_of_inputs, number_samples, options, SIMULATION_TYPE, pvt, a,  b,  c, num_cells, Ek_max, Ek_min, k_corr, cv_corr, j*options->time_step);
		  }
		  
		  else {
			  run_coherence_iteration_hf (sorted_cells, total_number_of_inputs, number_samples, options, SIMULATION_TYPE, pvt, a,  b,  c, num_cells, Ek_max, Ek_min, k_hf, cv_hf, j*options->time_step);
		  }
		  
		  
		  // -- Set the cell polarizations to the lambda_z value -- //
		  for (k = 0; k < number_of_cell_layers; k++)
			  for (l = 0; l < number_of_cells_in_layer[k]; l++)
			  {
				  // don't simulate the input and fixed cells //
				  if (((QCAD_CELL_INPUT == sorted_cells[k][l]->cell_function) ||
					   (QCAD_CELL_FIXED == sorted_cells[k][l]->cell_function)))
					  continue;
				  
                  if (options->include_correlations) {
                      ((coherence_model *)sorted_cells[0][l]->cell_model)->lambda_z = cv_corr[3*l+2];
                  }
                  else {
                      ((coherence_model *)sorted_cells[0][l]->cell_model)->lambda_z = cv_hf[3*l+2];
                  }
				  
				  if (fabs (((coherence_model *)sorted_cells[k][l]->cell_model)->lambda_z) > 1.0)
				  {
					  command_history_message ("I had to abort the simulation at iteration %d because the polarization = %e was diverging.\nPossible cause is the time step is too large.\nAlternatively, you can decrease the relaxation time to reduce oscillations.\n",j, ((coherence_model *)sorted_cells[k][l]->cell_model)->lambda_z);
					  command_history_message ("time step was set to %e\n", options->time_step);
					  return sim_data;
					  
				  }
				  
				  ((coherence_model *)sorted_cells[k][l]->cell_model)->lambda_x_old = ((coherence_model *)sorted_cells[k][l]->cell_model)->lambda_x;
				  ((coherence_model *)sorted_cells[k][l]->cell_model)->lambda_y_old = ((coherence_model *)sorted_cells[k][l]->cell_model)->lambda_y;
				  ((coherence_model *)sorted_cells[k][l]->cell_model)->lambda_z_old = ((coherence_model *)sorted_cells[k][l]->cell_model)->lambda_z;	
				  
				  /*	  
				   command_history_message ("I had to abort the simulation at iteration %d because the polarization = %e was diverging.\nPossible cause is the time step is too large.\nAlternatively, you can decrease the relaxation time to reduce oscillations.\n",j, ((coherence_model *)sorted_cells[k][l]->cell_model)->lambda_z);
				   command_history_message ("time step was set to %e\n", options->time_step);
				   return sim_data;
				   }
				   */
				  qcad_cell_set_polarization (sorted_cells[k][l], -((coherence_model *)sorted_cells[k][l]->cell_model)->lambda_z);
				  
			  }
          
          
          /*
          if (options->include_correlations) {
              //calculate the power for 2pt correlations
              for (k = 0; k < number_of_cells_in_layer[0]; k++) {
                  if (((QCAD_CELL_INPUT == sorted_cells[0][k]->cell_function) ||
                       (QCAD_CELL_FIXED == sorted_cells[0][k]->cell_function))) {
                      continue;
                  }
                  EkP = 0;
                  //printf("%d\n",k);
                  distDriver = ((coherence_model *)sorted_cells[0][k]->cell_model)->distDriver;
                  num_inv = ((coherence_model *)sorted_cells[0][k]->cell_model)->num_inv;
                  num_neighbours = ((coherence_model *)sorted_cells[0][k]->cell_model)->number_of_neighbours;
                  clock_value_k = calculate_clock_value (0, j, number_samples, total_number_of_inputs, options, SIMULATION_TYPE, pvt);
                  lambda_x_k = cv_corr[3*k];
                  lambda_y_k = cv_corr[3*k+1];
                  lambda_z_k = cv_corr[3*k+2];
                  
                  Pol = 0;
                  ss_neighbours = ((coherence_model *)sorted_cells[0][k]->cell_model)->ss_neighbours;
                  for (fk = 0; fk < ss_neighbours; fk++) {
                      Pol += qcad_cell_calculate_polarization (sorted_cells[0][((coherence_model *)sorted_cells[0][k]->cell_model)->Driver[fk]]);
                  }
                  lss_z = -tanh(0.5*Ek_max*Pol*optimization_options.over_kBT)*pow(tanh(0.5*Ek_max*optimization_options.over_kBT),distDriver)*pow(tanh(0.5*Ek_min*optimization_options.over_kBT),num_inv);
                  
                  for (l = 0; l < number_of_cells_in_layer[0]; l++) {
                      if (((QCAD_CELL_INPUT == sorted_cells[0][l]->cell_function) ||
                           (QCAD_CELL_FIXED == sorted_cells[0][l]->cell_function))) {
                          Ekd_p = (qcad_cell_calculate_polarization (((coherence_model *)sorted_cells[0][l])))*coherence_determine_Ek(sorted_cells[0][k], sorted_cells[0][l], 0, options);
                          power_corr += Ekd_p*(-2*clock_value_k*lambda_y_k - (1/options->relaxation)*(lambda_z_k - lss_z));
                          EkP += Ekd_p;
                      }
                      else if (l != k) {
                          clock_value_l = calculate_clock_value (0, j, number_samples, total_number_of_inputs, options, SIMULATION_TYPE, pvt);
                          lambda_x_l = ((coherence_model *)sorted_cells[0][l]->cell_model)->lambda_x;
                          lambda_y_l = ((coherence_model *)sorted_cells[0][l]->cell_model)->lambda_y;
                          lambda_z_l = ((coherence_model *)sorted_cells[0][l]->cell_model)->lambda_z;
                          Ek_temp_p = coherence_determine_Ek(sorted_cells[0][k], sorted_cells[0][l], 0, options);
                          EkP += Ek_temp_p*(qcad_cell_calculate_polarization (((coherence_model *)sorted_cells[0][l])));
                          power_corr += -Ek_temp_p*(-2*clock_value_k*lambda_y_k*lambda_z_l - 2*clock_value_l*lambda_z_k*lambda_y_l - (1/options->relaxation)*(cv_corr[(3+k+num_cells)*num_cells+l] - tanh(0.5*((coherence_model *)sorted_cells[0][k]->cell_model)->Ek_array[l]*optimization_options.over_kBT)));
                          //printf("C%d: %e\n",k,power_corr);
                      }
                  }
                  power_corr += -2*clock_value_k*(-EkP*lambda_y_k - (1/options->relaxation)*(lambda_x_k));
              }
              printf("%e\n", 0.5*power_corr);
          }
          else {
              
              
              //calculate the power for ICHA
              for (k = 0; k < number_of_cells_in_layer[0]; k++) {
                  if (((QCAD_CELL_INPUT == sorted_cells[0][k]->cell_function) ||
                       (QCAD_CELL_FIXED == sorted_cells[0][k]->cell_function))) {
                      continue;
                  }
                  clock_value_k = calculate_clock_value (0, j, number_samples, total_number_of_inputs, options, SIMULATION_TYPE, pvt);
                  lambda_x_k = cv_hf[3*k];
                  lambda_y_k = cv_hf[3*k+1];
                  lambda_z_k = cv_hf[3*k+2];	
                  Ekd_p = 0;
                  for (l = 0; l < number_of_cells_in_layer[0]; l++) {
                      if (l != k) {
                          Ekd_p += (qcad_cell_calculate_polarization (((coherence_model *)sorted_cells[0][l])))*coherence_determine_Ek(sorted_cells[0][k], sorted_cells[0][l], 0, options);
                      }
                  }
                  power_corr += -2*clock_value_k*(Ekd_p*lambda_y_k - (1/options->relaxation)*(lambda_x_k - lambda_ss_x(j*options->time_step, Ekd_p, clock_value_k, options))) + Ekd_p*(-2*clock_value_k*lambda_y_k - (1/options->relaxation)*(lambda_z_k - lambda_ss_z(j*options->time_step, Ekd_p, clock_value_k, options)));
              }
              printf("%e\n", 0.5*power_corr);
          }
          */ 
		  
		  // -- collect all the output data from the simulation -- //
		  if (0 == j % record_interval) {
			  for (design_bus_layout_iter_first (design->bus_layout, &bli, QCAD_CELL_OUTPUT, &i) ; i > -1 ; design_bus_layout_iter_next (&bli, &i))
				  sim_data->trace[total_number_of_inputs + i].data[j/record_interval] =
				  qcad_cell_calculate_polarization (exp_array_index_1d (design->bus_layout->outputs, BUS_LAYOUT_CELL, i).cell) ;
              /*
              printf("\n");
              for (k = 0; k < number_of_cells_in_layer[0]; k++) {
                  //if (QCAD_CELL_OUTPUT == sorted_cells[0][k]->cell_function) {
                      printf("%e\t", -((coherence_model *)sorted_cells[0][k]->cell_model)->lambda_z);
                      
                  //}
              }
			 */
          }
		  
		  if (TRUE == STOP_SIMULATION) {
			  for (k = 0 ; k < number_of_cell_layers ; k++) {
				  for (l = 0 ; l < number_of_cells_in_layer[k] ; l++)
				  {
					  if (sorted_cells[k][l]->cell_options.mode == QCAD_CELL_MODE_CLUSTER) {
						  qcad_cell_set_mode (sorted_cells[k][l], QCAD_CELL_MODE_NORMAL);
					  }
				  }
			  }
#ifdef DESIGNER
			  redraw_async(NULL);
			  gdk_flush () ;
#endif /* def DESIGNER */
			  return sim_data;
		  }
	  }//for number of samples
	  
	  
	  
	  for (k = 0 ; k < number_of_cell_layers ; k++) {
		  for (l = 0 ; l < number_of_cells_in_layer[k] ; l++)
		  {
			  if (sorted_cells[k][l]->cell_options.mode == QCAD_CELL_MODE_CLUSTER) {
				  qcad_cell_set_mode (sorted_cells[k][l], QCAD_CELL_MODE_NORMAL);
			  }
		  }
	  }
      
       
      
	  
#ifdef DESIGNER
	  redraw_async(NULL);
	  gdk_flush () ;
#endif /* def DESIGNER */
	  
  // Free the neigbours and Ek array introduced by this simulation//
  for (k = 0; k < number_of_cell_layers; k++)
    for (l = 0; l < number_of_cells_in_layer[k]; l++)
      {
      g_free (((coherence_model *)sorted_cells[k][l]->cell_model)->neighbours);
      g_free (((coherence_model *)sorted_cells[k][l]->cell_model)->neighbour_layer);
	  g_free (((coherence_model *)sorted_cells[k][l]->cell_model)->neighbour_index);
      g_free (((coherence_model *)sorted_cells[k][l]->cell_model)->Ek);
	  g_free (((coherence_model *)sorted_cells[k][l]->cell_model)->Ek_array);
      g_free (((coherence_model *)sorted_cells[k][l]->cell_model)->Driver);    
      }

  simulation_inproc_data_free (&number_of_cell_layers, &number_of_cells_in_layer, &sorted_cells) ;


	  for (l = 0; l < 7; l++) {
		  free(a[l]); a[l] = NULL;
	  }
	  free(a); a = NULL;
	  free(b); b = NULL;
	  free(c); c = NULL;
	  
	  free(Ek_matrix); Ek_matrix = NULL;
	
	  for (l = 0; l < 7; l++) {
          free(k_corr[l]); k_corr[l] = NULL;
          free(k_hf[l]); k_hf[l] = NULL;
      }
	  
      free(k_corr); k_corr = NULL;
      free(k_hf); k_hf = NULL;
      free(cv_corr); cv_corr = NULL;
      free(cv_hf); cv_hf = NULL;
	  
  // Restore the input flag for the inactive inputs
  if (VECTOR_TABLE == SIMULATION_TYPE)
    for (i = 0 ; i < pvt->inputs->icUsed ; i++)
      exp_array_index_1d (pvt->inputs, BUS_LAYOUT_CELL, i).cell->cell_function = QCAD_CELL_INPUT ;

  // -- get and print the total simulation time -- //
  if ((end_time = time (NULL)) < 0)
    fprintf (stderr, "Could not get end time\n");

  command_history_message (_("Total simulation time: %g s\n"), (double)(end_time - start_time));
  set_progress_bar_visible (FALSE) ;
  return sim_data;
  }//run_coherence

// -- completes one simulation iteration performs the approximations until the entire design has stabalized using 2pt correlations -- //
static void run_coherence_iteration (QCADCell ***sorted_cells, int total_number_of_inputs, unsigned long int number_samples, const coherence_OP *options, int SIMULATION_TYPE, VectorTable *pvt, double **a, double *b, double *c, int num_cells, double Ek_max, double Ek_min, double **k, double *cv, double t)
{
    
    int i, j;
    double *hC = NULL;
    double *temp_cv1 = NULL;
    double *temp_cv2 = NULL;
    double *cv_old = NULL;
    
    int num_entries = 3*num_cells + 2*(num_cells*num_cells);
    
    hC = (double*)malloc(7*sizeof(double));
    temp_cv1 = (double*)malloc(num_entries*sizeof(double));
    temp_cv2 = (double*)malloc(num_entries*sizeof(double));
    cv_old = (double*)malloc(num_entries*sizeof(double));
    
    for (j = 0; j < num_entries; j++) {
            temp_cv1[j] = 0;
            temp_cv2[j] = 0;
    }
    
    array_copy(k[6],k[0],num_entries); //copy 7th row to 1st row; i.e, k[0] = k[6]
    
    array_copy(c,hC,7); //copy c into hC
    mult_array_by_constant(hC,options->time_step,7); //multiply hC by time_step
    
    for (i = 1; i < 7; i++) {
        
        matrix_array_mult(a[i],k,temp_cv1, num_entries, 7); //multiply ith row of a (1x7) with k (7xnum_entries)
        mult_array_by_constant(temp_cv1, options->time_step, num_entries); //multiply new array with time_step
        add_array(cv,temp_cv1,num_entries, cv_old); //add current cv to temp_cv1... put into cv_old
        
        lambda_next (sorted_cells, t + hC[i], num_cells, options, number_samples, total_number_of_inputs, SIMULATION_TYPE, pvt, Ek_max, Ek_min, k[i], cv_old); 
    }
    
    matrix_array_mult(b,k,temp_cv2, num_entries, 7); //multiply b (1x7) with NEW k (7xnum_entries)
    mult_array_by_constant(temp_cv2, options->time_step, num_entries); //multiply this new array by time_step
    add_array(cv, temp_cv2, num_entries, cv); //add this new array to the cv array that was passed in... put into temp_cv1
    
    free(temp_cv1); temp_cv1 = NULL;
    free(temp_cv2); temp_cv2 = NULL;
    free(cv_old); cv_old = NULL;
    free(hC); hC = NULL;
    
}//run_iteration


// -- completes one simulation iteration performs the approximations until the entire design has stabalized using 2pt correlations -- //
static void run_coherence_iteration_hf (QCADCell ***sorted_cells, int total_number_of_inputs, unsigned long int number_samples, const coherence_OP *options, int SIMULATION_TYPE, VectorTable *pvt, double **a, double *b, double *c, int num_cells, double Ek_max, double Ek_min, double **k, double *cv, double t)
{
    
    int i, j;
    double *hC = NULL;
    double *temp_cv1 = NULL;
    double *temp_cv2 = NULL;
    double *cv_old = NULL;
    
    int num_entries = 3*num_cells;
    
    hC = (double*)malloc(7*sizeof(double));
    temp_cv1 = (double*)malloc(num_entries*sizeof(double));
    temp_cv2 = (double*)malloc(num_entries*sizeof(double));
    cv_old = (double*)malloc(num_entries*sizeof(double));
    
    for (j = 0; j < num_entries; j++) {
        temp_cv1[j] = 0;
        temp_cv2[j] = 0;
    }
    
    array_copy(k[6],k[0],num_entries); //copy 7th row to 1st row; i.e, k[0] = k[6]
    
    array_copy(c,hC,7); //copy c into hC
    mult_array_by_constant(hC,options->time_step,7); //multiply hC by time_step
    
    for (i = 1; i < 7; i++) {
        
        matrix_array_mult(a[i],k,temp_cv1, num_entries, 7); //multiply ith row of a (1x7) with k (7xnum_entries)
        mult_array_by_constant(temp_cv1, options->time_step, num_entries); //multiply new array with time_step
        add_array(cv,temp_cv1,num_entries, cv_old); //add current cv to temp_cv1... put into cv_old
        
        lambda_next_hf (sorted_cells, t + hC[i], num_cells, options, number_samples, total_number_of_inputs, SIMULATION_TYPE, pvt, Ek_max, Ek_min, k[i], cv_old); 
    }
    
    matrix_array_mult(b,k,temp_cv2, num_entries, 7); //multiply b (1x7) with NEW k (7xnum_entries)
    mult_array_by_constant(temp_cv2, options->time_step, num_entries); //multiply this new array by time_step
    add_array(cv, temp_cv2, num_entries, cv); //add this new array to the cv array that was passed in... put into temp_cv1
    
    free(temp_cv1); temp_cv1 = NULL;
    free(temp_cv2); temp_cv2 = NULL;
    free(cv_old); cv_old = NULL;
    free(hC); hC = NULL;
    
}//run_iteration


//-------------------------------------------------------------------//
// -- refreshes the array of Ek values for each cell in the design this is done to speed up the simulation
// since we can assume no design changes durring the simulation we can precompute all the Ek values then
// use them as necessary throughout the simulation -- //
static void coherence_refresh_all_Ek (int number_of_cell_layers, int *number_of_cells_in_layer, QCADCell ***sorted_cells, coherence_OP *options)
  {
  int icNeighbours = 0 ;
  coherence_model *cell_model = NULL ;
  int i,j,k;

  // calculate the Ek for each cell //
  for(i = 0 ; i < number_of_cell_layers ; i++)
    for(j = 0 ; j < number_of_cells_in_layer[i] ; j++)
      {
      // free up memory from previous simulations //
      g_free ((cell_model = (coherence_model *)sorted_cells[i][j]->cell_model)->neighbours);
      g_free (cell_model->Ek);
      g_free (cell_model->neighbour_layer);
	  g_free (cell_model->neighbour_index);
      cell_model->neighbours = NULL;
      cell_model->neighbour_layer = NULL;
      cell_model->Ek = NULL;
	  cell_model->neighbour_index = NULL;

      // select all neighbours within the provided radius //
      cell_model->number_of_neighbours = icNeighbours =
        select_cells_in_radius_cv(sorted_cells, sorted_cells[i][j], ((coherence_OP *)options)->radius_of_effect, i, number_of_cell_layers, number_of_cells_in_layer,
             ((coherence_OP *)options)->layer_separation, &(cell_model->neighbours), (int **)&(cell_model->neighbour_layer), (int **)&(cell_model->neighbour_index));

		  
      //printf("number of neighbors = %d\n", icNeighbours);

      if (icNeighbours > 0)
        {
        cell_model->Ek = g_malloc0 (sizeof (double) * icNeighbours);

        // ensure no memory allocation error has ocurred //
        if (((coherence_model *)sorted_cells[i][j]->cell_model)->neighbours == NULL ||
            ((coherence_model *)sorted_cells[i][j]->cell_model)->Ek == NULL)
          //printf ("memory allocation error in refresh_all_Ek()\n");
          exit (1);

        for (k = 0; k < icNeighbours; k++)
          //if(cell_model->neighbours[k]==NULL)printf("Null neighbour prior to passing into determine Ek for k = %d\n", k);
          // set the Ek of this cell and its neighbour //
          cell_model->Ek[k] = coherence_determine_Ek (sorted_cells[i][j], cell_model->neighbours[k], ABS(i-cell_model->neighbour_layer[k]), options);
          //printf("Ek = %e\n", cell_model->Ek[k]/1.602e-19);
        }
      }
  }//refresh_all_Ek

//-------------------------------------------------------------------//
// Determines the Kink energy of one cell with respect to another this is defined as the energy of those
// cells having opposite polarization minus the energy of those two cells having the same polarization -- //
static double coherence_determine_Ek (QCADCell * cell1, QCADCell * cell2, int layer_separation, coherence_OP *options)
  {
  int k;
  int j;

  double distance = 0;
  double Constant = 1 / (4 * PI * EPSILON * options->epsilonR);

  double charge1[4] = { -HALF_QCHARGE,  HALF_QCHARGE, -HALF_QCHARGE,  HALF_QCHARGE };
  double charge2[4] = {  HALF_QCHARGE, -HALF_QCHARGE,  HALF_QCHARGE, -HALF_QCHARGE };

  double EnergyDiff = 0;
  double EnergySame = 0;

  g_assert (cell1 != NULL);
  g_assert (cell2 != NULL);
  g_assert (cell1 != cell2);

  for (k = 0; k < cell1->number_of_dots; k++)
    for (j = 0; j < cell2->number_of_dots; j++)
      {
      // determine the distance between the dots //
      // printf("layer seperation = %d\n", layer_seperation);
      distance = determine_distance (cell1, cell2, k, j, (double)layer_separation * ((coherence_OP *)options)->layer_separation);
      g_assert (distance != 0);

      EnergyDiff += Constant * (charge1[k] * charge2[j]) / (distance*1e-9);
      EnergySame += Constant * (charge1[k] * charge1[j]) / (distance*1e-9);
      }//for other dots

  //printf("Ek = %e\n", (EnergyDiff - EnergySame)/ 1.602e-19);

  return EnergyDiff - EnergySame;
  }// coherence_determine_Ek

//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
// Calculates the clock data at a particular sample
static inline double calculate_clock_value (unsigned int clock_num, double sample, unsigned long int number_samples, int total_number_of_inputs, const coherence_OP *options, int SIMULATION_TYPE, VectorTable *pvt)
  {
  double clock = 0;
  int jitter_phases[4] = {options->jitter_phase_0, options->jitter_phase_1,
                          options->jitter_phase_2, options->jitter_phase_3} ;

//Added by Marco: phase shift included in (-PI/2, +P/2) with steps of (1/200)PI
//Edited by Konrad; Above is wrong, changed jitter to be actual phase shift shift = jitter/180*PI

  if (SIMULATION_TYPE == EXHAUSTIVE_VERIFICATION)
    {
    clock = optimization_options.clock_prefactor *
      cos (((double)(1 << total_number_of_inputs)) * sample * optimization_options.four_pi_over_number_samples - (double)((jitter_phases[clock_num]) / 180.0) * PI  - PI * (double)clock_num * 0.5) + optimization_options.clock_shift + options->clock_shift;

    // Saturate the clock at the clock high and low values
    clock = CLAMP (clock, options->clock_low, options->clock_high) ;
    }
  else
  if (SIMULATION_TYPE == VECTOR_TABLE)
    {
    clock = optimization_options.clock_prefactor *
      cos (((double)pvt->vectors->icUsed) * sample * optimization_options.two_pi_over_number_samples - (double)((jitter_phases[clock_num]) / 180.0) * PI  - PI * (double)clock_num * 0.5) + optimization_options.clock_shift + options->clock_shift;

    // Saturate the clock at the clock high and low values
    clock = CLAMP (clock, options->clock_low, options->clock_high) ;
    }
    
//End added by Marco
  return clock;
  }// calculate_clock_value


static inline double calculate_clock_value_cc (QCADCell *cell, double sample, unsigned long int number_samples, int total_number_of_inputs, const coherence_OP *options, int SIMULATION_TYPE, VectorTable *pvt) //Added by Faizal for cont. clocking
  {
  double clock = 0;

  if (SIMULATION_TYPE == EXHAUSTIVE_VERIFICATION)
    {
    clock = optimization_options.clock_prefactor *
      cos (((double)(1 << total_number_of_inputs)) * sample * optimization_options.four_pi_over_number_samples - options->wave_number_kx * QCAD_DESIGN_OBJECT (cell)->x - options->wave_number_ky * QCAD_DESIGN_OBJECT (cell)->y) + optimization_options.clock_shift + options->clock_shift;
	  
    // Saturate the clock at the clock high and low values
    clock = CLAMP (clock, options->clock_low, options->clock_high) ;
    }
  else
  if (SIMULATION_TYPE == VECTOR_TABLE)
    {
    clock = optimization_options.clock_prefactor *
      cos (((double)pvt->vectors->icUsed) * sample * optimization_options.two_pi_over_number_samples - options->wave_number_kx * QCAD_DESIGN_OBJECT (cell)->x - options->wave_number_ky * QCAD_DESIGN_OBJECT (cell)->y) + optimization_options.clock_shift + options->clock_shift;

    // Saturate the clock at the clock high and low values
    clock = CLAMP (clock, options->clock_low, options->clock_high) ;
    }
    
  return clock;
  }// calculate_clock_value_cc


//-------------------------------------------------------------------//

// Next value of lambda x with choice of algorithm
static void lambda_next (QCADCell ***sorted_cells, double t, int num_cells, const coherence_OP *options, unsigned long int number_samples, int total_number_of_inputs, int SIMULATION_TYPE, VectorTable *pvtdouble, double Ek_max, double Ek_min, double *cv, double *cv_old)  
{
	
    /* 
     cv[3*i] = lambda x(i)
     cv[3*i+1] = lambda y(i)
     cv[3*i+2] = lambda z(i)
     cv[(3+i)*num_cells + j] = Kxz(i,j)
     cv[(3+i+num_cells)*num_cells + j] = Kzz(i,j)
    */
    
    int i, j, m, k, fk;
	double clock_value, clock_value_neigh;
	int neighbour, neighbour_m, num_neighbours;
    double lss_z, Pol;
    double sample_number;
	double over_relaxation = 1/options->relaxation;
    int distDriver, num_inv, ss_neighbours;
    
    double PEk, EkKxz, Ekd, Ek_temp1;
    double *EkKzz = NULL;
    
    EkKzz = (double*)malloc(num_cells*sizeof(double));
    
    sample_number = t/options->time_step;
    
    
    for (i = 0; i < num_cells; i++) { //calcualte all coherence vector elements for each cell
        distDriver = ((coherence_model *)sorted_cells[0][i]->cell_model)->distDriver;
        num_inv = ((coherence_model *)sorted_cells[0][i]->cell_model)->num_inv;
        num_neighbours = ((coherence_model *)sorted_cells[0][i]->cell_model)->number_of_neighbours;
        
        if(FOUR_PHASE_CLOCKING == options->clocking){
			clock_value = calculate_clock_value((coherence_model *)sorted_cells[0][i]->cell_options.clock, sample_number, number_samples, total_number_of_inputs, options, SIMULATION_TYPE, pvtdouble); 
		}
		else {  
			clock_value = calculate_clock_value_cc((coherence_model *)sorted_cells[0][i], sample_number, number_samples, total_number_of_inputs, options, SIMULATION_TYPE, pvtdouble);
		}
        
        PEk = 0; //sum of Ek(i,j)*Pj
        EkKxz = 0; //sum of Ek*Kxz(i,j)
        Ekd = 0; //sum of Ek(i,d)*Pd 
        for (k = 0 ; k < num_neighbours ; k++) {
            neighbour = ((coherence_model *)sorted_cells[0][i]->cell_model)->neighbour_index[k]; //cell number of the kth neighboring cell
            
            Ek_temp1 = coherence_determine_Ek(sorted_cells[0][i], sorted_cells[0][neighbour], 0, options);
            PEk += (qcad_cell_calculate_polarization (((coherence_model *)sorted_cells[0][neighbour])))*Ek_temp1; 
            if (((QCAD_CELL_INPUT == sorted_cells[0][neighbour]->cell_function) || (QCAD_CELL_FIXED == sorted_cells[0][neighbour]->cell_function))) {
                Ekd += (qcad_cell_calculate_polarization (((coherence_model *)sorted_cells[0][neighbour])))*Ek_temp1; //if neighbour is a driver/fixed cell, add it to Ekd
                EkKzz[neighbour] = 0; //Ek(i,m)*Kzz(j,m)... for a given cell i and neighbour (j), sum over all other cells (m)
                continue;
            }
            else {
                EkKxz += cv_old[(3+i)*num_cells+neighbour] * Ek_temp1; //if neighbour is NOT a driver/fixed cell, add it to EkKxz
            }
            
            EkKzz[neighbour] = 0;
            for (m = 0; m < num_neighbours; m++) {
                neighbour_m = ((coherence_model *)sorted_cells[0][i]->cell_model)->neighbour_index[m];
                if (((QCAD_CELL_INPUT == sorted_cells[0][neighbour_m]->cell_function) || (QCAD_CELL_FIXED == sorted_cells[0][neighbour_m]->cell_function))) {
                    continue; //if cell m is a driver/fixed cell, then continue
                }
                EkKzz[neighbour] += cv_old[(3+neighbour+num_cells)*num_cells+neighbour_m] * coherence_determine_Ek(sorted_cells[0][i], sorted_cells[0][neighbour_m], 0, options);
            }
        }
        
        Pol = 0;
        ss_neighbours = ((coherence_model *)sorted_cells[0][i]->cell_model)->ss_neighbours;
        for (fk = 0; fk < ss_neighbours; fk++) {
            Pol += qcad_cell_calculate_polarization (sorted_cells[0][((coherence_model *)sorted_cells[0][i]->cell_model)->Driver[fk]]);
        }
        lss_z = -tanh(0.5*Ek_max*Pol*optimization_options.over_kBT)*pow(tanh(0.5*Ek_max*optimization_options.over_kBT), distDriver)*pow(tanh(0.5*Ek_min*optimization_options.over_kBT), num_inv);
        
        
        cv[3*i] = -over_hbar*PEk*cv_old[3*i+1] - over_relaxation*cv_old[3*i]; //lambda x
        cv[3*i+1] = over_hbar*(Ekd*cv_old[3*i] + 2*clock_value*cv_old[3*i+2] - EkKxz) - over_relaxation*cv_old[3*i+1]; //lambda y
        cv[3*i+2] = -over_hbar*2*clock_value*cv_old[3*i+1] - over_relaxation*(cv_old[3*i+2] - lss_z); //lambda z
        
        
        
        for (m = 0; m < num_neighbours; m++) {
            neighbour = ((coherence_model *)sorted_cells[0][i]->cell_model)->neighbour_index[m];
            
            if (((QCAD_CELL_INPUT == sorted_cells[0][neighbour]->cell_function) || (QCAD_CELL_FIXED == sorted_cells[0][neighbour]->cell_function))) {
                cv[(3+i)*num_cells+neighbour] = 0; //kxz
                cv[(3+i+num_cells)*num_cells+neighbour] = 0; //kzz
                continue;
            }	
            
            if(FOUR_PHASE_CLOCKING == options->clocking){
                clock_value_neigh = calculate_clock_value((coherence_model *)sorted_cells[0][neighbour]->cell_options.clock, sample_number, number_samples, total_number_of_inputs, options, SIMULATION_TYPE, pvtdouble); 
            }
            else {  
                clock_value_neigh = calculate_clock_value_cc((coherence_model *)sorted_cells[0][neighbour], sample_number, number_samples, total_number_of_inputs, options, SIMULATION_TYPE, pvtdouble);
            }
            
            cv[(3+i)*num_cells+neighbour] = over_hbar*((-Ekd*cv_old[3*neighbour+2] + ((coherence_model *)sorted_cells[0][i]->cell_model)->Ek_array[neighbour] + EkKzz[neighbour])*cv_old[3*i+1] - 2*clock_value*cv_old[3*neighbour+1]*cv_old[3*i]) - over_relaxation*cv_old[(3+i)*num_cells+neighbour]; //Kxz(i,m)
            
            cv[(3+i+num_cells)*num_cells+neighbour] = -over_hbar*(2*clock_value*cv_old[3*i+1]*cv_old[3*neighbour+2] + 2*clock_value_neigh*cv_old[3*i+2]*cv_old[3*neighbour+1]) - over_relaxation*(cv_old[(3+i+num_cells)*num_cells+neighbour] - tanh(0.5*((coherence_model *)sorted_cells[0][i]->cell_model)->Ek_array[neighbour]*optimization_options.over_kBT)); //Kzz(i,m)
        }	
    }
    free(EkKzz); EkKzz = NULL;
	
}

// Next value of lambda x with choice of algorithm
static void lambda_next_hf (QCADCell ***sorted_cells, double t, int num_cells, const coherence_OP *options, unsigned long int number_samples, int total_number_of_inputs, int SIMULATION_TYPE, VectorTable *pvtdouble, double Ek_max, double Ek_min, double *cv, double *cv_old)  
{
	
    /* 
     cv[3*i] = lambda x(i)
     cv[3*i+1] = lambda y(i)
     cv[3*i+2] = lambda z(i)
     */
    
    int i, j, m, k, fk;
	double clock_value;
	int neighbour, num_neighbours;
    double sample_number;
	double over_relaxation = 1/options->relaxation;
    double lss_x, lss_z, Pol;
    int distDriver, num_inv, ss_neighbours;
    
    double PEk, Ek_temp1;
    
    sample_number = t/options->time_step;
    
    
    for (i = 0; i < num_cells; i++) { //calcualte all coherence vector elements for each cell
        distDriver = ((coherence_model *)sorted_cells[0][i]->cell_model)->distDriver;
        num_inv = ((coherence_model *)sorted_cells[0][i]->cell_model)->num_inv;
        num_neighbours = ((coherence_model *)sorted_cells[0][i]->cell_model)->number_of_neighbours;
        
        if(FOUR_PHASE_CLOCKING == options->clocking){
			clock_value = calculate_clock_value((coherence_model *)sorted_cells[0][i]->cell_options.clock, sample_number, number_samples, total_number_of_inputs, options, SIMULATION_TYPE, pvtdouble); 
		}
		else {  
			clock_value = calculate_clock_value_cc((coherence_model *)sorted_cells[0][i], sample_number, number_samples, total_number_of_inputs, options, SIMULATION_TYPE, pvtdouble);
		}
        
        PEk = 0; //sum of Ek(i,j)*Pj
        
        for (k = 0 ; k < num_neighbours ; k++) {
            neighbour = ((coherence_model *)sorted_cells[0][i]->cell_model)->neighbour_index[k]; //cell number of the kth neighboring cell
            
            Ek_temp1 = coherence_determine_Ek(sorted_cells[0][i], sorted_cells[0][neighbour], 0, options);
            PEk += (qcad_cell_calculate_polarization (((coherence_model *)sorted_cells[0][neighbour])))*Ek_temp1; 
        }
        
        /*
        Pol = 0;
        ss_neighbours = ((coherence_model *)sorted_cells[0][i]->cell_model)->ss_neighbours;
        for (fk = 0; fk < ss_neighbours; fk++) {
            Pol += qcad_cell_calculate_polarization (sorted_cells[0][((coherence_model *)sorted_cells[0][i]->cell_model)->Driver[fk]]);
        }
        lss_z = -tanh(0.5*Ek_max*Pol*optimization_options.over_kBT)*pow(tanh(0.5*Ek_max*optimization_options.over_kBT), distDriver)*pow(tanh(0.5*Ek_min*optimization_options.over_kBT), num_inv);
        lss_x = 0;
        */
         
        lss_x = lambda_ss_x(t, PEk, clock_value, options);
        lss_z = lambda_ss_z(t, PEk, clock_value, options);
        
        
        cv[3*i] = -over_hbar*PEk*cv_old[3*i+1] - over_relaxation*(cv_old[3*i]-lss_x); //lambda x
        cv[3*i+1] = over_hbar*(PEk*cv_old[3*i] + 2*clock_value*cv_old[3*i+2]) - over_relaxation*cv_old[3*i+1]; //lambda y
        cv[3*i+2] = -over_hbar*2*clock_value*cv_old[3*i+1] - over_relaxation*(cv_old[3*i+2] - lss_z); //lambda z
        
        
    }
	
}

static inline double get_max_array(double *array, int num_elements) {
	
	int i;
	double max = -32000;
	
	for (i = 0; i < num_elements; i++) {
        if (array[i] > max) {
			max = array[i];
        }
	}
	
	return max;
}

static inline void matrix_copy(double **Mat1, int dimx, int dimy, double **Mat0)
{
	
	int i = 0;
	int j = 0;
	for(i = 0; i < dimx; i++)
		for( j = 0; j < dimy; j++) {
			Mat0[i][j] =  Mat1[i][j];
		}		
}

static inline double slope_x (double t, double PEk, double Gamma, double lambda_x, double lambda_y, double lambda_z, const coherence_OP *options)
  {
  double mag = magnitude_energy_vector (PEk, Gamma);
  return ((2.0 * Gamma * over_hbar / mag * tanh (optimization_options.hbar_over_kBT * mag * 0.5) - lambda_x) / options->relaxation - (PEk * lambda_y * over_hbar));
  }

static inline double slope_y (double t, double PEk, double Gamma, double lambda_x, double lambda_y, double lambda_z, const coherence_OP *options)
  {return (options->relaxation * (PEk * lambda_x + 2.0 * Gamma * lambda_z) - hbar * lambda_y) / (options->relaxation * hbar);}

static inline double slope_z (double t, double PEk, double Gamma, double lambda_x, double lambda_y, double lambda_z, const coherence_OP *options)
  {
  double mag = magnitude_energy_vector (PEk, Gamma);
  return (-PEk * tanh (optimization_options.hbar_over_kBT * mag * 0.5) - mag * (2.0 * Gamma * options->relaxation * lambda_y + hbar * lambda_z)) / (options->relaxation * hbar * mag);
  }


//-------------------------------------------------------------------//

// Next value of lambda x with choice of algorithm
static inline double lambda_x_next (double t, double PEk, double Gamma, double lambda_x, double lambda_y, double lambda_z, const coherence_OP *options)
{
	double k1 = options->time_step * slope_x (t, PEk, Gamma, lambda_x, lambda_y, lambda_z, options);
	/*
	double k2, k3, k4;
	
	
	if (RUNGE_KUTTA == options->algorithm)
    {
		k2 = options->time_step * slope_x (t, PEk, Gamma, lambda_x + k1/2, lambda_y, lambda_z, options);
		k3 = options->time_step * slope_x (t, PEk, Gamma, lambda_x + k2/2, lambda_y, lambda_z, options);
		k4 = options->time_step * slope_x (t, PEk, Gamma, lambda_x + k3,   lambda_y, lambda_z, options);
		return lambda_x + k1/6 + k2/3 + k3/3 + k4/6;
    }
	else
		if (EULER_METHOD == options->algorithm)
			return lambda_x + k1;
		else
			command_history_message ("coherence vector undefined algorithm\n");
	*/
	
	return lambda_x + k1;
}

// Next value of lambda y with choice of algorithm
static inline double lambda_y_next (double t, double PEk, double Gamma, double lambda_x, double lambda_y, double lambda_z, const coherence_OP *options)
{
	double k1 = options->time_step * slope_y (t, PEk, Gamma, lambda_x, lambda_y, lambda_z, options);
	/*
	double k2, k3, k4;
	
	if (RUNGE_KUTTA == options->algorithm)
    {
		k2 = options->time_step * slope_y (t, PEk, Gamma, lambda_x, lambda_y + k1/2, lambda_z, options);
		k3 = options->time_step * slope_y (t, PEk, Gamma, lambda_x, lambda_y + k2/2, lambda_z, options);
		k4 = options->time_step * slope_y (t, PEk, Gamma, lambda_x, lambda_y + k3,   lambda_z, options);
		return lambda_y + k1/6 + k2/3 + k3/3 + k4/6;
    }
	else
		if (EULER_METHOD == options->algorithm)
			return lambda_y + k1;
		else
			command_history_message("coherence vector undefined algorithm\n");
	*/
	return lambda_y + k1;
}

// Next value of lambda z with choice of algorithm
static inline double lambda_z_next (double t, double PEk, double Gamma, double lambda_x, double lambda_y, double lambda_z, const coherence_OP *options)
{
	double k1 = options->time_step * slope_z (t, PEk, Gamma, lambda_x, lambda_y, lambda_z, options);
	/*
	double k2, k3, k4;
	
	if (RUNGE_KUTTA == options->algorithm)
    {
		k2 = options->time_step * slope_z(t, PEk, Gamma, lambda_x, lambda_y, lambda_z + k1/2, options);
		k3 = options->time_step * slope_z(t, PEk, Gamma, lambda_x, lambda_y, lambda_z + k2/2, options);
		k4 = options->time_step * slope_z(t, PEk, Gamma, lambda_x, lambda_y, lambda_z + k3,   options);
		return lambda_z + k1/6 + k2/3 + k3/3 + k4/6;
    }
	else
		if (EULER_METHOD == options->algorithm)
			return lambda_z + k1;
		else
			command_history_message("coherence vector undefined algorithm\n");
	
	return 0;
	 */
	return lambda_z + k1;
}

//-------------------------------------------------------------------------------------------------------------------------//

// Steady-State Coherence Vector X component
static inline double lambda_ss_x(double t, double PEk, double Gamma, const coherence_OP *options)
  {return 2.0 * Gamma * over_hbar / magnitude_energy_vector(PEk, Gamma) * tanh (temp_ratio (PEk, Gamma, options->T));}

// Steady-State Coherence Vector y component
static inline double lambda_ss_y (double t, double PEk, double Gamma, const coherence_OP *options)
  {return 0.0;}

// Steady-State Coherence Vector Z component
static inline double lambda_ss_z(double t, double PEk, double Gamma, const coherence_OP *options)
  {return - PEk * over_hbar / magnitude_energy_vector (PEk, Gamma) * tanh (temp_ratio (PEk, Gamma, options->T));}

static int compareCoherenceQCells (const void *p1, const void *p2)
  {
  return
    ((coherence_model *)((*((QCADCell **)(p1)))->cell_model))->number_of_neighbours >
    ((coherence_model *)((*((QCADCell **)(p2)))->cell_model))->number_of_neighbours ?  1 :
    ((coherence_model *)((*((QCADCell **)(p1)))->cell_model))->number_of_neighbours <
    ((coherence_model *)((*((QCADCell **)(p2)))->cell_model))->number_of_neighbours ? -1 : 0 ;
  }//compareSortStructs



static inline int find_path (double **Ek_matrix, int driver, int *fork, double Ek_max, double Ek_min, int num_cells, int distance, QCADCell ***sorted_cells, int num_seen)
{
	int i, j, k;
	int num_NN;
	int neigh_ind;
	int fork_ind;
    int new_driver;
    int clock_diff;
    double Ek;
	
    Ek = Ek_max; //set Ek = Ek_max (default)
    
    num_NN = search_matrix_row(Ek_matrix, Ek, driver, num_cells); //find the total number of neighbours
    if (num_NN == 1) { //if there's only one neighbour, check to see if it's been seen arleady
        neigh_ind = find_matrix_row(Ek_matrix, Ek, driver, num_cells, 0);
        if (((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->distDriver > -1) { //if it has been seen, then look for diagonal neighbours
            if (search_matrix_row(Ek_matrix, Ek_min, driver, num_cells) > 0) { //if there are diagonal neighbours, then we'll find our path through them
                Ek = Ek_min;
            }
        }
    }
    
    //printf("driver: %d, num_NN: %d, num_seen: %d, Ek: %e\n",driver,num_NN,num_seen,Ek);
    
    if ((num_NN > 3) && (((coherence_model *)sorted_cells[0][driver]->cell_model)->ss_neighbours == 1)) {
        num_seen -= 1;
        fork_ind = search_array_int(fork, driver, num_cells);
        if (fork_ind == -1) {
            fork[driver] = driver;
        }
    }
    else {
        for (j = 0; j < num_NN; j++) {
            neigh_ind = find_matrix_row(Ek_matrix, Ek, driver, num_cells, j); //find the jth neighbour
            clock_diff = (int)(coherence_model *)sorted_cells[0][neigh_ind]->cell_options.clock - (int)(coherence_model *)sorted_cells[0][driver]->cell_options.clock;
            //printf("neigh_ind: %d\n", neigh_ind);
            
            if (((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->distDriver > -1) { //if a value has already been assigned to distDriver, continue
                continue;
            }
            //else... let's do this.
            
            if (((QCAD_CELL_INPUT == sorted_cells[0][driver]->cell_function) ||
                 (QCAD_CELL_FIXED == sorted_cells[0][driver]->cell_function))) { //if influencing cell is a driver cell, assign it as the neighboring cell's Driver
                ((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->ss_neighbours = 1;
                ((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->Driver = (int*)malloc(sizeof(int));
                ((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->Driver[0] = driver; 
                ((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->distDriver = distance+1;
                ((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->num_inv = 0;
                new_driver = neigh_ind;
                num_seen += 1;
                //printf("Driver: %d\n",driver);
            }
            else {
                //only considering cells in the same or adjacent clock zones
                if (clock_diff <= 1) {
                    ((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->ss_neighbours = 1;
                    ((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->Driver = (int*)malloc(sizeof(int));
                    // if in adjacent clock zone, the driver is the previous cell
                    if (clock_diff == 1) {
                        ((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->Driver[0] = driver;
                        ((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->num_inv = 0;
                        distance = 0;
                        //printf("diff: %d\n",driver);
                    }
                    // otherwise, the driver is the same driver of the previous cell
                    else {
                        if (((coherence_model *)sorted_cells[0][driver]->cell_model)->ss_neighbours == 1) {
                            ((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->Driver[0] = ((coherence_model *)sorted_cells[0][driver]->cell_model)->Driver[0];
                            //printf("same: %d\n",((coherence_model *)sorted_cells[0][driver]->cell_model)->Driver[0]);
                        }
                        else {
                            ((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->Driver[0] = driver; //unless the prev cell has multiple drivers, then just use the previous cell as the driver
                            //printf("fork: %d\n",driver);
                        }
                        if (Ek == Ek_max) {
                            ((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->num_inv = ((coherence_model *)sorted_cells[0][driver]->cell_model)->num_inv;
                        }
                        else {
                            ((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->num_inv = ((coherence_model *)sorted_cells[0][driver]->cell_model)->num_inv + 1;
                        }
                    }
                    //printf("num_inv: %d\n",((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->num_inv);
                    ((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->distDriver = distance+1;
                    new_driver = neigh_ind;
                    num_seen += 1;
                }
            }
            num_seen = find_path(Ek_matrix, new_driver, fork, Ek_max, Ek_min, num_cells, distance+1, sorted_cells, num_seen);
        }
    }
    return num_seen;
}

static inline void get_steadystate (int *number_of_cells_in_layer, QCADCell ***sorted_cells, double **Ek_matrix, int total_number_of_inputs, const coherence_OP *options)
{
	double Ek_max, Ek_min;
	int *fork;
	int *driver_ind;
	
	int i, j, k;
	int num_NN, neigh_ind, fork_ind;
    int num_cells = number_of_cells_in_layer;
    int num_seen = 0;
    int done;
    
	Ek_max = get_max(Ek_matrix, num_cells);
	Ek_min = get_min(Ek_matrix, num_cells);
    if (Ek_min == 0) {
        Ek_min = -Ek_max; //if there are no inverting cells, make Ek_min something that won't appear in Ek_matrix (i.e., not zero)
    }
	
	fork = (int*)malloc(num_cells*sizeof(int));
	driver_ind = (int*)malloc(total_number_of_inputs*sizeof(int));
	
	k = 0;
    
    for(j = 0 ; j < num_cells ; j++) //inititalze polarizations, drivers, etc
    {
        if (((QCAD_CELL_INPUT == sorted_cells[0][j]->cell_function) ||
             (QCAD_CELL_FIXED == sorted_cells[0][j]->cell_function))) {
            driver_ind[k] = j; //mark down location of driver
            ((coherence_model *)sorted_cells[0][j]->cell_model)->distDriver = 0;
            k += 1;
            num_seen += 1;
        }										   
        else {
            ((coherence_model *)sorted_cells[0][j]->cell_model)->distDriver = -1;
        }
        fork[j] = -1; 
    }
	
    done = num_cells - num_seen;
    while (done > 0) {
        
        for (i = 0; i < total_number_of_inputs; i++) {
            num_seen = find_path(Ek_matrix, driver_ind[i], fork, Ek_max, Ek_min, num_cells, 0, sorted_cells, num_seen);
        }
        
        for (i = 0; i < num_cells; i++) { //go to each fork and figure out its drivers
            if (fork[i] == -1) {
                continue;
            }
            else {
                num_NN = 4;
                free(((coherence_model *)sorted_cells[0][fork[i]]->cell_model)->Driver); ((coherence_model *)sorted_cells[0][fork[i]]->cell_model)->Driver = NULL;
                ((coherence_model *)sorted_cells[0][fork[i]]->cell_model)->Driver = (int*)malloc((num_NN-1)*sizeof(int));
                k = 0;
                for (j = 0; j < num_NN; j++) {
                    neigh_ind = find_matrix_row(Ek_matrix, Ek_max, fork[i], num_cells, j);
                    if (((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->distDriver > - 1) {
                        ((coherence_model *)sorted_cells[0][fork[i]]->cell_model)->Driver[k] = ((coherence_model *)sorted_cells[0][neigh_ind]->cell_model)->Driver[0];
                        //((coherence_model *)sorted_cells[0][fork[i]]->cell_model)->Driver[k] = neigh_ind;
                        k += 1;
                    }
                }
                if (k == (num_NN - 1)) { //if all of the cell's neighbours are accounted for...
                    if (((coherence_model *)sorted_cells[0][fork[i]]->cell_model)->ss_neighbours == 1); { //and we haven't already counted the cell..
                        num_seen += 1; //then add this cell to the seen cells
                        ((coherence_model *)sorted_cells[0][fork[i]]->cell_model)->ss_neighbours = num_NN-1; //update it's neighbours
                    }
                }
                else {
                    ((coherence_model *)sorted_cells[0][fork[i]]->cell_model)->ss_neighbours = 1;
                    num_seen += 1;
                }
                fork_ind = fork[i];
                fork[i] = -1; //remove cell from fork array
                num_seen = find_path(Ek_matrix, fork_ind, fork, Ek_max, Ek_min, num_cells, 0, sorted_cells, num_seen);
            }
        }
        done = num_cells - num_seen;
        //printf("num_cells: %d, num_seen: %d\n",num_cells, num_seen); 
    }
	
	free(driver_ind); driver_ind = NULL;
	free(fork); fork = NULL;
}


static inline void compare_array(double *A, double *B, int length, double *Out)
{
	int i;
	
	for (i = 0; i < length; i++) {
		Out[i] = fabs(A[i] - B[i]);
	}
}		

static inline void array_copy(double *Arr1, double *Arr0, int length)
{
	
	int i = 0;
	
	for(i = 0; i < length; i++) {
		Arr0[i] = Arr1[i];
	}
	
}

static inline void add_array(double *Arr1, double *Arr2, int length, double *Arr0)
{
	int i = 0;
	for (i = 0; i < length; i++) {
		Arr0[i] = Arr1[i] + Arr2[i];
	}
}

static inline void mult_array_by_constant(double *Arr1, double constant, int length)
{
	int i = 0;
	for (i = 0; i < length; i++) {
		Arr1[i] *= constant;
	}
}

static inline int search_matrix_row(double **A, double cmp, int row, int num_elements)
{
	
	int i;
	int num_NN = 0;
	
	for (i = 0; i<num_elements; i++) {
		if (fabs(A[row][i] - cmp) < fabs(1e-3*cmp)) {
			num_NN += 1;
		}
	}
	return num_NN;	
}

static inline int find_matrix_row(double **A, double cmp, int row, int num_elements, int select)
{
	
	int i;
	int j = 0;
	
	for (i = 0; i<num_elements; i++) {
		if (fabs(A[row][i] - cmp) < fabs(1e-3*cmp)) {
			if (j == select) {
				return i;
			}
			else {
				j += 1;
			}
		}
	}
	return -1;
}

static inline int search_array(double *A, double cmp, int num_elements )
{
	
	int i;
	
	for (i = 0; i<num_elements; i++) {
		if (A[i] == cmp) {
			return i;
		}
	}
	return -1;	
}				

static inline int search_array_thresh(double *A, double cmp, int num_elements )
{
	
	int i;
	
	for (i = 0; i<num_elements; i++) {
		if (A[i] > cmp) {
			return i;
		}
	}
	return -1;	
}			

static inline int search_array_int(int *A, double cmp, int num_elements )
{
	
	int i;
	
	for (i = 0; i<num_elements; i++) {
		if (A[i] == cmp) {
			return i;
		}
	}
	return -1;	
}	

static inline int find_array(double *A, double cmp, int num_elements)
{
	
	int i;
	int j = 0;
	
	for (i = 0; i<num_elements; i++) {
		if (A[i] == cmp) {
			j += 1;
		}
	}
	return j;	
}	

static inline int find_in_array(double *A, double cmp, int num_elements, int select)
{
	
	int i;
	int j = 0;
	
	for (i = 0; i<num_elements; i++) {
		if (A[i] == cmp) {
			if (j == select) {
				return j;
			}
			else {
				j += 1;
			}
		}
	}
	return -1;
}						

static inline double get_max(double **A, int num_elements) {
	
	int i,j;
	double max = -32000;
	
	for (i = 0; i < num_elements; i++) {
		for (j = 0; j < num_elements; j++) {
			if (A[i][j] > max) {
				max = A[i][j];
			}
		}
	}
	return max;
}

static inline double get_min(double **A, int num_elements) {
	
	int i,j;
	double min = 32000;
	
	for (i = 0; i < num_elements; i++) {
		for (j = 0; j < num_elements; j++) {
			if (A[i][j] < min) {
				min = A[i][j];
			}
		}
	}
	return min;
}

static inline void matrix_mult(double **A, double **B, double **C, int numrA, int numcB, int numcA) {
    int i, j, k;
    
    for (i = 0; i < numrA; i++) {
        for (j = 0; j < numcB; j++) {
            for (k = 0; k < numcA; k++) {
                C[i][j] += A[i][k]*B[k][j];
            }
        }
    }
}

static inline void matrix_array_mult(double *A, double **B, double *C, int numcB, int numcA) {
    int i, j, k;
    
    for (i = 0; i < 1; i++) {
        for (j = 0; j < numcB; j++) {
            for (k = 0; k < numcA; k++) {
                C[j] += A[k]*B[k][j];
            }
        }
    }
}


void coherence_options_dump (coherence_OP *coherence_options, FILE *pfile)
  {
  fprintf (stderr, "coherence_options_dump:\n") ;
	fprintf (stderr, "coherence_options->T                         = %e [K]\n",  coherence_options->T) ;
	fprintf (stderr, "coherence_options->relaxation                = %e [s]\n",  coherence_options->relaxation) ;
	fprintf (stderr, "coherence_options->time_step                 = %e [s]\n",  coherence_options->time_step) ;
	fprintf (stderr, "coherence_options->duration                  = %e [s]\n",  coherence_options->duration) ;
	fprintf (stderr, "coherence_options->clock_high                = %e [J]\n",  coherence_options->clock_high) ;
	fprintf (stderr, "coherence_options->clock_low                 = %e [J]\n",  coherence_options->clock_low) ;
	fprintf (stderr, "coherence_options->clock_shift               = %e [J]\n",  coherence_options->clock_shift) ;
	fprintf (stderr, "coherence_options->clock_amplitude_factor    = %e\n",      coherence_options->clock_amplitude_factor) ;
	fprintf (stderr, "coherence_options->radius_of_effect          = %e [nm]\n", coherence_options->radius_of_effect) ;
	fprintf (stderr, "coherence_options->epsilonR                  = %e\n",      coherence_options->epsilonR) ;
	fprintf (stderr, "coherence_options->layer_separation          = %e [nm]\n", coherence_options->layer_separation) ;
	//fprintf (stderr, "coherence_options->algorithm                 = %d\n",      coherence_options->algorithm) ;
	fprintf (stderr, "coherence_options->include_correlations      = %s\n",      coherence_options->include_correlations ? "TRUE" : "FALSE") ;
	fprintf (stderr, "coherence_options->animate_simulation        = %s\n",      coherence_options->animate_simulation ? "TRUE" : "FALSE") ;
// Added by Marco
	fprintf (stderr, "coherence_options->jitter_phase_0            = %f degrees\n",      coherence_options->jitter_phase_0) ;
	fprintf (stderr, "coherence_options->jitter_phase_1            = %f degrees\n",      coherence_options->jitter_phase_1) ;
	fprintf (stderr, "coherence_options->jitter_phase_2            = %f degrees\n",      coherence_options->jitter_phase_2) ;
	fprintf (stderr, "coherence_options->jitter_phase_3            = %f degrees\n",      coherence_options->jitter_phase_3) ;
// End added by Marco
//Added by Faizal
	fprintf (stderr, "coherence_options->wave_number_kx            = %lf [1/nm]\n",      coherence_options->wave_number_kx) ;
	fprintf (stderr, "coherence_options->wave_number_ky            = %lf [1/nm]\n",      coherence_options->wave_number_ky) ;
	fprintf (stderr, "coherence_options->clocking                  = %d\n",             coherence_options->clocking) ;
//End added by Faizal	
  }
