#include "HOST.hpp"
/*
 * File:   Escapement_FileCache.cpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2017, 2:33 PM
 * 
 * Copyright 2017.
 * 
 */

//
// Module: Escapement_FileCache
//
// Description: Escapement file information cache handling code.
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

#include <iostream>
#include <fstream>

//
// Escapement File cache
//

#include "Escapement_FileCache.hpp"

// Lohmann JSON library

#include "json.hpp"

// =========
// NAMESPACE
// =========

namespace Escapement_FileCache {

    // =======
    // IMPORTS
    // =======

    using namespace std;
    
    using namespace Escapement;
            
    using namespace Antik::FTP;
    
    using json = nlohmann::json;

    // ===============
    // LOCAL FUNCTIONS
    // ===============

    // ================
    // PUBLIC FUNCTIONS
    // ================
  
    
    void loadEscapmentOptions(EscapementOptions &optionData) {

        if (optionData.override) {

            ifstream jsonFileCacheStream{ optionData.fileCache};

            if (jsonFileCacheStream) {

                json completeJSONFile;
                jsonFileCacheStream >> completeJSONFile;

                json::iterator findOptions = completeJSONFile.find("EscapementOptions");
                if (findOptions != completeJSONFile.end()) {
                    json escapementOptions = findOptions.value();
                    optionData.serverName = escapementOptions["ServerName"];
                    optionData.serverPort = escapementOptions["ServerPort"];
                    optionData.userName = escapementOptions["UserName"];
                    optionData.userPassword = escapementOptions["UserPassword"];
                    optionData.remoteDirectory = escapementOptions["RemoteDirectory"];
                    optionData.localDirectory = escapementOptions["LocalDirectory"];
                }

            }

        }
    
    }
    //
    // Load local and remote file information from cache
    //

    void loadCachedFiles( EscapementRunContext &runContext) {

        if (!runContext.optionData.fileCache.empty()) {
            
            json fileArray = json::array();
            json completeJSONFile;

            ifstream jsonFileCacheStream { runContext.optionData.fileCache };

            if (jsonFileCacheStream) {

                jsonFileCacheStream >> completeJSONFile;

                json::iterator findFiles;

                findFiles = completeJSONFile.find("RemoteFiles");
                if (findFiles != completeJSONFile.end()) {
                    fileArray = findFiles.value();
                    for (auto file : fileArray) {
                        runContext.remoteFiles[file["Filename"]] = static_cast<CFTP::DateTime> (file["Modified"].get<string>());
                    }
                }

            }

        }

    }

    //
    // Save  local and remote file information to cache
    //

    void saveCachedFiles(const EscapementRunContext &runContext) {

        if (!runContext.optionData.fileCache.empty()) {

            json fileArray = json::array();
            json completeJSONFile;
            json escapementOptions;
            
            escapementOptions["ServerName"] = runContext.optionData.serverName;
            escapementOptions["ServerPort"] = runContext.optionData.serverPort;
            escapementOptions["UserName"] = runContext.optionData.userName;
            escapementOptions["UserPassword"] = runContext.optionData.userPassword;    
            escapementOptions["RemoteDirectory"] = runContext.optionData.remoteDirectory;    
            escapementOptions["LocalDirectory"] = runContext.optionData.localDirectory;  
         
            completeJSONFile["EscapementOptions"] = escapementOptions;
 
            for (auto file : runContext.remoteFiles) {
                json fileJSON;
                fileJSON["Filename"] = file.first;
                fileJSON["Modified"] = static_cast<string> (file.second);
                fileArray.push_back(fileJSON);
            }

            completeJSONFile["RemoteFiles"] = fileArray;
            fileArray.clear();

            for (auto file : runContext.localFiles) {
                json fileJSON;
                fileJSON["Filename"] = file.first;
                fileJSON["Modified"] = static_cast<string> (file.second);
                fileArray.push_back(fileJSON);
            }

            completeJSONFile["LocalFiles"] = fileArray;

            ofstream jsonFileCacheStream(runContext.optionData.fileCache);

            if (jsonFileCacheStream) {
                jsonFileCacheStream << setw(4) << completeJSONFile << endl;
            }

        }

    }

} // namespace Escapement_FileCache

