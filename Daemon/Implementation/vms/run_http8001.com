$!===========================================================================
$! RUN_HTTP8001.COM (Evoked via START_HTTP8001.COM)
$! ----------------
$!
$! 13-FEB-1994	F.Macrides (macrides@sci.wfeb.edu) -- Initial version.
$!		For testing CERN v2.14VMS http daemon as a detached
$!		process on port 8001
$!
$! 01-Apr-1994	F.Macrides Upgraded for CERN v2.16betavms
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
$! Define this foreign command, here, and similarly set up SpawnInit.com
$! BEFORE running this procedure, by replacing:
$!			"device"    apporpriately
$!			"platform"  with VAX or AXP
$!			"transport" with MULTINET or UCX
$!------------------------------------------------------------
$ httpd :== "$device:[WWW.DAEMON.platform.transport]httpd.exe"
$!------------------------------------------------------------
$ set noon
$ v = f$verify(1)
$!RUN_HTTP8001.COM procedure:
$!---------------------------
$ set proc/priv=(noall, sysprv, tmpmbx, netmbx)
$ if f$search("HTTPD_Dir:http8001_trace.log") -
	then purge/keep=3 HTTPD_Dir:http8001_trace.log
$ if f$search("HTTPD_Dir:http8001_error.log") -
	then purge/keep=3 HTTPD_Dir:http8001_error.log
$ show time
$!--------------------------------------------------------------
$!Run on  port (-p) 8001 with
$!   rule file (-r) assigned  HTTPD_Dir:http8001.conf
$!  TRACE info (-v) logged in HTTPD_Dir:http8001_trace.log
$!           errors logged in HTTPD_Dir:http8001_error.log
$!     and requests logged in HTTPD_Dir:http8001.log 
$!--------------------------------------------------------------
$ httpd -v -r "HTTPD_Dir:http8001.conf" -p 8001
$ exit
