#	Make basic WWW distribution
#
#  See the README and the documentation on the web.
#  When you have done BUILD you will have www so you will be able to
#  read the documentation online.
#
all :
	./BUILD

purify:
	(cd Daemon/sun4; make purify)

clean:
	rm -f *[~#] */*[~#] */*/*[~#]
	rm -f *.o */*.o */*/*.o
	rm -f *.old */*.old */*/*.old

clobber: clean
	rm -f Library/[a-z0-9]*/libwww.a
	rm -f LineMode/[a-z0-9]*/www*
	rm -f Daemon/[a-z0-9]*/httpd*
	rm -f Daemon/[a-z0-9]*/htimage
	rm -f Daemon/[a-z0-9]*/htadm
	rm -f Daemon/[a-z0-9]*/cgiparse
	rm -f Daemon/[a-z0-9]*/cgiutils

