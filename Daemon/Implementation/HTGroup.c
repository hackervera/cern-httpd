
/* MODULE							HTGroup.c
**		GROUP FILE ROUTINES
**
**	Contains group file parser and routines to match IP
**	address templates and to find out group membership.
**
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
**
** GROUP DEFINITION GRAMMAR:
**
**	string = "sequence of alphanumeric characters"
**	user_name ::= string
**	group_name ::= string
**	group_ref ::= group_name
**	user_def ::= user_name | group_ref
**	user_def_list ::= user_def { ',' user_def }
**	user_part = user_def | '(' user_def_list ')'
**
**	templ = "sequence of alphanumeric characters and '*'s"
**	ip_number_mask ::= templ '.' templ '.' templ '.' templ
**	domain_name_mask ::= templ { '.' templ }
**	address ::= ip_number_mask | domain_name_mask
**	address_def ::= address
**	address_def_list ::= address_def { ',' address_def }
**	address_part = address_def | '(' address_def_list ')'
**
**	item ::= [user_part] ['@' address_part]
**	item_list ::= item { ',' item }
**	group_def ::= item_list
**	group_decl ::= group_name ':' group_def
**
*/



#include <string.h>

#include "HTUtils.h"
#include "HTAAUtil.h"
#include "HTLex.h"	/* Lexical analysor	*/
#include "HTGroup.h"	/* Implemented here	*/
#include "HTAccess.h"
#include "HTWild.h"


/*
** Group file parser
*/

typedef HTList UserDefList;
typedef HTList AddressDefList;

typedef struct {
    UserDefList *	user_def_list;
    AddressDefList *	address_def_list;
} Item;

typedef struct {
    char *	name;
    GroupDef *	translation;
} Ref;

	

PRIVATE void syntax_error ARGS3(FILE *,	 fp,
				char *,	 msg,
				LexItem, lex_item)
{
    char buffer[41];
    int cnt = 0;
    char ch;

    while ((ch = getc(fp)) != EOF  &&  ch != '\n')
	if (cnt < 40) buffer[cnt++] = ch;
    buffer[cnt] = (char)0;

    CTRACE(stderr,
	   "SYNTAX ERROR in setup file at line %d before: \"%s\"\n",
	   lex_line, buffer);
    CTRACE(stderr, "Explanation. Got \"%s\" [HTGroup.c]\n",
	   lex_verbose(lex_item));
    lex_line++;
}


PUBLIC void Ref_delete ARGS1(Ref *, ref)
{
    if (ref) {
	FREE(ref->name);
	GroupDef_delete(ref->translation);
	free(ref);
    }
}

PUBLIC void RefList_delete ARGS1(HTList *, list)
{
    if (list) {
	HTList * cur = list;
	Ref * ref;

	while ((ref = (Ref*)HTList_nextObject(cur)))
	    Ref_delete(ref);
	HTList_delete(list);
    }
}

PUBLIC void UserDefList_delete ARGS1(UserDefList *, list)
{
    RefList_delete(list);
}

PUBLIC void AddressDefList_delete ARGS1(AddressDefList *, list)
{
    RefList_delete(list);
}

PUBLIC void Item_delete ARGS1(Item *, item)
{
    if (item) {
	UserDefList_delete(item->user_def_list);
	AddressDefList_delete(item->address_def_list);
	free(item);
    }
}

PUBLIC void ItemList_delete ARGS1(ItemList *, list)
{
    if (list) {
	HTList * cur = list;
	Item * item;

	while ((item = (Item *)HTList_nextObject(cur)))
	    Item_delete(item);
	HTList_delete(list);
    }
}

PUBLIC void GroupDef_delete ARGS1(GroupDef *, group_def)
{
    if (group_def) {
	FREE(group_def->group_name);
	ItemList_delete(group_def->item_list);	/* Leak fixed AL 6 Feb 1994 */
	free(group_def);
    }
}




PRIVATE AddressDefList *parse_address_part ARGS1(FILE *, fp)
{
    AddressDefList *address_def_list = NULL;
    LexItem lex_item;
    BOOL only_one = NO;

    lex_item = lex(fp);
    if (lex_item == LEX_ALPH_STR || lex_item == LEX_TMPL_STR)
	only_one = YES;
    else if (lex_item != LEX_OPEN_PAREN  ||
	     ((lex_item = lex(fp)) != LEX_ALPH_STR &&
	      lex_item != LEX_TMPL_STR)) {
	syntax_error(fp, "Expecting a single address or '(' beginning list",
		     lex_item);
	return NULL;
    }
    address_def_list = HTList_new();

    for(;;) {
	Ref *ref = (Ref*)malloc(sizeof(Ref));
	ref->name = NULL;
	ref->translation = NULL;
	StrAllocCopy(ref->name, lex_buffer);
	
	HTList_addObject(address_def_list, (void*)ref);

	if (only_one || (lex_item = lex(fp)) != LEX_ITEM_SEP)
	    break;
	/*
	** Here lex_item == LEX_ITEM_SEP; after item separator it
	** is ok to have one or more newlines (LEX_REC_SEP) and
	** they are ignored (continuation line).
	*/
	do {
	    lex_item = lex(fp);
	} while (lex_item == LEX_REC_SEP);

	if (lex_item != LEX_ALPH_STR && lex_item != LEX_TMPL_STR) {
	    syntax_error(fp, "Expecting an address template", lex_item);
	    HTList_delete(address_def_list);
	    return NULL;
	}
    }

    if (!only_one && lex_item != LEX_CLOSE_PAREN) {
	HTList_delete(address_def_list);
	syntax_error(fp, "Expecting ')' closing address list", lex_item);
	return NULL;
    }
    return address_def_list;
}


PRIVATE UserDefList *parse_user_part ARGS1(FILE *, fp)
{
    UserDefList *user_def_list = NULL;
    LexItem lex_item;
    BOOL only_one = NO;

    lex_item = lex(fp);
    if (lex_item == LEX_ALPH_STR)
	only_one = YES;
    else if (lex_item != LEX_OPEN_PAREN  ||
	     (lex_item = lex(fp)) != LEX_ALPH_STR) {
	syntax_error(fp, "Expecting a single name or '(' beginning list",
		     lex_item);
	return NULL;
    }
    user_def_list = HTList_new();

    for (;;) {
	Ref *ref = (Ref*)malloc(sizeof(Ref));
	ref->name = NULL;
	ref->translation = NULL;
	StrAllocCopy(ref->name, lex_buffer);

	HTList_addObject(user_def_list, (void*)ref);
	
	if (only_one || (lex_item = lex(fp)) != LEX_ITEM_SEP)
	    break;
	/*
	** Here lex_item == LEX_ITEM_SEP; after item separator it
	** is ok to have one or more newlines (LEX_REC_SEP) and
	** they are ignored (continuation line).
	*/
	do {
	    lex_item = lex(fp);
	} while (lex_item == LEX_REC_SEP);

	if (lex_item != LEX_ALPH_STR) {
	    UserDefList_delete(user_def_list);	/* Leak fixed AL 6 Feb 1994 */
	    syntax_error(fp, "Expecting user or group name", lex_item);
	    return NULL;
	}
    }

    if (!only_one && lex_item != LEX_CLOSE_PAREN) {
	UserDefList_delete(user_def_list);	/* Leak fixed AL 6 Feb 1994 */
	syntax_error(fp, "Expecting ')' closing user/group list", lex_item);
	return NULL;
    }
    return user_def_list;
}


PRIVATE Item *parse_item ARGS1(FILE *, fp)
{
    Item *item = NULL;
    UserDefList *user_def_list = NULL;
    AddressDefList *address_def_list = NULL;
    LexItem lex_item;

    lex_item = lex(fp);
    if (lex_item == LEX_ALPH_STR || lex_item == LEX_OPEN_PAREN) {
	unlex(lex_item);
	user_def_list = parse_user_part(fp);
	lex_item = lex(fp);
    }

    if (lex_item == LEX_AT_SIGN) {
	lex_item = lex(fp);
	if (lex_item == LEX_ALPH_STR || lex_item == LEX_TMPL_STR ||
	    lex_item == LEX_OPEN_PAREN) {
	    unlex(lex_item);
	    address_def_list = parse_address_part(fp);
	}
	else {
	    UserDefList_delete(user_def_list);	/* Leak fixed AL 6 Feb 1994 */
	    syntax_error(fp, "Expected address part (single address or list)",
			 lex_item);
	    return NULL;
	}
    }
    else unlex(lex_item);

    if (!user_def_list && !address_def_list) {
	syntax_error(fp, "Empty item not allowed", lex_item);
	return NULL;
    }
    item = (Item*)malloc(sizeof(Item));
    item->user_def_list = user_def_list;
    item->address_def_list = address_def_list;
    return item;
}


PRIVATE ItemList *parse_item_list ARGS1(FILE *, fp)
{
    ItemList *item_list = HTList_new();
    Item *item;
    LexItem lex_item;

    for(;;) {
	if (!(item = parse_item(fp))) {
	    HTList_delete(item_list);	/* @@@@ */
	    return NULL;
	}
	HTList_addObject(item_list, (void*)item);
	lex_item = lex(fp);
	if (lex_item != LEX_ITEM_SEP) {
	    unlex(lex_item);
	    return item_list;
	}
	/*
	** Here lex_item == LEX_ITEM_SEP; after item separator it
	** is ok to have one or more newlines (LEX_REC_SEP) and
	** they are ignored (continuation line).
	*/
	do {
	    lex_item = lex(fp);
	} while (lex_item == LEX_REC_SEP);
	unlex(lex_item);
    }
}


PUBLIC GroupDef *HTAA_parseGroupDef ARGS1(FILE *, fp)
{
    ItemList *item_list = NULL;
    GroupDef *group_def = NULL;
    LexItem lex_item;

    if (!(item_list = parse_item_list(fp))) {
	return NULL;
    }
    group_def = (GroupDef*)malloc(sizeof(GroupDef));
    group_def->group_name = NULL;
    group_def->item_list = item_list;

    if ((lex_item = lex(fp)) != LEX_REC_SEP) {
	syntax_error(fp, "Garbage after group definition", lex_item);
    }
    
    return group_def;
}    


PRIVATE GroupDef *parse_group_decl ARGS1(FILE *, fp)
{
    char *group_name = NULL;
    GroupDef *group_def = NULL;
    LexItem lex_item;

    do {
	lex_item = lex(fp);
    } while (lex_item == LEX_REC_SEP);	/* Ignore empty lines */

    if (lex_item != LEX_ALPH_STR) {
	if (lex_item != LEX_EOF)
	    syntax_error(fp, "Expecting group name", lex_item);
	return NULL;
    }
    StrAllocCopy(group_name, lex_buffer);

    if (LEX_FIELD_SEP != (lex_item = lex(fp))) {
	syntax_error(fp, "Expecting field separator", lex_item);
	free(group_name);
	return NULL;
    }

    if (!(group_def = HTAA_parseGroupDef(fp))) {
	free(group_name);
	return NULL;
    }
    group_def->group_name = group_name;

    return group_def;
}
	


/*
** Group manipulation routines
*/

PRIVATE GroupDef *find_group_def ARGS2(GroupDefList *,	group_list,
				       CONST char *,	group_name)
{
    if (group_list && group_name) {
	GroupDefList *cur = group_list;
	GroupDef *group_def;

	while (NULL != (group_def = (GroupDef*)HTList_nextObject(cur))) {
	    if (!strcmp(group_name, group_def->group_name)) {
		return group_def;
            }
        }
    }
    return NULL;
}


PUBLIC void HTAA_resolveGroupReferences ARGS2(GroupDef *,	group_def,
					      GroupDefList *,	group_def_list)
{
    if (group_def && group_def->item_list && group_def_list) {
	ItemList *cur1 = group_def->item_list;
	Item *item;

	while (NULL != (item = (Item*)HTList_nextObject(cur1))) {
	    UserDefList *cur2 = item->user_def_list;
	    Ref *ref;

	    while (NULL != (ref = (Ref*)HTList_nextObject(cur2)))
		ref->translation = find_group_def(group_def_list, ref->name);

	    /* Does NOT translate address_def_list */
	}
    }
}


PRIVATE void add_group_def ARGS2(GroupDefList *, group_def_list,
				 GroupDef *,	 group_def)
{
    HTAA_resolveGroupReferences(group_def, group_def_list);
    HTList_addObject(group_def_list, (void*)group_def);
}


PRIVATE GroupDefList *parse_group_file ARGS1(FILE *, fp)
{
    GroupDefList *group_def_list = HTList_new();
    GroupDef *group_def;
    
    while (NULL != (group_def = parse_group_decl(fp)))
	add_group_def(group_def_list, group_def);
    
    return group_def_list;
}


/*
** Trace functions
*/

PRIVATE void print_item ARGS1(Item *, item)
{
    if (!item)
	fprintf(stderr, "\tNULL-ITEM\n");
    else {
	UserDefList *cur1 = item->user_def_list;
	AddressDefList *cur2 = item->address_def_list;
	Ref *user_ref = (Ref*)HTList_nextObject(cur1);
	Ref *addr_ref = (Ref*)HTList_nextObject(cur2);

	if (user_ref) {
	    fprintf(stderr, "\t[%s%s", user_ref->name,
		    (user_ref->translation ? "*REF*" : ""));
	    while (NULL != (user_ref = (Ref*)HTList_nextObject(cur1)))
		fprintf(stderr, "; %s%s", user_ref->name,
			(user_ref->translation ? "*REF*" : ""));
	    fprintf(stderr, "] ");
	} else fprintf(stderr, "\tANYBODY ");

	if (addr_ref) {
	    fprintf(stderr, "@ [%s", addr_ref->name);
	    while (NULL != (addr_ref = (Ref*)HTList_nextObject(cur2)))
		fprintf(stderr, "; %s", addr_ref->name);
	    fprintf(stderr, "]\n");
	} else fprintf(stderr, "@ ANYADDRESS\n");
    }
}


PRIVATE void print_item_list ARGS1(ItemList *, item_list)
{
    ItemList *cur = item_list;
    Item *item;

    if (!item_list)
	fprintf(stderr, "EMPTY");
    else while (NULL != (item = (Item*)HTList_nextObject(cur)))
	print_item(item);
}


PUBLIC void HTAA_printGroupDef ARGS1(GroupDef *, group_def)
{
    if (!group_def) {
	fprintf(stderr, "\nNULL RECORD\n");
	return;
    }

    fprintf(stderr, "\nGroup %s:\n",
	    (group_def->group_name ? group_def->group_name : "NULL"));

    print_item_list(group_def->item_list);
    fprintf(stderr, "\n");
}


PRIVATE void print_group_def_list ARGS1(GroupDefList *, group_list)
{
    GroupDefList *cur = group_list;
    GroupDef *group_def;
    
    while (NULL != (group_def = (GroupDef*)HTList_nextObject(cur)))
	HTAA_printGroupDef(group_def);
}


/* PRIVATE							HTIpMaskMatch()
**		MATCH AN IP NUMBER MASK OR IP NAME MASK
**		AGAINST ACTUAL IP NUMBER OR IP NAME
**		
** ON ENTRY:
**	pat		Pattern that is matched agains the host name
**			and IP number, can be of either of these forms:
**			e.g.
**				128.141.*.*
**			or
**				*.cern.ch
**
**	ip_number	IP number of connecting host.
**	ip_name		IP name of the connecting host.
**
** ON EXIT:
**	returns		YES, if hostname/internet number
**			matches the mask.
**			NO, if no match (no fire).
*/
PUBLIC BOOL HTIpMaskMatch ARGS3(HTPattern *,	pat,
				CONST char *,	ip_number,
				CONST char *,	ip_name)
{
    char * match = NULL;

    if (ip_name) {
	char * ip_lowcase = (char*)malloc(strlen(ip_name)+1);
	char * p = ip_lowcase;
	if (!p) outofmem(__FILE__, "HTIpMaskMatch");
	strcpy(p,ip_name);
	while (*p) {
	    *p = TOLOWER(*p);
	    p++;
	}
	match = HTPattern_match(pat,NULL,ip_lowcase);
	free(ip_lowcase);
	if (match) {
	    free(match);
	    return YES;
	}
    }
    if (ip_number && (match = HTPattern_match(pat,NULL,ip_number))) {
	free(match);
	return YES;
    }
    return NO;
}


PRIVATE BOOL ip_in_def_list ARGS3(AddressDefList *,	address_def_list,
				  char *,		ip_number,
				  char *,		ip_name)
{
    if (address_def_list && (ip_number || ip_name)) {
	AddressDefList *cur = address_def_list;
	Ref *ref;

	while (NULL != (ref = (Ref*)HTList_nextObject(cur))) {
	    /* Value of ref->translation is ignored, i.e. */
	    /* no recursion for ip address tamplates.	  */
	    HTPattern * pat = HTPattern_new(ref->name);
	    BOOL flag = HTIpMaskMatch(pat, ip_number, ip_name);
	    HTPattern_free(pat);
	    if (flag)
		return YES;
	}
    }
    return NO;
}


/*
** Group file cached reading
*/

typedef struct {
    char *	   group_filename;
    GroupDefList * group_list;
} GroupCache;

typedef HTList GroupCacheList;

PRIVATE GroupCacheList *group_cache_list = NULL;


PUBLIC GroupDefList *HTAA_readGroupFile ARGS1(CONST char *, filename)
{
    FILE *fp;
    GroupCache *group_cache;

    if (!filename || !*filename) return NULL;

    if (!group_cache_list)
	group_cache_list = HTList_new();
    else {
	GroupCacheList *cur = group_cache_list;

	while (NULL != (group_cache = (GroupCache*)HTList_nextObject(cur))) {
	    if (!strcmp(filename, group_cache->group_filename)) {
		CTRACE(stderr,
		       "CacheHit.... Group file \"%s\" already loaded\n",
		       filename);
		return group_cache->group_list;
	    } /* if cache match */
	} /* while cached files remain */
    } /* cache exists */

    CTRACE(stderr, "Reading..... group file \"%s\"\n", filename);

    if (!(fp = fopen(filename, "r"))) {
	CTRACE(stderr,
	       "ERROR....... Unable to open group file \"%s\"\n",
	       filename);
	return NULL;
    }

    if (!(group_cache = (GroupCache*)malloc(sizeof(GroupCache))))
	outofmem(__FILE__, "HTAA_readGroupFile");

    group_cache->group_filename = NULL;
    StrAllocCopy(group_cache->group_filename, filename);
    group_cache->group_list = parse_group_file(fp);
    HTList_addObject(group_cache_list, (void*)group_cache);
    fclose(fp);

    if (TRACE) {
	fprintf(stderr, "GroupFile... %s defines:\n", filename);
	print_group_def_list(group_cache->group_list);
	fprintf(stderr, "............ End of group file dump\n");
    }

    return group_cache->group_list;
}


/* PUBLIC					HTAA_userAndInetInGroup()
**		CHECK IF USER BELONGS TO TO A GIVEN GROUP
**		AND THAT THE CONNECTION COMES FROM AN
**		ADDRESS THAT IS ALLOWED BY THAT GROUP
** ON ENTRY:
**	group		the group definition structure.
**	username	connecting user (NULL if anonymous).
**	ip_number	browser host IP number, optional.
**	ip_name		browser host IP name, optional.
**			However, one of ip_number or ip_name
**			must be given.
** ON EXIT:
**	returns		HTAA_IP_MASK, if IP address mask was
**			reason for failing.
**			HTAA_NOT_MEMBER, if user does not belong
**			to the group.
**			HTAA_OK if both IP address and user are ok.
*/
PUBLIC HTAAFailReason HTAA_userAndInetInGroup ARGS4(GroupDef *,	group,
						    char *,	username,
						    char *,	ip_number,
						    char *,	ip_name)
{
    HTAAFailReason reason = HTAA_NOT_MEMBER;
    static BOOL poss;
    poss = NO;
    if (group) {
	ItemList *cur1 = group->item_list;
	Item *item;

	while (NULL != (item = (Item*)HTList_nextObject(cur1))) {
	    if (!item->address_def_list ||	/* Any address allowed */
		ip_in_def_list(item->address_def_list, ip_number, ip_name)) {
		if (!item->user_def_list)	/* Any user allowed */
		    return HTAA_OK;
		else {
		    UserDefList *cur2 = item->user_def_list;
		    Ref *ref;

		    /*
		    ** Possible to be an authorized user from this address
		    */
		    poss = YES;

		    while (NULL != (ref = (Ref*)HTList_nextObject(cur2))) {
			
			if (ref->translation) {	/* Group, check recursively */
			    reason = HTAA_userAndInetInGroup(ref->translation,
							     username,
							     ip_number,ip_name);

			    if (reason == HTAA_OK)
				return HTAA_OK;
			}
			else {	/* Username, check directly */
			    if ((username && *username &&
				 (0==strcmp(ref->name, username) ||
				  0==strcasecomp(ref->name, "Users") ||
				  0==strcasecomp(ref->name, "All") ||
				  0==strcasecomp(ref->name, "*"))) ||
				(0==strcasecomp(ref->name, "Anybody") ||
				 0==strcasecomp(ref->name, "Anyone") ||
				 0==strcasecomp(ref->name, "Anonymous")))
				return HTAA_OK;
			}
		    } /* Every user/group name in this group */
		} /* search for username */
	    } /* IP address ok */
	    else {
		if (!poss) reason = HTAA_IP_MASK;
	    }
	} /* while items in group */
    } /* valid parameters */

    /*
    ** No match, or invalid parameters
    */
    if (poss) return HTAA_NOT_MEMBER;
    else return reason;
}


