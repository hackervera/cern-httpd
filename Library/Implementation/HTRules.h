/*                                                           Configuration Manager for libwww
                                  CONFIGURATION MANAGER
                                             
   Author Tim Berners-Lee/CERN. Public domain. Please mail changes to timbl@info.cern.ch.
   
   The configuration information loaded includes tables (file suffixes, presentation
   methods) in other modules. The most likely routines needed by developers will be:
   
  HTSetConfiguration     to load configuration information.
                         
  HTLoadRules            to load a whole file of configuration information
                         
  HTTranslate            to translate a URL using the rule table.
                         
 */
#ifndef HTRULE_H
#define HTRULE_H

#include "HTUtils.h"
#include "HTAccess.h"   /* HTRequest */

typedef enum _HTRuleOp {
        HT_Invalid,
        HT_Map,
        HT_Pass,
        HT_Fail,
        HT_DefProt,
        HT_Protect,
        HT_Exec,
        HT_Redirect,
        HT_Moved,
        HT_UseProxy
} HTRuleOp;

#ifdef SHORT_NAMES
#define HTSearSc HTSearchScript
#define HTPutScr HTPutScript
#define HTPostSc HTPostScript
#endif /*SHORT_NAMES*/

/*

SERVER SIDE SCRIPT EXECUTION

   If a URL starts with /htbin/it is understood to mean a script execution request on
   server. This feature needs to be turned on by setting HTBinDirby the htbinrule. Index
   searching is enabled by setting HTSearchScriptinto the name of script in BinDir doing
   the actual search by searchrule (BinDir must also be set in this case, of course).
   
 */
#ifdef NOT_USED
extern char * HTSearchScript;   /* Search script name */
extern char * HTPutScript;      /* Script handling PUT */
extern char * HTPostScript;     /* Script handling POST */
#endif /* NOT_USED */

/*

HTADDRULE: ADD RULE TO THE LIST

  On entry,
  
  pattern                points to 0-terminated string containing a single "*"
                         
  equiv                  points to the equivalent string with * for the place where the
                         text matched by * goes.
                         
  On exit,
  
  returns                0 if success, -1 if error.
                         
   Note that if BYTE_ADDRESSING is set, the three blocks required are allocated and
   deallocated as one. This will save time and storage, when malloc's allocation units are
   large.
   
 */
extern int HTAddRule PARAMS((HTRuleOp op, const char * pattern, const char * equiv));


/*

HTCLEARRULES: CLEAR ALL RULES

  On exit,
  
  Rule file              There are no rules
                         
  returns
                         0 if success, -1 if error.
                         
 */
#ifdef __STDC__
extern int HTClearRules(void);
#else
extern int HTClearRules();
#endif


/*

HTTRANSLATE: TRANSLATE BY RULES

 */
        
/*

  On entry,
  
  required               points to a string whose equivalent value is neeed
                         
  On exit,
  
  returns                the address of the equivalent string allocated from the heap
                         which the CALLER MUST FREE. If no translation occured, then it is
                         a copy of the original.
                         
 */
PUBLIC char * HTTranslate PARAMS((CONST char * required));

/*

HTSETCONFIGURATION: LOAD ONE LINE OF CONFIGURATION INFORMATION

  On entry,
  
  config                 is a string in the syntax of a rule file line.
                         
   This routine may be used for loading configuration information from sources other than
   the rule file, for example INI files for X resources.
   
 */
extern int HTSetConfiguration PARAMS((CONST char * config));


/*

HTLOADRULES: LOAD THE RULES FROM A FILE

  On entry,
  
  Rule table             Rules can be in any state
                         
  On exit,
  
  Rule table             Any existing rules will have been kept. Any new rules will have
                         been loaded on top, so as to be tried first.
                         
  Returns                0 if no error.
                         
 */
#ifdef __STDC__
extern int HTLoadRules(const char * filename);
#else
extern int HTLoadRules();
#endif
/*

 */
#endif /* HTUtils.h */
/*

   end */
