/*
 * Dummy module to prevent unresolved links to routines that are not called
 * anyway (client routines)
 *
 * Mark Donszelmann
 *
 * BUG: only routines referred to are included....
 *
 */

#include "htext.h"
#include "HTConfig.h"

PUBLIC void HText_appendCharacter ARGS2(HText *, text, char, ch) {};
PUBLIC void HText_appendText ARGS2(HText *, text, CONST char *, str) {};
PUBLIC void HText_appendParagraph ARGS1(HText *, text) {};
PUBLIC void HText_appendImage ARGS5(
        HText *         ,text,
        HTChildAnchor * ,anc,
        CONST char *    ,alternative_text,
        CONST char *    ,alignment,
        BOOL            ,isMap) {};
PUBLIC void HText_beginAppend ARGS1(HText *, text) {};
PUBLIC void HText_endAppend ARGS1(HText *, text) {};
PUBLIC void HText_beginAnchor ARGS2(HText *, text, HTChildAnchor *, anc) {};
PUBLIC void HText_endAnchor ARGS1(HText *, text) {};
PUBLIC BOOL HText_select ARGS1(HText *, text) { return (0); };
PUBLIC BOOL HText_selectAnchor ARGS2(HText *, text, HTChildAnchor*, anchor) { return (0); };
PUBLIC HText * HText_new2 ARGS2(HTParentAnchor *,     anchor,
                          HTStream *,              output_stream) { return (0); };
PUBLIC void HText_setStyle ARGS2(HText *, text, HTStyle *, style) {};

PUBLIC HTAAProt * HTDefProt = NULL;
PUBLIC HTAAProt * HTProt    = NULL;

PUBLIC HTParentAnchor * HTMainAnchor = 0;

PUBLIC HText * HTMainText = 0;

PUBLIC HTServerConfig   sc = 0;
PUBLIC HTResourceConfig rc = 0;
PUBLIC HTCacheConfig    cc = 0;
