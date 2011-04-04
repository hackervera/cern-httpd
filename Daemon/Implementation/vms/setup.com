$ THIS_PATH = F$Element (0, "]", F$Environment ("PROCEDURE")) + "]"
$ httpd :== $ 'THIS_PATH'httpd.exe
$ htadm :== $ 'THIS_PATH'htadm.exe
$ htimage :== $ 'THIS_PATH'htimage.exe
$ cgiparse :== $ 'THIS_PATH'cgiparse.exe
