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
    
    //
    // Load local and remote file information from cache
    //

    void loadCachedFiles(const EscapementOptions &optionData, FileInfoMap &remoteFiles, FileInfoMap &localFiles) {

        if (!optionData.fileCache.empty()) {
            
            json fileArray = json::array();
            json completeJSONFile;

            ifstream jsonFileCacheStream { optionData.fileCache };

            if (jsonFileCacheStream) {

                jsonFileCacheStream >> completeJSONFile;

                json::iterator findFiles;

                findFiles = completeJSONFile.find("RemoteFiles");
                if (findFiles != completeJSONFile.end()) {
                    fileArray = findFiles.value();
                    for (auto file : fileArray) {
                        remoteFiles[file["Filename"]] = static_cast<CFTP::DateTime> (file["Modified"].get<string>());
                    }
                }

            }

        }

    }

    //
    // Save  local and remote file information to cache
    //

    void saveCachedFiles(const EscapementOptions &optionData, FileInfoMap &remoteFiles, FileInfoMap &localFiles) {

        if (!optionData.fileCache.empty()) {

            json fileArray = json::array();
            json completeJSONFile;

            for (auto file : remoteFiles) {
                json fileJSON;
                fileJSON["Filename"] = file.first;
                fileJSON["Modified"] = static_cast<string> (file.second);
                fileArray.push_back(fileJSON);
            }

            completeJSONFile["RemoteFiles"] = fileArray;
            fileArray.clear();

            for (auto file : localFiles) {
                json fileJSON;
                fileJSON["Filename"] = file.first;
                fileJSON["Modified"] = static_cast<string> (file.second);
                fileArray.push_back(fileJSON);
            }

            completeJSONFile["LocalFiles"] = fileArray;

            ofstream jsonFileCacheStream(optionData.fileCache);

            if (jsonFileCacheStream) {
                jsonFileCacheStream << setw(4) << completeJSONFile << endl;
            }

        }

    }

} // namespace Escapement_FileCache

