#!/bin/csh -f
#			Build all WWW Code for this platform
#
#	Figure out what sort of unix this is
#	(NeXT machines don't have uname!)

# When BUILDing a SOCKSified httpd:
# Set this to the location of your ready-built SOCKS library
# setenv SOCKSLIB /xxxx/yyyy/libsocks.a

## Use this setting to enable SOCKS:
# setenv SOCKS_FLAGS -DSOCKS
## ..OR this setting to enable both SOCKS _and_ client access control:
# setenv SOCKS_FLAGS "-DSOCKS -DCLIENT_CONTROL"
## Note that cern_httpd's normal access control can be used instead.

set UNAME=""
if (-e /usr/bin/uname)		set UNAME=`/usr/bin/uname`
if (-e /bin/uname)		set UNAME=`/bin/uname`
if (-e /usr/apollo/bin)		set UNAME=`ver sys5.3 /bin/uname`
if (-e /usr/bin/ncrm)		set UNAME=ncr
if (-e /usr/bin/swconfig)	set UNAME=sco
if ( $UNAME == "" ) then
    if (-r /NextApps ) then
        hostinfo | grep I386
        if  ( $status == 0 ) then
            set UNAME=next-386
        else
            set UNAME=next
        endif
    endif
endif
#
setenv UNAME $UNAME

# For apollo, must use bsd mode. Also, WWW_MACH not inherited through make!
if ($UNAME == next)		setenv WWW_MACH	next
if ($UNAME == next-386)		setenv WWW_MACH	next-386
if ($UNAME == ncr)		setenv WWW_MACH	ncr
if ($UNAME == sco) then
	setenv WWW_MACH	sco
	setenv PATH "${PATH}:/usr/local/bin"
endif
if ($UNAME == "HP-UX")		setenv WWW_MACH	snake
if ($UNAME == "IRIX")		setenv WWW_MACH	sgi
if ($UNAME == "SunOS") then
	set arch=`arch`
	set revision=`uname -r`
	if ($revision =~ 5.* ) then
		setenv WWW_MACH		${arch}-sol2
	else
		setenv	WWW_MACH	${arch}
	endif
endif
if ($UNAME == "ULTRIX")		setenv WWW_MACH	decstation
if ($UNAME == "AIX")    	setenv WWW_MACH	rs6000
if ($UNAME == "OSF1")   	setenv WWW_MACH	osf1
if ($UNAME == "BSD/386")	setenv WWW_MACH	unix
if ($UNAME == "Linux")		setenv WWW_MACH	linux
if ($UNAME == "A/UX")		setenv WWW_MACH	aux
if ($UNAME == "SMP_DC.SOx")	setenv WWW_MACH	pyramid
if ($UNAME == "uts") then
	# differentiate between UTS 2 (SVR3/BSD) and UTS 4 (SVR4)
	set version=`uname -v`
	if ($version =~ 2.*) then
		setenv WWW_MACH		uts2
	else
		setenv WWW_MACH		uts4
	endif
endif

#
# ISC 3.0 (How can I tell I'm running on ISC 3.0 ???)
#
# set WWW_MACH=isc3.0
# setenv WWW_MACH isc3.0		# Lauren

#
# DELL Unix (How can I tell I'm running on DELL????)
#
# setenv WWW_MACH dell

#
# Unisys Unix (How can I tell I'm running on Unisys????)
#
# setenv WWW_MACH unisys

if ($WWW_MACH == "") then
	echo
	echo "Please edit BUILD file to include your machine OS"
	echo "and mail differences back to libwww@info.cern.ch"
	echo
	echo "If you are BUILDing for:"
	echo "	- ISC 3.0"
	echo "	- DELL Unix SVR4"
	echo "	- Unisys Unix SVR4"
	echo "just uncomment the corresponding lines in the BUILD script."
	echo
	exit -99
endif
echo "________________________________________________________________"
echo "WWW build for machine type:                            " $WWW_MACH

#	Now go do build

#	We don't want SHELL set to something funny to screw up make

(cd All/Implementation; unsetenv SHELL; make $1)
set stat = $status
echo
echo "WWW build for " $WWW_MACH  " done. status = " $stat
exit $stat
