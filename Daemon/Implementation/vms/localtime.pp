$! 'f$verify(0)'
$!============================================================================
$! LOCALTIME.PP	-- F.Macrides
$! ------------
$!				Test script handling
$!
$!		This simple script executes the "show time" command
$!		on the http server and returns the local time and date
$!		as a text/html file to the client.
$!
$!============================================================================
$ say := "write WWW_OUT"
$ say "Content-Type: text/html"
$ say ""
$ say "<HEAD><TITLE>Local Date and Time</TITLE></HEAD>"
$ say "<BODY><PRE>"
$ say "Our local date and time are:"
$ say ""
$ show time
$ say "</PRE></BODY>"
$exit
