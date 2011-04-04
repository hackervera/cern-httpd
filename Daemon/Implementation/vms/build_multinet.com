$ WRITE SYS$OUTPUT "Creating WWW Server for "MULTINET" on "VAX"." 
$ WRITE SYS$OUTPUT "=================================================" 
$ IF "''F$SEARCH("[--.VAX.MULTINET]*.*")'" .EQS. "" 	   THEN CREATE/DIR [--.VAX.MULTINET]
$ IF "''F$SEARCH("[--.ETC]*.*")'" .EQS. "" 	   THEN CREATE/DIR [--.ETC]
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTDAEMON.obj [-]HTDaemon.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTREQUEST.obj [-]HTRequest.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTRETRIEVE.obj [-]HTRetrieve.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTSCRIPT.obj [-]HTScript.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTLOAD.obj [-]HTLoad.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTCACHE.obj [-]HTCache.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTCACHEINFO.obj [-]HTCacheInfo.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTCONFIG.obj [-]HTConfig.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTWILD.obj [-]HTWild.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTSINIT.obj [-]HTSInit.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTSUTILS.obj [-]HTSUtils.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTIMS.obj [-]HTims.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTPASSWD.obj [-]HTPasswd.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTAUTH.obj [-]HTAuth.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTLEX.obj [-]HTLex.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTGROUP.obj [-]HTGroup.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTACL.obj [-]HTACL.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTAAPROT.obj [-]HTAAProt.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTAASERV.obj [-]HTAAServ.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTAAFILE.obj [-]HTAAFile.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTLOG.obj [-]HTLog.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTGC.obj [-]HTgc.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTUSERINIT.obj [-]HTUserInit.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTRFC931.obj [-]HTRFC931.c
$ link /exe=[--.VAX.MULTINET]httpd.exe [--.VAX.MULTINET]HTDaemon.obj, [--.VAX.MULTINET]HTRequest.obj, [--.VAX.MULTINET]HTRetrieve.obj, [--.VAX.MULTINET]HTScript.obj, [--.VAX.MULTINET]HTLoad.obj, 		[--.VAX.MULTINET]HTCache.obj, [--.VAX.MULTINET]HTCacheI-
nfo.obj, [--.VAX.MULTINET]HTConfig.obj, [--.VAX.MULTINET]HTWild.obj, 		[--.VAX.MULTINET]HTSInit.obj, [--.VAX.MULTINET]HTSUtils.obj, [--.VAX.MULTINET]HTims.obj, 		[--.VAX.MULTINET]HTPasswd.obj, [--.VAX.MULTINET]HTAuth.obj, [--.VAX.MULTINET]HTLex.obj, [--
-.VAX.MULTINET]HTGroup.obj, [--.VAX.MULTINET]HTACL.obj, [--.VAX.MULTINET]HTAAProt.obj, 		[--.VAX.MULTINET]HTAAServ.obj, [--.VAX.MULTINET]HTAAFile.obj, [--.VAX.MULTINET]HTLog.obj, [--.VAX.MULTINET]HTgc.obj, [--.VAX.MULTINET]HTUserInit.obj, [--.VAX.MULTI-
NET]HTRFC931.obj, [---.LIBRARY.VAX.MULTINET]wwwlib/lib, [---.LIBRARY.VAX.MULTINET]wwwlib.opt/opt
$ continue
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTADM.obj [-]HTAdm.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTEXTDUMMY.obj []HTextDummy.c
$ link /exe=[--.VAX.MULTINET]htadm.exe [--.VAX.MULTINET]HTAdm.obj, [--.VAX.MULTINET]HTPasswd.obj, [--.VAX.MULTINET]HTAAFile.obj,                 [--.VAX.MULTINET]HTextDummy.obj, [---.LIBRARY.VAX.MULTINET]wwwlib/lib, wwwlib.opt/opt
$ continue
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]HTIMAGE.obj [-]HTImage.c
$ link /exe=[--.VAX.MULTINET]htimage.exe [--.VAX.MULTINET]HTImage.obj,                 [--.VAX.MULTINET]HTextDummy.obj, [---.LIBRARY.VAX.MULTINET]wwwlib/lib, [---.LIBRARY.VAX.MULTINET]wwwlib.opt/opt
$ continue
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]CGIPARSE.obj [-]CGIParse.c
$ link /exe=[--.VAX.MULTINET]cgiparse.exe [--.VAX.MULTINET]CGIParse.obj,                 [--.VAX.MULTINET]HTextDummy.obj, [---.LIBRARY.VAX.MULTINET]wwwlib/lib, [---.LIBRARY.VAX.MULTINET]wwwlib.opt/opt
$ continue
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",MULTINET)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.MULTINET]CGIUTILS.obj [-]cgiutils.c
$ link /exe=[--.VAX.MULTINET]cgiutils.exe [--.VAX.MULTINET]cgiutils.obj, [--.VAX.MULTINET]HTSUtils.obj,                 [--.VAX.MULTINET]HTextDummy.obj, [---.LIBRARY.VAX.MULTINET]wwwlib/lib, [---.LIBRARY.VAX.MULTINET]wwwlib.opt/opt
$ continue
$ copy []setup.com [--.VAX.MULTINET]setup.com
$ continue
