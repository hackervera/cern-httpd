
/* MODULE							HTCache.c
**		CACHE GARBAGE COLLECTOR FOR GATEWAY HTTPD
**
** AUTHORS:
**	AL	Ari Luotonen	luotonen@dxcern.cern.ch
**
** HISTORY:
**	21 Feb 94  AL	Written from scratch.
**
** BUGS:
**
**
*/

#include <string.h>
#include <stdio.h>
#ifdef VMS
#include <types.h>
#include <time.h>
#else /* not VMS */
#include <sys/types.h>
#include <sys/time.h>
#endif /* not VMS */

#include "HTUtils.h"
#include "tcp.h"

#include "HTCache.h"
#include "HTConfig.h"
#include "HTDaemon.h"
#include "HTLog.h"

#ifndef VMS

#ifndef S_ISLNK
#define S_ISLNK(m) (NO)
#endif

#ifndef S_ISSOCK
#define S_ISSOCK(m) (NO)
#endif

#ifndef S_ISFIFO
#define S_ISFIFO(m) (NO)
#endif

#define HASH_SIZE	31


typedef struct _HTDirNode HTDirNode;
typedef struct _HTCacheNode HTCacheNode;

struct _HTCacheNode {
    char *		filename;	/* NOT absolute 		*/
    time_t		load_delay;	/* \				*/
    time_t		last_checked;	/*  \				*/
    time_t		expires;	/*   \				*/
    time_t		max_unused;	/*    \ For cache files		*/
    time_t		last_modified;	/*    /				*/
    time_t		last_access;	/*   /				*/
    long		bytes;		/*  /				*/
    float		value;		/* /				*/
    HTDirNode *		directory;	/* For directories		*/
    HTCacheNode *	next;		/*				*/
};

struct _HTDirNode {
    char *		dirname;	/* Absolute path		*/
    int			file_cnt;	/* Number of files in directory	*/
    int			total_node_cnt;	/* Total num of nodes under dir	*/
    HTCacheNode *	files[HASH_SIZE];
};


PRIVATE long bytes_total = 0;
PRIVATE long bytes_collected = 0;
PRIVATE int files_total = 0;
PRIVATE int files_collected = 0;

#define TABLE_SIZE 200
PRIVATE long table[ TABLE_SIZE ];
PRIVATE long total_table[ TABLE_SIZE ];


PRIVATE int make_ind ARGS1(float, value)
{
    int n = (int) 1000 * value;

    if (n < 0)
	n = 0;
    else if (n >= TABLE_SIZE/2) {
	n = ((n - TABLE_SIZE/2) / 10) + TABLE_SIZE/2;
	if (n >= TABLE_SIZE)
	    n = TABLE_SIZE - 1;
    }
    return n;
}


PRIVATE float find_threshold ARGS2(long,	must_collect,
				   long *,	tbl)
{
    float threshold = 0;
    int n;
    long incremental;

    if (!tbl) return 0;

    incremental = tbl[0];	/* Always collect expired */
    for(n=1; incremental<must_collect && n<TABLE_SIZE; n++)
	incremental += tbl[n];

    if (n < TABLE_SIZE/2)
	threshold = ((float)n)/1000;
    else
	threshold = ((float)(n - (9 * TABLE_SIZE)/20)) / 100;

    return threshold;
}


PRIVATE float get_threshold ARGS1(float, fraction)
{
    int i;
    long total_size = 0;
    long must_collect = 0;
    float threshold = 0;

    for(i=0; i<TABLE_SIZE; i++) {
	total_size     += table[i];
	total_table[i] += table[i];
    }
    must_collect = fraction * total_size;
    threshold = find_threshold(must_collect, table);

    for(i=0; i<TABLE_SIZE; i++)
	table[i] = 0;

    return threshold;
}


PRIVATE float get_optimal_threshold ARGS1(long, must_collect)
{
    return find_threshold(must_collect, total_table);
}


PUBLIC void HTCacheNode_free ARGS1(HTCacheNode *, killme)
{
    if (killme) {
	if (killme->filename)
	    free(killme->filename);
	free(killme);
    }
}

PUBLIC void HTDirNode_free ARGS1(HTDirNode *, killme)
{
    if (killme) {
	if (killme->dirname)
	    free(killme->dirname);
	free(killme);
    }
}


PRIVATE int hash ARGS1(char *, url)
{
    int h = 0;
    char * cur = url;

    if (!url) return 0;
    for( ; *cur; cur++)
	h += *cur;
    return h % HASH_SIZE;
}


/*
**	Find or create
*/
PRIVATE HTCacheNode * lookup ARGS3(HTDirNode *,	dir_info,
				   char *,	filename,
				   BOOL,	create)
{
    int h = hash(filename);
    HTCacheNode * node;

    if (!dir_info || !filename) return NULL;

    node = dir_info->files[h];
    while (node && node->filename && strcmp(node->filename, filename))
	node = node->next;

    if (!node && create) {
	node = (HTCacheNode*)calloc(1, sizeof(HTCacheNode));
	if (!node) outofmem(__FILE__, "lookup");
	StrAllocCopy(node->filename, filename);
	node->next = dir_info->files[h];
	dir_info->files[h] = node;
	files_total++;
    }
    return node;
}

PRIVATE void add_dir ARGS3(HTDirNode *,	dir_info,
			   char *,	dirname,
			   HTDirNode *,	contents)
{
    HTCacheNode * node = lookup(dir_info, dirname, YES);

    node->directory = contents;
    dir_info->total_node_cnt += contents->total_node_cnt;
}


/*
**	Called by directory scan
*/
PRIVATE void add_file ARGS5(HTDirNode *,	dir_info,
			    char *,		filename,
			    long,		bytes,
			    time_t,		created,
			    time_t,		last_accessed)
{
    HTCacheNode * node = lookup(dir_info, filename, YES);

    node->bytes = bytes;
    node->last_checked = created;
    node->last_access = last_accessed;
    bytes_total += bytes;
    dir_info->total_node_cnt++;
}


/*
**	Called by cache info reader
*/
PRIVATE void set_attributes ARGS7(HTDirNode *,	dir_info,
				  char *,	filename,
				  time_t,	load_delay,
				  time_t,	last_checked,
				  time_t,	expires,
				  time_t,	max_unused,
				  time_t,	last_modified)
{
    HTCacheNode * node = lookup(dir_info, filename, NO);

    if (node) {
	if (load_delay)
	    node->load_delay = load_delay;
	if (last_checked)
	    node->last_checked = last_checked;
	if (expires)
	    node->expires = expires;
	if (max_unused)
	    node->max_unused = max_unused;
	if (last_modified)
	    node->last_modified = last_modified;
    }
    else CTRACE(stderr, "File \"%s\" no longer exists\n", filename);
}


PRIVATE float byte_factor ARGS1(long, bytes)
{
    int kilos = bytes/1024;

    if (kilos <= cc.cache_limit_1) return 1;
    else if (kilos >= cc.cache_limit_2) return 0;
    else return 1 -
	((float)kilos) / ((float)(cc.cache_limit_2 - cc.cache_limit_1));
}

PRIVATE float load_delay_factor ARGS1(time_t, t_del)
{
    if (t_del >= 20) return 1;
    else return ((float)t_del) / (float)20;
}


PRIVATE float calculate_value ARGS7(time_t,	t_cre,
				    time_t,	t_acc,
				    time_t,	t_exp,
				    time_t,	t_del,
				    time_t,	max_unused,
				    long,	bytes,
				    char *,	filename)
{
    time_t t_age = cur_time - t_cre;
    time_t t_unu = cur_time - t_acc;
    float f_age = .05;
    float f_acc = .05;
    float f_exp = .001;
    float f_del = .05;
    float f_byt = .05;

    if (cc.cache_clean_def)
	f_age = 1 - ((float)t_age) / ((float)cc.cache_clean_def);
    if (max_unused)
	f_acc = 1 - ((float)t_unu) / ((float)max_unused);
    if (cc.keep_expired) {
	/*
	 * 2 million seconds is about 23 days; it's just some time that we
	 * extend the expiry to still get some kind of value factor.
	 * The actual removal is now mainly controlled by CacheUnused and
	 * CacheClean.
	 */
	if (t_exp  &&  t_exp + 2000000 != t_cre)
	    f_exp = ((float)(t_exp + 2000000 - cur_time)) /
			((float)(t_exp + 2000000 - t_cre));
    }
    else {
	if (t_exp && t_exp != t_cre)
	    f_exp = ((float)(t_exp - cur_time)) / ((float)(t_exp - t_cre));
    }
    if (t_del)
	f_del = load_delay_factor(t_del);
    f_byt = byte_factor(bytes);

    if (f_age < 0) f_age=0;
    if (f_acc < 0) f_acc=0;
    if (f_exp < 0) f_exp=0;
    if (f_del < 0) f_del=0;
    if (f_byt < 0) f_byt=0;

    if (TRACE) {
	char s_cre[30], s_acc[30], s_exp[30];

	strcpy(s_cre,ctime(&t_cre));
	strcpy(s_acc,ctime(&t_acc));
	strcpy(s_exp,ctime(&t_exp));

	s_cre[24]=0; s_acc[24]=0; s_exp[24]=0;

	fprintf(stderr,
		"............  %s  %s  %s  %3ld %8ld  %6.3f %6.3f %6.3f %6.3f %6.3f   %8.5f  %s\n",
		s_cre, s_acc, s_exp, (long)t_del, bytes,
		f_age, f_acc, f_exp, f_del, f_byt,
		f_age * f_acc * f_exp * f_del * f_byt,
		filename);
    }

    return f_age * f_acc * f_exp * f_del * f_byt;
}



PRIVATE void eval ARGS1(HTCacheNode *, node)
{
    if (node->expires && node->expires <= cur_time && !cc.keep_expired) {
	CTRACE(stderr, "gc Expired.. \"%s\"\n", node->filename);
	node->value = 0;
    }
    else if (node->last_checked &&
	     cur_time - node->last_checked >= cc.cache_clean_def) {
	CTRACE(stderr, "gc Too old.. \"%s\"\n", node->filename);
	node->value = 0;
    }
    else if (node->last_access &&
	     node->max_unused &&
	     cur_time - node->last_access >= node->max_unused) {
	CTRACE(stderr, "gc Unused... \"%s\"\n", node->filename);
	node->value = 0;
    }
    else {
	node->value = calculate_value(node->last_checked,
				      node->last_access,
				      node->expires,
				      node->load_delay,
				      node->max_unused,
				      node->bytes,
				      node->filename);
    }

    table[ make_ind(node->value) ] += node->bytes;
}


PRIVATE void evaluate ARGS1(HTDirNode *, dir_info)
{
    int i;
    HTCacheNode * cur;

    if (!dir_info) return;

    for (i=0; i<HASH_SIZE; i++) {
	cur = dir_info->files[i];
	while (cur) {
	    if (cur->directory)
		evaluate(cur->directory);
	    else
		eval(cur);
	    cur = cur->next;
	}
    }
}


/*
**	Does actual gc and frees everything hanging from dir_info,
**	including dir_info itself.
*/
PRIVATE void collect ARGS2(HTDirNode *,	dir_info,
			   float,	threshold)
{
    int i;
    HTCacheNode * cur;
    HTCacheNode * killme;
    FILE * cif = NULL;

    if (!dir_info || !dir_info->dirname) return;

    cif = HTCacheInfo_open(dir_info->dirname,"w");
    if (!cif)
	HTLog_error2("Can't rewrite cache info file in directory:",
		     dir_info->dirname);

    for (i=0; i<HASH_SIZE; i++) {
	cur = dir_info->files[i];
	while (cur) {
	    killme = cur;
	    cur = cur->next;
	    if (killme->directory)
		collect(killme->directory, threshold);
	    else {
		if (killme->value < threshold) {
		    char * fn = (char*)malloc(strlen(dir_info->dirname) +
					      strlen(killme->filename) + 2);
		    if (!fn) outofmem(__FILE__, "destroy");
		    sprintf(fn, "%s/%s", dir_info->dirname, killme->filename);
		    CTRACE(stderr, "gc Removing. \"%s\"\n", fn);
		    unlink(fn);
		    free(fn);

		    dir_info->file_cnt--;
		    files_collected++;
		    bytes_collected += killme->bytes;
		}
		else {
		    if (cif)
			HTCacheInfo_writeEntry(cif,
					       killme->filename,
					       killme->load_delay,
					       killme->last_checked,
					       killme->expires,
					       killme->max_unused,
					       killme->last_modified);
		}
	    }
	    HTCacheNode_free(killme);
	}
    }

    if (cif) {
	fflush(cif);
	fclose(cif);
	if (dir_info->file_cnt <= 0) {
	    HTCacheInfo_remove(dir_info->dirname);
	    CTRACE(stderr, "Removing.... empty directory %s\n",
		   dir_info->dirname);
	    rmdir(dir_info->dirname);
	}
    }

    HTDirNode_free(dir_info);
}


PRIVATE HTDirNode * gc_read_dir ARGS3(char *,	dirname,
				      BOOL,	do_collect,
				      float,	fraction)
{
    DIR * dp;
    STRUCT_DIRENT * dirbuf;
    HTDirNode * dir_info;
    char * path;
    FILE * fp;
    struct stat stat_info;
    BOOL cif_exists = NO;
    BOOL lockfile = NO;

    if (!dirname) return NULL;

    /* Read physical directory */
    dp = opendir(dirname);
    if (!dp) {
	CTRACE(stderr, "gc.......... Can't open directory \"%s\"\n", dirname);
	return NULL;
    }

    dir_info = (HTDirNode*)calloc(1, sizeof(HTDirNode));
    if (!dir_info) outofmem(__FILE__, "gc_read_dir");
    dir_info->file_cnt = 0;

    while ((dirbuf = readdir(dp))) {

	if (!dirbuf->d_ino) continue;	/* Not in use */

	if (!strcmp(dirbuf->d_name, ".") || !strcmp(dirbuf->d_name, ".."))
	    continue;	/* Ignore . and .. */

	if (!strcmp(dirbuf->d_name, CACHE_INFO)) {
	    cif_exists = YES;
	    continue;
	}

	dir_info->file_cnt++;

	if (!strcmp(dirbuf->d_name, ".gc_info"))
	    continue;	/* Ignore reserved files */

	if (strstr(dirbuf->d_name, LOCK_SUFFIX) &&
	    !strcmp(dirbuf->d_name +
		    strlen(dirbuf->d_name) - strlen(LOCK_SUFFIX),
		    LOCK_SUFFIX))
	    lockfile = YES;
	else
	    lockfile = NO;

	path = (char*)malloc(strlen(dirname) + strlen(dirbuf->d_name) + 2);
	if (!path) outofmem(__FILE__, "gc_read_dir");
	sprintf(path, "%s/%s", dirname, dirbuf->d_name);

	if (HTStat(path, &stat_info) == -1) {
	    CTRACE(stderr, "gc ERROR.... Can't stat \"%s\"\n", path);
	    free(path);
	    continue;
	}

	if (S_ISDIR(stat_info.st_mode)) {
	    HTDirNode * di = gc_read_dir(path, NO, fraction);
	    if (di) add_dir(dir_info, path, di);
	}
	else if (S_ISREG(stat_info.st_mode)) {
	    if (lockfile) {
		if (cur_time - stat_info.st_mtime > cc.cache_lock_timeout) {
		    CTRACE(stderr,
			   "Lockfile.... timed out -- removing \"%s\"\n",path);
		    unlink(path);
		    dir_info->file_cnt--;
		}
		else CTRACE(stderr,
			    "Lockfile.... left untouched: \"%s\"\n",path);
	    }
	    else {
		add_file(dir_info, dirbuf->d_name,
			 stat_info.st_size,
			 stat_info.st_mtime,
			 stat_info.st_atime);
	    }
	}
	else {
	    CTRACE(stderr, "Not processing \"%s\" because it is %s\n", path,
		   (S_ISCHR(stat_info.st_mode)  ? "character special file" :
		   (S_ISBLK(stat_info.st_mode)  ? "block special file" :
		   (S_ISLNK(stat_info.st_mode)  ? "a symbolic link" :
		   (S_ISSOCK(stat_info.st_mode) ? "a socket" :
		   (S_ISFIFO(stat_info.st_mode) ? "pipe or FIFO special file" :
		    "not a file or a directory"))))));
	}
	free(path);
    }
    closedir(dp);

    StrAllocCopy(dir_info->dirname, dirname);

    /* Read cache info file -- ignores files that no longer exist */
    if (cif_exists) {
	fp = HTCacheInfo_open(dirname,"r");
	if (fp) {
	    char * filename;
	    time_t load_delay;
	    time_t last_checked;
	    time_t expires;
	    time_t max_unused;
	    time_t last_modified;

	    while (HTCacheInfo_next(fp, &filename, &load_delay, &last_checked,
				    &expires, &max_unused, &last_modified)) {
		set_attributes(dir_info, filename,
			       load_delay, last_checked,
			       expires, max_unused, last_modified);
	    }
	    fclose(fp);
	}
	else {
	    CTRACE(stderr,"gc.......... No cache info file for directory %s\n",
		   dirname);
	}
    }

    if (do_collect  ||  dir_info->total_node_cnt >= cc.gc_mem_usage_control) {
	CTRACE(stderr, "Collecting.. (%d nodes in memory)\n",
	       dir_info->total_node_cnt);
	evaluate(dir_info);
	collect(dir_info, get_threshold(fraction));
	return NULL;
    }
    else return dir_info;
}


PRIVATE char * cache_sub_dir ARGS1(char *, subdirname)
{
    static char * cache_dir = NULL;

    FREE(cache_dir);	/* From previous call */
    cache_dir = (char*)malloc(strlen(cc.cache_root)+strlen(subdirname)+2);
    if (!cache_dir) outofmem(__FILE__, "cache_sub_dir");
    sprintf(cache_dir, "%s/%s", cc.cache_root, subdirname);
    return cache_dir;
}


PUBLIC void gc NOARGS
{
    FILE * fp;
    float fraction = 0;

    if (!cc.cache_root) return;

    if (cc.cache_max_k <= 0  ||  HTCacheUsage <= 0)
	fraction = 0;
    else {
	fraction = (HTCacheUsage - 0.9 * cc.cache_max_k) / HTCacheUsage;
	if (fraction < 0) fraction = 0;
    }

    CTRACE(stderr, "gc.......... Collecting %5.2f%% (%ldK) out of %ldK\n",
	   100*fraction, (long)(HTCacheUsage*fraction), HTCacheUsage);

    CTRACE(stderr,
	   "Ranking.....  ======= CREATED ========  ===== LAST ACCESSED ====  ======== EXPIRES =======  DELAY   SIZE   AGE-F  ACC-F  EXP-F  DEL-F  BYT-F    VALUE    NAME\n");

    time(&cur_time);

    {
	/*
	 * Keep_expired makes sense only for HTTP since only it has the
	 * if-modified-since GET.
	 *
	 * Actually, we could implement it for ftp, taking first the
	 * directory listing to find out last-modified dates, but that's
	 * something for the future.
	 *
	 * For now I'll just reset cc.keep_expired to false when I'm doing
	 * gc for other protocols than HTTP.  Ugly, I know, but it saves
	 * me from a lot of headache, plus it makes it more efficient.
	 * Otherwise I would have to explicitly check the protocol every
	 * time, and keep information about it separately available.
	 * Doesn't sound like too hot an idea, either.  I think in this
	 * it's well justified.
	 */

	BOOL keep_expired = cc.keep_expired;

	CTRACE(stderr, "http........\n");
	gc_read_dir(cache_sub_dir("http"), YES, fraction);

	cc.keep_expired = NO;

	CTRACE(stderr, "ftp.........\n");
	gc_read_dir(cache_sub_dir("ftp"), YES, fraction);

	CTRACE(stderr, "gopher......\n");
	gc_read_dir(cache_sub_dir("gopher"), YES, fraction);

	cc.keep_expired = keep_expired;
    }

    if (TRACE) {
	fprintf(stderr, "GC report... Original:  %8ldK %8d files\n",
		bytes_total/1024, files_total);
	fprintf(stderr, "............ Collected: %8ldK %8d files\n",
		bytes_collected/1024, files_collected);
	fprintf(stderr, "............ Current:   %8ldK %8d files\n",
		(bytes_total-bytes_collected)/1024,
		files_total - files_collected);
    }

    if (!gc_info_file) {
	gc_info_file = (char*)malloc(strlen(cc.cache_root) + 20);
	sprintf(gc_info_file, "%s/.gc_info", cc.cache_root);
	CTRACE(stderr, "BugFix...... gc report file wasn't set");
    }

    fp = fopen(gc_info_file, "w");
    if (!fp) {
	HTLog_error2("GC: Can't open cache report file for writing:",
		     gc_info_file);
	return;
    }
    fprintf(fp, "%d\t%ld\t%ld\t%d\t%d\t%f\n",
	    (int)getpid(),
	    bytes_total - bytes_collected,  bytes_collected,
	    files_total - files_collected,  files_collected,
	    get_optimal_threshold((long)(fraction * HTCacheUsage)));
    fflush(fp);
    fclose(fp);
    chmod(gc_info_file,
	  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    CTRACE(stderr, "GC COMPLETE.\n");
}

#endif /* not VMS */
