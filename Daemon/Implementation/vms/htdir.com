$! v = 'f$verify(0)'
$!============================================================================
$! HTDIR.COM   (modification of Nik Zapantis' dir_html.com, fetched by F.M. on
$! ---------	26-Feb-1994 via http://info.phys.uvic.ca/public_files.html)
$!
$! F.Macrides - Modified to work with Lynx (no images) and called as htdir
$! (27-Feb-94)	(was dir_html).  Now uses lowercase for display of file and
$!		directory names, doesn't put extension (.dir) on directories,
$!		uppercases ".Z" or "_Z" (e.g., foo.tar_Z), and appends a
$!		[date, size] display for text and binary file anchors.
$!
$! F.Macrides - Added code for displaying gopherd _ABOUT files, and symbols
$! (28-Feb-94)	(showREADME and show_ABOUT) for making display versus listing
$!		of README or _ABOUT files optional.  Added exclude symbol for
$!		excluding files from the listing based on their extensions.
$!		Added code for excluding gopherd .links and lookaside files.
$!
$! F.Macrides - Fixed code for appending a [date, size] display so that it
$! (03-Mar-94)	deals correctly with an initially one-digit day field.
$!
$! F.Macrides - Changed to use file Creation date instead of Revision date
$! (05-Mar-94)	for the [date, size] display.  That's what DIR/DATE and ZIP
$!		use.  Move comment where indicated, below, if you want to
$!		restore display of Revision date.
$!
$! F.Macrides - Modified to work with symbols from v2.16-1betavms httpd, and
$! (01-Apr-94)	to return error messages to client rather than just exiting
$!		on errors.
$!
$!============================================================================
$! Generate hypertext menu from directory list
$!
$! calls and corresponding symbols for CERN v2.16vmsbeta httpd:
$! ------------------------------------------------------------
$!		http://node/htbin/htdir/p1
$!		    WWW_PATH_INFO = p1  Root directory to list (if mapped)
$!
$!		http://node/htbin/htdir/p1?p2
$!		    WWW_PATH_INFO = p1  Path for subdirectory to list
$!		    WWW_KEY_1     = p2  Root directory (to check if mapped)
$!
$! make sure to change the RULES_MAP routine and the symbol node 
$!	----	------	   ^^^^^^^^^--------		    ^^^^
$! Use only the mapped directories in the URLs as specified in the RULES file
$!
$! check the symbol binary for inclusion of all binary extensions you use 
$!		    ^^^^^^
$! check the symbol exclude for file types you want excluded from listings
$!		    ^^^^^^^
$! Created by Nik Zapantis (zapantis@phys.UVic.CA)
$! UVic, Physics & Astronomy
$! 15-Feb-1994
$!
$! Disclaimer:
$! This software is placed in the Public Domain and can be copied and
$! distributed free of charge for non-commercial use.
$! No guarantee whatsoever is provided by the author 
$! or the University of Victoria.
$! No liability whatsoever is accepted for any loss or damage
$! of any kind resulting from any defect or inaccuracy in this information or
$! code.
$!
$! set up some symbols
$ p1  = "''WWW_PATH_INFO'"
$ p2  = "''WWW_KEY_1'"
$ say = "write WWW_OUT"
$ set symbol/scope=(nolocal,noglobal)
$ debug = 0
$! use the following for debugging
$ if debug 
$  then 
$    open/write f1 HTTPD_Dir:tmp.lis
$    write f1 "the parameters passed are: p1=''p1' p2=''p2'"
$ endif
$ set proc/priv=(noall,tmpmbx,netmbx)
$ !text = "/.DAT/.TXT/.DOC/.MAN/.TEX/.PS/.EPS/.COM/.DIS/.HTML/.ISO/.TEXT/"
$ binary= "/.EXE/.DVI/.Z/.TAR_Z/.BCK/.BAK/.GIF/.TIFF/.XBM/.MPEG/.IMG/.ZIP/.OLB/"
$ exclude= "/.IDX/.SEL/"
$!***************************************************************************
$ node = "sci.wfeb.edu:8001"	! change this to your node (port is optional)
$ showREADME := "FALSE"		! change this to TRUE to display README files
$ show_ABOUT := "FALSE"		! change this to TRUE to display _ABOUT files
$!***************************************************************************
$ if p1 .eqs. "" .and. p2 .eqs. "" ! no arguments
$  then
$    say "Content-Type: text/html"
$    say ""
$    say "<BODY><H1>Error 400</H1>"
$    say "Invalid request.</BODY>"
$    exit
$ endif 
$ udir = p1
$ if p2 .eqs. "" then p2=p1
$ root = p2
$ !fix directories
$ if f$ext(0,1,udir) .nes. "/" then udir = "/"+udir
$ if f$ext(f$len(udir)-1,1,udir) .nes. "/" then udir = udir+"/"
$ if f$ext(0,1,root) .nes. "/" then root = "/"+root
$ if f$ext(f$len(root)-1,1,root) .nes. "/" then root = root+"/"
$ if debug then write f1 "udir = ''udir   root = ''root'"
$ GOSUB RULES_MAP	! make sure directory is allowed
$ ftpdir = f$edit(ftpdir,"COLLAPSE,LOWERCASE") ! just to make sure
$ ! convert ftpdir to httpdir
$ if debug then write f1 "ftpdir = ''ftpdir'"
$ say "Content-Type: text/html"
$ say ""
$ say "<HEAD>"
$ say "<TITLE>Information under: ''udir'</TITLE>"
$ say "</HEAD><BODY>"
$ say "<H1> HTTP Served DIRECTORY ''udir'</H1>"
$ ! If there is a gopherd _ABOUT file we can include that as plain text
$ if  f$search("''ftpdir'_ABOUT.",1).nes."" .and. show_ABOUT.eqs."TRUE"
$  then
$    readfile = ''ftpdir' + "_ABOUT."
$    say "<PRE>"
$    copy 'readfile sys$output
$    say "</PRE><br>"
$ ! Else if there is a README file we can include that as plain text
$  else
$    if  f$search("''ftpdir'README*.*",1).nes."" .and. showREADME.eqs."TRUE"
$     then
$       readfile = ''ftpdir' + "README*.*"
$       say "<PRE>"
$       copy 'readfile sys$output
$       say "</PRE><br>"
$    endif
$ endif
$!
$! Now generate a list of links to files
$ say "<DIR>"
$ say "<A HREF=""http://''node'/"">Return to our HomePage</A><br>" 
$ if p1 .nes. p2	! this is a subdirectory
$  then 
$   GOSUB FIND_PARENT 
$   say "<A HREF=""/htbin/htdir''parent'?''root'"">Parent directory</A><br>"
$ endif ! if parent
$!
$ loop:
$ file = f$search("''ftpdir'*.*",2)
$ if file .eqs. "" then goto end
$ ! DON'T list the README*.* file if we displayed it
$ if f$locate("README",f$edit(file,"COLLAPSE,UPCASE")) .ne. f$len(file) -
     .and. showREADME .eqs. "TRUE" then goto loop
$ ! DON'T list the VMS Gopher Server's .links, or _lookaside files
$ ! but DO list a gopherd _ABOUT file if it wasn't displayed
$ firstchar := 'f$extract(0,1,f$parse(file,,,"NAME"))'
$ if firstchar .eqs. "_" .or. firstchar .eqs. ""
$  then
$    if f$parse(file,,,"NAME") .nes. "_ABOUT" then goto loop
$  else
$    if show_ABOUT .eqs. "TRUE" then goto loop
$ endif
$ ! DON'T list files with an exclude TYPE
$ if f$locate(f$parse(file,,,"TYPE"),exclude) .ne. f$len(exclude) .and. -
	f$parse(file,,,"TYPE") .nes. "." then goto loop
$ ! OK, here we go with a listing...
$ if f$parse(file,,,"TYPE") .eqs. ".DIR"
$  then
$    filetype := "DIR"		! is a directory file 
$  else				! is a text or binary file
$    if f$locate(f$parse(file,,,"TYPE"),binary) .eq. f$len(binary) .or. -
	f$parse(file,,,"TYPE") .eqs. "."
$     then
$       filetype := "TEXT"	! assume a text file 
$     else
$       filetype := "BIN"	! is a binary file
$    endif ! binary or text file
$ !  get date and size info for the text or binary file anchors
$ !  uses Creation date; move comment if you prefer Revision date
$!   filedate = f$edit(f$file(file,"RDT"),"TRIM")	! Revision date
$    filedate = f$edit(f$file(file,"CDT"),"TRIM")	! Creation date
$    if f$locate("-",filedate) .eq. 1 then filedate = "0''filedate'"
$    date = f$extract(0,f$locate(" ",filedate),filedate)
$    size = f$file(file,"ALQ")/2
$    dt := "[''date', ''size'KB]"
$ endif ! directory file
$ ! strip off device:[directory] fields
$ file = f$parse(file,,,"name")+f$parse(file,,,"type")
$ ! make file or directory name lowercase
$ file = f$edit(file,"LOWERCASE")
$ ! except terminal ".Z" or "_Z"
$ if f$locate("z",file) .eq. f$len(file)-1 .and. -
     (f$locate(".",file) .eq. f$len(file)-2 .or. -
      f$locate("_",file) .eq. f$len(file)-2) then -
     file = "''f$extract(0,f$len(file)-1,file)'"+"Z"
$ ! check file type and produce appropriate icon
$ ! special case of a directory
$ ! URL should call this routine again
$ if filetype .eqs. "DIR"
$   then ! directory file 
$    newdir="''udir'"+f$edit(f$parse(file,,,"name"),"COLLAPSE,LOWERCASE")+ "/" 
$ !  trim off ".dir"
$    file = f$extract(0,f$locate(".dir",file),file)
$ say "( DIR) <A HREF=""/htbin/htdir''newdir'?''root'"">''file'</A><br>"
$   else ! text or binary
$    if filetype .eqs. "TEXT"
$     then
$ say "(FILE) <A HREF=""http://''node'''udir'''file'"">''file'</A> ''dt'<br>"
$     else ! binary file
$ say "( BIN) <A HREF=""http://''node'''udir'''file'"">''file'</A> ''dt'<br>"
$    endif ! text or binary
$ endif ! check file type
$ !
$ goto loop 
$ end:
$ say "</DIR>
$ say "</BODY>"
$ set proc/priv=noall
$ if debug
$   then
$    write f1 "Final ftpdir = ''ftpdir'"
$    close f1
$ endif
$ exit
$ !**************** FIND_PARENT SUBROUTINE *********************************
$ ! find the parent of a given directory
$ !
$ FIND_PARENT: 
$ thisdir=udir
$ count =0
$ parent= ""
$ find_loop:
$ if f$element('count,"/",thisdir) .nes. "/"  
$  then
$    parent = parent + f$element(count,"/",thisdir)+"/"
$    count = count+1
$    goto find_loop
$  else
$    parent = parent-"//"-f$ele(count-2,"/",thisdir)
$    parent = f$edit(parent,"COLLAPSE,LOWERCASE")
$ endif
$ if f$len(parent) .le. f$len(root) then parent = root
$ !say "Parent Directory is ''parent'"
$ RETURN   
$!************************ END FIND_PARENT *********************************
$!************************ SUBROUTINE RULES_MAP *******************************
$!
$ RULES_MAP:
$! check for httpd rules mapping eg. /ftp/ maps to /d1/www/ftp/
$! and reverse it i.e. /d1/www/ftp/ --> ftp/
$!****************** change these rules and put your own **********************
$!rule# = "device:[directory]=/foo/"	! format for rules in this script
$!     also, put a map and pass for each /foo/* in the httpd rule file
$!                 ^^^     ^^^^                        ^^^^^
$ rule1 = "HTTPD_Dir:=/etc/"			! For the Port 8001 test
$!rule2 = "WWW_Root:[000000]=/www/"		! For the httpd data tree
$!rule3 = "device:[directory]=/ftp/"		! For an aFTP data tree
$!rule4 = "gopher_root:[directory]=/gopher/"	! For VMS Gopher Server
$!*****************************************************************************
$ vmsroot = ""
$ ftpdir = ""
$ count=1
$ loop_rules:
$ if f$type(rule'count) .eqs. "" ! no rules match
$  then
$    say "Content-Type: text/html"
$    say ""
$    say "<BODY><H1>Error 403</H1>"
$    say "Forbidden -- by rule</BODY>"
$    exit
$ endif
$ if f$ext(0,f$len(f$ele(1,"=",rule'count)),udir) .eqs. f$ele(1,"=",rule'count)
$   then
$   ! udir is ok
$    if f$ele(1,"=",rule'count) .eqs. udir 
$     then
$       ftpdir=f$ele(0,"=",rule'count)
$     else
$       vmsroot = f$ele(0,"=",rule'count)-"]"+"."+(udir-f$ele(1,"=",rule'count))
$       GOSUB VMSIFY
$    endif
$    return
$   else
$    count=count+1
$    goto loop_rules
$ endif
$!************************* END SUBROUTINE RULES_MAP ************************ 
$!
$!************************ SUBROUTINE VMSIFY ******************************
$!
$ VMSIFY:
$! convert all '/', to '.'
$ count=0
$ ftpdir = ""
$ ! now fix all subdirectories
$ loop_vms:
$ subdir= f$ele(count,"/",vmsroot)
$ if subdir .nes. ""
$   then
$     ftpdir=ftpdir+subdir+"."
$     count=count+1
$     goto loop_vms
$   else
$     ftpdir = ftpdir + "]" - ".]" + "]"  ! trust me
$     return
$ endif
$!************************ END SUBROUTINE VMSIFY *****************************
$!
