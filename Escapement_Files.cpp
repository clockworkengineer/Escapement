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

    //
    // Get all remote file last modified date/time and return as FileInfoMap
    //

    static FileInfoMap getRemoteFileListDateTime(CFTP &ftpServer, const FileList &fileList) {

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

    static FileInfoMap getLocalFileListDateTime(const FileList &fileList) {

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
    // Get all remote files
    //

    void getAllRemoteFiles(EscapementRunContext &runContext){

        FileList fileList;
             
        listRemoteRecursive(runContext.ftpServer, runContext.optionData.remoteDirectory, fileList);

        runContext.remoteFiles = getRemoteFileListDateTime(runContext.ftpServer, fileList);

        if (runContext.remoteFiles.empty()) {
            cout << "*** Remote server directory empty ***" << endl;
        }

    }
    
    //
    // Get all local files
    //

    void getAllLocalFiles(EscapementRunContext &runContext){

        FileList fileList;
             
        listLocalRecursive(runContext.optionData.localDirectory, fileList);
        runContext.localFiles = getLocalFileListDateTime(fileList);

        if (runContext.localFiles.empty()) {
            cout << "*** Local directory empty ***" << endl;
        }

    }

    //
    // Pull files from remote server to local directory
    //
    
    void pullFiles (EscapementRunContext &runContext) {
        
        int fileCount { 0 };
        Antik::FTP::FileCompletionFn completionFn = [&fileCount] (std::string fileName) {std::cout << "Pulled file No " << ++fileCount << " [" << fileName << "]" << std::endl;};
        
        if (!runContext.filesToProcess.empty()) {

            sort(runContext.filesToProcess.begin(), runContext.filesToProcess.end()); // getFiles() requires list to be sorted
            
            FileInfoMap filesTransfered {getLocalFileListDateTime(getFiles(runContext.ftpServer, runContext.optionData.localDirectory, runContext.filesToProcess, completionFn))};
            
            if (!filesTransfered.empty()) {
                for (auto &file : filesTransfered) {
                    runContext.localFiles[file.first] = file.second;
                }
            }
            
            if (runContext.filesToProcess.size() != filesTransfered.size()) {
                for (auto file : runContext.filesToProcess) {
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
    
    void pushFiles (EscapementRunContext &runContext) {
  
        int fileCount { 0 };
        Antik::FTP::FileCompletionFn completionFn = [&fileCount] (std::string fileName) {std::cout << "Pushed file No " << ++fileCount << " [" << fileName << "]" << std::endl;};
               
        if (!runContext.filesToProcess.empty()) {
            
            sort(runContext.filesToProcess.begin(), runContext.filesToProcess.end()); // Putfiles() requires list to be sorted
            
            FileInfoMap filesTransfered { getRemoteFileListDateTime(runContext.ftpServer, putFiles(runContext.ftpServer, runContext.optionData.localDirectory, runContext.filesToProcess, completionFn)) };
     
            if (!filesTransfered.empty()) {
                cout << "Number of files to transfer [" << filesTransfered.size() << "]" << endl;
                for (auto &file : filesTransfered) {
                    runContext.remoteFiles[file.first] = file.second;
                }
            }

            if (runContext.filesToProcess.size() != filesTransfered.size()) {
                for (auto file : runContext.filesToProcess) {
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
    
    void deleteFiles (EscapementRunContext &runContext) {

        if (!runContext.filesToProcess.empty()) {

            // Sort files in reverse order for delete so directories get deleted last
            
            sort(runContext.filesToProcess.rbegin(), runContext.filesToProcess.rend()); 

            for (auto &file : runContext.filesToProcess) {
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

        // Load any cached file information

        loadCachedFiles(runContext);

        // No cached remote files so get list from server

        if (runContext.remoteFiles.empty()) {
            getAllRemoteFiles(runContext);
        }

        // Create local file list (done at runtime to pickup changes).

        getAllLocalFiles(runContext);

    }
    
    //
    // Save maps containing local and remote files after synchronise
    //

    void saveFilesAfterSynchronise(const EscapementRunContext &runContext) {

        // Save any cached file information

        saveCachedFiles(runContext);

    }

} // namespace Escapement_Files