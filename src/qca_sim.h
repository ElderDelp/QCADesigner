#ifndef _QCASIM_H_
#define _QCASIM_H_

#include <set>
#include <list>
#include <stack>
#include <map>
#include <time.h>
using namespace std;

#include "enkidu.h"
#include "cell.h"
#include "cell_neighbor.h"
#include "vector_table.h"

#define EXTRACYCLES 4

class qcaSim : public component {
protected:
	CellSet * cells;
	CellSet * inputs;
	CellSet * outputs;
	list<Cell *> * activated_inputs;
	bool error;
	int cur_cycle;
	int current_input_combo;
	int num_zones;
	
	int SIMULATION_TYPE ;
	VectorTable *pvt ;

public:
	qcaSim(string nm, const int sim_type, const GQCell * my_cell, VectorTable *pvtIn);
	~qcaSim();
	void init();
	void calculateNumZones();
	virtual void setup() {printf("Setup does nothing!\n");}
	virtual void preTic();
	virtual void postTic();
	virtual void finish() {printf("Finish does nothing!\n");}
	virtual void handleParcel(parcel *p);
	void loadCells(const GQCell * my_cell);
	void prepareInitialParcels();
	bool simulationWasSuccessful() {return !error;}
	bool is_exhaustive;
};

#endif
