$! 'f$verify(0)'
$!============================================================================
$! WFEBGOPHER.PP  - F.Macrides
$! -------------
$!			Test "Location:" handling
$!
$!		This simple script redirects the URL
$!			http://<host>[:port]/htbin/wfebgopher
$!		to the entry menu of the WFEB gopher server.
$!
$!	Why?	Well...  A script could decide that what the client wants is
$!		a file, menu, or service on another server.  If the script
$!		returns a complete URL as a Location field (instead of a
$!		Content-Type), the client should go get it from that server,
$!		as a resubmission which invokes access checking on that other
$!		server.
$!
$!============================================================================
$ say := "write WWW_OUT"
$ say "Location: gopher://sci.wfeb.edu/1"
$ say ""
$exit
