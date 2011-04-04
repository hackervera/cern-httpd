
/* MODULE							HTAAServ.c
**		SERVER SIDE ACCESS AUTHORIZATION MODULE
**
**	Contains the means for checking the user access
**	authorization for a file.
**
** IMPORTANT:
**	Routines in this module use dynamic allocation, but free
**	automatically all the memory reserved by them.
**
**	Therefore the caller never has to (and never should)
**	free() any object returned by these functions.
**
**	Therefore also all the strings returned by this package
**	are only valid until the next call to the same function
**	is made. This approach is selected, because of the nature
**	of access authorization: no string returned by the package
**	needs to be valid longer than until the next call.
**
**	This also makes it easy to plug the AA package in:
**	you don't have to ponder whether to free() something
**	here or is it done somewhere else (because it is always
**	done somewhere else).
**
**	The strings that the package needs to store are copied
**	so the original strings given as parameters to AA
**	functions may be freed or modified with no side effects.
**
**	The AA package does not free() anything else than what
**	it has itself allocated.
**
** AUTHORS:
**	AL	Ari Luotonen	luotonen@dxcern.cern.ch
**
** HISTORY:
**
**
** BUGS:
**
**
*/

#include <stdio.h>		/* FILE */
#include <string.h>		/* strchr() */
#include <time.h>

#include "HTUtils.h"
#include "HTString.h"
#include "HTAccess.h"		/* HTSecure			*/
#include "HTFile.h"		/*				*/
#include "HTMulti.h"		/* Multiformat handling		*/
#include "HTRules.h"		/* 				*/
#include "HTParse.h"		/* URL parsing function		*/
#include "HTList.h"		/* HTList object		*/

#include "HTAAUtil.h"		/* AA common parts		*/
#include "HTAuth.h"		/* Authentication		*/
#include "HTACL.h"		/* Access Control List		*/
#include "HTGroup.h"		/* Group handling		*/
#include "HTAAProt.h"		/* Protection file parsing	*/
#include "HTAAServ.h"		/* Implemented here		*/
#include "HTDaemon.h"
#include "HTConfig.h"
#include "HTLog.h"


#define META_DIR	".web"
#define META_SUFFIX	".meta"

#ifdef VMS
#define gmtime localtime
#endif 

/*
** Global variables
*/
PUBLIC time_t theTime;
extern char * HTClientHostName;

/*
** Module-wide global variables
*/



/* SERVER PUBLIC					HTAA_statusMessage()
**		RETURN A STRING EXPLAINING ACCESS
**		AUTHORIZATION FAILURE
**		(Can be used in server reply status line
**		 with 401/403/404 replies.)
** ON ENTRY:
**	req	failing request.
** ON EXIT:
**	returns	a string containing the error message
**		corresponding to internal fail reason.
**		Sets the message also to HTReasonLine.
*/
PUBLIC char *HTAA_statusMessage ARGS1(HTRequest *, req)
{
    if (!req)
	return "Internal error, request structure pointer is NULL!";

    if (HTReasonLine)
	return HTReasonLine;

    switch (HTReason) {

    /* 401 cases */
      case HTAA_NO_AUTH:
	HTReasonLine = "Unauthorized - authentication failed";
	break;
      case HTAA_NOT_MEMBER:
	HTReasonLine = "Unauthorized to access the document";
	break;

    /* 403 cases */
      case HTAA_BY_RULE:
	HTReasonLine = "Forbidden - by rule";
	break;
      case HTAA_IP_MASK:
	HTReasonLine ="Forbidden - server refuses to serve to your IP address";
	break;
      case HTAA_IP_MASK_PROXY:
	HTReasonLine = "Proxy server refuses to serve to your IP address [at least with this HTTP method]";
	break;
      case HTAA_NO_ACL:
	HTReasonLine = "Forbidden - access to file is never allowed [no ACL file]";
	break;
      case HTAA_NO_ENTRY:
	HTReasonLine =
	    "Forbidden - access to file is never allowed [no ACL entry]";
	break;
      case HTAA_SETUP_ERROR:
	HTReasonLine = "Forbidden - server protection setup error [probably protection setup file not found or syntax error]";
	break;
      case HTAA_DOTDOT:
	HTReasonLine =
	    "Forbidden - URL containing .. forbidden [don't try to break in]";
	break;
      case HTAA_HTBIN:
	HTReasonLine = "Forbidden - /htbin feature not enabled on this server";
	break;
      case HTAA_INVALID_REDIRECT:
	HTReasonLine = "Forbidden - invalid redirection in configuration file";
	break;
      case HTAA_INVALID_USER:
	HTReasonLine = "Forbidden - bad user directory";
	break;
      case HTAA_NOT_ALLOWED:
	HTReasonLine = "Forbidden - PUT/DELETE must be explicitly allowed in server's protection setup";
	break;

    /* 404 cases */
      case HTAA_NOT_FOUND:
	HTReasonLine =
	    "Not found - file or directory doesn't exist or is read protected";
	break;
      case HTAA_MULTI_FAILED:
	HTReasonLine = "Not found - file doesn't exist or is read protected [even tried multi]";
	break;

    /* Success */
      case HTAA_OK:
	HTReasonLine = "Document follows"; 
	break;

      case HTAA_OK_GATEWAY:
	HTReasonLine = "Gatewaying";
	break;

      case HTAA_OK_MOVED:
	HTReasonLine = "Moved";
	break;

      case HTAA_OK_REDIRECT:
	HTReasonLine = "Found";
	break;

    /* Others */
      default:
	HTReasonLine = "Access denied - unable to specify reason [bug]";

    } /* switch */

    return HTReasonLine;
}




typedef struct _allowed {
    HTList * public;
    HTList * allowed;
} allowed;



PRIVATE GroupDef * get_mask ARGS2(HTAAProt *,	prot,
				  HTMethod,	method)
{
    GroupDef * mask = NULL;

    if (prot) {
	char * method_name = HTMethod_name(method);

	if (!strcmp(method_name, "GET") || !strcmp(method_name, "HEAD"))
	    mask = prot->get_mask;
	else if (!strcmp(method_name, "PUT"))
	    mask = prot->put_mask;
	else if (!strcmp(method_name, "POST"))
	    mask = prot->post_mask;
	else if (!strcmp(method_name, "DELETE"))
	    mask = prot->delete_mask;
	if (!mask)
	    mask = prot->gen_mask;
    }
    return mask;
}



PRIVATE char * get_meta_file ARGS1(char *, name)
{
    char * base = NULL;
    char * meta_file = NULL;

    if (!name) return NULL;
    if (!sc.meta_dir) StrAllocCopy(sc.meta_dir, META_DIR);
    if (!sc.meta_suffix) StrAllocCopy(sc.meta_suffix, META_SUFFIX);

    base = strrchr(name, '/');
    if (!base) return NULL;
    *base++ = 0;

    meta_file = (char*)malloc(strlen(name) + strlen(sc.meta_dir) +
			      strlen(base) + strlen(sc.meta_suffix) + 3);

    sprintf(meta_file, "%s/%s/%s%s", name, sc.meta_dir, base, sc.meta_suffix);

    *(base-1) = '/';
    return meta_file;
}

/* PRIVATE						check_authorization()
**		CHECK IF USER IS AUTHORIZED TO ACCESS A FILE
** ON ENTRY:
**	req	data structure representing the request.
**	no_acls	if true, no ACL lookups will be performed.
**		This is for proxy protection for which (non-local files)
**		ACLs make no sense.
** ON EXIT:
**	returns	HTAA_OK on success.
**		Otherwise the reason for failing.
** NOTE:
**	This function does not check whether the file
**	exists or not -- so the status  404 Not found
**	must be returned from somewhere else (this is
**	to avoid unnecessary overhead of opening the
**	file twice).
*/
PRIVATE HTAAFailReason check_authorization ARGS2(HTRequest *,	req,
						 BOOL,		no_acls)
{
    HTAAFailReason reason;
    GroupDef *mask_group;
    GroupDef *allowed_groups;
    FILE *acl_file = NULL;
    char *check_this;
    BOOL explicit_ok_required = NO;
    HTUser = NULL;

    if (!req || req->method == METHOD_INVALID)
	return HTAA_BY_RULE;

    if (req->method == METHOD_PUT  ||  req->method == METHOD_DELETE) {
	explicit_ok_required = YES;
	if (!HTProt) {
	    HTProt = HTDefProt;
	    if (HTProt && HTProt->ids) HTSetCurrentIds(HTProt->ids);
	    CTRACE(stderr, "%s for %s\n",
		   HTDefProt
		   ? "Using....... Default Protection"
		   : "Missing..... Protect and DefProt",
		   HTMethod_name(req->method));
	}
    }

    if (HTReqScript) {
	check_this = HTReqScript;
	CTRACE(stderr, "AuthCheck... Translated script name: \"%s\"\n",
	       HTReqScript);
    }
    else if (HTReqTranslated) {
	check_this = HTReqTranslated;
	CTRACE(stderr, "AuthCheck... Translated path: \"%s\" (method: %s)\n",
	       HTReqTranslated, HTMethod_name(req->method));
    }
    else {
	CTRACE(stderr, "AuthCheck... Forbidden by rule\n");
	return HTAA_BY_RULE;
    }


    /*
     * Check ACL existence
     */
    if (no_acls  ||  !(acl_file = HTAA_openAcl(check_this))) {
	if (HTProt) { /* protect rule, but no ACL */
	    mask_group = get_mask(HTProt, req->method);
	    if (mask_group) {
		GroupDefList *group_def_list = NULL;
		char * grfile = HTAssocList_lookup(HTProt->values,"groupfile");

		if (!grfile)
		    grfile = HTAssocList_lookup(HTProt->values, "group-file");

		/*
		 * Only mask enabled, check that
		 */
		group_def_list = HTAA_readGroupFile(grfile);

		/*
		 * Authenticate if authentication info given
		 */
		if (req->scheme!=HTAA_UNKNOWN && req->scheme!=HTAA_NONE) {
		    HTAA_authenticate(req);
		    CTRACE(stderr, "Authentic... %s\n",
			   (HTUser ? HTUser->username : "-nobody-"));
		}
		HTAA_resolveGroupReferences(mask_group,
					    group_def_list);
		reason = HTAA_userAndInetInGroup(mask_group,
						 HTUser
						  ? HTUser->username : "",
						 HTClientHost,
						 HTClientHostName);
		if (reason == HTAA_OK) {
		    CTRACE(stderr, "Accepted.... by Mask %s\n",
			   no_acls ? "for proxy" : "(no ACL, only Protect)");
		    if (no_acls)
			return HTAA_OK_GATEWAY;
		    else
			return HTAA_OK;
		}
		else {
		    CTRACE(stderr, "Denied...... by Mask %s\n",
			   no_acls ? "for proxy" : "(no ACL, only Protect)");
		    return reason;
		}
		/* NEVER REACHED */
	    }
	    else {	/* 403 Forbidden */
		CTRACE(stderr, "Protected... but no Mask %s -- forbidden\n",
		       no_acls ? "which is mandatory in proxy protection"
		               : "nor ACL");
		return HTAA_NO_ACL;
	    }
	    /* NEVER REACHED */
	}
	else { /* No protect rule and no ACL => OK 200 */
	    if (explicit_ok_required) {
		CTRACE(stderr, "Forbidden... method not explicitly allowed\n");
		return HTAA_NOT_ALLOWED;
	    }
	    CTRACE(stderr, "AccessOk.... No protect rule %s\n",
		   no_acls ? "for proxy access" : "nor ACL");
	    if (no_acls)
		return HTAA_OK_GATEWAY;
	    else
		return HTAA_OK;
	}
	/* NEVER REACHED */
    }

    /*
     * Now we know that ACL exists
     */
    if (!HTProt) {		/* Not protected by "protect" rule */
	CTRACE(stderr, "Default..... protection\n");
	HTProt = HTDefProt;  /* Get default protection */
	if (HTProt && HTProt->ids) HTSetCurrentIds(HTProt->ids);

	if (!HTProt) {	/* @@ Default protection not set ?? */
	    CTRACE(stderr, "SETUP ERROR. Default protection not set!!\n");
	    return HTAA_SETUP_ERROR;
	}
    }

    /*
     * Now we know that document is protected and ACL exists.
     * Check against ACL entry.
     */
    {
	GroupDefList *group_def_list = NULL;
	char * grfile = HTAssocList_lookup(HTProt->values, "groupfile");

	if (!grfile)
	    grfile = HTAssocList_lookup(HTProt->values, "group-file");

	group_def_list = HTAA_readGroupFile(grfile);

	/*
	 * Authenticate now that we know protection mode
	 */
	if (req->scheme != HTAA_UNKNOWN  &&  req->scheme != HTAA_NONE) {
	    HTAA_authenticate(req);
	    CTRACE(stderr, "Authentic... %s\n",
		   (HTUser ? HTUser->username : "-nobody-"));
	}
	/* 
	 * Check mask group
	 */
	mask_group = get_mask(HTProt, req->method);
	if (mask_group) {
	    if (HTProt->acl_override) {
		CTRACE(stderr, "ACLOverRide. %s (no mask checking)\n",
		       "ACL's configured to override Masks");
	    }
	    else {
		HTAA_resolveGroupReferences(mask_group, group_def_list);
		reason=HTAA_userAndInetInGroup(mask_group,
					       HTUser ? HTUser->username :"",
					       HTClientHost,
					       HTClientHostName);
		if (reason != HTAA_OK) {
		    CTRACE(stderr, "Denied...... by mask, host: %s\n",
			   HTClientHostName ? HTClientHostName : HTClientHost);
		    return reason;
		}
		else {
		    CTRACE(stderr, "Accepted.... by mask, host: %s\n",
			   HTClientHostName ? HTClientHostName : HTClientHost);
		    /* And continue authorization checking */
		}
	    }
	}

	/*
	 * Get ACL entries; get first one first, the loop others
	 * Remember, allowed_groups is automatically freed by
	 * HTAA_getAclEntry().
	 */
	allowed_groups =
	    HTAA_getAclEntry(acl_file, check_this, req->method);
	if (!allowed_groups) {
	    CTRACE(stderr, "AA notice... No ACL entry for \"%s\"\n", check_this);
	    HTAA_closeAcl(acl_file);
	    return HTAA_NO_ENTRY;	/* Forbidden -- no entry in the ACL */
	}
	else {
	    do {
		HTAA_resolveGroupReferences(allowed_groups, group_def_list);
		reason = HTAA_userAndInetInGroup(allowed_groups,
						 HTUser
						 ? HTUser->username : "",
						 HTClientHost,
						 HTClientHostName);
		if (reason == HTAA_OK) {
		    HTAA_closeAcl(acl_file);
		    return HTAA_OK;	/* OK */
		}
		allowed_groups =
		    HTAA_getAclEntry(acl_file, check_this, req->method);
	    } while (allowed_groups);
	    HTAA_closeAcl(acl_file);
	    return HTAA_NOT_MEMBER;	/* Unauthorized */
	}
    }
}



PUBLIC void HTSetAttributes ARGS2(HTRequest *,	 req,
				  struct stat *, stat_info)
{
    if (!req || !stat_info)
	return;

    if (S_ISREG(stat_info->st_mode)) {
	out.content_length = stat_info->st_size;
	CTRACE(stderr, "Content-Length %d\n", out.content_length);
    }

    {
	char * s = http_time(&stat_info->st_mtime);
	StrAllocCopy(HTLastModified, s);
	CTRACE(stderr, "Last-Modified %s\n", HTLastModified);
    }
}


PUBLIC BOOL HTGetAttributes ARGS2(HTRequest *,	req,
				  char *,	path)
{
    struct stat stat_info;

    if (!path || HTStat(path, &stat_info) == -1)
	return NO;

    HTSetAttributes(req, &stat_info);
    return YES;
}


#ifdef OLD_CODE

/* Replaced by HTCanon() */

/*
 * Converts the hostname (if any) in the given URL to all-lower-case
 */
PRIVATE void lowcase_hostname ARGS1(char *, url)
{
    char * p;

    if (!url || !(p = strchr(url,'/')) || p==url)
	return;

    if (*(p-1)==':' && *(p+1)=='/') {
	CTRACE(stderr,"Converting.. hostname to low-case in URL \"%s\"\n",url);
	for (p += 2;  *p && *p!='/';  p++)
	    *p = TOLOWER(*p);
	CTRACE(stderr,"Result...... \"%s\"\n",url);
    }
}
#endif /* OLD_CODE */

/* PUBLIC					      HTAA_checkAuthorization()
**		CHECK IF USER IS AUTHORIZED TO ACCESS A FILE
** ON ENTRY:
**	req	data structure representing the request.
**
** ON EXIT:
**	returns	status codes uniform with those of HTTP:
**	  200 OK	   if file access is ok.
**	  401 Unauthorized if user is not authorized to
**			   access the file.
**	  403 Forbidden	   if there is no entry for the
**			   requested file in the ACL.
**
** NOTE:
**	This function does not check whether the file
**	exists or not -- so the status  404 Not found
**	must be returned from somewhere else (this is
**	to avoid unnecessary overhead of opening the
**	file twice).
**
*/
PUBLIC int HTAA_checkAuthorization ARGS1(HTRequest *, req)
{
    BOOL proxy_forbidden = NO;
    char * untranslated = NULL;

    if (!req  ||  !HTReqArg  ||  HTReason != HTAA_OK) {
	if (req && out.status_code == 200)
	    out.status_code = 403;
	if (req && !HTReasonLine)
	    HTAA_statusMessage(req);
	return 403;
    }

    /*
     * Convert hostname (if any) to all-lower-case
     */
#if 0
    lowcase_hostname(HTReqArgPath);
#endif

    /*
     * Kludge for proxy to see if rule system mapped it, and if
     * not pass it as it came in [we have problems with bad servers
     * expecting URLs exactly as they output them, even though they
     * are not in canonical form].
     */
    StrAllocCopy(untranslated, HTReqArgPath);
	
    /*
    ** Translate into absolute pathname, and
    ** get protection by "protect" and "defprot" rules.
    */
    if (HTTranslateReq(req) &&
	HTReason != HTAA_INVALID_REDIRECT &&
	HTReason != HTAA_OK_REDIRECT &&
	HTReason != HTAA_OK_MOVED) {

	if (HTReqScript) {	/* Script request */
	    HTReason = check_authorization(req, NO);
	}
	else {			/* Not a script request */
	    static char * access = NULL;

	    FREE(access);
	    access = HTParse(HTReqTranslated, "", PARSE_ACCESS);

	    if (!access || !*access || !strcmp(access,"file")) {
		BOOL multi_failed = NO;

		/*
		** Local file =>
		** Check that file really exists, do multiformat handling,
		** get content-length etc and do AA check.
		*/
		if (req->method == METHOD_GET || req->method == METHOD_HEAD) {

		    struct stat stat_info, stat_info2;
		    char * physical= HTMulti(req, HTReqTranslated, &stat_info);

		    if (physical) {
			free(HTReqTranslated);
			HTReqTranslated = physical;

			/*
			** If it's a directory and our URL doesn't end in a
			** slash, find out if we could send a redirection
			** to the welcome page on that directory instead.
			*/
			if (sc.always_welcome && S_ISDIR(stat_info.st_mode) &&
			    physical[ strlen(physical)-1 ] != '/') {

			    char * welcome = NULL;
			    StrAllocCopy(welcome, physical);
			    StrAllocCat(welcome, "/");
			    physical = HTMulti(req, welcome, &stat_info2);
			    free(welcome);

			    if (physical && !S_ISDIR(stat_info2.st_mode)) {
				char * escaped = HTEscape(HTReqArgPath,
							  URL_PATH);
				int keylen = HTReqArgKeywords ?
				    strlen(HTReqArgKeywords) : 0;

				HTLocation = (char*)malloc(strlen(escaped) +
							   keylen + 3);
				sprintf(HTLocation, "%s/%s%s",
					escaped,
					keylen ? "?" : "",
					keylen ? HTReqArgKeywords : "");
				free(escaped);
				free(physical);
				HTReason = HTAA_OK_REDIRECT;
				goto done;
			    }
			    FREE(physical);
			}

			HTSetAttributes(req, &stat_info);
			req->content_type=HTFileFormat(HTReqTranslated,
						       &req->content_encoding,
						       &req->content_language);
			HTMetaFile = get_meta_file(HTReqTranslated);
		    }
		    else multi_failed = YES;
		}

		/*
		** Check authorization even if the file doesn't exist to find
		** out if we can give even that information to the user.
		*/
		HTReason = check_authorization(req, NO);

		/*
		** Tell that file doesn't exist *only* if user is
		** authorized to get that information.  Otherwise
		** just say "not authorized" instead of "not found".
		*/
		if (multi_failed  &&  HTReason == HTAA_OK)
		    HTReason = HTAA_MULTI_FAILED;
	    }
	    else if (HTAA_OK_GATEWAY ==
		     (HTReason = check_authorization(req, YES))) {

		if (!strncmp(HTReqTranslated, "wais:", 5)) {
		    /*
		     * Map'ped WAIS -- another kludge needed.
		     * HTEscape() fails to produce the correct URL for WAIS
		     * because the WAIS url contains ';' and '='
		     * so we need to retranslate the original, *unescaped*
		     * incoming URL to get the correctly escaped mapped URL.
		     */
		    CTRACE(stderr, "WAIS-HACK... retranslating thru rules\n");
		    FREE(HTReqTranslated);
		    StrAllocCopy(HTReqArgPath, HTReqArg);
		    HTTranslateReq(req);
		} else if (!strcmp(untranslated, HTReqTranslated)) {
		    /*
		     * Rule translation didn't change it -- use the orig
		     */
		    StrAllocCopy(HTReqTranslated, HTReqArg);

		    CTRACE(stderr, "Forwarding.. URL untouched\n");
#ifdef OLD_CODE
		    /* As the translated is canonicalized, we don't want to
		       use the original one */
		    char * front = HTParse(HTReqTranslated, "",
				PARSE_ACCESS | PARSE_HOST | PARSE_PUNCTUATION);
		    char * path = HTParse(HTReqTranslated, "",
					  PARSE_PATH | PARSE_PUNCTUATION);
		    char * escaped = HTEscape(path, URL_PATH);
		    free(HTReqTranslated);
		    HTReqTranslated = (char*)malloc(strlen(front) + 2 +
						    strlen(escaped) +
						    (HTReqArgKeywords ?
						     strlen(HTReqArgKeywords) :
						     0));
		    sprintf(HTReqTranslated, "%s%s", front, escaped);
		    free(front);
		    free(path);
		    free(escaped);

		    if (HTReqArgKeywords) {
			strcat(HTReqTranslated, "?");
			strcat(HTReqTranslated, HTReqArgKeywords);
		    }
		    CTRACE(stderr, "Forwarding.. URL `%s\'\n",
			   HTReqTranslated);
#endif /* OLD_CODE */
		} else {
		    /*
		     * Otherwise just escape the Map'ped URL for forwarding
		     * to another server -- remember to append keywords back
		     * to the end of URL.
		     */
		    char * front = HTParse(HTReqTranslated, "",
				PARSE_ACCESS | PARSE_HOST | PARSE_PUNCTUATION);
		    char * path = HTParse(HTReqTranslated, "",
					  PARSE_PATH | PARSE_PUNCTUATION);
		    char * escaped = HTEscape(path, URL_PATH);

		    free(HTReqTranslated);
		    HTReqTranslated = (char*)malloc(strlen(front) + 2 +
						    strlen(escaped) +
						    (HTReqArgKeywords ?
						     strlen(HTReqArgKeywords) :
						     0));
		    sprintf(HTReqTranslated, "%s%s", front, escaped);
		    free(front);
		    free(path);
		    free(escaped);

		    if (HTReqArgKeywords) {
			strcat(HTReqTranslated, "?");
			strcat(HTReqTranslated, HTReqArgKeywords);
		    }
		    CTRACE(stderr,"Gatewaying.. reconstructed URL: %s\n",
			   HTReqTranslated);
		}
	    }
	    else proxy_forbidden = YES;
	} /* not a script request */
    } /* if translation succeeded */

    FREE(untranslated);

  done:
    switch (HTReason) {

      case HTAA_NO_AUTH:
      case HTAA_NOT_MEMBER:
	out.status_code = 401;
	break;

      case HTAA_IP_MASK:
	if (proxy_forbidden)
	    HTReason = HTAA_IP_MASK_PROXY;
	out.status_code = 403;
	break;

      case HTAA_BY_RULE:
      case HTAA_NO_ACL:
      case HTAA_NO_ENTRY:
      case HTAA_SETUP_ERROR:
      case HTAA_DOTDOT:
      case HTAA_HTBIN:
      case HTAA_INVALID_REDIRECT:
      case HTAA_INVALID_USER:
      case HTAA_NOT_ALLOWED:
	out.status_code = 403;
	break;

      case HTAA_NOT_FOUND:
      case HTAA_MULTI_FAILED:
	out.status_code = 404;
	break;

      case HTAA_OK:
      case HTAA_OK_GATEWAY:
	out.status_code = 200;
	break;

      case HTAA_OK_MOVED:
	out.status_code = 301;
	break;

      case HTAA_OK_REDIRECT:
	out.status_code = 302;
	break;

      default:
	out.status_code = 500;
    } /* switch */

    HTAA_statusMessage(req);

    return out.status_code;
}





/* PRIVATE					compose_scheme_specifics()
**		COMPOSE SCHEME-SPECIFIC PARAMETERS
**		TO BE SENT ALONG WITH SERVER REPLY
**		IN THE WWW-Authenticate: FIELD.
** ON ENTRY:
**	scheme		is the authentication scheme for which
**			parameters are asked for.
**	prot		protection setup structure.
**
** ON EXIT:
**	returns		scheme specific parameters in an
**			auto-freed string.
*/
PRIVATE char *compose_scheme_specifics ARGS2(HTAAScheme,	scheme,
					     HTAAProt *,	prot)
{
    static char *result = NULL;

    FREE(result);	/* From previous call */

    switch (scheme) {
      case HTAA_BASIC:
	{
	    char *realm = HTAssocList_lookup(prot->values, "server");
	    result = (char*)malloc(60);
	    sprintf(result, "realm=\"%s\"",
		    (realm ? realm : "UNKNOWN"));
	    return result;
	}
	break;

      case HTAA_PUBKEY:
	{
	    char *realm = HTAssocList_lookup(prot->values, "server");
	    result = (char*)malloc(200);
	    sprintf(result, "realm=\"%s\", key=\"%s\"",
		    (realm ? realm : "UNKNOWN"),
		    "PUBKEY-NOT-IMPLEMENTED");
	    return result;
	}
	break;
      default:
	return NULL;
    }
}


/* SERVER PUBLIC				    HTAA_composeAuthHeaders()
**		COMPOSE WWW-Authenticate: HEADER LINES
**		INDICATING VALID AUTHENTICATION SCHEMES
**		FOR THE REQUESTED DOCUMENT
** ON ENTRY:
**	req	request structure.
**
** ON EXIT:
**	returns	a buffer containing all the WWW-Authenticate:
**		fields including CRLFs (this buffer is auto-freed).
**		NULL, if authentication won't help in accessing
**		the requested document.
**
*/
PUBLIC char *HTAA_composeAuthHeaders ARGS1(HTRequest *, req)
{
    static char *result = NULL;
    HTAAScheme scheme;
    char *scheme_name;
    char *scheme_params;

    if (!req) return NULL;

    if (!HTProt) {
	CTRACE(stderr, "Question..... Document not protected\n%s",
	       " -- why was this function called??\n");
	return NULL;
    }
    else CTRACE(stderr, "AA notice... WWW-Authenticate headers for \"%s\"\n",
		(HTReqTranslated ? HTReqTranslated : "-none-"));

    FREE(result);	/* From previous call */
    result = (char*)malloc(4096);	/* @@ */
    if (!result) outofmem(__FILE__, "HTAA_composeAuthHeaders");
    *result = 0;

    for (scheme=0; scheme < HTAA_MAX_SCHEMES; scheme++) {
	if (-1 < HTList_indexOf(HTProt->valid_schemes, (void*)scheme)) {
	    if ((scheme_name = HTAAScheme_name(scheme))) {
		scheme_params = compose_scheme_specifics(scheme,HTProt);
		strcat(result, "WWW-Authenticate: ");
		strcat(result, scheme_name);
		if (scheme_params) {
		    strcat(result, " ");
		    strcat(result, scheme_params);
		}
		strcat(result, "\r\n");
	    } /* scheme name found */
	    else CTRACE(stderr,
			"INTERNAL.... No name found for scheme number %d\n",
			(int)scheme);
	} /* scheme valid for requested document */
    } /* for every scheme */
    return result;
}

