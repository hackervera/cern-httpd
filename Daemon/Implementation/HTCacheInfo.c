/*
 * Handling of .cache_info files
 *
 * Cache info file format:
 *
 *	filename load-delay last-checked expires max-unused last-modified
 *
 * filename	the name of a file in the same directory
 * load-delay	time (in secs) it took to transfer this file from remote server
 * last-checked	time of last up-to-date check from remote server
 * expires	expiry time
 * max-unused	maximum time for this document to be unused before removal
 * last-modified as given from the remote server, or if not given the time
 *		of last update of the cache file is used
 *
 * All times are in seconds from epoch, as expressed in the time_t of the
 * host machine.
 *
 * Author:
 *	Ari Luotonen, May 1994, luotonen@www.cern.ch
 *
 */

#include <time.h>

#include "HTUtils.h"
#include "tcp.h"
#include "HTSUtils.h"
#include "HTCache.h"

/*
 * Get the cache info file name given the directory name.
 *
 *
 */
PRIVATE char * cif_name ARGS1(char *, dirname)
{
    if (dirname) {
	int dirlen = strlen(dirname);
	char * name = (char*)malloc(dirlen + strlen(CACHE_INFO) + 2);
	if (!name) outofmem(__FILE__,"cif_name");

	strcpy(name,dirname);
	if (dirname[dirlen-1] != '/')
	    strcat(name,"/");
	strcat(name,CACHE_INFO);
	return name;
    }
    return NULL;
}


/*
 * HTCacheInfo_open(dirname,mode), HTCacheInfo_openFor(filename,mode),
 * mode = "r" | "w" | "a".
 *
 * Open cache info file given the directory name, or a cache file name.
 *
 */
PUBLIC FILE * HTCacheInfo_open ARGS2(char *, dirname,
				     char *, mode)
{
    FILE * fp = NULL;
    char * name = cif_name(dirname);

    if (name) {
#ifdef VMS
	if (!strcmp(mode,"a"))
	    fp = fopen(name, "a+", "ctx=rec", "shr=get");
	else
	    fp = fopen(name,mode);
#else
	fp = fopen(name,mode);
#endif

	if (TRACE && !fp)
	    fprintf(stderr,"Error....... can't open cache info file: %s\n",
		    name);
	free(name);
    }
    return fp;
}

PUBLIC FILE * HTCacheInfo_openFor ARGS2(char *, pathname,
					char *, mode)
{
    FILE * fp = NULL;

    if (pathname) {
	char * slash = strrchr(pathname,'/');
	if (slash) {
	    *slash = 0;
	    fp = HTCacheInfo_open(pathname,mode);
	    *slash = '/';	/* Reconstruct */
	}
    }
    return fp;
}


/*
 * Remove cache info file from a given directory.
 *
 *
 */
PUBLIC void HTCacheInfo_remove ARGS1(char *, dirname)
{
    char * cifn = cif_name(dirname);
    if (cifn) {
	CTRACE(stderr, "Removing.... cache info file %s\n", cifn);
	unlink(cifn);
	free(cifn);
    }
}


/*
 * Read the next cache info entry from cache info file.
 *
 *
 */
PUBLIC BOOL HTCacheInfo_next ARGS7(FILE *,	fp,
				   char **,	filename_p,
				   time_t *,	load_delay,
				   time_t *,	last_checked,
				   time_t *,	expires,
				   time_t *,	max_unused,
				   time_t *,	last_modified)
{
    char buf[INFO_LINE_LEN];
    static char filename[INFO_LINE_LEN];

    if (!fp) return NO;

    *filename_p = NULL;
    *load_delay = (time_t)0;
    *last_checked = (time_t)0;
    *expires = (time_t)0;
    *max_unused = (time_t)0;
    *last_modified = (time_t)0;

    while (fgets(buf, INFO_LINE_LEN, fp)) {
	if (sscanf(buf, "%s %ld %ld %ld %ld %ld",
		   filename, (long*)load_delay, (long*)last_checked,
		   (long*)expires, (long*)max_unused,
		   (long*)last_modified) == 6) {
	    *filename_p = filename;
	    return YES;
	}
    }
    return NO;
}

/*
 * Read the whole cache info file for the given filename and
 * get the current attributes.
 *
 *
 */
PUBLIC BOOL HTCacheInfo_for ARGS6(char *,	pathname,
				  time_t *,	load_delay,
				  time_t *,	last_checked,
				  time_t *,	expires,
				  time_t *,	max_unused,
				  time_t *,	last_modified)
{
    BOOL found = NO;
    FILE * fp = HTCacheInfo_openFor(pathname,"r");
    char * filename = strrchr(pathname,'/');
    if (filename) filename++;
    else filename = pathname;
    if (fp) {
	char * fn;
	time_t ld,lc,ex,mu,lm;
	while (HTCacheInfo_next(fp,&fn,&ld,&lc,&ex,&mu,&lm)) {
	    if (fn && !strcmp(fn,filename)) {
		*load_delay = ld;
		*last_checked = lc;
		*expires = ex;
		*max_unused = mu;
		*last_modified = lm;
		found = YES;
	    }
	}
	fclose(fp);
    }
    return found;
}


/*
 * Write an entry to an already open cache info file.
 *
 *
 */
PUBLIC void HTCacheInfo_writeEntry ARGS7(FILE *,	fp,
					 char *,	filename,
					 time_t,	load_delay,
					 time_t,	last_checked,
					 time_t,	expires,
					 time_t,	max_unused,
					 time_t,	last_modified)
{
    if (fp && filename) {
	fprintf(fp, "%s\t%ld\t%ld\t%ld\t%ld\t%ld\n",
		filename, (long)load_delay, (long)last_checked,
		(long)expires, (long)max_unused, (long)last_modified);
	if (TRACE) {
	    fprintf(stderr,"CacheInfo... for %s:\n", filename);
	    fprintf(stderr,"- LoadDelay. %s\n", make_time_spec(&load_delay));
	    fprintf(stderr,"- LastMod... %s", ctime(&last_modified));
	    fprintf(stderr,"- LastCheck. %s", ctime(&last_checked));
	    fprintf(stderr,"- Expires... %s", ctime(&expires));
	    fprintf(stderr,"- MaxUnuse.. %s\n", make_time_spec(&max_unused));
	}
    }
}


/*
 * Open the cache info file for a given filename, and append an entry
 * to it.
 *
 *
 */
PUBLIC BOOL HTCacheInfo_writeEntryFor ARGS6(char *,	pathname,
					    time_t,	delay,
					    time_t,	last_chk,
					    time_t,	exp,
					    time_t,	max_unused,
					    time_t,	lm)
{
    FILE * fp = HTCacheInfo_openFor(pathname,"a");
    char * filename = strrchr(pathname,'/');

    if (filename) filename++;
    else filename = pathname;

    if (fp) {
	HTCacheInfo_writeEntry(fp,filename,delay,last_chk,exp,max_unused,lm);
	fflush(fp);
	fclose(fp);
	return YES;
    }
    return NO;
}

