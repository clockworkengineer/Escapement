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
// Dependencies: Escapement file handling code.
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

#include <iostream>

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
    // Convert file path to/from local/remote.
    //

    string convertFilePath(const EscapementOptions &optionData, const string &filePath) {
        if (filePath.find(optionData.localDirectory) == 0) {
            size_t localDirectoryLength = optionData.localDirectory.size();
            if (optionData.localDirectory.back() != kServerPathSep) localDirectoryLength++;
            return (optionData.remoteDirectory + kServerPathSep + filePath.substr(localDirectoryLength));
        } else  if (filePath.find(optionData.remoteDirectory) == 0) {
            size_t remoteDirectoryLength = optionData.remoteDirectory.size();
            if (optionData.localDirectory.back() == kServerPathSep) remoteDirectoryLength++;
            return (optionData.localDirectory + filePath.substr(remoteDirectoryLength));
        } else {
            std::cerr << "Error: Cannot convert file path " << filePath << std::endl;
            return(filePath);
        }
        
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

        return (std::move(fileInfoMap));

    }
    
    //
    // Get all local file last modified date/time and return as FileInfoMap
    //

    FileInfoMap getLocalFileListDateTime(const vector<string> &fileList) {

        FileInfoMap fileInfoMap;

          for (auto file : fileList) {
             if (fs::is_regular_file(file)) {
                time_t localModifiedTime{ 0};
                localModifiedTime = fs::last_write_time(file);
                fileInfoMap[file] = static_cast<CFTP::DateTime> (localtime(&localModifiedTime));
            } else if (fs::is_directory(file)) { 
                fileInfoMap[file] = CFTP::DateTime();
            }
        }

        return (std::move(fileInfoMap));

    }
    
    //
    // Get all remote files
    //

    void getAllRemoteFiles(CFTP &ftpServer, const std::string &remoteDirectory, FileInfoMap &remoteFiles){

        vector<string> fileList;
             
        listRemoteRecursive(ftpServer, remoteDirectory, fileList);

        remoteFiles = getRemoteFileListDateTime(ftpServer, fileList);

        if (remoteFiles.empty()) {
            cout << "*** Remote server directory empty ***" << endl;
        }

    }

    //
    // Pull files from remote server to local directory
    //
    
    void pullFiles (CFTP &ftpServer, EscapementOptions &optionData, FileInfoMap &localFiles, std::vector<string> &filesToTransfer) {
        
        int fileCount { 0 };
        Antik::FTP::FileCompletionFn completionFn = [&fileCount] (std::string fileName) {std::cout << "Pulled file No " << ++fileCount << " [" << fileName << "]" << std::endl;};
        
        if (!filesToTransfer.empty()) {
            
            sort(filesToTransfer.begin(), filesToTransfer.end()); // getFiles() requires list to be sorted
            
            FileInfoMap filesTransfered {getLocalFileListDateTime(getFiles(ftpServer, optionData.localDirectory, filesToTransfer, completionFn))};
            
            if (!filesTransfered.empty()) {
                cout << "Number of files to transfer [" << filesTransfered.size() << "]" << endl;
                for (auto &file : filesTransfered) {
                    localFiles[file.first] = file.second;
                }
            } else {
                cerr << "None of the selected files transferred." << endl;
            }
            
        }
        
    }
    
    //
    // Push files from local directory to server
    //
    
    void pushFiles (CFTP &ftpServer, EscapementOptions &optionData, FileInfoMap &remoteFiles, std::vector<string> &filesToTransfer) {
  
        int fileCount { 0 };
        Antik::FTP::FileCompletionFn completionFn = [&fileCount] (std::string fileName) {std::cout << "Pushed file No " << ++fileCount << " [" << fileName << "]" << std::endl;};
               
        if (!filesToTransfer.empty()) {
            
            sort(filesToTransfer.begin(), filesToTransfer.end()); // Putfiles() requires list to be sorted
            
            FileInfoMap filesTransfered { getRemoteFileListDateTime(ftpServer, putFiles(ftpServer, optionData.localDirectory, filesToTransfer, completionFn)) };
     
            if (!filesTransfered.empty()) {
                cout << "Number of files to transfer [" << filesTransfered.size() << "]" << endl;
                for (auto &file : filesTransfered) {
                    remoteFiles[file.first] = file.second;
                }
            } else {
                cerr << "None of the selected files transferred." << endl;
            }
            
        }
        
    }
    
    //
    // Purge any remote files from server that have been deleted locally
    //
    
    void deleteFiles (CFTP &ftpServer, EscapementOptions &optionData, FileInfoMap &remoteFiles, std::vector<string> &filesToDelete) {

        for (auto &file : filesToDelete) {
            if (ftpServer.deleteFile(file) == 250) {
                cout << "File [" << file << " ] removed from server." << endl;
                remoteFiles.erase(file);
            } else if (ftpServer.removeDirectory(file) == 250) {
                cout << "Directory [" << file << " ] removed from server." << endl;
                remoteFiles.erase(file);
            } else {
                cerr << "File [" << file << " ] could not be removed from server." << endl;
            }
        }
        
    }
   
    //
    // Load local and remote file information before synchronise
    //

    void loadFilesBeforeSynchronise(CFTP &ftpServer, const EscapementOptions &optionData, FileInfoMap &remoteFiles, FileInfoMap &localFiles) {

        vector<string> fileList;

        // Load any cached file information

        loadCachedFiles(optionData.fileCache, remoteFiles, localFiles);

        // No cached remote files so get list from server

        if (remoteFiles.empty()) {
            getAllRemoteFiles(ftpServer, optionData.remoteDirectory, remoteFiles);
        }

        // Create local file list (done at runtime to pickup changes).

        fileList.clear();

        listLocalRecursive(optionData.localDirectory, fileList);

        localFiles = getLocalFileListDateTime(fileList);
        
        if (localFiles.empty()) {
            cout << "*** Local directory empty ***" << endl;
        }

    }

    
    //
    // Save maps containing local and remote files after synchronise
    //

    void saveFilesAfterSynchronise(CFTP &ftpServer, const EscapementOptions &optionData, FileInfoMap &remoteFiles, FileInfoMap &localFiles) {

        // Save any cached file information

        saveCachedFiles(optionData.fileCache, remoteFiles, localFiles);

    }

} // namespace Escapement_Files