
/*								HTWild.h
 *	WILDCARD MATCHING MODULE
 *	* * * * * * * * * * * * *
 *
 * Author:
 *	Ari Luotonen, CERN, April 1994, <luotonen@dxcern.cern.ch>
 *
 * IMPORTANT:
 *	Throughout this module we have to check HTRuleCaseSense to find
 *	out if matching should be made in a case-sensitive manner.
 *	In Unix it's always case-sensitive, but in VMS it usually
 *	(but not always) has to be case-insensitive (for security,
 *	e.g. Protect rules might not be matched if someone is fooling
 *	around with upper-case characters when the pattern is
 *	lower-case).
 */

#include "HTWild.h"
#include "HTParse.h"
#include "tcp.h"

extern BOOL HTRuleCaseSense;

typedef struct _HTWildMatch {
    char *			text;
    struct _HTWildMatch *	next;
} HTWildMatch;


PRIVATE HTPattern * pat_new ARGS3(BOOL,		wild,
				  CONST char *,	text,
				  HTPattern *,	prev)
{
    HTPattern * p = (HTPattern*)calloc(1,sizeof(HTPattern));
    if (!p) outofmem(__FILE__, "HTPattern_new");

    p->wild = wild;
    if (text)
	StrAllocCopy(p->text,text);
    if (prev) prev->next = p;
    return p;
}


PUBLIC void HTPattern_free ARGS1(HTPattern *, p)
{
    HTPattern * killme;

    while (p) {
	if (p->text) free(p->text);
	killme = p;
	p = p->next;
	free(killme);
    }
}


PUBLIC HTPattern * HTPattern_new ARGS1(CONST char *, str)
{
    int len;
    CONST char * cur = str;
    char * buf = NULL;
    char * dest = NULL;
    HTPattern * head = NULL;
    HTPattern * tail = NULL;
    BOOL wild = NO;

    if (!str) return NULL;

    len = strlen(str);
    dest = buf = (char*)calloc(1,len+1);
    if (!buf) outofmem(__FILE__, "HTPattern_new");

    while (*cur) {
	if (*cur == '\\') {
	    cur++;
	    if (*cur)
		*dest = *cur;
	    else {
		*dest++ = '\n';
		break;
	    }
	}
	else if (*cur == '*') {
	    *dest = 0;
	    if (*buf) {
		tail = pat_new(wild,buf,tail);
		if (!head) head = tail;
	    }
	    dest = buf;
	    cur++;
	    wild = YES;
	    continue;
	}
	else {
	    *dest = *cur;
	}
	dest++;
	cur++;
    }
    *dest = 0;
    if (wild || *buf) {
	tail = pat_new(wild,buf,tail);
	if (!head) head = tail;
    }
    free(buf);
    return head;
}


PRIVATE void HTWildMatch_free ARGS1(HTWildMatch *, w)
{
    HTWildMatch * killme;

    while (w) {
	if (w->text) free(w->text);
	killme = w;
	w = w->next;
	free(killme);
    }
}


PRIVATE char * replace ARGS2(HTPattern *,	eqv,
			     HTWildMatch *,	matches)
{
    int len = 1;
    char * ret = NULL;
    char * cur = NULL;
    HTPattern * t = eqv;
    HTWildMatch * w = matches;

    while (t) {
	if (t->wild && w) {
	    if (w->text) len += strlen(w->text);
	    w = w->next;
	}
	if (t->text) len += strlen(t->text);
	t = t->next;
    }

    cur = ret = (char*)calloc(1,len);
    if (!ret) outofmem(__FILE__, "replace");

    t = eqv;
    w = matches;

    while (t) {
	if (t->wild && w) {
	    if (w->text) {
		strcpy(cur,w->text);
		cur += strlen(w->text);
	    }
	    w = w->next;
	}
	if (t->text) {
	    strcpy(cur,t->text);
	    cur += strlen(t->text);
	}
	t = t->next;
    }

    return ret;
}


PRIVATE HTWildMatch * HTWildMatch_new ARGS1(CONST char *, text)
{
    HTWildMatch * w = (HTWildMatch*)calloc(1,sizeof(HTWildMatch));
    if (!w) outofmem(__FILE__, "match");

    if (text) StrAllocCopy(w->text,text);
    return w;
}


PRIVATE BOOL is_user_dir ARGS1(CONST char *, str)
{
    if (!strncmp(str,"/~",2) || !strncmp(str,"file:/~",7))
	return YES;
    else
	return NO;
}


PRIVATE HTWildMatch * match ARGS2(HTPattern *,	pat,
				  CONST char *,	act)
{
    BOOL udir;

    if (!pat || !act) {
	if ((!pat && act && *act)  ||  (pat && !act))
	    return NULL;
	else
	    return HTWildMatch_new(NULL);
    }

    udir = is_user_dir(act);

    if (!pat->wild) {		/* No wildcard before string pattern */
	if (pat->text) {	/* String pattern given, just compare it */
	    int len = strlen(pat->text);
	    if (((HTRuleCaseSense && strncmp(pat->text,act,len) != 0) ||
		 (!HTRuleCaseSense && strncasecomp(pat->text,act,len) != 0))
		||  /* No match or not     */
		(udir && !is_user_dir(pat->text)))  /* explicitly user dir */
		return NULL;	/* No match */
	    else				 /* Strings match...	  */
		return match(pat->next,act+len); /* ...but does the rest? */
	}
	else {	/* No wild nor text */
	    if (!*act)
		return HTWildMatch_new(NULL);	/* String ended -- ok */
	    else
		return NULL;			/* No match */
	}
    }
    else {			/* Wildcard before string pattern */
	if ((!pat->text || !*pat->text) && !udir)  /* No string required... */
	    return HTWildMatch_new(act);	   /* ...anything matches.  */
	else if (udir && !is_user_dir(pat->text))
	    return NULL;		/* User dir not explicitly matched */
	else {
	    int len = strlen(pat->text);
	    char * cur = HTRuleCaseSense
		? strstr((char*)act,pat->text)
		: strcasestr((char*)act,pat->text);

	    while (cur) {
		HTWildMatch * m = match(pat->next,cur+len);
		HTWildMatch * w = NULL;

		if (m) {
		    char saved = *cur;
		    *cur = 0;
		    w = HTWildMatch_new(act);
		    *cur = saved;
		    w->next = m;
		    return w;
		}
		if (HTRuleCaseSense)
		    cur = strstr(cur+len,pat->text);
		else
		    cur = strcasestr(cur+len,pat->text);
	    }
	    return NULL;	/* Required string not found */
	}
    }

    /* Never reached, but stupid compilers warn without this: */
    return NULL;
}


void print_pat ARGS1(HTPattern *, p)
{
    fprintf(stderr, "\nGot pattern:\n");
    while (p) {
	if (p->wild)
	    fprintf(stderr, "  *");
	if (p->text)
	    fprintf(stderr, "  \"%s\"", p->text);
	p = p->next;
    }
    fprintf(stderr, "\n\n");
}


PUBLIC char * HTPattern_match ARGS3(HTPattern *,	pat,
				    HTPattern *,	eqv,
				    CONST char *,	act)
{
    HTWildMatch * matches = NULL;
    char * result = NULL;

    if (!pat || !act) return NULL;

    matches = match(pat,act);
    if (!matches) return NULL;

    if (eqv)
	result = replace(eqv,matches);
    else
	StrAllocCopy(result,act);

    HTWildMatch_free(matches);

    return result;
}


PUBLIC BOOL HTPattern_url_match ARGS2(HTPattern *,	pat,
				      CONST char *,	url)
{
    char * match;
    char * decoded = NULL;

    StrAllocCopy(decoded,url);
    HTUnEscape(decoded);

    match = HTPattern_match(pat,NULL,decoded);

    free(decoded);
    if (match) {
	free(match);
	return YES;
    }
    return NO;
}


PUBLIC char * HTPattern_firstWild ARGS2(HTPattern *,	pat,
					char *,		act)
{
    if (!pat || !act) return NULL;
    else if (pat->wild) return act;
    else if (pat->text && pat->next) return act + strlen(pat->text);
    else return NULL;
}


#ifdef TEST_MAIN

PUBLIC int main ARGS2(int,	argc,
		      char **,	argv)
{
    HTPattern * pat = NULL;
    HTPattern * eqv = NULL;
    char * act = NULL;
    char * res = NULL;

    if (argc < 3 || argc > 4) {
	fprintf(stderr,
		"\nMATCH TESTER: Usage:\n\t%s template [equiv] actual\n\n",
		argv[0]);
	exit(1);
    }

    pat = HTPattern_new(argv[1]);
    print_pat(pat);
    if (argc == 4) {
	eqv = HTPattern_new(argv[2]);
	print_pat(eqv);
	act = argv[3];
    }
    else {
	act = argv[2];
    }


    res = HTPattern_match(pat,eqv,act);
    if (res)
	printf("%s\n",res);
    else
	printf("No match.\n");

    HTPattern_free(pat);
    if (eqv) HTPattern_free(eqv);
}
#endif /* TEST_MAIN */

