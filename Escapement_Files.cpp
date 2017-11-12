#include "HOST.hpp"
/*
 * File:   Escapement_Files.cpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2017, 2:33 PM
 * 
 * Copyright 2017.
 * 
 */

//
// Module: Escapement_Files
//
// Description:
// 
// Dependencies: 
// 
// C11++              : Use of C11++ features.
// Misc.              : Lohmann JSON library
//

// =============
// INCLUDE FILES
// =============

//
// C++ STL
//

//#include <iostream>
//#include <fstream>

//
// Antik Classes
//

#include "FTPUtil.hpp"

//
// Escapement File cache
//

#include "Escapement_FileCache.hpp"
#include "Escapement_Files.hpp"

//
// Boost file system library
//

#include <boost/filesystem.hpp>

// Lohmann JSON library

#include "json.hpp"

// =========
// NAMESPACE
// =========

namespace Escapement_Files {

    // =======
    // IMPORTS
    // =======

    using namespace std;
    
    using namespace Escapement;
    using namespace Escapement_FileCache;
    using namespace Escapement_CommandLine;
            
    using namespace Antik::FTP;
   
    namespace fs = boost::filesystem;

    // ===============
    // LOCAL FUNCTIONS
    // ===============

    // ================
    // PUBLIC FUNCTIONS
    // ================
    
    //
    // Convert local file path to remote server path
    //

    string localFileToRemote(EscapementOptions &optionData, const string &localFilePath) {
        size_t localDirectoryLength = optionData.localDirectory.size();
        if (optionData.localDirectory.back() != kServerPathSep) localDirectoryLength++;
        return (optionData.remoteDirectory + kServerPathSep + localFilePath.substr(localDirectoryLength));
    }

    //
    // Convert remote server file path to local path
    //

    string remoteFileToLocal(EscapementOptions &optionData, const string &remoteFilePath) {
        size_t remoteDirectoryLength = optionData.remoteDirectory.size();
        if (optionData.localDirectory.back() == kServerPathSep) remoteDirectoryLength++;
        return (optionData.localDirectory + remoteFilePath.substr(remoteDirectoryLength));
    }

    //
    // Get all remote file last modified date/time and return as FileInfoMap
    //

    FileInfoMap getRemoteFileListDateTime(CFTP &ftpServer, const vector<string> &fileList) {

        FileInfoMap fileInfoMap;

        for (auto file : fileList) {
            CFTP::DateTime modifiedDateTime;
            ftpServer.getModifiedDateTime(file, modifiedDateTime);
            fileInfoMap[file] = modifiedDateTime;
        }

        return (move(fileInfoMap));

    }

    //
    // Load local and remote file information before synchronise
    //

    void loadFilesBeforeSynchronise(CFTP &ftpServer, EscapementOptions &optionData, FileInfoMap &remoteFiles, FileInfoMap &localFiles) {

        vector<string> fileList;

        // Load any cached file information

        loadCachedFiles(optionData.fileCache, remoteFiles, localFiles);

        // No cached remote files so get list from server

        if (remoteFiles.empty()) {

            listRemoteRecursive(ftpServer, optionData.remoteDirectory, fileList);

            remoteFiles = getRemoteFileListDateTime(ftpServer, fileList);

            if (remoteFiles.empty()) {
                cout << "*** Remote server directory empty ***" << endl;
            }

        }

        // Create local file list (done at runtime to pickup changes).

        fileList.clear();

        listLocalRecursive(optionData.localDirectory, fileList);

        for (auto file : fileList) {
            time_t localModifiedTime{ 0};
            if (fs::is_regular_file(file)) {
                localModifiedTime = fs::last_write_time(file);
            }
            localFiles[file] = static_cast<CFTP::DateTime> (localtime(&localModifiedTime));
        }

        if (localFiles.empty()) {
            cout << "*** Local directory empty ***" << endl;
        }

    }

    
    //
    // Save maps containing local and remote files after synchronise
    //

    void saveFilesAfterSynchronise(CFTP &ftpServer, EscapementOptions &optionData, FileInfoMap &remoteFiles, FileInfoMap &localFiles) {

        // Save any cached file information

        saveCachedFiles(optionData.fileCache, remoteFiles, localFiles);


    }

} // namespace Escapement_Files