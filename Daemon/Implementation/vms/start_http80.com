$!===========================================================================
$! START_HTTP80.COM (Evoked via SUBMIT_HTTP80.COM)
$! ----------------
$!
$! 01-APR-1994  F.Macrides	Command file for calling the CERN v2.16betavms
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
$on error then continue
$ set proc/priv=(detach, sysprv, tmpmbx, netmbx)
$ run/detached/Process_Name="HTTP80_Daemon"-
	/buffer_limit=140000-
	/working_set=500/extent=2500/maximum_working_set=1500-
	/input=  HTTPD_Dir:RUN_HTTP80.COM-
	/output= HTTPD_Dir:HTTP80_output.log-
	/priv=   (nosame, sysprv, tmpmbx, netmbx)-
    Sys$System:Loginout
$ exit
