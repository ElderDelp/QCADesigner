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
// GTK preamble. This is where we call gtk_init,        //
// register all the stock items, add the pixmap         //
// directories.                                         //
//                                                      //
//////////////////////////////////////////////////////////

#ifdef GTK_GUI

#include "support.h"
#include "gtk_preamble.h"
#include "qcadstock.h"
#include "global_consts.h"
#include "custom_widgets.h"
#ifdef WIN32
  #include <windows.h>

static char *get_locale () ;
static char *lcid_to_posix_locale (int lcid) ;

#ifdef QCAD_NO_CONSOLE
static char **CmdLineToArgv (char *pszCmdLine, int *pargc) ;
static void my_logger (const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data) ;
#endif /* ifdef QCAD_NO_CONSOLE */
#endif /* ifdef WIN32 */

static GtkStockItem stock_layers[] =
  {
  {QCAD_STOCK_SUBSTRATE_LAYER,    N_("Substrate"),    0, 0, PACKAGE},
  {QCAD_STOCK_CELL_LAYER,         N_("Cells"),        0, 0, PACKAGE},
  {QCAD_STOCK_CLOCKING_LAYER,     N_("Clocking"),     0, 0, PACKAGE},
  {QCAD_STOCK_DRAWING_LAYER,      N_("Drawing"),      0, 0, PACKAGE},
  {QCAD_STOCK_DISTRIBUTION_LAYER, N_("Distribution"), 0, 0, PACKAGE},
  } ;
static int n_stock_layers = G_N_ELEMENTS (stock_layers) ;

#ifdef QCAD_NO_CONSOLE
void gtk_preamble (int *pargc, char ***pargv, char *pszBaseName, char *pszCmdLine)
#else
void gtk_preamble (int *pargc, char ***pargv, char *pszBaseName)
#endif /* def QCAD_NO_CONSOLE */
  {
#ifdef WIN32
  char *psz = NULL, *pszModuleFName = NULL, szBuf[MAX_PATH] = "" ;
  char *pszHomeHDD = getenv ("HOMEDRIVE") ;
  char *pszHomeDIR = getenv ("HOMEPATH") ;
  int Nix ;
  // Need this buffer later on for the pixmap dirs
  char szMyPath[PATH_LENGTH] = "" ;

  // Must set the home directory to a reasonable value.  If all else fails,
  // set it to the current directory
  if (!(NULL == pszHomeHDD || NULL == pszHomeDIR))
    {
    putenv (psz = g_strdup_printf ("HOME=%s%s", pszHomeHDD, pszHomeDIR)) ;
    g_free (psz) ;
    }
  else
    putenv ("HOME=.") ;
#endif /* def WIN32 */
#ifdef WIN32
  GetModuleFileName (NULL, szBuf, MAX_PATH) ;
  pszModuleFName = g_strdup_printf ("%s", szBuf) ;
  GetShortPathName (pszModuleFName, szBuf, MAX_PATH) ;
  g_free (pszModuleFName) ;
  pszModuleFName = g_strdup_printf ("%s", szBuf) ;
  g_snprintf (szMyPath, MAX_PATH, "%s", pszModuleFName) ;
#ifdef QCAD_NO_CONSOLE
  if (pszCmdLine[0] != 0)
    psz = g_strdup_printf ("%s %s", pszModuleFName, pszCmdLine) ;
  else
    psz = g_strdup_printf ("%s", pszModuleFName) ;
  (*pargv) = (char **)CmdLineToArgv (psz, pargc) ;
  g_free (psz) ;
#endif /* def QCAD_NO_CONSOLE */
  g_free (pszModuleFName) ;

  for (Nix = strlen (szMyPath) ; Nix > -1 ; Nix--)
    if (G_DIR_SEPARATOR == szMyPath[Nix])
      {
      szMyPath[Nix] = 0 ;
      break ;
      }
  putenv (psz = g_strdup_printf ("MY_PATH=%s", szMyPath)) ;
  g_free (psz) ;
#endif

  gtk_init (pargc, pargv) ;

  // Set the locale
#ifdef ENABLE_NLS
#ifdef WIN32
  putenv (psz = g_strdup_printf ("LANG=%s", get_locale ())) ;
  g_free (psz) ;
  bindtextdomain (PACKAGE, psz = g_strdup_printf ("%s%slocale", szMyPath, G_DIR_SEPARATOR_S)) ;
  g_free (psz) ;
#else
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
#endif /* def WIN32 */
  textdomain (PACKAGE);
#endif
  bind_textdomain_codeset (PACKAGE, "UTF-8") ;
  gtk_set_locale ();

  // Add pixmap directories
#ifdef WIN32
  add_pixmap_directory (psz = g_strdup_printf ("%s%s..%sshare%s%s%spixmaps", szMyPath, G_DIR_SEPARATOR_S, G_DIR_SEPARATOR_S, G_DIR_SEPARATOR_S, PACKAGE, G_DIR_SEPARATOR_S)) ;
  g_free (psz) ;
  add_pixmap_directory (psz = g_strdup_printf ("%s\\..\\pixmaps", szMyPath)) ;
  g_free (psz) ;
#else /* ifndef WIN32 */
  // -- Pixmaps used by the buttons in the main window -- //
  add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");
  add_pixmap_directory (PACKAGE_SOURCE_DIR "/pixmaps");
#endif /* ifdef WIN32 */
// Done adding pixmap directories

#ifdef WIN32
#ifdef QCAD_NO_CONSOLE
  // Turn off logging by setting it to an empty function
  // This prevents a console from popping up when QCADesigner quits
  g_log_set_handler (NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, my_logger, NULL);
  g_log_set_handler ("Gtk", G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, my_logger, NULL);
  g_log_set_handler ("Gdk", G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, my_logger, NULL);
  g_log_set_handler ("GdkPixbuf", G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, my_logger, NULL);
  g_log_set_handler ("Pango", G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, my_logger, NULL);
#endif /* ifdef QCAD_NO_CONSOLE */
#endif /* ifdef WIN32 */

  set_window_icon (NULL, pszBaseName) ;

  gtk_stock_add (stock_layers, n_stock_layers) ;

  add_stock_icon ("drawing_layer.png",      QCAD_STOCK_DRAWING_LAYER) ;
  add_stock_icon ("clocks_layer.png",       QCAD_STOCK_CLOCKING_LAYER) ;
  add_stock_icon ("cells_layer.png",        QCAD_STOCK_CELL_LAYER) ;
  add_stock_icon ("substrate_layer.png",    QCAD_STOCK_SUBSTRATE_LAYER) ;
  add_stock_icon ("distribution_layer.png", QCAD_STOCK_DISTRIBUTION_LAYER) ;
#ifdef HAVE_LIBRSVG
  add_stock_icon ("substrate.svg",            QCAD_STOCK_SUBSTRATE) ;
  add_stock_icon ("rect_electrode.svg",       QCAD_STOCK_RECT_ELECTRODE) ;
  add_stock_icon ("label.svg",                QCAD_STOCK_LABEL) ;
  add_stock_icon ("reorder_layers.svg",       QCAD_STOCK_REORDER_LAYERS) ;
  add_stock_icon ("bus.svg",                  QCAD_STOCK_BUS) ;
  add_stock_icon ("clock.svg",                QCAD_STOCK_CLOCK) ;
  add_stock_icon ("bus_input.svg",            QCAD_STOCK_BUS_INPUT) ;
  add_stock_icon ("bus_output.svg",           QCAD_STOCK_BUS_OUTPUT) ;
  add_stock_icon ("cell_input.svg",           QCAD_STOCK_CELL_INPUT) ;
  add_stock_icon ("cell_output.svg",          QCAD_STOCK_CELL_OUTPUT) ;
  add_stock_icon ("graph_data_bin.svg",       QCAD_STOCK_GRAPH_BIN) ;
  add_stock_icon ("graph_data_hex.svg",       QCAD_STOCK_GRAPH_HEX) ;
  add_stock_icon ("graph_data_dec.svg",       QCAD_STOCK_GRAPH_DEC) ;
  add_stock_icon ("puzzle_piece_green.svg",   QCAD_STOCK_BLOCK_READ) ;
  add_stock_icon ("puzzle_piece_red.svg",     QCAD_STOCK_BLOCK_WRITE) ;
  add_stock_icon ("q_cell_def.svg",           QCAD_STOCK_CELL) ;
  add_stock_icon ("q_cell_array.svg",         QCAD_STOCK_ARRAY) ;
  add_stock_icon ("q_cell_rotate.svg",        QCAD_STOCK_ROTATE_CELL) ;
  add_stock_icon ("default.svg",              QCAD_STOCK_SELECT) ;
  add_stock_icon ("q_cell_alt.svg",           QCAD_STOCK_CELL_ALT_CROSSOVER) ;
  add_stock_icon ("q_cell_alt_circle.svg",    QCAD_STOCK_CELL_ALT_VERTICAL) ;
  add_stock_icon ("q_cell_copy.svg",          QCAD_STOCK_COPY) ;
  add_stock_icon ("q_cell_translate.svg",     QCAD_STOCK_TRANSLATE) ;
  add_stock_icon ("q_cell_mirror.svg",        QCAD_STOCK_MIRROR_VERTICAL) ;
  add_stock_icon ("q_cell_mirror_other.svg",  QCAD_STOCK_MIRROR_HORIZONTAL) ;
  add_stock_icon ("ruler.svg",                QCAD_STOCK_MEASURE) ;
  add_stock_icon ("insert_column_before.svg", QCAD_STOCK_INSERT_COL_BEFORE) ;
  add_stock_icon ("show_potential.svg",       QCAD_STOCK_SHOW_POTENTIAL) ;
  add_stock_icon ("no_show_potential.svg",    QCAD_STOCK_NO_SHOW_POTENTIAL) ;
#else
  add_stock_icon ("substrate.png",            QCAD_STOCK_SUBSTRATE) ;
  add_stock_icon ("rect_electrode.png",       QCAD_STOCK_RECT_ELECTRODE) ;
  add_stock_icon ("label.png",                QCAD_STOCK_LABEL) ;
  add_stock_icon ("reorder_layers.png",       QCAD_STOCK_REORDER_LAYERS) ;
  add_stock_icon ("bus.png",                  QCAD_STOCK_BUS) ;
  add_stock_icon ("clock.png",                QCAD_STOCK_CLOCK) ;
  add_stock_icon ("bus_input.png",            QCAD_STOCK_BUS_INPUT) ;
  add_stock_icon ("bus_output.png",           QCAD_STOCK_BUS_OUTPUT) ;
  add_stock_icon ("cell_input.png",           QCAD_STOCK_CELL_INPUT) ;
  add_stock_icon ("cell_output.png",          QCAD_STOCK_CELL_OUTPUT) ;
  add_stock_icon ("graph_data_bin.png",       QCAD_STOCK_GRAPH_BIN) ;
  add_stock_icon ("graph_data_hex.png",       QCAD_STOCK_GRAPH_HEX) ;
  add_stock_icon ("graph_data_dec.png",       QCAD_STOCK_GRAPH_DEC) ;
  add_stock_icon ("puzzle_piece_green.png",   QCAD_STOCK_BLOCK_READ) ;
  add_stock_icon ("puzzle_piece_red.png",     QCAD_STOCK_BLOCK_WRITE) ;
  add_stock_icon ("q_cell_def.png",           QCAD_STOCK_CELL) ;
  add_stock_icon ("q_cell_array.png",         QCAD_STOCK_ARRAY) ;
  add_stock_icon ("q_cell_rotate.png",        QCAD_STOCK_ROTATE_CELL) ;
  add_stock_icon ("default.png",              QCAD_STOCK_SELECT) ;
  add_stock_icon ("q_cell_alt.png",           QCAD_STOCK_CELL_ALT_CROSSOVER) ;
  add_stock_icon ("q_cell_alt_circle.png",    QCAD_STOCK_CELL_ALT_VERTICAL) ;
  add_stock_icon ("q_cell_move.png",          QCAD_STOCK_COPY) ;
  add_stock_icon ("q_cell_move.png",          QCAD_STOCK_TRANSLATE) ;
  add_stock_icon ("q_cell_mirror.png",        QCAD_STOCK_MIRROR_VERTICAL) ;
  add_stock_icon ("q_cell_mirror_other.png",  QCAD_STOCK_MIRROR_HORIZONTAL) ;
  add_stock_icon ("ruler.png",                QCAD_STOCK_MEASURE) ;
  add_stock_icon ("insert_column_before.png", QCAD_STOCK_INSERT_COL_BEFORE) ;
#endif
  }

#ifdef QCAD_NO_CONSOLE
static void my_logger (const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
  {
  // Handle log messages here
  // This logger ignores all messages, so as not to produce a console window
  }
#endif /* ifdef QCAD_NO_CONSOLE */

#ifdef WIN32
#ifdef QCAD_NO_CONSOLE
// Turn a string into an argv-style array
char **CmdLineToArgv (char *pszTmp, int *pargc)
  {
  char **argv = NULL, *psz = g_strdup_printf ("%s", pszTmp), *pszAt = psz, *pszStart = psz ;
  gboolean bString = FALSE ;

  (*pargc) = 0 ;

  for (pszAt = psz ; ; pszAt++)
    {
    if (0 == (*pszAt)) break ;
    if (' ' == (*pszAt))
      {
      if (!bString)
        {
        (*pszAt) = 0 ;
        argv = g_realloc (argv, ++(*pargc) * sizeof (char *)) ;
        argv[(*pargc) - 1] = g_strdup_printf ("%s", pszStart) ;
        pszAt++ ;
        while (' ' == (*pszAt))
          pszAt++ ;
        pszStart = pszAt ;
        }
      }

    if ('\"' == (*pszAt))
      {
      if (!bString)
        pszStart = pszAt = pszAt + 1 ;
      else
        {
        (*pszAt) = 0 ;
        argv = g_realloc (argv, ++(*pargc) * sizeof (char *)) ;
        argv[(*pargc) - 1] = g_strdup_printf ("%s", pszStart) ;
        pszAt++ ;
        while (' ' == (*pszAt))
          pszAt++ ;
        pszStart = pszAt ;
        }
      bString = !bString ;
      }
    }

  argv = g_realloc (argv, ++(*pargc) * sizeof (char *)) ;
  argv[(*pargc) - 1] = g_strdup_printf ("%s", pszStart) ;
  argv = g_realloc (argv, ++(*pargc) * sizeof (char *)) ;
  argv[(*pargc) - 1] = NULL ;

  (*pargc)-- ;

  g_free (psz) ;
  return argv ;
  }
#endif /* QCAD_NO_CONSOLE */

static char *get_locale ()
  {
  HKEY hk ;
  char *pszLocale = NULL, szVal[32] = "" ;
  DWORD cbVal = 31 ;
  DWORD dwType = REG_SZ ;

  if (ERROR_SUCCESS == RegOpenKeyEx (HKEY_CURRENT_USER, "Software\\" PACKAGE, 0, KEY_QUERY_VALUE, &hk))
    if (ERROR_SUCCESS == RegQueryValueEx (hk, "Installer Language", NULL, &dwType, szVal, &cbVal))
      {
      pszLocale = lcid_to_posix_locale (atoi (szVal)) ;
      RegCloseKey (hk) ;
      }

  if (NULL == pszLocale)
    pszLocale = lcid_to_posix_locale (GetUserDefaultLCID ()) ;

  return pszLocale ;
  }

static char *lcid_to_posix_locale (int lcid)
  {
  switch (lcid)
    {
//    case 1026: return "bg"; /* bulgarian */
//    case 1027: return "ca"; /* catalan */
//    case 1050: return "hr"; /* croation */
//    case 1029: return "cs"; /* czech */
//    case 1030: return "da"; /* danish */
//    case 1043: return "nl"; /* dutch - netherlands */
    case 1033: return "en_US"; /* english - us */
//    case 1035: return "fi"; /* finnish */
    case 1036: return "fr_FR"; /* french - france */
    case 1031: return "de_DE"; /* german - germany */
//    case 1032: return "el"; /* greek */
//    case 1037: return "he"; /* hebrew */
    case 1038: return "hu_HU"; /* hungarian */
//    case 1040: return "it"; /* italian - italy */
//    case 1041: return "ja"; /* japanese */
//    case 1042: return "ko"; /* korean */
//    case 1063: return "lt"; /* lithuanian */
//    case 1071: return "mk"; /* macedonian */
    case 1045: return "pl_PL"; /* polish */
//    case 2070: return "pt"; /* portuguese - portugal */
//    case 1046: return "pt_BR"; /* portuguese - brazil */
    case 1048: return "ro_RO"; /* romanian - romania */
//    case 1049: return "ru"; /* russian - russia */
//    case 2074: return "sr@Latn"; /* serbian - latin */
//    case 3098: return "sr"; /* serbian - cyrillic */
//    case 2052: return "zh_CN"; /* chinese - china (simple) */
//    case 1051: return "sk"; /* slovak */
//    case 1060: return "sl"; /* slovenian */
//    case 1034: return "es"; /* spanish */
//    case 1052: return "sq"; /* albanian */
//    case 1053: return "sv"; /* swedish */
//    case 1054: return "th"; /* thai */
//    case 1028: return "zh_TW"; /* chinese - taiwan (traditional) */
//    case 1055: return "tr"; /* turkish */
//    case 1058: return "uk"; /* ukrainian */
    } ;
  return "C" ;
  }
#endif /* ifdef WIN32 */

#endif /* def GTK_GUI */
