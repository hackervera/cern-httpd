/*		Configuration-specific Initialialization	HTSInit.c
**		----------------------------------------
** This file is for a server.
**
** History:
**
**	   Nov 93  NjH	Added some CAD-relevant suffixes.
**	   Sep 93  MD	Added some typical vms suffixes.
**		
*/

/* Implements: */

#include "HTInit.h"


/*	Define a basic set of suffixes and presentations
**	------------------------------------------------
*/

#include "HTFormat.h"
#include "HTAccess.h"
#include "HTML.h"	/* @@@@ I wonder why these two */
#include "HTPlain.h"	/* @@@@ were commented out...? */

#include "HTList.h"
#include "HTMLGen.h"
#include "HTFile.h"
#include "HTFormat.h"

#include "HTMIME.h"
#include "HTWSRC.h"
#include "HTFWriter.h"

PUBLIC void HTFormatInit ARGS1(HTList *,c)
{
    HTSetConversion(c, "www/mime",			"*",		HTMIMEConvert,	1.0, 0.0, 0.0);
    HTSetConversion(c, "application/x-wais-source",	"*",		HTWSRCConvert,	1.0, 0.0, 0.0);
    HTSetConversion(c, "text/plain",			"text/html",	HTPlainToHTML,	1.0, 0.0, 0.0);
}



/*	Define a basic set of suffixes
**	------------------------------
**
**	The first suffix for a type is that used for temporary files
**	of that type.
*/

#ifndef NO_INIT
PUBLIC void HTFileInit NOARGS
{
    
    HTAddType(".mime",   "www/mime",			"binary", 1.0);	/* Internal -- MIME is	*/
                                                                        /* not recursive	*/

    HTAddType(".bin",    "application/octet-stream",	"binary", 1.0); /* Uninterpreted binary	*/
    HTAddType(".oda",    "application/oda",		"binary", 1.0);
    HTAddType(".pdf",    "application/pdf",		"binary", 1.0);
    HTAddType(".ai",     "application/postscript",	"8bit",   0.5);	/* Adobe Illustrator	*/
    HTAddType(".PS",     "application/postscript",	"8bit",	  0.8);	/* PostScript		*/
    HTAddType(".eps",    "application/postscript",	"8bit",   0.8);
    HTAddType(".ps",     "application/postscript",	"8bit",   0.8);
    HTAddType(".rtf",    "application/x-rtf",		"7bit",   1.0);	/* RTF			*/
    HTAddType(".csh",    "application/x-csh",		"7bit",   0.5);	/* C-shell script	*/
    HTAddType(".dvi",    "application/x-dvi",		"binary", 1.0);	/* TeX DVI		*/
    HTAddType(".hdf",    "application/x-hdf",		"binary", 1.0);	/* NCSA HDF data file	*/
    HTAddType(".latex",  "application/x-latex",		"8bit",   1.0);	/* LaTeX source		*/
    HTAddType(".nc",     "application/x-netcdf",	"binary", 1.0);	/* Unidata netCDF data	*/
    HTAddType(".cdf",    "application/x-netcdf",	"binary", 1.0);
    HTAddType(".sh",     "application/x-sh",		"7bit",   0.5);	/* Shell-script		*/
    HTAddType(".tcl",    "application/x-tcl",		"7bit",   0.5);	/* TCL-script		*/
    HTAddType(".tex",    "application/x-tex",		"8bit",   1.0);	/* TeX source		*/
    HTAddType(".texi",   "application/x-texinfo",	"7bit",   1.0);	/* Texinfo		*/
    HTAddType(".texinfo","application/x-texinfo",	"7bit",   1.0);
    HTAddType(".t",      "application/x-troff",		"7bit",   0.5);	/* Troff		*/
    HTAddType(".roff",   "application/x-troff",		"7bit",   0.5);
    HTAddType(".tr",     "application/x-troff",		"7bit",   0.5);
    HTAddType(".man",    "application/x-troff-man",	"7bit",   0.5);	/* Troff with man macros*/
    HTAddType(".me",     "application/x-troff-me",	"7bit",   0.5);	/* Troff with me macros	*/
    HTAddType(".ms",     "application/x-troff-ms",	"7bit",   0.5);	/* Troff with ms macros	*/
    HTAddType(".src",    "application/x-wais-source",	"7bit",   1.0);	/* WAIS source		*/
    HTAddType(".bcpio",  "application/x-bcpio",		"binary", 1.0);	/* Old binary CPIO	*/
    HTAddType(".cpio",   "application/x-cpio",		"binary", 1.0);	/* POSIX CPIO		*/
    HTAddType(".gtar",   "application/x-gtar",		"binary", 1.0);	/* Gnu tar		*/
    HTAddType(".shar",   "application/x-shar",		"8bit",   1.0);	/* Shell archive	*/
    HTAddType(".sv4cpio","application/x-sv4cpio",	"binary", 1.0);	/* SVR4 CPIO		*/
    HTAddType(".sv4crc", "application/x-sv4crc",	"binary", 1.0);	/* SVR4 CPIO with CRC	*/

    /*
    ** The following are neutral CAE formats:
    */
    HTAddType(".igs",	"application/iges",		"binary", 1.0); /* IGES Graphics format */
    HTAddType(".iges",	"application/iges",		"binary", 1.0); /* IGES Graphics format */
    HTAddType(".IGS",	"application/iges",		"binary", 1.0); /* IGES Graphics format */    
    HTAddType(".IGES",	"application/iges",		"binary", 1.0); /* IGES Graphics format */ 
    HTAddType(".stp",	"application/STEP",		"8bit"	, 1.0); /* ISO-10303 STEP  -    */ 
    HTAddType(".STP",	"application/STEP",		"8bit"  , 1.0); /* Product data files   */
    HTAddType(".step",	"application/STEP",		"8bit"  , 1.0); 
    HTAddType(".STEP",	"application/STEP",		"8bit"  , 1.0); 
    HTAddType(".dxf",	"application/dxf",		"binary", 1.0); /* DXF (AUTODESK)	*/
    HTAddType(".DXF",	"application/dxf",		"binary", 1.0); 
    HTAddType(".vda",	"application/vda",		"binary", 1.0); /* VDA-FS Surface data	*/
    HTAddType(".VDA",	"application/vda",		"binary", 1.0);
    HTAddType(".set",	"application/set", 		"8bit",	  1.0); /* SET (French CAD std) */
    HTAddType(".SET",	"application/set", 		"8bit",	  1.0);
    HTAddType(".stl",	"application/SLA", 		"8bit",	  1.0); /*  Stereolithography 	*/
    HTAddType(".STL",	"application/SLA", 		"8bit",	  1.0);        

    /*
    ** The following are vendor-specific CAD-formats commonly
    ** used at CERN and in HEP institutes:
    */
    HTAddType(".dwg",	"application/acad",		"binary", 1.0); /* Autocad drawing files*/
    HTAddType(".DWG",	"application/acad",		"binary", 1.0);   
    HTAddType(".SOL",	"application/solids",		"binary", 1.0); /* MATRA Prelude solids */
    HTAddType(".DRW",	"application/drafting",		"binary", 1.0); /* Prelude Drafting	*/
    HTAddType(".prt",	"application/pro_eng",		"binary", 1.0); /* PTC Pro/ENGINEER part*/
    HTAddType(".PRT",	"application/pro_eng",		"binary", 1.0);
    HTAddType(".unv",	"application/i-deas",		"binary", 1.0); /* SDRC I-DEAS files	*/
    HTAddType(".UNV",	"application/i-deas",		"binary", 1.0);
    HTAddType(".CCAD",	"application/clariscad",	"binary", 1.0); /* ClarisCAD files	*/    

    HTAddType(".snd",    "audio/basic",			"binary", 1.0);	/* Audio		*/
    HTAddType(".au",     "audio/basic",			"binary", 1.0);
    HTAddType(".aiff",   "audio/x-aiff",		"binary", 1.0);
    HTAddType(".aifc",   "audio/x-aiff",		"binary", 1.0);
    HTAddType(".aif",    "audio/x-aiff",		"binary", 1.0);
    HTAddType(".wav",    "audio/x-wav",			"binary", 1.0);	/* Windows+ WAVE format	*/
    HTAddType(".gif",    "image/gif",			"binary", 1.0);	/* GIF			*/
    HTAddType(".ief",    "image/ief",			"binary", 1.0);	/* Image Exchange fmt	*/
    HTAddType(".jpg",    "image/jpeg",			"binary", 1.0);	/* JPEG			*/
    HTAddType(".JPG",    "image/jpeg",			"binary", 1.0);
    HTAddType(".JPE",    "image/jpeg",			"binary", 1.0);
    HTAddType(".jpe",    "image/jpeg",			"binary", 1.0);
    HTAddType(".JPEG",   "image/jpeg",			"binary", 1.0);
    HTAddType(".jpeg",   "image/jpeg",			"binary", 1.0);
    HTAddType(".tif",    "image/tiff",			"binary", 1.0);	/* TIFF			*/
    HTAddType(".tiff",   "image/tiff",			"binary", 1.0);
    HTAddType(".ras",    "image/cmu-raster",		"binary", 1.0);
    HTAddType(".png",    "image/png",			"binary", 1.0);	/* PNG image format	*/
    HTAddType(".pnm",    "image/x-portable-anymap",	"binary", 1.0);	/* PBM Anymap format	*/
    HTAddType(".pbm",    "image/x-portable-bitmap",	"binary", 1.0);	/* PBM Bitmap format	*/
    HTAddType(".pgm",    "image/x-portable-graymap",	"binary", 1.0);	/* PBM Graymap format	*/
    HTAddType(".ppm",    "image/x-portable-pixmap",	"binary", 1.0);	/* PBM Pixmap format	*/
    HTAddType(".rgb",    "image/x-rgb",			"binary", 1.0);
    HTAddType(".xbm",    "image/x-xbitmap",		"7bit",   1.0);	/* X bitmap		*/
    HTAddType(".xpm",    "image/x-xpixmap",		"binary", 1.0);	/* X pixmap format	*/
    HTAddType(".xwd",    "image/x-xwindowdump",		"binary", 1.0);	/* X window dump (xwd)	*/
    HTAddType(".html",   "text/html",			"8bit",   1.0);	/* HTML			*/
    HTAddType(".htm",    "text/html",			"8bit",   1.0);	/* HTML on PC's :-(	*/
    HTAddType(".htmls",	 "text/html",			"8bit",	  1.0);	/* Server-side includes	*/
    HTAddType(".c",      "text/plain",			"7bit",   0.5);	/* C source		*/
    HTAddType(".h",      "text/plain",			"7bit",   0.5);	/* C headers		*/
    HTAddType(".C",      "text/plain",			"7bit",   0.5);	/* C++ source		*/
    HTAddType(".cc",     "text/plain",			"7bit",   0.5);	/* C++ source		*/
    HTAddType(".hh",     "text/plain",			"7bit",   0.5);	/* C++ headers		*/
    HTAddType(".m",      "text/plain",			"7bit",   0.5);	/* Objective-C source	*/
    HTAddType(".f90",    "text/plain",			"7bit",   0.5);	/* Fortran 90 source	*/
    HTAddType(".txt",    "text/plain",			"7bit",   0.5);	/* Plain text		*/
    HTAddType(".rtx",    "text/richtext",		"7bit",   1.0);	/* MIME Richtext format	*/
    HTAddType(".tsv",    "text/tab-separated-values",	"7bit",   1.0);	/* Tab-separated values	*/
    HTAddType(".etx",    "text/x-setext",		"7bit",   0.9);	/* Struct Enchanced Txt	*/
    HTAddType(".MPG",    "video/mpeg",			"binary", 1.0);	/* MPEG			*/
    HTAddType(".mpg",    "video/mpeg",			"binary", 1.0);
    HTAddType(".MPE",    "video/mpeg",			"binary", 1.0);
    HTAddType(".mpe",    "video/mpeg",			"binary", 1.0);
    HTAddType(".MPEG",   "video/mpeg",			"binary", 1.0);
    HTAddType(".mpeg",   "video/mpeg",			"binary", 1.0);
    HTAddType(".qt",     "video/quicktime",		"binary", 1.0);	/* QuickTime		*/
    HTAddType(".mov",    "video/quicktime",		"binary", 1.0);
    HTAddType(".avi",    "video/x-msvideo",		"binary", 1.0);	/* MS Video for Windows	*/
    HTAddType(".movie",  "video/x-sgi-movie",		"binary", 1.0);	/* SGI "moviepalyer"	*/

    HTAddType(".zip",	 "application/octet-stream",	"binary", 1.0);	/* PKZIP		*/
    HTAddType(".tar",    "application/octet-stream",	"binary", 1.0);	/* 4.3BSD tar		*/
    HTAddType(".ustar",  "application/octet-stream",	"binary", 1.0);	/* POSIX tar		*/

    HTAddType("*.*",     "www/unknown",			"binary", 0.2); /* Try to guess		*/
    HTAddType("*",       "www/unknown",			"binary", 0.2); /* Try to guess		*/

#ifdef VMS
    HTAddType(".cxx",	"text/plain", "7bit", 0.5);	/* C++ */
    HTAddType(".for",	"text/plain", "7bit", 0.5);	/* Fortran */
    HTAddType(".mar",	"text/plain", "7bit", 0.5);	/* MACRO */
    HTAddType(".log",	"text/plain", "7bit", 0.5);	/* logfiles */
    HTAddType(".com",	"text/plain", "7bit", 0.5);	/* scripts */
    HTAddType(".sdml",	"text/plain", "7bit", 0.5);	/* SDML */
    HTAddType(".list",	"text/plain", "7bit", 0.5);	/* listfiles */
    HTAddType(".lst",	"text/plain", "7bit", 0.5);	/* listfiles */
    HTAddType(".def",	"text/plain", "7bit", 0.5);	/* definition files */
    HTAddType(".conf",	"text/plain", "7bit", 0.5);	/* definition files */
    HTAddType(".",  	"text/plain", "7bit", 0.5);	/* files with no extension */
#endif /* VMS */

    HTAddEncoding(".Z",		"compress",	1.0);	/* Compressed data	*/
    HTAddEncoding(".gz",	"gzip",	1.0);
}
#endif /* NO_INIT */

