/*                                                      System dependencies in the W3 library
                                   SYSTEM DEPENDENCIES
                                             
   System-system differences for TCP include files and macros. This file includes for each
   system the files necessary for network and file I/O.  Part of libwww.
   
  AUTHORS
  
  TBL                     Tim Berners-Lee, W3 project, CERN, <timbl@info.cern.ch>
                         
  EvA                     Eelco van Asperen <evas@cs.few.eur.nl>
                         
  MA                      Marc Andreesen NCSA
                         
  MD                      Mark Donszelmann <duns@vxcern.cern.ch>
                         
  AT                      Aleksandar Totic <atotic@ncsa.uiuc.edu>
                         
  SCW                     Susan C. Weber <sweber@kyle.eitech.com>
                         
  HF                      Henrik Frystyk, <frystyk@dxcern.cern.ch>
                         
  HISTORY:
  
  22 Feb 91               Written (TBL) as part of the WWW library.
                         
  16 Jan 92               PC code from (EvA)
                         
  22 Apr 93               Merged diffs bits from xmosaic release
                         
  29 Apr 93               Windows/NT code from (SCW)
                         
  29 Sep 93               VMS fixes (MD)
                         
 */

#ifndef TCP_H
#define TCP_H
/*

Default values

   These values may be reset and altered by system-specific sections later on. There is
   also a bunch of defaults at the end and a section for ordinary BSD unix versions.
   
 */
/* Default values of those: */
#define NETCLOSE close      /* Routine to close a TCP-IP socket         */
#define NETREAD  read       /* Routine to read from a TCP-IP socket     */
#define NETWRITE write      /* Routine to write to a TCP-IP socket      */

/* Unless stated otherwise, */
#define SELECT                  /* Can handle >1 channel.               */
#define GOT_SYSTEM              /* Can call shell with string           */

#ifdef unix
#define GOT_PIPE
#endif
#ifdef VM
#define GOT_PIPE                /* Of sorts */
#endif

#ifdef DECNET
typedef struct sockaddr_dn SockA;  /* See netdnet/dn.h or custom vms.h */
#else /* Internet */
typedef struct sockaddr_in SockA;  /* See netinet/in.h */
#endif
/*

                                 PLATFORM SPECIFIC STUFF
                                             
   Information below this line is general for most platforms.
   
   See also General Stuff
   
Macintosh - Think-C

   Think-C is one development environment on the Mac.
   
   We recommend that you compile with 4-byte ints to be compatible with MPW C. We used Tom
   Milligan's s_socket library which was written for 4 byte int, and the MacTCP library
   assumes 4-byte int.
   
 */

#ifdef THINK_C
#undef GOT_SYSTEM
#define DEBUG                   /* Can't put it on the CC command line  */
#define NO_UNIX_IO              /* getuid() missing                     */
#define NO_GETPID               /* getpid() does not exist              */
#define NO_GETWD                /* getwd() does not exist               */

#undef NETCLOSE             /* Routine to close a TCP-IP socket         */
#undef NETREAD              /* Routine to read from a TCP-IP socket     */
#undef NETWRITE             /* Routine to write to a TCP-IP socket      */
#define NETCLOSE s_close    /* Routine to close a TCP-IP socket         */
#define NETREAD  s_read     /* Routine to read from a TCP-IP socket     */
#define NETWRITE s_write    /* Routine to write to a TCP-IP socket      */

#define bind s_bind         /* Funny names presumably to prevent clashes */
#define connect s_connect
#define accept s_accept
#define listen s_listen
#define socket s_socket
#define getsockname s_getsockname

/* The function prototype checking is better than the include files */

extern s_close(int s);
extern s_read(int s, char *buffer, int buflen);
extern s_write(int s, const char *buffer, int buflen);

extern bind(int s, struct sockaddr *name, int namelen);
extern accept(int s, struct sockaddr *addr, int *addrlen);
extern listen(int s, int qlen);
extern connect(int s, struct sockaddr *addr, int addrlen);

extern s_socket(int domain, int type, int protocol);
extern s_getsockname(int s, struct sockaddr *name, int *namelen);
extern struct hostent *gethostent(const char * name);
extern unsigned long inet_addr(const char * name);
#endif /* THINK_C */
/*

Macintosh - MPW

   MPW is one development environment on the Mac.
   
   This entry was created by Aleksandar Totic (atotic@ncsa.uiuc.edu) this file is
   compatible with sockets package released by NCSA.  One major conflict is that this
   library redefines write/read/etc as macros.  In some of HTML code these macros get
   executed when they should not be. Such files should define NO_SOCKET_DEFS on top. This
   is a temporary hack.
   
 */

#ifdef applec                   /* MPW  */
#undef GOT_SYSTEM
#define DEBUG                   /* Can't put it on the CC command line */
#define NO_UNIX_IO              /* getuid() missing */
#define NO_GETPID               /* getpid() does not exist */
#define NO_GETWD                /* getwd() does not exist */

#undef NETCLOSE             /* Routine to close a TCP-IP socket */
#undef NETREAD              /* Routine to read from a TCP-IP socket */
#undef NETWRITE             /* Routine to write to a TCP-IP socket */
#define NETCLOSE s_close    /* Routine to close a TCP-IP socket */
#define NETREAD  s_read     /* Routine to read from a TCP-IP socket */
#define NETWRITE s_write    /* Routine to write to a TCP-IP socket */
#define _ANSI_SOURCE
#define GUI
#define LINEFEED 10
#define ANON_FTP_HOSTNAME
#ifndef NO_SOCKET_DEFS
#include <MacSockDefs.h>
#endif /* NO_SOCKET_DEFS */

#include <socket.ext.h>
#include <string.h>

#endif /* MPW */

#ifndef STDIO_H
#include <stdio.h>
#define STDIO_H
#endif
/*

Big Blue - the world of incompatibility

   IBM RS600On the IBM RS-6000, AIX is almost Unix.
   
 */

#ifdef _AIX
#define AIX
#endif
#ifdef AIX
#define unix
#endif

/*    AIX 3.2
**    -------
*/

#ifdef _IBMR2
#define USE_DIRENT              /* sys V style directory open */
#endif
/*

IBM VM-CMS, VM-XA Mainframes

   MVS is compiled as for VM. MVS has no unix-style I/O.  The command line compile options
   seem to come across in lower case.
   
 */

#ifdef mvs
#define MVS
#endif

#ifdef MVS
#define VM
#endif

#ifdef NEWLIB
#pragma linkage(newlib,OS)      /* Enables recursive NEWLIB */
#endif

/*      VM doesn't have a built-in predefined token, so we cheat: */
#ifndef VM
#include <string.h>             /* For bzero etc - not  VM */
#endif

/*      Note:   All include file names must have 8 chars max (+".h")
**
**      Under VM, compile with "(DEF=VM,SHORT_NAMES,DEBUG)"
**
**      Under MVS, compile with "NOMAR DEF(MVS)" to get rid of 72 char margin
**        System include files TCPIP and COMMMAC neeed line number removal(!)
*/

#ifdef VM                       /* or MVS -- see above. */
#define NOT_ASCII               /* char type is not ASCII */
#define NO_UNIX_IO              /* Unix I/O routines are not supported */
#define NO_GETPID               /* getpid() does not exist */
#define NO_GETWD                /* getwd() does not exist */
#ifndef SHORT_NAMES
#define SHORT_NAMES             /* 8 character uniqueness for globals */
#endif
#include <manifest.h>
#include <bsdtypes.h>
#include <stdefs.h>
#include <socket.h>
#include <in.h>
#include <inet.h>
#include <netdb.h>
#include <errno.h>          /* independent */
extern char asciitoebcdic[], ebcdictoascii[];
#define TOASCII(c)   (c=='\n' ?  10  : ebcdictoascii[c])
#define FROMASCII(c) (c== 10  ? '\n' : asciitoebcdic[c])
#include <bsdtime.h>
#include <time.h>
#include <string.h>
#define INCLUDES_DONE
#define TCP_INCLUDES_DONE
#endif
/*

IBM-PC running MS-DOS with SunNFS for TCP/IP

   This code thanks to Eelco van Asperen <evas@cs.few.eur.nl>
   
 */

#ifdef PCNFS
#include <sys/types.h>
#include <string.h>
#include <errno.h>          /* independent */
#include <sys/time.h>       /* independent */
#include <sys/stat.h>
#include <fcntl.h>          /* In place of sys/param and sys/file */
#define INCLUDES_DONE
#define FD_SET(fd,pmask) (*(unsigned*)(pmask)) |=  (1<<(fd))
#define FD_CLR(fd,pmask) (*(unsigned*)(pmask)) &= ~(1<<(fd))
#define FD_ZERO(pmask)   (*(unsigned*)(pmask))=0
#define FD_ISSET(fd,pmask) (*(unsigned*)(pmask) & (1<<(fd)))
#define NO_GROUPS
#endif  /* PCNFS */
/*

IBM-PC running Windows NT

   These parameters providede by  Susan C. Weber <sweber@kyle.eitech.com>.
   
 */

#ifdef _WINDOWS
#include "fcntl.h"                      /* For HTFile.c */
#include "sys\types.h"                  /* For HTFile.c */
#include "sys\stat.h"                   /* For HTFile.c */

#undef NETREAD
#undef NETWRITE
#undef NETCLOSE
#define NETREAD(s,b,l)  ((s)>10 ? recv((s),(b),(l),0) : read((s),(b),(l)))
#define NETWRITE(s,b,l) ((s)>10 ? send((s),(b),(l),0) : write((s),(b),(l)))
#define NETCLOSE(s)     ((s)>10 ? closesocket(s) : close(s))
#include <io.h>
#include <string.h>
#include <process.h>
#include <time.h>
#include <direct.h>
#include <stdio.h>
#include <winsock.h>
typedef struct sockaddr_in SockA;  /* See netinet/in.h */
#define INCLUDES_DONE
#define TCP_INCLUDES_DONE
#endif  /* WINDOWS */
/*

VAX/VMS

   Under VMS, there are many versions of TCP-IP. Define one if you do not use Digital's
   UCX product:
   
  UCX                     DEC's "Ultrix connection" (default)
                         
  WIN_TCP                 From Wollongong, now GEC software.
                         
  MULTINET                From SRI, now from TGV Inv.
                         
  DECNET                  Cern's TCP socket emulation over DECnet
                         
   The last three do not interfere with the unix i/o library, and so they need special
   calls to read, write and close sockets. In these cases the socket number is a VMS
   channel number, so we make the @@@ HORRIBLE @@@ assumption that a channel number will
   be greater than 10 but a unix file descriptor less than 10.  It works.
   
 */

#ifdef VMS
#define CACHE_FILE_PREFIX  "SYS$LOGIN:Z_"

#ifdef WIN_TCP
#undef NETREAD
#undef NETWRITE
#undef NETCLOSE
#define NETREAD(s,b,l)  ((s)>10 ? netread((s),(b),(l)) : read((s),(b),(l)))
#define NETWRITE(s,b,l) ((s)>10 ? netwrite((s),(b),(l)) : write((s),(b),(l)))
#define NETCLOSE(s)     ((s)>10 ? netclose(s) : close(s))
#endif /* WIN_TCP */

#ifdef MULTINET
#undef NETCLOSE
#undef NETREAD
#undef NETWRITE
#define NETREAD(s,b,l)  ((s)>10 ? socket_read((s),(b),(l)) : read((s),(b),(l)))
#define NETWRITE(s,b,l) ((s)>10 ? socket_write((s),(b),(l)) : \
                                write((s),(b),(l)))
#define NETCLOSE(s)     ((s)>10 ? socket_close(s) : close(s))
#endif /* MULTINET */

#ifdef DECNET
#undef SELECT  /* not supported */
#undef NETREAD
#undef NETWRITE
#undef NETCLOSE
#define NETREAD(s,b,l)  ((s)>10 ? recv((s),(b),(l),0) : read((s),(b),(l)))
#define NETWRITE(s,b,l) ((s)>10 ? send((s),(b),(l),0) : write((s),(b),(l)))
#define NETCLOSE(s)     ((s)>10 ? socket_close(s) : close(s))
#endif /* Decnet */

/*      Certainly this works for UCX and Multinet; not tried for Wollongong
*/
#ifdef MULTINET
#include <time.h>
#ifdef __TIME_T
#define __TYPES
#define __TYPES_LOADED
#endif /* __TIME_T */
#include "multinet_root:[multinet.include.sys]types.h"
#include "multinet_root:[multinet.include]errno.h"
#ifdef __TYPES
#define __TIME_T
#endif /* __TYPE */
#ifdef __TIME_LOADED
#define __TIME
#endif /* __TIME_LOADED */
#include "multinet_root:[multinet.include.sys]time.h"
#else /* not MULTINET */
#include <types.h>
#include <errno.h>
#include <time.h>
#endif /* not MULTINET */

#include string

#ifndef STDIO_H
#include <stdio>
#define STDIO_H
#endif

#include file

#ifndef DECNET  /* Why is it used at all ? Types conflict with "types.h" */
#include unixio
#endif

#define INCLUDES_DONE

#ifdef MULTINET  /* Include from standard Multinet directories */
#include "multinet_root:[multinet.include.sys]socket.h"
#ifdef __TIME_LOADED  /* defined by sys$library:time.h */
#define __TIME  /* to avoid double definitions in next file */
#endif
#include "multinet_root:[multinet.include.netinet]in.h"
#include "multinet_root:[multinet.include.arpa]inet.h"
#include "multinet_root:[multinet.include]netdb.h"
#include "multinet_root:[multinet.include.sys]ioctl.h"

#else  /* not multinet */
#ifdef DECNET
#include "types.h"  /* for socket.h */
#include "socket.h"
#include "dn"
#include "dnetdb"
/* #include "vms.h" */

#else /* UCX or WIN */
#ifdef CADDR_T
#define __CADDR_T
#endif /* problem with xlib.h inclusion */
#include <socket.h>
#include <in.h>
#include <inet.h>
#include <netdb.h>
#include <ucx$inetdef.h>

#endif  /* not DECNET */
#endif  /* of Multinet or other TCP includes */

#define TCP_INCLUDES_DONE
/*

   On VMS directory browsing is available through a separate copy of dirent.c. The
   definition of R_OK seem to be missing from the system include files...
   
 */

#define USE_DIRENT
#define GOT_READ_DIR 1
#include "dirent.h"
#define STRUCT_DIRENT struct dirent
#define R_OK 4
/*

   On VMS machines, the linker needs to be told to put global data sections into a data
   segment using these storage classes. (MarkDonszelmann)
   
 */

#ifdef VAXC
#define GLOBALDEF globaldef
#define GLOBALREF globalref
#endif /*  VAXC */
#endif  /* vms */
/*

   On non-VMS machines, the GLOBALDEF and GLOBALREF storage types default to normal C
   storage types.
   
 */

#ifndef GLOBALREF
#define GLOBALDEF
#define GLOBALREF extern
#endif
/*

   On non-VMS machines HTStat should be stat...On VMS machines HTStat is a function that
   converts directories and devices so that you can stat them.
   
 */

#ifdef VMS
#define HTLstat HTStat
#else /* non VMS */
#define HTStat stat
#define HTLstat lstat
#endif /* non VMS */
/*

SCO ODT unix version

   (by Brian King)
   
 */

#ifdef sco
#include <grp.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <time.h>
#define USE_DIRENT
#define NEED_INITGROUPS
#endif
/*

MIPS unix

   Mips hack (bsd4.3/sysV mixture...)
   
 */

#ifdef Mips     /* Bruker */
extern int errno;
typedef mode_t          int;
#define S_ENFMT         S_ISGID         /* record locking enforcement flag */
#define S_ISCHR(m)      (m & S_IFCHR)
#define S_ISBLK(m)      (m & S_IFBLK)
#define WEXITSTATUS(s)  (((s).w_status >> 8) & 0377)
#define NO_STRFTIME
/*

  FILE PERMISSIONS
  
 */

#ifndef S_IRWXU
#define S_IRWXU 0000700
#define S_IRWXG 0000070
#define S_IRWXO 0000007
#define S_IRUSR 0000400
#define S_IWUSR 0000200
#define S_IXUSR 0000100
#define S_IRGRP 0000040
#define S_IWGRP 0000020
#define S_IXGRP 0000010
#define S_IROTH 0000004
#define S_IWOTH 0000002
#define S_IXOTH 0000001
#endif /* S_IRWXU */
#endif /* Mips */
/*

Solaris

   Includes Linux (thanks to Rainer Klute) and other SVR5 based systems
   
 */

#if defined(__svr4__) || defined(_POSIX_SOURCE) || defined(__hpux)

#ifdef UTS4                                   /* UTS wants sys/types.h first */
#include <sys/types.h>
#endif

#include <unistd.h>

#ifdef UTS4
#include <sys/fcntl.h>
#define POSIXWAIT
#endif

#ifdef AIX                                                     /* Apple Unix */
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
/*

   This is for NCR 3000 and Pyramid that also are SVR4 systems. Thanks to Alex Podlecki,
   <a.podlecki@att.com>
   
 */

#ifndef NGROUPS_MAX
#include <limits.h>
#endif

#endif /* Solaris and SVR5 */
/*

UTS 2.1 (BSD-like)

 */

#ifdef UTS2
#include <time.h>
#include <fcntl.h>

#define NO_STRFTIME
#define WEXITSTATUS(x)  ((int)((x).w_T.w_Retcode))

#undef POSIXWAIT
#endif /* UTS2 */
/*

OSF/1

 */

#ifdef __osf__
#define USE_DIRENT
#endif /* OSF1 AXP */
/*

ISC 3.0

   by Lauren Weinstein <lauren@vortex.com>.
   
 */


#ifdef ISC3                     /* Lauren */
#define USE_DIRENT
#define GOT_READ_DIR 1
#include <sys/ipc.h>
#include <sys/dirent.h>
#define direct dirent
#include <sys/unistd.h>
#define d_namlen d_reclen
#include <sys/limits.h>

#define SIGSTP
#define POSIXWAIT
#include <sys/types.h>
#include <sys/wait.h>
#include <net/errno.h>

#define _SYSV3
#include <time.h>

#include <sys/fcntl.h>
#define S_ISDIR(m)      (((m) & 0170000) == 0040000)
#define S_ISCHR(m)      (((m) & 0170000) == 0020000)
#define S_ISBLK(m)      (((m) & 0170000) == 0060000)
#define S_ISREG(m)      (((m) & 0170000) == 0100000)
#define S_ISFIFO(m)     (((m) & 0170000) == 0010000)
#define S_ISLNK(m)      (((m) & 0170000) == 0120000)
#endif  /* ISC 3.0 */
/*

SOCKS

   SOCKS modifications by Ian Dunkin <imd1707@ggr.co.uk>.
   
 */

#if defined(SOCKS) && !defined(RULE_FILE)
#define connect Rconnect
#define accept  Raccept
#define getsockname Rgetsockname
#define bind Rbind
#define listen Rlisten
#endif
/*

NeXT

 */

#ifdef NeXT
#include "sys/types.h"
#include "sys/stat.h"

typedef unsigned short mode_t;

#ifndef S_ISDIR
#define S_ISDIR(m)     (m & S_IFDIR)
#define S_ISREG(m)     (m & S_IFREG)
#define S_ISCHR(m)     (m & S_IFCHR)
#define S_ISBLK(m)     (m & S_IFBLK)
#define S_ISLNK(m)     (m & S_IFLNK)
#define S_ISSOCK(m)    (m & S_IFSOCK)
#define S_ISFIFO(m)    (NO)
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(s) (((s).w_status >> 8) & 0377)
#endif
/*

  FILE PERMISSIONS FOR NEXT
  
 */

#ifndef S_IRWXU
#define S_IRWXU 0000700
#define S_IRWXG 0000070
#define S_IRWXO 0000007
#define S_IRUSR 0000400
#define S_IWUSR 0000200
#define S_IXUSR 0000100
#define S_IRGRP 0000040
#define S_IWGRP 0000020
#define S_IXGRP 0000010
#define S_IROTH 0000004
#define S_IWOTH 0000002
#define S_IXOTH 0000001
#endif /* S_IRWXU */
#endif /* NeXT */
/*

A/UX Apple UNIX

                 Kelly King, bhutab@apple.com and Jim Jagielski, jim@jagubox.gsfc.nasa.gov
                                                                                          
 */

#ifdef _AUX
#include <time.h>
#define WEXITSTATUS(s) (((s).w_status >> 8) & 0377)
#endif
/*

Regular BSD unix versions

   These are a default unix where not already defined specifically.
   
 */

#ifndef INCLUDES_DONE

#include <sys/types.h>
#include <string.h>

#include <errno.h>          /* independent */
#include <sys/time.h>       /* independent */
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/file.h>       /* For open() etc */
#define INCLUDES_DONE
#endif  /* Normal includes */
/*

  DIRECTORY READING STUFF - BSD OR SYS V
  
 */

#ifdef unix                       /* if this is to compile on a UNIX machine */
#define GOT_READ_DIR 1       /* if directory reading functions are available */

#ifndef NeXT
#define USE_DIRENT                /* Try this all the time, Henrik May 29 94 */
#endif
#ifdef USE_DIRENT                                           /* sys v version */
#include <dirent.h>
#define STRUCT_DIRENT struct dirent
#else
#include <sys/dir.h>
#define STRUCT_DIRENT struct direct
#endif

#if defined(sun) && defined(__svr4__)
#include <sys/fcntl.h>
#include <limits.h>
#endif
#ifndef FNDELAY
#define FNDELAY         O_NDELAY
#endif
#endif
/*

                                      GENERAL STUFF
                                             
   Information below this line is general for most platforms.
   
   See also Platform Specific Stuff
   
  INCLUDE FILES FOR TCP
  
 */

#ifndef TCP_INCLUDES_DONE
#include <sys/socket.h>
#include <netinet/in.h>

/* #ifndef __hpux */ /* this may or may not be good -marc -- according
                        to somebody, it wasn't a good thing...(?) -- Ari */
#include <arpa/inet.h>      /* Must be after netinet/in.h */
/* #endif */

#include <netdb.h>
#endif  /* TCP includes */
/*

  ROUGH ESTIMATE OF MAX PATH LENGTH
  
 */

#ifndef HT_MAX_PATH
#ifdef MAXPATHLEN
#define HT_MAX_PATH MAXPATHLEN
#else
#ifdef PATH_MAX
#define HT_MAX_PATH PATH_MAX
#else
#define HT_MAX_PATH 1024                        /* Any better ideas? */
#endif
#endif
#endif /* HT_MAX_PATH */
/*

  DEFINITION OF NGROUPS
  
 */

#ifdef NO_UNIX_IO
#define NO_GROUPS
#endif

#ifndef NO_GROUPS
#ifndef NGROUPS
#ifdef NGROUPS_MAX
#define NGROUPS NGROUPS_MAX
#else
#define NGROUPS 20                              /* Any better ideas? */
#endif
#endif
#endif
/*

  DEFINITION OF MAX DOMAIN NAME LENGTH
  
 */

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64                       /* Any better ideas? */
#endif
/*

  MACROS FOR MANIPULATING MASKS FOR SELECT()
  
 */

#ifdef SELECT
#ifndef FD_SET
typedef unsigned int fd_set;
#define FD_SET(fd,pmask) (*(pmask)) |=  (1<<(fd))
#define FD_CLR(fd,pmask) (*(pmask)) &= ~(1<<(fd))
#define FD_ZERO(pmask)   (*(pmask))=0
#define FD_ISSET(fd,pmask) (*(pmask) & (1<<(fd)))
#endif  /* FD_SET */
#endif  /* SELECT */
/*

  MACROS FOR CONVERTING CHARACTERS
  
 */

#ifndef TOASCII
#define TOASCII(c) (c)
#define FROMASCII(c) (c)
#endif
/*

  WHITE CHARACTERS
  
   Is character c white space? For speed, include all control characters
   
 */

#define WHITE(c) (((unsigned char)(TOASCII(c))) <= 32)
/*

  CACHE FILE PREFIX
  
   This is something onto which we tag something meaningful to make a cache file name.
   used in HTWSRC.c at least. If it is nor defined at all, caching is turned off.
   
 */

#ifndef CACHE_FILE_PREFIX
#ifdef unix
#define CACHE_FILE_PREFIX  "/usr/wsrc/"
#endif
#endif
/*

 */

#endif
/*

   End of system-specific file  */
