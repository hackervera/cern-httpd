
/* MODULE							HTCache.c
**		CACHE MANAGER FOR GATEWAY HTTPD
**
** AUTHORS:
**	AL	Ari Luotonen	luotonen@dxcern.cern.ch
**	FM	Fote Macrides	macrides@sci.wfeb.edu
**
** HISTORY:
**	31 Jan 94  AL	Written from scratch on a *very* beautiful
**			Sunday afternoon -- seems like the spring
**			is already coming, yippee!
**	 8 Jul 94  FM	Insulate free() from _free structure element.
**
** BUGS:
**
**
*/

#ifdef VMS
#define __TYPES
#include <types.h>
#include <time.h>
#define __TIME
#else /* not VMS */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#endif /* not VMS */

#include "HTFile.h"
#include "HTUtils.h"
#include "HTParse.h"	/* HTEscape() */

#include "tcp.h"
#include "HTTCP.h"

#include "HTFormat.h"	/* HTInputSocket */
#include "HTAccess.h"
#include "HTRules.h"
#include "HTWriter.h"
#include "HTStream.h"
#include "HTCache.h"
#include "HTConfig.h"
#include "HTDaemon.h"
#include "HTLog.h"

#undef TRACE
#undef CTRACE
#define TRACE	(WWW_TraceFlag || trace_cache)
#define CTRACE	if(TRACE)fprintf

#define BUF_SIZE 256

#define PUT_CHAR(c)	\
    (*me->output_stream->isa->put_character)(me->output_stream,c)
#define PUT_STRING(s)	\
    (*me->output_stream->isa->put_string)(me->output_stream,s)
#define PUT_BLOCK(b,l)	\
    (*me->output_stream->isa->put_block)(me->output_stream,b,l)


typedef enum {
    CW_STATUS_LINE = 0,
    CW_HEADERS,
    CW_SKIP,
    CW_COPY_BODY,
    CW_UPDATED,		/* With If-Modified-Since: */
    CW_JUNK
} CacheWriterState;

/************************************************************************
**
**	STREAM FOR CACHING
**
**	Mapping between URLs and cache files
**
*************************************************************************
*/
struct _HTStream {
    CONST HTStreamClass *	isa;
    HTStream *			output_stream;
    time_t			if_mod_since;
    char *			url;
    char *			cfn;
    FILE *			cf;
    int				status;
    int				content_count;
    CacheWriterState		state;
    time_t			expires;
    time_t			last_modified;
    time_t			transfer_started;
    char *			end;
    char *			last;
    char			buf[BUF_SIZE+2];
    HTList *			header_buf;
    int				header_buf_size;
};


PUBLIC int HTExitStatusToKilos ARGS1(int, status)
{
    if (status < 100)		return status;
    else if (status < 200)	return (status-100)*10 + 100;
    else if (status < 250)	return (status-200)*100 + 1000;
    else if (status < 255)	return (status-250)*1000 + 5000;
    else			return 10000;
}


PUBLIC int HTBytesToExitStatus ARGS1(long, bytes)
{
    int kilos = bytes / 1024;

    if (kilos < 100)		return kilos + 1;
    else if (kilos < 1000)	return 100 + (kilos-100)/10;
    else if (kilos < 5000)	return 200 + (kilos-1000)/100;
    else if (kilos < 10000)	return 250 + (kilos-5000)/1000;
    else			return 255;
}


PRIVATE char * lockname ARGS1(char *, cfn)
{
    char * lockfile = NULL;

    if (cfn) {
	lockfile = (char*)malloc(strlen(cfn) + strlen(LOCK_SUFFIX) + 1);
	if (!lockfile) outofmem(__FILE__, "locked");
	sprintf(lockfile, "%s%s", cfn, LOCK_SUFFIX);
    }
    return lockfile;
}


PRIVATE BOOL locked ARGS1(char *, cfn)
{
    char * lockfile = lockname(cfn);
    struct stat stat_info;
    BOOL ret = NO;

    if (!lockfile) return YES;

    if (HTStat(lockfile, &stat_info) != -1) {
	if (cur_time - stat_info.st_mtime > cc.cache_lock_timeout) {
	    CTRACE(stderr,
	    "Lock........ timed out after %ld minutes, breaking it: %s\n",
		   (long)cc.cache_lock_timeout/60, lockfile);
	    unlink(lockfile);
	}
	else {
	    CTRACE(stderr, "Locked...... by another process: %s\n", lockfile);
	    ret = YES;
	}
    }
    free(lockfile);
    return ret;
}


PRIVATE FILE * do_lock ARGS1(char *, cfn)
{
    char * lockfile = lockname(cfn);
    FILE * fp = NULL;

    if (lockfile) {
	int fd = open(lockfile, O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (fd == -1) {
	    CTRACE(stderr,
		   "FAILED...... to create lock file (already locked?): %s\n",
		   lockfile);
	}
	else {
	    CTRACE(stderr, "Lockfile.... created: %s\n", lockfile);
	    fp = fdopen(fd, "w");
	}
	free(lockfile);
    }
    return fp;
}


/************************************************************************
**
**	FILENAME GENERATION
**
**	Mapping between URLs and cache files
**
*************************************************************************
*/


/*
**	Replace given directory name with directory index name.
*/
PRIVATE void replace_with_indexname ARGS1(char **, dirname)
{
    char * indexname;

    if (!dirname || !*dirname) return;

    indexname = (char*)malloc(strlen(*dirname) + strlen(INDEX_FILE) + 2);
    sprintf(indexname, "%s/%s", *dirname, INDEX_FILE);
    free(*dirname);
    *dirname = indexname;
}



/*
**	Check that the name we're about to generate doesn't
**	clash with anything used by the caching system.
*/
PRIVATE BOOL reserved_name ARGS1(char *, url)
{
    char * name = strrchr(url, '/');
    char * suff = NULL;

    if (name) name++;
    else name = url;

    if (!strcmp(name, CACHE_INFO) ||
	!strcmp(name, INDEX_FILE) ||
	!strcmp(name, WELCOME_FILE))
	return YES;

    suff = strrchr(name, TMP_SUFFIX[0]);
    if (suff && !strcmp(suff, TMP_SUFFIX))
	return YES;

    suff = strrchr(name, LOCK_SUFFIX[0]);
    if (suff && !strcmp(suff, LOCK_SUFFIX))
	return YES;

    return NO;
}


/*
**	Map url to cache file name.
*/
PRIVATE char * cache_file_name ARGS1(char *, url)
{
    char * access = NULL;
    char * host = NULL;
    char * path = NULL;
    char * cfn = NULL;
    BOOL welcome = NO;
    BOOL res = NO;

    if (!url ||  strchr(url, '?')  ||  (res = reserved_name(url))  ||
	!(access = HTParse(url, "", PARSE_ACCESS)) ||
	(0 != strcmp(access, "http") &&
	 0 != strcmp(access, "ftp")  &&
	 0 != strcmp(access, "gopher"))) {

	if (access) free(access);

	if (res && TRACE)
	    fprintf(stderr,
		    "Cache....... Clash with reserved name (\"%s\")\n",url);

	return NULL;
    }

    host = HTParse(url, "", PARSE_HOST);
    path = HTParse(url, "", PARSE_PATH | PARSE_PUNCTUATION);
    if (path && path[strlen(path)-1] == '/')
	welcome = YES;

    if (host) {		/* Canonilize host name */
	char * cur = host;
	while (*cur) {
	    *cur = TOLOWER(*cur);
	    cur++;
	}
    }

    cfn = (char*)malloc(strlen(cc.cache_root) +
			strlen(access) +
			(host ? strlen(host) : 0) +
			(path ? strlen(path) : 0) +
			(welcome ? strlen(WELCOME_FILE) : 0) + 3);
    if (!cfn) outofmem(__FILE__, "cache_file_name");
    sprintf(cfn, "%s/%s/%s%s%s", cc.cache_root, access, host, path,
	    (welcome ? WELCOME_FILE : ""));

    FREE(access); FREE(host); FREE(path);

    /*
    ** This checks that the last component is not too long.
    ** It could check all the components, but the last one
    ** is most important because it could later blow up the
    ** whole gc when reading cache info files.
    ** Operating system handles other cases.
    ** 64 = 42 + 22  and  22 = 42 - 20  :-)
    ** In other words I just picked some number, it doesn't
    ** really matter that much.
    */
    {
	char * last = strrchr(cfn, '/');
	if (!last) last = cfn;
	if ((int)strlen(last) > 64) {
	    CTRACE(stderr, "Too long.... cache file name \"%s\"\n", cfn);
	    free(cfn);
	    cfn = NULL;
	}
    }
    return cfn;
}


/*
**	Create directory path for cache file
**
** On exit:
**	return YES
**		if directories created -- after that caller
**		can rely on fopen(cfn,"w") succeeding.
**
*/
PRIVATE BOOL create_cache_place ARGS1(char *, cfn)
{
    struct stat stat_info;
    char * cur = NULL;
    BOOL create = NO;

    if (!cfn  ||  (int)strlen(cfn) <= (int)strlen(cc.cache_root) + 1)
	return NO;

    cur = cfn + strlen(cc.cache_root) + 1;

    while ((cur = strchr(cur, '/'))) {
	*cur = 0;
	if (create || HTStat(cfn, &stat_info) == -1) {
	    create = YES;	/* To avoid doing stat()s in vain */
	    CTRACE(stderr, "Cache....... creating cache dir \"%s\"\n", cfn);
	    if (-1 == mkdir(cfn, 0775)) {
		CTRACE(stderr, "Cache....... can't create dir \"%s\"\n", cfn);
		return NO;
	    }
	}
	else {
	    if (S_ISREG(stat_info.st_mode)) {
		int len = strlen(cfn);
		char * tmp1 = (char*)malloc(len + strlen(TMP_SUFFIX) + 1);
		char * tmp2 = (char*)malloc(len + strlen(INDEX_FILE) + 2);
		time_t t1,t2,t3,t4,t5;

		sprintf(tmp1, "%s%s", cfn, TMP_SUFFIX);
		sprintf(tmp2, "%s/%s", cfn, INDEX_FILE);

		if (TRACE) {
		    fprintf(stderr,"Cache....... moving \"%s\" to \"%s\"\n",
			    cfn,tmp1);
		    fprintf(stderr,"and......... creating dir \"%s\"\n",
			    cfn);
		    fprintf(stderr,"and......... moving \"%s\" to \"%s\"\n",
			    tmp1,tmp2);
		}
		rename(cfn,tmp1);
		mkdir(cfn,0775);
		rename(tmp1,tmp2);

		if (HTCacheInfo_for(cfn,&t1,&t2,&t3,&t4,&t5)) {
		    CTRACE(stderr,"Adding...... info entry for %s\n",tmp2);
		    if (!HTCacheInfo_writeEntryFor(tmp2,t1,t2,t3,t4,t5))
			HTLog_error2("Can't write cache info entry for",tmp2);
		}

		free(tmp1);
		free(tmp2);
	    }
	    else {
		CTRACE(stderr,"Cache....... dir \"%s\" already exists\n",cfn);
	    }
	}
	*cur = '/';
	cur++;
    }
    return YES;
}


PRIVATE BOOL do_caching ARGS1(char *, url)
{
    HTList * cur = cc.no_caching;
    HTPattern * pat = NULL;

    if (!url) return NO;

    /*
     *	First see if caching is explicitly disabled for this URL
     */
    while ((pat = (HTPattern *)HTList_nextObject(cur))) {
	if (HTPattern_url_match(pat,url)) {
	    CTRACE(stderr, "No caching.. due to NoCaching config directive\n");
	    return NO;
	}
    }


    /*
     *	Then look if caching is only done selectively
     */
    if (!cc.cache_only) {
	CTRACE(stderr,
	"Caching..... 'cause didn't match NoCaching and no CacheOnly given\n");
	return YES;	/* Not selective */
    }
    cur = cc.cache_only;
    while ((pat = (HTPattern *)HTList_nextObject(cur))) {
	if (HTPattern_url_match(pat,url)) {
	    CTRACE(stderr,"Caching..... 'cause matched CacheOnly directive\n");
	    return YES;
	}
    }
    CTRACE(stderr,"Not caching. because didn't match CacheOnly directives\n");
    return NO;
}


/*
**	Look up a cache filename for url
**
** On entry:
**	url	for which we need the cache.
**	cfn	must point to valid char* storage.
**
** On exit:
**	returns CACHE_NO	if cache file doesn't exist nor should
**				it be created.
**
**		CACHE_FOUND	if cache file exists, and *cfn the name of
**				the file from which document can be read,
**				and *expires is the expiry date of this
**				cache item (needed, since Expires: header
**				needs to be filled out by the loader).
**
**		CACHE_IF_MODIFIED if cache file exists but may be out of date.
**				Caller should do a conditional GET with
**				If-Modified-Since header, *if_ms is the date.
**				*cfn is the cache file name and *cf is an
**				open lockfile to which document should be
**				written if it has expired.  After that
**				one of functions cache_not_modified(),
**				cache_created() or cache_cancel() should
**				be called.
**
**		CACHE_CREATE	if cache file didn't exist but should be
**				created.  *cfn is the cache file name and
**				*cf is an open lock file to write the document
**				to.  After this either cache_created() or
**				cache_cancel() should be called.
*/
PUBLIC HTCacheTask cache_lookup ARGS5(char *,	url,
				      char **,	cfn,
				      FILE **,	cf,
				      time_t *,	if_ms,
				      time_t *,	expires)
{
    struct stat stat_info;

    if (!url || !cfn || !cf || !if_ms) return CACHE_NO;
    *cfn = NULL;
    *cf = NULL;

    if (!do_caching(url)) {
	CTRACE(stderr, "No caching.. for %s\n", url);
	return CACHE_NO;
    }

    CTRACE(stderr, "Cache....... Looking up cache file for \"%s\"\n", url);
    *cfn = cache_file_name(url);
    if (!*cfn) return CACHE_NO;

    if (locked(*cfn)) {
	CTRACE(stderr, "Cache....... file is locked: \"%s\"\n", *cfn);
	FREE(*cfn);
	return CACHE_NO;
    }

    while (HTStat(*cfn, &stat_info) != -1) {
	if (S_ISREG(stat_info.st_mode)) {
	    BOOL success = NO;
	    time_t ld = 0;
	    time_t lc = 0;
	    time_t ex = 0;
	    time_t mu = 0;
	    time_t lm = 0;
	    time_t refresh_interval = get_refresh_interval(url);

	    if (!cc.do_exp_check && !in.no_cache_pragma) {
		CTRACE(stderr, "ExpiryCheck. is off -- cache file found\n");
		cache_hit = YES;
		return CACHE_FOUND;
	    }

	    success = HTCacheInfo_for(*cfn, &ld, &lc, &ex, &mu, &lm);
	    if (!success				  /* no entry */
		|| ex - cc.cache_time_margin <= cur_time  /* expired */
		|| cur_time - lc >= refresh_interval	  /* time to refresh */
		|| in.no_cache_pragma) {		  /* override cache */

		char * backup = NULL;

		if (TRACE) {
		    if (!success)
			fprintf(stderr, "NoEntry..... %s -- expiring\n",*cfn);
		    else if (in.no_cache_pragma)
			fprintf(stderr, "Forced...... refresh of %s\n",*cfn);
		    else if (ex - cc.cache_time_margin <= cur_time)
			fprintf(stderr, "Expired..... %s\n", *cfn);
		    else
			fprintf(stderr, "Time to..... refresh (every %s)\n",
				make_time_spec(&refresh_interval));
		}

		/*
		 * If we are running on top of a standalone cache [no
		 * external document retrieves] but expiry checking is
		 * turned on, return different error message to client.
		 * CACHE_IF_MODIFIED in cache_no_connect mode will be
		 * a signal for the caller about this.  Other values
		 * returned in return parameters are illegal.
		 */
		if (cc.cache_no_connect) {
		    CTRACE(stderr, "Standalone.. caching mode but expired\n");
		    cache_hit = YES;
		    return CACHE_IF_MODIFIED;
		}

		if (!(*cf = do_lock(*cfn))) {
		    FREE(*cfn);
		    return CACHE_NO;
		}

		backup = (char*)malloc(strlen(*cfn) + strlen(TMP_SUFFIX) + 1);
		if (!backup) outofmem(__FILE__, "cache_lookup");
		strcpy(backup, *cfn);
		strcat(backup, TMP_SUFFIX);

		CTRACE(stderr,"Cache....... backing up: %s\n",*cfn);
		rename(*cfn,backup);

		*if_ms = lm ? lm : lc;	/* If no Last-Modified use last check*/

		CTRACE(stderr,"Current..... time: %s", ctime(&cur_time));
		CTRACE(stderr,"Expiry...... time: %s", ctime(&ex));
		CTRACE(stderr,"IfModSince.. time: %s", ctime(if_ms));

		free(backup);
		return CACHE_IF_MODIFIED;
	    }
	    else {
		CTRACE(stderr, "Cache....... not expired %s\n", *cfn);
		*expires = ex;
		cache_hit = YES;
		return CACHE_FOUND;
	    }
	}
	else if (S_ISDIR(stat_info.st_mode))
	    replace_with_indexname(cfn);
	else {
	    CTRACE(stderr,"Cache....... Not following \"%s\"\n", *cfn);
	    FREE(*cfn);
	    return CACHE_NO;
	}
    }

    if (!cc.cache_no_connect &&
	create_cache_place(*cfn) && (*cf = do_lock(*cfn)))
	return CACHE_CREATE;
    else {
	FREE(*cfn);
	return CACHE_NO;
    }
}


PUBLIC void cache_cancel ARGS1(HTStream *, me)
{
    char * lockfile = NULL;
    struct stat stat_info;

    if (!me || !me->url) return;

    CTRACE(stderr,"Cache....... Canceling \"%s\"\n", me->url);

    lockfile = cache_file_name(me->url);

    if (lockfile) {
	StrAllocCat(lockfile, LOCK_SUFFIX);
	if (HTStat(lockfile,&stat_info) != -1  && S_ISREG(stat_info.st_mode)) {
	    CTRACE(stderr, "Removing.... cache lock \"%s\"\n", lockfile);
	    unlink(lockfile);
	}
	FREE(lockfile);
    }
}


PUBLIC void cache_not_modified ARGS2(HTStream *,	me,
				     time_t,		cur_t)
{
    time_t junk1, junk2, junk3;
    time_t transfer_time = 0;
    time_t last_mod = 0;

    if (!me || !me->url || !me->cfn) return;

    /*
     * Just to get the old transfer time.
     */
    if (!HTCacheInfo_for(me->cfn, &transfer_time,
			 &junk1, &junk2, &junk3, &last_mod)) {
	HTLog_error2("Things are going badly wrong - can't get cache info for",
		     me->cfn);
	/* But ok, let's still continue, only transfer_time will be zero. */
    }

    /*
     * Remove lock file and restore the backup file
     */
    {
	char * name = (char*)malloc(strlen(me->cfn) + strlen(LOCK_SUFFIX) +
				    strlen(TMP_SUFFIX) + 1);
	if (!name) outofmem(__FILE__, "cache_not_modified");

	sprintf(name, "%s%s", me->cfn, TMP_SUFFIX);
	CTRACE(stderr, "Restoring... backup version \"%s\"\n", name);
	rename(name, me->cfn);

	sprintf(name, "%s%s", me->cfn, LOCK_SUFFIX);
	CTRACE(stderr, "Removing.... lock file \"%s\"\n", name);
	unlink(name);
	FREE(name);
    }

    /*
     * Update cache info file
     */
    CTRACE(stderr,"Updating.... cache attributes for \"%s\"\n",me->cfn);
    if (!HTCacheInfo_writeEntryFor(me->cfn,
				   transfer_time,
				   cur_t,
				   me->expires,
				   get_unused(me->url),
				   last_mod)) {
	HTLog_error2("Can't write cache info entry for", me->cfn);
    }
}


PUBLIC void cache_created ARGS2(HTStream *,	me,
				time_t,		cur_t)
{
    if (!me || !me->url || !me->cfn) return;

    /*
     * Release lock
     */
    {
	char * lockfile = (char*)malloc(strlen(me->cfn)+strlen(LOCK_SUFFIX)+1);
	if (!lockfile) outofmem(__FILE__, "cache_created");
	sprintf(lockfile, "%s%s", me->cfn, LOCK_SUFFIX);

	rename(lockfile, me->cfn);
	free(lockfile);
    }

    /*
     * If we just did an IMS-GET remove the backup file -- it is
     * really out of date.
     */
    if (me->if_mod_since) {
	char * backup = (char*)malloc(strlen(me->cfn) + strlen(TMP_SUFFIX) +1);
	if (!backup) outofmem(__FILE__, "cache_created");
	sprintf(backup, "%s%s", me->cfn, TMP_SUFFIX);
	CTRACE(stderr, "Discarding.. backup version \"%s\"\n", backup);
	unlink(backup);
	free(backup);
    }

    /*
     * Write entry to cache info file
     */
    CTRACE(stderr, "Created..... cache file \"%s\"\n", me->cfn);
    if (!HTCacheInfo_writeEntryFor(me->cfn,
				   cur_t - me->transfer_started,
				   cur_t,
				   me->expires,
				   get_unused(me->url),
				   me->last_modified)) {
	HTLog_error2("Can't write cache info entry for",me->cfn);
    }
}


PRIVATE void buf_append_and_clear ARGS1(HTStream *, me)
{
    char * stuff = NULL;
    if (!me) return;

    StrAllocCopy(stuff, me->buf);
    if (!me->header_buf) me->header_buf = HTList_new();
    HTList_addObject(me->header_buf, (void*)stuff);
    me->header_buf_size += strlen(stuff);

    me->end = me->buf;
    *me->end = 0;
}

/*
 * With HTTP0 replies we have to dump the (possibly binary) data
 * to the remote client as it came in.
 */
PRIVATE void buf_dump ARGS1(HTStream *, me)
{
    if (me->output_stream)
	PUT_BLOCK(me->buf, me->end - me->buf);
}

PRIVATE void buf_discard ARGS1(HTStream *, me)
{
    HTList * cur;
    char * stuff;

    if (!me || !me->header_buf) return;

    cur = me->header_buf;
    while ((stuff = (char*)HTList_nextObject(cur)))
	free(stuff);
    HTList_delete(me->header_buf);
    me->header_buf = NULL;
    me->header_buf_size = 0;
}

PRIVATE char * doit ARGS2(char *,	buffer,
			  HTList *,	cur)
{
    char * my_place = NULL;

    if (!buffer || !cur) return buffer;

    if (cur->next) my_place = doit(buffer,cur->next);
    if (!my_place) my_place = buffer;
    if (cur->object) {
	strcpy(my_place, (char*)cur->object);
	return my_place + strlen((char*)cur->object);
    }
    else return my_place;
}


PRIVATE void buf_flush ARGS1(HTStream *, me)
{
    char * buffer = NULL;

    if (!me || !me->header_buf) return;

    if (!me->output_stream) {
	buf_discard(me);
	return;
    }

    buffer = malloc(me->header_buf_size + 1);
    if (!buffer) outofmem(__FILE__, "buf_flush");
    *buffer = 0;
    doit(buffer, me->header_buf->next);

    if (me->state != CW_JUNK  &&  me->cf)
	fputs(buffer, me->cf);

    if (me->output_stream)
	PUT_STRING(buffer);

    free(buffer);
    buf_discard(me);
}


PRIVATE int get_status ARGS1(char *, buf)
{
    char * start = NULL;

    if (!buf || 0!=strncmp(buf, "HTTP/", 5) ||
	!(start = strchr(buf, ' '))) {
	CTRACE(stderr, "DARN........ I got HTTP0 response back - how rude!\n");
	return -1;
    }

    while (*start && (*start==' ' || *start=='\t'))
	start++;

    if (!*start)
	return -1;
    else
	return atoi(start);
}


/*
 * Figures out the expiry date if not given by the remote server.
 * Uses LM factor if given, otherwise default expiry.
 *
 * Returns YES if ok to write to cache.
 * Returns NO if expiry time is so short that it's not worth caching.
 */
PRIVATE BOOL figure_out_expires ARGS1(HTStream *, me)
{
    time_t t_cur;
    time(&t_cur);

    if (me->expires) {
	/*
	 * Expires: given => great, use it!
	 */
	CTRACE(stderr,"Expires..... %s", ctime(&me->expires));
    }
    else {
	/*
	 * No Expires: given => approximate
	 */
	float lm_factor = -1;
	time_t time_to_live = 0;

	CTRACE(stderr, "No expires.. given by remote\n");

	if (!me->last_modified && me->if_mod_since) {
	    /*
	     * If last-modified was not given but we did a conditional
	     * (if-modified-since) GET and got back "304 Not modified"
	     * then use the old last-modified
	     */
	    CTRACE(stderr,"L-M......... not given by remote, using old: %s",
		   ctime(&me->if_mod_since));
	    me->last_modified = me->if_mod_since;
	}

	/*
	 * Get LM factor (last-modified factor)
	 */
	if (me->last_modified)
	    lm_factor = get_lm_factor(me->url);

	if (lm_factor > 0) {
	    /*
	     * Use last-modified and LM factor to calculate expiry date
	     */
	    time_to_live = lm_factor * (t_cur - me->last_modified);
	    if (TRACE) {
		time_t t = t_cur - me->last_modified;
		fprintf(stderr,"Modified.... %s ago\n", make_time_spec(&t));
		fprintf(stderr,"LM factor... %f\n", lm_factor);
		fprintf(stderr,"Lifetime.... [LM approximation] %s\n",
			make_time_spec(&time_to_live));
	    }
	}
	else {
	    /*
	     * No last-modified or LM factor => use default expiry
	     */
	    time_to_live = get_default_expiry(me->url);
	    CTRACE(stderr,"Lifetime.... [default expiry] %s\n",
		   make_time_spec(&time_to_live));
	}

	if (time_to_live > cc.cache_time_margin) {
	    me->expires = t_cur + time_to_live;
	    CTRACE(stderr, "Approx...... expires %s", ctime(&me->expires));
	}
	else {
	    CTRACE(stderr, "Not worth... caching, would expire too soon\n");
	    return NO;	/* Not worth caching */
	}
    }

    if (!me->last_modified) {
	me->last_modified = t_cur;
	CTRACE(stderr,"Using....... current time as Last-Modified\n");
    }

    return YES;	/* Yep, is worth writing to cache */
}


PRIVATE void cache_put_char ARGS2(HTStream *,	me,
				  char,		ch)
{
    if (me->state==CW_COPY_BODY || me->state==CW_JUNK)
	me->content_count++;

    switch (me->state) {

      case CW_SKIP:
	if (ch==LF) me->state = CW_HEADERS;
	/* and fall thru */

      case CW_COPY_BODY:
	putc(ch, me->cf);
	/* and fall thru */

      case CW_JUNK:
	if (me->output_stream) PUT_CHAR(ch);
	break;

      case CW_UPDATED:
	break;

      case CW_STATUS_LINE:
      case CW_HEADERS:
	*(me->end++) = ch;
	*(me->end) = 0;

	if (ch == LF || me->end >= me->last) {
	    if (me->state == CW_STATUS_LINE) {
		me->status = get_status(me->buf);
		if (me->status != 200 &&
		    (me->status != 304 || !me->if_mod_since)) {
		    me->state = CW_JUNK;
		    buf_dump(me);
		}
	    }
	    else {
		if (*me->buf==CR || *me->buf==LF) {	/* End of headers */

		    if (me->status == 304) {	/* 304 Not modified */
			CTRACE(stderr,"Not modified response from remote\n");
			CTRACE(stderr,"Updating.... cache attributes\n");
			figure_out_expires(me);
			me->state = CW_UPDATED;
			buf_discard(me);
		    }
		    else if (me->status == 200) {
			if (figure_out_expires(me))
			    me->state = CW_COPY_BODY;	/* Caching ok */
			else
			    me->state = CW_JUNK;	/* Not worth caching */
			buf_append_and_clear(me);
			if (HTClientProtocol)
			    buf_flush(me);	/* Send HTTP1 headers */
			else {
			    CTRACE(stderr,
				   "Junking..... headers [sending HTTP0]\n");
			    buf_discard(me);	/* Blah, send HTTP0 reply */
			}
		    }
		    break;
		}
		else if (!strncasecomp(me->buf, "expires:", 8)) {
		    me->expires = parse_http_time(me->buf + 8);
		    if (me->expires <= cur_time + cc.cache_time_margin) {
			CTRACE(stderr, "Expires..... IMMEDIATELY\n");
			me->state = CW_JUNK;
			buf_append_and_clear(me);
			buf_flush(me);
			break;
		    }
		    else {
			/*
			 * Expires: field will be followed by a 33 char string
			 * that will be filled-out by the proxy when sending
			 * stuff from cache to clients.
			 */
			strcpy(me->buf,
			"Expires: Will-be-filled-out-by-cache-proxy\r\n");
			buf_append_and_clear(me);
		    }
		}
		else if (!strncasecomp(me->buf, "last-modified:", 14)) {
		    me->last_modified = parse_http_time(me->buf + 14);
		}
		else if (!strncasecomp(me->buf, "content-length:", 15)) {
		    out.content_length = atoi(me->buf + 15);
		    CTRACE(stderr, "Remote...... gives Content-Length: %d\n",
			   out.content_length);
		}
	    }

	    if (me->state != CW_COPY_BODY  &&  me->state != CW_UPDATED  &&
		me->state != CW_SKIP  &&  me->state != CW_JUNK) {
		me->state = CW_HEADERS;
	    }

	    if (ch!=LF && me->state!=CW_COPY_BODY && me->state!=CW_UPDATED) {
		me->state = CW_SKIP;
	    }

	    buf_append_and_clear(me);
	}
	break;

      default:
	CTRACE(stderr, "BUG......... cache_put_character (%d as enum value)\n",
	       (int)me->state);
    }
}


PRIVATE void cache_write ARGS3(HTStream *,	me,
			       CONST char *,	buf,
			       int,		len)
{
    CONST char * cur = buf;

    if (me->output_stream && sigpipe_caught) {
	CTRACE(stderr,
	       "Interrupted. client connection [SIGPIPE], aborting sending\n");
	(*me->output_stream->isa->abort)(me->output_stream, NULL);
	me->output_stream = NULL;
	CTRACE(stderr,
	       "Renicing.... to lower priority (no user waiting anymore)\n");
	nice(4);
    }

    if (me->state == CW_UPDATED)
	return;

    if (me->state == CW_JUNK) {
	if (me->output_stream) PUT_BLOCK(buf,len);
	me->content_count += len;
	return;
    }

    while (len > 0 && (me->state != CW_COPY_BODY  &&
		       me->state != CW_UPDATED  &&
		       me->state != CW_JUNK)) {
	cache_put_char(me, *(cur++));
	len--;
    }
    if (len > 0) {
	if (me->state == CW_COPY_BODY)
	    fwrite(cur, 1, len, me->cf);
	if (me->state == CW_COPY_BODY || me->state == CW_JUNK) {
	    if (me->output_stream) PUT_BLOCK(cur,len);
	    me->content_count += len;
	}
    }
}

PRIVATE void cache_put_str ARGS2(HTStream *,	me,
				 CONST char *,	str)
{
    if (str) {
	int len = strlen(str);
	cache_write(me,str,len);
    }
}



PRIVATE void cache_free ARGS1(HTStream *, me)
{
    struct stat stat_info;

    if (out.status_code == 200 && me->status != 200 && me->status != 304)
	out.status_code = me->status;

    if (me->state == CW_COPY_BODY) {
	if (fstat(fileno(me->cf), &stat_info) == -1)
	    HTLog_error2("Can't fstat() new cache file:",me->cfn);
	else {
	    HTCachedBytes = stat_info.st_size;
	    HTChildExitStatus = HTBytesToExitStatus(HTCachedBytes);
	}
    }

    fclose(me->cf);
    if (!me->url) goto clean_up;

    if (me->state == CW_COPY_BODY  ||  me->state == CW_UPDATED) {

	if (me->state == CW_COPY_BODY && out.content_length > 0 &&
	    me->content_count != out.content_length) {
	    HTLog_error2("Content-Length mismatch when retrieved",me->url);
	    CTRACE(stderr, "Expected.... %d, actual %d\n",
		   out.content_length, me->content_count);
	    cache_cancel(me);
	}
	else {

	    time_t t_cur;
	    time(&t_cur);

	    if (TRACE && me->state == CW_COPY_BODY) {
		if (out.content_length > 0)
		    fprintf(stderr,
			    "Correct..... content-length received: %d\n",
			    out.content_length);
		else
		    fprintf(stderr,"Calculated.. content-length: %d\n",
			    me->content_count);
	    }

	    if (me->state == CW_UPDATED) {
		cache_not_modified(me,t_cur);
		if (me->output_stream) {
		    CTRACE(stderr,"Opening..... old cache file %s\n", me->cfn);
		    me->cf = fopen(me->cfn, "r");
		    if (me->cf) {
			CTRACE(stderr, "Sending..... from cache\n");
			cache_hit = YES;
			HTLoadCacheToStream(me->cf, me->output_stream,
					    me->expires);
			fclose(me->cf);
		    }
		    /* else: Here should be an error-load to stream */
		}
		else {
		    CTRACE(stderr,
			   "No more..... connection to client, I'm thru!\n");
		}
	    }
	    else {
		cache_created(me, t_cur);
	    }
	}
    }
    else
	cache_cancel(me);
    free(me->url);

  clean_up:
#if 0
    buf_discard(me);
#endif
    if (me->cfn) free(me->cfn);
    if (me->output_stream) {
	/*
	 * Make sure the correct content-length is logged; however,
	 * we do it before we free our sink, since that may be an
	 * If-Modified-Since-stream or a HEAD-stream, which may
	 * further modify the content-length (reset it to zero).
	 */
	if (me->state == CW_COPY_BODY || me->state == CW_JUNK) {
	    out.content_length = me->content_count;
	}
	(*me->output_stream->isa->_free)(me->output_stream);
    }
    free(me);
}

PRIVATE void cache_abort ARGS2(HTStream *,	me,
			       HTError,		err)
{
    me->state = CW_JUNK;
    if (me->output_stream) {
	(*me->output_stream->isa->abort)(me->output_stream,err);
	me->output_stream = NULL;
    }
    cache_free(me);
}

PRIVATE CONST HTStreamClass HTProxyCacheClass =
{
    "ProxyCache",
    cache_free,
    cache_abort,
    cache_put_char,
    cache_put_str,
    cache_write
};

PUBLIC HTStream * HTProxyCache ARGS5(HTStream *,	output_stream,
				     FILE *,		cf,
				     char *,		cfn,
				     char *,		url,
				     time_t,		if_ms)
{
    HTStream * me = (HTStream *)calloc(1, sizeof(HTStream));
    if (!me) outofmem(__FILE__, "HTProxyCache");

    me->isa = &HTProxyCacheClass;
    me->output_stream = output_stream;
    me->if_mod_since = if_ms;
    StrAllocCopy(me->url,url);
    StrAllocCopy(me->cfn,cfn);
    me->cf = cf;
    me->state = CW_STATUS_LINE;
    me->status = 0;
    me->end = me->buf;
    me->last = me->buf + BUF_SIZE;
    time(&me->transfer_started);

    return me;
}


/*
 *	This function loads cache file to a stream,
 *	stripping headers for HTTP0 clients, and
 *	filling out Expires: field according to cache info.
 *
 * IMPORTANT:
 *	INPUT_BUFFER_SIZE (currently seems to be 4096) must be
 *	large enough to contain Expires: header in the first
 *	buffer load.  In practise this is always the case.
 *	This way we can just do a string search, rather than
 *	parse the whole header.
 */
PUBLIC void HTLoadCacheToStream ARGS3(FILE *,		fp,
				      HTStream*,	sink,
				      time_t,		expires)
{
    HTStreamClass targetClass;    
    char input_buffer[INPUT_BUFFER_SIZE+1];
    BOOL first = YES;
    BOOL headers = YES;
    BOOL flag = NO;

    targetClass = *(sink->isa);	/* Copy pointers to procedures */
    out.header_length = 0;
    out.content_length = 0;

    for(;;) {
	int status = fread(input_buffer, 1, INPUT_BUFFER_SIZE, fp);
	if (status == 0) { /* EOF or error */
	    if (ferror(fp) == 0) break;
	    HTLog_errorN("HTLoadCacheToStream: Read error, read returns",
			 ferror(fp));
	    break;
	}

	if (first) {
	    char * e = NULL;
	    first = NO;
	    input_buffer[status] = 0;
	    e = strstr(input_buffer,
		       "Expires: Will-be-filled-out-by-cache-proxy\r\n");
	    if (e) {
		char * timestr;
		if (expires && (timestr = http_time(&expires))) {
		    int len = strlen(timestr);
		    e += 9;
		    strncpy(e,timestr,len);
		    while (len < 33) {
			e[len] = ' ';
			len++;
		    }
		    e[33] = '\r';
		    e[34] = '\n';
		}
		else {
		    /*
		     * Be careful to write EXACTLY the correct length!
		     */
		    strcpy(e,"X-Proxy-Notice: Expiry checking disabled. ");
		    e[42] = '\r';
		    e[43] = '\n';
		}
	    }
	    else {
		CTRACE(stderr, "Expires..... field not in cached file\n");
	    }
	}

	if (headers) {
	    int i;
	    for(i=0; i<status; i++) {
		out.header_length++;
		if (flag) {
		    if (input_buffer[i] == LF) {
			headers = NO;
			out.content_length = status - i - 1;
			if (!HTClientProtocol)
			    (*targetClass.put_block)(sink,
						     input_buffer + i + 1,
						     status - i - 1);
			break;
		    }
		    else if (input_buffer[i] != CR)
			flag = NO;
		}
		else if (input_buffer[i] == LF) {
		    flag = YES;
		}
	    }
	    if (HTClientProtocol)
		(*targetClass.put_block)(sink, input_buffer, status);
	}
	else {
	    out.content_length += status;
	    (*targetClass.put_block)(sink, input_buffer, status);
	}

    } /* next bufferload */

    CTRACE(stderr,"Returned.... %d + %d = %d bytes from cache\n",
	   out.header_length, out.content_length,
	   out.header_length + out.content_length);
}

