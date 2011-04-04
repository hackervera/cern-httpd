$ V = 'F$VERIFY(0)'
$ ON ERROR THEN CONTINUE
$!
$!****************************************************************************
$!*									     *
$!*                          SPAWNINIT.COM                      	     *
$!*     (For passing symbols/logicals to sys$creprc()-created processes)     *
$!*                   Foteos Macrides (MACRIDES@SCI.WFEB.EDU)                *
$!*									     *
$!****************************************************************************
$!
$!  This command procedure should be executed by the sys$creprc()-created
$!  processes for script execution on VMS.
$!
$!  It's location and name should be defined via the "spawninit" field of
$!  the CERN httpd's rule file.
$!
$!  The rule file also can include a "scratchdir" field for designating the
$!  directory in which to create the httpd's temporary files associated with
$!  script execution.  Otherwise, they will be created in whatever is the
$!  "current default" directory of the httpd image.
$!
$!  Note that when the httpd is installed under Inetd/MULTINET_SERVER,
$!  such processes have FMODE() .eqs. "OTHER", do not (and should not)
$!  execute SYS$MANAGER:SYLOGIN.COM, and therefore need to acquire *all*
$!  their foreign command symbols from this alternate command procedure.
$!
$!  This command procedure also can be used to pass logicals to the
$!  sys$creprc()-created processes.
$!
$ CGIPARSE :== $device:[directory]CGIPARSE
$ HTIMAGE  :== $device:[directory]HTIMAGE
$!
$ EGR*EP   :== $device:[directory]EGREP
$ GAWK     :== $device:[directory]GAWK
$!
$ COP*Y    :== COPY/LOG
$ DEL*ETE  :== DELETE/LOG
$ PUR*GE   :== PURGE/LOG
$ REN*AME  :== RENAME/LOG
$ PRI*NT   :== PRINT/NOFLAG
$!
$ EXIT
