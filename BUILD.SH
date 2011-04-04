#!/bin/sh
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

UNAME=""
if [ -s /usr/bin/uname ]; then		UNAME=`/usr/bin/uname`; fi
if [ -s /bin/uname ]; then 		UNAME=`/bin/uname`; fi
if [ -s /usr/apollo/bin ]; then 	UNAME=`ver sys5.3 /bin/uname`; fi
if [ -s $UNAME == "" ]; then
    if [ -r /NextApps ]; then
        hostinfo | grep I386
        if [ $stat == 0 ]; then
            UNAME=next-386
        else
            UNAME=next
        fi
    fi
fi
if [ -s /usr/bin/ncrm ]; then 		UNAME=ncr; fi
if [ -s /usr/bin/swconfig ]; then 	UNAME=sco; fi
#
export UNAME

# For apollo, must use bsd mode. Also, WWW_MACH not inherited through make!
if [ "$UNAME" = "DomainOS" ]; then	WWW_MACH=apollo_m68k;export WWW_MACH;fi
if [ "$UNAME" = "next" ]; then		WWW_MACH=next;export WWW_MACH;fi
if [ "$UNAME" = "next-386" ]; then	WWW_MACH=next-386;export WWW_MACH;fi
if [ "$UNAME" = "ncr" ]; then		WWW_MACH=ncr;export WWW_MACH;fi
if [ "$UNAME" = "sco" ]; then
	WWW_MACH=sco;export WWW_MACH;
	PATH=${PATH}:/usr/local/bin;export PATH;
fi
if [ "$UNAME" = "HP-UX" ]; then		WWW_MACH=snake;export WWW_MACH;fi
if [ "$UNAME" = "IRIX" ]; then		WWW_MACH=sgi;export WWW_MACH;fi
if [ "$UNAME" = "SunOS" ]; then 
	arch=`arch`
	revision=`uname -r`
	if [ "`echo $revision | cut -d. -f1`" = "5" ]; then
		WWW_MACH=${arch}-sol2;export WWW_MACH
	else
		WWW_MACH=${arch};export WWW_MACH
	fi
fi
if [ "$UNAME" = "ULTRIX" ]; then	WWW_MACH=decstation;export WWW_MACH;fi
if [ "$UNAME" = "AIX" ]; then		WWW_MACH=rs6000;export WWW_MACH;fi
if [ "$UNAME" = "OSF1" ]; then		WWW_MACH=osf1;export WWW_MACH;fi
if [ "$UNAME" = "BSD/386" ]; then	WWW_MACH=unix;export WWW_MACH;fi
if [ "$UNAME" = "Linux" ]; then		WWW_MACH=linux;export WWW_MACH;fi
if [ "$UNAME" = "A/UX" ]; then		WWW_MACH=aux;export WWW_MACH;fi
if [ "$UNAME" = "SMP_DC.SOx" ]; then	WWW_MACH=pyramid;export WWW_MACH;fi

#
# ISC 3.0 (How can I tell I'm running on ISC 3.0 ???)
#
# set WWW_MACH=isc3.0
# setenv WWW_MACH isc3.0		# Lauren

if [ "$WWW_MACH" = "" ]; then
	echo
	echo "Please edit BUILD file to include your machine OS"
	echo "and mail differences back to www-bug@info.cern.ch"
	echo
	echo "If you are BUILDing for:"
	echo "	- ISC 3.0"
	echo "	- SCO"
	echo "just uncomment the corresponding lines in the BUILD script."
	echo
	exit -99
fi
echo "________________________________________________________________"
echo "WWW build for machine type:                            " $WWW_MACH

#	Now go do build

#	We don't want SHELL set to something funny to screw up make

(cd All/Implementation; unset SHELL; make)
stat=$?
echo
echo "WWW build for " $WWW_MACH  " done. status = " $stat
exit $stat
