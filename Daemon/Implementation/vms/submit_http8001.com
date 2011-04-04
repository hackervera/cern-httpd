$!===========================================================================
$! SUBMIT_HTTP8001.COM
$! -------------------
$!
$! 13-FEB-1994	F.Macrides (macrides@sci.wfeb.edu) -- Initial version.
$!		For testing CERN v2.14VMS http daemon as a detached
$!		 process on port 8001
$!
$! 01-APR-1994  F.Macrides Upgraded for CERN v2.16betavms
$!
$!===========================================================================
$!	Define these logicals BEFORE running this procedure:
$!---------------------------------------------------------------------------
$! DEFINE/SYSTEM/NOLOG "HTTPD_Dir" device:[WWW.DAEMON.ETC]
$! DEFINE/SYSTEM/EXEC/TRAN=CONCEAL/NAME=NO_ALIAS "WWW_Root" device:[WWW_DATA.]
$!----------------------------------------------------------------------------
$!	and have a startup file:  WWW_Root:[000000]Welcome.html
$!============================================================================
$!
$ v = f$verify(1)
$ set proc/priv=(detach, sysprv, tmpmbx, netmbx)
$ SUBMIT/user=SYSTEM/nolog HTTPD_Dir:START_HTTP8001.COM
$ v = f$verify(v)
$ exit
