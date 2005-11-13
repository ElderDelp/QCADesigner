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
// Header for the QCA cell.                             //
//                                                      //
//////////////////////////////////////////////////////////

#ifndef _OBJECTS_QCADRectangleElectrode_H_
#define _OBJECTS_QCADRectangleElectrode_H_

#include <glib-object.h>
#include "../gdk_structs.h"
#include "../exp_array.h"
#include "QCADElectrode.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct
  {
  QCADElectrodeOptions electrode_options ;
  double angle ;
  int n_x_divisions ;
  int n_y_divisions ;
  double cxWorld ;
  double cyWorld ;
  } QCADRectangleElectrodeOptions ;

typedef struct
  {
  double pt1x_minus_pt0x ;
  double pt1y_minus_pt0y ;
  double pt3x_minus_pt2x ;
  double pt3y_minus_pt2y ;
  double reciprocal_of_x_divisions ;
  double reciprocal_of_y_divisions ;
  double rho_factor ;
  } QCADRectangleElectrodePrecompute ;

typedef struct
  {
  QCADElectrode parent_instance ;
  double angle ;
  int n_x_divisions ;
  int n_y_divisions ;
  double cxWorld ;
  double cyWorld ;
  WorldPoint pt[4] ;
  QCADRectangleElectrodePrecompute precompute_params ;
  } QCADRectangleElectrode ;

typedef struct
  {
  /* public */
  QCADElectrodeClass parent_class ;
  double default_angle ;
  int default_n_x_divisions ;
  int default_n_y_divisions ;
  double default_cxWorld ;
  double default_cyWorld ;
  } QCADRectangleElectrodeClass ;

GType qcad_rectangle_electrode_get_type () ;

#define QCAD_TYPE_STRING_RECTANGLE_ELECTRODE "QCADRectangleElectrode"
#define QCAD_TYPE_RECTANGLE_ELECTRODE (qcad_rectangle_electrode_get_type ())
#define QCAD_RECTANGLE_ELECTRODE(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), QCAD_TYPE_RECTANGLE_ELECTRODE, QCADRectangleElectrode))
#define QCAD_RECTANGLE_ELECTRODE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), QCAD_TYPE_RECTANGLE_ELECTRODE, QCADRectangleElectrodeClass))
#define QCAD_IS_RECTANGLE_ELECTRODE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), QCAD_TYPE_RECTANGLE_ELECTRODE))
#define QCAD_IS_RECTANGLE_ELECTRODE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), QCAD_TYPE_RECTANGLE_ELECTRODE))
#define QCAD_RECTANGLE_ELECTRODE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), QCAD_TYPE_RECTANGLE_ELECTRODE, QCADRectangleElectrodeClass))

QCADDesignObject *qcad_rectangle_electrode_new () ;

///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
#endif /* _OBJECTS_QCADElectrode_H_ */
