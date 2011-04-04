
/* MODULE							HTAAProt.c
**		PROTECTION FILE PARSING MODULE
**
** AUTHORS:
**	AL	Ari Luotonen	luotonen@dxcern.cern.ch
**	MD	Mark Donszelmann    duns@vxdeop.cern.ch
**
** HISTORY:
**	20 Oct 93  AL	Now finds uid/gid for nobody/nogroup by name
**			(doesn't use default 65534 right away).
**			Also understands negative uids/gids.
**	14 Nov 93  MD	Added VMS compatibility
**
** BUGS:
**
**
*/

#include <string.h>
#include <time.h>

#ifndef VMS
#include <pwd.h>	/* Unix password file routine: getpwnam()	*/
#include <grp.h>	/* Unix group file routine: getgrnam()		*/
#endif /* not VMS */

#include "HTUtils.h"
#include "HTAAUtil.h"
#include "HTAAFile.h"
#include "HTLex.h"	/* Lexical analysor	*/
#include "HTAssoc.h"	/* Association list	*/
#include "HTAAProt.h"	/* Implemented here	*/
#include "HTDaemon.h"
#include "HTConfig.h"
#include "HTLog.h"
#include "HTSUtils.h"


/*
** Protection setup caching
*/
typedef struct {
    char *	prot_filename;
    HTAAProt *	prot;
} HTAAProtCache;

PRIVATE HTList *  prot_cache	= NULL;	/* Protection setup cache. */
PUBLIC HTAAProt * HTProt = NULL;	/* Current protection setup */
PUBLIC HTAAProt * HTDefProt = NULL;	/* Default protection setup */
PUBLIC HTUidGid * HTIds = NULL;		/* Current uid and gid	    */


PUBLIC HTUidGid * HTUidGid_new ARGS2(char *, u,
				     char *, g)
{
    HTUidGid * ug;

    if (!u && !g) return NULL;

    ug = (HTUidGid*)calloc(1,sizeof(HTUidGid));
    if (!ug) outofmem(__FILE__, "HTUidGid_new");

    if (!g) {
	g = strchr(u,'.');
	if (g) *g++ = 0;
    }

    if (u && *u) {
	if (HTIsNumber(u))
	    ug->uid = atoi(u);
	else
	    StrAllocCopy(ug->uname,u);
    }

    if (g && *g) {
	if (HTIsNumber(g))
	    ug->gid = atoi(g);
	else
	    StrAllocCopy(ug->gname,g);
    }

    CTRACE(stderr,
    "Parsed...... strings %s and %s as uname:%s uid:%d gname:%s gid:%d\n",
	   u ? u : "-null-", g ? g : "-null-",
	   ug->uname ? ug->uname : "-null-", ug->uid,
	   ug->gname ? ug->gname : "-null-", ug->gid);

    return ug;
}


PUBLIC void HTUidGid_free ARGS1(HTUidGid *, ug)
{
    if (ug) {
	if (ug->uname) free(ug->uname);
	if (ug->gname) free(ug->gname);
	free(ug);
    }
}


PUBLIC void HTSetCurrentIds ARGS1(HTUidGid *, ids)
{
    HTIds = ids;
}


#ifdef VMS

/* PUBLIC						HTAA_getUidName()
**		GET THE USER ID NAME (VMS ONLY)
** ON ENTRY:
**	No arguments.
**
** ON EXIT:
**	returns	the user name 
**		Default is "" (nobody).
*/
PUBLIC char * HTAA_getUidName NOARGS
{
    if (HTIds && HTIds->uname && (0 != strcmp(HTIds->uname,"nobody")) )
	return(HTIds->uname);
    else
	return("");
}

#else /* not VMS */



PRIVATE BOOL uid_by_str ARGS2(char *,	str,
			      int *,	uid)
{
    struct passwd *pw = NULL;

    if (!str || !uid) return NO;

    if (HTIsNumber(str))
	pw = getpwuid(atoi(str));
    else /* User name (not number) */
	pw = getpwnam(str);

    if (pw) {
	CTRACE(stderr, "SysInfo..... %s means user (%s:%s:%d:%d:...)\n", str,
	       pw->pw_name, pw->pw_passwd, (int) pw->pw_uid, (int) pw->pw_gid);
	*uid = pw->pw_uid;	
	return YES;
    }
    else {
	HTLog_error2("User not found:", str);
	return NO;
    }
}


PRIVATE BOOL gid_by_str ARGS2(char *,	str,
			      int *,	gid)
{
    struct group *gr = NULL;

    if (!str || !gid) return NO;

    if (HTIsNumber(str))
	gr = getgrgid(atoi(str));
    else /* Group name (not number) */
	gr = getgrnam(str);

    if (gr) {
	CTRACE(stderr, "SysInfo..... %s means group (%s:%s:%d:...)\n",
	       str, gr->gr_name, gr->gr_passwd, (int) gr->gr_gid);
	*gid = gr->gr_gid;
	return YES;
    }
    else {
	HTLog_error2("Group not found:", str);
	return NO;
    }
}



/* PUBLIC							HTAA_getUid()
**		GET THE USER ID TO CHANGE THE PROCESS UID TO
**
** ON EXIT:
**	returns	the uid number to give to setuid() system call.
**		Default is 65534 (nobody).
*/
PUBLIC int HTAA_getUid NOARGS
{
    int uid;

    if (HTIds) {
	if (HTIds->uid > 0)
	    return HTIds->uid;
	if (uid_by_str(HTIds->uname, &uid))
	    return uid;
    }
    if (uid_by_str(sc.user_id, &uid) ||
	uid_by_str("nobody", &uid))
	return uid;

    return 65534;	/* nobody -- will fail */
}


/* PUBLIC							HTAA_getGid()
**		GET THE GROUP ID TO CHANGE THE PROCESS GID TO
**
** ON EXIT:
**	returns	the uid number to give to setgid() system call.
**		Default is 65534 (nogroup).
*/
PUBLIC int HTAA_getGid NOARGS
{
    int gid;

    if (HTIds) {
	if (HTIds->gid > 0)
	    return HTIds->gid;
	if (gid_by_str(HTIds->gname, &gid))
	    return gid;
    }
    if (gid_by_str(sc.group_id, &gid) ||
	gid_by_str("nogroup", &gid) ||
	gid_by_str("nobody", &gid))	/* nogroup is nobody on some systems */
	return gid;

    return 65534;	/* nogroup -- will fail */
}

#endif /* not VMS */




/* PRIVATE						HTAA_parseProtFile()
**		PARSE A PROTECTION SETUP FILE AND
**		PUT THE RESULT IN A HTAAProt STRUCTURE
** ON ENTRY:
**	prot		destination structure.
**	fp		open protection file.
**	end_on_brace	if true we are reading an inlined protection
**			setup directly from the main config file, and
**			we should stop reading when we encounter "}".
** ON EXIT:
**	returns		nothing.
*/
PRIVATE void HTAA_parseProtFile ARGS3(HTAAProt *, prot,
				      FILE *,	  fp,
				      BOOL,	  end_on_brace)
{
    if (prot && fp) {
	LexItem lex_item;
	char *fieldname = NULL;

	while (LEX_EOF != (lex_item = lex(fp))) {

	    while (lex_item == LEX_REC_SEP)	/* Ignore empty lines */
		lex_item = lex(fp);

	    if (lex_item == LEX_EOF)		/* End of file */
		break;

	    if (lex_item == LEX_CLOSE_BRACE && end_on_brace)
		break;		/* End of "inlined" protection setup */

	    if (lex_item == LEX_ALPH_STR) {	/* Valid setup record */
		
		StrAllocCopy(fieldname, lex_buffer);
		
		if (LEX_FIELD_SEP != (lex_item = lex(fp)))
		    unlex(lex_item);	/* If someone wants to use colon */
		                        /* after field name it's ok, but */
		                        /* not required. Here we read it.*/

		if (0==strncasecomp(fieldname, "Auth", 4)) {
		    lex_item = lex(fp);
		    while (lex_item == LEX_ALPH_STR) {
			HTAAScheme scheme = HTAAScheme_enum(lex_buffer);
			if (scheme != HTAA_UNKNOWN) {
			    if (!prot->valid_schemes)
				prot->valid_schemes = HTList_new();
			    HTList_addObject(prot->valid_schemes,(void*)scheme);
			    CTRACE(stderr, "OkScheme.... %s\n",
				   HTAAScheme_name(scheme));
			}
			else CTRACE(stderr, "Unknown..... auth.scheme \"%s\"\n",
				    lex_buffer);
			
			if (LEX_ITEM_SEP != (lex_item = lex(fp)))
			    break;
			/*
			** Here lex_item == LEX_ITEM_SEP; after item separator
			** it is ok to have one or more newlines (LEX_REC_SEP)
			** and they are ignored (continuation line).
			*/
			do {
			    lex_item = lex(fp);
			} while (lex_item == LEX_REC_SEP);
		    } /* while items in list */
		} /* if "Authenticate" */

		else if (0==strncasecomp(fieldname, "acloverride", 7) ||
			 0==strncasecomp(fieldname, "acl-override", 8)) {

		    if (LEX_ALPH_STR == (lex_item = lex(fp))) {
			if (!strcasecomp(lex_buffer,"on") ||
			    !strcasecomp(lex_buffer,"yes") ||
			    !strncasecomp(lex_buffer,"enabled",6))
			    prot->acl_override = YES;
			else
			    prot->acl_override = NO;
		        lex_item = lex(fp);  /* Read record separator */
			CTRACE(stderr,"ACL-OverRide %s\n",
			       prot->acl_override ? "On" : "Off");
		    }
		} /* ACL-OverRide */

		else if (0==strncasecomp(fieldname, "maskgroup", 9) ||
			 0==strncasecomp(fieldname, "mask-group", 10) ||
			 0==strncasecomp(fieldname, "get", 3)) {
		    prot->get_mask = HTAA_parseGroupDef(fp);
		    lex_item=LEX_REC_SEP; /*groupdef parser read this already*/
		    if (TRACE) {
			if (prot->get_mask) {
			    fprintf(stderr, "GET-Mask....\n");
			    HTAA_printGroupDef(prot->get_mask);
			}
			else fprintf(stderr,"SYNTAX ERROR parsing GET-Mask\n");
		    }
		} /* if "Get-Mask" */

		else if (0==strncasecomp(fieldname, "put", 3)) {
		    prot->put_mask = HTAA_parseGroupDef(fp);
		    lex_item=LEX_REC_SEP; /*groupdef parser read this already*/
		    if (TRACE) {
			if (prot->put_mask) {
			    fprintf(stderr, "PUT-Mask....\n");
			    HTAA_printGroupDef(prot->put_mask);
			}
			else fprintf(stderr,"SYNTAX ERROR parsing PUT-Mask\n");
		    }
		} /* if "Put-Mask" */

		else if (0==strncasecomp(fieldname, "post", 4)) {
		    prot->post_mask = HTAA_parseGroupDef(fp);
		    lex_item=LEX_REC_SEP; /*groupdef parser read this already*/
		    if (TRACE) {
			if (prot->post_mask) {
			    fprintf(stderr, "POST-Mask...\n");
			    HTAA_printGroupDef(prot->post_mask);
			}
			else fprintf(stderr,"SYNTAX ERROR parsing POST-Mask\n");
		    }
		} /* if "Post-Mask" */

		else if (0==strncasecomp(fieldname, "delete", 6)) {
		    prot->delete_mask = HTAA_parseGroupDef(fp);
		    lex_item=LEX_REC_SEP; /*groupdef parser read this already*/
		    if (TRACE) {
			if (prot->delete_mask) {
			    fprintf(stderr, "DELETE-Mask.\n");
			    HTAA_printGroupDef(prot->delete_mask);
			}
			else fprintf(stderr, "SYNTAX ERROR parsing DELETE-Mask\n");
		    }
		} /* if "Delete-Mask" */

		else if (0==strncasecomp(fieldname, "mask", 4)) {
		    prot->gen_mask = HTAA_parseGroupDef(fp);
		    lex_item=LEX_REC_SEP; /*groupdef parser read this already*/
		    if (TRACE) {
			if (prot->gen_mask) {
			    fprintf(stderr,
				"General mask (used if no other mask set)\n");
			    HTAA_printGroupDef(prot->gen_mask);
			}
			else fprintf(stderr, "SYNTAX ERROR parsing Mask\n");
		    }
		}

		else {	/* Just a name-value pair, put it to assoclist */

		    if (LEX_ALPH_STR == (lex_item = lex(fp))) {
			if (!prot->values)
			    prot->values = HTAssocList_new();
			HTAssocList_add(prot->values, fieldname, lex_buffer);
		        lex_item = lex(fp);  /* Read record separator */
			CTRACE(stderr, "Binding..... \"%s\" bound to \"%s\"\n",
			       fieldname, lex_buffer);
		    }
		} /* else name-value pair */

		FREE(fieldname);	/* Leak fixed AL 6 Feb 1994 */

	    } /* if valid field */

	    if (lex_item != LEX_EOF  &&  lex_item != LEX_REC_SEP) {
		CTRACE(stderr,
		       "SYNTAX ERROR in prot.setup file at line %d (that line ignored)\n",
		       lex_line);
		do {
		    lex_item = lex(fp);
		} while (lex_item != LEX_EOF && lex_item != LEX_REC_SEP);
	    } /* if syntax error */
	} /* while not end-of-file */
    } /* if valid parameters */
}




/* PRIVATE						HTAAProt_new()
**		ALLOCATE A NEW HTAAProt STRUCTURE AND
**		INITIALIZE IT FROM PROTECTION SETUP FILE
** ON ENTRY:
**	prot_filename	protection setup file name.
**			If NULL, not an error.
**	ids		Uid and gid names or numbers,
**			May be NULL, defaults to nobody.nogroup.
**			Should be NULL, if prot_file is NULL.
**
** ON EXIT:
**	returns		returns a new and initialized protection
**			setup structure.
**			If setup file is already read in (found
**			in cache), only sets user and group ids,
**			and returns that.
*/
PRIVATE HTAAProt *HTAAProt_new ARGS2(CONST char *,	prot_filename,
				     HTUidGid *,	ids)
{
    HTList *cur = prot_cache;
    HTAAProtCache *cache_item = NULL;
    HTAAProt *prot;
    FILE *fp;

    if (!prot_cache)
	prot_cache = HTList_new();
    
    while (NULL != (cache_item = (HTAAProtCache*)HTList_nextObject(cur))) {
	if (!strcmp(cache_item->prot_filename, prot_filename))
	    break;
    }
    if (cache_item) {
	prot = cache_item->prot;
	CTRACE(stderr,
	       "CacheHit... Protection file \"%s\" already loaded\n",
	       prot_filename);
    } else {
	CTRACE(stderr, "Loading..... protection file \"%s\"\n",
	       prot_filename);

	prot = (HTAAProt*)calloc(1, sizeof(HTAAProt));
	if (!prot) outofmem(__FILE__, "HTAAProt_new");

	prot->valid_schemes = HTList_new();
	prot->values	= HTAssocList_new();

	if (prot_filename && NULL != (fp = fopen(prot_filename, "r"))) {
	    HTAA_parseProtFile(prot, fp, NO);
	    fclose(fp);
	    if (!(cache_item = (HTAAProtCache*)malloc(sizeof(HTAAProtCache))))
		outofmem(__FILE__, "HTAAProt_new");
	    cache_item->prot = prot;
	    cache_item->prot_filename = NULL;
	    StrAllocCopy(cache_item->prot_filename, prot_filename);
	    HTList_addObject(prot_cache, (void*)cache_item);
	    /*
	    ** Make "Basic" by default if password file defined
	    */
	    if (HTList_isEmpty(prot->valid_schemes) &&
		HTAssocList_lookup(prot->values, "passw")) {
		HTList_addObject(prot->valid_schemes, (void*)HTAA_BASIC);
		CTRACE(stderr, "Default..... \"Basic\" scheme by default\n");
	    }
	}
	else CTRACE(stderr, "ERROR....... Unable to open prot.setup file \"%s\"\n",
		    (prot_filename ? prot_filename : "-null-"));
    }

    prot->ids = ids;

    return prot;
}


PUBLIC HTAAProt * HTAAProt_parseInlined ARGS1(FILE *, fp)
{
    HTAAProt * prot;

    if (!fp) {
	CTRACE(stderr,"BUG......... HTAAProt_readInlined() param NULL\n");
	return NULL;
    }
    CTRACE(stderr,"Reading..... protection setup directly from config file\n");

    prot = (HTAAProt*)calloc(1, sizeof(HTAAProt));
    if (!prot) outofmem(__FILE__, "HTAAProt_parseInlined");

    prot->valid_schemes = HTList_new();
    prot->values	= HTAssocList_new();

    HTAA_parseProtFile(prot, fp, YES);

    /* Make "Basic" by default if password file defined */
    if (HTList_isEmpty(prot->valid_schemes) &&
	HTAssocList_lookup(prot->values, "passw")) {
	HTList_addObject(prot->valid_schemes, (void*)HTAA_BASIC);
	CTRACE(stderr, "Default..... \"Basic\" scheme by default\n");
    }

    /* Get and set uid and gid */
    {
	char * uid = HTAssocList_lookup(prot->values, "userid");
	char * gid = HTAssocList_lookup(prot->values, "groupid");
	if (uid || gid) {
	    prot->ids = HTUidGid_new(uid,gid);
	    if (TRACE) {
		fprintf(stderr,"UserId...... %s in this protection setup\n",
			uid ? uid : "-default-");
		fprintf(stderr,"GroupId..... %s in this protection setup\n",
			gid ? gid : "-default-");
	    }
	}
    }
    return prot;
}



/* PUBLIC					HTAA_setDefaultProtection()
**		SET THE DEFAULT PROTECTION MODE
**		(called by rule system when a
**		"defprot" rule is matched)
** ON ENTRY:
**	req		request structure.
**	prot_filename	is the protection setup file (second argument
**			for "defprot" rule, optional)
**	ids		contains user and group names separated by
**			a dot, corresponding to the uid
**			gid under which the server should run,
**			default is "nobody.nogroup" (third argument
**			for "defprot" rule, optional; can be given
**			only if protection setup file is also given).
**
** ON EXIT:
**	returns		nothing.
**			Sets default protection mode of the request.
*/
PUBLIC void HTAA_setDefaultProtection ARGS3(HTRequest *,	req,
					    CONST char *,	prot_filename,
					    HTUidGid *,		ids)
{
    if (!req) {
	CTRACE(stderr,
	       "INTERNAL.... req == NULL! [HTAA_setDefaultProtection()]\n");
	return;
    }

    HTDefProt = NULL;	/* Not free()'d because this is in cache */

    if (prot_filename) {
	HTDefProt = HTAAProt_new(prot_filename, ids);
    } else {
	HTLog_error("Protection file not given [obligatory for DefProt rule]");
    }
}


/* PUBLIC					HTAA_setCurrentProtection()
**		SET THE CURRENT PROTECTION MODE
**		(called by rule system when a
**		"protect" rule is matched)
** ON ENTRY:
**	req		request structure.
**	prot_filename	is the protection setup file (second argument
**			for "protect" rule, optional)
**	ids		contains user and group names/ids.
**			Default are nobody and nogroup. (Third argument
**			for "Protect" rule, optional; can be given
**			only if protection setup file is also given).
**
** ON EXIT:
**	returns		nothing.
**			Sets current protection mode of the request.
*/
PUBLIC void HTAA_setCurrentProtection ARGS3(HTRequest *,	req,
					    CONST char *,	prot_filename,
					    HTUidGid *,		ids)
{
    if (!req) {
	CTRACE(stderr,
	       "INTERNAL.... req == NULL! [HTAA_setCurrentProtection()]\n");
	return;
    }

    HTProt = NULL;	/* Not free()'d because this is in cache */

    if (prot_filename) {
	HTProt = HTAAProt_new(prot_filename, ids);
	HTSetCurrentIds(ids);
    } else {
	if (HTDefProt) {
	    HTProt = HTDefProt;
	    if (HTProt->ids) HTSetCurrentIds(HTProt->ids);
	    CTRACE(stderr,"Default..... protection used for Protect rule\n");
	} else {
	    HTLog_error("Protection file not specified for Protect rule and default protection not set");
	}
    }
}

