!	Make WorldWideWeb SERVER under VMS
!       =======================================================
!
!  07 Sep 93 	MD	Created for version 2.07 of httpd
!  06 Nov 93	MD	changed to run in [.vms] directory
!  13 Nov 93    MD	Added image part
!  22 Feb 94 (MD)	Changed for version 2.15
!			- added modules
!			- (WG) added DECC, LIST and MAP flags
!			- added ALPHA flag
!			- generates obj directory structure...
!
! Bugs:
!	The dependencies are anything but complete - they were
!	just enough to allow the files to be compiled.
!
! Instructions:
!	Use the correct command line for your TCP/IP implementation,
!	inside the [.VMS] directory:
!
!	$ MMS/MACRO=(MULTINET=1)	for Multinet
!	$ MMS/MACRO=(WIN_TCP=1)		for Wollongong TCP/IP
!	$ MMS/MACRO=(UCX=1)		for DEC/UCX
!	$ MMS/MACRO=(DECNET=1)		for socket emulation over DECnet
!
! To compile without rules (default is with rules):
!
!	$ MMS/MACRO=(NORULES=1)		without rules
!
! To compile with debug mode:
!	
!	$ MMS/MACRO=(DEBUG=1)		Debug flag on
!	$ MMS/MACRO=(LIST=1)       	Produce Listing Files
!	$ MMS/MACRO=(MAP=1)       	Produce Link Map
!
! To compile for DECC use:
!
!	$ MMS/MACRO=(DECC=1)		for DECC only. Generates .OLB
!
! But to compile for ALPHA use:
!                                                                       target dir
!	$ MMS/MACRO=(ALPHA=1)		on ALPHA (implies DECC) 	[.ALPHA.MULTINET]
!
!
!
! If you are on HEP net and want to build using the really latest sources on
! DXCERN:: then define an extra macro U=PRIAM::, e.g.
!
!	$ MMS/MACRO=(MULTINET=1, U=DXCERN::)	for Multinet
!
! This will copy the sources from DXCERN  as necessary. You can also try
!
!	$ MMS/MACRO=(U=DXCERN::) descrip.mms
!
! to update this file.
!

SRC = [-]
VMS = []
ETC = [--.ETC]
WWW_INCL = [---.Library.Implementation]
WWW_VMS  = [---.Library.Implementation.vms]



.INCLUDE $(SRC)Version.make


! debug flags
.IFDEF DEBUG
DEBUGFLAGS = /DEBUG/NOOPT
.ENDIF

.IFDEF ALPHA
ALPHA_EXT=_ALPHA
MACH=ALPHA
DECC=1
.ELSE
ALPHA_EXT= 
MACH=VAX
.ENDIF

.IFDEF DECC
.IFDEF DEBUG
LFLAGS=/DEBUG
.ELSE
LFLAGS=
.ENDIF
.IFDEF UCX
CQUALDECC=/Standard=VAXC/Prefix=ALL
.ELSE
CQUALDECC=/Standard=VAXC/Prefix=ANSI
.ENDIF
.ELSE
CQUALDECC=
.IFDEF DEBUG
LFLAGS=/DEBUG
.ELSE
LFLAGS= 
.ENDIF
.ENDIF

.IFDEF LIST
CLIST=/LIST/SHOW=ALL
.ELSE
CLIST=
.ENDIF

.IFDEF MAP
LMAP=/MAP=$(MMS$TARGET_NAME)/CROSS/FULL
.ELSE
LMAP=
.ENDIF

! extra defines
.IFDEF NORULES
EXTRADEFINES = VMS,DEBUG,ACCESS_AUTH,VD="""$(VD)"""
.ELSE
EXTRADEFINES = VMS,DEBUG,RULES,ACCESS_AUTH,VD="""$(VD)"""
.ENDIF


.IFDEF UCX
TCP=UCX
.ENDIF
.IFDEF MULTINET
TCP=MULTINET
.ENDIF
.IFDEF WIN_TCP
TCP=WIN_TCP
.ENDIF
.IFDEF DECNET
TCP=DECNET
.ENDIF

.IFDEF TCP
.ELSE
TCP=MULTINET
.ENDIF

! now exe points at [--.machine.tcp layer]
EXE=[--.$(MACH).$(TCP)]
WWW_LIB=[---.LIBRARY.$(MACH).$(TCP)]
CFLAGS = $(DEBUGFLAGS)/DEFINE=($(EXTRADEFINES),$(TCP))/INC=($(SRC),$(VMS),$(WWW_INCL),$(WWW_VMS))$(CQUALDECC)$(CLIST)

WWW_LIBS = $(WWW_LIB)wwwlib/lib

SETUP_FILES = $(SRC)Version.make

VMS_FILES = $(VMS)setup.com $(VMS)descrip.mms -
		$(ETC)00readme.html -
		$(ETC)00readme.txt -
		$(ETC)HTImage.com -
		$(ETC)echo.com -
		$(ETC)example_httpd.conf -
		$(ETC)htdir.com -
		$(ETC)http80.conf $(ETC)http8001.conf -
		$(ETC)inetd80.conf $(ETC)inetd_httpd.conf -
		$(ETC)localtime.pp -
		$(ETC)query.com $(ETC)queryvms.c -
		$(ETC)run_http80.com $(ETC)run_http8001.com -
		$(ETC)spawninit.com -
		$(ETC)start_http80.com $(ETC)start_http8001.com -
		$(ETC)submit_http80.com $(ETC)submit_http8001.com -
		$(ETC)wfebgopher.pp -
		$(ETC)welcome.pp

HTTPD_OBJECTS = $(EXE)HTDaemon.obj, $(EXE)HTRequest.obj, $(EXE)HTRetrieve.obj, $(EXE)HTScript.obj, $(EXE)HTLoad.obj, -
		$(EXE)HTCache.obj, $(EXE)HTCacheInfo.obj, $(EXE)HTConfig.obj, $(EXE)HTWild.obj, -
		$(EXE)HTSInit.obj, $(EXE)HTSUtils.obj, $(EXE)HTims.obj, -
		$(EXE)HTPasswd.obj, $(EXE)HTAuth.obj, $(EXE)HTLex.obj, $(EXE)HTGroup.obj, $(EXE)HTACL.obj, $(EXE)HTAAProt.obj, -
		$(EXE)HTAAServ.obj, $(EXE)HTAAFile.obj, $(EXE)HTLog.obj, $(EXE)HTgc.obj, $(EXE)HTUserInit.obj, $(EXE)HTRFC931.obj
          
HTTPD_HEADERS = $(SRC)HTDaemon.h $(SRC)HTRequest.h $(SRC)HTims.h -
		$(SRC)HTCache.h $(SRC)HTConfig.h $(SRC)HTWild.h -
		$(SRC)HTScript.h $(SRC)HTPasswd.h $(SRC)HTAuth.h -
		$(SRC)HTLex.h $(SRC)HTGroup.h $(SRC)HTACL.h -
		$(SRC)HTAAProt.h $(SRC)HTAAServ.h $(SRC)HTAAFile.h -
		$(SRC)HTLog.h -
		$(SRC)HTUserInit.h $(SRC)HTSUtils.h

HTADM_OBJECTS = $(EXE)HTAdm.obj, $(EXE)HTPasswd.obj, $(EXE)HTAAFile.obj, -
                $(EXE)HTextDummy.obj

HTADM_HEADERS =

HTIMAGE_OBJECTS = $(EXE)HTImage.obj, -
                $(EXE)HTextDummy.obj

HTIMAGE_HEADERS = 

CGIPARSE_OBJECTS = $(EXE)CGIParse.obj, -
                $(EXE)HTextDummy.obj

CGIPARSE_HEADERS = 

CGIUTILS_OBJECTS = $(EXE)cgiutils.obj, $(EXE)HTSUtils.obj, -
                $(EXE)HTextDummy.obj

CGIUTILS_HEADERS = $(SRC)HTSUtils.h

VMSHELP_OBJECTS = $(EXE)VMSHelpGate.obj, $(EXE)HTDaemon.obj, -
                $(EXE)HTextDummy.obj
          
VMSHELP_HEADERS = $(SRC)HTDaemon.h

!___________________________________________________________________
.FIRST
	@ WRITE SYS$OUTPUT "Creating WWW Server for "$(TCP)" on "$(MACH)"." 
	@ WRITE SYS$OUTPUT "=================================================" 
	@ IF "''F$SEARCH("$(EXE)*.*")'" .EQS. "" -
	   THEN CREATE/DIR $(EXE)
	@ IF "''F$SEARCH("$(ETC)*.*")'" .EQS. "" -
	   THEN CREATE/DIR $(ETC)
!___________________________________________________________________
! ALL executables
!vmshelpgate 
exe : $(SETUP_FILES) $(VMS_FILES) $(EXE)setup.com httpd htadm htimage cgiparse cgiutils $(VMS)build_$(TCP).com$(ALPHA_EXT)
	@ continue

exe_only : httpd htadm htimage cgiparse cgiutils $(EXE)setup.com 
	@ continue

$(VMS)build_$(TCP).com$(ALPHA_EXT) : $(VMS)descrip.mms
	mms/noaction/from_sources/out=$(VMS)build_$(TCP).com$(ALPHA_EXT)/macro=($(TCP)=1,$(MACH)=1) exe_only
	
$(EXE)setup.com : $(VMS)setup.com
	copy $(VMS)setup.com $(EXE)setup.com

!___________________________________________________________________
! htadm EXE

htadm : $(HTADM_HEADERS) $(EXE)htadm.exe 
 	@ continue

$(EXE)htadm.exe : 	$(HTADM_OBJECTS)
	link $(LFLAGS)$(LMAP)/exe=$(EXE)htadm.exe $(HTADM_OBJECTS), $(WWW_LIBS), $(LIB)wwwlib.opt/opt

!___________________________________________________________________
! htimage EXE

htimage : $(HTIMAGE_HEADERS) $(EXE)htimage.exe 
 	@ continue

$(EXE)htimage.exe : 	$(HTIMAGE_OBJECTS)
	link $(LFLAGS)$(LMAP)/exe=$(EXE)htimage.exe $(HTIMAGE_OBJECTS), $(WWW_LIBS), $(WWW_LIB)wwwlib.opt/opt

!___________________________________________________________________
! cgiparse EXE

cgiparse : $(CGIPARSE_HEADERS) $(EXE)cgiparse.exe 
 	@ continue

$(EXE)cgiparse.exe : 	$(CGIPARSE_OBJECTS)
	link $(LFLAGS)$(LMAP)/exe=$(EXE)cgiparse.exe $(CGIPARSE_OBJECTS), $(WWW_LIBS), $(WWW_LIB)wwwlib.opt/opt

!___________________________________________________________________
! cgiutils EXE

cgiutils : $(CGIUTILS_HEADERS) $(EXE)cgiutils.exe 
 	@ continue

$(EXE)cgiutils.exe : 	$(CGIUTILS_OBJECTS)
	link $(LFLAGS)$(LMAP)/exe=$(EXE)cgiutils.exe $(CGIUTILS_OBJECTS), $(WWW_LIBS), $(WWW_LIB)wwwlib.opt/opt

!___________________________________________________________________
! httpd EXE

httpd : $(HTTPD_HEADERS) $(EXE)httpd.exe 
 	@ continue

$(EXE)httpd.exe : $(HTTPD_OBJECTS)
	link $(LFLAGS)$(LMAP)/exe=$(EXE)httpd.exe $(HTTPD_OBJECTS), $(WWW_LIBS), $(WWW_LIB)wwwlib.opt/opt

!___________________________________________________________________
! VMSHelpGate EXE

vmshelpgate : $(VMSHELP_HEADERS) $(EXE)vmshelpgate.exe 
 	@ continue

$(EXE)vmshelpgate.exe : $(VMSHELP_OBJECTS)
	link $(LFLAGS)$(LMAP)/exe=$(EXE)vmshelpgate.exe $(VMSHELP_OBJECTS), $(WWW_LIBS), $(LIBS) 

!_____________________________	HTDaemon

$(EXE)HTDaemon.obj   : $(SRC)HTDaemon.c $(SRC)HTDaemon.h $(SRC)Version.make -
		 $(WWW_INCL)HTUtils.h $(WWW_INCL)tcp.h -
                 $(WWW_INCL)HTTCP.h $(WWW_VMS)HTVMSUtils.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTDaemon.c
.IFDEF U
$(SRC)HTDaemon.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTDaemon.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTDaemon.c" - 
             $(SRC)HTDaemon.c
$(SRC)HTDaemon.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTDaemon.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTDaemon.h" -
             $(SRC)HTDaemon.h
.ENDIF
!_____________________________	HTextDummy

$(EXE)HTextDummy.obj   : $(VMS)HTextDummy.c $(WWW_INCL)HText.h
        cc $(CFLAGS)/obj=$*.obj $(VMS)HTextDummy.c
.IFDEF U
$(VMS)HTextDummy.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/HTextDummy.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/HTextDummy.c" - 
             $(VMS)HTextDummy.c
.ENDIF
!_____________________________	HTSUtils

$(EXE)HTSUtils.obj   : $(SRC)HTSUtils.c $(SRC)HTSUtils.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTSUtils.c
.IFDEF U
$(SRC)HTSUtils.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTSUtils.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTSUtils.c" - 
             $(SRC)HTSUtils.c
$(SRC)HTSUtils.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTSUtils.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTSUtils.h" - 
             $(SRC)HTSUtils.h
.ENDIF
!_____________________________	HTSInit

$(EXE)HTSInit.obj   : $(SRC)HTSInit.c 
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTSInit.c
.IFDEF U
$(SRC)HTSInit.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTSInit.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTSInit.c" - 
             $(SRC)HTSInit.c
.ENDIF
!_____________________________	HTUserInit

$(EXE)HTUserInit.obj   : $(SRC)HTUserInit.c $(SRC)HTUserInit.h -
		 $(WWW_INCL)HTUtils.h 
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTUserInit.c
.IFDEF U
$(SRC)HTUserInit.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTUserInit.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTUserInit.c" - 
             $(SRC)HTUserInit.c
$(SRC)HTUserInit.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTUserInit.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTUserInit.h" -
             $(SRC)HTUserInit.h
.ENDIF
!_____________________________	HTRequest

$(EXE)HTRequest.obj   : $(SRC)HTRequest.c $(SRC)Version.make -
		 $(WWW_INCL)HTUtils.h $(WWW_INCL)tcp.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTRequest.c
.IFDEF U
$(SRC)HTRequest.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTRequest.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTRequest.c" - 
             $(SRC)HTRequest.c
$(SRC)HTRequest.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTRequest.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTRequest.h" - 
             $(SRC)HTRequest.h
.ENDIF
!_____________________________	HTRetrieve

$(EXE)HTRetrieve.obj   : $(SRC)HTRetrieve.c -
		 $(WWW_INCL)HTUtils.h $(WWW_INCL)tcp.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTRetrieve.c
.IFDEF U
$(SRC)HTRetrieve.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTRetrieve.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTRetrieve.c" - 
             $(SRC)HTRetrieve.c
.ENDIF
!_____________________________	HTLoad

$(EXE)HTLoad.obj   : $(SRC)HTLoad.c -
		 $(WWW_INCL)HTUtils.h 
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTLoad.c
.IFDEF U
$(SRC)HTLoad.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTLoad.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTLoad.c" - 
             $(SRC)HTLoad.c
.ENDIF
!_____________________________	HTAnnotate

$(EXE)HTAnnotate.obj   : $(SRC)HTAnnotate.c -
		 $(WWW_INCL)HTUtils.h 
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTAnnotate.c
.IFDEF U
$(SRC)HTAnnotate.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAnnotate.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAnnotate.c" - 
             $(SRC)HTAnnotate.c
.ENDIF
!_____________________________	HTPut

$(EXE)HTPut.obj   : $(SRC)HTPut.c -
		 $(WWW_INCL)HTUtils.h 
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTPut.c
.IFDEF U
$(SRC)HTPut.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTPut.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTPut.c" - 
             $(SRC)HTPut.c
.ENDIF
!_____________________________	HTCache

$(EXE)HTCache.obj   : $(SRC)HTCache.c -
		 $(WWW_INCL)HTUtils.h $(SRC)HTCache.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTCache.c
.IFDEF U
$(SRC)HTCache.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTCache.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTCache.c" - 
             $(SRC)HTCache.c
$(SRC)HTCache.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTCache.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTCache.h" - 
             $(SRC)HTCache.h
.ENDIF
!_____________________________	HTCacheInfo

$(EXE)HTCacheInfo.obj   : $(SRC)HTCacheInfo.c -
		 $(WWW_INCL)HTUtils.h $(SRC)HTCache.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTCacheInfo.c
.IFDEF U
$(SRC)HTCacheInfo.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTCacheInfo.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTCacheInfo.c" - 
             $(SRC)HTCacheInfo.c
.ENDIF
!_____________________________	HTConfig

$(EXE)HTConfig.obj   : $(SRC)HTConfig.c -
		 $(WWW_INCL)HTUtils.h $(SRC)HTConfig.h $(WWW_VMS)HTVMSUtils.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTConfig.c
.IFDEF U
$(SRC)HTConfig.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTConfig.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTConfig.c" - 
             $(SRC)HTConfig.c
$(SRC)HTConfig.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTConfig.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTConfig.h" - 
             $(SRC)HTConfig.h
.ENDIF
!_____________________________	HTWild

$(EXE)HTWild.obj   : $(SRC)HTWild.c -
		 $(WWW_INCL)HTUtils.h $(SRC)HTWild.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTWild.c
.IFDEF U
$(SRC)HTWild.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTWild.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTWild.c" - 
             $(SRC)HTWild.c
$(SRC)HTWild.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTWild.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTWild.h" - 
             $(SRC)HTWild.h
.ENDIF
!_____________________________	HTScript

$(EXE)HTScript.obj   : $(SRC)HTScript.c $(SRC)HTScript.h $(SRC)Version.make -
		 $(WWW_INCL)HTUtils.h $(WWW_INCL)tcp.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTScript.c
.IFDEF U
$(SRC)HTScript.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTScript.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTScript.c" - 
             $(SRC)HTScript.c
$(SRC)HTScript.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTScript.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTScript.h" - 
             $(SRC)HTScript.h
.ENDIF
!_____________________________	HTRFC931

$(EXE)HTRFC931.obj   : $(SRC)HTRFC931.c 
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTRFC931.c
.IFDEF U
$(SRC)HTRFC931.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTRFC931.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTRFC931.c" - 
             $(SRC)HTRFC931.c
.ENDIF

! Access Authorisation code

!_____________________________	HTAAFile

$(EXE)HTAAFile.obj   : $(SRC)HTAAFile.c $(SRC)HTAAFile.h $(WWW_INCL)HTAAUtil.h $(WWW_INCL)HTUtils.h $(WWW_INCL)HTList.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTAAFile.c
.IFDEF U
$(SRC)HTAAFile.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAAFile.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAAFile.c" - 
             $(SRC)HTAAFile.c
$(SRC)HTAAFile.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAAFile.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAAFile.h" -
             $(SRC)HTAAFile.h
.ENDIF
!_____________________________	HTPasswd

$(EXE)HTPasswd.obj   : $(SRC)HTPasswd.c $(SRC)HTPasswd.h $(WWW_INCL)HTAAUtil.h $(SRC)HTAAFile.h 
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTPasswd.c
.IFDEF U
$(SRC)HTPasswd.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTPasswd.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTPasswd.c" - 
             $(SRC)HTPasswd.c
$(SRC)HTPasswd.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTPasswd.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTPasswd.h" -
             $(SRC)HTPasswd.h
.ENDIF
!_____________________________	HTGroup

$(EXE)HTGroup.obj   : $(SRC)HTGroup.c $(SRC)HTGroup.h $(WWW_INCL)HTAAUtil.h $(SRC)HTAAFile.h $(WWW_INCL)HTAssoc.h $(SRC)HTLex.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTGroup.c
.IFDEF U
$(SRC)HTGroup.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTGroup.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTGroup.c" - 
             $(SRC)HTGroup.c
$(SRC)HTGroup.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTGroup.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTGroup.h" -
             $(SRC)HTGroup.h
.ENDIF
!_____________________________	HTACL

$(EXE)HTACL.obj   : $(SRC)HTACL.c $(SRC)HTACL.h $(WWW_INCL)HTAAUtil.h $(SRC)HTAAFile.h $(SRC)HTGroup.h $(WWW_INCL)HTAssoc.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTACL.c
.IFDEF U
$(SRC)HTACL.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTACL.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTACL.c" - 
             $(SRC)HTACL.c
$(SRC)HTACL.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTACL.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTACL.h" -
             $(SRC)HTACL.h
.ENDIF
!_____________________________	HTAuth

$(EXE)HTAuth.obj   : $(SRC)HTAuth.c $(SRC)HTAuth.h $(WWW_INCL)HTAAUtil.h $(SRC)HTPasswd.h $(SRC)HTAAFile.h $(WWW_INCL)HTAssoc.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTAuth.c
.IFDEF U
$(SRC)HTAuth.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAuth.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAuth.c" - 
             $(SRC)HTAuth.c
$(SRC)HTAuth.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAuth.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAuth.h" -
             $(SRC)HTAuth.h
.ENDIF
!_____________________________	HTAAServ

$(EXE)HTAAServ.obj   : $(SRC)HTAAServ.c $(SRC)HTAAServ.h $(WWW_INCL)HTAAUtil.h - 
			$(SRC)HTAAFile.h $(SRC)HTPasswd.h $(SRC)HTGroup.h - 
			$(SRC)HTACL.h $(SRC)HTAuth.h $(WWW_INCL)HTUU.h -
			$(WWW_INCL)HTParse.h $(WWW_INCL)HTList.h -
			$(WWW_INCL)HTUtils.h $(WWW_INCL)HTString.h -
			$(WWW_INCL)HTRules.h $(SRC)HTAAProt.h -
			$(WWW_INCL)HTAssoc.h $(SRC)HTLex.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTAAServ.c
.IFDEF U
$(SRC)HTAAServ.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAAServ.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAAServ.c" - 
             $(SRC)HTAAServ.c
$(SRC)HTAAServ.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAAServ.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAAServ.h" -
             $(SRC)HTAAServ.h
.ENDIF
!_____________________________	HTAAProt

$(EXE)HTAAProt.obj   : $(SRC)HTAAProt.c $(SRC)HTAAProt.h $(WWW_INCL)HTUtils.h $(WWW_INCL)HTAAUtil.h $(SRC)HTAAfile.h $(WWW_INCL)HTAssoc.h $(SRC)HTLex.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTAAProt.c
.IFDEF U
$(SRC)HTAAProt.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAAProt.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAAProt.c" - 
             $(SRC)HTAAProt.c
$(SRC)HTAAProt.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAAProt.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAAProt.h" -
             $(SRC)HTAAProt.h
.ENDIF
!_____________________________	HTLex

$(EXE)HTLex.obj   : $(SRC)HTLex.c $(SRC)HTLex.h $(WWW_INCL)HTUtils.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTLex.c
.IFDEF U
$(SRC)HTLex.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTLex.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTLex.c" - 
             $(SRC)HTLex.c
$(SRC)HTLex.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTLex.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTLex.h" -
             $(SRC)HTLex.h
.ENDIF
!_____________________________	HTLog

$(EXE)HTLog.obj   : $(SRC)HTLog.c $(SRC)HTLog.h $(WWW_INCL)HTUtils.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTLog.c
.IFDEF U
$(SRC)HTLog.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTLog.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTLog.c" - 
             $(SRC)HTLog.c
$(SRC)HTLog.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTLog.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTLog.h" - 
             $(SRC)HTLog.h
.ENDIF

!_____________________________	HTims

$(EXE)HTims.obj   : $(SRC)HTims.c $(SRC)HTims.h $(WWW_INCL)HTUtils.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTims.c
.IFDEF U
$(SRC)HTims.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTims.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTims.c" - 
             $(SRC)HTims.c
$(SRC)HTims.h : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTims.h"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTims.h" - 
             $(SRC)HTims.h
.ENDIF

!_____________________________	HTgc

$(EXE)HTgc.obj   : $(SRC)HTgc.c 
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTgc.c
.IFDEF U
$(SRC)HTgc.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTgc.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTgc.c" - 
             $(SRC)HTgc.c
.ENDIF



!_____________________________	HTAdm

$(EXE)HTAdm.obj   : $(SRC)HTAdm.c -
		 $(WWW_INCL)HTUtils.h $(WWW_INCL)HTAlert.h -
                 $(WWW_INCL)HTAAUtil.h $(SRC)HTPasswd.h
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTAdm.c
.IFDEF U
$(SRC)HTAdm.c :    $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAdm.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTAdm.c" - 
             $(SRC)HTAdm.c
.ENDIF
!_____________________________	CGIParse

$(EXE)CGIParse.obj   : $(SRC)CGIParse.c -
		 $(WWW_INCL)HTUtils.h 
        cc $(CFLAGS)/obj=$*.obj $(SRC)CGIParse.c
.IFDEF U
$(SRC)CGIParse.c :    $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/CGIParse.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/CGIParse.c" - 
             $(SRC)CGIParse.c
.ENDIF
!_____________________________	cgiutils

$(EXE)cgiutils.obj   : $(SRC)cgiutils.c -
		 $(WWW_INCL)HTUtils.h $(SRC)HTSUtils.h 
        cc $(CFLAGS)/obj=$*.obj $(SRC)cgiutils.c
.IFDEF U
$(SRC)cgiutils.c :    $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/cgiutils.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/cgiutils.c" - 
             $(SRC)cgiutils.c
.ENDIF
!_____________________________	HTImage

$(EXE)HTImage.obj   : $(SRC)HTImage.c -
		 $(WWW_INCL)HTUtils.h 
        cc $(CFLAGS)/obj=$*.obj $(SRC)HTImage.c
.IFDEF U
$(SRC)HTImage.c :    $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTImage.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/HTImage.c" - 
             $(SRC)HTImage.c
.ENDIF
!_____________________________	VMSHelpGate

$(EXE)VMSHelpGate.obj   : $(SRC)VMSHelpGate.c -
		 $(WWW_INCL)HTUtils.h $(WWW_INCL)tcp.h -
                 $(WWW_INCL)HTString.h 
        cc $(CFLAGS)/obj=$*.obj $(SRC)VMSHelpGate.c
.IFDEF U
$(SRC)VMSHelpGate.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/VMSHelpGate.c"
	     copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/VMSHelpGate.c" - 
             $(SRC)VMSHelpGate.c
.ENDIF
! ______________________________  The version file

.IFDEF U
$(SRC)Version.make :  $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/Version.make"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/Version.make" - 
        $(SRC)Version.make
	@- write sys$output "Please rebuild with new Version file"
	@- exit 2	! Error
.ENDIF

! __________________________ VMS SPECIAL FILES:
! latest version of this one:

.IFDEF U
$(VMS)descrip.mms : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/descrip.mms"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/descrip.mms" -
	$(VMS)descrip.mms
	@- write sys$output "Please rebuild with new MMS file"
	@- exit 2	! Error
                    
$(VMS)setup.com : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/setup.com"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/setup.com" -
	$(VMS)setup.com

$(ETC)00readme.html : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/00readme.html"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/00readme.html" -
	$(ETC)00readme.html
                    
$(ETC)00readme.txt : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/00readme.txt"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/00readme.txt" -
	$(ETC)00readme.txt
                    
$(ETC)HTImage.com : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/HTImage.com"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/HTImage.com" -
	$(ETC)HTImage.com
                    
$(ETC)echo.com : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/echo.com"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/echo.com" -
	$(ETC)echo.com
                    
$(ETC)example_httpd.conf : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/example_httpd.conf"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/example_httpd.conf" -
	$(ETC)example_httpd.conf
                    
$(ETC)htdir.com : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/htdir.com"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/htdir.com" -
	$(ETC)htdir.com
                    
$(ETC)http80.conf : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/http80.conf"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/http80.conf" -
	$(ETC)http80.conf
                    
$(ETC)http8001.conf : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/http8001.conf"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/http8001.conf" -
	$(ETC)http8001.conf
                    
$(ETC)inetd80.conf : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/inetd80.conf"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/inetd80.conf" -
	$(ETC)inetd80.conf
                    
$(ETC)inetd_httpd.conf : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/inetd_httpd.conf"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/inetd_httpd.conf" -
	$(ETC)inetd_httpd.conf
                    
$(ETC)localtime.pp : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/localtime.pp"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/localtime.pp" -
	$(ETC)localtime.pp
                    
$(ETC)query.com : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/query.com"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/query.com" -
	$(ETC)query.com
                    
$(ETC)queryvms.c : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/queryvms.c"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/queryvms.c" -
	$(ETC)queryvms.c
                    
$(ETC)run_http80.com : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/run_http80.com"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/run_http80.com" -
	$(ETC)run_http80.com
                    
$(ETC)run_http8001.com : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/run_http8001.com"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/run_http8001.com" -
	$(ETC)run_http8001.com
                    
$(ETC)spawninit.com : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/spawninit.com"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/spawninit.com" -
	$(ETC)spawninit.com
                    
$(ETC)start_http80.com : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/start_http80.com"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/start_http80.com" -
	$(ETC)start_http80.com
                    
$(ETC)start_http8001.com : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/start_http8001.com"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/start_http8001.com" -
	$(ETC)start_http8001.com
                    
$(ETC)submit_http80.com : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/submit_http80.com"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/submit_http80.com" -
	$(ETC)submit_http80.com
                    
$(ETC)submit_http8001.com : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/submit_http8001.com"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/submit_http8001.com" -
	$(ETC)submit_http8001.com
                    
$(ETC)wfebgopher.pp : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/wfebgopher.pp"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/wfebgopher.pp" -
	$(ETC)wfebgopher.pp
                    
$(ETC)welcome.pp : $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/welcome.pp"
	copy $(U)"/userd/tbl/hypertext/WWW-duns/Daemon/Implementation/vms/welcome.pp" -
	$(ETC)welcome.pp
                    
.ENDIF


