/*                                                                          File descriptions
                                    FILE DESCRIPTIONS
                                             
   Descriptions appearing in directory listings are produced by this module.  This may be
   overridden by another module for those who which descriptions to come from somewhere
   else.
   
 */


#ifndef HTDESCRIPT_H
#define HTDESCRIPT_H

/*

Description File

   This module gets descriptions from the file defined by global variable
   HTHTDesctiptionFile in the same directory. The default value is .www_descript:
   
 */

extern char * HTDescriptionFile;
/*

   In the description file lines starting with a word starting with 'D' are taken to be
   descriptions (this looks funny now, but this is to make it easy to extend these
   description files to contain also other information.
   
 */

/*
 * Example:
 *      DESCRIBE  welcome.html  Our welcome page
 *      DESCRIBE  map*.gif      Map as a GIF image
 *      DESCRIBE  map*.ps       Map as a PostScript image
 */
/*

HTML Titles

   If description is not specified for a file that is of type text/html, this module uses
   the HTML TITLE as the description.  This feature can be turned off by setting the
   HTPeekTitles variable to false.
   
 */

extern BOOL HTPeekTitles;
/*

Read Description File

   The description file for a directory is read in only once by HTReadDescriptions(), and
   the result returned by it is given as an argument when finding out a description for a
   single file.
   
 */

PUBLIC HTList * HTReadDescriptions PARAMS((char * dirname));
/*

Get Description For a File

   Once description file has been read and the list of descriptions is returned by
   HTReadDescriptions(), the function HTGetDescription() can be used to get a description
   for a given file:
   
 */

PUBLIC char * HTGetDescription PARAMS((HTList * descriptions,
                                       char *   dirname,
                                       char *   filename,
                                       HTFormat format));
/*

   Directory name has to be present because this function may then take a peek at the file
   itself (to get the HTML TITLE, for example). If format is WWW_HTML and description is
   not found, this module may be configured to use the HTML TITLE as the description.
   
   No string returned by this function should be freed!
   
Freeing Descriptions

   Once descriptions have been gotten, the description list returned by
   HTReadDescriptions() must be freed by HTFreeDescriptions():
   
 */

PUBLIC void HTFreeDescriptions PARAMS((HTList * descriptions));
/*

 */

#endif /* !HTDESCRIPT_H */
/*

   End of HTDescript.h.  */
