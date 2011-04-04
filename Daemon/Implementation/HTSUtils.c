
/* MODULE							HTSUtils.c
**		SERVER UTILITIES
**
** AUTHORS:
**	AL	Ari Luotonen	luotonen@dxcern.cern.ch
**	MD	Mark Dönszelmann 	duns@vxdeop.cern.ch
**
** HISTORY:
**	10 Mar 94  AL	First written.
**
** BUGS:
**	MD : VMS does not support GMT, so no offset given...
**
**
*/

#include "HTSUtils.h"
#include "tcp.h"
#include "HTConfig.h"

#if defined(Mips) || (defined(VMS) && !defined(DECC))
PRIVATE char * wkdays[7] = {
    "Mon","Tue","Wed","Thu","Fri","Sat","Sun"
};
#endif

PUBLIC char * month_names[12] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};


/*								isNumber()
 *		Does a character string represent an integer
 */
PUBLIC BOOL HTIsNumber ARGS1(CONST char *, s)
{
    CONST char *cur = s;

    if (!s || !*s) return NO;

    if (*cur == '-')
	cur++;		/* Allow initial minus sign in a number */

    while (*cur) {
	if (*cur < '0' || *cur > '9')
	    return NO;
	cur++;
    }
    return YES;
}


PRIVATE int make_num ARGS1(char *, s)
{
    if (*s >= '0' && *s <= '9')
	return 10 * (*s - '0') + *(s+1) - '0';
    else
	return *(s+1) - '0';
}

PRIVATE int make_month ARGS1(char *, s)
{
    int i;

    *s = TOUPPER(*s);
    *(s+1) = TOLOWER(*(s+1));
    *(s+2) = TOLOWER(*(s+2));

    for (i=0; i<12; i++)
	if (!strncmp(month_names[i], s, 3))
	    return i;
    return 0;
}


PUBLIC time_t parse_http_time ARGS1(char *, str)
{
    char * s;
    struct tm tm;
    time_t t;
#ifdef ISC3   /* Lauren */
    struct tm * gorl;		/* GMT or localtime */
    long z = 0L;
#endif

    if (!str) return 0;

    if ((s = strchr(str, ','))) {	/* Thursday, 10-Jun-93 01:29:59 GMT */
	s++;				/* or: Thu, 10 Jan 1993 01:29:59 GMT */
	while (*s && *s==' ') s++;
	if (strchr(s,'-')) {		/* First format */
	    CTRACE(stderr, "Format...... Weekday, 00-Mon-00 00:00:00 GMT\n");
	    if ((int)strlen(s) < 18) {
		CTRACE(stderr,
		       "ERROR....... Not a valid time format \"%s\"\n", s);
		return 0;
	    }
	    tm.tm_mday = make_num(s);
	    tm.tm_mon = make_month(s+3);
	    tm.tm_year = make_num(s+7);
	    tm.tm_hour = make_num(s+10);
	    tm.tm_min = make_num(s+13);
	    tm.tm_sec = make_num(s+16);
	}
	else {				/* Second format */
	    CTRACE(stderr, "Format...... Wkd, 00 Mon 0000 00:00:00 GMT\n");
	    if ((int)strlen(s) < 20) {
		CTRACE(stderr,
		       "ERROR....... Not a valid time format \"%s\"\n", s);
		return 0;
	    }
	    tm.tm_mday = make_num(s);
	    tm.tm_mon = make_month(s+3);
	    tm.tm_year = (100*make_num(s+7) - 1900) + make_num(s+9);
	    tm.tm_hour = make_num(s+12);
	    tm.tm_min = make_num(s+15);
	    tm.tm_sec = make_num(s+18);

	}
    }
    else {	/* Try the other format:  Wed Jun  9 01:29:59 1993 GMT */
	CTRACE(stderr, "Format...... Wkd Mon 00 00:00:00 0000 GMT\n");
	s = str;
	while (*s && *s==' ') s++;
	CTRACE(stderr, "Trying...... The Wrong time format: %s\n", s);
	if ((int)strlen(s) < 24) {
	    CTRACE(stderr, "ERROR....... Not a valid time format \"%s\"\n", s);
	    return 0;
	}
	tm.tm_mday = make_num(s+8);
	tm.tm_mon = make_month(s+4);
	tm.tm_year = make_num(s+22);
	tm.tm_hour = make_num(s+11);
	tm.tm_min = make_num(s+14);
	tm.tm_sec = make_num(s+17);
    }
    if (tm.tm_sec  < 0  ||  tm.tm_sec  > 59  ||
	tm.tm_min  < 0  ||  tm.tm_min  > 59  ||
	tm.tm_hour < 0  ||  tm.tm_hour > 23  ||
	tm.tm_mday < 1  ||  tm.tm_mday > 31  ||
	tm.tm_mon  < 0  ||  tm.tm_mon  > 11  ||
	tm.tm_year <70  ||  tm.tm_year >120) {
	CTRACE(stderr,
	"ERROR....... Parsed illegal time: %02d.%02d.%02d %02d:%02d:%02d\n",
	       tm.tm_mday, tm.tm_mon+1, tm.tm_year,
	       tm.tm_hour, tm.tm_min, tm.tm_sec);
	return 0;
    }

    tm.tm_isdst = -1;

    /*
     *	What a pain it is to get the timezone correctly.
     */

#if defined(sun) && !defined(__svr4__)
    t = timegm(&tm);
#else /* not sun, except svr4 */

    t = mktime(&tm);

/* BSD, have tm_gmtoff */
#if defined(SIGTSTP) && !defined(AIX) && !defined(__sgi) && !defined(_AUX) && !defined(__svr4__)
    {
	time_t cur_t = time(NULL);
	struct tm * local = localtime(&cur_t);
	t += local->tm_gmtoff;
	CTRACE(stderr,"TimeZone.... %02d hours from GMT\n",
	       (int)local->tm_gmtoff / 3600);
    }
#else /* SysV or VMS */
    {
#ifdef VMS
	CTRACE(stderr,"TimeZone.... undefined\n");
#else /* SysV */
#ifdef ISC3   /* Lauren */
	time_t cur_t = time(NULL);
	gorl = localtime(&cur_t);
	if (daylight && gorl->tm_isdst)	/* daylight time? */
	    z = altzone;	/* yes */
	else
	    z = timezone;	/* no */

	z /= 60;		/* convert to minutes */
	z = -z;			/* ISC 3.0 has it vice versa */
	t += z * 60;	
	CTRACE(stderr,"TimeZone.... %02d hours from GMT\n", z / 60);
#else
	int dst = 0;
	/*
	 * The following assumes a fixed DST offset of 1 hour,
	 * which is probably wrong.
	 */
	if (tm.tm_isdst > 0)
	    dst = -3600;
	t -= (timezone + dst);
	CTRACE(stderr,"TimeZone.... %02d hours from GMT\n", 
	       (timezone + dst) / 3600);
#endif
#endif /* SysV */
    }
#endif /* SysV or VMS */

#endif /* not sun, except svr4 */

    CTRACE(stderr, "Time string. %s", str);
    CTRACE(stderr, "Parsed...... to %ld seconds, %s", (long)t, ctime(&t));
    return t;
}


PUBLIC char * http_time ARGS1(time_t *, t)
{
    static char buf[40];
#if defined(Mips) || defined(_AUX) || (defined(VMS) && !defined(DECC))
    {
#if 1
	struct tm * gmt = gmtime(t);
	sprintf(buf,"%s, %02d %s 19%02d %02d:%02d:%02d GMT",
		wkdays[gmt->tm_wday],
		gmt->tm_mday,
		month_names[gmt->tm_mon],
		gmt->tm_year % 100,
		gmt->tm_hour,
		gmt->tm_min,
		gmt->tm_sec);
#else
	/* Mips, VAXC doesn't have strftime() :-( */
	sprintf(buf,"%s, %02d-%s-%02d %02d:%02d:%02d GMT",
		weekdays[gmt->tm_wday],
		gmt->tm_mday,
		month_names[gmt->tm_mon],
		gmt->tm_year % 100,
		gmt->tm_hour,
		gmt->tm_min,
		gmt->tm_sec);
    }
#endif
#else
#if defined(HT_REENTRANT) || defined(SOLARIS)
    {
	struct tm gmt;
	gmtime_r(t, &gmt);
    	strftime(buf, 40, "%a, %d %b %Y %H:%M:%S GMT", &gmt);
    }
#else
    {
	struct tm *gmt = gmtime(t);
    	strftime(buf, 40, "%a, %d %b %Y %H:%M:%S GMT", gmt);
    }
#endif /* SOLARIS || HT_REENTRANT */

#if 0
    strftime(buf, 40, "%A, %d-%b-%y %H:%M:%S GMT", gmt);
#endif
#endif

    return buf;
}


typedef struct _TimeUnit {
    char *	name;
    int		len;
    int		seconds;
} TimeUnit;

PRIVATE TimeUnit time_units[] =
{
    { "sec",	3, 1 },
    { "min",	3, 60 },
    { "hour",	4, 3600 },
    { "day",	3, 86400 },
    { "week",	4, 604800 },
    { "month",	5, 2592000 },	/* 30 days */
    { "year",	4, 31536000 },	/* 365 days */
    { "forever",7, 315360000 },	/* 10 years seems like forever */
    { NULL,	0, 0 }
};


PUBLIC char * make_time_spec ARGS1(time_t *, t)
{
    static char bufd[20];
    static char buft[10];
    time_t n = t ? *t : 0;
    int days = 0;
    int hours = 0;
    int mins = 0;
    int secs = 0;

    days = n / (24*3600);
    n -= days * 24*3600;
    hours = n / 3600;
    n -= hours * 3600;
    mins = n / 60;
    secs = n - 60*mins;

    if (hours)
	if (secs)
	    sprintf(buft,"%02d:%02d:%02d", hours, mins, secs);
	else if (mins)
	    sprintf(buft,"%02d:%02d", hours, mins);
	else
	    sprintf(buft,"%d hours", hours);
    else
	if (!secs)
	    sprintf(buft,"%d mins", mins);
	else if (!mins)
	    sprintf(buft,"%d secs", secs);
	else
	    sprintf(buft," 0:%02d:%02d", mins, secs);

    if (days > 0) {
	if (hours || mins || secs)
	    sprintf(bufd, "%d days %s", days, buft);
	else
	    sprintf(bufd, "%d days", days);
	return bufd;
    } else {
	return buft;
    }
}


#define STATE_HOUR 1
#define STATE_MINS 2
#define STATE_SECS 3

PUBLIC BOOL parse_time ARGS3(char *,	str,
			     int,	def,
			     time_t *,	tp)
{
    BOOL first = YES;
    int state = STATE_HOUR;
    char * p = str;
    *tp = 0;

    if (!str) return NO;
    while (*str && WHITE(*str)) str++;
    if (!*str) return NO;
    while (*p) {
	*p = TOLOWER(*p);
	p++;
    }
    while (--p >= str && WHITE(*p)) {
	*p = 0;
    }

    p = str;
    while (*p) {
	int i;
	int n = 0;
	BOOL found = NO;

	while (*p && WHITE(*p)) p++;
	while (*p && isdigit(*p)) {
	    n = 10*n + *p - '0';
	    p++;
	}
	while (*p && WHITE(*p)) p++;
	if (!*p && first) {
	    *tp = n * def;
	    break;
	}
	first = NO;
	for(i=0; time_units[i].name; i++) {
	    if (!strncmp(p, time_units[i].name, time_units[i].len)) {
		*tp += n * time_units[i].seconds;
		while (*p && ((*p>='a' && *p<='z') || *p==','))
		    p++;
		found = YES;
		break;
	    }
	}
	if (!found) {
	    if (!*p || *p++ == ':') {
		if (state==STATE_HOUR)
		    *tp += n * 3600;
		else if (state==STATE_MINS)
		    *tp += n * 60;
		else if (state==STATE_SECS)
		    *tp += n;
		else {
		    CTRACE(stderr,
		    "ERROR....... Too many colons in time specifier: %s\n",str);
		    return NO;
		}
		state++;
	    }
	    else {
		CTRACE(stderr,"ERROR....... Invalid time specifier: %s\n",str);
		return NO;
	    }
	}
    }
    return YES;
}


PUBLIC int parse_bytes ARGS2(char *,	str,
			     char,	def)
{
    char * ptr;
    long bytes = strtol(str,&ptr,10);

    while (*ptr && WHITE(*ptr)) ptr++;

    switch (*ptr ? TOUPPER(*ptr) : TOUPPER(def)) {
      case 'G':
	bytes *= 1024;
      case 'M':
	bytes *= 1024;
      case 'K':
	bytes *= 1024;
      case 'B':
	break;
      default:
	CTRACE(stderr,"Unknown letter after byte count: %s\n",str);
    }
    return (int)bytes;
}


PUBLIC char * get_http_reason ARGS1(int, status)
{
    switch (status) {
      case 200: return "OK";
      case 201: return "Created";
      case 202: return "Accepted";
      case 203: return "Partial information";
      case 204: return "No response";
      case 301: return "Moved";
      case 302: return "Found";
      case 303: return "Method";
      case 304: return "Not modified";
      case 400: return "Bad request";
      case 401: return "Unauthorized";
      case 402: return "Payment required";
      case 403: return "Forbidden";
      case 404: return "Not found";
      case 500: return "Internal error";
      case 501: return "Not implemented";
      default: return "Oops, dunno...";
    }
}


/*
 *	Send a string down a socket
 */
PUBLIC int HTWriteASCII ARGS2(int, soc, char *, s)
{
    if (!s) return -1;
    else {
	int len = strlen(s);
#ifdef NOT_ASCII
	char * p;
	char * ascii = (char*)malloc(len+1);
	char * q = ascii;
	int status;

	if (!ascii) outofmem(__FILE__, "HTWriteASCII");

	for (p=s; *p; p++)
	    *q++ = TOASCII(*p);

	status = NETWRITE(soc, ascii, len);
	free(ascii);
	return status;
#else
        return NETWRITE(soc, s, len);
#endif
    }
}


/*
 * This function is borrowed from NCSA httpd and was written
 * by Rob McCool.
 */
#ifdef NEED_INITGROUPS
PUBLIC int initgroups ARGS2(CONST char *,	name,
			    gid_t,		basegid)
{
    gid_t groups[NGROUPS_MAX];
    struct group *g;
    int index = 0;

    groups[index++] = basegid;

    while (index < NGROUPS_MAX && ((g = getgrent()) != NULL)) {
	if (g->gr_gid != basegid) {
	    char ** names;

	    for (names = g->gr_mem; *names != NULL; ++names)
		if (!strcmp(*names, name))
		    groups[index++] = g->gr_gid;
	}
    }
    return setgroups(index, groups);
}
#endif
