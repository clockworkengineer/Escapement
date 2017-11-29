## [Escapement](https://github.com/clockworkengineer/Escapement/blob/master/Escapement.cpp) (FTP Server File Synchronisation Program) ##

**Description**:  This is a Linux command line program to log on to an FTP server and sychronise a local directory (master) with a remote server (copy). If a local file is updated or deleted then this is mirriored on the server. The program also has the option of pulling a complete copy from the server to overrite the local files and also performing a synchronise at set time intervals.

**Parameters:**

    Escapement: FTP Server File Synchronisation Program
    Program Options:
    --help                Print help messages
    -c [ --config ] arg   Config File Name
    -s [ --server ] arg   FTP Server name
    -o [ --port ] arg     FTP Server port
    -u [ --user ] arg     Account username
    -p [ --password ] arg User password
    -r [ --remote ] arg   Remote directory to restore
    -l [ --local ] arg    Local directory as base for restore
    -c [ --cache ] arg    JSON file cache
    -t [ --polltime ] arg Server poll time in minutes
    -g [ --pull ]         Pull (get) files from server to local directory.
    -f [ --refresh ]      Re(f)resh JSON cache file from local/remote directories
    -n [ --nossl ]        Switch off ssl for connection
    -v [ --override ]     Override any command line options from cache file



## To Do List ##

1. Encrypt all saved passwords.
2. QT Interface

