
/* MODULE							HTLog.c
**		LOGGING FOR HTTPD
**
** AUTHORS:
**	AL	Ari Luotonen	luotonen@dxcern.cern.ch
**
** HISTORY:
**	13 Feb 94  AL	Separated from HTDaemon.c
**
** BUGS:
**
**
*/

#include <stdio.h>
#include <time.h>

#include "tcp.h"
#include "HTUtils.h"	/* gmtime foor VMS */
#include "HTLog.h"
#include "HTList.h"
#include "HTGroup.h"	/* HTIpMaskMatch() */
#include "HTDaemon.h"
#include "HTConfig.h"
#include "HTAuth.h"

#define n_pick(x,y)	((x) ? (x) : ((y) ? (y) : "-"))
#define n_noth(x)	((x) ? (x) : "")
#define n_hyph(x)	(((x) && (*x)) ? (x) : "-")

#define HTABS(x)	(((x) < 0) ? (-(x)) : (x))

#ifndef VMS
#define HTFlush fflush
#endif /* not VMS */


PRIVATE FILE *	access_log	= NULL;
PRIVATE FILE *	proxy_log	= NULL;		
PRIVATE FILE *	cache_log	= NULL;
PRIVATE FILE *	error_log	= NULL;

extern char * HTClientHost;
extern char * HTClientHostName;

#if defined(Mips) || (defined(VMS) && !defined(DECC))
extern char * month_names[];
#endif

#define ERR_QUEUE_SIZE 40
PRIVATE char * err_queue[ERR_QUEUE_SIZE];
PRIVATE int err_index = 0;

#define DATE_EXT_LEN 20
PRIVATE char * time_escapes ARGS1(CONST char *, filename)
{
    char *result = (char *) malloc(strlen(filename)+DATE_EXT_LEN+2);
    if (sc.log_file_date_ext) {
	time_t cur_time = time(NULL);
	struct tm * gorl;
	char date[DATE_EXT_LEN+1];

	if (sc.use_gmt)
	    gorl = gmtime(&cur_time);
	else
	    gorl = localtime(&cur_time);

	/* Not all platforms have strftime() :-( */
#if defined(Mips) || defined(_AUX) || (defined(VMS) && !defined(DECC))
	/* A shortcut as we should use the value of log_file_date_ext :-( */
	sprintf(date, "%s%02d%02d",
		month_names[gorl->tm_mon], gorl->tm_mday, gorl->tm_year);
#else
	strftime(date, DATE_EXT_LEN, sc.log_file_date_ext, gorl);
	*(date+DATE_EXT_LEN)='\0';
#endif /* strftime() */
	{
	    char *ptr = date;
	    while (*ptr) {
		if (*ptr == ' ') *ptr = '_';
		ptr++;
	    }
	}
	if (*(filename+strlen(filename)-1) == '.')
	    sprintf(result, "%s%s", filename, date);
	else
	    sprintf(result, "%s.%s", filename, date);
    } else
	sprintf(result, filename);

    if (*result != '/') {
	if (sc.server_root) {
	    int srlen = strlen(sc.server_root);
	    char * n = (char*)malloc(srlen + strlen(result) + 2);
	    if (!n) outofmem(__FILE__, "time_escapes");
	    CTRACE(stderr,
		   "Relative.... filename for log mapped under ServerRoot\n");
	    sprintf(n, "%s%s%s", sc.server_root,
		    sc.server_root[srlen-1]!='/' ? "/" : "",  result);
	    CTRACE(stderr, "Absolute.... filename \"%s\"\n", n);
	    free(result);
	    result = n;
	}
	else {
	    HTLog_error2("Relative log name when ServerRoot not specified:",
			 filename);
	}
    }
    return result;
}


#ifdef VMS
#define TIMES_TO_REOPEN	20
PRIVATE int HTFlush ARGS1(FILE *, fptr)
{
static access_log_times = 0;
static error_log_times = 0;
static cache_log_times = 0;

   /* flush C rtl buffers */
   fflush(fptr);
  
   /* flush RMS buffers */
   fsync(fileno(fptr));

   /* reopen file if some lines have been written, so we can read it under VMS */
   if (fptr == access_log)
   {
      access_log_times++;
      if (access_log_times > TIMES_TO_REOPEN)
      {
         access_log_times = 0;
         access_log = freopen(time_escapes(sc.access_log_name), "a+", 
				access_log, "shr=get", "shr=put", "shr=upd");
      }
   }
   else
   if (fptr == error_log)
   {
      error_log_times++;
      if (error_log_times > TIMES_TO_REOPEN)
      {
         error_log_times = 0;
         error_log = freopen(time_escapes(sc.error_log_name), "a+", 
				error_log, "shr=get","shr=put", "shr=upd");
      }
   }
   else
   if (fptr == cache_log)
   {
      cache_log_times++;
      if (cache_log_times > TIMES_TO_REOPEN)
      {
         cache_log_times = 0;
         cache_log = freopen(time_escapes(sc.cache_log_name), "a+", 
				cache_log, "shr=get","shr=put", "shr=upd");
      }
   }

   return(1);
}
#endif /* VMS */


PRIVATE void flush_queue NOARGS
{
    int i;

    for(i=0; i<err_index && i<ERR_QUEUE_SIZE; i++) {
	if (err_queue[i]) {
	    fprintf(error_log, "%s\n", err_queue[i]);
	    free(err_queue[i]);
	    err_queue[i] = NULL;
	}
    }
    HTFlush(error_log);
    if (err_index > ERR_QUEUE_SIZE)
	HTLog_errorN("There were more errors, queued only", ERR_QUEUE_SIZE);
    err_index = 0;
}


PRIVATE char * log_time NOARGS
{
    struct tm * gorl;		/* GMT or localtime */
    static char ret[30];
#ifdef ISC3
    long z = 0L;		/* Lauren */
#else /* not ISC3 */
    int z = 0;			/* Timezone diff in minutes */
#endif /* not ISC3 */

    if (sc.use_gmt)
	gorl = gmtime(&cur_time);
    else {
	gorl = localtime(&cur_time);
/* have tm_gmtoff */
#if defined(SIGTSTP) && !defined(AIX) && !defined(__sgi) && !defined(_AUX) && !defined(__svr4__)
	z = gorl->tm_gmtoff/60;

#else /* SysV or VMS */					

#ifdef VMS
	z = 0;

#else  /* SysV */
#ifdef ISC3	/* Lauren */
	if (daylight && gorl->tm_isdst)	/* daylight time? */
	    z = altzone;	/* yes */
	else
	    z = timezone;	/* no */

	z /= 60;		/* convert to minutes */
	z = -z;			/* ISC 3.0 has it vice versa :-( */
#else /* not ISC3 */
	z = timezone / 60;			/* Thanks to Michael Fischer */
#endif /* not ISC3 */
#endif /* SysV */
#endif /* SysV or VMS */
    }

    sprintf(ret, "%02d/%s/%04d:%02d:%02d:%02d %c%02d%02d",
	    gorl->tm_mday,
	    month_names[gorl->tm_mon],
	    1900 + gorl->tm_year,
	    gorl->tm_hour,
	    gorl->tm_min,
	    gorl->tm_sec,
	    z < 0 ? '-' : '+',
	    (int) HTABS(z) / 60,
	    (int) HTABS(z) % 60);

    return ret;
}



PUBLIC void HTLog_access ARGS1(HTRequest *, req)
{
    time_t t;
    struct tm * gorl;
    char * authuser = "-";
    char * r_ident = "-";
    FILE * log = access_log;

    if (cache_hit && out.status_code == 200 && cache_log)
	log = cache_log;
    else if (proxy_access && proxy_log)
	log = proxy_log;

    if (!log) return;

    if (sc.no_log) {
	HTList * cur = sc.no_log;
	HTPattern * pat;

	while ((pat = (HTPattern*)HTList_nextObject(cur))) {
	    if (HTIpMaskMatch(pat, HTClientHost, HTClientHostName)) {
		CTRACE(stderr,
		       "Log......... NOT making a log entry for %s [%s]\n",
		       HTClientHost, n_noth(HTClientHostName));
		return;
	    }
	}
    }

    if (HTUser && HTUser->username && *HTUser->username)
	authuser = HTUser->username;
    if (remote_ident && *remote_ident && strcmp(remote_ident,"unknown")!=0)
	r_ident = remote_ident;

    if (sc.new_logfile_format) {
	char buf[30];

	if (out.status_code > 0) {
#if 0
	    if (HTReason == HTAA_OK_GATEWAY &&
		bytes_from_cache > 0  &&  HTStatusCode==200)
		sprintf(buf, "%d %d", HTStatusCode, bytes_from_cache);
	    else if (HTReason == HTAA_OK_GATEWAY && 
		HTProxyBytes > 0 && HTStatusCode==200)
		sprintf(buf, "%d %ld", HTStatusCode, HTProxyBytes);
	    else
#endif /* 0 */
	    if (req->method == METHOD_HEAD || out.status_code == 304)
		sprintf(buf, "%d 0", out.status_code);
	    else if (out.content_length > 0)
		sprintf(buf, "%d %d", out.status_code, out.content_length);
	    else
		sprintf(buf, "%d -", out.status_code);
	}
	else strcpy(buf, "- -");

	fprintf(log, "%s %s %s [%s] \"%s\" %s\n",
		n_pick(HTClientHostName,HTClientHost),
		r_ident, authuser, log_time(), n_noth(HTReqLine), buf);
    }
    else {	/* Still old format */
	time(&t);
	if (sc.use_gmt)
	    gorl = gmtime(&t);
	else
	    gorl = localtime(&t);

	fprintf(log, "%24.24s %s %s\n",
		asctime(gorl), HTClientHost, n_noth(HTReqLine));
    }

    HTFlush(log);
}


PRIVATE char * status_name NOARGS
{
    switch (HTReason) {

    /* Success */
      case HTAA_OK:			return "OK";
      case HTAA_OK_GATEWAY:		return "OK-GATEWAY";
      case HTAA_OK_REDIRECT:		return "OK-REDIRECT";
      case HTAA_OK_MOVED:		return "OK-MOVED";

    /* 401 cases */
      case HTAA_NO_AUTH:		return "NOT AUTHENTICATED";
      case HTAA_NOT_MEMBER:		return "NOT AUTHORIZED";

    /* 403 cases */
      case HTAA_IP_MASK:		return "FORBIDDEN BY IP";
      case HTAA_IP_MASK_PROXY:		return "FORBIDDEN BY IP FOR PROXY";
      case HTAA_BY_RULE:		return "FORBIDDEN BY RULE";
      case HTAA_NO_ACL:			return "NO ACL FILE";
      case HTAA_NO_ENTRY:		return "NO ACL ENTRY";
      case HTAA_SETUP_ERROR:		return " * SETUP ERROR * ";
      case HTAA_DOTDOT:			return "DOT-DOT IN URL";
      case HTAA_HTBIN:			return "HTBIN OFF";
      case HTAA_INVALID_REDIRECT:	return "INVALID REDIRECT";
      case HTAA_INVALID_USER:		return "NO SUCH USER";
      case HTAA_NOT_ALLOWED:		return "PUT NOT ALLOWED";

    /* 404 cases */
      case HTAA_NOT_FOUND:		return "NOT FOUND";
      case HTAA_MULTI_FAILED:		return "MULTI FAILED";

    /* Others */
      default:	return "?";		/* A new status maybe?? */
    } /* switch */
}


PUBLIC void HTLog_error2 ARGS2(CONST char *, msg,
			       CONST char *, param)
{
    char * t = log_time();
    char * extras = NULL;

    CTRACE(stderr, "ERROR....... [%s] %s %s\n", t, n_hyph(msg), n_noth(param));

    if (HTClientHost) {
	char * host = n_pick(HTClientHostName,HTClientHost);
	char * user = (HTUser && HTUser->username) ? HTUser->username : "";
	char * ref  = n_noth(HTReferer);

	extras = (char*)malloc(strlen(host) + strlen(user) + strlen(ref) +100);
	if (!extras) outofmem(__FILE__, "HTLog_error2");

	sprintf(extras, " [%s] [host: %s", status_name(), host);
	if (*user) {
	    strcat(extras, " user: ");
	    strcat(extras, user);
	}
	if (*ref) {
	    strcat(extras, " referer: ");
	    strcat(extras, ref);
	}
	strcat(extras, "]");
    }

    if (!error_log) {
	char * q = (char*)malloc(strlen(n_noth(msg)) + strlen(n_noth(param)) +
				 strlen(n_noth(extras)) + 100);
	if (!q) outofmem(__FILE__, "HTLog_error2");

	sprintf(q, "[%s]%s %s %s",
		t, n_noth(extras), n_noth(msg), n_noth(param));
	err_queue[err_index++] = q;
    }
    else {
	fprintf(error_log, "[%s]%s %s %s\n",
		t, n_noth(extras), n_noth(msg), n_noth(param));
	HTFlush(error_log);
    }

    if (extras) free(extras);
}


PUBLIC void HTLog_error ARGS1(CONST char *, msg)
{
    HTLog_error2(msg, NULL);
}


PUBLIC void HTLog_errorN ARGS2(CONST char *,	msg,
			       int,		num)
{
    char buf[20];
    sprintf(buf, "%d", num);
    HTLog_error2(msg, buf);
}


PUBLIC void HTLog_closeAll NOARGS
{
    if (access_log) {
	fclose(access_log);
	access_log = NULL;
    }
    if (error_log) {
	fclose(error_log);
	error_log = NULL;
    }
    if (cache_log) {
	fclose(cache_log);
	cache_log = NULL;
    }
}


PUBLIC FILE * do_open ARGS1(char *, filename)
{
#ifdef VMS
    return fopen(filename, "a+", "shr=get", "shr=put", "shr=upd");
#else
#ifndef NO_UNIX_IO
    /* Here we can use open with the O_APPEND which causes the positioning
       of the file pointer and the write to be an atomic operation.
       if the file doesn't exist the file is created with the
       permissions required from POSIX.1 (see Advanced Programming in the
       UNIX Environment by W. Richard Stevens). This might, however, be 
       overwritten by the umask of the process */
    int fd;
    if ((fd = open(filename, O_WRONLY | O_CREAT | O_APPEND,
	 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) < 0)
	return NULL;
    
    /* Now bind this file descriptor to an ANSI stream. fdopen _is_ POSIX.1
       but I wonder where it will cause problems??? :-( */
    return fdopen(fd, "a");
#else  /* If ANSI FILE descriptor */
    return fopen(filename, "a");
#endif /* NO_UNIX_IO vs. ANSI */
#endif /* VMS vs. UNIX */
}


PUBLIC BOOL HTLog_openAll NOARGS
{
    BOOL flag = YES;
    char * aln = NULL;
    char * pln = NULL;
    char * cln = NULL;
    char * eln = NULL;

    /*
     * Open access log file
     */
    if (sc.access_log_name) {
	aln = time_escapes(sc.access_log_name);
	access_log = do_open(aln);
	if (access_log) {
	    CTRACE(stderr, "Log......... \"%s\" opened\n", aln);
	}
	else {
	    HTLog_error2("Can't open log file:", aln);
	    flag = NO;
	}
    }

    /*
     * Open proxy access log file
     */
    if (sc.proxy_log_name) {
	pln = time_escapes(sc.proxy_log_name);
	proxy_log = do_open(pln);
	if (proxy_log) {
	    CTRACE(stderr, "Proxy log... \"%s\" opened\n", pln);
	}
	else {
	    HTLog_error2("Can't open proxy access log:", pln);
	    flag = NO;
	}
    }

    /*
     * Open cache access log file
     */
    if (!cc.caching_explicitly_off &&
	cc.cache_root &&
	sc.cache_log_name) {

	cln = time_escapes(sc.cache_log_name);
	cache_log = do_open(cln);
	if (cache_log) {
	    CTRACE(stderr, "Cache log... \"%s\" opened\n", cln);
	}
	else {
	    HTLog_error2("Can't open cache access log:", cln);
	    flag = NO;
	}
    }

    /*
     * Open error log and flush the current error queue to it
     */
    if (sc.error_log_name)
	eln = time_escapes(sc.error_log_name);
    else if (aln) {
	char * dot = strrchr(aln, '.');
	if (dot) *dot = 0;
	eln = (char*)malloc(strlen(aln) + 7);
	sprintf(eln, "%s.error", aln);
	if (dot) *dot = '.';
    }
    else {
	CTRACE(stderr, "No error logging\n");
	goto done;
    }

    error_log = do_open(eln);
    if (error_log) {
	CTRACE(stderr, "Error log... \"%s\" opened\n", eln);
	flush_queue();
    }
    else {
	HTLog_error2("Can't open error log:", eln);
	flag = NO;
    }

  done:
    FREE(aln);
    FREE(pln);
    FREE(eln);
    FREE(cln);

    return flag;
}

