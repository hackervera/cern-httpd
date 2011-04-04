/* Module for users to insert their own initialisation routines */

#include "HTUtils.h"
#include "HTUserInit.h"

/* called just after main has started */
PUBLIC int HTUserInit NOARGS
{
   return(0);
}
