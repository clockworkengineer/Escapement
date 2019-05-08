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
// Description: Escapement file handling code.
// 
// Dependencies: 
// 
// C11++              : Use of C11++ features.
// Antik Classes      : CFile, CPath.
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
#include "CFile.hpp"
#include "CPath.hpp"

//
// Escapement File cache
//

#include "Escapement_FileCache.hpp"
#include "Escapement_Files.hpp"

// Lohmann JSON library

#include "json.hpp"

// =========
// NAMESPACE
// =========

namespace Escapement_Files {

    // =======
    // IMPORTS
    // =======
    
    using namespace Escapement;
    using namespace Escapement_FileCache;
    using namespace Escapement_CommandLine;
    
    using namespace Antik;
    using namespace Antik::FTP;
    using namespace Antik::File;

    // ===============
    // LOCAL FUNCTIONS
    // ===============
    
    //
    // Return true if file is remote
    //

    static bool isRemoteFile(const EscapementOptions &optionData, const std::string &filePath) {
        return (filePath.find(optionData.remoteDirectory) == 0);
    }

    //
    // Return true if file is local
    //
    
    static bool isLocalFile(const EscapementOptions &optionData, const std::string &filePath) {
        return (filePath.find(optionData.localDirectory) == 0);
    }
       
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

        return (fileInfoMap);

    }
    
    //
    // Get all local file last modified date/time and return as FileInfoMap
    //

    static FileInfoMap getLocalFileListDateTime(const FileList &fileList) {

        FileInfoMap fileInfoMap;

          for (auto file : fileList) {
             if (CFile::isFile(file)) {
                time_t localModifiedTime{ 0};
                localModifiedTime = CFile::lastWriteTime(file);
                fileInfoMap[file] = static_cast<CFTP::DateTime> (localtime(&localModifiedTime));
            } else if (CFile::isDirectory(file)) { 
                fileInfoMap[file] = CFTP::DateTime();
            }
        }

        return (fileInfoMap);

    }
    
    // ================
    // PUBLIC FUNCTIONS
    // ================
    
    //
    // Convert file path to/from local/remote.
    //

    std::string convertFilePath(const EscapementOptions &optionData, const std::string &filePath) {
        
        CPath convertedPath;
        
        if (isLocalFile(optionData, filePath)) {
            convertedPath = optionData.remoteDirectory + Antik::kServerPathSep + filePath.substr(optionData.localDirectory.size());
        } else  if (isRemoteFile(optionData, filePath)) {
            convertedPath = optionData.localDirectory + filePath.substr(optionData.remoteDirectory.size());
        } else {
            std::cerr << "Error: Cannot convert file path " << filePath << std::endl;
        }
        
        convertedPath.normalize();
        
        return(convertedPath.toString());
                   
    }
    
    //
    // Get all remote files
    //

    void getAllRemoteFiles(EscapementRunContext &runContext){

        FileList fileList;
             
        listRemoteRecursive(runContext.ftpServer, runContext.optionData.remoteDirectory, fileList);

        runContext.remoteFiles = getRemoteFileListDateTime(runContext.ftpServer, fileList);

        if (runContext.remoteFiles.empty()) {
            std::cout << "*** Remote server directory empty ***" << std::endl;
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
            std::cout << "*** Local directory empty ***" << std::endl;
        }

    }

    //
    // Pull files from remote server to local directory
    //
    
    void pullFiles (EscapementRunContext &runContext) {
        
        int fileCount { 0 };
        FileCompletionFn completionFn = [&fileCount] (std::string fileName) {std::cout << "Pulled file No " << ++fileCount << " [" << fileName << "]" << std::endl;};
        
        if (!runContext.filesToProcess.empty()) {

            std::sort(runContext.filesToProcess.begin(), runContext.filesToProcess.end()); // getFiles() requires list to be sorted
            
            FileList successList { getFiles(runContext.ftpServer, runContext.optionData.localDirectory, runContext.filesToProcess, completionFn, true) };
            
            FileInfoMap filesTransfered {getLocalFileListDateTime(successList)};
            
            if (!filesTransfered.empty()) {
                for (auto &file : filesTransfered) {
                    runContext.localFiles[file.first] = file.second;
                }
                runContext.totalFilesProcessed += filesTransfered.size();
            }
            
            if (runContext.filesToProcess.size() != filesTransfered.size()) {
                for (auto file : runContext.filesToProcess) {
                    if (!filesTransfered.count(convertFilePath(runContext.optionData, file))) {
                        std::cout << "File [" << file << "] not transferred/created." << std::endl;                  
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
        FileCompletionFn completionFn = [&fileCount] (std::string fileName) {std::cout << "Pushed file No " << ++fileCount << " [" << fileName << "]" << std::endl;};
               
        if (!runContext.filesToProcess.empty()) {
            
            std::sort(runContext.filesToProcess.begin(), runContext.filesToProcess.end()); // Putfiles() requires list to be sorted
            
            FileList successList {  putFiles(runContext.ftpServer, runContext.optionData.localDirectory, runContext.filesToProcess, completionFn, true) };

            FileInfoMap filesTransfered { getRemoteFileListDateTime(runContext.ftpServer, successList ) };
     
            if (!filesTransfered.empty()) {
                std::cout << "Number of files to transfer [" << filesTransfered.size() << "]" << std::endl;
                for (auto &file : filesTransfered) {
                    runContext.remoteFiles[file.first] = file.second;
                }
                runContext.totalFilesProcessed += filesTransfered.size();
            }

            if (runContext.filesToProcess.size() != filesTransfered.size()) {
                for (auto file : runContext.filesToProcess) {
                    if (!filesTransfered.count(convertFilePath(runContext.optionData, file))) {
                        std::cout << "File [" << file << "] not transferred/created." << std::endl;
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
                    std::cout << "File [" << file << " ] removed from server." << std::endl;
                    runContext.remoteFiles.erase(file);
                    runContext.totalFilesProcessed++;
                } else if (runContext.ftpServer.removeDirectory(file) == 250) {
                    std::cout << "Directory [" << file << " ] removed from server." << std::endl;
                    runContext.remoteFiles.erase(file);
                    runContext.totalFilesProcessed++;
                } else {
                    std::cerr << "File [" << file << " ] could not be removed from server." << std::endl;
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