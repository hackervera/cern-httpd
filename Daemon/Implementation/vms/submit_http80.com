$!===========================================================================
$! SUBMIT_HTTP80.COM
$! -----------------
$!
$! 01-APR-1994  F.Macrides	Command file for running the CERN v2.16betavms
$!				httpd as a detached process on port 80
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
$!-------------------------------------------------------
$! Specify a suitably privileged username in place of FOO
$!-------------------------------------------------------
$ SUBMIT/user=FOO/nolog HTTPD_Dir:START_HTTP80.COM
$ v = f$verify(v)
$ exit
