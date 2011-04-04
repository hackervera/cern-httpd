/*                                                                   Socket writer for libwww
                          UNIX FILE DESCRIPTOR OR SOCKET WRITER
                                             
   This version of the stream object just writes to a socket. The socket is assumed open
   and closed afterward.There are two versions (identical on ASCII machines) one of which
   converts to ASCII on output.
   
   Part of libwww. See also HTFWriter for writing to C files.
   
  BUGS:
  
      strings written must be less than buffer size.
      
 */
#ifndef HTWRITE_H
#define HTWRITE_H

#include "HTStream.h"

extern HTStream * HTWriter_new PARAMS((int soc));
extern HTStream * HTWriter_newNoClose PARAMS((int soc));

extern HTStream * HTASCIIWriter PARAMS((int soc));

#endif

/*

   end */
