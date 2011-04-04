$! 'f$verify(0)'
$!============================================================================
$! WELCOME.PP  - Foteos Macrides
$! ----------
$!			Test "Location:" handling
$!
$!		This simple script redirects the URL
$!			http://<host>[:port]/htbin/welcome
$!		to your default Welcome.html.
$!
$!	Why?    Well... a script could determine that a local file should be
$!		sent to the client.  You want the server to recheck the
$!		authorization via REDIRECTION_ON_THE_FLY before sending it,
$!		rather than kludging in such code yourself.  If you don't
$!		include an access and host field, as in this example, the
$!		server will do that and process the file if OK, rather than
$!		returning a complete URL for the client to resubmit.
$!
$!============================================================================
$ say := "write WWW_OUT"
$ say "Location: /www/Welcome.html"
$ say ""
$exit
