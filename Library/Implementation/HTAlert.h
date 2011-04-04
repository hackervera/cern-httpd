/*                                                  HTAlert: Handling user messages in libwww
                          DISPLAYING MESSAGES AND GETTING INPUT
                                             
   This module is a part of the CERN WWW Library. It may be overridden for GUI clients. It
   allows progress indications and warning messages to be communicated to the user in a
   portable way.
   
      May 92 Created By C.T. Barker
      
      Feb 93 Portablized etc TBL
      
 */

#include "HTUtils.h"

#ifdef SHORT_NAMES
#define HTPrPass        HTPromptPassword
#define HTPUnAPw        HTPromptUsernameAndPassword
#endif /*SHORT_NAMES*/
/*

Flags for This Module

 */

extern BOOL HTInteractive;                  /* Any prompts from the Library? */
/*

HTPrompt and HTPromptPassword: Display a message and get the input

   HTPromptPassword() doesn't echo reply on the screen.
   
  ON ENTRY,
  
  Msg                     String to be displayed.
                         
  deflt                   If NULL the default value (only for HTPrompt())
                         
  ON EXIT,
  
  Return value            is malloc'd string which must be freed.
                         
 */
                
extern char * HTPrompt PARAMS((CONST char * Msg, CONST char * deflt));
extern char * HTPromptPassword PARAMS((CONST char * Msg));

/*

HTPromptUsernameAndPassword: Get both username and password

  ON ENTRY,
  
  Msg                    String to be displayed.
                         
  username                Pointer to char pointer, i.e. *usernamepoints to a string.  If
                         non-NULL it is taken to be a default value.
                         
  password                Pointer to char pointer, i.e. *passwordpoints to a string.
                         Initial value discarded.
                         
  ON EXIT,
  
  *username               and
                         
  *password               point to newly allocated strings representing the typed-in
                         username and password.  Initial strings pointed to by them are
                         NOT freed!
                         
 */

extern void HTPromptUsernameAndPassword PARAMS((CONST char *    Msg,
                                                char **         username,
                                                char **         password));

/*

Display a message, don't wait for input

  ON ENTRY,
  
  Msg                     String to be displayed.
                         
 */

extern void HTAlert PARAMS((CONST char * Msg));


/*

Display a progress message for information (and diagnostics) only

  ON ENTRY,
  
   The input is a list of parameters for printf.
   
 */
extern void HTProgress PARAMS((CONST char * Msg));


/*

Display a message, then wait for 'yes' or 'no'.

  ON ENTRY,
  
  Msg                     String to be displayed
                         
  ON EXIT,
  
  Returns                 If the user reacts in the affirmative, returns TRUE, returns
                         FALSE otherwise.
                         
 */

extern BOOL HTConfirm PARAMS ((CONST char * Msg));





/*

    */
