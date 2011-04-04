
   See the CERN server documentation for detailed descriptions of the
   v2.16 httpd and explanations of how to use it.
   
   To build the server, execute DESCRIP.MMS, BUILD_MULTINET.COM or
   BUILD_UCX.COM in [.LIBRARY.IMPLEMENTATION.VMS] to create the common
   object library (WWWLib.olb), then execute the corresponding file in
   [.DAEMON.IMPLEMENTATION.VMS] to build the server (HTTPD.exe),
   clickable image support (HTImage.exe), and authorization file utility
   (HTAdm.exe). If you do not yet have a W3 client, you can build the
   simple Line Mode Client (WWW.exe) with the MMS or appropriate command
   file in [.LINEMODE.IMPLEMENTATION.VMS] to check out the server.
   
   Start by testing the server as a detached process on port 8001 with
   TRACE implemented, after reading all the headers and comments in the
   foo_HTTP8001.COM files, HTTP8001.CONF, and SPAWNINIT.COM.
   
   Read the headers and comments of the other foo.COM and foo.PP files
   for information on what they do and how to use them.
   
   If the server checks out OK on port 8001, install it on port 80,
   either:
   
    1. as a detached process by using SUBMIT_HTTP80.COM,
       or
    2. under Inetd (MultNet's MULTINET_SERVER or UCX's AUX) with a system
       logical (HTTPD_CONFIG80) pointing to HTTP80.CONF, and after
       INSTALLing the image /SHAR/OPEN/HEAD to speed up initiation of
       multiple Inetd processes.
