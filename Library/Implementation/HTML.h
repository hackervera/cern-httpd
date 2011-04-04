/*                                                     HTML to rich text converter for libwww
                         THE HTML TO STYLED TEXT OBJECT CONVERTER
                                             
   This interprets the HTML semantics and some HTMLPlus.
   
   Part of libwww . Implemented by HTML.c
   
 */
#ifndef HTML_H
#define HTML_H

#include "HTUtils.h"
#include "HTFormat.h"
#include "HTAnchor.h"
#include "HTMLPDTD.h"

#define DTD HTMLP_dtd

#ifdef SHORT_NAMES
#define HTMLPresentation        HTMLPren
#define HTMLPresent             HTMLPres
#endif

extern CONST HTStructuredClass HTMLPresentation;
/*

HTML_new: A structured stream to parse HTML

   When this routine is called, the request structure may contain a childAnchor value.  In
   that case it is the responsability of this module to select the anchor.
   
   
   
 */
extern HTStructured* HTML_new PARAMS((HTRequest * request,
                                        void *   param,
                                        HTFormat input_format,
                                        HTFormat output_format,
                                        HTStream * output_stream));

/*

  REOPEN
  
   Reopening an existing HTML object allows it to be retained (for example by the styled
   text object) after the structured stream has been closed.  To be actually deleted, the
   HTML object must be closed once more times than it has been reopened.
   
 */

extern void HTML_reopen PARAMS((HTStructured * me));
/*

Converters

   These are the converters implemented in this module:
   
 */

#ifndef pyramid
extern HTConverter HTMLToPlain, HTMLToC, HTMLPresent, HTMLToTeX;
#endif
/*

Selecting internal character set representations

 */
typedef enum _HTMLCharacterSet {
        HTML_ISO_LATIN1,
        HTML_NEXT_CHARS,
        HTML_PC_CP950
} HTMLCharacterSet;

extern void HTMLUseCharacterSet PARAMS((HTMLCharacterSet i));

/*

Record error message as a hypertext object

   The error message should be marked as an error so that it can be reloaded later. This
   implementation just throws up an error message and leaves the document unloaded.
   
  ON ENTRY,
  
  sink                    is a stream to the output device if any
                         
  number                  is the HTTP error number
                         
  message                 is the human readable message.
                         
  ON EXIT,
  
   a return code like HT_LOADED if object exists else < 0
   
 */
PUBLIC int HTLoadError PARAMS((
        HTRequest *     req,
        int             number,
        CONST char *    message));


/*

White Space Treatment

   There is a small number of different ways of treating white space in SGML, in mapping
   from a text object to HTML. These have to be programmed it seems.
   
 */

/*
In text object  \n\n            \n      tab     \n\n\t
--------------  -------------   -----   -----   -------
in Address,
Blockquote,
Normal,

                
        -               NORMAL
H1-6:           close+open
        -               HEADING
Glossary

                                                GLOSSARY
List,
Menu

                        -

        LIST
Dir

                                        DIR
Pre etc         \n\n            \n      \t              PRE

*/

typedef enum _white_space_treatment {
        WS_NORMAL,
        WS_HEADING,
        WS_GLOSSARY,
        WS_LIST,
        WS_DIR,
        WS_PRE
} white_space_treatment;

/*

Nesting State

   These elements form tree with an item for each nesting state: that is, each unique
   combination of nested elements which has a specific style.
   
 */

typedef struct _HTNesting {
    void *                      style;  /* HTStyle *: Platform dependent */
    white_space_treatment       wst;
    struct _HTNesting *         parent;
    int                         element_number;
    int                         item_number;    /* only for ordered lists */
    int                         list_level;     /* how deep nested */
    HTList *                    children;
    BOOL                        paragraph_break;
    int                         magic;
    BOOL                        object_gens_HTML; /* we don't generate HTML */
} HTNesting;


/*

Nesting functions

   These functions were new with HTML2.c.  They allow the tree of SGML nesting states to
   be manipulated, and SGML regenerated from the style sequence.
   
 */


extern void HTRegenInit NOPARAMS;

extern void HTRegenCharacter PARAMS((
        char                    c,
        HTNesting *             nesting,
        HTStructured *          target));

extern  void HTNestingChange PARAMS((
        HTStructured*   s,
        HTNesting*              old,
        HTNesting *             new,
        HTChildAnchor *         info,
        CONST char *            aName));

extern HTNesting * HTMLCommonality PARAMS((
        HTNesting *     s1,
        HTNesting *     s2));

extern HTNesting * HTNestElement PARAMS((HTNesting * p, int ele));
extern /* HTStyle * */ void * HTStyleForNesting PARAMS((HTNesting * n));

extern HTNesting* HTMLAncestor PARAMS((HTNesting * old, int depth));
extern HTNesting* CopyBranch PARAMS((HTNesting * old, HTNesting * new, int depth));
extern HTNesting * HTInsertLevel PARAMS((HTNesting * old,
                int     element_number,
                int     level));
extern HTNesting * HTDeleteLevel PARAMS((HTNesting * old,
                int     level));
extern int HTMLElementNumber PARAMS((HTNesting * s));
extern int HTMLLevel PARAMS(( HTNesting * s));
extern HTNesting* HTMLAncestor PARAMS((HTNesting * old, int depth));

#endif          /* end HTML_H */

/*

   end */
