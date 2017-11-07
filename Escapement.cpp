#include "HOST.hpp"
/*
 * File:   Escapement.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

//
// Program: Escapement
//
// Description: Simple FTP program that takes a local directory and keeps it 
// synchronised with a remote server directory. To save on FTP requests it can 
// keep a local JSON file that contains a cache of remote file details.
//
// Dependencies: C11++, Classes (CFTP, CSocket), Boost C++ Libraries, Lohmann JSON library.
//
// Escapement
// Program Options:
//   --help                 Print help messages
//   -c [ --config ] arg    Config File Name
//   -s [ --server ] arg    FTP Server
//   -p [ --port ] arg      FTP Server port
//   -u [ --user ] arg      Account username
//   -p [ --password ] arg  User password
//   -r [ --remote ] arg    Remote server directory
//   -l [ --local ] arg     Local directory
//   -t [ --polltime ] arg  Server poll time in minutes
//   -c [ --cache ] arg     JSON filename cache
//   

// =============
// INCLUDE FILES
// =============

//
// C++ STL
//

#include <iostream>
#include <unordered_map>

//
// Antik Classes
//

#include "CFTP.hpp"
#include "FTPUtil.hpp"

using namespace Antik::FTP;

//
// Boost program options  & file system library
//

#include <boost/program_options.hpp>  
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

// Lohmann JSON library

#include "json.hpp"

using json = nlohmann::json;

// ======================
// LOCAL TYES/DEFINITIONS
// ======================

// Command line parameter data

struct ParamArgData {
    std::string userName;        // FTP account user name
    std::string userPassword;    // FTP account user name password
    std::string serverName;      // FTP server
    std::string serverPort;      // FTP server port
    std::string remoteDirectory; // FTP remote directory for sync
    std::string localDirectory;  // Local directory for sync with server
    int pollTime { 0 };          // Poll time in minutes.
    std::string fileCache;       // JSON tile to hold remote/local file info
    std::string configFileName;  // Configuration file name
};

typedef std::unordered_map<std::string, CFTP::DateTime> FileInfoMap;  

// ===============
// LOCAL FUNCTIONS
// ===============

//
// Exit with error message/status
//

void exitWithError(std::string errMsg) {

    // Display error and exit.

    std::cout.flush();
    std::cerr << errMsg << std::endl;
    exit(EXIT_FAILURE);

}

//
// Add options common to both command line and config file
//

void addCommonOptions(po::options_description& commonOptions, ParamArgData& argData) {

    commonOptions.add_options()
            ("server,s", po::value<std::string>(&argData.serverName)->required(), "FTP Server name")
            ("port,o", po::value<std::string>(&argData.serverPort)->required(), "FTP Server port")
            ("user,u", po::value<std::string>(&argData.userName)->required(), "Account username")
            ("password,p", po::value<std::string>(&argData.userPassword)->required(), "User password")
            ("remote,r", po::value<std::string>(&argData.remoteDirectory)->required(), "Remote directory to restore")
            ("local,l", po::value<std::string>(&argData.localDirectory)->required(), "Local directory as base for restore")
            ("cache,c",  po::value<std::string>(&argData.fileCache), "JSON file cache")
            ("polltime,t", po::value<int>(&argData.pollTime), "Server poll time in minutes");

}

//
// Read in and process command line arguments using boost.
//

void procCmdLine(int argc, char** argv, ParamArgData &argData) {

    // Define and parse the program options

    po::options_description commandLine("Program Options");
    commandLine.add_options()
            ("help", "Print help messages")
            ("config,c", po::value<std::string>(&argData.configFileName), "Config File Name");

    addCommonOptions(commandLine, argData);

    po::options_description configFile("Config Files Options");

    addCommonOptions(configFile, argData);

    po::variables_map vm;

    try {

        // Process arguments

        po::store(po::parse_command_line(argc, argv, commandLine), vm);

        // Display options and exit with success

        if (vm.count("help")) {
            std::cout << "Escapement" << std::endl << commandLine << std::endl;
            exit(EXIT_SUCCESS);
        }

        if (vm.count("config")) {
            if (fs::exists(vm["config"].as<std::string>().c_str())) {
                std::ifstream ifs{vm["config"].as<std::string>().c_str()};
                if (ifs) {
                    po::store(po::parse_config_file(ifs, configFile), vm);
                }
            } else {
                throw po::error("Specified config file does not exist.");
            }
        }

        po::notify(vm);
        
    } catch (po::error& e) {
        std::cerr << "Escapement Error: " << e.what() << std::endl << std::endl;
        std::cerr << commandLine << std::endl;
        exit(EXIT_FAILURE);
    }

}

//
// Convert local file path to remote server path
//

static inline std::string localFileToRemote(ParamArgData &argData, const std::string &localFilePath) {
    size_t localDirectoryLength = argData.localDirectory.size();
    if (argData.localDirectory.back() != '/') localDirectoryLength++;
    return (argData.remoteDirectory + kServerPathSep + localFilePath.substr(localDirectoryLength));
}

//
// Convert remote server file path to local path
//

static inline std::string remoteFileToLocal(ParamArgData &argData, const std::string &remoteFilePath) {
    size_t remoteDirectoryLength = argData.remoteDirectory.size();
    if (argData.localDirectory.back() == '/') remoteDirectoryLength++;
    return (argData.localDirectory + remoteFilePath.substr(remoteDirectoryLength));
}

//
// Get all remote file last modified date/time and return as FileInfoMap
//

static inline FileInfoMap getRemoteFileListDateTime(CFTP &ftpServer, const std::vector<std::string> &fileList) {

    FileInfoMap fileInfoMap;
    
    for (auto file : fileList) {
        CFTP::DateTime modifiedDateTime;
        ftpServer.getModifiedDateTime(file, modifiedDateTime);
        fileInfoMap[file] = modifiedDateTime; 
    }
    
    return(std::move(fileInfoMap));
    
}

//
// Load cached remote file info list from cache
//

static inline void loadCachedRemoteFiles(ParamArgData &argData, FileInfoMap &remoteFiles) {

    if (!argData.fileCache.empty()) {

        json fileArray = json::array();
        json completeJSONFile;

        std::ifstream jsonFileCacheStream(argData.fileCache);

        if (jsonFileCacheStream) {

            jsonFileCacheStream >> completeJSONFile;

            json::iterator findFiles;

            findFiles = completeJSONFile.find("RemoteFiles");
            if (findFiles != completeJSONFile.end()) {
                fileArray = findFiles.value();
                for (auto file : fileArray) {
                    std::string modifiedDataTime = file["Modified"];
                    remoteFiles[file["Filename"]] = static_cast<CFTP::DateTime> (modifiedDataTime);
                }
            }
            
        }

    }
    
}

//
// Save  local and remote information to cache
//

static inline void saveFilesToCache(ParamArgData &argData, FileInfoMap &remoteFiles, FileInfoMap &localFiles) {

    if (!argData.fileCache.empty()) {

        json fileArray = json::array();
        json completeJSONFile;

        for (auto file : remoteFiles) {
            json fileJSON;
            fileJSON["Filename"] = file.first;
            fileJSON["Modified"] = static_cast<std::string> (file.second);
            fileArray.push_back(fileJSON);
        }

        completeJSONFile["RemoteFiles"] = fileArray;
        fileArray.clear();

        for (auto file : localFiles) {
            json fileJSON;
            fileJSON["Filename"] = file.first;
            fileJSON["Modified"] = static_cast<std::string> (file.second);
            fileArray.push_back(fileJSON);
        }

        completeJSONFile["LocalFiles"] = fileArray;

        std::ofstream jsonFileCacheStream(argData.fileCache);

        if (jsonFileCacheStream) {
            jsonFileCacheStream << std::setw(4) << completeJSONFile << std::endl;
        }

    }
    
}

//
// Load maps containing local and remote files to synchronise
//

static inline void loadFilesToSynchronize(CFTP &ftpServer, ParamArgData &argData, FileInfoMap &remoteFiles, FileInfoMap &localFiles) {

    std::vector<std::string> fileList;
    
    // Load any cached remote files
    
    loadCachedRemoteFiles(argData, remoteFiles);
    
    // No cached remote files so get list from server

    if (remoteFiles.empty()) {

        listRemoteRecursive(ftpServer, argData.remoteDirectory, fileList);

        remoteFiles = getRemoteFileListDateTime(ftpServer, fileList);

        if (remoteFiles.empty()) {
            std::cout << "*** Remote server directory empty ***" << std::endl;
        }
        
    }
    
    // Create local file list (done at runtime to pickup changes).
    
    fileList.clear();
    
    listLocalRecursive(argData.localDirectory, fileList);

    for (auto file : fileList) {
        std::time_t localModifiedTime{ 0};
        if (fs::is_regular_file(file)) {
            localModifiedTime = fs::last_write_time(file);
        }
        localFiles[file] = static_cast<CFTP::DateTime> (std::localtime(&localModifiedTime));
    }
      
    if (localFiles.empty()) {
        std::cout << "*** Local directory empty ***" << std::endl;
    }

}

// ============================
// ===== MAIN ENTRY POint =====
// ============================

int main(int argc, char** argv) {

    try {

        ParamArgData argData;
        CFTP ftpServer;
        FileInfoMap localFiles;
        FileInfoMap remoteFiles;

        // Read in command line parameters and process

        procCmdLine(argc, argv, argData);

        std::cout << "Server [" << argData.serverName << "]" << " Port [" << argData.serverPort << "]" << " User [" << argData.userName << "]";
        std::cout << " Remote Directory [" << argData.remoteDirectory << "]" << " Local Directory [" << argData.localDirectory << "]\n" << std::endl;

        // Set server and port

        ftpServer.setServerAndPort(argData.serverName, argData.serverPort);

        // Set FTP account user name and password

        ftpServer.setUserAndPassword(argData.userName, argData.userPassword);

        // Enable SSL

        ftpServer.setSslEnabled(true);

        do {

            // Connect

            if (ftpServer.connect() != 230) {
                throw CFTP::Exception("Unable to connect status returned = " + ftpServer.getCommandResponse());
            }

            // Create remote directory

            if (!ftpServer.fileExists(argData.remoteDirectory)) {
                makeRemotePath(ftpServer, argData.remoteDirectory);
                if (!ftpServer.fileExists(argData.remoteDirectory)) {
                    throw CFTP::Exception("Remote FTP server directory " + argData.remoteDirectory + " could not created.");
                }
            }

            ftpServer.changeWorkingDirectory(argData.remoteDirectory);
            ftpServer.getCurrentWoringDirectory(argData.remoteDirectory);
            
            std::cout << "Current Working Direcory [" << argData.remoteDirectory << "]\n" << std::endl;

            // Get local and remote file lists

            loadFilesToSynchronize(ftpServer, argData, remoteFiles, localFiles);

            // PASS 1) Copy new files to server

            std::cout << "*** Transferring any new files to server ***" << std::endl;

            std::vector<std::string> newFiles;
               for (auto &file : localFiles) {
                if (remoteFiles.find(localFileToRemote(argData, file.first)) == remoteFiles.end()) {
                    newFiles.push_back( file.first );
                }
            }
            
            if (!newFiles.empty()) {
                sort(newFiles.begin(), newFiles.end());  // Putfiles() requires list to be sorted
                FileInfoMap newFilesTransfered{ getRemoteFileListDateTime(ftpServer, putFiles(ftpServer, argData.localDirectory, newFiles))};
                std::cout << "Number of new files transfered [" << newFilesTransfered.size() << "]" << std::endl;
                remoteFiles.insert(newFilesTransfered.begin(), newFilesTransfered.end());
            }

            // PASS 2) Remove any deleted local files from server and local cache

            std::cout << "*** Removing any deleted local files from server ***" << std::endl;

            for (auto fileIter = remoteFiles.begin(); fileIter != remoteFiles.end();) {
                if (localFiles.find(remoteFileToLocal(argData, fileIter->first)) == localFiles.end()) {
                    if (ftpServer.deleteFile(fileIter->first) == 250) {
                        fileIter = remoteFiles.erase(fileIter);
                        std::cout << "File [" << fileIter->first << " ] removed from server." << std::endl;
                    } else if (ftpServer.removeDirectory(fileIter->first) == 250) {
                        fileIter = remoteFiles.erase(fileIter);
                        std::cout << "Directory [" << fileIter->first << " ] removed from server." << std::endl;
                    } else {
                        fileIter++;
                        std::cerr << "File [" << fileIter->first << " ] could not be removed from server." << std::endl;
                    }
                } else {
                    fileIter++;
                }
            }

            // PASS 3) Copy any updated local files to remote server. Note: PASS 2 may
            // have deleted some remote files but if the get modified date/time fails
            // it is ignored and not added to remoteFileModifiedTimes.

            std::cout << "*** Copying updated local files to server ***" << std::endl;

            // Copy updated files to server

            for (auto file : localFiles) {
                if (fs::is_regular_file(file.first)) {
                    if (remoteFiles[localFileToRemote(argData, file.first)] < localFiles[file.first]) {
                        std::cout << "Server file " << localFileToRemote(argData, file.first) << " out of date." << std::endl;
                        if (ftpServer.putFile(localFileToRemote(argData, file.first), file.first) == 226) {
                            std::cout << "File [" << file.first << " ] copied to server." << std::endl;
                            remoteFiles[localFileToRemote(argData, file.first)] = localFiles[file.first];
                        } else {
                            std::cerr << "File [" << file.first << " ] not copied to server." << std::endl;
                        }
                    }
                }
            }

            if (localFiles.size() != remoteFiles.size()) {
                std::cerr << "FTP server seems to be out of sync with local directory." << std::endl;
            }
            
            // Disconnect 

            ftpServer.disconnect();

            std::cout << "*** Files synchronized with server ***\n" << std::endl;
            
            // Write away file cache
            
            saveFilesToCache(argData, remoteFiles, localFiles);

            // Wait poll interval (pollTime == 0 then one pass)

            std::this_thread::sleep_for(std::chrono::minutes(argData.pollTime));

        } while (argData.pollTime);

        //
        // Catch any errors
        //    

    } catch (CFTP::Exception &e) {
        exitWithError(e.what());
    } catch (std::exception &e) {
        exitWithError(std::string("Standard exception occured: [") + e.what() + "]");
    }

    exit(EXIT_SUCCESS);

}