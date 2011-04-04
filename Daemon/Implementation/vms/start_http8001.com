$!===========================================================================
$! START_HTTP8001.COM (Evoked via SUBMIT_HTTP8001.COM)
$! ------------------
$!
$! 13-FEB-1994	F.Macrides (macrides@sci.wfeb.edu) -- Initial version.
$!		For testing CERN v2.14VMS http daemon as a detached
$!		 process on port 8001
$!
$! 01-APR-1994  F.Macrides Ungraded for CERN v2.16betavms
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
$ run/detached/Process_Name="HTTP8001_Daemon"-
	/buffer_limit=140000-
	/working_set=500/extent=2500/maximum_working_set=1500-
	/input=  HTTPD_Dir:RUN_HTTP8001.COM-
	/output= HTTPD_Dir:HTTP8001_trace.log-
	/priv=   (nosame, sysprv, tmpmbx, netmbx)-
    Sys$System:Loginout
$ exit
