
/* PROGRAM								HTAdm.c
**		PROGRAM FOR DOING ADMINISTRATIVE TASKS
**		ON SERVER SIDE (PASSWORD FILE MAINTENANCE)
**
** USAGE:
**	Changing password:
**
**		htadm -passwd pwfile username password
**
**	Checking for password validity:
**
**		htadm -check pwfile username password
**
**	Making a new user entry to password file:
**
**		htadm -adduser pwfile username password realname...
**
**	Deleting a user:
**
**		htadm -deluser pwfile username
**
**	Creating password file:
**
**		htadm -create pwfile
**
**	pwfile must always be specified. Other missing parameters
**	are prompted interactively.
**
**
** SECURITY CONSIDERATION:
**	htadm overwrites the password from argv[] as soon as
**	possible to minimize the risk of someone seing it in
**	process listing.
**
**	The written file size is checked and if it doesn't match
**	the expected size the original is left untouched.
**	Otherwise the original file is renamed with .bak suffix.
**
** AUTHORS:
**	AL	Ari Luotonen	luotonen@dxcern.cern.ch
**      MD      Mark Donszelmann   duns@vxdeop.cern.ch
**
** HISTORY:
**	 7 Oct 93  AL	Written on a rainy October night.
**	 1 Nov 93  AL	Added -deluser and -check.
**	 5 Nov 93  MD   Some fixes for VMS.
**	21 Feb 94  AL	Sets permissions according to original password
**			file, not current umask (for Unix).
** BUGS:
**
*/

#include "tcp.h"

#ifdef VMS
#include <stat.h>
#include <types.h>
#else /* not VMS */
#include <sys/types.h>
#ifndef Mips
#include <sys/stat.h>
#endif
#include <fcntl.h>
#endif /* not VMS */

#include "HTUtils.h"
#include "HTAlert.h"
#include "HTAAUtil.h"
#include "HTPasswd.h"

#ifdef VMS
/* Take away unresolved references (linker problem on VMS) */
#include "HText.h"
PUBLIC char *HTAppName = "HTAdm";
PUBLIC char *HTAppVersion = "unknown";
PUBLIC HTStyleSheet *styleSheet;
#endif /* VMS */


#define MAX_LINE_LEN 200

typedef enum {
    HTADM_UNKNOWN,
    HTADM_PASSWD,
    HTADM_CHECK,
    HTADM_ADDUSER,
    HTADM_DELUSER,
    HTADM_CREATE
} HTAdmOper;


PUBLIC time_t theTime;		/* For HTPasswd.c */


PRIVATE void usage NOARGS
{
    fprintf(stderr, "\n%s\n\nUsage:\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\n",
       "Administrative tool for CERN WWW daemon with access authorization.",
       "htadm -adduser pwfile [user [password  [real name & other stuff...]]]",
       "htadm -deluser pwfile [user]",
       "htadm -passwd  pwfile [user [password]]",
       "htadm -check   pwfile [user [password]]",
       "htadm -create  pwfile");
    exit(1);
}


PUBLIC int main ARGS2(int, argc, char **, argv)
{
    HTAdmOper oper;
    FILE *fin;
    FILE *fout;
    int fd;
    struct stat stat_in, stat_out;
    char *username = NULL;
    char *password = NULL;
    char *password_check = NULL;
    char *realname = NULL;
    char *enc_password = NULL;
    char *templ = NULL;
    char *filename = NULL;
    char *backup = NULL;
    char line[MAX_LINE_LEN+1];
    char newentry[MAX_LINE_LEN+1];
    char *found;
    int sizediff;
    int len;
    int i;

    if (argc < 2) usage();

    if (!strcmp(argv[1], "-passwd"))
	oper = HTADM_PASSWD;
    else if (!strcmp(argv[1], "-check"))
	oper = HTADM_CHECK;
    else if (!strcmp(argv[1], "-adduser"))
	oper = HTADM_ADDUSER;
    else if (!strcmp(argv[1], "-deluser"))
	oper = HTADM_DELUSER;
    else if (!strcmp(argv[1], "-create"))
	oper = HTADM_CREATE;
    else
	oper = HTADM_UNKNOWN;

    switch (oper) {
      case HTADM_CREATE:
	if (argc != 3) usage();
	if ((fin = fopen(argv[2], "r"))) {
	    fprintf(stderr,
		    "htadm:  Error: File \"%s\" already exists\n", argv[2]);
	    fclose(fin);
	    exit(10);
	}
	else if (!(fout = fopen(argv[2], "w"))) {
	    fprintf(stderr,
		    "htadm:  Error: Couldn't create file \"%s\"\n", argv[2]);
	    exit(11);
	}
	else {
	    fclose(fout);
	    exit(0);
	}
	break;

      case HTADM_DELUSER:
	if (argc > 4) usage();
      case HTADM_PASSWD:
      case HTADM_CHECK:
	if (argc > 5) usage();
      case HTADM_ADDUSER:
	if (argc < 3) usage();
	if (argc >= 5) {			/* Password given */
	    len = strlen(argv[4]);
	    password = (char*)malloc(len+1);
	    strcpy(password, argv[4]);
	    for ( ; len-- > 0 ; )		/* Destroy the password	*/
		argv[4][len] = (char)0;		/* from command line.	*/
	}
	if (argc >= 4) {			/* Username given */
	    username = (char*)malloc(strlen(argv[3])+1);
	    strcpy(username, argv[3]);
	}

	if (HTStat(argv[2], &stat_in) == -1) {
	    fprintf(stderr,
		    "htadm:  Error: Can't stat password file \"%s\"\n",
		    argv[2]);
	    exit(2);
	}

	if (!(fin = fopen(argv[2], "r"))) {
	    fprintf(stderr,
		    "htadm:  Error: Unable to open password file \"%s\"\n",
		    argv[2]);
	    exit(2);
	}

	if (oper == HTADM_CHECK) {
	    fclose(fin);	/* Opened just to check that it exists */
	    while (!username || !*username)
		username = HTPrompt("Username: ", NULL);
	    if (!password) {
		for (;;) {
		    password = HTPromptPassword("Password: ");
		    password_check = HTPromptPassword("Verify: ");
		    if (0==strcmp(password, password_check))
			break;
		    else
			HTAlert("Mismatch, type password again");
		}
	    }
	    if (HTAA_checkPassword(username, password, argv[2])) {
		fprintf(stdout, "Correct\n");
		exit(0);
	    }
	    else {
		fprintf(stdout, "Incorrect\n");
		exit(6);
	    }
	}

	StrAllocCopy(filename, argv[2]);
#ifndef VMS
	StrAllocCat(filename, ".tmp");
#else
	StrAllocCat(filename, "_tmp");
#endif /* VMS */

#ifndef VMS
	umask(0);
	fd = open(filename,
		  O_WRONLY | O_CREAT | O_TRUNC, 
		  (S_IRWXU | S_IRWXG | S_IRWXO) & stat_in.st_mode);
	if (fd == -1 || !(fout = fdopen(fd, "w"))) {
#else
	if (!(fout = fopen(filename, "w"))) {
#endif
	    fprintf(stderr,
		    "htadm:  Error: Unable to create temporary file \"%s\"\n",
		    filename);
	    fclose(fin);
	    exit(3);
	}

	while (!username || !*username)
	    username = HTPrompt("Username: ", NULL);
	StrAllocCopy(templ, username);
	StrAllocCat(templ, ":");
	len = strlen(templ);

	while ((found = fgets(line, MAX_LINE_LEN, fin))) {
	    if (!strncmp(line, templ, len))
		break;
	    fprintf(fout, "%s", line);	/* Copy lines to tmp file */
	}

	if ((found && oper == HTADM_ADDUSER) ||
	    (!found && (oper == HTADM_PASSWD || oper == HTADM_DELUSER))) {
	    fprintf(stderr, 
		"htadm:  Error: Entry for user \"%s\" %s in password file\n",
		    username,
		    (found ? "already" : "not found"));
	    fclose(fin);
	    fclose(fout);
#ifdef VMS
            delete(filename);
#else /* not VMS */
	    unlink(filename);	/* Remove tmp file */
#endif /* not VMS */
	    exit(4);
	}

	if (oper == HTADM_ADDUSER || oper == HTADM_PASSWD) {
	    if (!password) {
		for (;;) {
		    password = HTPromptPassword("Password: ");
		    password_check = HTPromptPassword("Verify: ");
		    if (0==strcmp(password, password_check))
			break;
		    else
			HTAlert("Mismatch, type password again");
		}
	    }
	    time(&theTime);	/* Password encryption routine needs this */
	    enc_password = HTAA_encryptPasswd(password ? password : "");
	    
	    sprintf(newentry, "%s:%s:", username, enc_password);
	    if (oper == HTADM_ADDUSER) {
		if (argc>5) {		/* Real name given in command line */
		    strcat(newentry, argv[5]);
		    for (i=6; i < argc; i++) {
			strcat(newentry, " ");
			strcat(newentry, argv[i]);
		    }
		}
		else {
		    if ((realname = HTPrompt("Real name: ", NULL)))
			strcat(newentry, realname);
		}
		strcat(newentry, "\n");
		sizediff = strlen(newentry);
	    }
	    else {		/* oper == HTADM_PASSWD */
		if ((realname = strchr(line, ':'))) {	/* Find real name */
		    realname++;				/* from old entry */
		    if ((realname = strchr(realname, ':'))) {
			realname++;
			strcat(newentry, realname);
		    }
		}
		sizediff = strlen(newentry) - strlen(line);
	    }

	    fprintf(fout, "%s", newentry);
	}
	else {	/* oper == HTADM_DELUSER */
	    sizediff = -strlen(line);
	}

	if (oper == HTADM_PASSWD || oper == HTADM_DELUSER) {
	    /* Append the rest of original file */
	    while (fgets(line, MAX_LINE_LEN, fin))
		fprintf(fout, "%s", line);
	}
	fclose(fin);
	fclose(fout);

	/* Check that everything went ok */
	HTStat(filename, &stat_out);
	if (stat_out.st_size != stat_in.st_size + sizediff) {
	    fprintf(stderr, "%s %d %s %d\n%s\n",
		    "htadm:  Error: new file size",
		    (int)stat_out.st_size,
		    "doesn't match expected",
		    (int)(stat_in.st_size + sizediff),
		    "htadm:  Original password file left intact");
	    exit(5);
	}

	StrAllocCopy(backup, argv[2]);
#ifndef VMS
	StrAllocCat(backup, ".bak");
#else
	StrAllocCat(backup, "_bak");
#endif /* VMS */
	rename(argv[2], backup);
	rename(filename, argv[2]);

	break;

      default:
	usage();

    } /* switch (oper) */

#ifdef VMS
    exit(1);
#endif /* VMS */

    return 0;	/* For gcc :-( */
}

