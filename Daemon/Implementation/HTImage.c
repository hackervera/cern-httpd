
/* PROGRAM							HTImage.c
**		PROGRAM FOR HANDLING REQUESTS FROM
**		ISMAP IMAGES FOR CERN SERVER.
**
** ENVIRONMENT:
**	PATH_TRANSLATED	physical (translated) config file.
**	PATH_INFO	contains URL path to image config file.
**
**	The actual image config file is searched in the
**	following order:
**
**		IMAGE_DIR/PATH_INFO
**		IMAGE_DIR/PATH_TRANSLATED
**		PATH_TRANSLATED
**		PATH_INFO
**
**	QUERY_STRING	contains coordinates as x,y
**			Origin is in upper lefthand corner.
**			Or: coordinates are given as x and y
**			form field values: x=x&y=y
**
** AUTHORS:
**	AL	Ari Luotonen	luotonen@dxcern.cern.ch
**
** HISTORY:
**	 8 Nov 93  AL	Written on a crisp Monday afternoon from scratch.
**      18 Jan 94  AL	Ported to CGI/1.0.
**
** BUGS:
**
*/

#include <stdio.h>

#include "HTUtils.h"
#include "HTString.h"
#include "HTAlert.h"
#include "HTAAUtil.h"

/*
** Define IMAGE_DIR to the image mask directory under which
** you put all your image config files, if you don't want them
** to be directly translated by the rule file.
*/
#ifndef IMAGE_DIR
#define IMAGE_DIR NULL
#endif

#define MAX_STR_LEN 256


PRIVATE char * path_info = NULL;
PRIVATE char * path_translated = NULL;
PRIVATE char * query_string = NULL;


/*
** Types used to describe graphical areas.
*/
typedef struct {
    int	x;
    int	y;
} HTCoord;

typedef struct {
    HTCoord	corner1;
    HTCoord	corner2;
} HTRect;

typedef struct {
    HTCoord	center;
    int		radius;
} HTCirc;

typedef struct {
    HTList *	coords;
} HTPoly;

typedef union {
    HTRect	rect;
    HTCirc	circ;
    HTPoly	poly;
} HTShape;

typedef enum {
    HTRECT, HTCIRC, HTPOLY
} HTShapeType;

typedef struct {
    char *	url;
    HTShapeType	type;
    HTShape	shape;
} HTMapping;

typedef struct {
    char *	default_url;
    HTList *	mappings;
} HTPict;


#ifdef VMS
#include "HTStyle.h"
PUBLIC char *HTAppName = "HTImage";
PUBLIC char *HTAppVersion = "unknown";
PUBLIC HTStyleSheet *styleSheet;
#endif /* VMS */


/* PRIVATE							fatal()
**		WRITE OUT AN ERROR MESSAGE AND EXIT
** ON ENTRY:
**	msg	error message to print.
**
** ON EXIT:
**	Program is dead.
*/
PRIVATE void fatal ARGS1(char *, msg)
{
    printf("Content-Type: text/html\r\n\r\n");
    printf("<HEAD><TITLE>HTImage error</TITLE></HEAD>\n<BODY>\n");
    printf("<H1>Error</H1>\nError calling HTImage:<P>\n%s<P>\n</BODY>\n", msg);
    exit(0);    
}


PRIVATE void bad_query_string NOARGS
{
    char *msg = malloc(strlen(query_string) + 100);

    sprintf(msg, "Invalid QUERY_STRING: '%s' expecting either x,y or x=x&y=y",
	    query_string);
    fatal(msg);
}


/*
** ******************************************************************
**	LEXICAL ANALYSOR
** ******************************************************************
*/
typedef enum {
    LEX_NONE,		/* Internally used	*/
    LEX_EOF,		/* End of file		*/
    LEX_COMMA,		/* Comma		*/
    LEX_OPEN_PAREN,	/* Opening parenthesis	*/
    LEX_CLOSE_PAREN,	/* Closing parenthesis	*/
    LEX_STR,		/* Alphanumeric string	*/
    LEX_INT		/* Integer value	*/
} LexItem;

PRIVATE char lex_str[MAX_STR_LEN+1];	/* Read lexical string		*/
PRIVATE int lex_int;			/* Read integer value		*/
PRIVATE int lex_line = 1;		/* Line number in source file	*/
PRIVATE int lex_cnt;
PRIVATE LexItem lex_pushed_back = LEX_NONE;


/* PRIVATE							isNumber()
**		DOES A CHARACTER STRING REPRESENT A NUMBER
*/
PRIVATE BOOL isNumber ARGS1(CONST char *, s)
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


PRIVATE void unlex ARGS1(LexItem, lex_item)
{
    lex_pushed_back = lex_item;
}



PRIVATE LexItem lex ARGS1(FILE *, fp)
{
    int ch;

    if (lex_pushed_back != LEX_NONE) {
	LexItem ret = lex_pushed_back;
	lex_pushed_back = LEX_NONE;
	return ret;
    }

    lex_cnt = 0;

    for(;;) {
	switch (ch = getc(fp)) {
	  case EOF:
	  case ' ':
	  case '\t':
	  case '\r':
	  case '\n':
	  case '(':
	  case ')':
	  case ',':
	    if (lex_cnt > 0) {
		if (ch != EOF)
		    ungetc(ch,fp);
		if (isNumber(lex_str)) {
		    lex_int = atoi(lex_str);
		    return LEX_INT;
		}
		else return LEX_STR;
	    }
	    else switch(ch) {
	      case EOF:		return LEX_EOF;		break;
	      case '\n':	lex_line++;		break;
	      case ',':		return LEX_COMMA;	break;
	      case '(':		return LEX_OPEN_PAREN;	break;
	      case ')':		return LEX_CLOSE_PAREN;	break;
	      default:	;	/* Leading white space ignored (SP,TAB,CR) */
	    }
	    break;

	  default:
	    lex_str[lex_cnt++] = ch;
	    lex_str[lex_cnt] = (char)0;
	} /* switch ch */
    } /* forever */
}


/*
** ******************************************************************
**	PICTURE CONFIGURATION FILE PARSER
** ******************************************************************
*/

PRIVATE void syntax_error ARGS3(FILE *,	 fp,
				char *,	 message,
				LexItem, lex_item)
{
    char buffer[41];
    int cnt = 0;
    char ch;
    char msg[256];
    char *item_name;

    switch (lex_item) {
      case LEX_NONE:		item_name = "*InternalFlag*";		break;
      case LEX_EOF:		item_name = "end of file";		break;
      case LEX_COMMA:		item_name = "a comma";			break;
      case LEX_OPEN_PAREN:	item_name = "opening parenthesis";	break;
      case LEX_CLOSE_PAREN:	item_name = "closing parenthesis";	break;
      case LEX_STR:		item_name = "an alphanumeric string";	break;
      case LEX_INT:		item_name = "an integer value";		break;
      default:			item_name = "*Bug*";
    }

    while ((ch = getc(fp)) != EOF  &&  ch != '\n')
	if (cnt < 40) buffer[cnt++] = ch;
    buffer[cnt] = (char)0;

    sprintf(msg, "%s %d\n%s (got %s)\n",
	    "HTImage.c: Syntax error at line", lex_line,
	    message, item_name);
    lex_line++;
    fatal(msg);
}


PRIVATE void rev_list ARGS1(HTList *, list)
{
    if (list) {
	HTList *tmp = HTList_new();
	HTList *cur = list;
	HTList *old = list->next;
	void *object;

	while (NULL != (object = HTList_nextObject(cur)))
	    HTList_addObject(tmp, object);

	list->next = tmp->next;
	tmp->next = old;
	HTList_delete(tmp);
    }
}


PRIVATE HTCoord *parse_coord ARGS1(FILE *, fp)
{
    LexItem lex_item;
    HTCoord *coord;
    int x;

    if (!fp) return NULL;

    if (LEX_OPEN_PAREN != (lex_item = lex(fp))) {
	unlex(lex_item);
	return NULL;
    }
    else {
	if (LEX_INT != (lex_item = lex(fp))) {
	    syntax_error(fp, "expecting x coordinate", lex_item);
	    return NULL;
	}
	else {
	    x = lex_int;
	    if (LEX_COMMA != (lex_item = lex(fp))) {
		syntax_error(fp,"expecting comma separating x and y",lex_item);
		return NULL;
	    }
	    else if (LEX_INT != (lex_item = lex(fp))) {
		syntax_error(fp, "expecting y coordinate", lex_item);
		return NULL;
	    }
	    else {
		if (!(coord = (HTCoord*)malloc(sizeof(HTCoord))))
		    outofmem(__FILE__, "parse_coord");
		coord->x = x;
		coord->y = lex_int;

		if (LEX_CLOSE_PAREN != (lex_item = lex(fp)))
		    syntax_error(fp,"expecting closing parenthesis",lex_item);

		return coord;
	    }
	}
    }
}


PRIVATE HTMapping *parse_rectangle ARGS1(FILE *, fp)
{
    HTMapping *mapping = NULL;
    HTCoord *coord = NULL;
    LexItem lex_item;

    if (!fp) return NULL;
    if (!(mapping = (HTMapping*)malloc(sizeof(HTMapping))))
	outofmem(__FILE__, "parse_rectangle");
    mapping->url = NULL;
    mapping->type = HTRECT;
    mapping->shape.rect.corner1.x = 0;
    mapping->shape.rect.corner1.y = 0;
    mapping->shape.rect.corner2.x = 0;
    mapping->shape.rect.corner2.y = 0;

    if (!(coord = parse_coord(fp))) {
	lex_item = lex(fp);
	syntax_error(fp, "expecting first coordinate pair", lex_item);
    }
    else {
	mapping->shape.rect.corner1.x = coord->x;
	mapping->shape.rect.corner1.y = coord->y;
	free(coord);
	if (!(coord = parse_coord(fp))) {
	    lex_item = lex(fp);
	    syntax_error(fp, "expecting second coordinate pair", lex_item);
	}
	else {
	    mapping->shape.rect.corner2.x = coord->x;
	    mapping->shape.rect.corner2.y = coord->y;
	    free(coord);
	    if (LEX_STR != (lex_item = lex(fp)))
		syntax_error(fp, "expecting URL", lex_item);
	    else
		StrAllocCopy(mapping->url, lex_str);
	}
    }
    return mapping;
}


PRIVATE HTMapping *parse_circle ARGS1(FILE *, fp)
{
    HTMapping *mapping = NULL;
    HTCoord *coord = NULL;
    LexItem lex_item;

    if (!fp) return NULL;
    if (!(mapping = (HTMapping*)malloc(sizeof(HTMapping))))
	outofmem(__FILE__, "parse_circle");
    mapping->url = NULL;
    mapping->type = HTCIRC;
    mapping->shape.circ.center.x = 0;
    mapping->shape.circ.center.y = 0;
    mapping->shape.circ.radius = 0;

    if (!(coord = parse_coord(fp))) {
	lex_item = lex(fp);
	syntax_error(fp, "expecting coordinate pair", lex_item);
    }
    else {
	mapping->shape.circ.center.x = coord->x;
	mapping->shape.circ.center.y = coord->y;
	free(coord);
	if (LEX_INT != (lex_item = lex(fp)))
	    syntax_error(fp, "expecting radius", lex_item);
	else {
	    mapping->shape.circ.radius = lex_int;
	    if (LEX_STR != (lex_item = lex(fp)))
		syntax_error(fp, "expecting URL", lex_item);
	    else
		StrAllocCopy(mapping->url, lex_str);
	}
    }
    return mapping;
}


PRIVATE HTMapping *parse_polygon ARGS1(FILE *, fp)
{
    LexItem lex_item;
    HTMapping *mapping = NULL;
    HTCoord *first = NULL;
    HTCoord *prev = NULL;
    HTCoord *coord = NULL;

    if (!fp) return NULL;
    if (!(mapping = (HTMapping*)malloc(sizeof(HTMapping))))
	outofmem(__FILE__, "parse_polygon");
    mapping->url = NULL;
    mapping->type = HTPOLY;
    mapping->shape.poly.coords = HTList_new();

    coord = first = parse_coord(fp);
    while (coord) {
	HTList_addObject(mapping->shape.poly.coords, (void*)coord);
	prev = coord;
	coord = NULL;
	do {	/* Ignore if same coordinate pair appears multiply */
	    if (coord) free(coord);
	    coord = parse_coord(fp);
	} while (coord  &&  coord->x == prev->x  &&  coord->y == prev->y);
    }
    /*
    ** Add the first coordinate pair also to be the last so
    ** that we have a closed path (if the user has already
    ** given a closed path then do nothing more.
    */
    if (prev && (first->x != prev->x  ||  first->y != prev->y))
	HTList_addObject(mapping->shape.poly.coords, (void*)first);

    if (LEX_STR != (lex_item = lex(fp)))
	syntax_error(fp, "expecting URL", lex_item);
    else
	StrAllocCopy(mapping->url, lex_str);

    return mapping;
}


PRIVATE HTPict *parse_picture ARGS1(FILE *, fp)
{
    LexItem lex_item;
    HTPict *picture;

    if (!fp) return NULL;
    if (!(picture = (HTPict*)malloc(sizeof(HTPict))))
	outofmem(__FILE__, "parse_picture");
    picture->default_url = NULL;
    picture->mappings = HTList_new();

    while (LEX_EOF != (lex_item = lex(fp))) {
	if (lex_item == LEX_STR) {
	    if (0==strncasecomp(lex_str, "default", 3)) {
		if (LEX_STR != (lex_item = lex(fp)))
		    syntax_error(fp, "expecting default URL", lex_item);
		else
		    StrAllocCopy(picture->default_url, lex_str);
	    }
	    else if (0==strncasecomp(lex_str, "rectangle", 4))
		HTList_addObject(picture->mappings, parse_rectangle(fp));
	    else if (0==strncasecomp(lex_str, "circle", 4))
		HTList_addObject(picture->mappings, parse_circle(fp));
	    else if (0==strncasecomp(lex_str, "polygon", 4))
		HTList_addObject(picture->mappings, parse_polygon(fp));
	    else {
		char msg[100];
		sprintf(msg, "Bad field name, expecting %s",
			"'default', 'rectangle', 'circle' or 'polygon'");
		syntax_error(fp, msg, lex_item);
	    }
	}
	else
	    syntax_error(fp, "expecting field name", lex_item);
    }

    rev_list(picture->mappings);	/* Because HTList_addObject()	*/
                                        /* adds to the start.		*/
    return picture;
}



/*
** ******************************************************************
**	ROUTINES FOR POINT INSIDE/OUTSIDE RESOLUTION
** ******************************************************************
*/

#define SQR(x)   ((x) * (x))

PRIVATE BOOL inside_rect ARGS3(HTRect *,	rect,
			       int,		x,
			       int,		y)
{
    if (rect &&
	HTMIN(rect->corner1.x, rect->corner2.x) <= x &&
	HTMAX(rect->corner1.x, rect->corner2.x) >= x &&
	HTMIN(rect->corner1.y, rect->corner2.y) <= y &&
	HTMAX(rect->corner1.y, rect->corner2.y) >= y)
	return YES;
    else
	return NO;
}


PRIVATE BOOL inside_circ ARGS3(HTCirc *,	circ,
			       int,		x,
			       int,		y)
{
    if (circ &&
	SQR(circ->center.x - x) + SQR(circ->center.y - y) <= SQR(circ->radius))
	return YES;
    else
	return NO;
}


PRIVATE BOOL inside_poly ARGS3(HTPoly *,	poly,
			       int,		x,
			       int,		y)
{
    if (!poly || !poly->coords)
	return NO;
    else {
	HTList *cur = poly->coords;
	HTCoord *a = (HTCoord*)HTList_nextObject(cur);
	HTCoord *b = (HTCoord*)HTList_nextObject(cur);
	HTCoord *first = a;
	HTCoord *check_previous = NULL;
	double cross_x;
	int crossings = 0;

	while (a && b) {

	    if (x == a->x  &&  y == a->y) {
		/* Point is a vertex, so it's inside */
		return YES;
	    }
	    else if (a->y == b->y) {	/* Horizontal line */
		if (y == a->y &&
		    ((a->x <= x  &&  x <= b->x) ||
		     (a->x >= x  &&  x >= b->x))) {
		    /* Line horizontal and point is on that line */
		    return YES;
		}
	    }
	    else if (check_previous) {
		if ((check_previous->y < a->y  &&  a->y < b->y)  ||
		    (check_previous->y > a->y  &&  a->y > b->y)) {
		    /* Lines from vertex go to different sides -- crossing */
		    crossings++;
		}
		/* Else lines from vertex go to same side -- no crossing */
		check_previous = NULL;
	    }
	    else if (y >= HTMIN(a->y, b->y) &&
		     y <= HTMAX(a->y, b->y) &&
		     x <= HTMAX(a->x, b->x)) {	/* If crossing is possible */

		if (a->x == b->x) {		/* Vertical line */
		    if (a->x == x) {
			/* Point is on an edge -- inside */
			return YES;
		    }
		    else if (a->x > x) {
			crossings++;
		    }
		}
		else {
		    /* At this point we know that lines cross -- the	*/
		    /* question is which side of the point they cross.	*/
		    cross_x = (float)a->x +
			(float)((y - a->y) * (b->x - a->x)) /
			    (float)(b->y - a->y);

		    if (cross_x == b->x) {
			/* Crosses exactly at the ending vertex	*/
			/* -- handled on the next round.	*/
			check_previous = a;
		    }
		    else if (cross_x == x) {
			/* Point is on an edge -- inside */
			return YES;
		    }
		    else if (cross_x > x) {
			crossings++;
		    }
		}
	    }
	    /* Else crossing is not possible */

	    a = b;
	    b = (HTCoord*)HTList_nextObject(cur);
	}

	if (check_previous) {
	    if ((check_previous->y < a->y  &&  a->y < first->y)  ||
		(check_previous->y > a->y  &&  a->y > first->y)) {
		/* Lines from vertex go to different sides -- crossing */
		crossings++;
	    }
	}

	if (crossings % 2 == 1)
	    return YES;
	else return NO;
    }
}


PRIVATE char *get_url ARGS3(HTPict *,	pict,
			    int,	x,
			    int,	y)
{
    if (!pict)
	return NULL;
    else {
	HTList *cur = pict->mappings;
	HTMapping *mapping;

	while (NULL != (mapping = (HTMapping*)HTList_nextObject(cur))) {
	    if ((mapping->type == HTRECT &&
		 inside_rect(&mapping->shape.rect,x,y)) ||
		(mapping->type == HTCIRC &&
		 inside_circ(&mapping->shape.circ,x,y)) ||
		(mapping->type == HTPOLY &&
		 inside_poly(&mapping->shape.poly,x,y)))
		return mapping->url;
	}
	return pict->default_url;
    }
}



/* MAIN PROGRAM FOR htimage					main()
**
*/
PUBLIC int main ARGS2(int,	argc,
		      char **,	argv)
{
    char *config_file1 = NULL;
    char *config_file2 = NULL;
    char *coords = NULL;
    char *p;
    int x = -1;
    int y = -1;
    HTPict *pict = NULL;
    FILE *fp = NULL;
    char *url;

#ifdef VMS
    query_string = getenv("WWW_QUERY_STRING");
    path_info = getenv("WWW_PATH_INFO");
    path_translated = getenv("WWW_PATH_TRANSLATED");
#else /* non VMS */
    query_string = getenv("QUERY_STRING");
    path_info = getenv("PATH_INFO");
    path_translated = getenv("PATH_TRANSLATED");
#endif /* non VMS */

    if (!path_info && !path_translated)
	fatal("Neither PATH_INFO nor PATH_TRANSLATED env.variable not set");
    if (!query_string)
	fatal("QUERY_STRING environment variable not set");

    StrAllocCopy(coords, query_string);
    if (strchr(coords, '=')) {
	char *cur;
	for(cur=coords; cur; ) {
	    char *end1 = strchr(cur, '=');
	    char *end2 = (end1 ? strchr(end1, '&') : NULL);

	    if (end1) {
		*(end1++) = 0;
		if (end2) *(end2++) = 0;
		if (!strcmp(cur, "x"))
		    x = atoi(end1);
		else if (!strcmp(cur, "y"))
		    y = atoi(end1);
	    }
	    cur = end2;
	}
	if (x == -1  ||  y == -1)
	    bad_query_string();
    }
    else if (!(p = strchr(coords, ','))) {
	bad_query_string();
    }
    else {
	*(p++) = (char)0;
	x = atoi(coords);
	y = atoi(p);
    }

    /*
    ** Get config file
    */
    if (path_info && IMAGE_DIR) {
	config_file1 =
	    (char*)malloc(strlen(IMAGE_DIR) + strlen(path_info) + 2);
	if (!config_file1) outofmem(__FILE__, "config_file1-malloc");
	strcpy(config_file1, IMAGE_DIR);
	if (path_info[0] != '/') strcat(config_file1, "/");
	strcat(config_file1, path_info);
	fp = fopen(config_file1, "r");
    }
    if (!fp && path_translated && IMAGE_DIR) {
	config_file2 =
	    (char*)malloc(strlen(IMAGE_DIR) + strlen(path_translated) + 2);
	if (!config_file2) outofmem(__FILE__, "config_file2-malloc");
	strcpy(config_file2, IMAGE_DIR);
	if (path_translated[0] != '/') strcat(config_file2, "/");
	strcat(config_file2, path_translated);
	fp = fopen(config_file2, "r");
    }
    if (!fp && path_translated)
	fp = fopen(path_translated, "r");
    if (!fp && path_info)
	fp = fopen(path_info, "r");
    if (!fp) {
	char msg[1024];
	sprintf(msg, 
	"Picture config file not found, tried the following:<UL>");
	if (config_file1) {
	    strcat(msg, "\n<LI> "); strcat(msg, config_file1);
	}
	if (config_file2) {
	    strcat(msg, "\n<LI> "); strcat(msg, config_file2);
	}
	if (path_translated) {
	    strcat(msg, "\n<LI> "); strcat(msg, path_translated);
	}
	if (path_info) {
	    strcat(msg, "\n<LI> "); strcat(msg, path_info);
	}
	strcat(msg, "\n</UL>\n");
	fatal(msg);
    }

    pict = parse_picture(fp);
    fclose(fp);

    if (!pict)
	fatal("Error parsing picture config file");

    if (!(url = get_url(pict, x, y)))
	fatal("No URL returned, not even default set for the picture.");

    printf("Location: %s\r\n\r\n", url);

    return(0);	/* For gcc */
}

