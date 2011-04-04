/*		Handle a request from a WWW client	DCLServer.c
**		==================================
**
** Authors
**	JFG	Jean-Francois Groff, CERN, Geneva	jfg@info.cern.ch
**      TBL	Tim Berners-Lee CERN                    tbl@info.cern.ch
**      JS      Jonathan Streets, FNAL                  streets@fnal.fnal.gov
**
** History:
**
** 1.0   9 Jul 92 (TBL, JS) DocDB gateway hacked from VMSHelp gateway
**
** 0.3	27 Sep 91 (JFG)	VMSHelp gateway created from h2h.c
**
*/

/* (c) CERN WorldWideWeb project 1990-1992. See Copyright.html for details */
/* and FNAl etc etc */

/* This is the DCL command which will be used
*/
#define DCL_COMMAND "@docdbgate"

/** Headers taken from HTRetrieve.c **/

#define BUFFER_SIZE 4096	/* Arbitrary size for efficiency */

#include "HTUtils.h"
#include "tcp.h"

extern int WWW_TraceFlag;	/* Control diagnostic output */
extern FILE * LogFile;		/* Log file output */
extern char HTClientHost[16];	/* Client name to be output */
extern int HTWriteASCII(int soc, char * s);	/* In HTDaemon.c */


/** Headers taken from h2h.c **/

/* Maximum line size for buffers */
#define LSIZE 256

/* Maximum command line size for VMS */
#define CSIZE 256

/* Maximum VMS file name size (on 5.3, it's really 41, yuk! hope for more) */
#define FSIZE 80



/******************************************************************************

HTRetrieve : Retrieves information for the W3 server by calling a DCL file

Inputs :
	char *arg : HT address
	char *keywords : plus-separated keyword list, if any
	int soc : output socket

DCL file parameters:

      p1   The device name down which the result should be sent
      p2   The document id requested
      p3   The keywords requested if any, still + separated.


-----------------------------------------------------------------------------*/

#include <dvidef.h>

int HTRetrieve
#ifdef __STDC__
  (const char *arg, const char *keywords, const int soc)
#else
  (arg, keywords, soc)
    char *arg;
    char *keywords;
    int soc;
#endif
{
  FILE *hlp;

  char *query;
  char *s;
  char command[1024];		/* bug */
#define STRING_SIZE	65
  char devname[STRING_SIZE];
  struct {
	int	size;
	char*	string;
	} desc_devname;
  int status;
  int length;
  int item;

/*    First of all, we must get the device name of the socket
**    to pass to the DCL coimmand file.
*/
  desc_devname.string = &devname[0];
  desc_devname.size = STRING_SIZE;
  item = DVI$_DEVNAM;

  status=lib$getdvi(&item,&soc,0,0,&desc_devname,&length);
  if (!(status&1)) 
	lib$signal(status);

  desc_devname.string[length] = '\0';
  if (TRACE) fprintf(stderr, "DCLServer: socket devname is %s \n",devname);

/*     Now we call the DCL file to do the work
*/
  if (keywords) sprintf(command,
     "%s \"%s\" \"%s\"  \"%s\" ", DCL_COMMAND, devname, arg, keywords);
  else sprintf(command,
     "%s \"%s\" \"%s\"", DCL_COMMAND, devname, arg);

  system(command);  /* ought to check return code @@@ */

  return 0;     /* OK */

}
