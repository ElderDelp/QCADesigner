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
//                                                      //
//////////////////////////////////////////////////////////

#include <gtk/gtk.h>
#include "../support.h"
#include "../bus_layout_dialog.h"
#include "QCADCellRendererVT.h"

// Copied from gtk+-2.2.4/gtk/gtkcellrenderertoggle.c
#define TOGGLE_WIDTH 12

enum
  {
  QCADCRVT_PROP_ACTIVE = 1,
  QCADCRVT_PROP_ROW_TYPE
  } ;

//static guint qcad_cell_renderer_vt_signals[QCADCRVT_LAST_SIGNAL] = {0} ;

static void qcad_cell_renderer_vt_class_init (QCADCellRendererVTClass *klass) ;
static void qcad_cell_renderer_vt_instance_init (QCADCellRendererVT *qcadcrvt) ;
static void qcad_cell_renderer_vt_get_property (GObject *object, guint param_id, GValue *value, GParamSpec *pspec) ;
static void qcad_cell_renderer_vt_set_property (GObject *object, guint param_id, const GValue *value, GParamSpec *pspec) ;
static void qcad_cell_renderer_vt_render (GtkCellRenderer *cr, GdkWindow *window, GtkWidget *widget, GdkRectangle *background_area, GdkRectangle *cell_area, GdkRectangle *expose_area, GtkCellRendererState flags) ;
static void qcad_cell_renderer_vt_get_size (GtkCellRenderer *cr, GtkWidget *widget, GdkRectangle *cell_area, gint *x_offset, gint *y_offset, gint *width, gint *height) ;
static gboolean qcad_cell_renderer_vt_activate (GtkCellRenderer *cell, GdkEvent *event, GtkWidget *widget, const gchar *path, GdkRectangle *background_area, GdkRectangle *cell_area, GtkCellRendererState flags);
static GtkCellEditable *qcad_cell_renderer_vt_start_editing (GtkCellRenderer *cell, GdkEvent *event, GtkWidget *widget, const gchar *path, GdkRectangle *background_area, GdkRectangle *cell_area, GtkCellRendererState flags) ;

GType qcad_cell_renderer_vt_get_type ()
  {
  static GType qcadcrvt_type = 0 ;

  if (0 == qcadcrvt_type)
    {
    static GTypeInfo info = 
      {
      sizeof (QCADCellRendererVTClass),
      NULL,
      NULL,
      (GClassInitFunc)qcad_cell_renderer_vt_class_init,
      NULL,
      NULL,
      sizeof (QCADCellRendererVT),
      0,
      (GInstanceInitFunc)qcad_cell_renderer_vt_instance_init
      } ;
    if ((qcadcrvt_type = g_type_register_static (GTK_TYPE_CELL_RENDERER_TEXT, QCAD_TYPE_STRING_CELL_RENDERER_VT, &info, 0)))
      g_type_class_ref (qcadcrvt_type) ;
    }
  return qcadcrvt_type ;
  }

static void qcad_cell_renderer_vt_class_init (QCADCellRendererVTClass *klass)
  {
  GObjectClass *object_klass = G_OBJECT_CLASS (klass) ;
  GtkCellRendererClass *cr_klass = GTK_CELL_RENDERER_CLASS (klass) ;

  object_klass->get_property = qcad_cell_renderer_vt_get_property ;
  object_klass->set_property = qcad_cell_renderer_vt_set_property ;

  cr_klass->get_size = qcad_cell_renderer_vt_get_size ;
  cr_klass->render = qcad_cell_renderer_vt_render ;
  cr_klass->start_editing = qcad_cell_renderer_vt_start_editing ;
  cr_klass->activate = qcad_cell_renderer_vt_activate;

  g_object_class_install_property (object_klass, QCADCRVT_PROP_ACTIVE,
    g_param_spec_boolean ("active", _("Toggle state"), _("The toggle state of the button"),
    FALSE, G_PARAM_READABLE | G_PARAM_WRITABLE)) ;

  g_object_class_install_property (object_klass, QCADCRVT_PROP_ROW_TYPE,
    g_param_spec_int ("row-type", _("Row Type"), _("Row type:Cell/Bus"),
      1, (1 << 5) - 1, ROW_TYPE_CELL_INPUT, G_PARAM_READABLE | G_PARAM_WRITABLE)) ;
  }

static void qcad_cell_renderer_vt_instance_init (QCADCellRendererVT *qcadcrvt)
  {
  qcadcrvt->value = 0 ;
  GTK_CELL_RENDERER (qcadcrvt)->mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;
  GTK_CELL_RENDERER (qcadcrvt)->xpad = 2;
  GTK_CELL_RENDERER (qcadcrvt)->ypad = 2;
  }

static void qcad_cell_renderer_vt_get_property (GObject *object, guint param_id, GValue *value, GParamSpec *pspec)
  {
  QCADCellRendererVT *qcadcrvt = QCAD_CELL_RENDERER_VT (object) ;

  switch (param_id)
    {
    case QCADCRVT_PROP_ACTIVE:
      g_value_set_boolean (value, (0 == qcadcrvt->value)) ;
      break ;

    case QCADCRVT_PROP_ROW_TYPE:
      g_value_set_int (value, qcadcrvt->row_type) ;
      break ;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
  }

static void qcad_cell_renderer_vt_set_property (GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
  {
  QCADCellRendererVT *qcadcrvt = QCAD_CELL_RENDERER_VT (object) ;

  switch (param_id)
    {
    case QCADCRVT_PROP_ACTIVE:
      if (qcadcrvt->row_type & ROW_TYPE_CELL)
        {
        qcadcrvt->value = (g_value_get_boolean (value) ? 1 : 0) ;
        g_object_notify (object, "active") ;
        }
      break ;

    case QCADCRVT_PROP_ROW_TYPE:
      qcadcrvt->row_type = g_value_get_int (value) ;
      GTK_CELL_RENDERER (qcadcrvt)->mode = (qcadcrvt->row_type & ROW_TYPE_CELL)
        ? GTK_CELL_RENDERER_MODE_ACTIVATABLE 
        : GTK_CELL_RENDERER_MODE_EDITABLE ;
      g_object_notify (object, "row-type") ;
      break ;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
  }

GtkCellRenderer *qcad_cell_renderer_vt_new ()
  {return g_object_new (QCAD_TYPE_CELL_RENDERER_VT, NULL) ;}

static void qcad_cell_renderer_vt_get_size (GtkCellRenderer *cr, GtkWidget *widget, GdkRectangle *cell_area, gint *x_offset, gint *y_offset, gint *width, gint *height)
  {
  if (QCAD_CELL_RENDERER_VT (cr)->row_type & ROW_TYPE_BUS)
    (GTK_CELL_RENDERER_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_CELL_RENDERER_VT))))->get_size (cr, widget, cell_area, x_offset, y_offset, width, height) ;
  else
    {
    // Copied straight out of gtk+-2.2.4/gtk/gtkcellrenderer.c
    gint calc_width;
    gint calc_height;

    calc_width = (gint) cr->xpad * 2 + TOGGLE_WIDTH;
    calc_height = (gint) cr->ypad * 2 + TOGGLE_WIDTH;

    if (width)
      *width = calc_width;

    if (height)
      *height = calc_height;

    if (cell_area)
      {
      if (x_offset)
        {
        *x_offset = cr->xalign * (cell_area->width - calc_width);
        *x_offset = MAX (*x_offset, 0);
        }
      if (y_offset)
        {
        *y_offset = cr->yalign * (cell_area->height - calc_height);
        *y_offset = MAX (*y_offset, 0);
        }
      }
    }
  }

static void qcad_cell_renderer_vt_render (GtkCellRenderer *cr, GdkWindow *window, GtkWidget *widget, GdkRectangle *background_area, GdkRectangle *cell_area, GdkRectangle *expose_area, GtkCellRendererState flags)
  {
  if (QCAD_CELL_RENDERER_VT (cr)->row_type & ROW_TYPE_BUS)
    (GTK_CELL_RENDERER_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_CELL_RENDERER_VT))))->render (cr, window, widget, background_area, cell_area, expose_area, flags) ;
  else
    {
    // Copied and simplified from gtk+-2.2.4/gtk/gtkcellrenderertoggle.c
    QCADCellRendererVT *qcadcrvt = (QCADCellRendererVT *)cr;
    gint width, height;
    gint x_offset, y_offset;
    GtkShadowType shadow;
    GtkStateType state = 0;

    qcad_cell_renderer_vt_get_size (cr, widget, cell_area, &x_offset, &y_offset, &width, &height);
    width -= cr->xpad*2;
    height -= cr->ypad*2;

    if (width <= 0 || height <= 0)
      return;

    shadow = (1 == qcadcrvt->value) ? GTK_SHADOW_IN : GTK_SHADOW_OUT;

    if ((flags & GTK_CELL_RENDERER_SELECTED) == GTK_CELL_RENDERER_SELECTED)
      {
      if (GTK_WIDGET_HAS_FOCUS (widget))
        state = GTK_STATE_SELECTED;
      else
        state = GTK_STATE_ACTIVE;
      }
    else
      state = GTK_STATE_NORMAL;

    gtk_paint_check (widget->style,
    window,
    state, shadow,
    cell_area, widget, "cellcheck",
    cell_area->x + x_offset + cr->xpad,
    cell_area->y + y_offset + cr->ypad,
    width - 1, height - 1);
    }
  }

static GtkCellEditable *qcad_cell_renderer_vt_start_editing (GtkCellRenderer *cell, GdkEvent *event, GtkWidget *widget, const gchar *path, GdkRectangle *background_area, GdkRectangle *cell_area, GtkCellRendererState flags)
  {
  if (QCAD_CELL_RENDERER_VT (cell)->row_type & ROW_TYPE_BUS)
    return (GTK_CELL_RENDERER_CLASS (g_type_class_peek (g_type_parent (QCAD_TYPE_CELL_RENDERER_VT))))->start_editing (cell, event, widget, path, background_area, cell_area, flags) ;
  else
    return NULL ;
  }

static gboolean qcad_cell_renderer_vt_activate (GtkCellRenderer *cell, GdkEvent *event, GtkWidget *widget, const gchar *path, GdkRectangle *background_area, GdkRectangle *cell_area, GtkCellRendererState flags)
  {
  char *psz = NULL ;
  if (QCAD_CELL_RENDERER_VT (cell)->row_type & ROW_TYPE_CELL)
    {
//    QCAD_CELL_RENDERER_VT (cell)->value = (0 == QCAD_CELL_RENDERER_VT (cell)->value) ? 1 : 0 ;
    g_signal_emit_by_name (cell, "edited", path, psz = g_strdup_printf ("%llu", QCAD_CELL_RENDERER_VT (cell)->value)) ;
    //g_signal_emit (cell, qcad_cell_renderer_vt_signals[QCADCRVT_TOGGLED_SIGNAL], 0, path) ;
    return TRUE ;
    }
  return FALSE ;
  }
