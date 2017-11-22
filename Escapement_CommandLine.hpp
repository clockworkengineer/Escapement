/*
 * File:   Escapement_CommandLine.hpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2017, 2:33 PM
 * 
 * Copyright 2017.
 * 
 */

#ifndef ESCAPEMENT_COMMANDLINE_HPP
#define ESCAPEMENT_COMMANDLINE_HPP

//
// C++ STL
//

#include <string>

// =========
// NAMESPACE
// =========

namespace Escapement_CommandLine {

    //
    // Decoded option argument data.
    //

    struct EscapementOptions {
        std::string configFileName;     // Configuration file name
        std::string userName;           // FTP account user name
        std::string userPassword;       // FTP account user name password
        std::string serverName;         // FTP server
        std::string serverPort;         // FTP server port
        std::string remoteDirectory;    // FTP remote directory for sync
        std::string localDirectory;     // Local directory for sync with server
        int pollTime { 0 };             // Poll time in minutes.
        std::string fileCache;          // JSON tile to hold remote/local file info
        bool pullFromServer { false };  // == true then pulling files from server
        bool refreshCache { false };    // == true then refresh cache file
        bool noSSL { false };           // == true switch off default SSL connection
    };

    EscapementOptions fetchCommandLineOptions(int argc, char** argv);

} // namespace Escapement_CommandLine

#endif /* ESCAPEMENT_COMMANDLINE_HPP */

