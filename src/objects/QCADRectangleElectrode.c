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
// The QCA cell.                                        //
//                                                      //
//////////////////////////////////////////////////////////

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib-object.h>

#ifdef GTK_GUI
  #include <gtk/gtk.h>
#endif

#include "objects_debug.h"

#include "../generic_utils.h"
#include "../support.h"
#include "../global_consts.h"
#include "../custom_widgets.h"
#include "../fileio_helpers.h"
#include "QCADRectangleElectrode.h"
#include "mouse_handlers.h"

#ifdef GTK_GUI
typedef struct
  {
  GtkWidget *tbl ;
  GtkWidget *clock_function_option_menu ;
  GtkAdjustment *adjAmplitude ;
  GtkAdjustment *adjFrequency ;
  GtkAdjustment *adjPhase ;
  GtkAdjustment *adjMinClock ;
  GtkAdjustment *adjMaxClock ;
  GtkAdjustment *adjDCOffset ;
  GtkAdjustment *adjAngle ;
  GtkAdjustment *adjNXDivisions ;
  GtkAdjustment *adjNYDivisions ;
  GtkAdjustment *adjCX ;
  GtkAdjustment *adjCY ;
  } DEFAULT_PROPERTIES ;

typedef struct
  {
  GtkWidget *dlg ;
  DEFAULT_PROPERTIES contents ;
  } PROPERTIES ;
#endif /* def GTK_GUI */

#ifdef DESIGNER
extern DropFunction drop_function ;
#endif /* def DESIGNER */

static struct { ClockFunction clock_function ; char *pszDescription ; } clock_functions[1]  =
  {
  {sin, "sin"}
  } ;

int n_clock_functions = G_N_ELEMENTS (clock_functions) ;

static void qcad_rectangle_electrode_class_init (GObjectClass *klass, gpointer data) ;
static void qcad_rectangle_electrode_instance_init (GObject *object, gpointer data) ;
static void qcad_rectangle_electrode_instance_finalize (GObject *object) ;

#ifdef GTK_GUI
static void draw (QCADDesignObject *obj, GdkDrawable *dst, GdkFunction rop, GdkRectangle *rcClip) ;
static gboolean button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data) ;
static GCallback default_properties_ui (QCADDesignObjectClass *klass, void *default_properties, GtkWidget **pTopContainer, gpointer *pData) ;
#ifdef UNDO_REDO
gboolean properties (QCADDesignObject *obj, GtkWidget *parent, QCADUndoEntry **pentry) ;
#else
gboolean properties (QCADDesignObject *obj, GtkWidget *parent) ;
#endif /* def UNDO_REDO */
#endif /* def GTK_GUI */
static void *default_properties_get (struct QCADDesignObjectClass *klass) ;
static void default_properties_set (struct QCADDesignObjectClass *klass, void *props) ;
static void default_properties_destroy (struct QCADDesignObjectClass *klass, void *props) ;
static void move (QCADDesignObject *obj, double dxDelta, double dyDelta) ;
static double get_potential (QCADElectrode *electrode, double x, double y, double z, double t) ;
static double get_area (QCADElectrode *electrode) ;
static void serialize (QCADDesignObject *obj, FILE *fp) ;
static gboolean unserialize (QCADDesignObject *obj, FILE *fp) ;
static EXTREME_POTENTIALS extreme_potential (QCADElectrode *electrode, double z) ;
#ifdef GTK_GUI
static void create_default_properties_dialog (DEFAULT_PROPERTIES *dialog) ;
static void create_properties_dialog (PROPERTIES *dialog) ;
static void default_properties_apply (gpointer data) ;
#endif
static void precompute (QCADElectrode *electrode) ;

GType qcad_rectangle_electrode_get_type ()
  {
  static GType qcad_rectangle_electrode_type = 0 ;

  if (!qcad_rectangle_electrode_type)
    {
    static const GTypeInfo qcad_rectangle_electrode_info =
      {
      sizeof (QCADRectangleElectrodeClass),
      (GBaseInitFunc)NULL,
      (GBaseFinalizeFunc)NULL,
      (GClassInitFunc)qcad_rectangle_electrode_class_init,
      (GClassFinalizeFunc)NULL,
      NULL,
      sizeof (QCADRectangleElectrode),
      0,
      (GInstanceInitFunc)qcad_rectangle_electrode_instance_init
      } ;

    if ((qcad_rectangle_electrode_type = g_type_register_static (QCAD_TYPE_ELECTRODE, QCAD_TYPE_STRING_RECTANGLE_ELECTRODE, &qcad_rectangle_electrode_info, 0)))
      g_type_class_ref (qcad_rectangle_electrode_type) ;
    DBG_OO (fprintf (stderr, "Registered QCADRectangleElectrode as %d\n", qcad_cell_type)) ;
    }
  return qcad_rectangle_electrode_type ;
  }

static void qcad_rectangle_electrode_class_init (GObjectClass *klass, gpointer data)
  {
  DBG_OO (fprintf (stderr, "QCADRectangleElectrode::class_init:Leaving\n")) ;
  G_OBJECT_CLASS (klass)->finalize = qcad_rectangle_electrode_instance_finalize ;
#ifdef GTK_GUI
  QCAD_DESIGN_OBJECT_CLASS (klass)->draw                       = draw ;
  QCAD_DESIGN_OBJECT_CLASS (klass)->default_properties_ui      = default_properties_ui ;
  QCAD_DESIGN_OBJECT_CLASS (klass)->properties                 = properties ;
  QCAD_DESIGN_OBJECT_CLASS (klass)->mh.button_pressed          = (GCallback)button_pressed ;
#endif /* def GTK_GUI */
  QCAD_DESIGN_OBJECT_CLASS (klass)->move                       = move ;
  QCAD_DESIGN_OBJECT_CLASS (klass)->default_properties_get     = default_properties_get ;
  QCAD_DESIGN_OBJECT_CLASS (klass)->default_properties_set     = default_properties_set ;
  QCAD_DESIGN_OBJECT_CLASS (klass)->default_properties_destroy = default_properties_destroy ;
  QCAD_DESIGN_OBJECT_CLASS (klass)->serialize                  = serialize ;
  QCAD_DESIGN_OBJECT_CLASS (klass)->unserialize                = unserialize ;

  QCAD_ELECTRODE_CLASS (klass)->get_potential     = get_potential ;
  QCAD_ELECTRODE_CLASS (klass)->get_area          = get_area ;
  QCAD_ELECTRODE_CLASS (klass)->precompute        = precompute ;
  QCAD_ELECTRODE_CLASS (klass)->extreme_potential = extreme_potential ;

  QCAD_RECTANGLE_ELECTRODE_CLASS (klass)->default_angle = 0.0 ;
  QCAD_RECTANGLE_ELECTRODE_CLASS (klass)->default_n_x_divisions = 2 ;
  QCAD_RECTANGLE_ELECTRODE_CLASS (klass)->default_n_y_divisions = 40 ;
  QCAD_RECTANGLE_ELECTRODE_CLASS (klass)->default_cxWorld =  6.0 ;
  QCAD_RECTANGLE_ELECTRODE_CLASS (klass)->default_cyWorld = 40.0 ;
  }

static void qcad_rectangle_electrode_instance_init (GObject *object, gpointer data)
  {
  QCADRectangleElectrode *rc_electrode = QCAD_RECTANGLE_ELECTRODE (object) ;
  QCADRectangleElectrodeClass *klass = QCAD_RECTANGLE_ELECTRODE_GET_CLASS (object) ;
  DBG_OO (fprintf (stderr, "QCADElectrode::instance_init:Entering\n")) ;

  rc_electrode->n_x_divisions = klass->default_n_x_divisions ;
  rc_electrode->n_y_divisions = klass->default_n_y_divisions ;
  rc_electrode->cxWorld       = klass->default_cxWorld ;
  rc_electrode->cyWorld       = klass->default_cyWorld ;
  rc_electrode->angle         = klass->default_angle ;
  precompute (QCAD_ELECTRODE (object)) ;

  DBG_OO (fprintf (stderr, "QCADElectrode::instance_init:Leaving\n")) ;
  }

static void qcad_rectangle_electrode_instance_finalize (GObject *object)
  {
  DBG_OO (fprintf (stderr, "QCADElectrode::instance_finalize:Entering\n")) ;
  G_OBJECT_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_RECTANGLE_ELECTRODE)))->finalize (object) ;
  DBG_OO (fprintf (stderr, "QCADElectrode::instance_finalize:Leaving\n")) ;
  }

///////////////////////////////////////////////////////////////////////////////

QCADDesignObject *qcad_rectangle_electrode_new ()
  {return QCAD_DESIGN_OBJECT (g_object_new (QCAD_TYPE_RECTANGLE_ELECTRODE, NULL)) ;}

///////////////////////////////////////////////////////////////////////////////

static EXTREME_POTENTIALS extreme_potential (QCADElectrode *electrode, double z)
  {
  EXTREME_POTENTIALS ret = {0, 0} ;
  double p_over_two_pi_f = (electrode->electrode_options.phase / (TWO_PI * electrode->electrode_options.frequency)) ;
  double one_over_four_f = (1.0 / (4.0 * electrode->electrode_options.frequency)) ;
  // This assumes a sin function
  ret.max = get_potential (electrode, QCAD_DESIGN_OBJECT (electrode)->x, QCAD_DESIGN_OBJECT (electrode)->y, z, 
    p_over_two_pi_f + one_over_four_f) ;
  ret.min = get_potential (electrode, QCAD_DESIGN_OBJECT (electrode)->x, QCAD_DESIGN_OBJECT (electrode)->y, z, 
    p_over_two_pi_f - one_over_four_f) ;

  return ret ;
  }

static void serialize (QCADDesignObject *obj, FILE *fp)
  {
  char pszDouble[G_ASCII_DTOSTR_BUF_SIZE] = "" ;
  QCADRectangleElectrode *rc_electrode = QCAD_RECTANGLE_ELECTRODE (obj) ;

  fprintf (fp, "[TYPE:" QCAD_TYPE_STRING_RECTANGLE_ELECTRODE "]\n") ;
  QCAD_DESIGN_OBJECT_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_RECTANGLE_ELECTRODE)))->serialize (obj, fp) ;
  fprintf (fp, "angle=%s\n", g_ascii_dtostr (pszDouble, G_ASCII_DTOSTR_BUF_SIZE, rc_electrode->angle)) ;
  fprintf (fp, "n_x_divisions=%d\n", rc_electrode->n_x_divisions) ;
  fprintf (fp, "n_y_divisions=%d\n", rc_electrode->n_y_divisions) ;
  fprintf (fp, "cxWorld=%s\n", g_ascii_dtostr (pszDouble, G_ASCII_DTOSTR_BUF_SIZE, rc_electrode->cxWorld)) ;
  fprintf (fp, "cyWorld=%s\n", g_ascii_dtostr (pszDouble, G_ASCII_DTOSTR_BUF_SIZE, rc_electrode->cyWorld)) ;
  fprintf (fp, "[#TYPE:" QCAD_TYPE_STRING_RECTANGLE_ELECTRODE "]\n") ;
  }

static gboolean unserialize (QCADDesignObject *obj, FILE *fp)
  {
  QCADRectangleElectrode *rc_electrode = QCAD_RECTANGLE_ELECTRODE (obj) ;
  char *pszLine = NULL, *pszValue = NULL ;
  gboolean bStopReading = FALSE, bParentInit = FALSE ;

  if (!SkipPast (fp, '\0', "[TYPE:" QCAD_TYPE_STRING_RECTANGLE_ELECTRODE "]", NULL)) return FALSE ;

  while (TRUE)
    {
    if (NULL == (pszLine = ReadLine (fp, '\0', TRUE))) break ;

    if (!strcmp ("[#TYPE:" QCAD_TYPE_STRING_RECTANGLE_ELECTRODE "]", pszLine))
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

        if (!strcmp (pszValue, QCAD_TYPE_STRING_ELECTRODE))
          {
          if (!(bParentInit = QCAD_DESIGN_OBJECT_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_RECTANGLE_ELECTRODE)))->unserialize (obj, fp)))
            bStopReading = TRUE ;
          }
        }
      else
      if (!strcmp (pszLine, "angle"))
        rc_electrode->angle = g_ascii_strtod (pszValue, NULL) ;
      else
      if (!strcmp (pszLine, "n_x_divisions"))
        rc_electrode->n_x_divisions = atoi (pszValue) ;
      else
      if (!strcmp (pszLine, "n_y_divisions"))
        rc_electrode->n_y_divisions = atoi (pszValue) ;
      else
      if (!strcmp (pszLine, "cxWorld"))
        rc_electrode->cxWorld = g_ascii_strtod (pszValue, NULL) ;
      else
      if (!strcmp (pszLine, "cyWorld"))
        rc_electrode->cyWorld = g_ascii_strtod (pszValue, NULL) ;
      }
    g_free (pszLine) ;
    g_free (ReadLine (fp, '\0', FALSE)) ;
    }

  if (bParentInit && !bStopReading)
    {
    if (QCAD_DESIGN_OBJECT_CLASS (QCAD_RECTANGLE_ELECTRODE_GET_CLASS (rc_electrode))->unserialize == unserialize)
      QCAD_ELECTRODE_GET_CLASS (rc_electrode)->precompute (QCAD_ELECTRODE (rc_electrode)) ;
    return TRUE ;
    }
  else
    return FALSE ;
  }

static void move (QCADDesignObject *obj, double dxDelta, double dyDelta)
  {
  int Nix, Nix1 ;
  QCADRectangleElectrode *rc_electrode = QCAD_RECTANGLE_ELECTRODE (obj) ;

  QCAD_DESIGN_OBJECT_CLASS (g_type_class_peek_parent (QCAD_DESIGN_OBJECT_GET_CLASS (obj)))->move (obj, dxDelta, dyDelta) ;

  rc_electrode->precompute_params.pt[0].xWorld += dxDelta ;
  rc_electrode->precompute_params.pt[0].yWorld += dyDelta ;
  rc_electrode->precompute_params.pt[1].xWorld += dxDelta ;
  rc_electrode->precompute_params.pt[1].yWorld += dyDelta ;
  rc_electrode->precompute_params.pt[2].xWorld += dxDelta ;
  rc_electrode->precompute_params.pt[2].yWorld += dyDelta ;
  rc_electrode->precompute_params.pt[3].xWorld += dxDelta ;
  rc_electrode->precompute_params.pt[3].yWorld += dyDelta ;

  if (NULL != rc_electrode->precompute_params.pts)
    for (Nix = 0 ; Nix < rc_electrode->n_x_divisions ; Nix++)
      for (Nix1 = 0 ; Nix1 < rc_electrode->n_y_divisions ; Nix1++)
        {
        exp_array_index_2d (rc_electrode->precompute_params.pts, WorldPoint, Nix1, Nix).xWorld += dxDelta ;
        exp_array_index_2d (rc_electrode->precompute_params.pts, WorldPoint, Nix1, Nix).yWorld += dyDelta ;
        }
  }

#ifdef GTK_GUI
#ifdef UNDO_REDO
gboolean properties (QCADDesignObject *obj, GtkWidget *parent, QCADUndoEntry **pentry)
#else
gboolean properties (QCADDesignObject *obj, GtkWidget *parent)
#endif /* def UNDO_REDO */
  {
  QCADElectrode *electrode = QCAD_ELECTRODE (obj) ;
  QCADRectangleElectrode *rc_electrode = QCAD_RECTANGLE_ELECTRODE (obj) ;
  static PROPERTIES dialog = {NULL} ;

  if (NULL == dialog.dlg)
    create_properties_dialog (&dialog) ;
  
  gtk_option_menu_set_history (GTK_OPTION_MENU (dialog.contents.clock_function_option_menu), (sin == electrode->electrode_options.clock_function) ? 0 : -1) ;
  gtk_adjustment_set_value (dialog.contents.adjAmplitude,   electrode->electrode_options.amplitude) ;
  gtk_adjustment_set_value (dialog.contents.adjFrequency,   electrode->electrode_options.frequency) ;
  gtk_adjustment_set_value (dialog.contents.adjPhase,       electrode->electrode_options.phase) ;
  gtk_adjustment_set_value (dialog.contents.adjMinClock,    electrode->electrode_options.min_clock) ;
  gtk_adjustment_set_value (dialog.contents.adjMaxClock,    electrode->electrode_options.max_clock) ;
  gtk_adjustment_set_value (dialog.contents.adjDCOffset,    electrode->electrode_options.dc_offset) ;
  gtk_adjustment_set_value (dialog.contents.adjAngle,       rc_electrode->angle) ;
  gtk_adjustment_set_value (dialog.contents.adjNXDivisions, rc_electrode->n_x_divisions) ;
  gtk_adjustment_set_value (dialog.contents.adjNYDivisions, rc_electrode->n_y_divisions) ;
  gtk_adjustment_set_value (dialog.contents.adjCX,          rc_electrode->cxWorld) ;
  gtk_adjustment_set_value (dialog.contents.adjCY,          rc_electrode->cyWorld) ;

  if (GTK_RESPONSE_OK == gtk_dialog_run (GTK_DIALOG (dialog.dlg)))
    {
    
    electrode->electrode_options.amplitude = gtk_adjustment_get_value (dialog.contents.adjAmplitude) ;
    electrode->electrode_options.frequency = gtk_adjustment_get_value (dialog.contents.adjFrequency) ;
    electrode->electrode_options.phase     = gtk_adjustment_get_value (dialog.contents.adjPhase) ;
    electrode->electrode_options.min_clock = gtk_adjustment_get_value (dialog.contents.adjMinClock) ;
    electrode->electrode_options.max_clock = gtk_adjustment_get_value (dialog.contents.adjMaxClock) ;
    electrode->electrode_options.dc_offset = gtk_adjustment_get_value (dialog.contents.adjDCOffset) ;
    rc_electrode->angle                    = gtk_adjustment_get_value (dialog.contents.adjAngle) ;
    rc_electrode->n_x_divisions            = gtk_adjustment_get_value (dialog.contents.adjNXDivisions) ;
    rc_electrode->n_y_divisions            = gtk_adjustment_get_value (dialog.contents.adjNYDivisions) ;
    rc_electrode->cxWorld                  = gtk_adjustment_get_value (dialog.contents.adjCX) ;
    rc_electrode->cyWorld                  = gtk_adjustment_get_value (dialog.contents.adjCY) ;
    }

  gtk_widget_hide (dialog.dlg) ;

  return FALSE ;
  }

static GCallback default_properties_ui (QCADDesignObjectClass *klass, void *default_properties, GtkWidget **pTopContainer, gpointer *pData)
  {
  int Nix ;
  QCADElectrodeClass *electrode_class = QCAD_ELECTRODE_CLASS (klass) ;
  QCADRectangleElectrodeClass *rc_electrode_class = QCAD_RECTANGLE_ELECTRODE_CLASS (klass) ;
  static DEFAULT_PROPERTIES dialog = {NULL} ;
  QCADRectangleElectrodeOptions rcz_options ;

  if (NULL == dialog.tbl)
    create_default_properties_dialog (&dialog) ;

  if (NULL == default_properties)
    {
    memcpy (&rcz_options, &(electrode_class->default_electrode_options), sizeof (QCADElectrodeOptions)) ;
    rcz_options.angle         = rc_electrode_class->default_angle ;
    rcz_options.n_x_divisions = rc_electrode_class->default_n_x_divisions ;
    rcz_options.n_y_divisions = rc_electrode_class->default_n_y_divisions ;
    rcz_options.cxWorld       = rc_electrode_class->default_cxWorld ;
    rcz_options.cyWorld       = rc_electrode_class->default_cyWorld ;
    }
  else
    memcpy (&rcz_options, default_properties, sizeof (QCADRectangleElectrodeOptions)) ;

  for (Nix = 0 ; Nix < n_clock_functions ; Nix++)
    if (clock_functions[Nix].clock_function == rcz_options.electrode_options.clock_function) break ;
  if (Nix == n_clock_functions) Nix = -1 ;

  gtk_option_menu_set_history (GTK_OPTION_MENU (dialog.clock_function_option_menu), Nix) ;
  gtk_adjustment_set_value_infinite (dialog.adjAmplitude,   rcz_options.electrode_options.amplitude) ;
  gtk_adjustment_set_value_infinite (dialog.adjFrequency,   rcz_options.electrode_options.frequency / 1000000.0) ;
  gtk_adjustment_set_value_infinite (dialog.adjDCOffset,    rcz_options.electrode_options.dc_offset) ;
  gtk_adjustment_set_value_infinite (dialog.adjMinClock,    rcz_options.electrode_options.min_clock) ;
  gtk_adjustment_set_value_infinite (dialog.adjMaxClock,    rcz_options.electrode_options.max_clock) ;
  gtk_adjustment_set_value_infinite (dialog.adjNXDivisions, rcz_options.n_x_divisions) ;
  gtk_adjustment_set_value_infinite (dialog.adjNYDivisions, rcz_options.n_y_divisions) ;
  gtk_adjustment_set_value_infinite (dialog.adjCX,          rcz_options.cxWorld) ;
  gtk_adjustment_set_value_infinite (dialog.adjCY,          rcz_options.cyWorld) ;
  gtk_adjustment_set_value (dialog.adjPhase,       (rcz_options.electrode_options.phase * 180.0) / PI) ;
  gtk_adjustment_set_value (dialog.adjAngle,       (rcz_options.angle * 180.0) / PI) ;

  (*pTopContainer) = dialog.tbl ;
  (*pData) = &dialog ;
  return (GCallback)default_properties_apply ;
  }

static void draw (QCADDesignObject *obj, GdkDrawable *dst, GdkFunction rop, GdkRectangle *rcClip)
  {
  int Nix, Nix1 ;
  GdkGC *gc = NULL ;
  GdkRectangle rcReal ;
  GdkPoint ptSrc, ptDst ;
  QCADRectangleElectrode *rc_electrode = QCAD_RECTANGLE_ELECTRODE (obj) ;
  GdkColor *clr = NULL ;

  world_to_real_rect (&(obj->bounding_box), &rcReal) ;

  if (!RECT_INTERSECT_RECT (rcReal.x, rcReal.y, rcReal.width, rcReal.height, rcClip->x, rcClip->y, rcClip->width, rcClip->height))
    return ;

  clr = obj->bSelected ? clr_idx_to_clr_struct (RED) : &(obj->clr) ;
  gc = gdk_gc_new (dst) ;
  gdk_gc_set_foreground (gc, clr) ;
  gdk_gc_set_background (gc, clr) ;
  gdk_gc_set_function (gc, rop) ;
  gdk_gc_set_clip_rectangle (gc, rcClip) ;

  ptSrc.x = world_to_real_x (rc_electrode->precompute_params.pt[0].xWorld) ;
  ptSrc.y = world_to_real_y (rc_electrode->precompute_params.pt[0].yWorld) ;
  ptDst.x = world_to_real_x (rc_electrode->precompute_params.pt[1].xWorld) ;
  ptDst.y = world_to_real_y (rc_electrode->precompute_params.pt[1].yWorld) ;
  gdk_draw_line (dst, gc, ptSrc.x, ptSrc.y, ptDst.x, ptDst.y) ;
  ptSrc = ptDst ;
  ptDst.x = world_to_real_x (rc_electrode->precompute_params.pt[2].xWorld) ;
  ptDst.y = world_to_real_y (rc_electrode->precompute_params.pt[2].yWorld) ;
  gdk_draw_line (dst, gc, ptSrc.x, ptSrc.y, ptDst.x, ptDst.y) ;
  ptSrc = ptDst ;
  ptDst.x = world_to_real_x (rc_electrode->precompute_params.pt[3].xWorld) ;
  ptDst.y = world_to_real_y (rc_electrode->precompute_params.pt[3].yWorld) ;
  gdk_draw_line (dst, gc, ptSrc.x, ptSrc.y, ptDst.x, ptDst.y) ;
  ptSrc = ptDst ;
  ptDst.x = world_to_real_x (rc_electrode->precompute_params.pt[0].xWorld) ;
  ptDst.y = world_to_real_y (rc_electrode->precompute_params.pt[0].yWorld) ;
  gdk_draw_line (dst, gc, ptSrc.x, ptSrc.y, ptDst.x, ptDst.y) ;

  for (Nix = 0 ; Nix < rc_electrode->n_x_divisions ; Nix++)
    for (Nix1 = 0 ; Nix1 < rc_electrode->n_y_divisions ; Nix1++)
      {
      ptSrc.x = world_to_real_x (exp_array_index_2d (rc_electrode->precompute_params.pts, WorldPoint, Nix1, Nix).xWorld) ;
      ptSrc.y = world_to_real_y (exp_array_index_2d (rc_electrode->precompute_params.pts, WorldPoint, Nix1, Nix).yWorld) ;
      if (PT_IN_RECT (ptSrc.x, ptSrc.y, rcClip->x, rcClip->y, rcClip->width, rcClip->height))
        gdk_draw_point (dst, gc, ptSrc.x, ptSrc.y) ;
      }

  g_object_unref (gc) ;
  }

static gboolean button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data)
  {
  QCADDesignObject *obj = NULL ;
  double xWorld = real_to_world_x (event->x), yWorld = real_to_world_y (event->y) ;

  if (1 != event->button) return FALSE ;

#ifdef DESIGNER
  world_to_grid_pt (&xWorld, &yWorld) ;
#endif /* def DESIGNER */

  fprintf (stderr, "QCADRectangleElectrode::button_pressed:Calling qcad_rectangle_electrode_new\n") ;
  obj = qcad_rectangle_electrode_new () ;
  fprintf (stderr, "QCADRectangleElectrode::button_pressed:Calling qcad_design_object_move\n") ;
  qcad_design_object_move (obj, xWorld - obj->bounding_box.xWorld + obj->bounding_box.cxWorld / 2.0, yWorld - obj->bounding_box.yWorld + obj->bounding_box.cyWorld / 2.0) ;

#ifdef DESIGNER
  if (NULL != drop_function)
    if ((*drop_function) (obj))
      return FALSE ;
#endif /* def DESIGNER */

  g_object_unref (obj) ;

  return FALSE ;
  }
#endif /* def GTK_GUI */

static void *default_properties_get (struct QCADDesignObjectClass *klass)
  {
  QCADElectrodeClass *parent_class = QCAD_ELECTRODE_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_RECTANGLE_ELECTRODE))) ;
  QCADElectrodeOptions *pDefaults = g_malloc (sizeof (QCADRectangleElectrodeOptions)) ;
  QCADRectangleElectrodeOptions *p_rcz_Defaults = (QCADRectangleElectrodeOptions *)pDefaults ;

  memcpy (pDefaults, &(parent_class->default_electrode_options), sizeof (QCADElectrodeOptions)) ;
  p_rcz_Defaults->angle         = QCAD_RECTANGLE_ELECTRODE_CLASS (klass)->default_angle ;
  p_rcz_Defaults->n_x_divisions = QCAD_RECTANGLE_ELECTRODE_CLASS (klass)->default_n_x_divisions ;
  p_rcz_Defaults->n_y_divisions = QCAD_RECTANGLE_ELECTRODE_CLASS (klass)->default_n_y_divisions ;
  p_rcz_Defaults->cxWorld       = QCAD_RECTANGLE_ELECTRODE_CLASS (klass)->default_cxWorld ;
  p_rcz_Defaults->cyWorld       = QCAD_RECTANGLE_ELECTRODE_CLASS (klass)->default_cyWorld ;
  return (void *)pDefaults ;
  }

static void default_properties_set (struct QCADDesignObjectClass *klass, void *props)
  {
  QCADElectrodeClass *parent_class = QCAD_ELECTRODE_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_RECTANGLE_ELECTRODE))) ;

  memcpy (&(parent_class->default_electrode_options), props, sizeof (QCADElectrodeOptions)) ;
  QCAD_RECTANGLE_ELECTRODE_CLASS (klass)->default_angle = ((QCADRectangleElectrodeOptions *)props)->angle ;
  QCAD_RECTANGLE_ELECTRODE_CLASS (klass)->default_n_x_divisions = ((QCADRectangleElectrodeOptions *)props)->n_x_divisions ;
  QCAD_RECTANGLE_ELECTRODE_CLASS (klass)->default_n_y_divisions = ((QCADRectangleElectrodeOptions *)props)->n_y_divisions ;
  QCAD_RECTANGLE_ELECTRODE_CLASS (klass)->default_cxWorld = ((QCADRectangleElectrodeOptions *)props)->cxWorld ;
  QCAD_RECTANGLE_ELECTRODE_CLASS (klass)->default_cyWorld = ((QCADRectangleElectrodeOptions *)props)->cyWorld ;
  }

static void default_properties_destroy (struct QCADDesignObjectClass *klass, void *props)
  {g_free (props) ;}

static double get_potential (QCADElectrode *electrode, double x, double y, double z, double t)
  {
  QCADRectangleElectrode *rc_electrode = QCAD_RECTANGLE_ELECTRODE (electrode) ;
  int Nix, Nix1 ;
  double potential = 0, rho = 0 ;
  double cx, cy, cz_mir, cx_sq_plus_cy_sq ;

  if (z < 0 || NULL == electrode) return 0 ;

  rho = rc_electrode->precompute_params.rho_factor * qcad_electrode_get_voltage (electrode, t) ;
  
  for (Nix = 0 ; Nix < rc_electrode->n_x_divisions ; Nix++)
    for (Nix1 = 0 ; Nix1 < rc_electrode->n_y_divisions ; Nix1++)
      {
      cx = x - exp_array_index_2d (rc_electrode->precompute_params.pts, WorldPoint, Nix1, Nix).xWorld ;
      cy = y - exp_array_index_2d (rc_electrode->precompute_params.pts, WorldPoint, Nix1, Nix).yWorld ;
      cx_sq_plus_cy_sq = cx * cx + cy * cy ;

      cz_mir = electrode->precompute_params.two_z_to_ground - z ;

      potential +=
        (rho * ((1.0 / sqrt (cx_sq_plus_cy_sq +   z    *   z   )) - 
                (1.0 / sqrt (cx_sq_plus_cy_sq + cz_mir * cz_mir))) * 1e9)
        / (FOUR_PI * electrode->precompute_params.permittivity) ;
      }

  return potential ;
  }

static double get_area (QCADElectrode *electrode)
  {
  QCADRectangleElectrode *rc_electrode = QCAD_RECTANGLE_ELECTRODE (electrode) ;

  return rc_electrode->cxWorld * rc_electrode->cyWorld * 1e-18 ;
  }
///////////////////////////////////////////////////////////////////////////////

#ifdef GTK_GUI
static void create_default_properties_dialog (DEFAULT_PROPERTIES *dialog)
  {
  int Nix ;
  GtkWidget *lbl = NULL, *mnu = NULL, *mnui = NULL, *spn = NULL ;

  dialog->tbl = gtk_table_new (7, 3, FALSE) ;
  gtk_widget_show (dialog->tbl) ;
  gtk_container_set_border_width (GTK_CONTAINER (dialog->tbl), 2) ;

  lbl = gtk_label_new (_("Clock Function:")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_RIGHT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5) ;

  dialog->clock_function_option_menu = gtk_option_menu_new () ;
  gtk_widget_show (dialog->clock_function_option_menu) ;
  mnu = gtk_menu_new () ;
  gtk_widget_show (mnu) ;
  for (Nix = 0 ; Nix < n_clock_functions ; Nix++)
    {
    mnui = gtk_menu_item_new_with_label (clock_functions[Nix].pszDescription) ;
    gtk_widget_show (mnui) ;
    gtk_container_add (GTK_CONTAINER (mnu), mnui) ;
    }
  gtk_option_menu_set_menu (GTK_OPTION_MENU (dialog->clock_function_option_menu), mnu) ;
  gtk_option_menu_set_history (GTK_OPTION_MENU (dialog->clock_function_option_menu), 0) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), dialog->clock_function_option_menu, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 2, 2) ;

  lbl = gtk_label_new (_("Amplitude:")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_RIGHT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5) ;

  spn = gtk_spin_button_new_infinite (dialog->adjAmplitude = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 1, 0.0001, 0.001, 0)), 0.0001, 4, ISB_DIR_UP) ;
  gtk_widget_show (spn) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), spn, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_entry_set_activates_default (GTK_ENTRY (spn), TRUE) ;

  lbl = gtk_label_new (_("Frequency:")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_RIGHT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5) ;

  spn = gtk_spin_button_new_infinite (dialog->adjFrequency = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 1, 0.1, 1, 0)), 0.1, 4, ISB_DIR_UP) ;
  gtk_widget_show (spn) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), spn, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_entry_set_activates_default (GTK_ENTRY (spn), TRUE) ;

  lbl = gtk_label_new (_("MHz")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 2, 3, 2, 3, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_LEFT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 0.0, 0.5) ;

  lbl = gtk_label_new (_("Phase:")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_RIGHT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5) ;

  spn = gtk_spin_button_new (dialog->adjPhase = GTK_ADJUSTMENT (gtk_adjustment_new (0, -360, 360, 1, 5, 0)), 1, 0) ;
  gtk_widget_show (spn) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), spn, 1, 2, 3, 4, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_entry_set_activates_default (GTK_ENTRY (spn), TRUE) ;

  lbl = gtk_label_new (_("°")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 2, 3, 3, 4, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_LEFT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 0.0, 0.5) ;

  lbl = gtk_label_new (_("DC Offset:")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_RIGHT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5) ;

  spn = gtk_spin_button_new_infinite (dialog->adjDCOffset = GTK_ADJUSTMENT (gtk_adjustment_new (0, -1, 1, 0.1, 1, 0)), 0.1, 3, ISB_DIR_UP | ISB_DIR_DN) ;
  gtk_widget_show (spn) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), spn, 1, 2, 4, 5, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_entry_set_activates_default (GTK_ENTRY (spn), TRUE) ;

  lbl = gtk_label_new (_("V")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 2, 3, 4, 5, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_LEFT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 0.0, 0.5) ;

  lbl = gtk_label_new (_("Minimum voltage:")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 0, 1, 5, 6, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_RIGHT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5) ;

  spn = gtk_spin_button_new_infinite (dialog->adjMinClock = GTK_ADJUSTMENT (gtk_adjustment_new (0, -1, 1, 0.1, 1, 0)), 0.1, 3, ISB_DIR_UP | ISB_DIR_DN) ;
  gtk_widget_show (spn) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), spn, 1, 2, 5, 6, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_entry_set_activates_default (GTK_ENTRY (spn), TRUE) ;

  lbl = gtk_label_new (_("V")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 2, 3, 5, 6, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_LEFT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 0.0, 0.5) ;

  lbl = gtk_label_new (_("Maximum voltage:")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 0, 1, 6, 7, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_RIGHT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5) ;

  spn = gtk_spin_button_new_infinite (dialog->adjMaxClock = GTK_ADJUSTMENT (gtk_adjustment_new (0, -1, 1, 0.1, 1, 0)), 0.1, 3, ISB_DIR_UP | ISB_DIR_DN) ;
  gtk_widget_show (spn) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), spn, 1, 2, 6, 7, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_entry_set_activates_default (GTK_ENTRY (spn), TRUE) ;

  lbl = gtk_label_new (_("V")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 2, 3, 6, 7, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_LEFT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 0.0, 0.5) ;

  lbl = gtk_label_new (_("Angle:")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 0, 1, 7, 8, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_RIGHT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5) ;

  spn = gtk_spin_button_new (dialog->adjAngle = GTK_ADJUSTMENT (gtk_adjustment_new (0, -360, 360, 1, 5, 0)), 1, 0) ;
  gtk_widget_show (spn) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), spn, 1, 2, 7, 8, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_entry_set_activates_default (GTK_ENTRY (spn), TRUE) ;

  lbl = gtk_label_new (_("°")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 2, 3, 7, 8, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_LEFT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 0.0, 0.5) ;

  lbl = gtk_label_new (_("Number of x divisions:")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 0, 1, 8, 9, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_RIGHT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5) ;

  spn = gtk_spin_button_new_infinite (dialog->adjNXDivisions = GTK_ADJUSTMENT (gtk_adjustment_new (1, 1, 2, 1, 1, 0)), 1, 0, ISB_DIR_UP) ;
  gtk_widget_show (spn) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), spn, 1, 2, 8, 9, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_entry_set_activates_default (GTK_ENTRY (spn), TRUE) ;

  lbl = gtk_label_new (_("Number of y divisions:")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 0, 1, 9, 10, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_RIGHT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5) ;

  spn = gtk_spin_button_new_infinite (dialog->adjNYDivisions = GTK_ADJUSTMENT (gtk_adjustment_new (1, 1, 2, 1, 1, 0)), 1, 0, ISB_DIR_UP) ;
  gtk_widget_show (spn) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), spn, 1, 2, 9, 10, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_entry_set_activates_default (GTK_ENTRY (spn), TRUE) ;

  lbl = gtk_label_new (_("Width:")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 0, 1, 10, 11, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_RIGHT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5) ;

  spn = gtk_spin_button_new_infinite (dialog->adjCX = GTK_ADJUSTMENT (gtk_adjustment_new (1, 0, 2, 1, 10, 0)), 1, 1, ISB_DIR_UP) ;
  gtk_widget_show (spn) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), spn, 1, 2, 10, 11, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_entry_set_activates_default (GTK_ENTRY (spn), TRUE) ;

  lbl = gtk_label_new (_("nm")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 2, 3, 10, 11, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_LEFT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 0.0, 0.5) ;

  lbl = gtk_label_new (_("Height:")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 0, 1, 11, 12, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_RIGHT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 1.0, 0.5) ;

  spn = gtk_spin_button_new_infinite (dialog->adjCY = GTK_ADJUSTMENT (gtk_adjustment_new (1, 0, 2, 1, 10, 0)), 1, 1, ISB_DIR_UP) ;
  gtk_widget_show (spn) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), spn, 1, 2, 11, 12, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_entry_set_activates_default (GTK_ENTRY (spn), TRUE) ;

  lbl = gtk_label_new (_("nm")) ;
  gtk_widget_show (lbl) ;
  gtk_table_attach (GTK_TABLE (dialog->tbl), lbl, 2, 3, 11, 12, GTK_FILL, GTK_FILL, 2, 2) ;
  gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_LEFT) ;
  gtk_misc_set_alignment (GTK_MISC (lbl), 0.0, 0.5) ;
  }

static void create_properties_dialog (PROPERTIES *dialog)
  {
  dialog->dlg = gtk_dialog_new () ;
  gtk_widget_show (dialog->dlg) ;

  create_default_properties_dialog (&(dialog->contents)) ;
  gtk_widget_show (dialog->contents.tbl) ;
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dlg)->vbox), dialog->contents.tbl, TRUE, TRUE, 0) ;

  gtk_dialog_add_button (GTK_DIALOG (dialog->dlg), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL) ;
  gtk_dialog_add_button (GTK_DIALOG (dialog->dlg), GTK_STOCK_OK, GTK_RESPONSE_OK) ;
  gtk_dialog_set_default_response (GTK_DIALOG (dialog->dlg), GTK_RESPONSE_OK) ;
  }

static void default_properties_apply (gpointer data)
  {
  DEFAULT_PROPERTIES *dialog = (DEFAULT_PROPERTIES *)data ;
  QCADElectrodeClass *electrode_class = QCAD_ELECTRODE_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_RECTANGLE_ELECTRODE))) ;
  QCADRectangleElectrodeClass *rc_electrode_class = QCAD_RECTANGLE_ELECTRODE_CLASS (g_type_class_peek (QCAD_TYPE_RECTANGLE_ELECTRODE)) ;

  electrode_class->default_electrode_options.clock_function = clock_functions[gtk_option_menu_get_history (GTK_OPTION_MENU (dialog->clock_function_option_menu))].clock_function ;
  electrode_class->default_electrode_options.amplitude      = gtk_adjustment_get_value (dialog->adjAmplitude) ;
  electrode_class->default_electrode_options.frequency      = gtk_adjustment_get_value (dialog->adjFrequency) * 1000000.0 ;
  electrode_class->default_electrode_options.phase          = (gtk_adjustment_get_value (dialog->adjPhase) * PI) / 180.0 ;
  electrode_class->default_electrode_options.dc_offset      = gtk_adjustment_get_value (dialog->adjDCOffset) ;
  electrode_class->default_electrode_options.min_clock      = gtk_adjustment_get_value (dialog->adjMinClock) ;
  electrode_class->default_electrode_options.max_clock      = gtk_adjustment_get_value (dialog->adjMaxClock) ;
  rc_electrode_class->default_angle         = (gtk_adjustment_get_value (dialog->adjAngle) * PI) / 180.0 ;
  rc_electrode_class->default_n_x_divisions = (int)gtk_adjustment_get_value (dialog->adjNXDivisions) ;
  rc_electrode_class->default_n_y_divisions = (int)gtk_adjustment_get_value (dialog->adjNYDivisions) ;
  rc_electrode_class->default_cxWorld       = gtk_adjustment_get_value (dialog->adjCX) ;
  rc_electrode_class->default_cyWorld       = gtk_adjustment_get_value (dialog->adjCY) ;
  }
#endif

static void precompute (QCADElectrode *electrode)
  {
  int Nix, Nix1 ;
  WorldPoint ptSrcLine, ptDstLine ;
  double factor1, factor2, dstx_minus_srcx, dsty_minus_srcy ;
  double pt1x_minus_pt0x, pt1y_minus_pt0y, pt3x_minus_pt2x, pt3y_minus_pt2y ;
  double reciprocal_of_x_divisions, reciprocal_of_y_divisions ;
  QCADRectangleElectrode *rc_electrode = QCAD_RECTANGLE_ELECTRODE (electrode) ;
  QCADDesignObject *obj = QCAD_DESIGN_OBJECT (electrode) ;
  double 
    kose =  cos (rc_electrode->angle),
    msin = -sin (rc_electrode->angle),
    sine =  sin (rc_electrode->angle),
    half_cx = rc_electrode->cxWorld / 2.0,
    half_cy = rc_electrode->cyWorld / 2.0,
    xMin, yMin, xMax, yMax ;
  WorldPoint pt[4] = {{0,0},{0,0},{0,0},{0,0}}, ptCenter = {0,0} ;

  // Call parent precompute function
  QCAD_ELECTRODE_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_RECTANGLE_ELECTRODE)))->precompute (electrode) ;

  ptCenter.xWorld = obj->bounding_box.xWorld + obj->bounding_box.cxWorld / 2.0 ;
  ptCenter.yWorld = obj->bounding_box.yWorld + obj->bounding_box.cyWorld / 2.0 ;

  // Create corner points
  pt[0].xWorld = ptCenter.xWorld - half_cx ;
  pt[0].yWorld = ptCenter.yWorld - half_cy ;
  pt[1].xWorld = ptCenter.xWorld + half_cx ;
  pt[1].yWorld = ptCenter.yWorld - half_cy ;
  pt[2].xWorld = ptCenter.xWorld + half_cx ;
  pt[2].yWorld = ptCenter.yWorld + half_cy ;
  pt[3].xWorld = ptCenter.xWorld - half_cx ;
  pt[3].yWorld = ptCenter.yWorld + half_cy ;

  if (0 != rc_electrode->angle)
    {
    // rotate corner points
    rc_electrode->precompute_params.pt[0].xWorld = kose * pt[0].xWorld + sine * pt[0].yWorld ;
    rc_electrode->precompute_params.pt[0].yWorld = msin * pt[0].xWorld + kose * pt[0].yWorld ;
    rc_electrode->precompute_params.pt[1].xWorld = kose * pt[1].xWorld + sine * pt[1].yWorld ;
    rc_electrode->precompute_params.pt[1].yWorld = msin * pt[1].xWorld + kose * pt[1].yWorld ;
    rc_electrode->precompute_params.pt[2].xWorld = kose * pt[2].xWorld + sine * pt[2].yWorld ;
    rc_electrode->precompute_params.pt[2].yWorld = msin * pt[2].xWorld + kose * pt[2].yWorld ;
    rc_electrode->precompute_params.pt[3].xWorld = kose * pt[3].xWorld + sine * pt[3].yWorld ;
    rc_electrode->precompute_params.pt[3].yWorld = msin * pt[3].xWorld + kose * pt[3].yWorld ;

    // Find binding box
    xMin = MIN (rc_electrode->precompute_params.pt[0].xWorld, MIN (rc_electrode->precompute_params.pt[1].xWorld, MIN (rc_electrode->precompute_params.pt[2].xWorld, rc_electrode->precompute_params.pt[3].xWorld))) ;
    xMax = MAX (rc_electrode->precompute_params.pt[0].xWorld, MAX (rc_electrode->precompute_params.pt[1].xWorld, MAX (rc_electrode->precompute_params.pt[2].xWorld, rc_electrode->precompute_params.pt[3].xWorld))) ;
    yMin = MIN (rc_electrode->precompute_params.pt[0].yWorld, MIN (rc_electrode->precompute_params.pt[1].yWorld, MIN (rc_electrode->precompute_params.pt[2].yWorld, rc_electrode->precompute_params.pt[3].yWorld))) ;
    yMax = MAX (rc_electrode->precompute_params.pt[0].yWorld, MAX (rc_electrode->precompute_params.pt[1].yWorld, MAX (rc_electrode->precompute_params.pt[2].yWorld, rc_electrode->precompute_params.pt[3].yWorld))) ;

    obj->bounding_box.xWorld = xMin ;
    obj->bounding_box.yWorld = yMin ;
    obj->bounding_box.cxWorld = xMax - xMin ;
    obj->bounding_box.cyWorld = yMax - yMin ;
    }
  else
    {
    rc_electrode->precompute_params.pt[0] = pt[0] ;
    rc_electrode->precompute_params.pt[1] = pt[1] ;
    rc_electrode->precompute_params.pt[2] = pt[2] ;
    rc_electrode->precompute_params.pt[3] = pt[3] ;
    obj->bounding_box.xWorld = pt[0].xWorld ;
    obj->bounding_box.yWorld = pt[0].yWorld ;
    obj->bounding_box.cxWorld = rc_electrode->cxWorld ;
    obj->bounding_box.cyWorld = rc_electrode->cyWorld ;
    }

  obj->x = obj->bounding_box.xWorld + obj->bounding_box.cxWorld / 2.0 ;
  obj->y = obj->bounding_box.yWorld + obj->bounding_box.cyWorld / 2.0 ;

  if (NULL != rc_electrode->precompute_params.pts)
    exp_array_free (rc_electrode->precompute_params.pts) ;

  rc_electrode->precompute_params.pts = exp_array_new (sizeof (WorldPoint), 2) ;

  for (Nix = 0 ; Nix < rc_electrode->n_y_divisions ; Nix++)
    exp_array_insert_vals (rc_electrode->precompute_params.pts, NULL, rc_electrode->n_x_divisions, 2, -1, 0) ;

  rc_electrode->precompute_params.rho_factor = QCAD_ELECTRODE (rc_electrode)->precompute_params.capacitance / (rc_electrode->n_x_divisions * rc_electrode->n_y_divisions) ;
  pt1x_minus_pt0x = rc_electrode->precompute_params.pt[1].xWorld - rc_electrode->precompute_params.pt[0].xWorld ;
  pt1y_minus_pt0y = rc_electrode->precompute_params.pt[1].yWorld - rc_electrode->precompute_params.pt[0].yWorld ;
  pt3x_minus_pt2x = rc_electrode->precompute_params.pt[3].xWorld - rc_electrode->precompute_params.pt[2].xWorld ;
  pt3y_minus_pt2y = rc_electrode->precompute_params.pt[3].yWorld - rc_electrode->precompute_params.pt[2].yWorld ;
  reciprocal_of_x_divisions = 1.0 / rc_electrode->n_x_divisions ;
  reciprocal_of_y_divisions = 1.0 / rc_electrode->n_y_divisions ;
  
  for (Nix = 0 ; Nix < rc_electrode->n_x_divisions ; Nix++)
    {
    factor1 = reciprocal_of_x_divisions * (Nix + 0.5) ;
    factor2 = reciprocal_of_x_divisions * ((rc_electrode->n_x_divisions - Nix) - 0.5) ;

    ptSrcLine.xWorld = rc_electrode->precompute_params.pt[0].xWorld + pt1x_minus_pt0x * factor1 ;
    ptSrcLine.yWorld = rc_electrode->precompute_params.pt[0].yWorld + pt1y_minus_pt0y * factor1 ;
    ptDstLine.xWorld = rc_electrode->precompute_params.pt[2].xWorld + pt3x_minus_pt2x * factor2 ;
    ptDstLine.yWorld = rc_electrode->precompute_params.pt[2].yWorld + pt3y_minus_pt2y * factor2 ;

    dstx_minus_srcx = ptDstLine.xWorld - ptSrcLine.xWorld ;
    dsty_minus_srcy = ptDstLine.yWorld - ptSrcLine.yWorld ;
    for (Nix1 = 0 ; Nix1 < rc_electrode->n_y_divisions ; Nix1++)
      {
      exp_array_index_2d (rc_electrode->precompute_params.pts, WorldPoint, Nix1, Nix).xWorld = 
        ptSrcLine.xWorld + dstx_minus_srcx * reciprocal_of_y_divisions * (Nix1 + 0.5) ;
      exp_array_index_2d (rc_electrode->precompute_params.pts, WorldPoint, Nix1, Nix).yWorld = 
        ptSrcLine.yWorld + dsty_minus_srcy * reciprocal_of_y_divisions * (Nix1 + 0.5) ;
      fprintf (stderr, "QCADRectangleElectrode::precompute:(%lf,%lf)\n",
        exp_array_index_2d (rc_electrode->precompute_params.pts, WorldPoint, Nix1, Nix).xWorld,
        exp_array_index_2d (rc_electrode->precompute_params.pts, WorldPoint, Nix1, Nix).yWorld) ;
      }
    }
  }
