#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "support.h"
#include "blocking_dialog.h"
#include "message_box.h"
#include "file_selection_window.h"
#include "new_vector_table_options_dialog.h"

#define DBG_NVTO(s)

typedef struct
  {
  GtkWidget *dlgVectorTable;
  GtkWidget *dialog_vbox1;
  GtkWidget *tblVTMain;
  GtkWidget *fmVTOps;
  GtkWidget *tblOps;
  GtkWidget *lblMsg;
  GtkWidget *hbVTOps;
  GtkWidget *btnLoad;
  GtkWidget *btnSave;
  GtkWidget *btnSaveAs;
  GtkWidget *swndVT;
  GtkWidget *viewport1;
  GtkWidget *ebVT;
  GtkWidget *tblVT;
  GtkWidget **plblIdx;
  GtkWidget **pebIdx;
  GtkWidget **pfmInput;
  GtkWidget **plblInput;
  GtkWidget **pebInput;
  GtkWidget ***pptbBit;
  GtkWidget ***pptblblBit;
  GtkWidget *hbVTFile ;
  GtkWidget *lblVTFileMsg ;
  GtkWidget *lblVTFile;
  GtkWidget *dialog_action_area1;
  GtkWidget *hbuttonbox1;
  GtkWidget *btnOK;
  GtkWidget *btnCancel;
  GtkWidget *mnuVT ;
  GtkWidget *mnuDeact ;
  GtkWidget *mnuAct ;
  GtkWidget *mnuInsBefore ;
  GtkWidget *mnuInsAfter ;
  GtkWidget *mnuDel ;
  GtkWidget *mnuAdd ;
  int icInputs ;
  int icInputsUsed ;
  int icVectors ;
  int icVectorsUsed ;
  } new_vector_table_options_D ;

static new_vector_table_options_D nvto = {NULL} ;

void create_new_vector_table_options_dialog (new_vector_table_options_D *pnvto) ;
gboolean Input_buttondown (GtkWidget *widget, GdkEventButton *ev, gpointer user_data) ;
gboolean Vector_buttondown (GtkWidget *widget, GdkEventButton *ev, gpointer user_data) ;
gboolean VT_buttondown (GtkWidget *widget, GdkEventButton *ev, gpointer user_data) ;
gboolean Bit_buttondown (GtkWidget *widget, GdkEventButton *ev, gpointer user_data) ;
void set_column_active (GtkWidget *widget, gpointer user_data) ;
void create_vector (GtkWidget *widget, gpointer user_data) ;
void delete_vector (GtkWidget *widget, gpointer user_data) ;
void click_bit_button (GtkWidget *widget, gpointer user_data) ;
void load_vector_table (GtkWidget *widget, gpointer user_data) ;
void save_vector_table (GtkWidget *widget, gpointer user_data) ;
void vector_table_options_dialog_btnOK_clicked (GtkWidget *widget, gpointer user_data) ;

void VectorTableToDialog (new_vector_table_options_D *dialog, VectorTable *pvt) ;
void DialogToVectorTable (new_vector_table_options_D *dialog, VectorTable *pvt) ;
void SetColumnActive (new_vector_table_options_D *dialog, int idx, gboolean bActive) ;
void CreateIdxLabel (new_vector_table_options_D *dialog, int idx) ;
void CreateVectorToggle (new_vector_table_options_D *dialog, gboolean bValue, int idxRow, int idxCol) ;
void CreateInputHeading (new_vector_table_options_D *dialog, VectorTable *pvt, int idx) ;
void DestroyIdxLabel (new_vector_table_options_D *dialog, int idx) ;
void DestroyVectorToggle (new_vector_table_options_D *dialog, int idxRow, int idxCol) ;
void DestroyInputHeading (new_vector_table_options_D *dialog, int idx) ;
void CreateVector (new_vector_table_options_D *dialog, int idx) ;
void DeleteVector (new_vector_table_options_D *dialog, int idx) ;
void SetCurrentFileName (new_vector_table_options_D *dialog, char *pszFName) ;
int CountActiveInputs (new_vector_table_options_D *dialog) ;

void get_vector_table_options_from_user (GtkWindow *parent, VectorTable *pvt)
  {
  VectorTable *pvtDlg = VectorTable_copy (pvt) ;
  char szCurrentFName[PATH_LENGTH] = "" ;
  
  /* We want an inputs-only copy of the vector table passed to us.  It is this pvtDlg that
     we shall load new vector tables from files into.  Thus, we delete all the vectors from
     pvtDlg. */
  while (pvtDlg->num_of_vectors > 0)
    VectorTable_del_vector (pvtDlg, pvtDlg->num_of_vectors - 1) ;
  
  if (NULL == nvto.dlgVectorTable)
    create_new_vector_table_options_dialog (&nvto) ;
  
  gtk_object_set_data (GTK_OBJECT (nvto.dlgVectorTable), "dialog", &nvto) ;
  gtk_object_set_data (GTK_OBJECT (nvto.dlgVectorTable), "pvt", pvt) ;
  gtk_object_set_data (GTK_OBJECT (nvto.dlgVectorTable), "idxInput", (gpointer)-1) ;
  gtk_object_set_data (GTK_OBJECT (nvto.dlgVectorTable), "idxVector", (gpointer)-1) ;
  gtk_object_set_data (GTK_OBJECT (nvto.dlgVectorTable), "pvtDlg", pvtDlg) ;
  gtk_object_set_data (GTK_OBJECT (nvto.dlgVectorTable), "szCurrentFName", szCurrentFName) ;
  
  gtk_window_set_transient_for (GTK_WINDOW (nvto.dlgVectorTable), parent) ;
  
  VectorTableToDialog (&nvto, pvt) ;
    
  /* At long last, pop the box */
  show_dialog_blocking (nvto.dlgVectorTable) ;
  
  DBG_NVTO (fprintf (stderr, "After having popped the dialog, there are %d vectors used and %d inputs used\n", nvto.icVectorsUsed, nvto.icInputsUsed)) ;
  
  VectorTable_clear (pvtDlg) ;
  }

void VectorTableToDialog (new_vector_table_options_D *dialog, VectorTable *pvt)
  {
  int Nix, Nix1 ;
  DBG_NVTO (fprintf (stderr, "Filling dialog with the following vector table:\n")) ;
  DBG_NVTO (VectorTable_dump (pvt, stderr)) ;
  DBG_NVTO (fprintf (stderr, "Making sure there is enough memory for all the widgets\n")) ;
  
  if (pvt->num_of_inputs > dialog->icInputs)
    {
    DBG_NVTO (fprintf (stderr, "I need %d inputs, but I only have %d\n", pvt->num_of_inputs, dialog->icInputs)) ;
    dialog->pfmInput = realloc (dialog->pfmInput, pvt->num_of_inputs * sizeof (GtkWidget *)) ;
    dialog->plblInput = realloc (dialog->plblInput, pvt->num_of_inputs * sizeof (GtkWidget *)) ;
    dialog->pebInput = realloc (dialog->pebInput, pvt->num_of_inputs * sizeof (GtkWidget *)) ;
    for (Nix = 0 ; Nix < dialog->icVectors ; Nix++)
      {
      dialog->pptbBit[Nix] = realloc (dialog->pptbBit, pvt->num_of_inputs * sizeof (GtkWidget *)) ;
      dialog->pptblblBit[Nix] = realloc (dialog->pptblblBit, pvt->num_of_inputs * sizeof (GtkWidget *)) ;
      }
    dialog->icInputs = pvt->num_of_inputs ;
    }
  
  if (pvt->num_of_vectors > dialog->icVectors)
    {
    DBG_NVTO (fprintf (stderr, "I need %d vectors, but I only have %d\n", pvt->num_of_vectors, dialog->icVectors)) ;
    dialog->plblIdx = realloc (dialog->plblIdx, pvt->num_of_vectors * sizeof (GtkWidget *)) ;
    dialog->pebIdx = realloc (dialog->pebIdx, pvt->num_of_vectors * sizeof (GtkWidget *)) ;
    dialog->pptbBit = realloc (dialog->pptbBit, pvt->num_of_vectors * sizeof (GtkWidget **)) ;
    dialog->pptblblBit = realloc (dialog->pptblblBit, pvt->num_of_vectors * sizeof (GtkWidget **)) ;
    for (Nix = dialog->icVectors ; Nix < pvt->num_of_vectors ; Nix++)
      {
      dialog->pptbBit[Nix] = malloc (pvt->num_of_inputs * sizeof (GtkWidget *)) ;
      dialog->pptblblBit[Nix] = malloc (pvt->num_of_inputs * sizeof (GtkWidget *)) ;
      }
    dialog->icVectors = pvt->num_of_vectors ;
    }
  
  DBG_NVTO (fprintf (stderr, "Destroying excess widgets. %d vectors and %d inputs are used.\n", dialog->icVectorsUsed, dialog->icInputsUsed)) ;
  
  if (dialog->icInputsUsed > pvt->num_of_inputs)
    {
    DBG_NVTO (fprintf (stderr, "There are %d inputs used, but only %d inputs in the pvt\n", dialog->icInputsUsed, pvt->num_of_inputs)) ;
    for (Nix = pvt->num_of_inputs ; Nix < dialog->icInputsUsed ; Nix++)
      {
      DestroyInputHeading (&nvto, Nix) ;
      for (Nix1 = 0 ; Nix1 < dialog->icVectorsUsed ; Nix1++)
	DestroyVectorToggle (&nvto, Nix1, Nix) ;
      }
    dialog->icInputsUsed = pvt->num_of_inputs ;
    }
    
  if (dialog->icVectorsUsed > pvt->num_of_vectors)
    {
    DBG_NVTO (fprintf (stderr, "There are %d vectors used, but only %d vectors in the pvt\n", dialog->icVectorsUsed, pvt->num_of_vectors)) ;
    for (Nix = pvt->num_of_vectors ; Nix < dialog->icVectorsUsed ; Nix++)
      {
      DestroyIdxLabel (&nvto, Nix) ;
      for (Nix1 = 0 ; Nix1 < dialog->icInputsUsed ; Nix1++)
	DestroyVectorToggle (&nvto, Nix, Nix1) ;
      }
    dialog->icVectorsUsed = pvt->num_of_vectors ;
    }
  
  /* create new widgets if this time around we have more than last time around.  Reuse existing
     widgets first. */
  
  DBG_NVTO (fprintf (stderr, "Reusing existing widgets horizontally (0 -> %d)\n", dialog->icInputsUsed - 1)) ;
  
  for (Nix = 0 ; Nix < dialog->icInputsUsed ; Nix++)
    {
    gtk_label_set_text (GTK_LABEL (dialog->plblInput[Nix]), pvt->inputs[Nix]->label) ;
    for (Nix1 = 0 ; Nix1 < dialog->icVectorsUsed ; Nix1++)
      {
      gtk_label_set_text (GTK_LABEL (dialog->pptblblBit[Nix1][Nix]), pvt->vectors[Nix1][Nix] ? "1" : "0") ;
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->pptbBit[Nix1][Nix]), pvt->vectors[Nix1][Nix]) ;
      }
    }
  
  DBG_NVTO (fprintf (stderr, "Creating new widgets horizontally (%d -> %d)\n", Nix, pvt->num_of_inputs - 1)) ;
  
  for (; Nix < pvt->num_of_inputs ; Nix++)
    {
    CreateInputHeading (&nvto, pvt, Nix) ;
    for (Nix1 = 0 ; Nix1 < dialog->icVectorsUsed ; Nix1++)
      CreateVectorToggle (&nvto, pvt->vectors[Nix1][Nix], Nix1, Nix) ;
    }
  
  dialog->icInputsUsed = pvt->num_of_inputs ;
  
  DBG_NVTO (fprintf (stderr, "Creating new widgets vertically (%d -> %d)\n", dialog->icVectorsUsed, pvt->num_of_vectors - 1)) ;
  DBG_NVTO (fprintf (stderr, "dialog->icVectors = %d\ndialog->icInputs = %d\n", dialog->icVectors, dialog->icInputs)) ;
  
  for (Nix = dialog->icVectorsUsed ; Nix < pvt->num_of_vectors ; Nix++)
    {
    CreateIdxLabel (&nvto, Nix) ;
    for (Nix1 = 0 ; Nix1 < dialog->icInputsUsed ; Nix1++)
      CreateVectorToggle (&nvto, pvt->vectors[Nix][Nix1], Nix, Nix1) ;
    }
  
  dialog->icVectorsUsed = pvt->num_of_vectors ;
  
  gtk_table_resize (GTK_TABLE (dialog->tblVT), dialog->icInputsUsed + 1, dialog->icVectorsUsed + 1) ;
  
  /* First, make sure /everything/ is visible */
  
  DBG_NVTO (fprintf (stderr, "Disabling deactivated columns\n")) ;
  
  for (Nix = 0 ; Nix < pvt->num_of_inputs ; Nix++)
    SetColumnActive (&nvto, Nix, pvt->active_flag[Nix]) ;

  SetCurrentFileName (dialog, pvt->szFName) ;
  }

void DialogToVectorTable (new_vector_table_options_D *dialog, VectorTable *pvt)
  {
  int idx = -1, Nix, Nix1 ;
  
  g_snprintf (pvt->szFName, PATH_LENGTH, "%s", (char *)gtk_object_get_data (GTK_OBJECT (dialog->dlgVectorTable), "szCurrentFName")) ;
  
  while (pvt->num_of_vectors > 0)
    VectorTable_del_vector (pvt, pvt->num_of_vectors - 1) ;
  
  for (Nix = 0 ; Nix < dialog->icVectorsUsed ; Nix++)
    {
    idx = VectorTable_add_vector (pvt, Nix) ;
    for (Nix1 = 0 ; Nix1 < MIN (dialog->icInputsUsed, pvt->num_of_inputs) ; Nix1++)
      pvt->vectors[idx][Nix1] = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->pptbBit[Nix][Nix1])) ;
    }
  
  for (Nix = 0 ; Nix < MIN (dialog->icInputsUsed, pvt->num_of_inputs) ; Nix++)
    pvt->active_flag[Nix] = (gboolean)gtk_object_get_data (GTK_OBJECT (dialog->pebInput[Nix]), "bActive") ;
  }

void SetColumnActive (new_vector_table_options_D *dialog, int idx, gboolean bActive)
  {
  int Nix ;
  
  gtk_object_set_data (GTK_OBJECT (dialog->pebInput[idx]), "bActive", (gpointer)bActive) ;
  gtk_widget_set_sensitive (dialog->plblInput[idx], bActive) ;
  
  for (Nix = 0 ; Nix < dialog->icVectorsUsed ; Nix++)
    {
    gtk_widget_set_sensitive (dialog->pptbBit[Nix][idx], bActive) ;
    gtk_widget_set_sensitive (dialog->pptblblBit[Nix][idx], bActive) ;
    }
  }

void CreateIdxLabel (new_vector_table_options_D *dialog, int idx)
  {
  char sz[16] = "" ;
  
  dialog->pebIdx[idx] = gtk_event_box_new () ;
  gtk_widget_ref (dialog->pebIdx[idx]) ;
  g_snprintf (sz, 16, "0x%08X", (int)dialog->pebIdx[idx]) ;
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), sz, dialog->pebIdx[idx],
      	      	      	    (GtkDestroyNotify) gtk_widget_unref) ;
  gtk_widget_show (dialog->pebIdx[idx]) ;
  gtk_table_attach (GTK_TABLE (dialog->tblVT), dialog->pebIdx[idx], 0, 1, idx + 1, idx + 2,
      (GtkAttachOptions)(GTK_FILL),
      (GtkAttachOptions)(GTK_FILL), 2, 2) ;
  gtk_object_set_data (GTK_OBJECT (dialog->pebIdx[idx]), "idx", (gpointer)idx) ;

  g_snprintf (sz, 16, "%d", idx) ;
  dialog->plblIdx[idx] = gtk_label_new (sz) ;
  gtk_widget_ref (dialog->plblIdx[idx]) ;
  g_snprintf (sz, 16, "0x%08X", (int)dialog->plblIdx[idx]) ;
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), sz, dialog->plblIdx[idx],
      	      	      	    (GtkDestroyNotify) gtk_widget_unref) ;
  gtk_widget_show (dialog->plblIdx[idx]) ;
  gtk_container_add (GTK_CONTAINER (dialog->pebIdx[idx]), dialog->plblIdx[idx]) ;
  gtk_label_set_justify (GTK_LABEL (dialog->plblIdx[idx]), GTK_JUSTIFY_RIGHT) ;
  gtk_misc_set_alignment (GTK_MISC (dialog->plblIdx[idx]), 2, 0.5) ;
  
  gtk_signal_connect (GTK_OBJECT (dialog->pebIdx[idx]), "button_press_event", GTK_SIGNAL_FUNC (Vector_buttondown), dialog->dlgVectorTable) ;
  }

void DestroyIdxLabel (new_vector_table_options_D *dialog, int idx)
  {
  char sz[16] = "" ;
  gtk_container_remove (GTK_CONTAINER (dialog->pebIdx[idx]), dialog->plblIdx[idx]) ;
  g_snprintf (sz, 16, "0x%08X", (int)dialog->plblIdx[idx]) ;
  gtk_object_remove_data (GTK_OBJECT (dialog->dlgVectorTable), sz) ;
//  gtk_widget_destroy (dialog->plblIdx[idx]) ;
  gtk_signal_disconnect_by_func (GTK_OBJECT (dialog->pebIdx[idx]), GTK_SIGNAL_FUNC (Vector_buttondown), dialog->dlgVectorTable) ;
  gtk_container_remove (GTK_CONTAINER (dialog->tblVT), dialog->pebIdx[idx]) ;
  g_snprintf (sz, 16, "0x%08X", (int)dialog->pebIdx[idx]) ;
  gtk_object_remove_data (GTK_OBJECT (dialog->dlgVectorTable), sz) ;
//  gtk_widget_destroy (dialog->pebIdx[idx]) ;
  }

void CreateVectorToggle (new_vector_table_options_D *dialog, gboolean bValue, int idxRow, int idxCol)
  {
  char sz[16] = "" ;
  dialog->pptbBit[idxRow][idxCol] = gtk_toggle_button_new () ;
  gtk_widget_ref (dialog->pptbBit[idxRow][idxCol]) ;
  g_snprintf (sz, 16, "0x%08X", (int)dialog->pptbBit[idxRow][idxCol]) ;
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), sz, dialog->pptbBit[idxRow][idxCol],
      	      	      	    (GtkDestroyNotify) gtk_widget_unref) ;
  gtk_widget_show (dialog->pptbBit[idxRow][idxCol]) ;
  gtk_table_attach (GTK_TABLE (dialog->tblVT), dialog->pptbBit[idxRow][idxCol],
    idxCol + 1, idxCol + 2, idxRow + 1, idxRow + 2,
    (GtkAttachOptions)(GTK_FILL),
    (GtkAttachOptions)(GTK_FILL), 0, 0) ;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->pptbBit[idxRow][idxCol]), bValue) ;
  gtk_object_set_data (GTK_OBJECT (dialog->pptbBit[idxRow][idxCol]), "idxRow", (gpointer)idxRow) ;
  gtk_object_set_data (GTK_OBJECT (dialog->pptbBit[idxRow][idxCol]), "idxCol", (gpointer)idxCol) ;
  GTK_WIDGET_UNSET_FLAGS (dialog->pptbBit[idxRow][idxCol], GTK_CAN_FOCUS | GTK_CAN_DEFAULT) ;

  dialog->pptblblBit[idxRow][idxCol] = gtk_label_new (bValue ? "1" : "0") ;
  gtk_widget_ref (dialog->pptblblBit[idxRow][idxCol]) ;
  g_snprintf (sz, 16, "0x%08X", (int)dialog->pptblblBit[idxRow][idxCol]) ;
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), sz, dialog->pptblblBit[idxRow][idxCol],
      	      	      	    (GtkDestroyNotify) gtk_widget_unref) ;
  gtk_widget_show (dialog->pptblblBit[idxRow][idxCol]) ;
  gtk_container_add (GTK_CONTAINER (dialog->pptbBit[idxRow][idxCol]), dialog->pptblblBit[idxRow][idxCol]) ;
  
  gtk_signal_connect (GTK_OBJECT (dialog->pptbBit[idxRow][idxCol]), "clicked", GTK_SIGNAL_FUNC (click_bit_button), dialog->pptblblBit[idxRow][idxCol]) ;
  gtk_signal_connect (GTK_OBJECT (dialog->pptbBit[idxRow][idxCol]), "button_press_event", GTK_SIGNAL_FUNC (Bit_buttondown), dialog->dlgVectorTable) ;
  }

void DestroyVectorToggle (new_vector_table_options_D *dialog, int idxRow, int idxCol)
  {
  char sz[16] = "" ;
  gtk_signal_disconnect_by_func (GTK_OBJECT(dialog->pptbBit[idxRow][idxCol]), GTK_SIGNAL_FUNC (click_bit_button), dialog->pptblblBit[idxRow][idxCol]) ;
  gtk_container_remove (GTK_CONTAINER (dialog->pptbBit[idxRow][idxCol]), dialog->pptblblBit[idxRow][idxCol]) ;
  g_snprintf (sz, 16, "0x%08X", (int)dialog->pptblblBit[idxRow][idxCol]) ;
  gtk_object_remove_data (GTK_OBJECT (dialog->dlgVectorTable), sz) ;
//  gtk_widget_destroy (dialog->pptblblBit[idxRow][idxCol]) ;
  gtk_signal_disconnect_by_func (GTK_OBJECT (dialog->pptbBit[idxRow][idxCol]), GTK_SIGNAL_FUNC (Bit_buttondown), dialog->dlgVectorTable) ;
  gtk_container_remove (GTK_CONTAINER (dialog->tblVT), dialog->pptbBit[idxRow][idxCol]) ;
  g_snprintf (sz, 16, "0x%08X", (int)dialog->pptbBit[idxRow][idxCol]) ;
  gtk_object_remove_data (GTK_OBJECT (dialog->dlgVectorTable), sz) ;
//  gtk_widget_destroy (dialog->pptbBit[idxRow][idxCol]) ;
  }

void CreateInputHeading (new_vector_table_options_D *dialog, VectorTable *pvt, int idx)
  {
  dialog->pebInput[idx] = gtk_event_box_new () ;
  gtk_widget_show (dialog->pebInput[idx]) ;
  gtk_table_attach (GTK_TABLE (dialog->tblVT), dialog->pebInput[idx], idx + 1, idx + 2, 0, 1,
      	      	    (GtkAttachOptions)(GTK_FILL),
		    (GtkAttachOptions)(GTK_FILL), 0, 0) ;
  gtk_object_set_data (GTK_OBJECT (dialog->pebInput[idx]), "idx", (gpointer)idx) ;
  gtk_object_set_data (GTK_OBJECT (dialog->pebInput[idx]), "bActive", (gpointer)TRUE) ;
  
  dialog->pfmInput[idx] = gtk_frame_new (NULL) ;
  gtk_widget_show (dialog->pfmInput[idx]) ;
  gtk_container_add (GTK_CONTAINER (dialog->pebInput[idx]), dialog->pfmInput[idx]) ;

  gtk_frame_set_shadow_type (GTK_FRAME (dialog->pfmInput[idx]), GTK_SHADOW_ETCHED_OUT) ;
  
  dialog->plblInput[idx] = gtk_label_new (pvt->inputs[idx]->label) ;
  gtk_widget_show (dialog->plblInput[idx]) ;
  gtk_container_add (GTK_CONTAINER (dialog->pfmInput[idx]), dialog->plblInput[idx]) ;
  
  gtk_signal_connect (GTK_OBJECT (dialog->pebInput[idx]), "button_press_event", GTK_SIGNAL_FUNC (Input_buttondown), dialog->dlgVectorTable) ;
  }

void DestroyInputHeading (new_vector_table_options_D *dialog, int idx)
  {
  char sz[16] = "" ;
  gtk_container_remove (GTK_CONTAINER (dialog->pfmInput[idx]), dialog->plblInput[idx]) ;
  g_snprintf (sz, 16, "0x%08X", (int)dialog->plblInput[idx]) ;
  gtk_object_remove_data (GTK_OBJECT (dialog->dlgVectorTable), sz) ;
//  gtk_widget_destroy (dialog->plblInput[idx]) ;
  gtk_container_remove (GTK_CONTAINER (dialog->pebInput[idx]), dialog->pfmInput[idx]) ;
  g_snprintf (sz, 16, "0x%08X", (int)dialog->pfmInput[idx]) ;
  gtk_object_remove_data (GTK_OBJECT (dialog->dlgVectorTable), sz) ;
//  gtk_widget_destroy (dialog->pfmInput[idx]) ;
  gtk_signal_disconnect_by_func (GTK_OBJECT (dialog->pebInput[idx]), GTK_SIGNAL_FUNC (Input_buttondown), dialog->dlgVectorTable) ;
  gtk_container_remove (GTK_CONTAINER (dialog->tblVT), dialog->pebInput[idx]) ;
  g_snprintf (sz, 16, "0x%08X", (int)dialog->pebInput[idx]) ;
  gtk_object_remove_data (GTK_OBJECT (dialog->dlgVectorTable), sz) ;
//  gtk_widget_destroy (dialog->pebInput[idx]) ;
  }

void create_new_vector_table_options_dialog (new_vector_table_options_D *dialog)
  {
  GtkWidget *mnuSp = NULL, *spacer = NULL ;

  dialog->dlgVectorTable = gtk_dialog_new ();
  gtk_object_set_data (GTK_OBJECT (dialog->dlgVectorTable), "dlgVectorTable", dialog->dlgVectorTable);
  gtk_widget_set_usize (dialog->dlgVectorTable, 640, 480);
  gtk_window_set_modal (GTK_WINDOW (dialog->dlgVectorTable), TRUE);
  gtk_window_set_title (GTK_WINDOW (dialog->dlgVectorTable), _("Vector Table Setup"));
  gtk_window_set_default_size (GTK_WINDOW (dialog->dlgVectorTable), 640, 480);
  gtk_window_set_policy (GTK_WINDOW (dialog->dlgVectorTable), FALSE, FALSE, FALSE);

  dialog->dialog_vbox1 = GTK_DIALOG (dialog->dlgVectorTable)->vbox;
  gtk_object_set_data (GTK_OBJECT (dialog->dlgVectorTable), "dialog_vbox1", dialog->dialog_vbox1);
  gtk_widget_show (dialog->dialog_vbox1);

  dialog->tblVTMain = gtk_table_new (3, 1, FALSE);
  gtk_widget_ref (dialog->tblVTMain);
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "tblVTMain", dialog->tblVTMain,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dialog->tblVTMain);
  gtk_box_pack_start (GTK_BOX (dialog->dialog_vbox1), dialog->tblVTMain, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (dialog->tblVTMain), 2);

  dialog->fmVTOps = gtk_frame_new (_("Vector Table Operations"));
  gtk_widget_ref (dialog->fmVTOps);
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "fmVTOps", dialog->fmVTOps,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dialog->fmVTOps);
  gtk_table_attach (GTK_TABLE (dialog->tblVTMain), dialog->fmVTOps, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (dialog->fmVTOps), 2);

  dialog->tblOps = gtk_table_new (2, 1, FALSE);
  gtk_widget_ref (dialog->tblOps);
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "tblOps", dialog->tblOps,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dialog->tblOps);
  gtk_container_add (GTK_CONTAINER (dialog->fmVTOps), dialog->tblOps);

  dialog->lblMsg = gtk_label_new (_("Note: Right-click the vector table for more options."));
  gtk_widget_ref (dialog->lblMsg);
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "lblMsg", dialog->lblMsg,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dialog->lblMsg);
  gtk_table_attach (GTK_TABLE (dialog->tblOps), dialog->lblMsg, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_label_set_justify (GTK_LABEL (dialog->lblMsg), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (dialog->lblMsg), 0, 0.5);

  dialog->hbVTOps = gtk_hbutton_box_new ();
  gtk_widget_ref (dialog->hbVTOps);
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "hbVTOps", dialog->hbVTOps,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dialog->hbVTOps);
  gtk_table_attach (GTK_TABLE (dialog->tblOps), dialog->hbVTOps, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 2, 2);
  gtk_container_set_border_width (GTK_CONTAINER (dialog->hbVTOps), 2);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog->hbVTOps), GTK_BUTTONBOX_START);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog->hbVTOps), 0);
  gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (dialog->hbVTOps), 0, 0);

  dialog->btnLoad = gtk_button_new_with_label (_("Load..."));
  gtk_widget_ref (dialog->btnLoad);
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "btnLoad", dialog->btnLoad,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dialog->btnLoad);
  gtk_container_add (GTK_CONTAINER (dialog->hbVTOps), dialog->btnLoad);
  GTK_WIDGET_SET_FLAGS (dialog->btnLoad, GTK_CAN_DEFAULT);

  dialog->btnSave = gtk_button_new_with_label (_("Save"));
  gtk_widget_ref (dialog->btnSave);
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "btnSave", dialog->btnSave,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dialog->btnSave);
  gtk_container_add (GTK_CONTAINER (dialog->hbVTOps), dialog->btnSave);
  GTK_WIDGET_SET_FLAGS (dialog->btnSave, GTK_CAN_DEFAULT);

  dialog->btnSaveAs = gtk_button_new_with_label (_("Save As..."));
  gtk_widget_ref (dialog->btnSaveAs);
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "btnSaveAs", dialog->btnSaveAs,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dialog->btnSaveAs);
  gtk_container_add (GTK_CONTAINER (dialog->hbVTOps), dialog->btnSaveAs);
  GTK_WIDGET_SET_FLAGS (dialog->btnSaveAs, GTK_CAN_DEFAULT);

  dialog->swndVT = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (dialog->swndVT);
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "swndVT", dialog->swndVT,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dialog->swndVT);
  gtk_table_attach (GTK_TABLE (dialog->tblVTMain), dialog->swndVT, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 2, 2);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (dialog->swndVT), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  dialog->viewport1 = gtk_viewport_new (NULL, NULL);
  gtk_widget_ref (dialog->viewport1);
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "viewport1", dialog->viewport1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dialog->viewport1);
  gtk_container_add (GTK_CONTAINER (dialog->swndVT), dialog->viewport1);
  
  dialog->ebVT = gtk_event_box_new () ;
  gtk_widget_ref (dialog->ebVT) ;
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "ebVT", dialog->ebVT,
      	      	      	    (GtkDestroyNotify) gtk_widget_unref) ;
  gtk_widget_show (dialog->ebVT) ;
  gtk_container_add (GTK_CONTAINER (dialog->viewport1), dialog->ebVT) ;

  dialog->tblVT = gtk_table_new (2, 2, TRUE);
  gtk_widget_ref (dialog->tblVT);
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "tblVT", dialog->tblVT,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dialog->tblVT);
  gtk_container_add (GTK_CONTAINER (dialog->ebVT), dialog->tblVT);
  
  spacer = gtk_label_new ("") ;
  gtk_widget_ref (spacer) ;
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "spacer", spacer,
      	      	      	    (GtkDestroyNotify) gtk_widget_unref) ;
  gtk_widget_show (spacer) ;
  gtk_table_attach (GTK_TABLE (dialog->tblVT), spacer, 0, 1, 0, 1,
    (GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(GTK_FILL), 0, 0) ;
  gtk_widget_set_usize (spacer, 60, 0) ;
  
  dialog->hbVTFile = gtk_hbox_new (FALSE, 2) ;
  gtk_widget_ref (dialog->hbVTFile) ;
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "hbVTFile", dialog->hbVTFile,
      	      	      	    (GtkDestroyNotify) gtk_widget_unref) ;
  gtk_widget_show (dialog->hbVTFile) ;
  gtk_table_attach (GTK_TABLE (dialog->tblVTMain), dialog->hbVTFile, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);

  dialog->lblVTFileMsg = gtk_label_new ("File:") ;
  gtk_widget_ref (dialog->lblVTFileMsg) ;
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "lblVTFileMsg", dialog->lblVTFileMsg,
      	      	      	    (GtkDestroyNotify) gtk_widget_unref) ;
  gtk_widget_show (dialog->lblVTFileMsg) ;
  gtk_box_pack_start (GTK_BOX (dialog->hbVTFile), dialog->lblVTFileMsg, FALSE, TRUE, 0) ;
  gtk_label_set_justify (GTK_LABEL (dialog->lblVTFileMsg), GTK_JUSTIFY_RIGHT) ;
  gtk_misc_set_alignment (GTK_MISC (dialog->lblVTFileMsg), 1.0, 0.5) ;

  dialog->lblVTFile = gtk_label_new ("");
  gtk_widget_ref (dialog->lblVTFile);
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "lblVTFile", dialog->lblVTFile,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dialog->lblVTFile);
  gtk_box_pack_start (GTK_BOX (dialog->hbVTFile), dialog->lblVTFile, FALSE, TRUE, 0) ;
  gtk_label_set_justify (GTK_LABEL (dialog->lblVTFile), GTK_JUSTIFY_LEFT) ;
  gtk_misc_set_alignment (GTK_MISC (dialog->lblVTFile), 0.0, 0.5) ;

  dialog->dialog_action_area1 = GTK_DIALOG (dialog->dlgVectorTable)->action_area;
  gtk_object_set_data (GTK_OBJECT (dialog->dlgVectorTable), "dialog_action_area1", dialog->dialog_action_area1);
  gtk_widget_show (dialog->dialog_action_area1);
  gtk_container_set_border_width (GTK_CONTAINER (dialog->dialog_action_area1), 1);

  dialog->hbuttonbox1 = gtk_hbutton_box_new ();
  gtk_widget_ref (dialog->hbuttonbox1);
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "hbuttonbox1", dialog->hbuttonbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dialog->hbuttonbox1);
  gtk_box_pack_start (GTK_BOX (dialog->dialog_action_area1), dialog->hbuttonbox1, TRUE, TRUE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog->hbuttonbox1), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog->hbuttonbox1), 0);
  gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (dialog->hbuttonbox1), 0, 0);

  dialog->btnOK = gtk_button_new_with_label (_("OK"));
  gtk_widget_ref (dialog->btnOK);
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "btnOK", dialog->btnOK,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dialog->btnOK);
  gtk_container_add (GTK_CONTAINER (dialog->hbuttonbox1), dialog->btnOK);
  GTK_WIDGET_SET_FLAGS (dialog->btnOK, GTK_CAN_DEFAULT);

  dialog->btnCancel = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_ref (dialog->btnCancel);
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "btnCancel", dialog->btnCancel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dialog->btnCancel);
  gtk_container_add (GTK_CONTAINER (dialog->hbuttonbox1), dialog->btnCancel);
  GTK_WIDGET_SET_FLAGS (dialog->btnCancel, GTK_CAN_DEFAULT);
  
  dialog->mnuVT = gtk_menu_new () ;
  gtk_widget_ref (dialog->mnuVT) ;
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "mnuVT", dialog->mnuVT,
      	      	      	    (GtkDestroyNotify) gtk_widget_unref) ;

  dialog->mnuDeact = gtk_menu_item_new_with_label (_("Deactivate Input")) ;
  gtk_widget_ref (dialog->mnuDeact) ;
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "mnuDeact", dialog->mnuDeact,
      	      	      	    (GtkDestroyNotify) gtk_widget_unref) ;
  gtk_widget_show (dialog->mnuDeact) ;
  gtk_container_add (GTK_CONTAINER (dialog->mnuVT), dialog->mnuDeact) ;
  
  dialog->mnuAct = gtk_menu_item_new_with_label (_("Activate Input")) ;
  gtk_widget_ref (dialog->mnuAct) ;
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "mnuAct", dialog->mnuAct,
      	      	      	    (GtkDestroyNotify) gtk_widget_unref) ;
  gtk_widget_show (dialog->mnuAct) ;
  gtk_container_add (GTK_CONTAINER (dialog->mnuVT), dialog->mnuAct) ;
  
  mnuSp = gtk_menu_item_new () ;
  gtk_widget_show (mnuSp) ;
  gtk_container_add (GTK_CONTAINER (dialog->mnuVT), mnuSp) ;
  
  dialog->mnuInsBefore = gtk_menu_item_new_with_label (_("Insert Vector Before")) ;
  gtk_widget_ref (dialog->mnuInsBefore) ;
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "mnuInsBefore", dialog->mnuInsBefore,
      	      	      	    (GtkDestroyNotify) gtk_widget_unref) ;
  gtk_widget_show (dialog->mnuInsBefore) ;
  gtk_container_add (GTK_CONTAINER (dialog->mnuVT), dialog->mnuInsBefore) ;
  
  dialog->mnuInsAfter = gtk_menu_item_new_with_label (_("Insert Vector After")) ;
  gtk_widget_ref (dialog->mnuInsAfter) ;
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "mnuInsAfter", dialog->mnuInsAfter,
      	      	      	    (GtkDestroyNotify) gtk_widget_unref) ;
  gtk_widget_show (dialog->mnuInsAfter) ;
  gtk_container_add (GTK_CONTAINER (dialog->mnuVT), dialog->mnuInsAfter) ;
  
  dialog->mnuDel = gtk_menu_item_new_with_label (_("Delete Vector")) ;
  gtk_widget_ref (dialog->mnuDel) ;
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "mnuDel", dialog->mnuDel,
      	      	      	    (GtkDestroyNotify) gtk_widget_unref) ;
  gtk_widget_show (dialog->mnuDel) ;
  gtk_container_add (GTK_CONTAINER (dialog->mnuVT), dialog->mnuDel) ;
  
  mnuSp = gtk_menu_item_new () ;
  gtk_widget_show (mnuSp) ;
  gtk_container_add (GTK_CONTAINER (dialog->mnuVT), mnuSp) ;
  
  dialog->mnuAdd = gtk_menu_item_new_with_label (_("Add Vector")) ;
  gtk_widget_ref (dialog->mnuAdd) ;
  gtk_object_set_data_full (GTK_OBJECT (dialog->dlgVectorTable), "mnuAdd", dialog->mnuAdd,
      	      	      	    (GtkDestroyNotify) gtk_widget_unref) ;
  gtk_widget_show (dialog->mnuAdd) ;
  gtk_container_add (GTK_CONTAINER (dialog->mnuVT), dialog->mnuAdd) ;
  
  gtk_signal_connect_object (GTK_OBJECT (dialog->dlgVectorTable), "delete_event", GTK_SIGNAL_FUNC (gtk_widget_hide), GTK_OBJECT (dialog->dlgVectorTable)) ;
  gtk_signal_connect_object (GTK_OBJECT (dialog->btnCancel), "clicked", GTK_SIGNAL_FUNC (gtk_widget_hide), GTK_OBJECT (dialog->dlgVectorTable)) ;
  gtk_signal_connect (GTK_OBJECT (dialog->btnOK), "clicked", GTK_SIGNAL_FUNC (vector_table_options_dialog_btnOK_clicked), GTK_OBJECT (dialog->dlgVectorTable)) ;
  gtk_signal_connect (GTK_OBJECT (dialog->btnLoad), "clicked", GTK_SIGNAL_FUNC (load_vector_table), GTK_OBJECT (dialog->dlgVectorTable)) ;
  gtk_signal_connect (GTK_OBJECT (dialog->btnSave), "clicked", GTK_SIGNAL_FUNC (save_vector_table), GTK_OBJECT (dialog->dlgVectorTable)) ;
  gtk_signal_connect (GTK_OBJECT (dialog->btnSaveAs), "clicked", GTK_SIGNAL_FUNC (save_vector_table), GTK_OBJECT (dialog->dlgVectorTable)) ;
  gtk_signal_connect (GTK_OBJECT (dialog->ebVT), "button_press_event", GTK_SIGNAL_FUNC (VT_buttondown), dialog->dlgVectorTable) ;
  gtk_signal_connect (GTK_OBJECT (dialog->mnuDeact), "activate", GTK_SIGNAL_FUNC (set_column_active), dialog->dlgVectorTable) ;
  gtk_signal_connect (GTK_OBJECT (dialog->mnuAct), "activate", GTK_SIGNAL_FUNC (set_column_active), dialog->dlgVectorTable) ;
  gtk_signal_connect (GTK_OBJECT (dialog->mnuInsBefore), "activate", GTK_SIGNAL_FUNC (create_vector), dialog->dlgVectorTable) ;
  gtk_signal_connect (GTK_OBJECT (dialog->mnuInsAfter), "activate", GTK_SIGNAL_FUNC (create_vector), dialog->dlgVectorTable) ;
  gtk_signal_connect (GTK_OBJECT (dialog->mnuAdd), "activate", GTK_SIGNAL_FUNC (create_vector), dialog->dlgVectorTable) ;
  gtk_signal_connect (GTK_OBJECT (dialog->mnuDel), "activate", GTK_SIGNAL_FUNC (delete_vector), dialog->dlgVectorTable) ;
  }

gboolean Input_buttondown (GtkWidget *widget, GdkEventButton *ev, gpointer user_data)
  {
  new_vector_table_options_D *dialog = (new_vector_table_options_D *)gtk_object_get_data (GTK_OBJECT (user_data), "dialog") ;
  
  if (3 == ev->button) /* right-click */
    {
    int idx ;
    gboolean bActive ;
    gtk_object_set_data (GTK_OBJECT (dialog->dlgVectorTable), "idxInput",
      (gpointer)(idx = (int)gtk_object_get_data (GTK_OBJECT (widget), "idx"))) ;
    bActive = (gboolean)gtk_object_get_data (GTK_OBJECT (dialog->pebInput[idx]), "bActive") ;
    gtk_widget_set_sensitive (dialog->mnuAct, !bActive) ;
    gtk_widget_set_sensitive (dialog->mnuDeact, bActive && (CountActiveInputs (dialog) > 1)) ;
    gtk_widget_set_sensitive (dialog->mnuInsBefore, FALSE) ;
    gtk_widget_set_sensitive (dialog->mnuInsAfter, FALSE) ;
    gtk_widget_set_sensitive (dialog->mnuAdd, FALSE) ;
    gtk_widget_set_sensitive (dialog->mnuDel, FALSE) ;
    gtk_menu_popup (GTK_MENU (dialog->mnuVT), NULL, NULL, NULL, NULL, ev->button, ev->time) ;
    
    return TRUE ;
    }
  return FALSE ;
  }

gboolean Vector_buttondown (GtkWidget *widget, GdkEventButton *ev, gpointer user_data)
  {
  new_vector_table_options_D *dialog = (new_vector_table_options_D *)gtk_object_get_data (GTK_OBJECT (user_data), "dialog") ;

  if (3 == ev->button) /* right-click */
    {
    gtk_object_set_data (GTK_OBJECT (dialog->dlgVectorTable), "idxVector",
      (gpointer)gtk_object_get_data (GTK_OBJECT (widget), "idx")) ;
    gtk_widget_set_sensitive (dialog->mnuAct, FALSE) ;
    gtk_widget_set_sensitive (dialog->mnuDeact, FALSE) ;
    gtk_widget_set_sensitive (dialog->mnuInsBefore, TRUE) ;
    gtk_widget_set_sensitive (dialog->mnuInsAfter, TRUE) ;
    gtk_widget_set_sensitive (dialog->mnuAdd, FALSE) ;
    gtk_widget_set_sensitive (dialog->mnuDel, TRUE) ;
    gtk_menu_popup (GTK_MENU (dialog->mnuVT), NULL, NULL, NULL, NULL, ev->button, ev->time) ;
    
    return TRUE ;
    }
  return FALSE ;
  }

gboolean VT_buttondown (GtkWidget *widget, GdkEventButton *ev, gpointer user_data)
  {
  new_vector_table_options_D *dialog = (new_vector_table_options_D *)gtk_object_get_data (GTK_OBJECT (user_data), "dialog") ;

  if (3 == ev->button) /* right-click */
    {
    gtk_object_set_data (GTK_OBJECT (dialog->dlgVectorTable), "idxVector", (gpointer)-1) ;
    gtk_object_set_data (GTK_OBJECT (dialog->dlgVectorTable), "idxInput", (gpointer)-1) ;
    gtk_widget_set_sensitive (dialog->mnuAct, FALSE) ;
    gtk_widget_set_sensitive (dialog->mnuDeact, FALSE) ;
    gtk_widget_set_sensitive (dialog->mnuInsBefore, FALSE) ;
    gtk_widget_set_sensitive (dialog->mnuInsAfter, FALSE) ;
    gtk_widget_set_sensitive (dialog->mnuAdd, TRUE) ;
    gtk_widget_set_sensitive (dialog->mnuDel, FALSE) ;
    gtk_menu_popup (GTK_MENU (dialog->mnuVT), NULL, NULL, NULL, NULL, ev->button, ev->time) ;
    
    return TRUE ;
    }
  return FALSE ;
  }

gboolean Bit_buttondown (GtkWidget *widget, GdkEventButton *ev, gpointer user_data)
  {
  new_vector_table_options_D *dialog = (new_vector_table_options_D *)gtk_object_get_data (GTK_OBJECT (user_data), "dialog") ;

  if (3 == ev->button) /* right-click */
    {
    gboolean bActive ;
    int idxRow = -1, idxCol = -1 ;
    gtk_object_set_data (GTK_OBJECT (dialog->dlgVectorTable), "idxVector",
      (gpointer)(idxRow = (int)gtk_object_get_data (GTK_OBJECT (widget), "idxRow"))) ;
    gtk_object_set_data (GTK_OBJECT (dialog->dlgVectorTable), "idxInput",
      (gpointer)(idxCol = (int)gtk_object_get_data (GTK_OBJECT (widget), "idxCol"))) ;
    
    DBG_NVTO (fprintf (stderr, "Button is (r,c)=(%d,%d)\n", idxRow, idxCol)) ;
    
    bActive = (gboolean)gtk_object_get_data (GTK_OBJECT (dialog->pebInput[idxCol]), "bActive") ;
    
    gtk_widget_set_sensitive (dialog->mnuAct, !bActive) ;
    gtk_widget_set_sensitive (dialog->mnuDeact, bActive && (CountActiveInputs (dialog) > 1)) ;
    gtk_widget_set_sensitive (dialog->mnuInsBefore, TRUE) ;
    gtk_widget_set_sensitive (dialog->mnuInsAfter, TRUE) ;
    gtk_widget_set_sensitive (dialog->mnuAdd, TRUE) ;
    gtk_widget_set_sensitive (dialog->mnuDel, TRUE) ;
    gtk_menu_popup (GTK_MENU (dialog->mnuVT), NULL, NULL, NULL, NULL, ev->button, ev->time) ;

    return TRUE ;
    }
  return FALSE ;
  }

void click_bit_button (GtkWidget *widget, gpointer user_data)
  {
  gtk_label_set_text (GTK_LABEL (user_data), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)) ? "1" : "0") ;
  }

void set_column_active (GtkWidget *widget, gpointer user_data)
  {
  new_vector_table_options_D *dialog = (new_vector_table_options_D *)gtk_object_get_data (GTK_OBJECT (user_data), "dialog") ;
  int idx = (int)gtk_object_get_data (GTK_OBJECT (user_data), "idxInput") ;
  
  if (-1 != idx)
    SetColumnActive (dialog, idx, (dialog->mnuAct == widget)) ;
  }

void create_vector (GtkWidget *widget, gpointer user_data)
  {
  int idx = (int)gtk_object_get_data (GTK_OBJECT (user_data), "idxVector") ;
  new_vector_table_options_D *dialog = (new_vector_table_options_D *)gtk_object_get_data (GTK_OBJECT (user_data), "dialog") ;
  
  if (-1 == idx) idx = dialog->icVectorsUsed ;
  if (dialog->mnuInsAfter == widget) idx++ ;
  idx = CLAMP (idx, 0, dialog->icVectorsUsed) ;
  
  CreateVector (dialog, idx) ;
  }

void delete_vector (GtkWidget *widget, gpointer user_data)
  {
  int idx = (int)gtk_object_get_data (GTK_OBJECT (user_data), "idxVector") ;
  new_vector_table_options_D *dialog = (new_vector_table_options_D *)gtk_object_get_data (GTK_OBJECT (user_data), "dialog") ;
  
  if (-1 != idx)
    DeleteVector (dialog, idx) ;
  }

void load_vector_table (GtkWidget *widget, gpointer user_data)
  {
  new_vector_table_options_D *dialog = (new_vector_table_options_D *)gtk_object_get_data (GTK_OBJECT (user_data), "dialog") ;
  VectorTable *pvtDlg = (VectorTable *)gtk_object_get_data (GTK_OBJECT (user_data), "pvtDlg") ;
  VTL_RESULT vtlr = VTL_OK ;
  char szFName[PATH_LENGTH] = "" ;
  
  get_file_name_from_user (GTK_WINDOW (dialog->dlgVectorTable), "Load Vector Table", szFName, PATH_LENGTH) ;
  if (0 == szFName[0])
    return ;
  else
    g_snprintf (pvtDlg->szFName, PATH_LENGTH, "%s", szFName) ;
  
  if (VTL_OK != (vtlr = VectorTable_load (pvtDlg)))
    message_box (GTK_WINDOW (dialog->dlgVectorTable), MB_OK, "Vector Table Load",
      vtlr == VTL_FILE_FAILED ?  "Failed to open file '%s'." :
      vtlr == VTL_MAGIC_FAILED ? "File '%s' does not appear to be a vector table file (Invalid magic)." :
      vtlr == VTL_SHORT ?        "File '%s' contained fewer inputs than the current design.  The vectors were padded with 0s." :
      	      	      		 "File '%s' contained more inputs than the current design.  The vectors were truncated.",
      pvtDlg->szFName) ;
  
  if (VTL_OK == vtlr || VTL_SHORT == vtlr || VTL_TRUNC == vtlr)
    VectorTableToDialog (dialog, pvtDlg) ;

  while (pvtDlg->num_of_vectors > 0)
    VectorTable_del_vector (pvtDlg, pvtDlg->num_of_vectors - 1) ;
  }

void save_vector_table (GtkWidget *widget, gpointer user_data)
  {
  new_vector_table_options_D *dialog = (new_vector_table_options_D *)gtk_object_get_data (GTK_OBJECT (user_data), "dialog") ;
  VectorTable *pvtDlg = (VectorTable *)gtk_object_get_data (GTK_OBJECT (user_data), "pvtDlg") ;
  
  if (dialog->btnSaveAs == widget)
    pvtDlg->szFName[0] = 0 ;
  
  if (0 == pvtDlg->szFName[0])
    {
    char szFName[PATH_LENGTH] = "" ;
    get_file_name_from_user (GTK_WINDOW (dialog->dlgVectorTable), "Save Vector Table As", szFName, PATH_LENGTH) ;
    if (0 == szFName[0])
      return ;
    else
      g_snprintf (pvtDlg->szFName, PATH_LENGTH, "%s", szFName) ;
    }
  
  DialogToVectorTable (dialog, pvtDlg) ;
  if (!VectorTable_save (pvtDlg))
    message_box (GTK_WINDOW (dialog->dlgVectorTable), MB_OK, "Vector Table Save", "Failed to save vector table to file '%s'.", pvtDlg->szFName) ;
  else
    SetCurrentFileName (dialog, pvtDlg->szFName) ;
  }

void vector_table_options_dialog_btnOK_clicked (GtkWidget *widget, gpointer user_data)
  {
  new_vector_table_options_D *dialog = (new_vector_table_options_D *)gtk_object_get_data (GTK_OBJECT (user_data), "dialog") ;
  VectorTable *pvt = (VectorTable *)gtk_object_get_data (GTK_OBJECT (user_data), "pvt") ;
  
  DialogToVectorTable (dialog, pvt) ;
  
  gtk_widget_hide (dialog->dlgVectorTable) ;
  }

void DeleteVector (new_vector_table_options_D *dialog, int idx)
  {
  char sz[16] = "" ;
  int Nix, Nix1 ;

  DestroyIdxLabel (dialog, idx) ;
  for (Nix = 0 ; Nix < dialog->icInputsUsed ; Nix++)
    DestroyVectorToggle (dialog, idx, Nix) ;

  for (Nix = idx + 1 ; Nix < dialog->icVectorsUsed ; Nix++)
    {
    g_snprintf (sz, 16, "%d", Nix - 1) ;

    gtk_container_remove (GTK_CONTAINER (dialog->tblVT), dialog->pebIdx[Nix]) ;
    gtk_table_attach (GTK_TABLE (dialog->tblVT), dialog->pebIdx[Nix],
      0, 1, Nix, Nix + 1, (GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(GTK_FILL), 2, 2) ;
    gtk_label_set_text (GTK_LABEL (dialog->plblIdx[Nix]), sz) ;
    gtk_object_set_data (GTK_OBJECT (dialog->pebIdx[Nix]), "idx", (gpointer)(Nix - 1)) ;
    for (Nix1 = 0 ; Nix1 < dialog->icInputsUsed ; Nix1++)
      {
      gtk_container_remove (GTK_CONTAINER (dialog->tblVT), dialog->pptbBit[Nix][Nix1]) ;
      gtk_table_attach (GTK_TABLE (dialog->tblVT), dialog->pptbBit[Nix][Nix1],
	Nix1 + 1, Nix1 + 2, Nix, Nix + 1, (GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(GTK_FILL), 0, 0) ;
      gtk_object_set_data (GTK_OBJECT (dialog->pptbBit[Nix][Nix1]), "idxRow", (gpointer)(Nix - 1)) ;
      }
    }

  if (idx < dialog->icVectorsUsed - 1)
    {
    GtkWidget **p = NULL ;
    memmove (&(dialog->plblIdx[idx]), &(dialog->plblIdx[idx + 1]), (dialog->icVectorsUsed - idx) * sizeof (GtkWidget *)) ;
    memmove (&(dialog->pebIdx[idx]), &(dialog->pebIdx[idx + 1]), (dialog->icVectorsUsed - idx) * sizeof (GtkWidget *)) ;

    p = dialog->pptblblBit[idx] ;
    memmove (&(dialog->pptblblBit[idx]), &(dialog->pptblblBit[idx + 1]), (dialog->icVectorsUsed - idx) * sizeof (GtkWidget **)) ;
    dialog->pptblblBit[dialog->icVectorsUsed - 1] = p ;

    p = dialog->pptbBit[idx] ;
    memmove (&(dialog->pptbBit[idx]), &(dialog->pptbBit[idx + 1]), (dialog->icVectorsUsed - idx) * sizeof (GtkWidget **)) ;
    dialog->pptbBit[dialog->icVectorsUsed - 1] = p ;
    }
  dialog->icVectorsUsed-- ;

  gtk_table_resize (GTK_TABLE (dialog->tblVT), dialog->icInputsUsed + 1, dialog->icVectorsUsed + 1) ;
  }

void CreateVector (new_vector_table_options_D *dialog, int idx)
  {
  char sz[16] = "" ;
  int Nix, Nix1 ;
  if (dialog->icVectorsUsed == dialog->icVectors)
    {
    dialog->icVectors = dialog->icVectors * 2 + 1 ;
    dialog->pebIdx = realloc (dialog->pebIdx, dialog->icVectors * sizeof (GtkWidget *)) ;
    dialog->plblIdx = realloc (dialog->plblIdx, dialog->icVectors * sizeof (GtkWidget *)) ;
    dialog->pptblblBit = realloc (dialog->pptblblBit, dialog->icVectors * sizeof (GtkWidget **)) ;
    dialog->pptbBit = realloc (dialog->pptbBit, dialog->icVectors * sizeof (GtkWidget **)) ;
    for (Nix = dialog->icVectorsUsed ; Nix < dialog->icVectors ; Nix++)
      {
      dialog->pptblblBit[Nix] = malloc (dialog->icInputs * sizeof (GtkWidget *)) ;
      dialog->pptbBit[Nix] = malloc (dialog->icInputs * sizeof (GtkWidget *)) ;
      }
    }

  if (idx < dialog->icVectorsUsed)
    {
    GtkWidget **p = NULL ;
    for (Nix = dialog->icVectorsUsed - 1 ; Nix > idx - 1 ; Nix--)
      {
      gtk_container_remove (GTK_CONTAINER (dialog->tblVT), dialog->pebIdx[Nix]) ;
      gtk_table_attach (GTK_TABLE (dialog->tblVT), dialog->pebIdx[Nix], 0, 1, Nix + 2, Nix + 3,
      	(GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(GTK_FILL), 2, 2) ;
      g_snprintf (sz, 16, "%d", Nix + 1) ;
      gtk_label_set_text (GTK_LABEL (dialog->plblIdx[Nix]), sz) ;
      gtk_object_set_data (GTK_OBJECT (dialog->pebIdx[Nix]), "idx", (gpointer)(Nix + 1)) ;
      for (Nix1 = 0 ; Nix1 < dialog->icInputsUsed ; Nix1++)
        {
	gtk_container_remove (GTK_CONTAINER (dialog->tblVT), dialog->pptbBit[Nix][Nix1]) ;
	gtk_table_attach (GTK_TABLE (dialog->tblVT), dialog->pptbBit[Nix][Nix1],
	  Nix1 + 1, Nix1 + 2, Nix + 2, Nix + 3, (GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(GTK_FILL), 0, 0) ;
        gtk_object_set_data (GTK_OBJECT (dialog->pptbBit[Nix][Nix1]), "idxRow", (gpointer)(Nix + 1)) ;
	}
      }
    memmove (&(dialog->pebIdx[idx + 1]), &(dialog->pebIdx[idx]), (dialog->icVectorsUsed - idx) * sizeof (GtkWidget *)) ;
    memmove (&(dialog->plblIdx[idx + 1]), &(dialog->plblIdx[idx]), (dialog->icVectorsUsed - idx) * sizeof (GtkWidget *)) ;
    
    p = dialog->pptbBit[dialog->icVectorsUsed] ;
    memmove (&(dialog->pptbBit[idx + 1]), &(dialog->pptbBit[idx]), (dialog->icVectorsUsed - idx) * sizeof (GtkWidget **)) ;
    dialog->pptbBit[idx] = p ;
    
    p = dialog->pptblblBit[dialog->icVectorsUsed] ;
    memmove (&(dialog->pptblblBit[idx + 1]), &(dialog->pptblblBit[idx]), (dialog->icVectorsUsed - idx) * sizeof (GtkWidget **)) ;
    dialog->pptblblBit[idx] = p ;
    }
  dialog->icVectorsUsed++ ;
  
  CreateIdxLabel (dialog, idx) ;
  for (Nix = 0 ; Nix < dialog->icInputsUsed ; Nix++)
    CreateVectorToggle (dialog, FALSE, idx, Nix) ;
  
  gtk_table_resize (GTK_TABLE (dialog->tblVT), dialog->icInputsUsed + 1, dialog->icVectorsUsed + 1) ;
  }

void SetCurrentFileName (new_vector_table_options_D *dialog, char *pszFName)
  {
  gtk_label_set_text (GTK_LABEL (dialog->lblVTFile), pszFName) ;
  g_snprintf ((char *)gtk_object_get_data (GTK_OBJECT (dialog->dlgVectorTable), "szCurrentFName"), PATH_LENGTH, "%s", pszFName) ;
  }

int CountActiveInputs (new_vector_table_options_D *dialog)
  {
  int iRet = 0, Nix ;
  
  for (Nix = 0 ; Nix < dialog->icInputs ; Nix++)
    if ((gboolean)gtk_object_get_data (GTK_OBJECT (dialog->pebInput[Nix]), "bActive"))
      iRet++ ;
  
  return iRet ;
  }
