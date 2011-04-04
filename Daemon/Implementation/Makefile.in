#       Makefile input file for CERN httpd Daemon
#       -----------------------------------------
#
#   Copyright CERN 1990, 1991, 1994 -- See Copyright.html for conditions.
#
SHELL = /bin/sh

.SUFFIXES :
.SUFFIXES : .c .o .h .html .txt
.DEFAULT  : all

# Import configuration information
VD = @VD@

CC = @CC@
VPATH = @srcdir@

INSTALL = @INSTALL@
INSTALL_PROGRAM = @INSTALL_PROGRAM@
INSTALL_DATA = @INSTALL_DATA@
MKDIR = @MKDIR@
CHMOD = @CHMOD@
CP = @CP@
RM = @RM@
SED = @SED@
TAR = @TAR@
CVS = @CVS@
COMPRESS = @COMPRESS@
LN_S = @LN_S@

CFLAGS = @CFLAGS@ @DEFS@ -I@srcdir@ -I. -I../../Library/Implementation
LDFLAGS = @LDFLAGS@
LIBS = @WWWLIB@ -lwww @LIBS@

prefix = @prefix@
exec_prefix = @exec_prefix@
top_srcdir = @top_srcdir@
srcdir = @srcdir@
transform = @program_transform_name@

VMS = @srcdir@/vms

LIBDIR = @exec_prefix@/lib
BINDIR = @exec_prefix@/bin

PURIFY = purify -logfile=~/purify-output -cache-dir=~/purify-cache

# Conversion rules
.html.h:
	-$(CHMOD) +w $@
	$(BINDIR)/www -w90 -na -p -to text/x-c $< > $@
	-$(CHMOD) -w $@

.html.txt:
	$(BINDIR)/www -n -p66 $< > $@

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

OBJS = HTDaemon.o HTRequest.o   HTRetrieve.o HTScript.o HTLoad.o   \
       HTCache.o  HTCacheInfo.o HTConfig.o   HTWild.o   HTSInit.o  \
       HTSUtils.o HTims.o       HTPasswd.o   HTAuth.o   HTLex.o    \
       HTGroup.o  HTACL.o       HTAAProt.o   HTAAServ.o HTAAFile.o \
       HTLog.o    HTgc.o        HTUserInit.o HTRFC931.o

HFILES = HTPasswd.h HTAuth.h   HTLex.h HTGroup.h HTACL.h HTAAProt.h \
         HTAAServ.h HTAAFile.h sysdep.h

all: httpd htadm htimage cgiparse cgiutils

httpd: $(OBJS)
	$(CC) -o ./httpd_$(VD) $(OBJS) $(LDFLAGS) -L$(LIBDIR) $(LIBS)
	-$(RM) ./httpd
	$(LN_S) ./httpd_$(VD) ./httpd

htadm: HTAdm.o HTPasswd.o HTAAFile.o
	$(CC) -o ./htadm_$(VD) HTAdm.o HTPasswd.o HTAAFile.o $(LDFLAGS) -L$(LIBDIR) $(LIBS)
	-$(RM) ./htadm
	$(LN_S) ./htadm_$(VD) ./htadm

cgiparse: CGIParse.o
	$(CC) -o ./cgiparse_$(VD) CGIParse.o $(LDFLAGS) -L$(LIBDIR) $(LIBS)
	-$(RM) ./cgiparse
	$(LN_S) ./cgiparse_$(VD) ./cgiparse

cgiutils: cgiutils.o HTSUtils.o
	$(CC) -o ./cgiutils_$(VD) cgiutils.o HTSUtils.o $(LDFLAGS) -L$(LIBDIR) $(LIBS)
	-$(RM) ./cgiutils
	$(LN_S) ./cgiutils_$(VD) ./cgiutils

htimage: HTImage.o
	$(CC) -o ./htimage_$(VD) HTImage.o $(LDFLAGS) -L$(LIBDIR) $(LIBS)
	-$(RM) ./htimage
	$(LN_S) ./htimage_$(VD) ./htimage

purify: $(OBJS)
	$(PURIFY) $(CC) -o ./httpd_$(VD) $(OBJS) $(LDFLAGS) -L$(LIBDIR) $(LIBS)
	-$(RM) ./httpd
	$(LN_S) ./httpd_$(VD) ./httpd

install: httpd htadm cgiparse cgiutils htimage
	$(INSTALL_PROGRAM) ./httpd_$(VD) $(BINDIR)/`echo httpd_$(VD) | $(SED) '$(transform)'`
	$(INSTALL_PROGRAM) ./htadm_$(VD) $(BINDIR)/`echo htadm_$(VD) | $(SED) '$(transform)'`
	$(INSTALL_PROGRAM) ./cgiparse_$(VD) $(BINDIR)/`echo cgiparse_$(VD) | $(SED) '$(transform)'`
	$(INSTALL_PROGRAM) ./cgiutils_$(VD) $(BINDIR)/`echo cgiutils_$(VD) | $(SED) '$(transform)'`
	$(INSTALL_PROGRAM) ./htimage_$(VD) $(BINDIR)/`echo htimage_$(VD) | $(SED) '$(transform)'`
	( cd $(BINDIR ; \
          $(LN_S) `echo httpd_$(VD) | $(SED) '$(transform)'` `echo httpd | $(SED) '$(transform)'` ; \
          $(LN_S) `echo htadm_$(VD) | $(SED) '$(transform)'` `echo htadm | $(SED) '$(transform)'` ; \
          $(LN_S) `echo cgiparse_$(VD) | $(SED) '$(transform)'` `echo cgiparse | $(SED) '$(transform)'` ; \
          $(LN_S) `echo cgiutils_$(VD) | $(SED) '$(transform)'` `echo cgiutils | $(SED) '$(transform)'` ; \
          $(LN_S) `echo htimage_$(VD) | $(SED) '$(transform)'` `echo htimage | $(SED) '$(transform)'` )

uninstall:
	-$(RM) -f $(BINDIR)/`echo httpd | $(SED) '$(transform)'`
	-$(RM) -f $(BINDIR)/`echo htadm | $(SED) '$(transform)'`
	-$(RM) -f $(BINDIR)/`echo cgiparse | $(SED) '$(transform)'`
	-$(RM) -f $(BINDIR)/`echo cgiutils | $(SED) '$(transform)'`
	-$(RM) -f $(BINDIR)/`echo htimage | $(SED) '$(transform)'`
	-$(RM) -f $(BINDIR)/`echo httpd_$(VD) | $(SED) '$(transform)'`
	-$(RM) -f $(BINDIR)/`echo htadm_$(VD) | $(SED) '$(transform)'`
	-$(RM) -f $(BINDIR)/`echo cgiparse_$(VD) | $(SED) '$(transform)'`
	-$(RM) -f $(BINDIR)/`echo cgiutils_$(VD) | $(SED) '$(transform)'`
	-$(RM) -f $(BINDIR)/`echo htimage_$(VD) | $(SED) '$(transform)'`

mostlyclean:
	-$(RM) $(OBJS) HTAdm.o CGIParse.o cgiutils.o HTImage.o

clean: mostlyclean
	-$(RM) -f ./httpd ./htadm ./cgiparse ./cgiutils ./htimage
	-$(RM) -f ./httpd_$(VD) ./htadm_$(VD) ./cgiparse_$(VD) ./cgiutils_$(VD) ./htimage_$(VD)

distclean: clean
	-$(RM) -f ./Makefile ./config.h ./config.cache ./config.status
	-$(RM) -f ./config.log

realclean: clean
	-$(RM) -f ./Makefile ./config.h
	( cd $(srcdir) ; $(CHMOD) +w $(HFILES) ; $(RM) -f $(HFILES) )

dist: $(HFILES) README.txt Copyright.txt
	-$(MKDIR) ./../WWWDaemon_$(VD)
	( cd ./.. ; $(LN_S) ./WWWDaemon_$(VD) ./Daemon )
	$(CP) -pr $(srcdir) ./../WWWDaemon_$(VD)
	$(CP) -p README.txt Copyright.txt ./../WWWDaemon_$(VD)
	$(CP) -pr $(VMS) ./../WWWDaemon_$(VD)
	( cd ./.. ; $(CP) -p ../Paper/www-server-guide.{ps,txt} WWWDaemon_$(VD) )
	( cd ./.. ; $(TAR) cvf ./../WWWDaemon_$(VD).tar ./WWWDaemon_$(VD) )
	$(COMPRESS) ./../../WWWDaemon_$(VD).tar
#	( cd $(srcdir) ; $(CVS) tag v`echo $(VD) | $(SED) 's?\.?/?'` )
	-$(RM) -rf ./../WWWDaemon_$(VD)
	echo Distribution of Daemon version $(VD) ready.

# Other targets
distribute: dist
inc: $(HFILES)

# Other dependencies
$(OBJS): sysdep.h

HTAAFile.o    : HTAAFile.h
HTAAProt.o    : HTAAFile.h   HTLex.h     HTAAProt.h  HTDaemon.h   HTConfig.h \
                HTLog.h      HTSUtils.h
HTAAServ.o    : HTAuth.h     HTACL.h     HTGroup.h   HTAAProt.h   HTAAServ.h \
                HTDaemon.h   HTConfig.h  HTLog.h
HTACL.o       : HTAAFile.h   HTGroup.h   HTACL.h
HTAdm.o       : HTPasswd.h
HTAuth.o      : HTPasswd.h   HTAuth.h    HTDaemon.h  HTConfig.h   HTLog.h
HTCache.o     : HTCache.h    HTConfig.h  HTDaemon.h  HTLog.h
HTCacheInfo.o : HTSUtils.h   HTCache.h
HTConfig.o    : HTAAProt.h   HTDaemon.h  HTConfig.h  HTLog.h
HTDaemon.o    : HTSUtils.h   HTAAServ.h  HTRequest.h HTAuth.h     HTConfig.h \
                HTDaemon.h   HTLog.h     HTCache.h   HTUserInit.h HTims.h
HTGroup.o     : HTLex.h      HTGroup.h   HTWild.h
HTLex.o       : HTLex.h
HTLoad.o      : HTRequest.h  HTDaemon.h
HTLog.o       : HTLog.h      HTGroup.h   HTDaemon.h  HTConfig.h   HTAuth.h
HTPasswd.o    : HTAAFile.h   HTPasswd.h
HTRFC931.o    : HTLog.h
HTRequest.o   : HTRequest.h  HTDaemon.h  HTConfig.h  HTAuth.h
HTRetrieve.o  : HTDaemon.h   HTScript.h  HTAAServ.h
HTSUtils.o    : HTSUtils.h   HTConfig.h
HTScript.o    : HTAuth.h     HTRequest.h HTDaemon.h  HTConfig.h   HTLog.h
HTUserInit.o  : HTUserInit.h
HTWild.o      : HTWild.h
HTgc.o        : HTCache.h    HTConfig.h  HTDaemon.h  HTLog.h
HTims.o       : HTims.h      HTSUtils.h  HTDaemon.h
cgiutils.o    : sysdep.h     HTSUtils.h
HTAAFile.h    : sysdep.h
HTAAProt.h    : sysdep.h
HTAAServ.h    : sysdep.h     HTAuth.h
HTACL.h       : sysdep.h     HTGroup.h
HTAuth.h      : sysdep.h     HTAAProt.h
HTCache.h     : sysdep.h
HTConfig.h    : sysdep.h     HTWild.h    HTAAProt.h
HTDaemon.h    : sysdep.h     HTSUtils.h  HTConfig.h  HTLog.h
HTGroup.h     : sysdep.h     HTWild.h
HTLex.h       : sysdep.h
HTLog.h       : sysdep.h
HTPasswd.h    : sysdep.h
HTRequest.h   : sysdep.h
HTSUtils.h    : sysdep.h
HTScript.h    : sysdep.h
HTUserInit.h  : sysdep.h
HTWild.h      : sysdep.h
HTims.h       : sysdep.h


# Other common tags, unused here:

TAGS:
info:
dvi:
check:
installcheck:
installdirs:

depend:

