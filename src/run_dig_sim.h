#ifndef _RUNDIGSIM_H_
#define _RUNDIGSIM_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "simulation.h"
#include "globals.h"
#include "vector_table.h"

simulation_data * run_digital_simulation(int sim_type, qcell *my_cell, void *p, VectorTable *pvt);

#ifdef __cplusplus
}
#endif
#endif
