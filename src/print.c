//////////////////////////////////////////////////////////
// QCADesigner                                          //
// Copyright 2002 Konrad Walus                          //
// All Rights Reserved                                  //
// Author: Konrad Walus                                 //
// Email: walus@atips.ca                                //
// **** Please use complete names in variables and      //
// **** functions. This will reduce ramp up time for new//
// **** people trying to contribute to the project.     //
//////////////////////////////////////////////////////////
// This file written by Gabriel Schulhof                //
// (schulhof@vlsi.enel.ucalgary.edu).  It implements    //
// a PostScript printer for QCADesigner.                //
// Completion Date: June 2003                           //
//////////////////////////////////////////////////////////

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "print.h"
#include "print_util.h"
#include "cad.h"
#include "stdqcell.h"

#define DBG_P(s)

static void AddQCellToPage (GQCell *pqc, GQCell ***pppPage, int *picOnPage) ;
static void GetPrintRange (GQCell *first_cell, print_design_OP *pPO, double *pdxMin, double *pdyMin, double *pdxMax, double *pdyMax) ;
static void PrintPages (print_design_OP *pPO, GQCell ***pppqcPages, int *pcCellsOnPage, int icCells,
  double dxMinNm, double dyMinNm, double dxMaxNm, double dyMaxNm, double dxDiffMinNm, double dyDiffMinNm) ;
static void PrintProlog (FILE *pfile, print_design_OP *pPO, double dxMinNm, double dyMinNm, double dxDiffMinNm,
  double dyDiffMinNm, double dCYPageNm) ;
static void PlaceQCellsOnPage (GQCell *first_cell, GQCell ***pppqcPages, int *pcCellsOnPage, int *picCells, double dPageWidthNm,
  double dPageHeightNm, int iCXPages, int iCYPages, double dXOffsetNm, double dYOffsetNm, gboolean bRowMajor) ;
static void PrintSinglePage (FILE *pfile, print_design_OP *pPO, GQCell ***pppqcPage, int *picQC,
  double dCXPageNm, double dCYPageNm, int idx, int idxX, int idxY, double dXMedian, double dYMedian) ;
static void PrintSingleCell (FILE *pfile, GQCell *pqc, double xOffset, double yOffset, double dXMedian,
  double dYMedian, gboolean bColour) ;
static void PrintCellColour (FILE *pfile, GQCell *pqc, gboolean bPrintColour) ;
static void PrintCellSides (FILE *pfile, GQCell *pqc, double xOffset, double yOffset) ;
static void PrintCellLabel (FILE *pfile, GQCell *pqc, double dXMedian, double dYMedian, double xOffset, double yOffset) ;

extern double subs_width ;
extern double subs_height ;

void print_world (print_design_OP *pPO, GQCell *first_cell)
  {
  double 
    dxMinNm = 0, dyMinNm = 0, dxMaxNm = 0, dyMaxNm = 0, dxDiffMinNm = 0, dyDiffMinNm = 0,
    dEffPageCXPts = pPO->po.dPaperCX - pPO->po.dLMargin - pPO->po.dRMargin,
    dEffPageCYPts = pPO->po.dPaperCY - pPO->po.dTMargin - pPO->po.dBMargin ;
  GQCell ***pppqcPages = NULL ;
  int *pcCellsOnPage = NULL ;
  int iPages = 0 ;
  int icCells = 0 ;
  
  GetPrintRange (first_cell, pPO, &dxMinNm, &dyMinNm, &dxMaxNm, &dyMaxNm) ;
  
  if (pPO->bCenter)
    {
    double dWidthPoints = (dxMaxNm - dxMinNm) * pPO->dPointsPerNano, 
           dHeightPoints = (dyMaxNm - dyMinNm) * pPO->dPointsPerNano ;
    dxDiffMinNm = (((dEffPageCXPts * pPO->iCXPages) - dWidthPoints) / 2) / pPO->dPointsPerNano ;
    dyDiffMinNm = (((dEffPageCYPts * pPO->iCYPages) - dHeightPoints) / 2) / pPO->dPointsPerNano ;
    }
  
  iPages = pPO->iCXPages * pPO->iCYPages ;
  
  if (pPO->pbPrintedObjs[PRINTED_OBJECTS_CELLS])
    {
    int Nix ;
    pppqcPages = malloc (iPages * sizeof (GQCell *)) ;
    pcCellsOnPage = malloc (iPages * sizeof (int)) ;
    
    for (Nix = 0 ; Nix < iPages ; Nix++)
      {
      pppqcPages[Nix] = NULL ;
      pcCellsOnPage[Nix] = 0 ;
      }
    
    PlaceQCellsOnPage (first_cell, pppqcPages, pcCellsOnPage, &icCells, dEffPageCXPts / pPO->dPointsPerNano, 
      dEffPageCYPts / pPO->dPointsPerNano, pPO->iCXPages, pPO->iCYPages, dxMinNm - dxDiffMinNm, 
      dyMinNm - dyDiffMinNm, pPO->bPrintOrderOver) ;
    }
  
  PrintPages (pPO, pppqcPages, pcCellsOnPage, icCells, dxMinNm, dyMinNm, dxMaxNm, dyMaxNm, dxDiffMinNm, dyDiffMinNm) ;
  }

static void PlaceQCellsOnPage (GQCell *first_cell, GQCell ***pppqcPages, int *pcCellsOnPage, int *picCells, double dPageWidthNm, 
  double dPageHeightNm, int iCXPages, int iCYPages, double dXOffsetNm, double dYOffsetNm, gboolean bRowMajor)
  {
  GQCell *pqc = NULL ;
  int Nix, Nix1 ;
  int idxX1 = -1, idxY1 = -1, idxX2 = -1, idxY2 = 1, idx = -1 ;

  double dCellX1 = -1, dCellY1 = -1, dCellX2 = -1, dCellY2 = -1 ;
  
  *picCells = 0 ;
  
  for (pqc = first_cell ; NULL != pqc ; pqc = pqc->next)
    {
    // This is adding to the pointer and then dereferencing it
    // I suspect you need paranthesis around the *picCells
    *picCells++ ;
    dCellX1 = pqc->x - (pqc->cell_width / 2) ;
    dCellY1 = pqc->y - (pqc->cell_height / 2) ;
    dCellX2 = pqc->x + (pqc->cell_width / 2) ;
    dCellY2 = pqc->y + (pqc->cell_height / 2) ;
    
    idxX1 = (int) ((dCellX1 - dXOffsetNm) / dPageWidthNm) ;
    idxX1 = idxX1 < 0 ? 0 : idxX1 >= iCXPages ? iCXPages - 1 : idxX1 ; /* Paranoia check */
    idxY1 = (int) ((dCellY1 - dYOffsetNm) / dPageHeightNm) ;
    idxY1 = idxY1 < 0 ? 0 : idxY1 >= iCYPages ? iCYPages - 1 : idxY1 ; /* Paranoia check */
    idxX2 = (int) ((dCellX2 - dXOffsetNm) / dPageWidthNm) ;
    idxX2 = idxX2 < 0 ? 0 : idxX2 >= iCXPages ? iCXPages - 1 : idxX2 ; /* Paranoia check */
    idxY2 = (int) ((dCellY2 - dYOffsetNm) / dPageHeightNm) ;
    idxY2 = idxY2 < 0 ? 0 : idxY2 >= iCYPages ? iCYPages - 1 : idxY2 ; /* Paranoia check */
    
    /* Add the cell to every page it appears on */
    for (Nix = idxX1 ; Nix <= idxX2 ; Nix++)
      for (Nix1 = idxY1 ; Nix1 <= idxY2 ; Nix1++)
        {
	idx = bRowMajor ? Nix1 * iCXPages + Nix : Nix * iCYPages + Nix1 ;
	AddQCellToPage (pqc, &pppqcPages[idx], &pcCellsOnPage[idx]) ;
	}
    }
  }

static void AddQCellToPage (GQCell *pqc, GQCell ***pppPage, int *picOnPage)
  {
  (*pppPage) = realloc ((*pppPage), ++(*picOnPage) * sizeof (GQCell *)) ;
  (*pppPage)[(*picOnPage) - 1] = pqc ;
  }

static void GetPrintRange (GQCell *first_cell, print_design_OP *pPO, double *pdxMin, double *pdyMin, double *pdxMax, double *pdyMax)
  {
  *pdxMin = *pdyMin = *pdxMax = *pdyMax = 0 ;
  
  if (pPO->pbPrintedObjs[PRINTED_OBJECTS_CELLS])
    get_extents (first_cell, pdxMin, pdyMin, pdxMax, pdyMax) ;
  
  DBG_P (fprintf (stderr, "get_extents returned (%lf,%lf)->(%lf,%lf)\n", (*pdxMin), (*pdyMin), (*pdxMax), (*pdyMax))) ;

  if (pPO->pbPrintedObjs[PRINTED_OBJECTS_DIE])
    {
    *pdxMin = *pdxMin > 0 ? 0 : *pdxMin ;
    *pdyMin = *pdyMin > 0 ? 0 : *pdyMin ;
    *pdxMax = *pdxMax < subs_width ? subs_width : *pdxMax ;
    *pdyMax = *pdyMax < subs_height ? subs_height : *pdyMax ;
    }
  }

static void PrintPages (print_design_OP *pPO, GQCell ***pppqcPages, int *pcCellsOnPage, int icCells,
  double dxMinNm, double dyMinNm, double dxMaxNm, double dyMaxNm, double dxDiffMinNm, double dyDiffMinNm)
  {
  FILE *pfile = NULL ;
  int Nix[2] = {0, 0}, limit[2] = {pPO->iCXPages, pPO->iCYPages}, inner = -1, outer = -1, idx = 0 ;
  double dPageWidthNm =  (pPO->po.dPaperCX - pPO->po.dLMargin - pPO->po.dRMargin) / pPO->dPointsPerNano,
         dPageHeightNm = (pPO->po.dPaperCY - pPO->po.dTMargin - pPO->po.dBMargin) / pPO->dPointsPerNano,
	 dXMedian = 0, dYMedian = 0 ;
  
  inner = pPO->bPrintOrderOver ? 0 : 1 ;
  outer = pPO->bPrintOrderOver ? 1 : 0 ;
  
  if (NULL == (pfile = OpenPrintStream ((print_OP *)pPO)))
    {
    fprintf (stderr, "Failed to open file/command for writing.\n") ;
    return ;
    }

  fprintf (pfile,
    "%%!PS-Adobe 3.0\n"
    "%%%%Pages: (atend)\n"
    "%%%%BoundingBox: 0 0 %d %d\n"
    "%%%%HiResBoundingBox: %f %f %f %f\n"
    "%%........................................................\n"
    "%%%%Creator: QCADesigner\n"
    "%%%%EndComments\n",
    (int)(pPO->po.dPaperCX), (int)(pPO->po.dPaperCY),
    0.0, 0.0, pPO->po.dPaperCX, pPO->po.dPaperCY) ;
  
  PrintProlog (pfile, pPO, dxMinNm, dyMinNm, dxDiffMinNm, dyDiffMinNm, dPageHeightNm) ;
  
  dXMedian = (dxMaxNm + dxMinNm) / 2 ;
  dYMedian = (dyMaxNm + dyMinNm) / 2 ;
  
  for (Nix[0] = 0 ; Nix[0] < limit[outer] ; Nix[0]++)
    for (Nix[1] = 0 ; Nix[1] < limit[inner] ; Nix[1]++)
      {
      PrintSinglePage (pfile, pPO, pppqcPages, pcCellsOnPage, dPageWidthNm, dPageHeightNm, idx, 
        Nix[outer], Nix[inner], dXMedian, dYMedian) ;
      idx++ ;
      }

  fprintf (pfile,
    "%%%%Trailer\n"
    "%%%%Pages: %d\n"
    "%%%%EOF\n",
    pPO->iCXPages * pPO->iCYPages) ;

  if (pPO->po.bPrintFile)
    fclose (pfile) ;
  else
    pclose (pfile) ;
  }

static void PrintProlog (FILE *pfile, print_design_OP *pPO, double dxMinNm, double dyMinNm, double dxDiffMinNm,
  double dyDiffMinNm, double dCYPageNm)
  {
  fprintf (pfile,
    "%%%%BeginProlog\n"
    "/nm { %f mul } def\n"
    "/nmx { %f sub %f mul %f add %f add} def\n"
    "/nmy { %f sub %f sub -1 mul %f mul %f add %f sub } def\n"
    "/labelfontsize 12 nm def\n"
    "/txtlt { gsave dup 0 -1 labelfontsize mul rmoveto show grestore } def\n"
    "/txtlm { gsave dup 0 labelfontsize 2 div -1 mul rmoveto show grestore } def\n"
    "/txtlb { gsave dup 0 0 rmoveto show grestore } def\n"
    "/txtct { gsave dup stringwidth exch 2 div -1 mul exch pop labelfontsize -1 mul rmoveto show grestore } def\n"
    "/txtcm { gsave dup stringwidth exch 2 div -1 mul exch pop labelfontsize 2 div -1 mul rmoveto show grestore } def\n"
    "/txtcb { gsave dup stringwidth pop 2 div -1 mul 0 rmoveto show grestore } def\n"
    "/txtrt { gsave dup stringwidth exch -1 mul exch pop labelfontsize -1 mul rmoveto show grestore } def\n"
    "/txtrm { gsave dup stringwidth exch -1 mul exch pop labelfontsize 2 div -1 mul rmoveto show grestore } def\n"
    "/txtrt { gsave dup stringwidth exch -1 mul exch pop 0 rmoveto show grestore } def\n"
    "%%%%EndProlog\n",
    pPO->dPointsPerNano,
    dxMinNm, pPO->dPointsPerNano, pPO->po.dLMargin, dxDiffMinNm * pPO->dPointsPerNano,
    dCYPageNm, dyMinNm, pPO->dPointsPerNano, pPO->po.dTMargin, dyDiffMinNm * pPO->dPointsPerNano) ;
  }

static void PrintSinglePage (FILE *pfile, print_design_OP *pPO, GQCell ***pppqcPage, int *picQC, double dCXPageNm,
  double dCYPageNm, int idx, int idxX, int idxY, double dXMedian, double dYMedian)
  {
  int Nix ;
  double xOffset = dCXPageNm * idxX,
         yOffset = dCYPageNm * idxY ;
  
  fprintf (pfile,
    "%%%%Page: %d %d\n"
    "gsave\n"
    "newpath\n" /* The margins */
    "%f %f moveto\n"
    "%f %f lineto\n"
    "%f %f lineto\n"
    "%f %f lineto\n"
    "closepath eoclip\n\n",
    idx + 1, idx + 1,
    pPO->po.dLMargin, pPO->po.dBMargin,
    pPO->po.dLMargin, pPO->po.dPaperCY - pPO->po.dTMargin,
    pPO->po.dPaperCX - pPO->po.dRMargin, pPO->po.dPaperCY - pPO->po.dTMargin,
    pPO->po.dPaperCX - pPO->po.dRMargin, pPO->po.dBMargin) ;

/*
  fprintf (pfile,
    "newpath\n"
    "%lf nmx %lf nmy 12 sub moveto\n"
    "0 24 rlineto\n"
    "%lf nmx 12 sub %lf nmy moveto\n"
    "24 0 rlineto\n"
    "stroke\n\n",
    dXMedian - xOffset, dYMedian - yOffset,
    dXMedian - xOffset, dYMedian - yOffset) ;
*/
  if (pPO->pbPrintedObjs[PRINTED_OBJECTS_CELLS])
    for (Nix = 0 ; Nix < picQC[idx] ; Nix++)
      PrintSingleCell (pfile, pppqcPage[idx][Nix], xOffset, yOffset, dXMedian, dYMedian,
        pPO->pbPrintedObjs[PRINTED_OBJECTS_COLOURS]) ;
  if (pPO->pbPrintedObjs[PRINTED_OBJECTS_DIE])
    {
    fprintf (pfile,
      "gsave\n"
      "2 setlinewidth\n"
      "newpath\n"
      "%f nmx %f nmy moveto\n"
      "%f nmx %f nmy lineto\n"
      "%f nmx %f nmy lineto\n"
      "%f nmx %f nmy lineto\n"
      "%f nmx %f nmy lineto\n"
      "stroke\n"
      "grestore\n",
      0.0 - xOffset, 0.0 - yOffset,
      subs_width - xOffset, 0.0 - yOffset,
      subs_width - xOffset, subs_height - yOffset,
      0.0 - xOffset, subs_height - yOffset,
      0.0 - xOffset, 0.0 - yOffset) ;
    }

  fprintf (pfile,
    "grestore\n"
    "showpage\n"
    "%%%%PageTrailer\n") ;
  }

void PrintCellCorner (FILE *pfile, GQCell *pqc, double xOffset, double yOffset, double dArcOffset, int iCorner)
  {
  if (0 == dArcOffset) return ;

  if (pqc->is_input)
    fprintf (pfile, "%f nmx %f nmy %f nm %d %d arc\n",
      pqc->x + (iCorner < 1 || iCorner > 2 ? (-1) : (1)) * (pqc->cell_width * 0.5) - xOffset,
      pqc->y + (iCorner < 2 ? (-1) : (1)) * (pqc->cell_height * 0.5) - yOffset, dArcOffset,
      270 - iCorner * 90, (360 - iCorner * 90) % 360) ;
  else if (pqc->is_output)
    {
    fprintf (pfile, "%f nmx %f nmy %f nm %d %d arcn\n",
      pqc->x + (iCorner < 1 || iCorner > 2 ? (-1) : (1)) * (pqc->cell_width * 0.5) - xOffset +
        dArcOffset * (iCorner >= 1 && iCorner <= 2 ? (-1) : (1)),
      pqc->y + (iCorner < 2 ? (-1) : (1)) * (pqc->cell_height * 0.5) - yOffset +
        dArcOffset * (iCorner >= 2 ? (-1) : (1)), dArcOffset,
      (540 - iCorner * 90) % 360, (450 - iCorner * 90) % 360) ;
    }
  else if (pqc->is_fixed)
    fprintf (pfile, "%f %f lineto\n",
      pqc->x + (iCorner < 1 || iCorner > 2 ? (-1) : (1)) * (pqc->cell_width * 0.5) - xOffset +
        dArcOffset * (iCorner < 2 ? (1) : (-1)),
      pqc->y + (iCorner < 2 ? (-1) : (1)) * (pqc->cell_height * 0.5) - yOffset +
      dArcOffset * (iCorner >= 1 && iCorner <= 2 ? (1) : (-1))) ;
  }

static void PrintCellSides (FILE *pfile, GQCell *pqc, double xOffset, double yOffset)
  {
  double dArcOffset = 0 ;

//  dArcOffset = pqc->is_input || pqc->is_output || pqc->is_fixed ? 3 : 0 ;

  fprintf (pfile, "%f nmx %f nmy moveto\n",
    (pqc->x - (pqc->cell_width * 0.5)) - xOffset + dArcOffset,
    (pqc->y - (pqc->cell_height * 0.5)) - yOffset) ;
  fprintf (pfile, "%f nmx %f nmy lineto\n",
    (pqc->x + (pqc->cell_width * 0.5)) - xOffset - 2 * dArcOffset,
    (pqc->y - (pqc->cell_height * 0.5)) - yOffset) ;
//  PrintCellCorner (pfile, pqc, xOffset, yOffset, dArcOffset, 1) ;
  fprintf (pfile, "%f nmx %f nmy lineto\n",
    (pqc->x + (pqc->cell_width * 0.5)) - xOffset,
    (pqc->y + (pqc->cell_height * 0.5)) - yOffset - 2 * dArcOffset) ;
//  PrintCellCorner (pfile, pqc, xOffset, yOffset, dArcOffset, 2) ;
  fprintf (pfile, "%f nmx %f nmy lineto\n",
    (pqc->x - (pqc->cell_width * 0.5)) - xOffset + dArcOffset,
    (pqc->y + (pqc->cell_height * 0.5)) - yOffset) ;
//  PrintCellCorner (pfile, pqc, xOffset, yOffset, dArcOffset, 3) ;
  fprintf (pfile, "%f nmx %f nmy lineto\n",
    (pqc->x - (pqc->cell_width * 0.5)) - xOffset,
    (pqc->y - (pqc->cell_height * 0.5)) - yOffset + dArcOffset) ;
//  PrintCellCorner (pfile, pqc, xOffset, yOffset, dArcOffset, 0) ;
  }

static void PrintSingleCell (FILE *pfile, GQCell *pqc, double xOffset, double yOffset, double dXMedian,
  double dYMedian, gboolean bColour)
  {
  int Nix ;

  fprintf (pfile, "%%Begin Cell\n") ;

  /* Cell outline */
  fprintf (pfile, "newpath\n") ;
  PrintCellSides (pfile, pqc, xOffset, yOffset) ;
  fprintf (pfile, "stroke\n") ;

  /* Cell shading */
  fprintf (pfile,
    "gsave\n"
    "newpath\n") ;
  PrintCellSides (pfile, pqc, xOffset, yOffset) ;

  for (Nix = 0 ; Nix < pqc->number_of_dots ; Nix++)
    {
    fprintf (pfile,
      "%f nmx %f nmy moveto\n"
      "%f nmx %f nmy %f nm 0 360 arc\n",
      pqc->cell_dots[Nix].x + pqc->cell_dots[Nix].diameter / 2.0 - xOffset,
      pqc->cell_dots[Nix].y - yOffset,
      pqc->cell_dots[Nix].x - xOffset, pqc->cell_dots[Nix].y - yOffset, pqc->cell_dots[Nix].diameter / 2.0) ;
    }
  PrintCellColour (pfile, pqc, bColour) ;
  fprintf (pfile,
    "fill\n"
    "grestore\n") ;
  PrintCellLabel (pfile, pqc, dXMedian, dYMedian, xOffset, yOffset) ;

  for (Nix = 0 ; Nix < pqc->number_of_dots ; Nix++)
    {
    fprintf (pfile,
      "newpath\n" /* dot outline */
      "%f nmx %f nmy %f nm 0 360 arc\n"
      "closepath stroke\n\n"
      "newpath\n" /* dot charge */
      "%f nmx %f nmy %f nm 0 360 arc\n"
      "closepath fill\n\n",
      pqc->cell_dots[Nix].x - xOffset, pqc->cell_dots[Nix].y - yOffset, pqc->cell_dots[Nix].diameter / 2.0,
      pqc->cell_dots[Nix].x - xOffset, pqc->cell_dots[Nix].y - yOffset,
      (pqc->cell_dots[Nix].diameter * pqc->cell_dots[Nix].charge / QCHARGE) / 2.0) ;
    }
  fprintf (pfile, "%%End Cell\n") ;
  }

static void PrintCellColour (FILE *pfile, GQCell *pqc, gboolean bPrintColour)
  {
  float clr[4][3] = {
    {0.0, 0.5, 0.0}, /* dark green */
    {0.5, 0.0, 0.5}, /* dark purple */
    {0.0, 0.5, 0.5}, /* turquoise */
    {1.0, 1.0, 1.0}  /* white */
    } ;
  float gray[4] = {0.45, 0.65, 0.85, 1.00} ; /* equivalent gray values in lieu of colours */

  if (!bPrintColour)
    fprintf (pfile, "%f setgray fill\n", gray[pqc->clock]) ;
  else
    {
    if (pqc->is_fixed)
      fprintf (pfile, "%s setrgbcolor\n", PS_ORANGE) ; /* dark orange */
    else if (pqc->is_input)
      fprintf (pfile, "%s setrgbcolor\n", PS_BLUE) ; /* dark azure blue */
    else if (pqc->is_output)
      fprintf (pfile, "%s setrgbcolor\n", PS_YELLOW) ; /* maroonish yellow */
    else
      fprintf (pfile, "%f %f %f setrgbcolor\n", clr[pqc->clock][0], clr[pqc->clock][1], clr[pqc->clock][2]) ;
    }
  }

static void PrintCellLabel (FILE *pfile, GQCell *pqc, double dXMedian, double dYMedian, double xOffset, double yOffset)
  {
  char szLabel[256] = "" ;

  if (!(pqc->is_fixed || pqc->is_input || pqc->is_output)) return ;

  fprintf (pfile, "(Courier) findfont labelfontsize scalefont setfont\n") ;

  if (pqc->is_fixed)
    g_snprintf (szLabel, 256, "(%1.2f)", gqcell_calculate_polarization (pqc)) ;
  else
    g_snprintf (szLabel, 256, "(%s)", pqc->label) ;

  if (fabs (pqc->x - dXMedian) > fabs (pqc->y - dYMedian))
    {
    fprintf (pfile,
      "%f nmx %f nmy moveto\n"
      "%s %s\n",
      pqc->x + (2 + pqc->cell_width / 2) * (dXMedian > pqc->x ? -1 : 1) - xOffset, pqc->y - yOffset,
      szLabel, (dXMedian > pqc->x ? "txtrm" : "txtlm")) ;
    }
  else
    {
    fprintf (pfile,
      "%f nmx %f nmy 2 add moveto\n"
      "%s %s\n",
      pqc->x - xOffset, pqc->y + (2 + pqc->cell_height / 2) * (dYMedian > pqc->y ? -1 : 1) - yOffset,
      szLabel, (dYMedian > pqc->y ? "txtcb" : "txtct")) ;
    }
  }
