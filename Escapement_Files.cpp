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
        
        fs::path convertedPath;
        
        if (filePath.find(optionData.localDirectory) == 0) {
            convertedPath = optionData.remoteDirectory + kServerPathSep + filePath.substr(optionData.localDirectory.size());
            return (convertedPath.normalize().string());
        } else  if (filePath.find(optionData.remoteDirectory) == 0) {
            convertedPath = optionData.localDirectory + filePath.substr(optionData.remoteDirectory.size());
            return (convertedPath.normalize().string());
        } else {
            std::cerr << "Error: Cannot convert file path " << filePath << std::endl;
            return(filePath);
        }
        
    }

    //
    // Get all remote file last modified date/time and return as FileInfoMap
    //

    FileInfoMap getRemoteFileListDateTime(CFTP &ftpServer, const FileList &fileList) {

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

    FileInfoMap getLocalFileListDateTime(const FileList &fileList) {

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

    void getAllRemoteFiles(EscapementRunContext &runContext){

        vector<string> fileList;
             
        listRemoteRecursive(runContext.ftpServer, runContext.optionData.remoteDirectory, fileList);

        runContext.remoteFiles = getRemoteFileListDateTime(runContext.ftpServer, fileList);

        if (runContext.remoteFiles.empty()) {
            cout << "*** Remote server directory empty ***" << endl;
        }

    }

    //
    // Pull files from remote server to local directory
    //
    
    void pullFiles (EscapementRunContext &runContext, FileList &filesToTransfer) {
        
        int fileCount { 0 };
        Antik::FTP::FileCompletionFn completionFn = [&fileCount] (std::string fileName) {std::cout << "Pulled file No " << ++fileCount << " [" << fileName << "]" << std::endl;};
        
        if (!filesToTransfer.empty()) {

            sort(filesToTransfer.begin(), filesToTransfer.end()); // getFiles() requires list to be sorted
            
            FileInfoMap filesTransfered {getLocalFileListDateTime(getFiles(runContext.ftpServer, runContext.optionData.localDirectory, filesToTransfer, completionFn))};
            
            if (!filesTransfered.empty()) {
                for (auto &file : filesTransfered) {
                    runContext.localFiles[file.first] = file.second;
                }
            }
            
            if (filesToTransfer.size() != filesTransfered.size()) {
                for (auto file : filesToTransfer) {
                    if (!filesTransfered.count(convertFilePath(runContext.optionData, file))) {
                        cout << "File [" << file << "] not transferred/created." << std::endl;                  
                    }
                }
            }
            
        }
        
    }
    
    //
    // Push files from local directory to server
    //
    
    void pushFiles (EscapementRunContext &runContext, FileList &filesToTransfer) {
  
        int fileCount { 0 };
        Antik::FTP::FileCompletionFn completionFn = [&fileCount] (std::string fileName) {std::cout << "Pushed file No " << ++fileCount << " [" << fileName << "]" << std::endl;};
               
        if (!filesToTransfer.empty()) {
            
            sort(filesToTransfer.begin(), filesToTransfer.end()); // Putfiles() requires list to be sorted
            
            FileInfoMap filesTransfered { getRemoteFileListDateTime(runContext.ftpServer, putFiles(runContext.ftpServer, runContext.optionData.localDirectory, filesToTransfer, completionFn)) };
     
            if (!filesTransfered.empty()) {
                cout << "Number of files to transfer [" << filesTransfered.size() << "]" << endl;
                for (auto &file : filesTransfered) {
                    runContext.remoteFiles[file.first] = file.second;
                }
            }

            if (filesToTransfer.size() != filesTransfered.size()) {
                for (auto file : filesToTransfer) {
                    if (!filesTransfered.count(convertFilePath(runContext.optionData, file))) {
                        cout << "File [" << file << "] not transferred/created." << std::endl;
                    }
                }
            }

        }
        
    }
    
    //
    // Purge any remote files from server that have been deleted locally
    //
    
    void deleteFiles (EscapementRunContext &runContext, FileList &filesToDelete) {

        if (!filesToDelete.empty()) {

            // Sort files in reverse order for delete so directories get deleted last
            
            sort(filesToDelete.rbegin(), filesToDelete.rend()); 

            for (auto &file : filesToDelete) {
                if (runContext.ftpServer.deleteFile(file) == 250) {
                    cout << "File [" << file << " ] removed from server." << endl;
                    runContext.remoteFiles.erase(file);
                } else if (runContext.ftpServer.removeDirectory(file) == 250) {
                    cout << "Directory [" << file << " ] removed from server." << endl;
                    runContext.remoteFiles.erase(file);
                } else {
                    cerr << "File [" << file << " ] could not be removed from server." << endl;
                }
            }

        }
        
    }
   
    //
    // Load local and remote file information before synchronise
    //

    void loadFilesBeforeSynchronise(EscapementRunContext &runContext) {

        vector<string> fileList;

        // Load any cached file information

        loadCachedFiles(runContext);

        // No cached remote files so get list from server

        if (runContext.remoteFiles.empty()) {
            getAllRemoteFiles(runContext);
        }
        
        // Create local file list (done at runtime to pickup changes).

        fileList.clear();

        listLocalRecursive(runContext.optionData.localDirectory, fileList);

        runContext.localFiles = getLocalFileListDateTime(fileList);
        
        if (runContext.localFiles.empty()) {
            cout << "*** Local directory empty ***" << endl;
        }

    }

    
    //
    // Save maps containing local and remote files after synchronise
    //

    void saveFilesAfterSynchronise(const EscapementRunContext &runContext) {

        // Save any cached file information

        saveCachedFiles(runContext);

    }

} // namespace Escapement_Files