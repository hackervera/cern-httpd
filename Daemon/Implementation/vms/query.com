$! 'f$verify(0)'
$!============================================================================
$! QUERY.COM	(Called as an htbin script)
$! ---------
$!	      Report POST or GET Form name/value pairs (F.Macrides)
$!
$!	This simple script calls queryvms.exe with the headers switch
$!	to return a form entries report for the CERN httpd, like those
$!	returned by the post-query and query procedures for the NCSA
$!	httpd.
$!
$!============================================================================
$ queryvms := $HTTPD_Dir:queryvms.exe
$ queryvms -headers
$exit
