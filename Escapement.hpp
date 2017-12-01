/*
 * File:   Escapement.hpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2017, 2:33 PM
 * 
 * Copyright 2017.
 * 
 */

#ifndef ESCAPEMENT_HPP
#define ESCAPEMENT_HPP

//
// C++ STL
//

#include <string>
#include <unordered_map>
#include <deque>

//
// Antik Classes
//

#include "CFTP.hpp"

// =========
// NAMESPACE
// =========

namespace Escapement {
    
    //
    // Escapement commands
    //
    
    const int kEscapementSynchronise  { 0 };
    const int kEscapementPullFiles    { 1 };
    const int kEscapementRefreshCache { 2 };
    
    //
    // Escapement decoded option argument data.
    //

    struct EscapementOptions {
        std::string configFileName;            // Configuration file name
        std::string userName;                  // FTP account user name
        std::string userPassword;              // FTP account user name password
        std::string serverName;                // FTP server
        std::string serverPort;                // FTP server port
        std::string remoteDirectory;           // FTP remote directory for sync
        std::string localDirectory;            // Local directory for sync with server
        int pollTime { 0 };                    // Poll time in minutes.
        std::string fileCache;                 // JSON file to hold remote/local file info
        int command { kEscapementSynchronise };// == 0 Synchronise, == 1 Pull , == 2 Refresh cache
        bool noSSL { false };                  // == true switch off default SSL connection
        bool override { false };               // == true override any option values from cache file
    };


   // File information map (indexed by filename, value last modified date/time)

   typedef std::unordered_map<std::string, Antik::FTP::CFTP::DateTime> FileInfoMap;

   // Escapement run context (run options, file lists and ftp server data)
   
    struct EscapementRunContext {
        EscapementOptions optionData;           // Program parameters
        Antik::FTP::CFTP ftpServer;             // FTP server object
        FileInfoMap localFiles;                 // List of local files
        FileInfoMap remoteFiles;                // List of remote files
        Antik::FTP::FileList filesToProcess;    // List of files to be processed
        int totalFilesProcessed { 0 };          // Total files processed
    };

} // namespace Escapement

#endif /* ESCAPEMENT_HPP */

