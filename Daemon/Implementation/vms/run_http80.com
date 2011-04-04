$!===========================================================================
$! RUN_HTTP80.COM (Evoked via START_HTTP80.COM)
$! --------------
$!
$! 01-APR-1994  F.Macrides	Input file for the CERN v2.16betavms httpd
$!				invoked as a detached process on port 80
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
$!RUN_HTTP80.COM procedure:
$!---------------------------
$ set proc/priv=(noall, sysprv, tmpmbx, netmbx)
$ if f$search("HTTPD_Dir:http80_trace.log") -
	then purge/keep=3 HTTPD_Dir:http80_trace.log
$ if f$search("HTTPD_Dir:http80_error.log") -
	then purge/keep=3 HTTPD_Dir:http80_error.log
$ show time
$!--------------------------------------------------------------
$!Run on  port (-p) 80 with
$!   rule file (-r) assigned  HTTPD_Dir:http80.conf
$!           errors logged in HTTPD_Dir:http80_error.log
$!         requests logged in HTTPD_Dir:http80.log
$! and job messages logged in HTTPD_Dir:http80_output.log 
$!--------------------------------------------------------------
$ httpd -r "HTTPD_Dir:http80.conf" -p 80
$ exit
