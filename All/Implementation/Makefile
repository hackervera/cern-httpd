#	Make all W3C distributed WWW products
#
#	(c) COPYRIGHT MIT 1995.
#	Please first read the full copyright statement in the file COPYRIGH.
#
#	The local products are built using machine-specific
#	makefiles where they exist; the X products use xmkmf
#	and soft links to the sources in the source directories.
#
#	This master Makefile contains the inter-product dependecies
#	needed for a first build. I checks that the needed subproducts exist,
#	it does not check they are internally up-to-date.
#
# Weirdities:
#	*** On sgi and decstation SHELL can be set to /bin/csh which not
#	only screws up sh-isms in makefiles, but seems to override a setting
#	of SHELL in the Makefile itself.
#       *** BSD unix didn't have .include in make for someone, so
#	     we cat makefiles together. ALso gets round screwed up
#		relative filenames on include in OSF1 and NeXT makes.
#	*** decstation/GNUmake: Doesn't like an "if [ ...] then ... ; fi"
#	because if the "if" fails, error code 1 is returned, which stops make.
#	Anyway, we put in "else echo OK ;" clauses for this.
#
# Pick up machine-specific bits:

WWW = ../..
WTMP = ../..

#	These are the subproducts:

LIBRARY = $(WTMP)/Library/$(WWW_MACH)/libwww.a
LINEMODE= $(WWW)/LineMode/$(WWW_MACH)/www
DAEMON  = $(WWW)/Daemon/$(WWW_MACH)/httpd
COMLINE = $(WWW)/ComLine/$(WWW_MACH)/w3c
MINISERV= $(WWW)/MiniServ/$(WWW_MACH)/webd
ROBOT   = $(WWW)/Robot/$(WWW_MACH)/webbot
PICS    = $(WWW)/Pics/$(WWW_MACH)/libpics.a

VIOLA = $(WWW)/Viola/$(WWW_MACH)/viola
XPA   = $(WWW)/XPA/$(WWW_MACH)/libXpa.a
XPM   = $(WWW)/XPM/$(WWW_MACH)/libXpm.a
MIDAS = $(WWW)/MidasWWW/$(WWW_MACH)/midaswww

# Tools sometimes not available with X:
XMKMF = $(WWW)/Tools/bin/xmkmf
LNXMAKE = $(WWW)/Tools/bin/lnxmake

#	Default target -- remove things you don't want
all :	$(LIBRARY) $(DAEMON)
	@echo BUILD complete!

library : $(LIBRARY)
	@echo BUILD complete!

linemode : $(LINEMODE)
	@echo BUILD complete!

miniserv : $(MINISERV)
	@echo BUILD complete!

comline : $(COMLINE)
	@echo BUILD complete!

robot : $(ROBOT)
	@echo BUILD complete!

pics : $(PICS)
	@echo BUILD complete!

daemon : $(DAEMON)
	@echo BUILD complete!

others : $(VIOLA) $(MIDAS) 
	@echo BUILD complete!

$(LIBRARY) :
	@echo
	@echo '------ Building W3C Reference Library ------'
	@echo
	@echo Object files of libwww will go to directory Library/$(WWW_MACH)
	@if [ ! -r $(WWW)/Library/$(WWW_MACH) ] ; \
	then	echo "	- creating"; \
		mkdir $(WWW)/Library/$(WWW_MACH); \
	else	echo "	- already exists"; \
	fi
	@echo
	@cat	$(WWW)/All/$(WWW_MACH)/Makefile.include \
		$(WWW)/Library/Implementation/Version.make \
		$(WWW)/Library/Implementation/CommonMakefile > \
		$(WWW)/Library/$(WWW_MACH)/Makefile;
	@(cd $(WWW)/Library/$(WWW_MACH); make)
	@echo
	@echo libwww compiled successfully and can be found in Library/$(WWW_MACH)
	@echo
	@echo "Have fun! If you have any problems with this software feel free to"
	@echo "contact <libwww@w3.org>. Online documentation is available via"
	@echo "the <URL:http://www.w3.org/pub/WWW/Library/>"

$(LINEMODE) : $(LIBRARY)
	@echo
	@echo '------ Building W3C Line Mode Browser ------'
	@echo
	@echo Object files and executable binary will go to directory LineMode/$(WWW_MACH)
	@if [ ! -r $(WWW)/LineMode/$(WWW_MACH) ] ; \
	then	echo "	- creating"; \
		mkdir $(WWW)/LineMode/$(WWW_MACH); \
	else	echo "	- already exists"; \
	fi
	@echo
	@cat	$(WWW)/All/$(WWW_MACH)/Makefile.include \
		$(WWW)/LineMode/Implementation/Version.make \
		$(WWW)/LineMode/Implementation/CommonMakefile > \
		$(WWW)/LineMode/$(WWW_MACH)/Makefile;
	@(cd $(WWW)/LineMode/$(WWW_MACH); make)
	@echo
	@echo www built successfully and can be found in LineMode/$(WWW_MACH)
	@echo
	@echo
	@echo "Have fun! If you have any problems with this software feel free to"
	@echo "contact <www-bug@w3.org>. Online documentation is available via"
	@echo "the <URL:http://www.w3.org/pub/WWW/LineMode/>"
	@echo

$(DAEMON) : $(LIBRARY)
	@echo
	@echo '------ Building W3C httpd, htadm, htimage, cgiparse, cgiutils ------'
	@echo
	@echo Object files and binaries will go to directory Daemon/$(WWW_MACH)
	@if [ ! -r $(WWW)/Daemon/$(WWW_MACH) ] ; \
	then	echo "	- creating"; \
		mkdir $(WWW)/Daemon/$(WWW_MACH); \
	else	echo "	- already exists"; \
	fi
	@echo
	@cat	$(WWW)/All/$(WWW_MACH)/Makefile.include \
		$(WWW)/Daemon/Implementation/Version.make \
		$(WWW)/Daemon/Implementation/CommonMakefile > \
		$(WWW)/Daemon/$(WWW_MACH)/Makefile;
	@(cd $(WWW)/Daemon/$(WWW_MACH); make)
	@echo
	@echo httpd, htadm, htimage, cgiparse and cgiutils built successfully
	@echo and can be found in directory Daemon/$(WWW_MACH).
	@echo
	@echo "Have fun! If you have any problems with this software feel free to"
	@echo "contact <httpd@w3.org>. Online documentation is available via"
	@echo "the <URL:http://www.w3.org/pub/WWW/Daemon/>"
	@echo

$(COMLINE) : $(LIBRARY)
	@echo
	@echo '------ Building W3C Command Line Tool ------'
	@echo
	@echo Object files and executable binary will go to directory ComLine/$(WWW_MACH)
	@if [ ! -r $(WWW)/ComLine/$(WWW_MACH) ] ; \
	then	echo "	- creating"; \
		mkdir $(WWW)/ComLine/$(WWW_MACH); \
	else	echo "	- already exists"; \
	fi
	@echo
	@cat	$(WWW)/All/$(WWW_MACH)/Makefile.include \
		$(WWW)/ComLine/Implementation/Version.make \
		$(WWW)/ComLine/Implementation/CommonMakefile > \
		$(WWW)/ComLine/$(WWW_MACH)/Makefile;
	@(cd $(WWW)/ComLine/$(WWW_MACH); make)
	@echo
	@echo w3c built successfully and can be found in ComLine/$(WWW_MACH)
	@echo
	@echo
	@echo "Have fun! If you have any problems with this software feel free to"
	@echo "contact <libwww@w3.org>. Online documentation is available via"
	@echo "the <URL:http://www.w3.org/pub/WWW/ComLine/>"
	@echo

$(MINISERV) : $(LIBRARY)
	@echo
	@echo '------ Building W3C Mini Server ------'
	@echo
	@echo Object files and executable binary will go to directory MiniServ/$(WWW_MACH)
	@if [ ! -r $(WWW)/MiniServ/$(WWW_MACH) ] ; \
	then	echo "	- creating"; \
		mkdir $(WWW)/MiniServ/$(WWW_MACH); \
	else	echo "	- already exists"; \
	fi
	@echo
	@cat	$(WWW)/All/$(WWW_MACH)/Makefile.include \
		$(WWW)/MiniServ/Implementation/Version.make \
		$(WWW)/MiniServ/Implementation/CommonMakefile > \
		$(WWW)/MiniServ/$(WWW_MACH)/Makefile;
	@(cd $(WWW)/MiniServ/$(WWW_MACH); make)
	@echo
	@echo webd built successfully and can be found in MiniServ/$(WWW_MACH)
	@echo
	@echo
	@echo "Have fun! If you have any problems with this software feel free to"
	@echo "contact <libwww@w3.org>. Online documentation is available via"
	@echo "the <URL:http://www.w3.org/pub/WWW/MiniServ/>"
	@echo

$(ROBOT) : $(LIBRARY)
	@echo
	@echo '------ Building W3C Mini Robot ------'
	@echo
	@echo Object files and executable binary will go to directory Robot/$(WWW_MACH)
	@if [ ! -r $(WWW)/Robot/$(WWW_MACH) ] ; \
	then	echo "	- creating"; \
		mkdir $(WWW)/Robot/$(WWW_MACH); \
	else	echo "	- already exists"; \
	fi
	@echo
	@cat	$(WWW)/All/$(WWW_MACH)/Makefile.include \
		$(WWW)/Robot/Implementation/Version.make \
		$(WWW)/Robot/Implementation/CommonMakefile > \
		$(WWW)/Robot/$(WWW_MACH)/Makefile;
	@(cd $(WWW)/Robot/$(WWW_MACH); make)
	@echo
	@echo webd built successfully and can be found in Robot/$(WWW_MACH)
	@echo
	@echo
	@echo "Have fun! If you have any problems with this software feel free to"
	@echo "contact <libwww@w3.org>. Online documentation is available via"
	@echo "the <URL:http://www.w3.org/pub/WWW/Robot/>"
	@echo

$(PICS) : $(LIBRARY)
	@echo
	@echo '------ Building W3C Pics Reference Library ------'
	@echo
	@echo Object files and executable binary will go to directory Pics/$(WWW_MACH)
	@if [ ! -r $(WWW)/Pics/$(WWW_MACH) ] ; \
	then	echo "	- creating"; \
		mkdir $(WWW)/Pics/$(WWW_MACH); \
	else	echo "	- already exists"; \
	fi
	@echo
	@cat	$(WWW)/All/$(WWW_MACH)/Makefile.include \
		$(WWW)/Pics/Implementation/Version.make \
		$(WWW)/Pics/Implementation/CommonMakefile > \
		$(WWW)/Pics/$(WWW_MACH)/Makefile;
	@(cd $(WWW)/Pics/$(WWW_MACH); make)
	@echo
	@echo libpics.a built successfully and can be found in Pics/$(WWW_MACH)
	@echo
	@echo
	@echo "Have fun! If you have any problems with this software feel free to"
	@echo "contact <libwww@w3.org>. Online documentation is available via"
	@echo "the <URL:http://www.w3.org/pub/WWW/Pics/> (or would be if it was"
	@echo "written yet.)"
	@echo

$(VIOLA) : $(XPA) $(XPM) $(LIBRARY)
	-mkdir $(WWW)/Viola/$(WWW_MACH)
	(cd $(WWW)/Viola/$(WWW_MACH); $(LNXMAKE))

$(XPM) :
	-mkdir $(WWW)/XPM/$(WWW_MACH)
	(cd $(WWW)/XPM/$(WWW_MACH); $(LNXMAKE))

$(XPA) :
	-mkdir $(WWW)/XPA/$(WWW_MACH)
	(cd $(WWW)/XPA/$(WWW_MACH); $(LNXMAKE))

$(MIDAS) :
	-mkdir $(WWW)/MidasWWW/$(WWW_MACH)
	(cd $(WWW)/MidasWWW/$(WWW_MACH); $(LNXMAKE))

.FAILED :
	@echo
	@echo "One or more of the modules required in order to build"
	@echo "this software could not be found. Please note that the"
	@echo "W3C Reference Library is part of all W3C applications"
	@echo "in this package. You can see whether you have this by"
	@echo "checking that you have a directory called WWW/Library"
	@echo
	@echo "You can get more information about the various modules"
	@echo "available in the README file."
	@echo
	@echo "If you have any problems with this software feel free to"
	@echo "contact <libwww@w3.org>. Online documentation is available via"
	@echo "the <URL:http://www.w3.org/pub/WWW/Distribution.html/>"
	@echo
