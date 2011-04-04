$!
$!     Echo command for HTbin.
$!
$!
$ say = "write WWW_OUT"
$ say "Content-Type: text/html"
$ say ""
$ say "<HTML>"
$ say "<TITLE>Echo for htbin</TITLE>"
$ say "<H1>Echo for htbin</H1>"
$ say "The following symbols were passed to this script:<br>"
$ say "<PRE>"
$ show sym WWW_*
$ say "</PRE>"
$exit
