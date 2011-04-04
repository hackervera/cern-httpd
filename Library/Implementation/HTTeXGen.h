/*                                                                           Latex Generation
                                     LATEX GENERATOR
                                             
   This module converts a structured stream from HTML into LaTeX format. The conversion is
   mostly a 1:1 translation, but as the LaTeX compiler is much more strict than a typical
   HTML converter some typographical constraints are put on the translation. Only text is
   translated for the moment. The module is a part of the CERN Common WWW Library.
   
 */

#ifndef HTTEXGEN_H
#define HTTEXGEN_H

#include "HTML.h"
#include "HTStream.h"
/*

Conversion Module

   The conversion module is defined as
   
 */

extern HTStructured * HTTeXGenerator PARAMS((HTStream * output));
/*

 */

#endif
/*

   End of module HTTeXGen  */
