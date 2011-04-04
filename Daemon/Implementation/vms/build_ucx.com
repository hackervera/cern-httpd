$ WRITE SYS$OUTPUT "Creating WWW Server for "UCX" on "VAX"." 
$ WRITE SYS$OUTPUT "=================================================" 
$ IF "''F$SEARCH("[--.VAX.UCX]*.*")'" .EQS. "" 	   THEN CREATE/DIR [--.VAX.UCX]
$ IF "''F$SEARCH("[--.ETC]*.*")'" .EQS. "" 	   THEN CREATE/DIR [--.ETC]
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTDAEMON.obj [-]HTDaemon.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTREQUEST.obj [-]HTRequest.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTRETRIEVE.obj [-]HTRetrieve.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTSCRIPT.obj [-]HTScript.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTLOAD.obj [-]HTLoad.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTCACHE.obj [-]HTCache.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTCACHEINFO.obj [-]HTCacheInfo.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTCONFIG.obj [-]HTConfig.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTWILD.obj [-]HTWild.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTSINIT.obj [-]HTSInit.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTSUTILS.obj [-]HTSUtils.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTIMS.obj [-]HTims.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTPASSWD.obj [-]HTPasswd.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTAUTH.obj [-]HTAuth.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTLEX.obj [-]HTLex.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTGROUP.obj [-]HTGroup.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTACL.obj [-]HTACL.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTAAPROT.obj [-]HTAAProt.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTAASERV.obj [-]HTAAServ.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTAAFILE.obj [-]HTAAFile.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTLOG.obj [-]HTLog.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTGC.obj [-]HTgc.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTUSERINIT.obj [-]HTUserInit.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTRFC931.obj [-]HTRFC931.c
$ link /exe=[--.VAX.UCX]httpd.exe [--.VAX.UCX]HTDaemon.obj, [--.VAX.UCX]HTRequest.obj, [--.VAX.UCX]HTRetrieve.obj, [--.VAX.UCX]HTScript.obj, [--.VAX.UCX]HTLoad.obj, 		[--.VAX.UCX]HTCache.obj, [--.VAX.UCX]HTCacheInfo.obj, [--.VAX.UCX]HTConfig.obj, [--.V-
AX.UCX]HTWild.obj, 		[--.VAX.UCX]HTSInit.obj, [--.VAX.UCX]HTSUtils.obj, [--.VAX.UCX]HTims.obj, 		[--.VAX.UCX]HTPasswd.obj, [--.VAX.UCX]HTAuth.obj, [--.VAX.UCX]HTLex.obj, [--.VAX.UCX]HTGroup.obj, [--.VAX.UCX]HTACL.obj, [--.VAX.UCX]HTAAProt.obj, 		[--.VA-
X.UCX]HTAAServ.obj, [--.VAX.UCX]HTAAFile.obj, [--.VAX.UCX]HTLog.obj, [--.VAX.UCX]HTgc.obj, [--.VAX.UCX]HTUserInit.obj, [--.VAX.UCX]HTRFC931.obj, [---.LIBRARY.VAX.UCX]wwwlib/lib, [---.LIBRARY.VAX.UCX]wwwlib.opt/opt
$ continue
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTADM.obj [-]HTAdm.c
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTEXTDUMMY.obj []HTextDummy.c
$ link /exe=[--.VAX.UCX]htadm.exe [--.VAX.UCX]HTAdm.obj, [--.VAX.UCX]HTPasswd.obj, [--.VAX.UCX]HTAAFile.obj,                 [--.VAX.UCX]HTextDummy.obj, [---.LIBRARY.VAX.UCX]wwwlib/lib, wwwlib.opt/opt
$ continue
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]HTIMAGE.obj [-]HTImage.c
$ link /exe=[--.VAX.UCX]htimage.exe [--.VAX.UCX]HTImage.obj,                 [--.VAX.UCX]HTextDummy.obj, [---.LIBRARY.VAX.UCX]wwwlib/lib, [---.LIBRARY.VAX.UCX]wwwlib.opt/opt
$ continue
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]CGIPARSE.obj [-]CGIParse.c
$ link /exe=[--.VAX.UCX]cgiparse.exe [--.VAX.UCX]CGIParse.obj,                 [--.VAX.UCX]HTextDummy.obj, [---.LIBRARY.VAX.UCX]wwwlib/lib, [---.LIBRARY.VAX.UCX]wwwlib.opt/opt
$ continue
$ cc /DEFINE=(VMS,DEBUG,RULES,ACCESS_AUTH,VD="""3.0pre6vms2""",UCX)/INC=([-],[],[---.Library.Implementation],[---.Library.Implementation.vms])/obj=[--.VAX.UCX]CGIUTILS.obj [-]cgiutils.c
$ link /exe=[--.VAX.UCX]cgiutils.exe [--.VAX.UCX]cgiutils.obj, [--.VAX.UCX]HTSUtils.obj,                 [--.VAX.UCX]HTextDummy.obj, [---.LIBRARY.VAX.UCX]wwwlib/lib, [---.LIBRARY.VAX.UCX]wwwlib.opt/opt
$ continue
$ copy []setup.com [--.VAX.UCX]setup.com
$ continue
