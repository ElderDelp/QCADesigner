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
// The layer. This is a structure containing design     //
// objects. The kinds of objects a layer may contain    //
// depend on the kind of layer it is.                   //
//                                                      //
//////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "QCADClockingLayer.h"
#include "../intl.h"
#include "../fileio_helpers.h"
#include "QCADElectrode.h"
#include "QCADRectangleElectrode.h"
#include "../global_consts.h"
#include "../custom_widgets.h"
#include "../ts_field_clock.h"
enum
  {
  QCAD_CLOCKING_LAYER_PROPERTY_FIRST = 1,
  QCAD_CLOCKING_LAYER_PROPERTY_SHOW_POTENTIAL,
  QCAD_CLOCKING_LAYER_PROPERTY_Z_SHOWING,
  QCAD_CLOCKING_LAYER_PROPERTY_TILE_SIZE,
  QCAD_CLOCKING_LAYER_PROPERTY_TIME_COORD,
  QCAD_CLOCKING_LAYER_PROPERTY_LAST
  } ;

#ifdef PROPERTY_UIS
// Gotta be static so the strings don't die
static QCADPropertyUIBehaviour behaviour[] =
  {
    {
    // clocking_layer.z-showing.sensitive = clocking_layer.show-potential
    "show-potential", NULL, "z-showing", "sensitive", 
    CONNECT_OBJECT_PROPERTIES_ASSIGN, NULL, NULL,
    NULL, NULL, NULL
    },
    {
    // clocking_layer.tile-size.sensitive = clocking_layer.show-potential
    "show-potential", NULL, "tile-size", "sensitive", 
    CONNECT_OBJECT_PROPERTIES_ASSIGN, NULL, NULL,
    NULL, NULL, NULL
    },
    {
    // clocking_layer.time-coord.sensitive = clocking_layer.show-potential
    "show-potential", NULL, "time-coord", "sensitive", 
    CONNECT_OBJECT_PROPERTIES_ASSIGN, NULL, NULL,
    NULL, NULL, NULL
    }
  } ;
#endif /* def PROPERTY_UIS */

static void qcad_clocking_layer_class_init (QCADDesignObjectClass *klass, gpointer data) ;
static void qcad_clocking_layer_instance_init (QCADDesignObject *object, gpointer data) ;
static void qcad_clocking_layer_instance_finalize (GObject *object) ;
static void set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) ;
static void get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec) ;
static void serialize (QCADDesignObject *obj, FILE *fp) ;
static gboolean unserialize (QCADDesignObject *obj, FILE *fp) ;
static gboolean do_container_add (QCADDOContainer *container, QCADDesignObject *obj) ;
static gboolean do_container_remove (QCADDOContainer *container, QCADDesignObject *obj) ;
#ifdef GTK_GUI
static void draw (QCADDesignObject *obj, GdkDrawable *dst, GdkFunction rop, GdkRectangle *rcClip) ;
#endif /* def GTK_GUI */
static QCADObject *class_get_default_object () ;

static void qcad_clocking_layer_calculate_extreme_potentials (QCADClockingLayer *clocking_layer) ;
static void qcad_clocking_layer_set_tile_size (QCADClockingLayer *clocking_layer, guint new_tile_size) ;
void get_grid_param(int N_x, int N_y, int N_z, double d_x, double d_y, double d_z, int x_min, int y_min);

int Nx, Ny, Nz;
double dx, dy, dz;
int xmin, ymin;

GType qcad_clocking_layer_get_type ()
  {
  static GType qcad_clocking_layer_type = 0 ;

  if (!qcad_clocking_layer_type)
    {
    static const GTypeInfo qcad_clocking_layer_info =
      {
      sizeof (QCADClockingLayerClass),
      (GBaseInitFunc)NULL,
      (GBaseFinalizeFunc)NULL,
      (GClassInitFunc)qcad_clocking_layer_class_init,
      (GClassFinalizeFunc)NULL,
      NULL,
      sizeof (QCADClockingLayer),
      0,
      (GInstanceInitFunc)qcad_clocking_layer_instance_init
      } ;

    if ((qcad_clocking_layer_type = g_type_register_static (QCAD_TYPE_LAYER, QCAD_TYPE_STRING_CLOCKING_LAYER, &qcad_clocking_layer_info, 0)))
      g_type_class_ref (qcad_clocking_layer_type) ;
    }
  return qcad_clocking_layer_type ;
  }

static void qcad_clocking_layer_class_init (QCADDesignObjectClass *klass, gpointer data)
  {
#ifdef PROPERTY_UIS
  // Gotta be static so the strings don't die
  static QCADPropertyUIProperty properties[] =
    {
    {NULL,          "title", {0, }},
    {"z-showing",   "units", {0, }},
    {"tile-size",   "units", {0, }},
    {"time-coord",  "units", {0, }},
    } ;

  // clocking_layer.title = "QCA Clocking Layer"
  g_value_set_string (g_value_init (&(properties[0].ui_property_value), G_TYPE_STRING), _("QCA Clocking Layer")) ;
  // clocking_layer.z-showing.units = "nm"
  g_value_set_string (g_value_init (&(properties[1].ui_property_value), G_TYPE_STRING), "nm") ;
  // clocking_layer.tile-size.units = "pixels"
  g_value_set_string (g_value_init (&(properties[2].ui_property_value), G_TYPE_STRING), _("pixels")) ;
  // clocking_layer.time-coord.units = "ns"
  g_value_set_string (g_value_init (&(properties[3].ui_property_value), G_TYPE_STRING), "ns") ;

  qcad_object_class_install_ui_properties (QCAD_OBJECT_CLASS (klass), properties, G_N_ELEMENTS (properties)) ;
  qcad_object_class_install_ui_behaviour (QCAD_OBJECT_CLASS (klass), behaviour, G_N_ELEMENTS (behaviour)) ;
#endif /* def PROPERTY_UIS */

  G_OBJECT_CLASS (klass)->finalize     = qcad_clocking_layer_instance_finalize ;
  G_OBJECT_CLASS (klass)->set_property = set_property ;
  G_OBJECT_CLASS (klass)->get_property = get_property ;

  QCAD_OBJECT_CLASS (klass)->class_get_default_object = class_get_default_object ;

#ifdef GTK_GUI
  QCAD_DESIGN_OBJECT_CLASS (klass)->draw        = draw ;
#endif /* def GTK_GUI */
  QCAD_DESIGN_OBJECT_CLASS (klass)->serialize   = serialize ;
  QCAD_DESIGN_OBJECT_CLASS (klass)->unserialize = unserialize ;

  QCAD_LAYER_CLASS (klass)->do_container_add    = do_container_add ;
  QCAD_LAYER_CLASS (klass)->do_container_remove = do_container_remove ;

  g_object_class_install_property (G_OBJECT_CLASS (klass), QCAD_CLOCKING_LAYER_PROPERTY_SHOW_POTENTIAL,
    g_param_spec_boolean ("show-potential", _("Show Potential"), _("Show potential created by the electrodes on this layer"),
      FALSE, G_PARAM_READABLE | G_PARAM_WRITABLE)) ;

  g_object_class_install_property (G_OBJECT_CLASS (klass), QCAD_CLOCKING_LAYER_PROPERTY_Z_SHOWING,
    g_param_spec_double ("z-showing", _("Distance from layer"), _("Distance from clocking layer to show the cross-section for"),
      -G_MAXDOUBLE, G_MAXDOUBLE, 1, G_PARAM_READABLE | G_PARAM_WRITABLE)) ;

  g_object_class_install_property (G_OBJECT_CLASS (klass), QCAD_CLOCKING_LAYER_PROPERTY_TILE_SIZE,
    g_param_spec_uint ("tile-size", _("Tile Size"), _("Resolution (n x n pixels) used to draw the potential"),
      1, G_MAXUINT, 16, G_PARAM_READABLE | G_PARAM_WRITABLE)) ;

  g_object_class_install_property (G_OBJECT_CLASS (klass), QCAD_CLOCKING_LAYER_PROPERTY_TIME_COORD,
    g_param_spec_double ("time-coord", _("Time Coordinate"), _("Time coordinate to draw the potential for"),
      0, G_MAXDOUBLE, 0, G_PARAM_READABLE | G_PARAM_WRITABLE)) ;
  }

static void qcad_clocking_layer_instance_init (QCADDesignObject *object, gpointer data)
  {
  QCADLayer *layer = QCAD_LAYER (object) ;

  QCAD_CLOCKING_LAYER (layer)->bDrawPotential = FALSE ;
  QCAD_CLOCKING_LAYER (layer)->z_to_draw             =  1 ;
  QCAD_CLOCKING_LAYER (layer)->tile_size             = 16 ;
  QCAD_CLOCKING_LAYER (layer)->time_coord            =  0.0 ;
  QCAD_CLOCKING_LAYER (layer)->dExtremePotential     =  0 ;
  }

static void qcad_clocking_layer_instance_finalize (GObject *object)
  {G_OBJECT_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_LAYER)))->finalize (object) ;}

static gboolean do_container_add (QCADDOContainer *container, QCADDesignObject *obj)
  {
  gboolean bRet = FALSE ;
  QCADClockingLayer *clocking_layer = QCAD_CLOCKING_LAYER (container) ;

  if ((bRet = QCAD_LAYER_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_CLOCKING_LAYER)))->do_container_add (container, obj)))
    if (QCAD_IS_ELECTRODE (obj))
      {
      EXTREME_POTENTIALS dElectrodeExtremePotential = qcad_electrode_get_extreme_potential (QCAD_ELECTRODE (obj), clocking_layer->z_to_draw) ;

//      fprintf (stderr, "QCADClockingLayer::do_container_add:Adding electrode 0x%08X with potentials (%e,%e)\n", (int)obj, dElectrodeExtremePotential.min, dElectrodeExtremePotential.max) ;

      dElectrodeExtremePotential.min = fabs (dElectrodeExtremePotential.min) ;
      dElectrodeExtremePotential.max = fabs (dElectrodeExtremePotential.max) ;
      clocking_layer->dExtremePotential = MAX (dElectrodeExtremePotential.min, MAX (dElectrodeExtremePotential.max, clocking_layer->dExtremePotential)) ;

//      fprintf (stderr, "QCADClockingLayer::do_container_add:Resulting extreme potential is %e\n", clocking_layer->dExtremePotential) ;
      }

  return bRet ;
  }

static gboolean do_container_remove (QCADDOContainer *container, QCADDesignObject *obj)
  {
  gboolean bRet = FALSE ;
//  QCADClockingLayer *clocking_layer = QCAD_CLOCKING_LAYER (container) ;

  if ((bRet = QCAD_LAYER_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_CLOCKING_LAYER)))->do_container_remove (container, obj)))
    if (QCAD_IS_ELECTRODE (obj))
//      {
//      fprintf (stderr, "QCADClockingLayer::do_container_remove:Removing electrode 0x%08X\n", (int)obj) ;
      qcad_clocking_layer_calculate_extreme_potentials (QCAD_CLOCKING_LAYER (container)) ;
//      fprintf (stderr, "QCADClockingLayer::do_container_remove:Resulting extreme potential is %e\n", clocking_layer->dExtremePotential) ;
//      }

  return bRet ;
  }

static QCADObject *class_get_default_object ()
  {return QCAD_OBJECT (g_object_new (QCAD_TYPE_CLOCKING_LAYER, NULL)) ;}

static void set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
  {
  QCADClockingLayer *clocking_layer = QCAD_CLOCKING_LAYER (object) ;

  switch (property_id)
    {
    case QCAD_CLOCKING_LAYER_PROPERTY_SHOW_POTENTIAL:
      clocking_layer->bDrawPotential = g_value_get_boolean (value) ;
      g_object_notify (object, "show-potential") ;
      break ;

    case QCAD_CLOCKING_LAYER_PROPERTY_Z_SHOWING:
      clocking_layer->z_to_draw = g_value_get_double (value) ;
      g_object_notify (object, "z-showing") ;
      qcad_clocking_layer_calculate_extreme_potentials (clocking_layer) ;
      break ;

    case QCAD_CLOCKING_LAYER_PROPERTY_TILE_SIZE:
      qcad_clocking_layer_set_tile_size (clocking_layer, g_value_get_uint (value)) ;
      break ;

    case QCAD_CLOCKING_LAYER_PROPERTY_TIME_COORD:
      clocking_layer->time_coord = g_value_get_double (value) ;
      g_object_notify (object, "time-coord") ;
      break ;
    }
  }

static void get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
  {
  QCADClockingLayer *layer = QCAD_CLOCKING_LAYER (object) ;

  switch (property_id)
    {
    case QCAD_CLOCKING_LAYER_PROPERTY_SHOW_POTENTIAL:
      g_value_set_boolean (value, layer->bDrawPotential) ;
      break ;

    case QCAD_CLOCKING_LAYER_PROPERTY_Z_SHOWING:
      g_value_set_double (value, layer->z_to_draw) ;
      break ;

    case QCAD_CLOCKING_LAYER_PROPERTY_TILE_SIZE:
       g_value_set_uint (value, layer->tile_size) ;
      break ;

    case QCAD_CLOCKING_LAYER_PROPERTY_TIME_COORD:
       g_value_set_double (value, layer->time_coord) ;
      break ;
    }
  }

static void serialize (QCADDesignObject *obj, FILE *fp)
  {
  QCADClockingLayer *clocking_layer = NULL ;
  char pszDouble[G_ASCII_DTOSTR_BUF_SIZE] = "" ;

  if (NULL == obj) return ;

  clocking_layer = QCAD_CLOCKING_LAYER (obj) ;

  fprintf (fp, "[TYPE:" QCAD_TYPE_STRING_CLOCKING_LAYER "]\n") ;
  QCAD_DESIGN_OBJECT_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_CLOCKING_LAYER)))->serialize (obj, fp) ;
  fprintf (fp, "bDrawPotential=%s\n", clocking_layer->bDrawPotential ? "TRUE" : "FALSE") ;
  fprintf (fp, "z_to_draw=%s\n", g_ascii_dtostr (pszDouble, G_ASCII_DTOSTR_BUF_SIZE, clocking_layer->z_to_draw)) ;
  fprintf (fp, "tile_size=%d\n", clocking_layer->tile_size) ;
  fprintf (fp, "time_coord=%s\n", g_ascii_dtostr (pszDouble, G_ASCII_DTOSTR_BUF_SIZE, clocking_layer->time_coord)) ;
  fprintf (fp, "[#TYPE:" QCAD_TYPE_STRING_CLOCKING_LAYER "]\n") ;
  }

static gboolean unserialize (QCADDesignObject *obj, FILE *fp)
  {
  QCADClockingLayer *clocking_layer = QCAD_CLOCKING_LAYER (obj) ;
  char *pszLine = NULL, *pszValue = NULL ;
  gboolean bStopReading = FALSE, bParentInit = FALSE ;

  if (!SkipPast (fp, '\0', "[TYPE:" QCAD_TYPE_STRING_CLOCKING_LAYER "]", NULL)) return FALSE ;

  while (TRUE)
    {
    if (NULL == (pszLine = ReadLine (fp, '\0', TRUE))) break ;
    if (!strcmp ("[#TYPE:" QCAD_TYPE_STRING_CLOCKING_LAYER "]", pszLine))
      {
      g_free (pszLine) ;
      break ;
      }

    if (!bStopReading)
      {
      tokenize_line (pszLine, strlen (pszLine), &pszValue, '=') ;

      if (!strncmp (pszLine, "[TYPE:", 6))
        {
        tokenize_line_type (pszLine, strlen (pszLine), &pszValue, ':') ;

        if (!strcmp (pszValue, QCAD_TYPE_STRING_LAYER))
          {
          if (!(bParentInit = QCAD_DESIGN_OBJECT_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_CLOCKING_LAYER)))->unserialize (obj, fp)))
            bStopReading = TRUE ;
          }
        }
      else
      if (!strcmp (pszLine, "bDrawPotential"))
        clocking_layer->bDrawPotential = strcmp (pszValue, "TRUE") ? FALSE : TRUE ;
      else
      if (!strcmp (pszLine, "z_to_draw"))
        clocking_layer->z_to_draw = g_ascii_strtod (pszValue, NULL) ;
      else
      if (!strcmp (pszLine, "tile_size"))
        clocking_layer->tile_size = atoi (pszValue) ;
      else
      if (!strcmp (pszLine, "time_coord"))
        clocking_layer->time_coord = g_ascii_strtod (pszValue, NULL) ;
      }
    g_free (pszLine) ;
    g_free (ReadLine (fp, '\0', FALSE)) ;
    }
  return (bParentInit && !bStopReading) ;
  }

#ifdef GTK_GUI
static void draw (QCADDesignObject *obj, GdkDrawable *dst, GdkFunction rop, GdkRectangle *rcClip)
  {
	int Nix, Nix1 ;
  QCADClockingLayer *clocking_layer = QCAD_CLOCKING_LAYER (obj) ;
  QCADLayer *layer = QCAD_LAYER (obj) ;
  GdkGC *gc = NULL ;
  double potential ;
  GList *llItr = NULL ;
  double x, y;

  QCAD_DESIGN_OBJECT_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_CLOCKING_LAYER)))->draw (obj, dst, rop, rcClip) ;

  gc = gdk_gc_new (dst) ;
  gdk_gc_set_foreground (gc, clr_idx_to_clr_struct (RED)) ;
  gdk_gc_set_background (gc, clr_idx_to_clr_struct (RED)) ;
  gdk_gc_set_clip_rectangle (gc, rcClip) ;

  if (NULL != layer->lstObjs && clocking_layer->bDrawPotential)
    {
    GdkPixbuf *pb = NULL ;
    pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, clocking_layer->tile_size, clocking_layer->tile_size) ;
	
		for (Nix = 0; Nix < Nx; Nix++) {
			x = xmin+dx*Nix;
			for (Nix1 = 0; Nix1 < Ny; Nix1++) {
				y = ymin+dy*Nix1;
				potential = get_potential (x, y, clocking_layer->z_to_draw, Nx, Ny, Nz, dx, dy, dz, xmin, ymin)  ;
								
/*
				 
        if (fabs (potential) < clocking_layer->dExtremePotential / 100.0)
          {
          fprintf (stderr, "Potential too small - breaking out\n") ;
//          continue ;
          }
*/
        gdk_pixbuf_fill (pb,
          ((potential > 0) ? 0xFF000000 : 0x0000FF00) | (((int)((fabs (potential) / clocking_layer->dExtremePotential) * 128.0)) & 0xFF)) ;
//        fprintf (stderr, "opacity = %lf/%lf * 255\n", potential, clocking_layer->dExtremePotential) ;
				
        gdk_draw_pixbuf (dst, gc, pb, 0, 0, Nix * clocking_layer->tile_size,  Nix1 * clocking_layer->tile_size, clocking_layer->tile_size, clocking_layer->tile_size, GDK_RGB_DITHER_NONE, 0, 0) ;
		//gdk_draw_pixbuf (dst, gc, pb, 0, 0, Nix * clocking_layer->tile_size, Nix1 * clocking_layer->tile_size, clocking_layer->tile_size, clocking_layer->tile_size, GDK_RGB_DITHER_NONE, 0, 0) ;
				
//        gdk_draw_rectangle (dst, gc, TRUE, 
//          Nix * clocking_layer->tile_size + (clocking_layer->tile_size >> 1) - 2,
//          Nix1 * clocking_layer->tile_size + (clocking_layer->tile_size >> 1) - 2,
//          5, 5) ;
			}
		}
    g_object_unref (pb) ;
    }

  g_object_unref (gc) ;
	}
#endif /* def GTK_GUI */

static void qcad_clocking_layer_calculate_extreme_potentials (QCADClockingLayer *clocking_layer)
  {
  GList *llItr = NULL ;
  EXTREME_POTENTIALS dElectrodeExtremePotential = {0, 0} ;

//  fprintf (stderr, "QCADClockingLayer::calculate_extreme_potentials:Entering\n") ;

  clocking_layer->dExtremePotential = 0 ;

  for (llItr = QCAD_LAYER (clocking_layer)->lstObjs ; llItr != NULL ; llItr = llItr->next)
    if (NULL != llItr->data)
      if (QCAD_IS_ELECTRODE (llItr->data))
        {
        dElectrodeExtremePotential = qcad_electrode_get_extreme_potential (QCAD_ELECTRODE (llItr->data), clocking_layer->z_to_draw) ;
        dElectrodeExtremePotential.min = fabs (dElectrodeExtremePotential.min) ;
        dElectrodeExtremePotential.max = fabs (dElectrodeExtremePotential.max) ;
        clocking_layer->dExtremePotential = MAX (dElectrodeExtremePotential.min, MAX (dElectrodeExtremePotential.max, clocking_layer->dExtremePotential)) ;
        }
  }

static void qcad_clocking_layer_set_tile_size (QCADClockingLayer *clocking_layer, guint new_tile_size)
  {
  guint old_tile_size = clocking_layer->tile_size ;
  int bits = floor (log (new_tile_size) / log (2)) + 1 ;

  // if new_tile_size is NOT a power of two
  if (new_tile_size & (new_tile_size - 1))
    {
    // User wants to increase
    if (new_tile_size > old_tile_size)
      new_tile_size = (1 << ((int)(MAX (bits, floor (log (new_tile_size) / log (2)) + 1)))) ;
    // User wants to decrease
    else
      new_tile_size = (1 << ((int)(MIN (bits, floor (log (new_tile_size) / log (2)))))) ;
    }

  if (clocking_layer->tile_size != new_tile_size)
    {
    clocking_layer->tile_size = new_tile_size ;
    g_object_notify (G_OBJECT (clocking_layer), "tile-size") ;
    }
  }

void get_grid_param(int N_x, int N_y, int N_z, double d_x, double d_y, double d_z, int x_min, int y_min) 
{
	Nx = N_x;
	Ny = N_y;
	Nz = N_z;
	dx = d_x;
	dy = d_y;
	dz = d_z;
	xmin = x_min;
	ymin = y_min;
}