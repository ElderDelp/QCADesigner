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
// This file was contributed by Gabriel Schulhof        //
// (schulhof@atips.ca).                                 //
//////////////////////////////////////////////////////////
// Contents:                                            //
//                                                      //
// A batch mode simulator aimed specifically at         //
// verifying outputs against their corresponding        //
// inputs.                                              //
//                                                      //
//////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "intl.h"
#include "fileio.h"
#include "fileio_helpers.h"
#include "design.h"
#include "graph_dialog.h"
#include "global_consts.h"
#include "simulation.h"
#include "simulation_data.h"
#include "coherence_vector.h"
#include "graph_dialog_widget_data.h"
#include "bistable_simulation.h"
#include "semi_coherent.h"
#include "ts_field_clock.h"
#include "preamble.h"

extern bistable_OP bistable_options ;
extern coherence_OP coherence_options ;
#ifdef HAVE_FORTRAN
extern semi_coherent_OP semi_coherent_options ;
extern ts_fc_OP ts_fc_options ;
#endif /* HAVE_FORTRAN */

typedef struct
  {
  gboolean bExitOnFailure ;
  gboolean bDisplaceInputs ;
  gboolean bDisplaceOutputs ;
  gboolean bNormalDisp ;
  int number_of_sims ;
  int sim_engine ;
  double dTolerance ;
  double dVariance ;
  double dThreshLower ;
  double dThreshUpper ;
  int icAverageSamples ;
  char *pszFailureFNamePrefix ;
  char *pszSimOptsFName ;
  char *pszReferenceSimOutputFName ;
  char *pszFName ;
  char *pszVTFName ;
  int circuit_delay ;
  int ignore_from_end ;
  int n_to_displace ;
  } CMDLINE_ARGS ;

static void randomize_design_cells (GRand *rnd, DESIGN *design, double dMinRadius, double dMaxRadius, gboolean bDisplaceInputs, gboolean bDisplaceOutputs, int n_to_displace, double dVariance, gboolean Dist) ;
static EXP_ARRAY *create_honeycombs_from_buses (simulation_data *sim_data, BUS_LAYOUT *bus_layout, int bus_function, double dThreshLower, double dThreshUpper, int icAverageSamples) ;
static int determine_success (HONEYCOMB_DATA *hcdRef, HONEYCOMB_DATA *hcdOut, int delay, int ignore_from_end) ;
static void parse_cmdline (int argc, char **argv, CMDLINE_ARGS *cmdline_args) ;

int main (int argc, char **argv)
  {
  VectorTable *pvt = NULL ;
  VTL_RESULT vt_load_result = VTL_OK ;
  int Nix, Nix1, Nix2 ;
  DESIGN *design = NULL, *working_design = NULL ;
  simulation_data *sim_data = NULL ;
  GRand *rnd = NULL ;
  EXP_ARRAY *ref_hcs = NULL, *out_hcs = NULL ;
  HONEYCOMB_DATA *hcdRef = NULL, *hcdOut = NULL ;
  EXP_ARRAY *icSuccesses = NULL ;
  int icOutputBuses = 0 ;
  SIMULATION_OUTPUT *sim_output = NULL ;
  int single_success = 0 ;
  gboolean bDie = FALSE ;
  CMDLINE_ARGS cmdline_args = 
    {
    .bExitOnFailure             = FALSE,
    .bDisplaceInputs            = FALSE,
    .bDisplaceOutputs           = FALSE,
    .number_of_sims             =  1,
    .sim_engine                 = COHERENCE_VECTOR,
    .dTolerance                 =  0.0,
    .dThreshLower               = -0.5,
    .dThreshUpper               =  0.5,
    .icAverageSamples           =  1,
    .pszFailureFNamePrefix      = NULL,
    .pszSimOptsFName            = NULL,
    .pszReferenceSimOutputFName = NULL,
    .pszFName                   = NULL,
    .pszVTFName                 = NULL,
    .circuit_delay              =  0,
    .ignore_from_end            =  0,
    .n_to_displace              = -1,
    .bNormalDisp		= FALSE,
    .dVariance			=  1.0,
    };

  preamble (&argc, &argv) ;

  parse_cmdline (argc, argv, &cmdline_args) ;

  flush_fprintf (stderr,
  _("Simulation engine               : %s\n"
    "Simulation options file         : %s\n"
    "Circuit file                    : %s\n"
    "Reference simulation output file: %s\n"
    "number of simulations           : %d\n"
    "uniform distribution ?	     : %s\n"
    "tolerance                       : %lf\n"
    "variance			     : %lf\n"
    "lower polarization threshold    : %lf\n"
    "upper polarization threshold    : %lf\n"
    "samples for running average     : %d\n"
    "exit on failure ?               : %s\n"
    "failure output file name prefix : %s\n"
    "initial honeycombs to ignore    : %d\n"
    "trailing honeycombs to ignore   : %d\n"
    "displace input cells also?      : %d\n"
    "displace output cells also?     : %d\n"
    "number of cells to displace     : %d\n"
    ),
    SIM_ENGINE_TO_STRING(cmdline_args.sim_engine),
    cmdline_args.pszSimOptsFName,         cmdline_args.pszFName,         cmdline_args.pszReferenceSimOutputFName, 
    cmdline_args.number_of_sims,          cmdline_args.bNormalDisp ? "TRUE" : "FALSE",  cmdline_args.dTolerance,  cmdline_args.dVariance,     
    cmdline_args.dThreshLower,            cmdline_args.dThreshUpper,     cmdline_args.icAverageSamples, cmdline_args.bExitOnFailure ? "TRUE" : "FALSE",
    cmdline_args.pszFailureFNamePrefix,   cmdline_args.circuit_delay, cmdline_args.ignore_from_end, cmdline_args.bDisplaceInputs ? "TRUE" : "FALSE", 
    cmdline_args.bDisplaceOutputs ? "TRUE" : "FALSE", cmdline_args.n_to_displace) ;

#ifdef GTK_GUI
  gtk_init (&argc, &argv) ;
#else
  g_type_init () ;
#endif /* def GTK_GUI */

  if (!open_project_file (cmdline_args.pszFName, &design))
    {
    flush_fprintf (stderr, _("Failed to open file \"%s\"!"), cmdline_args.pszFName) ;
    return 1 ;
    }
  else
  if (NULL == design)
    {
    flush_fprintf (stderr, _("Failed to open file \"%s\"!"), cmdline_args.pszFName) ;
    return 1 ;
    }

  if (NULL == (sim_output = open_simulation_output_file (cmdline_args.pszReferenceSimOutputFName)))
    {
    flush_fprintf (stderr, _("Failed to open the reference simulation output file !\n")) ;
    return 3 ;
    }

  if (NULL != cmdline_args.pszVTFName)
    {
    pvt = VectorTable_new () ;
    if (NULL != pvt->pszFName)
      g_free (pvt->pszFName) ;
    pvt->pszFName = g_strdup (cmdline_args.pszVTFName) ;
    VectorTable_add_inputs (pvt, design) ;
    vt_load_result = VectorTable_load (pvt) ;
    if (VTL_FILE_FAILED == vt_load_result || VTL_MAGIC_FAILED == vt_load_result)
      {
      VectorTable_free (pvt) ;
      pvt = NULL ;
      }
    }

  if (NULL != pvt)
    {
    flush_fprintf (stderr, _("Using the following vector table:\n")) ;
    VectorTable_dump (pvt, stderr, 0) ;
    }

  if (BISTABLE == cmdline_args.sim_engine)
    {
    bistable_OP *bo = NULL ;

    if (NULL == (bo = open_bistable_options_file (cmdline_args.pszSimOptsFName)))
      {
      flush_fprintf (stderr, _("Failed to open simulation options file !\n")) ;
      return 4 ;
      }
    bistable_options_dump (bo, stderr) ;
    memcpy (&bistable_options, bo, sizeof (bistable_OP)) ;
    }

  else
  if (COHERENCE_VECTOR == cmdline_args.sim_engine)
    {
    coherence_OP *co = NULL ;

    if (NULL == (co = open_coherence_options_file (cmdline_args.pszSimOptsFName)))
      {
      flush_fprintf (stderr, _("Failed to open simulation options file !\n")) ;
      return 5 ;
      }
    coherence_options_dump (co, stderr) ;
    memcpy (&coherence_options, co, sizeof (coherence_OP)) ;
    }

#ifdef HAVE_FORTRAN
  else
  if (SEMI_COHERENT == cmdline_args.sim_engine)
    {
    semi_coherent_OP *sco = NULL ;

    if (NULL == (sco = open_semi_coherent_options_file (cmdline_args.pszSimOptsFName)))
      {
      flush_fprintf (stderr, _("Failed to open simulation options file !\n")) ;
      return 5 ;
      }
    semi_coherent_options_dump (sco, stderr) ;
    memcpy (&semi_coherent_options, sco, sizeof (semi_coherent_OP)) ;
    }
	  
  else
  if (TS_FIELD_CLOCK == cmdline_args.sim_engine)
    {
    ts_fc_OP *sco = NULL ;

    if (NULL == (sco = open_ts_fc_options_file (cmdline_args.pszSimOptsFName)))
      {
      flush_fprintf (stderr, _("Failed to open simulation options file !\n")) ;
      return 5 ;
      }
    ts_fc_options_dump (sco, stderr) ;
    memcpy (&ts_fc_options, sco, sizeof (ts_fc_OP)) ;
    }  
#endif /* HAVE_FORTRAN */

  printf (_("Running %d %s with a radial tolerance of %lf\n"), cmdline_args.number_of_sims, 
    1 == cmdline_args.number_of_sims ? _("simulation") : _("simulations"), cmdline_args.dTolerance) ;

  ref_hcs = create_honeycombs_from_buses (sim_output->sim_data, sim_output->bus_layout, QCAD_CELL_OUTPUT, cmdline_args.dThreshLower, cmdline_args.dThreshUpper, cmdline_args.icAverageSamples) ;

  rnd = g_rand_new () ;

  icSuccesses = exp_array_new (sizeof (int), 1) ;
  for (Nix = 0 ; Nix < design->bus_layout->buses->icUsed ; Nix++)
    if (QCAD_CELL_OUTPUT == exp_array_index_1d (design->bus_layout->buses, BUS, Nix).bus_function)
      icOutputBuses++ ;
  exp_array_1d_insert_vals (icSuccesses, NULL, icOutputBuses, 0) ;
  for (Nix = 0 ; Nix < icSuccesses->icUsed ; Nix++)
    exp_array_index_1d (icSuccesses, int, Nix) = 0 ;

  for (Nix = 0 ; Nix < cmdline_args.number_of_sims && !bDie ; Nix++)
    {
    flush_fprintf (stderr, _("Running %s simulation %d of %d\n"), NULL == pvt ? _("exhaustive") : _("vector table"), Nix + 1, cmdline_args.number_of_sims) ;
    if (NULL != (working_design = design_copy (design)))
      {
      VectorTable_fill (pvt, working_design) ;
      // Gotta re-load VT from the file, because VectorTable_fill destroys the vectors
      vt_load_result = VectorTable_load (pvt) ;
      if (VTL_FILE_FAILED == vt_load_result || VTL_MAGIC_FAILED == vt_load_result)
        {
        VectorTable_free (pvt) ;
        pvt = NULL ;
        }
		  if (cmdline_args.n_to_displace != 0) {  
			  if (cmdline_args.bNormalDisp)
			  {
				  randomize_design_cells (rnd, working_design, 0.0, cmdline_args.dTolerance, cmdline_args.bDisplaceInputs, cmdline_args.bDisplaceOutputs, cmdline_args.n_to_displace, cmdline_args.dVariance, TRUE) ;
			  }	
			  else
			  {
				  randomize_design_cells (rnd, working_design, 0.0, cmdline_args.dTolerance, cmdline_args.bDisplaceInputs, cmdline_args.bDisplaceOutputs, cmdline_args.n_to_displace, cmdline_args.dVariance, FALSE) ;
			  }
		  }
        if (NULL != (sim_data = run_simulation (cmdline_args.sim_engine, (NULL == pvt ? EXHAUSTIVE_VERIFICATION : VECTOR_TABLE), working_design, pvt)))
        {
        out_hcs = create_honeycombs_from_buses (sim_data, working_design->bus_layout, QCAD_CELL_OUTPUT, cmdline_args.dThreshLower, cmdline_args.dThreshUpper, cmdline_args.icAverageSamples) ;
        // Print out the results for comparison
        for (Nix1 = 0 ; Nix1 < out_hcs->icUsed && !bDie ; Nix1++)
          {
          if (Nix1 < ref_hcs->icUsed)
            {
            hcdRef = exp_array_index_1d (ref_hcs, HONEYCOMB_DATA *, Nix1) ;
            hcdOut = exp_array_index_1d (out_hcs, HONEYCOMB_DATA *, Nix1) ;

            flush_fprintf (stderr, _("Reference output bus is: (0x%08x)\"%s\"\n"), (int)hcdRef, hcdRef->bus->pszName) ;
            for (Nix2 = 0 ; Nix2 < hcdRef->arHCs->icUsed ; Nix2++)
              flush_fprintf (stderr, "%llu ", exp_array_index_1d (hcdRef->arHCs, HONEYCOMB, Nix2).value) ;
            flush_fprintf (stderr, "\n") ;

            flush_fprintf (stderr, _("Output bus is: (0x%08x)\"%s\"\n"), (int)hcdOut, hcdOut->bus->pszName) ;
            for (Nix2 = 0 ; Nix2 < hcdOut->arHCs->icUsed ; Nix2++)
              flush_fprintf (stderr, "%llu ", exp_array_index_1d (hcdOut->arHCs, HONEYCOMB, Nix2).value) ;
            flush_fprintf (stderr, "\n") ;
            }
          }

        // Compare the output honeycombs to the reference honeycombs
        for (Nix1 = 0 ; Nix1 < out_hcs->icUsed && !bDie ; Nix1++)
          {
          if (Nix1 < ref_hcs->icUsed)
            {
            hcdRef = exp_array_index_1d (ref_hcs, HONEYCOMB_DATA *, Nix1) ;
            hcdOut = exp_array_index_1d (out_hcs, HONEYCOMB_DATA *, Nix1) ;

            single_success =
              determine_success (hcdRef, hcdOut, cmdline_args.circuit_delay, cmdline_args.ignore_from_end) ;

            flush_fprintf (stderr, _("This run[%d] bus[%d] was %s\n"), Nix, Nix1, 0 == single_success ? _("unsuccessful") : _("successful")) ;
			flush_fprintf (stderr, _("hcdOut->arHCs->icUsed equals %d\n"),hcdOut->arHCs->icUsed);
			}
          else
            {
            flush_fprintf (stderr, _("Output has more buses than reference !\n")) ;
            single_success = 0 ;
            }

          if (0 == single_success && cmdline_args.bExitOnFailure)
            {
            if (NULL != cmdline_args.pszFailureFNamePrefix)
              {
              char *psz = NULL ;
              SIMULATION_OUTPUT failed_sim_output = {sim_data, design->bus_layout, FALSE} ;

              psz = g_strdup_printf ("%s.sim_output", cmdline_args.pszFailureFNamePrefix) ;
              fprintf (stderr, _("Creating %s to record the failure.\n"), psz) ;
              create_simulation_output_file (psz, &failed_sim_output) ;
              g_free (psz) ;

              psz = g_strdup_printf ("%s.qca", cmdline_args.pszFailureFNamePrefix) ;
              fprintf (stderr, _("Creating %s to record the failure.\n"), psz) ;
              create_file (psz, working_design) ;
              g_free (psz) ;
              }
            bDie = TRUE ;
            }
          else
            exp_array_index_1d (icSuccesses, int, Nix1) += single_success ;
          }

        flush_fprintf (stderr, "*******************\n") ;
        for (Nix1 = 0 ; Nix1 < out_hcs->icUsed ; Nix1++)
          honeycomb_data_free (exp_array_index_1d (out_hcs, HONEYCOMB_DATA *, Nix1)) ;
        out_hcs = exp_array_free (out_hcs) ;
        sim_data = simulation_data_destroy (sim_data) ;
        }
      working_design = design_destroy (working_design) ;
      }
    }

  for (Nix1 = 0 ; Nix1 < ref_hcs->icUsed ; Nix1++)
    honeycomb_data_free (exp_array_index_1d (ref_hcs, HONEYCOMB_DATA *, Nix1)) ;
  ref_hcs = exp_array_free (ref_hcs) ;

  for (Nix = 0 ; Nix < icSuccesses->icUsed ; Nix++)
    printf ("success_rate[%d] = %.2lf%%\n", Nix, (double)(exp_array_index_1d (icSuccesses, int, Nix)) / ((double)(cmdline_args.number_of_sims)) * 100.0) ;

  g_rand_free (rnd) ;

  return 0 ;
  }

static EXP_ARRAY *create_honeycombs_from_buses (simulation_data *sim_data, BUS_LAYOUT *bus_layout, int bus_function, double dThreshLower, double dThreshUpper, int icAverageSamples)
  {
  GdkColor clr = {0, 0, 0, 0} ;
  int Nix1 ;
  HONEYCOMB_DATA *hc = NULL ;
  BUS *bus = NULL ;
  EXP_ARRAY *output_hcs = exp_array_new (sizeof (HONEYCOMB_DATA *), 1) ;
  // For each bus, create the appropriate HONEYCOMB_DATA, fill it in with
  // the TRACEDATA structures, and place each HONEYCOMB_DATA into its
  // appropriate EXP_ARRAY.
  for (Nix1 = 0 ; Nix1 < bus_layout->buses->icUsed ; Nix1++)
    if (bus_function == (bus = &exp_array_index_1d (bus_layout->buses, BUS, Nix1))->bus_function)
      {
      hc = honeycomb_data_new_with_array (&clr, sim_data, bus, (QCAD_CELL_INPUT == bus->bus_function ? 0 : bus_layout->inputs->icUsed), dThreshLower, dThreshUpper, icAverageSamples, 2) ;
      exp_array_1d_insert_vals (output_hcs, &hc, 1, -1) ;
      }
  return output_hcs ;
  }

static void randomize_design_cells (GRand *rnd, DESIGN *design, double dMinRadius, double dMaxRadius, gboolean bDisplaceInputs, gboolean bDisplaceOutputs, int n_to_displace, double dVariance, gboolean Dist)
  {
  int Nix, idx, idx_start ;
  float u1x, u2x, u1y, u2y;
  double dRadius = -1.0, dAngle = 0.0 ;
  float dx = 0.0, dy = 0.0, zx = 0.0, zy = 0.0 ;
  GList *llItr = NULL, *llItrObj = NULL ;
  QCADLayer *layer = NULL ;
  gint *ar = NULL, n_cells = 0;
  
  double var = dVariance;  
  
  srand ( (unsigned)time(0) );
 
  if (NULL == rnd || NULL == design) return ;

  if (n_to_displace > 0)
    {
    for (llItr = design->lstLayers ; llItr != NULL ; llItr = llItr->next)
      if (LAYER_TYPE_CELLS == (layer = QCAD_LAYER (llItr->data))->type)
        for (llItrObj = layer->lstObjs ; llItrObj != NULL ; llItrObj = llItrObj->next)
          if (NULL != llItrObj->data)
            n_cells++;

    ar = g_malloc0(n_cells * sizeof(int));
    for (Nix = 0 ; Nix < n_to_displace ; Nix++)
      {
      idx = g_rand_int_range(rnd, 0, n_cells);
      if(!ar[idx])
        ar[idx] = 1;
      else
        {
        idx_start = idx;
        if (g_rand_int_range(rnd, 0, 1))
          {
          for (; idx > -1; idx--)
            if (!ar[idx])
              {
              ar[idx] = 1;
              break;
              }
          

        if (-1 == idx)
          {
          idx = idx_start;
          for (; idx < n_cells ; idx++)
            if (!ar[idx])
              {
              ar[idx] = 1;
              break;
              }
          if (idx == n_cells)
            idx = -1;
          }
        }
      
	else
        {
	 idx = idx_start;
	 for (; idx < n_cells ; idx++)
	   if (!ar[idx])
	    {
	    ar[idx] = 1;
	    break;
            }
	   if (idx == n_cells)
	   idx = -1;
	 
	if (-1 == idx)
	{
	idx = idx_start;
	for (; idx > -1; idx--)
	  if (!ar[idx])
	   {
	   ar[idx] = 1;
	   break;
           }
        }
      }
      

      if (-1 == idx)
        {
        g_warning("randomize_design_cells: Couldn't find an index to set\n");
        break;
        }
      }
    }
  }
  idx = 0;
  for (llItr = design->lstLayers ; llItr != NULL ; llItr = llItr->next)
    if (LAYER_TYPE_CELLS == (layer = QCAD_LAYER (llItr->data))->type)
      for (llItrObj = layer->lstObjs ; llItrObj != NULL ; llItrObj = llItrObj->next)
        if (NULL != llItrObj->data)
          {
          if (ar)
            if (idx < n_cells)
              if (!ar[idx]) {
 		idx++;
                continue;
	      }
	  if(QCAD_CELL_INPUT == QCAD_CELL(llItrObj->data)->cell_function && !bDisplaceInputs)continue;
          if(QCAD_CELL_OUTPUT == QCAD_CELL(llItrObj->data)->cell_function && !bDisplaceOutputs)continue;
          
	  dRadius = g_rand_double_range (rnd, dMinRadius, dMaxRadius) ;
          dAngle = g_rand_double_range (rnd, 0, 2.0 * PI) ;


	  if (Dist)
	    {	
	     dx = dRadius * cos (dAngle) ;
             dy = dRadius * sin (dAngle) ;
	    }
          else
            {
  	     u1x = (double)rand()/(double)RAND_MAX;
  	     u2x = (double)rand()/(double)RAND_MAX;
  	     u1y = (double)rand()/(double)RAND_MAX;
             u2y = (double)rand()/(double)RAND_MAX;

             zx = sqrt(-2.0*log(u1x))*cos(2*PI*u2x);
             zy = sqrt(-2.0*log(u1y))*cos(2*PI*u2y);

             dx = var*zx;
	     dy = var*zy;
            }

          qcad_design_object_move (QCAD_DESIGN_OBJECT (llItrObj->data), dx, dy) ;
          idx++;
          }
  }

static void parse_cmdline (int argc, char **argv, CMDLINE_ARGS *cmdline_args)
  {
  gboolean bDie = FALSE ;
  int icParms = 0 ;
  int Nix ;

  // defaults
  cmdline_args->sim_engine = COHERENCE_VECTOR ;

  for (Nix = 0 ; Nix < argc ; Nix++)
    {
    if (!(strcmp (argv[Nix], _("-D")) && strcmp (argv[Nix], _("--n-to-displace"))))
      {
      if (++Nix < argc)
        cmdline_args->n_to_displace = atoi (argv[Nix]) ;
      }
    else
    if (!(strcmp (argv[Nix], _("-a")) && strcmp (argv[Nix], _("--average"))))
      {
      if (++Nix < argc)
        cmdline_args->icAverageSamples = atoi (argv[Nix]) ;
      }
    else
    if (!(strcmp (argv[Nix], _("-d")) && strcmp (argv[Nix], _("--delay"))))
      {
      if (++Nix < argc)
        cmdline_args->circuit_delay = atoi (argv[Nix]) ;
      }
    else
    if (!(strcmp (argv[Nix], _("-e")) && strcmp (argv[Nix], _("--engine"))))
      {
      if (++Nix < argc)
        cmdline_args->sim_engine = SIM_ENGINE_FROM_STRING(argv[Nix]);
      }
    else
    if (!(strcmp (argv[Nix], _("-f")) && strcmp (argv[Nix], _("--file"))))
      {
      if (++Nix < argc)
        {
        cmdline_args->pszFName = argv[Nix] ;
        icParms++ ;
        }
      }
    else
    if (!(strcmp (argv[Nix], _("-i")) && strcmp (argv[Nix], _("--ignore-trailing"))))
      {
      if (++Nix < argc)
        {
        cmdline_args->ignore_from_end = atoi (argv[Nix]);
        icParms++ ;
        }
      }
    else
    if (!(strcmp (argv[Nix], _("-l")) && strcmp (argv[Nix], _("--lower"))))
      {
      if (++Nix < argc)
        cmdline_args->dThreshLower = g_ascii_strtod (argv[Nix], NULL) ;
      }
    else
    if (!(strcmp (argv[Nix], _("-n")) && strcmp (argv[Nix], _("--number"))))
      {
      if (++Nix < argc)
        {
        cmdline_args->number_of_sims = atoi (argv[Nix]) ;
        icParms++ ;
        }
      }
    else
    if (!(strcmp (argv[Nix], _("-o")) && strcmp (argv[Nix], _("--options"))))
      {
      if (++Nix < argc)
        {
        cmdline_args->pszSimOptsFName = argv[Nix] ;
        icParms++ ;
        }
      }
    else
    if (!(strcmp (argv[Nix], _("-r")) && strcmp (argv[Nix], _("--results"))))
      {
      if (++Nix < argc)
        {
        cmdline_args->pszReferenceSimOutputFName = argv[Nix] ;
        icParms++ ;
        }
      }
    else
    if (!(strcmp (argv[Nix], _("-s")) && strcmp (argv[Nix], _("--save"))))
      {
      if (++Nix < argc)
        cmdline_args->pszFailureFNamePrefix = argv[Nix] ;
      }
    else
    if (!(strcmp (argv[Nix], _("-t")) && strcmp (argv[Nix], _("--tolerance"))))
      {
      if (++Nix < argc)
        {
        cmdline_args->dTolerance = g_ascii_strtod (argv[Nix], NULL) ;
        }
      }
    else
    if (!(strcmp (argv[Nix], _("-V")) && strcmp(argv[Nix], _("--variance"))))	
      {
      if (++Nix < argc)
	{
	cmdline_args->dVariance = g_ascii_strtod (argv[Nix], NULL) ;
        }
    }
    else
    if (!(strcmp (argv[Nix], _("-u")) && strcmp (argv[Nix], _("--upper"))))
      {
      if (++Nix < argc)
        cmdline_args->dThreshUpper = g_ascii_strtod (argv[Nix], NULL) ;
      }
    else
    if (!(strcmp (argv[Nix], _("-v")) && strcmp (argv[Nix], _("--vector-table"))))
      {
      if (++Nix < argc)
        {
        cmdline_args->pszVTFName = argv[Nix] ;
        icParms++ ;
        }
      }
    else
    if (!(strcmp (argv[Nix], _("-x")) && strcmp (argv[Nix], _("--exit"))))
      cmdline_args->bExitOnFailure = TRUE ;
    else
    if (!(strcmp (argv[Nix], _("-I")) && strcmp (argv[Nix], _("--displace-in"))))
      cmdline_args->bDisplaceInputs = TRUE ;
    else
    if (!(strcmp (argv[Nix], _("-O")) && strcmp (argv[Nix], _("--displace-out"))))
      cmdline_args->bDisplaceOutputs = TRUE ;
    else
    if (!(strcmp (argv[Nix], _("-U")) && strcmp (argv[Nix], _("--distribution"))))
      cmdline_args->bNormalDisp = TRUE ;
    }

  if (icParms < 4 || bDie)
    {
    printf (
    _("Usage: batch_sim options...\n"
      "\n"
      "Options are:\n"
      "  -D  --n-to-displace   number        Optional: Number of cells to displace. Default is -1, meaning \"displace all cells\".\n"
      "  -U  --distribution    		     Optional: Displace Cells uniformly. Default is Normal displacement.\n"
      "  -V  --variance        number	     Optional: Variance for normal distribution. Default is 1.\n"
      "  -a  --average         samples       Optional: Number of samples to use for running average. Default is 1.\n"
      "  -d  --delay           honeycombs    Optional: Number of initial honeycombs to ignore because of circuit delay. Default is 0.\n"
      "  -e  --engine          engine        Optional: The simulation engine. One of COHERENCE_VECTOR (DEFAULT), BISTABLE, SEMI_COHERENT or TS_FIELD_CLOCK.\n"
      "  -f  --file            file          Required: The circuit file.\n"
      "  -i  --ignore-trailing honeycombs    Optional: Ignore this many honeycombs at the end.\n"
      "  -l  --lower           polarization  Optional: Lower polarization threshold. Between -1.00 and 1.00. Default is -0.5.\n"
      "  -n  --number          number        Required: Number of simulations to perform.\n"
      "  -o  --options         file          Required: Simulation engine options file.\n"
      "  -r  --results         file          Required: Simulation results file to compare generated results to.\n"
      "  -t  --tolerance       tolerance     Optional: Radial tolerance. Non-negative floating point value. Default is 0.\n"
      "  -u  --upper           polarization  Optional: Upper polarization threshold. Between -1.00 and 1.00. Default is 0.5.\n"
      "  -v  --vector-table    file          Optional: Vector table file to use.\n"
      "  -x  --exit                          Optional: Turn on exit-on-first-failure.\n"
      "  -s  --save            file_prefix   Optional: Save failing simulation output to file_prefix.sim_output\n"
      "                                                  and the circuit to file_prefix.qca.\n"
      "  -I  --displace-in                   Optional: include the inputs in the displacements.\n"
      "  -O  --displace-out                  Optional: include the outputs in the displacements.\n")) ;
    exit (1) ;
    }
  }

static int determine_success (HONEYCOMB_DATA *hcdRef, HONEYCOMB_DATA *hcdOut, int delay, int ignore_from_end)
  {
  int Nix ;
  int idxRef = 0 ;
  HONEYCOMB *hcRef = NULL, *hcOut = NULL ;

  if (NULL == hcdRef || NULL == hcdOut) return 0 ;
  if (NULL == hcdRef->arHCs || NULL == hcdOut->arHCs) return 0 ;
  if (0 == hcdRef->arHCs->icUsed || 0 == hcdOut->arHCs->icUsed) return 0 ;

  flush_fprintf (stderr, _("Determining success for hcdRef = 0x%08x vs. hcdOut = 0x%08x\n"), (int)hcdRef, (int)hcdOut) ;

  // If both honeycomb arrays have the same number of honeycombs, compare the values
  if (hcdRef->arHCs->icUsed == hcdOut->arHCs->icUsed)
    {
    for (Nix = delay ; Nix < hcdRef->arHCs->icUsed - ignore_from_end ; Nix++)
      {
      hcRef = &exp_array_index_1d (hcdRef->arHCs, HONEYCOMB, Nix) ;
      hcOut = &exp_array_index_1d (hcdOut->arHCs, HONEYCOMB, Nix) ;
      // windoze doesn't seem to be able to format a string containing multiple %llu tokens
      flush_fprintf (stderr, _("Honeycomb value[%d]: "), Nix) ;
      flush_fprintf (stderr, _("reference: %llu "), hcRef->value) ;
      flush_fprintf (stderr, _("output: %llu\n"), hcOut->value) ;
      if (hcRef->value != hcOut->value)
        break ;
      }
	 
    flush_fprintf (stderr, "Nix = %d vs. hcdRef->arHCs->icUsed = %d\n", Nix, hcdRef->arHCs->icUsed - ignore_from_end) ;
    return (Nix == hcdRef->arHCs->icUsed - ignore_from_end) ? 1 : 0 ;
    }

  else
    {
	for (Nix = delay ; Nix < (hcdOut->arHCs->icUsed) ; Nix++)
      {
      hcRef = &exp_array_index_1d (hcdRef->arHCs, HONEYCOMB, Nix) ;
      hcOut = &exp_array_index_1d (hcdOut->arHCs, HONEYCOMB, Nix) ;
      // windoze doesn't seem to be able to format a string containing multiple %llu tokens
      flush_fprintf (stderr, _("Honeycomb value[%d]: "), Nix) ;
      flush_fprintf (stderr, _("reference: %llu "), hcRef->value) ;
      flush_fprintf (stderr, _("output: %llu\n"), hcOut->value) ;
      if (hcRef->value != hcOut->value)
        break ;
      }
	 
    flush_fprintf (stderr, "Nix = %d vs. hcdRef->arHCs->icUsed = %d\n", Nix, hcdRef->arHCs->icUsed - ignore_from_end) ;
    return (Nix == hcdRef->arHCs->icUsed - ignore_from_end) ? 1 : 0 ;
    }

  hcRef = &exp_array_index_1d (hcdRef->arHCs, HONEYCOMB, 0) ;

  for (Nix = 0 ; Nix < hcdOut->arHCs->icUsed ; Nix++)
    {
    hcOut = &exp_array_index_1d (hcdOut->arHCs, HONEYCOMB, Nix) ;

    // This output honeycomb may be contained withing the next input honeycomb
    if (hcOut->idxBeg > hcRef->idxEnd)
      {
      if (++idxRef == hcdRef->arHCs->icUsed - ignore_from_end) return 0 ;
      hcRef = &exp_array_index_1d (hcdRef->arHCs, HONEYCOMB, idxRef) ;
      }

    // The output honeycomb is not entirely contained within the input honeycomb
    if (hcOut->idxBeg < hcRef->idxBeg || hcOut->idxEnd > hcRef->idxEnd)
      return 0 ;

    if (hcOut->value != hcRef->value)
      return 0 ;
    }

  return 1 ;
  }
