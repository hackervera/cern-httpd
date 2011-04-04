/*	Configuration manager for Hypertext Daemon		HTConfig.c
**	==========================================
**
** Authors:
**	AL		Ari Luotonen	 <luotonen@www.cern.ch>
**	MD		Mark Donszelmann <duns@vxdeop.cern.ch>
**	TBL		Tim Berners-Lee	 <timbl@info.cern.ch>
**	FM		Foteos Macrides
**
** History:
**	 3 Jun 91	Written TBL
**	10 Aug 91	Authorisation added after Daniel Martin (pass, fail)
**			Rule order in file changed
**			Comments allowed with # on 1st char of rule line
**      17 Jun 92       Bug fix: pass and fail failed if didn't contain '*' TBL
**       1 Sep 93       Bug fix: no memory check - Nathan Torkington
**                      BYTE_ADDRESSING removed - Arthur Secret
**	11 Sep 93  MD	Changed %i into %d in debug printf. 
**			VMS does not recognize %i.
**			Bug Fix: in case of PASS, only one parameter to printf.
**	19 Sep 93  AL	Added Access Authorization stuff.
**	 1 Nov 93  AL	Added htbin.
**	30 Nov 93  AL	Added HTTranslateReq().
**	 4 Feb 94  AL	Moved to daemon distribution and renamed
**			(HTRules->HTConfig).  Library still has its
**			own HTRules.c, which this file overrides.
**	04 Mar 94  MD	Added case insensitivity option checking for rules.
**	   Apr 94  AL	Reorganized the whole config system into structures.
**	31 Mar 94  FM	Added ability to define spawninit and scratchdir for
**			script handling via the rule file on VMS.
**	06 Jul 94  FM	Require that username own the file for access to
**			UserDir on VMS (otherwise, a user could alias the
**			the UserDir and access any file with SYSPRV).
**
*/

/* (c) CERN WorldWideWeb project 1990,91. See Copyright.html for details */
#include <time.h>

#include "HTRules.h"

#include "tcp.h"
#include "HTFile.h"
#include "HTDirBrw.h"
#include "HTDescript.h"
#include "HTIcons.h"
#include "HTParse.h"	/* HTParse() */
#include "HTAAUtil.h"
#include "HTAAProt.h"
#include "HTMulti.h"	/* HTAddWelcome() */
#include "HTDaemon.h"
#include "HTConfig.h"
#include "HTLog.h"
#include "HTError.h"
#include "HTTCP.h"	/* HTHostName() */


#ifndef VMS
#include <pwd.h>	/* Unix password file routine: getpwnam() */
#else /* VMS */
#include "HTVMSUtils.h"
#endif /* VMS */


#define LINE_LENGTH	1024	/* Maximum configuration line length */
#define MAX_ARGS	50	/* Max num of space-separated values on line */
#define TIME_TRACE(m,t) \
    if (TRACE) fprintf(stderr,"%s %s\n", m, make_time_spec(&t))
#define REQ(l,h)	\
    if (!required(vec[0],c,l,h)) continue


typedef struct _HTAAProtNode {
    char *	prot_name;
    HTAAProt *	prot;
} HTAAProtNode;

#ifdef VMS
PRIVATE BOOL	HTUserOwnsFile;		/* Does username own the file?	*/
#endif /* VMS */


/*
 *	Global variables
 */
PUBLIC HTServerConfig	sc;		/* Server configuration		*/
PUBLIC HTResourceConfig	rc;		/* Resource configuration	*/
PUBLIC HTCacheConfig	cc;		/* Cache & proxy configuration	*/

extern BOOL HTSuffixCaseSense;		/* From HTFile.c		*/

#ifdef VMS
PUBLIC BOOL	HTRuleCaseSense = NO;	/* rules are checked case insensitive*/
PUBLIC char*	HTSpawnInit	= NULL;	/* script process initialization file */
PUBLIC char*	HTScratchDir	= NULL;	/* directory for script scratch files */
#else
PUBLIC BOOL	HTRuleCaseSense = YES;	/* rules are checked case sensitive */
#endif



PRIVATE char * rule_names[] =
{
    "Invalid.....", "Map.........", "Pass........", "Fail........",
    "DefProt.....", "Protect.....", "Exec........", "Redirect....",
    "Move........"
};


/*
 *	Set configuration flags to default values
 */
PUBLIC void HTDefaultConfig NOARGS
{
    sc.input_timeout = DEFAULT_INPUT_TIMEOUT;
    sc.output_timeout = DEFAULT_OUTPUT_TIMEOUT;
    sc.script_timeout = DEFAULT_SCRIPT_TIMEOUT;
    sc.do_accept_hack = YES;
    sc.disabled[ (int)METHOD_PUT ] = YES;
    sc.disabled[ (int)METHOD_DELETE ] = YES;
    sc.disabled[ (int)METHOD_CHECKIN ] = YES;
    sc.disabled[ (int)METHOD_CHECKOUT ] = YES;
    sc.disabled[ (int)METHOD_SHOWMETHOD ] = YES;
    sc.disabled[ (int)METHOD_LINK ] = YES;
    sc.disabled[ (int)METHOD_UNLINK ] = YES;
    sc.new_logfile_format = YES;
    sc.do_dns_lookup = YES;
    sc.always_welcome = YES;
    sc.max_content_len_buf = 51200;	/* 50K buffer for finding C-L	*/

    cc.cache_clean_def =  21*24*3600;	/* Remove everything after 3 wks*/
    cc.cache_unused_def = 14*24*3600;	/* Remove if unused 2 weeks	*/
    cc.cache_max_k = 5 * 1024;		/* Default cache size 5MB	*/
    cc.cache_limit_1 = 100;		/* Size no problem until 100K	*/
    cc.cache_limit_2 = 4000;		/* No caching after 4MB		*/
    cc.cache_time_margin = 120;		/* Time accuracy (in seconds)	*/
    cc.do_exp_check = YES;		/* Do expiry check		*/
    cc.gc_daily_gc = 3*3600;		/* gc daily at 3:00am		*/
    cc.gc_mem_usage_control = 500;	/* How radical mem usage	*/
    cc.cache_lock_timeout = DEFAULT_OUTPUT_TIMEOUT;
}


PRIVATE char * tilde_to_home ARGS1(CONST char *, path)
{
    char * result = NULL;

    if (!path) return NULL;

    if (*path == '~') {
	char * end = strchr(path+1,'/');
	struct passwd * pw = NULL;

	if (end) *end = 0;
	pw = getpwnam(path+1);
	if (pw && pw->pw_dir) {
	    int len = strlen(pw->pw_dir) + 2;
	    if (end) len += strlen(end+1);
	    result = malloc(len);
	    if (!result) outofmem(__FILE__, "tilde_to_home");
	    strcpy(result, pw->pw_dir);
	}
	if (end) {
	    *end = '/';
	    if (result)
		strcat(result, end);
	}
	if (TRACE) {
	    if (result)
		fprintf(stderr,"Tilde....... %s expanded to %s\n",path,result);
	    else
		HTLog_error2("Can't find home directory:",path);
	}
    }
    if (!result)
	StrAllocCopy(result, path);

    return result;
}


PRIVATE void HTAddNamedProtection ARGS2(char *,		name,
					HTAAProt *,	prot)
{
    HTAAProtNode * node = (HTAAProtNode*)calloc(1,sizeof(HTAAProtNode));
    if (!node) outofmem(__FILE__, "HTAddNamedProtection");

    StrAllocCopy(node->prot_name, name);
    node->prot = prot;

    if (!rc.named_prots) rc.named_prots = HTList_new();
    HTList_addObject(rc.named_prots, (void*)node);
}


PRIVATE HTAAProt * HTGetNamedProtection ARGS1(CONST char *,	name)
{
    HTList * cur = rc.named_prots;
    HTAAProtNode * node;

    if (!name) return NULL;

    CTRACE(stderr, "Looking up.. named protection \"%s\"\n", name);

    while ((node = (HTAAProtNode*)HTList_nextObject(cur))) {
	CTRACE(stderr, "Checking.... \"%s\"\n", node->prot_name);
	if (node->prot_name && !strcasecomp(node->prot_name, name)) {
	    CTRACE(stderr, "Found.......\n");
	    return node->prot;
	}
    }
    return NULL;
}


/*	Append a rule						HTAppendRule()
 *
 *  This is for: Map, Pass, Fail, Exec, Redirect, Moved
 *	(only internally: Protect, DefProt when called by HTAddProtRule()).
 *
 *  On entry,
 *	op	rule type
 *	pattern	string pattern that this rule matches
 *	equiv	string pattern that this rule maps to
 *  On exit,
 *	returns	the newly created rule, or NULL on failure.
 */
PUBLIC HTRule * HTAppendRule ARGS3(HTRuleOp,		op,
				   CONST char *,	pattern,
				   CONST char *,	equiv)
{
    HTRule * r = (HTRule *)calloc(1, sizeof(HTRule));
    if (!r) outofmem(__FILE__, "HTAppendRule"); 

    if (!pattern) {
	CTRACE(stderr,"BUG: HTAppendRule called with NULL pattern\n");
	return NULL;
    }

    r->op = op;
    r->pat = HTPattern_new(pattern);
    if (equiv) r->eqv = HTPattern_new(equiv);

    if (rc.rule_tail)
	rc.rule_tail->next = r;
    else
	rc.rule_head = r;
    rc.rule_tail = r;

    if (TRACE) {
	fprintf(stderr, "%s \"%s\"", rule_names[(int)op], pattern);
	if (equiv) fprintf(stderr, " --> \"%s\"\n", equiv);
	else fprintf(stderr, "\n");
    }

    return r;
}


PRIVATE void HTPrependPass ARGS2(CONST char *,	pattern,
				 CONST char *,	equiv)
{
    HTRule * r = (HTRule *)calloc(1, sizeof(HTRule));
    if (!r) outofmem(__FILE__, "HTPrependPass"); 

    if (!pattern) return;

    r->op = HT_Pass;
    r->pat = HTPattern_new(pattern);
    if (equiv) r->eqv = HTPattern_new(equiv);

    r->next = rc.rule_head;
    rc.rule_head = r;
    if (!rc.rule_tail)
	rc.rule_tail = r;

    if (TRACE) {
	fprintf(stderr,"AutoRule.... Pass \"%s\"",pattern);
	if (equiv) fprintf(stderr, " --> \"%s\"\n",equiv);
	else fprintf(stderr,"\n");
    }
}


/*								HTAddProtRule()
 *	Add a Protect or DefProt rule
 *
 *	op	is either HT_Protect or HT_DefProt
 *	templ	is a string template matching this rule
 *	prot	protection name or protection file name
 *	ids	string of form uid.gid for specifying user and group ids
 */
PRIVATE HTRule * HTAddProtRule ARGS4(HTRuleOp,		op,
				     CONST char *,	templ,
				     CONST char *,	prot,
				     char *,		ids)
{
    HTRule * r = HTAppendRule(op,templ,NULL);
    if (!r) return NULL;

    if (op != HT_Protect && op != HT_DefProt) {
	CTRACE(stderr,"BUG: HTAddProtRule called with op: %d\n",(int)op);
	return NULL;
    }

    if (ids) r->ids = HTUidGid_new(ids,NULL);
    if (prot) {
	if (strchr(prot,'/'))	/* Abs.path of prot.setup file given */
	    r->prot_file = tilde_to_home(prot);
	else {	/* Named protection setup used */
	    r->prot_setup = HTGetNamedProtection(prot);
	    if (!r->prot_setup) {
		HTLog_error2("Protection setup name undefined:",prot);
		return NULL;
	    }
	}
    }
    return r;
}


PRIVATE HTRule * HTAddInlinedProtRule ARGS3(HTRuleOp,		op,
					    CONST char *,	pattern,
					    HTAAProt *,		prot)
{
    HTRule * r = HTAddProtRule(op,pattern,NULL,NULL);
    if (!r) return NULL;

    r->prot_setup = prot;

    CTRACE(stderr, "%s \"%s\" (setup in config file, parsed above)\n",
	   rule_names[(int)op], pattern);
    return r;
}



/*
 *	Clear the list of rules in a resource configuration object.
 */
PRIVATE void clear_rules NOARGS
{
    HTRule * cur = rc.rule_head;
    HTRule * killme;

    while (cur) {
	if (cur->pat) HTPattern_free(cur->pat);
	if (cur->eqv) HTPattern_free(cur->eqv);
	if (cur->ids) HTUidGid_free(cur->ids);
	killme = cur;
	cur = cur->next;
	free(killme);
    }
    rc.rule_head = NULL;
    rc.rule_tail = NULL;
}

		 
PRIVATE char * get_user_dir ARGS1(char *, current)
{
    char * username = current+2;
    char * end = strchr(username, '/');
    if (end)  *end++ = 0;

#ifdef VMS
    HTUserOwnsFile = YES;
#endif /* VMS */
    if (*username) {
	struct passwd * pw = getpwnam(username);

	if (pw && pw->pw_dir) {
	    int homelen = strlen(pw->pw_dir);
	    char * d = (char*)malloc(homelen +
				     strlen(sc.user_dir) +
				     (end ? strlen(end) : 0) + 3);
	    strcpy(d, pw->pw_dir);
	    if (pw->pw_dir[homelen-1] != '/' && sc.user_dir[0] != '/')
		strcat(d, "/");
	    strcat(d, sc.user_dir);
	    if (end) {
		if (sc.user_dir[strlen(sc.user_dir)-1] != '/')
		    strcat(d, "/");
		strcat(d, end);
	    }
	    CTRACE(stderr,"UserDir..... \"%s\" mapped to \"%s\"\n",current,d);
#ifdef VMS /* Require that username own the file on VMS */
	    if (!HTVMS_isOwner(d, username)) {
		FREE(d);
		HTUserOwnsFile = NO;
		return NULL;
	    }
#endif /* VMS */
	    return d;
	}
	else {
	    CTRACE(stderr,"ERROR....... User dir for \"%s\" not found\n",
		   current);
	}
    }
    else {
	CTRACE(stderr,
	       "ERROR....... Invalid user dir request \"%s\"\n",
	       current);
    }

    return NULL;
}


PRIVATE char * unwrap ARGS1(char *, s)
{
    if (s && !strncmp(s, "file:", 5)) {
	char * t;
	char * n = NULL;

	if (!strncmp(s, "file://", 7))
	    t = strchr(s+7, '/');
	else t = s+5;

	if (!t) {
	    free(s);
	    return NULL;
	}
	StrAllocCopy(n, t);
	free(s);
	return n;
    }
    return s;
}


/*	Translate by rules				HTTranslateReq()
 *	------------------
 *
 * On entry,
 *	req		request structure.
 *
 *	dry (static)	if YES translates PATH_INFO to PATH_TRANSLATED
 *			instead of doing the real translation (set to
 *			YES only when HTTranslateReq() calls itself
 *			recursively).
 *	HTReqArgPath	simplified pathname (no ..'s etc in it),
 *			which will be translated.
 *
 * On exit,
 *	returns		YES on success, NO on failure (Forbidden).
 *	HTReqTranslated	contains the translated filename;
 *			NULL if a script call.
 *	HTReqScript	contains the executable script name;
 *			NULL if not a script call.
 */
PUBLIC BOOL HTTranslateReq ARGS1(HTRequest *, req)
{
    static BOOL dry = NO;
    HTRule * r;
    char * current = NULL;

    if (!req  ||  !HTReqArgPath) {
	HTReason = HTAA_BY_RULE;
	return NO;
    }

    if (dry)
	StrAllocCopy(current, HTScriptPathInfo);
    else {
	StrAllocCopy(current, HTReqArgPath);
    }

    for(r = rc.rule_head; r; r = r->next) {

	/*
	 *	Do matching and mapping
	 */
	{
	    char * mapped = HTPattern_match(r->pat, r->eqv, current);
	    if (!mapped) continue;	/* No match */

	    CTRACE(stderr, "%s rule matched \"%s\" -> \"%s\"\n",
		   rule_names[(int)(r->op)], current,mapped);
	    free(current);
	    current = mapped;
	    mapped = NULL;
	}


	if (dry && r->op != HT_Map && r->op != HT_Pass && r->op != HT_Fail)
	    continue;	/* PATH_INFO translation ignores these */

	switch (r->op) {		/* Perform operation */

	  case HT_DefProt:
	  case HT_Protect:

	    CTRACE(stderr, "Protection.. setup %s\n",
		   (r->prot_file ? r->prot_file :
		    (r->prot_setup ? "as defined in config file" :
		     (r->op==HT_Protect ? "-default-" : "-not-set-"))));

	    if (r->prot_setup) {
		if (r->op == HT_Protect) {
		    HTProt = r->prot_setup;
		    HTSetCurrentIds(HTProt->ids);
		}
		else
		    HTDefProt = r->prot_setup;
	    }
	    else {
		if (r->op == HT_Protect)
		    HTAA_setCurrentProtection(req, r->prot_file, r->ids);
		else
		    HTAA_setDefaultProtection(req, r->prot_file, r->ids);
	    }
	    break;

	  case HT_Exec:
	    {
		HTPattern * pat = r->eqv ? r->eqv : r->pat;
		char * wild = HTPattern_firstWild(pat,current);
		char * pathinfo = NULL;

		if (wild) pathinfo = strchr(wild,'/');

		HTReqScript = current;
		if (!pathinfo) {
		    CTRACE(stderr, "No%s PATH_INFO\n",
			   wild ? ".........." : " wilds.... so no");
		    HTScriptPathInfo = NULL;
		    HTScriptPathTrans = NULL;
		    return YES;
		}
		else {
		    StrAllocCopy(HTScriptPathInfo,pathinfo);
		    *pathinfo = 0;	/* Terminate script name */
		    dry = YES;
		    HTTranslateReq(req);
		    dry = NO;
		}

		CTRACE(stderr,
		     "Exec........ \"%s\"\nPathinfo.... %s\nTranslated.. %s\n",
		     HTReqScript,
		     (HTScriptPathInfo ? HTScriptPathInfo : "-none-"),
		     (HTScriptPathTrans? HTScriptPathTrans: "-none-"));

		return YES;
	    }
	    break;

	  case HT_Redirect:

	    if (!r->eqv) {
		CTRACE(stderr,
		       "ERROR....... No destination for redirect \"%s\"\n",
		       current);
		HTReason = HTAA_INVALID_REDIRECT;
		free(current);		/* Leak fixed AL 6 Feb 1994 */
		return NO;
	    }
	    else {
		char * query = strchr(HTReqArg,'?');
		HTReason = HTAA_OK_REDIRECT;
		HTLocation = current;
		if (query)
		    StrAllocCat(HTLocation, query);
		return YES;
	    }
	    break;

	  case HT_Moved:

	    if (!r->eqv) {
		CTRACE(stderr,
		       "ERROR....... No destination for moved \"%s\"\n",
		       current);
		HTReason = HTAA_INVALID_REDIRECT;
		free(current);		/* Leak fixed AL 6 Feb 1994 */
		return NO;
	    }
	    else {
		char * query = strchr(HTReqArg,'?');
		HTReason = HTAA_OK_MOVED;
		HTLocation = current;
		if (query)
		    StrAllocCat(HTLocation, query);
		return YES;
	    }
	    break;

	  case HT_Pass:
	    CTRACE(stderr, "Passing..... \"%s\"\n", current);
	    goto ok;

	  case HT_Map:
	    break;

	  case HT_Invalid:
	  case HT_Fail:
	    if (!strncmp(current,"/~",2) || !strncmp(current,"file:/~",7)) {
		CTRACE(stderr,
		       "No effect... User directories don't match Fail\n");
	    }
	    else {
		CTRACE(stderr, "Fail........ \"%s\"\n", current);
		free(current);	/* Leak fixed AL 6 Feb 1994 */
		HTReason = HTAA_BY_RULE;
		return NO;
	    }
	    break;

	  case HT_UseProxy:		/* NOT USED :-( */
	    break;

	} /* switch on op */

    } /* loop over rules */

    if (current) {
	if (!strncmp(current,"/~",2) || !strncmp(current,"file:/~",7)) {
	    /* User supported directory => pass */
	    CTRACE(stderr,
		   "AutoPass.... Passing user-supported directory \"%s\"\n",
		   current);
	    goto ok;
	}
	else {
	    CTRACE(stderr, "DefaultFail. Failing \"%s\" by default\n",
		   current);
	    CTRACE(stderr, "Explanation. no Pass rule\n");
	    free(current);
	}
    }
    HTReqTranslated = NULL;
    HTReason = HTAA_BY_RULE;
    return NO;	/* Default for server is to fail (explicit Pass required) */

  ok:
    current = unwrap(current);
    if (sc.user_dir && current[0] == '/' && current[1] == '~') {
	if (!strchr(current+2, '/')) {  /* Need to add slash and redir */
	    char * query = strchr(HTReqArg,'?');
	    HTLocation = (char*)malloc(strlen(HTReqArg) + 2);
	    if (!HTLocation) outofmem(__FILE__, "HTTranslateReq:redirect");
	    if (query) *query = 0;	/* Temporarily chop off keywords */
	    sprintf(HTLocation, "%s/", HTReqArg);
	    if (query) {
		*query = '?';	/* Reconstruct again */
		strcat(HTLocation,query);
	    }
	    free(current);
	    HTReason = HTAA_OK_REDIRECT;
	    return YES;
	}
	else {
	    char * mapped = get_user_dir(current);
	    free(current);
	    if (!mapped) {
#ifdef VMS
	      if (!HTUserOwnsFile)
		HTReason = HTAA_NOT_FOUND;
	      else
#endif /* VMS */
		HTReason = HTAA_INVALID_USER;
		return NO;
	    }
	    else current = mapped;
	}
    }

    if (dry)
	HTScriptPathTrans = current;
    else
	HTReqTranslated = current;
    return YES;
}


PRIVATE BOOL required ARGS4(char *,	name,
			    int,	count,
			    int,	low,
			    int,	high)
{
    if (count < low || count > high) {
	HTLog_error2("Invalid number of parameters to directive:",name);
	return NO;
    }
    else
	return YES;
}


PRIVATE void standardize ARGS1(char *, word)
{
    char * cur = word;
    char * p;

    if (!word) return;

    while (*cur) {
	*cur = TOLOWER(*cur);
	if (*cur == '-' || *cur == '_') {
	    for (p=cur; *p; p++) *p = *(p+1);
	}
	else cur++;
    }
}


PRIVATE BOOL positive ARGS1(char *, word)
{
    standardize(word);
    if (word &&
	(!strcmp(word,"yes") || !strcmp(word,"on") || !strcmp(word,"ok") ||
	 !strncmp(word,"enable",6)))
	return YES;
    else
	return NO;
}


PRIVATE BOOL negative ARGS1(char *, word)
{
    standardize(word);
    if (word &&
	(!strcmp(word,"no") || !strcmp(word,"off") || !strcmp(word,"none") ||
	 !strncmp(word,"disable",7)))
	return YES;
    else
	return NO;
}


PRIVATE BOOL unlimited ARGS1(char *, word)
{
    standardize(word);
    if (word &&
	(!strncmp(word,"unlimited",7) || !strncmp(word,"nolimit",7)))
	return YES;
    else
	return NO;
}


/*
 *	Creates a new node with pattern pat and corresponding time t.
 *
 *	If first != NULL, adds the new node as the last in that list,
 *	and returns the same list.
 *
 *	If first == NULL, creates a new list with only this node,
 *	and returns that.
 */
PRIVATE HTPatTime * pt_new ARGS3(HTPattern *,	pat,
				 time_t,	t,
				 HTPatTime *,	first)
{
    HTPatTime * pt = NULL;
    if (pat) {
	pt = (HTPatTime*)calloc(1,sizeof(HTPatTime));
	if (!pt) outofmem(__FILE__, "pt_new");
	pt->pat = pat;
	pt->time = t;
    }

    if (first) {
	first->last->next = pt;
	first->last = pt;
	return first;
    }
    else {
	pt->last = pt;
	return pt;
    }
}

PRIVATE HTPatTime * parse_pt ARGS2(char *,	params,
				   HTPatTime *,	list)
{
    char * templ = HTNextField(&params);
    HTPattern * pat;
    time_t t;

    if (!params || !templ || !parse_time(params, 24*3600, &t))
	return NULL;
    templ = HTStrip(templ);
    pat = HTPattern_new(templ);
    CTRACE(stderr, "template \"%s\", time %s\n", templ, make_time_spec(&t));
    return pt_new(pat,t,list);
}

PRIVATE time_t get_pt ARGS3(HTPatTime *,	list,
			    char *,		url,
			    time_t,		deflt)
{
    HTPatTime * pt = list;

    while (pt) {
	if (HTPattern_url_match(pt->pat,url))
	    return pt->time;
	pt = pt->next;
    }

    return deflt;
}

PUBLIC time_t get_default_expiry ARGS1(char *, url)
{
    return get_pt(cc.cache_exp, url, cc.cache_exp_def);
}

PUBLIC time_t get_unused ARGS1(char *, url)
{
    return get_pt(cc.cache_unused, url, cc.cache_unused_def);
}

PUBLIC time_t get_clean ARGS1(char *, url)
{
    return get_pt(cc.cache_clean, url, cc.cache_clean_def);
}

/*
 * Returns the time in seconds that is the maximum allowed time
 * between retrieves for the given URL (after this time cache is
 * refreshed even if the cache file hasn't expired yet).
 */
PUBLIC time_t get_refresh_interval ARGS1(char *, url)
{
    return get_pt(cc.cache_refresh_interval, url, DEFAULT_REFRESH_INTERVAL);
}

/*
 * Returns the LM factor corresponding to given URL.
 * Negative if LM factor is turned Off for this URL, and
 * DEFAULT_LM_FACTOR (something like 0.1) if no match is found.
 */
PUBLIC float get_lm_factor ARGS1(char *, url)
{
    HTPatFact * pf = cc.cache_lm_factors;

    while (pf) {
	if (HTPattern_url_match(pf->pat,url))
	    return pf->factor;
	pf = pf->next;
    }
    return DEFAULT_CACHE_LM_FACTOR;
}



PRIVATE void dir_show_config ARGS2(char *,	name,
				   char *,	value)
{
    BOOL flag = positive(value);

    if (!strncmp(name,"dirshowicons",10)) {
	if (flag)	HTDirShowMask |= HT_DIR_SHOW_ICON;
	else		HTDirShowMask &= ~HT_DIR_SHOW_ICON;
	CTRACE(stderr, "DirIcons.... %s\n", flag ? "On" : "Off");
    }
    else if (!strncmp(name,"dirshowbrackets",10)) {
	HTDirShowBrackets = flag;
	CTRACE(stderr, "ALT tags.... %ssurrounded by square brackets\n",
	       flag ? "" : "not ");
    }
    else if (!strncmp(name,"dirshowmaxdescriptionlength",12)) {
	int n = atoi(value);
	if (n > 0) {
	    HTDirMaxDescrLength = n;
	    CTRACE(stderr,"Maximum..... description length %d chars\n",n);
	}
	else {
	    HTLog_error2("Invalid DirShowMaxDescriptionLength param:",value);
	}
    }
    else if (!strncmp(name,"dirshowminlength",12)) {
	int n = atoi(value);
	if (n > 2) {
	    HTDirMinFileLength = n;
	    CTRACE(stderr,"Minimum..... chars in filename %d\n",
		   HTDirMinFileLength);
	    if (n > HTDirMaxFileLength) {
		HTDirMaxFileLength = n;
		CTRACE(stderr,"Maximum..... also set to the same value\n");
	    }
	}
	else {
	    HTLog_error2("Invalid DirShowMinLength parameter:",value);
	}
    }
    else if (!strncmp(name,"dirshowmaxlength",12)) {
	int n = atoi(value);
	if (n > 2) {
	    HTDirMaxFileLength = n;
	    CTRACE(stderr,"Maximum..... chars in filename %d\n",
		   HTDirMaxFileLength);
	    if (n < HTDirMinFileLength) {
		HTDirMinFileLength = n;
		CTRACE(stderr,"Minimum..... also set to the same value\n");
	    }
	}
	else {
	    HTLog_error2("Invalid DirShowMaxLength parameter:",value);
	}
    }
    else if (!strncmp(name,"dirshowdate",10)) {
	if (flag)	HTDirShowMask |= HT_DIR_SHOW_DATE;
	else		HTDirShowMask &= ~HT_DIR_SHOW_DATE;
	CTRACE(stderr, "DirDate..... %s\n", flag ? "On" : "Off");
    }
    else if (!strncmp(name,"dirshowsize",10)) {
	if (flag)	HTDirShowMask |= HT_DIR_SHOW_SIZE;
	else		HTDirShowMask &= ~HT_DIR_SHOW_SIZE;
	CTRACE(stderr, "DirDate..... %s\n", flag ? "On" : "Off");
    }
    else if (!strncmp(name,"dirshowbytes",10)) {
	HTDirShowBytes = flag;
	CTRACE(stderr,"DirBytes.... shown as %s\n", flag ? "bytes" : "1K");
    }
    else if (!strncmp(name,"dirshowhidden",10)) {
	if (flag)	HTDirShowMask |= HT_DIR_SHOW_HID;
	else		HTDirShowMask &= ~HT_DIR_SHOW_HID;
	CTRACE(stderr, "DirHidden... %s\n", flag ? "On" : "Off");
    }
    else if (!strncmp(name,"dirshowowner",10)) {
	if (flag)	HTDirShowMask |= HT_DIR_SHOW_OWNER;
	else		HTDirShowMask &= ~HT_DIR_SHOW_OWNER;
	CTRACE(stderr, "DirOwner.... %s\n", flag ? "On" : "Off");
    }
    else if (!strncmp(name,"dirshowgroup",10)) {
	if (flag)	HTDirShowMask |= HT_DIR_SHOW_GROUP;
	else		HTDirShowMask &= ~HT_DIR_SHOW_GROUP;
	CTRACE(stderr, "DirGroup.... %s\n", flag ? "On" : "Off");
    }
    else if (!strncmp(name,"dirshowmode",10)) {
	if (flag)	HTDirShowMask |= HT_DIR_SHOW_MODE;
	else		HTDirShowMask &= ~HT_DIR_SHOW_MODE;
	CTRACE(stderr, "DirMode..... %s\n", flag ? "On" : "Off");
    }
    else if (!strncmp(name,"dirshowdescription",10)) {
	HTDirDescriptions = flag;
	CTRACE(stderr, "DirDescribe. %s\n", flag ? "On" : "Off");
    }
    else if (!strncmp(name,"dirshowcase",10)) {
	if (flag)	HTDirShowMask |= HT_DIR_SHOW_CASE;
	else		HTDirShowMask &= ~HT_DIR_SHOW_CASE;
	CTRACE(stderr, "DirCaseSense %s\n", flag ? "On" : "Off");
    }
    else if (!strncmp(name,"dirshowhtmltitles",12)) {
	HTPeekTitles = flag;
	CTRACE(stderr, "HTML titles. %s\n", HTPeekTitles ? "On" : "Off");
    }
    else {
	HTLog_error2("Invalid DirShow directive:",name);
    }
}


PRIVATE void icon_config ARGS2(char **,	vec,
			       int,	n)
{
    if (!strcmp(vec[0], "iconpath")) {
	/*
	 *	vec[0]		vec[1]
	 *	IconPath	URL
	 */
	if (!required(vec[0], n, 1, 3)) return;
	StrAllocCopy(sc.icon_path, vec[1]);
    } else if (!strcmp(vec[0], "addicontostd")) {     /* Append to std icons */
	/*
	 *	vec[0]	vec[1]	vec[2]	vec[3]	vec[4]	...
	 *	AddIcon	URL	ALT	c/t	c/t	...
	 */
	int i;
	if (!required(vec[0],n,4,MAX_ARGS)) return;
	for(i=3; i<n; i++)
	    HTAddIcon(vec[1],vec[2],vec[i]);
    } else if (!strcmp(vec[0],"addicon")) {
	/*
	 *	vec[0]	vec[1]	vec[2]	vec[3]	vec[4]	...
	 *	AddIcon	URL	ALT	c/t	c/t	...
	 */
	int i;
	sc.icons_inited = YES;		   /* No automatic icon init anymore */
	if (!required(vec[0],n,4,MAX_ARGS)) return;
	for(i=3; i<n; i++)
	    HTAddIcon(vec[1],vec[2],vec[i]);
    } else {
	/*
	 *	vec[0]	 vec[1]	vec[2]
	 *	Add*Icon URL	ALT
	 */
	sc.icons_inited = YES;		   /* No automatic icon init anymore */
	if (!strncmp(vec[0], "addblankicon", 6)) {
	    if (!required(vec[0], n, 1, 4)) return;
	    if (n == 3)
		HTAddBlankIcon(vec[1], vec[2]);
	    else
		HTAddBlankIcon(vec[1], "");
	} else {			    /* The rest requires 3 arguments */
	    if (!required(vec[0],n,3,3)) return;
	    if (!strncmp(vec[0],"addunknownicon",6))
		HTAddUnknownIcon(vec[1],vec[2]);
	    else if (!strncmp(vec[0],"addparenticon",6))
		HTAddParentIcon(vec[1],vec[2]);
	    else if (!strncmp(vec[0],"adddiricon",6))
		HTAddDirIcon(vec[1],vec[2]);
	    else
		HTLog_error2("Unknown Icon directive:",vec[0]);
	}
    }
}

#ifdef NEW_CODE
PRIVATE void errormsg_config ARGS2(char **,	vec,
				   int,		n)
{
    HTErrorShowMask += HT_ERR_SHOW_LINKS;
    if (!strncmp(vec[0], "errormsgnolinks", 10)) {   /* No links in messages */
	/*
	 *	vec[0]
	 *	ErrorMsgNoLinks
	 */
	HTErrorShowMask -= HT_ERR_SHOW_LINKS;
    } else if (!strcmp(vec[0], "errormsgpath")) {	   /* Path to errors */
	/*
	 *	vec[0]		vec[1]
	 *	IconPath	URL
	 */
	if (!required(vec[0], n, 1, 3)) return;
	StrAllocCopy(sc.errormsg_path, vec[1]);
    } else
	HTLog_error2("Unknown Error Message directive:", vec[0]);
}
#endif /* NEW_CODE */

/*
 *	Chop off comments and split a line from config file at
 *	white-spaces and place pointers to different arguments
 *	to vec[], strip extra whitespace off.
 *
 *	Returns the number of parsed arguments.
 */
PRIVATE int parse_args ARGS2(char *,	line,
			     char **,	vec)
{
    int n = 0;
    char * start = line;
    char * p = strchr(line, '#');	/* Chop off comments */
    if (p) *p = 0;

    while (*start) {
	while (*start && WHITE(*start)) start++;
	p = start;
	while (*p) {
	    if (*p && WHITE(*p)) {
		*p++ = 0;
		vec[n++] = start;
		start = p;
		break;
	    } else if (*p == '\\') {
		if (*(p+1) == ' ') {		     /* Unescape '\ ' to ' ' */
		    char *orig=p, *dest=p+1;
		    while ((*orig++ = *dest++));
		}
	    }
	    p++;
	}
	if (start != p) {	/* End of line reached */
	    if (*start) vec[n++] = start;
	    break;
	}
	if (n >= MAX_ARGS) {
	    HTLog_errorN("Too many arguments on one line in config file, max:",
			 MAX_ARGS);
	    break;
	}
    }
#if 0
    if (TRACE) {
	int i;
	fprintf(stderr, "Configuration:\n");
	for(i=0; i<n; i++) fprintf(stderr, "\t%d: %s\n", i, vec[i]);
    }
#endif
    vec[n] = NULL;
    return n;
}


/*
 *	Load the configuration file to server config and cache
 *	config structures (sc & cc).
 *
 *	Returns	YES on success,
 *		NO  on failure (couldn't open config file).
 */
PUBLIC BOOL HTLoadConfig ARGS1(char *,	filename)
{
    FILE * fp;
    char line[LINE_LENGTH+1];
    char config[LINE_LENGTH+1];
    char * vec[MAX_ARGS+1];
    int c;

    if (!filename) return NO;

    /*
     *	Store config file names so later we can reload them
     */
    if (!sc.reloading) {
	char * dup = NULL;
	StrAllocCopy(dup,filename);
	if (!sc.rule_files) sc.rule_files = HTList_new();
	HTList_addObject(sc.rule_files, (char*)dup);
    }

    /*
     *	Open config file
     */
    fp = fopen(filename, "r");
    if (!fp) {
	HTLog_error2("Can't open configuration file:",filename);
	return NO;
    }

    /*
     *	Now parse it!
     */
    for(;;) {
	char * params;

	if (!fgets(config, LINE_LENGTH+1, fp)) break;	/* EOF or error */
	strcpy(line,config);	/* config will be left untouched */

	c = parse_args(line,vec);
	if (!c) continue;	/* Empty line or a comment */
	if (c < 2)
	    HTLog_error2("Insufficient parameters for directive:",vec[0]);

	/*
	 * Set params to point to the second word on the config line
	 */
	params = config;
	while (*params && WHITE(*params)) params++;
	while (*params && !WHITE(*params)) params++;
	while (*params && WHITE(*params)) params++;

	standardize(vec[0]);

	if (!strcmp(vec[0], "suffix") || !strcmp(vec[0], "addtype")) {
	    /*
	     *	vec[0]	vec[1]	vec[2]	    vec[3]	vec[4]
	     *	AddType	.suff	mime/type   encoding	quality
	     */
	    float quality = 1.0;
	    char * encoding = "binary";
	    REQ(3,5);
	    if (c > 3)
		encoding = vec[3];
	    if (c == 5) {
		int status = sscanf(vec[4], "%f", &quality);
		if (!status) quality = 1.0;
	    }
	    HTAddType(vec[1], vec[2], encoding, quality);

 	} else if (!strcmp(vec[0], "diraddhref")) {
	    int i;
	    /*
	     * vec[0]	vec[1]	vec[2]
	     * AddHref	URL	.suff1	... 
	     *(a pathname will be added to the URL)
	     */
	    for(i=2; i<c; i++)
		HTAddHref(vec[1],vec[i]);

	} else if (!strcmp(vec[0], "addencoding")) {
	    /*
	     *	vec[0]		vec[1]	vec[2]	  vec[3]
	     *	AddEncoding	.suff	encoding  quality
	     */
	    float quality = 1.0;
	    REQ(3,4);
	    if (c == 4) {
		int status = sscanf(vec[3], "%f", &quality);
		if (!status) quality = 1.0;
	    }
	    HTAddEncoding(vec[1], vec[2], quality);

	} else if (!strncmp(vec[0], "addlang", 7)) {
	    /*
	     *	vec[0]		vec[1]	vec[2]	   vec[3]
	     *	AddLanguage	.suff	lang_code  quality
	     */
	    float quality = 1.0;
	    REQ(3,4);
	    if (c == 4) {
		int status = sscanf(vec[3], "%f", &quality);
		if (!status) quality = 1.0;
	    }
	    HTAddLanguage(vec[1], vec[2], quality);

	} else if (!strncmp(vec[0], "metadir", 7)) {
	    StrAllocCopy(sc.meta_dir, vec[1]);

	} else if (!strncmp(vec[0], "metasuff", 8)) {
	    StrAllocCopy(sc.meta_suffix, vec[1]);

	} else if (!strncmp(vec[0], "nolog", 5)) {
	    int i = 0;
	    if (!sc.no_log) sc.no_log = HTList_new();

	    while (++i < c) {
		HTPattern * pat;
		char * cur = vec[i];
		/*
		 * Make hostnames all-lower-case
		 */
		while (*cur) {
		    *cur = TOLOWER(*cur);
		    cur++;
		}
		pat = HTPattern_new(vec[i]);
		HTList_addObject(sc.no_log, (void*)pat);
		CTRACE(stderr, "NoLogging... for host %s\n", vec[i]);
	    }

	} else if (!strncmp(vec[0], "ftpdirinfo", 6)) {
	    if (negative(vec[1]))
		HTDirInfo = HT_DIR_INFO_NONE;
	    else if (!strcmp(vec[1],"top"))
		HTDirInfo = HT_DIR_INFO_TOP;
	    else if (!strcmp(vec[1],"bottom"))
		HTDirInfo = HT_DIR_INFO_BOTTOM;
	    else
		HTLog_error2("Invalid parameter for FTPDirInfo:",vec[1]);

	    if (TRACE) {
		if (!HTDirInfo)
		    fprintf(stderr, "FTPDirInfo.. hidden\n");
		else
		    fprintf(stderr,
			    "FTPDirInfo.. shown on the %s of the listing\n",
			    HTDirInfo==1 ? "top" : "bottom");
	    }

	} else if (strstr(vec[0], "dirshow")) {	/* All DirShow* directives */
	    dir_show_config(vec[0],vec[1]);

	} else if (!strncmp(vec[0],"dircasesense",7)) {
	    BOOL flag = positive(vec[1]);
	    if (flag) HTDirShowMask |= HT_DIR_SHOW_CASE;
	    else	  HTDirShowMask &= ~HT_DIR_SHOW_CASE;
	    CTRACE(stderr, "DirCaseSense %s\n", flag ? "On" : "Off");

	} else if (strstr(vec[0], "errorurl")) {	/* Error URLs */
	    int code = atoi(vec[1]);
	    HTError_addUrl(code, vec[2]);
	
	} else if (strstr(vec[0],"icon")) {	/* All *Icon* directives */
	    icon_config(vec,c);

	} else if (!strncmp(vec[0], "serverroot", 10)) {
	    sc.server_root = tilde_to_home(vec[1]);
	    HTSaveLocallyDir = sc.server_root;		/* Henrik Aug 31, 94 */
	    CTRACE(stderr, "ServerRoot.. %s\n", sc.server_root);

	} else if (!strncmp(vec[0], "securitylevel", 5)) {
	    standardize(vec[1]);
	    if (!strcmp(vec[1],"high"))
		sc.security_level = 1;
	    else if (!strcmp(vec[1],"normal"))
		sc.security_level = 0;
	    else
		HTLog_error2("Invalid SecurityLevel parameter:",vec[1]);
	    CTRACE(stderr, "Security.... %s\n",
		   sc.security_level ? "high" : "normal");

	} else if (!strncmp(vec[0], "disable", 7)) {
	    HTMethod m = HTMethod_enum(vec[1]);
	    if (m != METHOD_INVALID) {
		sc.disabled[(int)m] = YES;
		CTRACE(stderr,"Disabled.... method %s\n", HTMethod_name(m));
	    }
	    else HTLog_error2("Invalid method to disable:",vec[1]);

	} else if (!strncmp(vec[0], "enable", 6)) {
	    HTMethod m = HTMethod_enum(vec[1]);
	    if (m != METHOD_INVALID) {
		sc.disabled[(int)m] = NO;
		CTRACE(stderr,"Enabled..... method %s\n", HTMethod_name(m));
	    }
	    else HTLog_error2("Invalid method to enable:",vec[1]);

	} else if (!strncmp(vec[0], "identitycheck", 5)) {
	    sc.do_rfc931 = positive(vec[1]);
	    CTRACE(stderr, "IdentCheck.. %s\n", sc.do_rfc931 ? "On" : "Off");

	} else if (!strncmp(vec[0], "dnslookup", 5)) {
	    sc.do_dns_lookup = positive(vec[1]);
	    CTRACE(stderr, "DNS-lookup.. %s\n", sc.do_dns_lookup ? "On":"Off");

	} else if (!strncmp(vec[0], "accepthack", 10)) {
	    /*
	     * No desire to document this... ;-)
	     */
	    sc.do_accept_hack = positive(vec[1]);
	    CTRACE(stderr,"AcceptHack.. %sabled\n",
		   sc.do_accept_hack ? "en" : "dis");

	} else if (!strcmp(vec[0], "linger")) {
	    sc.do_linger = positive(vec[1]);
	    CTRACE(stderr, "Linger...... %s\n", sc.do_linger ? "On" : "Off");

	} else if (!strncmp(vec[0], "maxcontentlengthbuf", 12)) {
	    sc.max_content_len_buf = parse_bytes(params,'K');
	    CTRACE(stderr,
		   "Maximum..... buf size for getting content-length: %dK\n",
		   sc.max_content_len_buf / 1024);

	} else if (!strncmp(vec[0], "cacheroot", 9)) {
	    if (cc.caching_explicitly_off) {
		CTRACE(stderr, "CacheRoot... ignored (caching is off)\n");
	    }
	    else {
		cc.cache_root = tilde_to_home(vec[1]);
		CTRACE(stderr, "CacheRoot... %s\n", vec[1]);
	    }

	} else if (!strcmp(vec[0], "caching")) {
	    cc.caching_explicitly_off = negative(vec[1]);
	    if (cc.caching_explicitly_off) {
		CTRACE(stderr, "Caching..... off%s\n",
		       cc.cache_root ? " (CacheRoot directive ignored)" : "");
		FREE(cc.cache_root);
	    }
	    else {
		CTRACE(stderr, "Caching..... on\n");
	    }

	} else if (!strncmp(vec[0], "nocaching", 6)) {
	    int i;

	    if (!cc.no_caching) cc.no_caching = HTList_new();

	    for(i=1; i<c; i++) {
		HTPattern * pat = HTPattern_new(vec[i]);
		if (pat) {
		    HTList_addObject(cc.no_caching, (void*)pat);
		    CTRACE(stderr,"No caching.. if matching mask %s\n",vec[i]);
		}
	    }

	} else if (!strncmp(vec[0], "cacheonly", 8)) {
	    int i;

	    if (!cc.cache_only) cc.cache_only = HTList_new();

	    for(i=1; i<c; i++) {
		HTPattern * pat = HTPattern_new(vec[i]);
		if (pat) {
		    HTList_addObject(cc.cache_only, (void*)pat);
		    CTRACE(stderr,"Cache only.. if matching mask %s\n",vec[i]);
		}
	    }

	} else if (!strncmp(vec[0], "gcdailygc", 7)) {
	    if (negative(vec[1])) {
		cc.gc_daily_gc = 0;
		CTRACE(stderr, "Daily gc.... disabled\n");
	    }
	    else {
		if (!parse_time(params, 3600, &cc.gc_daily_gc)) {
		    HTLog_error2("Invalid daily gc time spec:",params);
		    cc.gc_daily_gc = 3*3600;
		}
		TIME_TRACE("Daily gc at. at", cc.gc_daily_gc);
	    }

	} else if (!strcmp(vec[0],"gc") ||
		   !strncmp(vec[0],"garbagecollection",10)) {
	    cc.gc_disabled = negative(vec[1]);
	    CTRACE(stderr,"Garbage collection %s\n",
		   cc.gc_disabled ? "Off" : "On");

	} else if (!strncmp(vec[0], "gcmemusage", 5)) {
	    cc.gc_mem_usage_control = atoi(vec[1]);
	    if (cc.gc_mem_usage_control < 20) {
		HTLog_errorN("gc-mem-usage must be >20; using 100; small values prevent gc from being smart; you suggested:",cc.gc_mem_usage_control);
		cc.gc_mem_usage_control = 100;
	    }
	    else {
		CTRACE(stderr, "gc.......... Memory usage value set to %d\n",
		       cc.gc_mem_usage_control);
	    }

	} else if (!strcmp(vec[0], "cachesize")) {
	    if (unlimited(vec[1])) {
		cc.cache_max_k = 0;
		CTRACE(stderr, "Cache....... no size limit\n");
	    }
	    else {
		/*
		 * Here I should divide by 1024 but it seems that cache
		 * often becomes fuller so it's safer to take in a
		 * slightly (10%) smaller cache size parameter.
		 */		 
		cc.cache_max_k = parse_bytes(params,'M') / 1137;
		CTRACE(stderr,"Cache....... size %ldK\n", cc.cache_max_k);
	    }

	} else if (!strncmp(vec[0], "cachefiles", 9)) {
	    /*
	     * Not documented nor implemented.
	     */
	    if (unlimited(vec[1])) {
		cc.cache_max_f = 0;
		CTRACE(stderr,
		       "Cache....... no limit on the number of files\n");
	    }
	    else {
		cc.cache_max_f = atoi(vec[1]);
		CTRACE(stderr,"Cache....... max number of files %d\n",
		       cc.cache_max_f);
	    }

	} else if (!strcmp(vec[0], "cachelimit1")) {
	    cc.cache_limit_1 = parse_bytes(params,'K') / 1024;
	    CTRACE(stderr,
		   "Cache....... files smaller than %dK equally valuable\n",
		   cc.cache_limit_1);

	} else if (!strcmp(vec[0], "cachelimit2")) {
	    cc.cache_limit_2 = parse_bytes(params,'K') / 1024;
	    CTRACE(stderr,
		   "Cache....... files bigger than %dK will be discarded\n",
		   cc.cache_limit_2);

	} else if (!strcmp(vec[0], "cacheclean")) {
	    if (!vec[2] || !*vec[2]) {	/* OLD STYLE */
		CTRACE(stderr,
		       "OLD-STYLE... CacheClean directive, use new format\n");
		if (parse_time(params, 24*3600, &cc.cache_clean_def)) {
		    TIME_TRACE("Cache....... remove files older than",
			       cc.cache_clean_def);
		}
		else HTLog_error2("Invalid cache_clean_def time spec:",params);
	    }
	    else {		/* NEW ENHANCED FORMAT WITH TEMPLATES */
		CTRACE(stderr, "CacheClean.. ");
		cc.cache_clean = parse_pt(params,cc.cache_clean);
	    }

	} else if (!strncmp(vec[0], "cacheunused", 7)) {
	    if (!vec[2] || !*vec[2]) {	/* OLD STYLE */
		CTRACE(stderr,
		       "OLD-STYLE... CacheUnused directive, use new format\n");
		if (parse_time(params, 24*3600, &cc.cache_unused_def)) {
		    TIME_TRACE("Cache....... remove if unused",
			       cc.cache_unused_def);
		}
		else
		    HTLog_error2("Invalid cache_unused_def time spec:",params);
	    }
	    else {		/* NEW ENHANCED FORMAT WITH TEMPLATES */
		CTRACE(stderr, "CacheUnUsed. ");
		cc.cache_unused = parse_pt(params,cc.cache_unused);
	    }

	} else if (!strncmp(vec[0], "cachedefaultexp", 15)) {
	    if (!vec[2] || !*vec[2]) {
		CTRACE(stderr,
		"OLD-STYLE... CacheDefaultExpiry directive, use new format\n");
		if (parse_time(params, 24*3600, &cc.cache_exp_def)) {
		    TIME_TRACE("Cache....... default expiry after",
			       cc.cache_exp_def);
		}
		else HTLog_error2("Invalid default expiry time spec:",params);
	    }
	    else {		/* NEW ENHANCED FORMAT WITH TEMPLATES */
		CTRACE(stderr, "CacheDefaultExpiry ");
		cc.cache_exp = parse_pt(params,cc.cache_exp);
	    }

	} else if (!strncmp(vec[0], "cachelocktimeout", 10)) {
	    if (parse_time(params, 1, &cc.cache_lock_timeout)) {
		TIME_TRACE("Cache....... lock timeout",cc.cache_lock_timeout);
	    }
	    else HTLog_error2("Invalid cache lock timeout spec:", params);

	} else if (!strncmp(vec[0], "cachelastmodifiedfactor", 10) ||
		   !strncmp(vec[0], "cachelmfactor", 8)) {
	    if (c == 2 || c == 3) {	/* 2 = old, 3 = new format */
		char * factor = NULL;
		HTPatFact * pf = (HTPatFact*)calloc(1,sizeof(HTPatFact));
		if (!pf) outofmem(__FILE__, "HTLoadConfig():LM-factor");

		if (c == 3) {
		    pf->pat = HTPattern_new(vec[1]);
		    factor = vec[2];
		}
		else {
		    pf->pat = HTPattern_new("*");
		    factor = vec[1];
		}

		if (negative(factor)) {
		    pf->factor = -1;	/* Off */
		    CTRACE(stderr, "LM factor... off for %s\n",
			   c==3 ? vec[1] : "*");
		}
		else if (sscanf(factor, "%f", &pf->factor) != 1) {
		    HTLog_error2("Invalid CacheLastModifiedFactor:",factor);
		    pf->factor = 0;
		}
		else {
		    CTRACE(stderr, "LM factor... %s => %f\n",
			   c==3 ? vec[1] : "*", pf->factor);
		}

		if (cc.cache_lm_factors) {
		    cc.cache_lm_factors->last->next = pf;
		    cc.cache_lm_factors->last = pf;
		}
		else {
		    pf->last = pf;
		    cc.cache_lm_factors = pf;
		}
	    }
            else
		HTLog_errorN("Too many params for CacheLastModifiedFactor:",c);

	} else if (!strncmp(vec[0], "cacherefreshinterval", 12)) {
	    CTRACE(stderr, "CacheRefreshInterval ");
	    cc.cache_refresh_interval =
		parse_pt(params,cc.cache_refresh_interval);

	} else if (!strncmp(vec[0], "cachetimemargin", 12)) {
	    if (parse_time(params, 1, &cc.cache_time_margin)) {
		TIME_TRACE("Cache....... time margin set to",
			   cc.cache_time_margin);
	    }
	    else HTLog_error2("Invalid cache time margin spec:", params);

	} else if (!strcmp(vec[0], "keepexpired")) {
	    cc.keep_expired = positive(vec[1]);
	    CTRACE(stderr, "KeepExpired. %s\n",
		   cc.keep_expired ? "On (maximum cache capasity used)"
				   : "Off (expired files will be removed)");

	} else if (!strncmp(vec[0], "cacheexpirycheck", 14)) {
	    cc.do_exp_check = positive(vec[1]);
	    CTRACE(stderr,"ExpiryCheck. %s\n", cc.do_exp_check ? "On" : "Off");

	} else if (!strncmp(vec[0], "cachenoconnect", 10)) {
	    cc.cache_no_connect = positive(vec[1]);
	    CTRACE(stderr, "%s\n",
		   cc.cache_no_connect
	   ? "Standalone.. cache mode [no external document retrieves]"
	   : "Normal...... cache mode [docs not in cache will be retrieved]");

	} else if (!strncmp(vec[0],"proxyaccesslog",12)) {
	    sc.proxy_log_name = tilde_to_home(vec[1]);
	    CTRACE(stderr, "Proxy log... %s\n", sc.proxy_log_name);

	} else if (strstr(vec[0], "proxy") || strstr(vec[0], "gateway")) {
	    char * access = NULL;

	    if (!strcmp(vec[0],"httpproxy"))
		access = "http";
	    else if (!strcmp(vec[0],"gopherproxy"))
		access = "gopher";
	    else if (!strcmp(vec[0],"ftpproxy"))
		access = "ftp";
	    else if (!strcmp(vec[0],"waisproxy"))
		access = "wais";
	    else if (!strcmp(vec[0],"newsproxy"))
		access = "news";
	    else if (!strcmp(vec[0],"noproxy"))
		access = "no";

	    if (access) {
		char * envstr = (char *)malloc(strlen(vec[1]) + 20);
		char * envname = (char *)malloc(20);
		char * old;

		if (TRACE) {
		    if (!strcmp(access,"no"))
			fprintf(stderr,
				"Proxy....... direct access for domain %s\n",
				vec[1]);
		    else
			fprintf(stderr,
			"Proxy....... using another proxy %s for %s access\n",
			vec[1],access);
		}

		sprintf(envname, "%s_proxy", access);
		sprintf(envstr, "%s_proxy=%s", access, vec[1]);

		old = getenv(envname);
#ifndef NeXT
		putenv(envstr);
#else
		HTLog_error("xxx_proxy directive not supported on the NeXT");
		HTLog_error("Use xxx_proxy environment variables instead");
		fprintf(stderr,
			"%s_proxy directive not supported on the NeXT",access);
		fprintf(stderr,
			"Use %s_proxy environment variables instead",access);
		exit(1);
#endif
		if (old)
		    HTLog_error2("Environment variable overridden:", old);
	    }
	    else HTLog_error2("Unknown proxy directive:", vec[0]);

	} else if (!strncmp(vec[0], "suffixcasesense", 10)) {
	    HTSuffixCaseSense = positive(vec[1]);

	} else if (!strncmp(vec[0], "rulecasesense", 10)) {
	    /*
	     * Not documented, at least on Unix side.
	     * I dunno about VMS...
	     * Just discussed with duns -- no need to document this,
	     * actully we could just take this directive away...
	     */
	    HTRuleCaseSense = positive(vec[1]);

	} else if (!strncmp(vec[0], "inputtimeout", 10) ||
		   !strncmp(vec[0], "outputtimeout", 11) ||
		   !strncmp(vec[0], "scripttimeout", 11)) {
	    time_t n;

	    if (!parse_time(params, 1, &n))
		HTLog_error2("Invalid timeout specifier:", params);
	    else {
		if (vec[0][0] == 'i')
		    sc.input_timeout = n;
		else if (vec[0][0] == 'o')
		    sc.output_timeout = n;
		else
		    sc.script_timeout = n;
		CTRACE(stderr, "TimeOut..... for %s set to %ld secs\n",
		       (vec[0][0]=='i' ? "input" :
			(vec[0][0]=='o' ? "output" : "scripts")), (long)n);
	    }

	} else if (!strncmp(vec[0], "dir", 3)) {
	    standardize(vec[1]);
	    if (!strncmp(vec[0], "diraccess", 6)) {
		if (!strncmp(vec[1], "selective", 3))
		    HTDirAccess = HT_DIR_SELECTIVE;
		else if (positive(vec[1]))
		    HTDirAccess = HT_DIR_OK;
		else if (negative(vec[1]))
		    HTDirAccess = HT_DIR_FORBID;
		else {
		    CTRACE(stderr,"Unknown..... DirAccess mode \"%s\"\n",
			   vec[1]);
		    HTLog_error2("Unknown DirAccess mode:", vec[1]);
		}
	    }
	    else if (!strncmp(vec[0], "dirreadme", 7)) {
		if (!strncmp(vec[1], "bottom", 3))
		    HTDirReadme = HT_DIR_README_BOTTOM;
		else if (!strncmp(vec[1], "top", 3))
		    HTDirReadme = HT_DIR_README_TOP;
		else if (negative(vec[1]))
		    HTDirReadme = HT_DIR_README_NONE;
		else
		    HTLog_error2("Unknown DirReadme mode:", vec[1]);
	    }
	    else
		HTLog_error2("Unknown Dir directive:", vec[0]);

	} else if (!strcmp(vec[0], "setuid")) {
	    /*
	     * Not documented because semantics is still vague...
	     */
	    sc.do_setuid = positive(vec[1]);
	    CTRACE(stderr,"SetUid...... turned %s\n",sc.do_setuid?"on":"off");

	} else if (!strcmp(vec[0], "userid")) {
	    StrAllocCopy(sc.user_id, vec[1]);
	    CTRACE(stderr, "Default..... user id %s\n", vec[1]);

	} else if (!strcmp(vec[0], "groupid")) {
	    StrAllocCopy(sc.group_id, vec[1]);
	    CTRACE(stderr, "Default..... group id %s\n", vec[1]);

	} else if (!strncmp(vec[0], "parentuserid", 10)) {
	    StrAllocCopy(sc.parent_uid, vec[1]);
	    CTRACE(stderr, "Parent...... user id %s\n", vec[1]);

	} else if (!strncmp(vec[0], "parentgroupid", 10)) {
	    StrAllocCopy(sc.parent_gid, vec[1]);
	    CTRACE(stderr, "Parent...... group id %s\n", vec[1]);

	} else if (!strncmp(vec[0],"accesslog",9) ||
		   !strcmp(vec[0],"logfile")) {
	    sc.access_log_name = tilde_to_home(vec[1]);
	    CTRACE(stderr, "AccessLog... %s\n", sc.access_log_name);

	} else if (!strncmp(vec[0],"cacheaccesslog",12)) {
	    sc.cache_log_name = tilde_to_home(vec[1]);
	    CTRACE(stderr, "Cache log... %s\n", sc.cache_log_name);

	} else if (!strcmp(vec[0], "errorlog")) {
	    sc.error_log_name = tilde_to_home(vec[1]);
	    CTRACE(stderr, "ErrorLog.... %s\n", sc.error_log_name);

	} else if (!strcmp(vec[0], "logfiledateext")) {
	    StrAllocCopy(sc.log_file_date_ext, vec[1]);
	    CTRACE(stderr, "Date Suffix.. for logfiles '%s\'\n",
		   sc.log_file_date_ext);

	} else if (!strncmp(vec[0], "logformat", 7) ||
		   !strncmp(vec[0], "logfileformat", 8)) {
	    standardize(vec[1]);
	    if (!strcmp(vec[1],"new") || !strcmp(vec[1],"common"))
		sc.new_logfile_format = YES;
	    else if (!strcmp(vec[1],"old"))
		sc.new_logfile_format = NO;
	    else
		HTLog_error2("Unknown logfile format [use Old or Common]:",
			     vec[1]);

	} else if (!strncmp(vec[0], "logtime", 4)) {
	    standardize(vec[1]);
	    if (!strcmp(vec[1],"gmt"))
		sc.use_gmt = YES;
	    else if (!strncmp(vec[1], "localtime", 5))
		sc.use_gmt = NO;
	    else
		HTLog_error2("Unknown logtime [use GMT or LocalTime]:",vec[1]);

	} else if (!strncmp(vec[0], "putscript", 9)) {
	    rc.put_script = tilde_to_home(vec[1]);
	    CTRACE(stderr,"PUT......... handled by script \"%s\"\n", vec[1]);

	} else if (!strncmp(vec[0], "postscript", 10)) {
	    rc.post_script = tilde_to_home(vec[1]);
	    CTRACE(stderr,"POST........ handled by script \"%s\"\n", vec[1]);

	} else if (!strncmp(vec[0], "deletescript", 10)) {
	    rc.delete_script = tilde_to_home(vec[1]);
	    CTRACE(stderr,"DELETE...... handled by script \"%s\"\n", vec[1]);

	} else if (!strncmp(vec[0], "userdir", 7)) {
	    CTRACE(stderr,
		   "UserDirs.... enabled [\"%s\" under each user's home]\n",
		   vec[1]);
	    StrAllocCopy(sc.user_dir, vec[1]);

	} else if (!strncmp(vec[0], "alwayswelcome", 9)) {
	    sc.always_welcome = positive(vec[1]);
	    CTRACE(stderr,
		   "Directory... names %sget redirected to welcome pages\n",
		   sc.always_welcome ? "" : "don't ");

	} else if (!strncmp(vec[0], "welcome", 7)) {
	    HTAddWelcome(vec[1]);
	    CTRACE(stderr,"Welcome pages are called \"%s\"\n", vec[1]);

	} else if (!strncmp(vec[0], "htbin", 5)) {
	    char *bindir = (char*)malloc(strlen(vec[1]) + 3);
	    if (!bindir) outofmem(__FILE__, "HTSetConfiguration");
	    strcpy(bindir, vec[1]);
	    strcat(bindir, "/*");
	    HTAppendRule(HT_Exec, "/htbin/*", bindir);
	    CTRACE(stderr, "HTBin....... obsolete rule, treated as:\n");
	    CTRACE(stderr, "............ Exec /htbin/* %s\n",bindir);
	    free(bindir);

	} else if (!strncmp(vec[0], "search", 6)) {
	    rc.search_script = tilde_to_home(vec[1]);
	    CTRACE(stderr, "Search...... handled by %s\n",rc.search_script);

	} else if (!strcmp(vec[0], "hostname")) {
	    CTRACE(stderr, "HostName.... is \"%s\"\n",vec[1]);
	    StrAllocCopy(sc.hostname,vec[1]);
	    HTSetHostName(vec[1]);			    /* Henrik Aug 28 */

#ifdef VMS
        } else if (!strcmp(vec[0], "spawninit")) {
	   if (strchr(vec[1], '/') != NULL) {
              /** Must begin with '/' **/
	      if (vec[1][0] == '/') {
	         StrAllocCopy(HTSpawnInit, HTVMS_name("",vec[1]));
	         CTRACE(stderr,
	               "SpawnInit... designated as \"%s\"\n", HTSpawnInit);
	      } else
	         CTRACE(stderr,
	    	       "SpawnInit... \" %s\" not substituted, bad format!\n",
	    			     vec[1]);
	    } else {
	       StrAllocCopy(HTSpawnInit, vec[1]);
	       CTRACE(stderr, "SpawnInit... designated as \"%s\"\n", HTSpawnInit);
            }

        } else if (!strcmp(vec[0], "scratchdir")) {
           if (strchr(vec[1], '/') != NULL) {
	      /** Must begin and end with '/' **/
	      if (vec[1][0] == '/' && vec[1][strlen(vec[1])-1] == '/') {
	         StrAllocCopy(HTScratchDir, HTVMS_name("",vec[1]));
	         CTRACE(stderr,
	               "ScratchDir.. scratch files designated into \"%s\"\n",
		   		     HTScratchDir);
	      } else
	         CTRACE(stderr, "ScratchDir.. %s not used, bad format!\n",
					     vec[1]);
	   } else {
	      StrAllocCopy(HTScratchDir, vec[1]);
	      CTRACE(stderr,
	           "ScratchDir.. scratch files designated into \"%s\"\n",
		   		 HTScratchDir);
	   }
#endif /* VMS */

	} else if (!strcmp(vec[0], "servertype")) {
	    standardize(vec[1]);
	    if (!strcmp(vec[1],"standalone"))
		sc.server_type = SERVER_TYPE_STANDALONE;
	    else if (!strcmp(vec[1],"inetd"))
		sc.server_type = SERVER_TYPE_INETD;
	    else
		HTLog_error2("Invalid ServerType:",vec[1]);
	    CTRACE(stderr, "ServerType.. %s\n",
		   sc.server_type == SERVER_TYPE_STANDALONE ? "StandAlone" :
		   sc.server_type == SERVER_TYPE_INETD ? "Inetd" : "Default");

	} else if (!strcmp(vec[0], "port")) {
	    if (sc.port) {
		CTRACE(stderr,
		       "Ignoring.... Port %s directive (-p %d overrides it)\n",
		       vec[1], sc.port);
	    }
	    else {
		sc.port = atoi(vec[1]);
		if (sc.port > 0) {
		    CTRACE(stderr, "Port........ %d\n", sc.port);
		}
		else {
		    HTLog_error2("Invalid param to Port directive:",vec[1]);
		}
	    }

	} else if (!strncmp(vec[0],"pidfile",7)) {
	    sc.pid_file = tilde_to_home(vec[1]);
	    CTRACE(stderr, "PidFile..... %s\n", sc.pid_file);

	} else if (!strcmp(vec[0],"protection")) {
	    CTRACE(stderr, "Protection.. definition for name \"%s\"\n",vec[1]);
	    if (vec[2] && strchr(vec[2],'{'))
		HTAddNamedProtection(vec[1],HTAAProt_parseInlined(fp));
	    else
		HTLog_error2("Syntax error in config file, expecting '{' to start protection definition of",vec[1]);

	} else {
	    HTRuleOp op;
                     op = !strcmp(vec[0], "map")		? HT_Map
		        : !strcmp(vec[0], "pass")		? HT_Pass
			: !strcmp(vec[0], "fail")		? HT_Fail
			: !strcmp(vec[0], "exec")		? HT_Exec
                        : 					  HT_Invalid;
            if (op==HT_Invalid) {
		     op = !strncmp(vec[0], "redirect", 5)	? HT_Redirect
			: !strncmp(vec[0], "move", 4)		? HT_Moved
			: !strncmp(vec[0], "defprot", 7)	? HT_DefProt
			: !strncmp(vec[0], "protect", 7)	? HT_Protect
			: !strncmp(vec[0], "useproxy", 6)	? HT_UseProxy
			:					  HT_Invalid;
            }
	    if (op==HT_Invalid) {
		HTLog_error2("Bad configuration directive:", config);
	    }
	    else if (op==HT_Protect || op==HT_DefProt) {
		if (vec[2] && strchr(vec[2], '{'))
		    HTAddInlinedProtRule(op,vec[1],HTAAProt_parseInlined(fp));
		else if (op==HT_Protect || op==HT_DefProt)
		    HTAddProtRule(op, vec[1], vec[2], vec[3]);
	    }
	    else {
		HTAppendRule(op, vec[1], vec[2]);
	    }
	}
    }

    fclose(fp);
    return YES;
}


PRIVATE BOOL reload_config NOARGS
{
    HTList * cur = sc.rule_files;
    char * filename;
    BOOL flag = YES;

    CTRACE(stderr, "Clearing.... rules\n");
    clear_rules();

    sc.reloading = YES;

    CTRACE(stderr, "Reloading... rules\n");
    while ((filename = (char*)HTList_nextObject(cur)))
	if (!HTLoadConfig(filename)) flag = NO;

    return flag;
}



PRIVATE void icon_init ARGS1(BOOL, call_libwww)
{
    char * icon_url_prefix = NULL;
    char * icon_dir = NULL;

    if (!sc.hostname) sc.hostname = (char*) HTGetHostName();
    if (sc.icon_path) {
	StrAllocCopy(icon_dir, sc.icon_path);
	if (*(icon_dir+strlen(icon_dir)-1) != '/') {
	    StrAllocCat(icon_dir, "/*");
	} else {
	    StrAllocCat(icon_dir, "*");
	}
    } else {
	StrAllocCopy(icon_dir, sc.server_root);
	if (*(icon_dir+strlen(icon_dir)-1) != '/') {
	    StrAllocCat(icon_dir, "/icons/*");
	} else {
	    StrAllocCat(icon_dir, "icons/*");
	}
	HTPrependPass("/httpd-internal-icons/*", icon_dir);

    }

    CTRACE(stderr, "Automatic... icon initialization at `%s\'\n", icon_dir);

    if (sc.hostname) {
	char * icon_url_templ = NULL;
	char portstr[10];
	if (sc.port && sc.port != 80)
	    sprintf(portstr,":%d",sc.port);
	else
	    *portstr = 0;

	icon_url_prefix = (char*)malloc(strlen(sc.hostname)+100);
	if (!icon_url_prefix) outofmem(__FILE__, "HTServerInit");
	if (sc.icon_path)
	    strcpy(icon_url_prefix, sc.icon_path);
	else {
	    sprintf(icon_url_prefix,
		    "http://%s%s/httpd-internal-icons/", sc.hostname, portstr);
	    /*
	     * Pass also absolute URL to proxy server itself
	     */
	    StrAllocCopy(icon_url_templ, icon_url_prefix);
	    StrAllocCat(icon_url_templ, "*");
	    HTPrependPass(icon_url_templ, icon_dir);
	    FREE(icon_url_templ);
	}
    }
    else {
	HTLog_error("Can't get hostname - icon URLs won't have host name");
	HTPrependPass("/httpd-internal-icons/*", "/icons/*");
	StrAllocCopy(icon_url_prefix, "/httpd-internal-icons/");
    }

    CTRACE(stderr,"Icon URL.... prefix is %s\n", icon_url_prefix);
    CTRACE(stderr,"Icons....... should be in %s\n", icon_dir);

    if (call_libwww)
	HTStdIconInit(icon_url_prefix);

    FREE(icon_dir);
    FREE(icon_url_prefix);
}

PUBLIC BOOL HTServerInit NOARGS
{
    static BOOL first_time = YES;
    BOOL flag = YES;

    if (!first_time) {
	HTLog_closeAll();
	if (!reload_config()) flag = NO;
    }

    /*
     * Open access, error and cache access logs
     */
    if (!HTLog_openAll()) flag = NO;

#ifdef NEW_CODE 
    /* Initialize default error messages */
    errormsg_init();
#endif /* NEW_CODE */

    /*
     * Initialize default icons; also call the libwww icon init routine.
     */
    if (sc.server_root && !sc.icons_inited &&
	(HTDirShowMask & HT_DIR_SHOW_ICON))
	icon_init(first_time);

    if (first_time)
	first_time = NO;

    return flag;
}

