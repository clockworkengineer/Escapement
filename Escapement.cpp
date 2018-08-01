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
// Description: Simple FTP based client program that takes a local directory and 
// keeps it synchronised with a remote server directory. To save on FTP requests it can 
// keep a local JSON file that contains a cache of remote file details.
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
//   -m [ --command ] arg   Command: 0 (Synchronise), 1 (Pull) , 2 (Refresh cache)
//   -n [ --nossl ]         Switch off ssl for connection
//   -v [ --override ]      Override any command line options from cache file
//
// Dependencies:
//
// C11++              : Use of C11++ features.
// Antik Classes      : CFTP, CSocket, CFile. 
// Linux              : Target platform
// Boost              : File system, program option, iterator.
// Misc.              : Lohmann JSON library.
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

//
// Program components.
//

#include "Escapement.hpp"
#include "Escapement_CommandLine.hpp"
#include "Escapement_Files.hpp"

// =========
// NAMESPACE
// =========

namespace Escapement {

    // =======
    // IMPORTS
    // =======

    using namespace Antik::FTP;
    using namespace Antik::File;

    using namespace Escapement_CommandLine;
    using namespace Escapement_Files;

    // ===============
    // LOCAL FUNCTIONS
    // ===============

    //
    // Exit with error message/status.
    //

    static void exitWithError(std::string errMsg) {

        // Display error and exit.

        std::cerr << errMsg << std::endl;
        exit(EXIT_FAILURE);

    }

    //
    // Set up FTP parameters and connect to server.
    //

    static void connectToServer(EscapementRunContext &runContext) {

        try {

            // Set server and port

            runContext.ftpServer.setServerAndPort(runContext.optionData.serverName, runContext.optionData.serverPort);

            // Set FTP account user name and password

            runContext.ftpServer.setUserAndPassword(runContext.optionData.userName, runContext.optionData.userPassword);

            // Set SSL

            runContext.ftpServer.setSslEnabled(!runContext.optionData.noSSL);

            // Connect

            if (runContext.ftpServer.connect() != 230) {
                throw CFTP::Exception("Unable to connect status returned = " + runContext.ftpServer.getCommandResponse());
            }

            // Binary transfer more on

            runContext.ftpServer.setBinaryTransfer(true);

            // Remote directory does not exist so create

            if (!runContext.ftpServer.fileExists(runContext.optionData.remoteDirectory)) {
                makeRemotePath(runContext.ftpServer, runContext.optionData.remoteDirectory);
                if (!runContext.ftpServer.fileExists(runContext.optionData.remoteDirectory)) {
                    runContext.ftpServer.disconnect();
                    throw CFTP::Exception("Remote FTP server directory " + runContext.optionData.remoteDirectory + " could not be created.");
                }
            }

            // Overwrite remote directory with server path for it

            runContext.ftpServer.changeWorkingDirectory(runContext.optionData.remoteDirectory);
            runContext.ftpServer.getCurrentWoringDirectory(runContext.optionData.remoteDirectory);

            std::cout << "*** Current Working Directory [" << runContext.optionData.remoteDirectory << "] ***" << std::endl;

        } catch (...) {
            std::cerr << "Escapement error: Failed to connect to server." << std::endl;
        }

    }

    //
    // Refresh local file cache from local directory and local server.
    //

    static void refreshFileCache(EscapementRunContext &runContext) {

        std::cout << "*** Refresh file cache ***" << std::endl;

        // Connect to server

        connectToServer(runContext);

        // If connection successful refresh file list cache

        if (runContext.ftpServer.isConnected()) {

            // Get all remote file information for pull

            std::cout << "*** Refreshing file list from remote directory... ***" << std::endl;

            getAllRemoteFiles(runContext);

            std::cout << "*** Refreshing file list from local directory... ***" << std::endl;

            getAllLocalFiles(runContext);

            // Report disparity in number of files

            if (runContext.localFiles.size() != runContext.remoteFiles.size()) {
                std::cerr << "Not all files pulled from FTP server." << std::endl;
                if (!runContext.ftpServer.isConnected()) {
                    std::cerr << "FTP server disconnected unexpectedly." << std::endl;
                }
            }

            // Disconnect from server

            runContext.ftpServer.disconnect();

            std::cout << "*** File cache refreshed ***\n" << std::endl;

            // Save refreshed file lists

            saveFilesAfterSynchronise(runContext);

        }

    }

    //
    // Pull files from server.
    //

    static void pullFilesFromServer(EscapementRunContext &runContext) {

        std::cout << "*** Pulling remote Files ***" << std::endl;

        // Connect to server

        connectToServer(runContext);

        // If connection successful pull files from server

        if (runContext.ftpServer.isConnected()) {

            // Get all remote file information for pull

            std::cout << "*** Getting file list from remote directory... ***" << std::endl;

            getAllRemoteFiles(runContext);

            for (auto &file : runContext.remoteFiles) {
                runContext.filesToProcess.push_back(file.first);
            }

            // Get non empty list

            if (!runContext.filesToProcess.empty()) {
                std::cout << "*** Pulling " << runContext.filesToProcess.size() << " files from server. ***" << std::endl;
                pullFiles(runContext);
            }

            // Report disparity in number of files

            if (runContext.localFiles.size() != runContext.remoteFiles.size()) {
                std::cerr << "Not all files pulled from FTP server." << std::endl;
                if (!runContext.ftpServer.isConnected()) {
                    std::cerr << "FTP server disconnected unexpectedly." << std::endl;
                }
            }

            // Disconnect 

            runContext.ftpServer.disconnect();

            if (!runContext.filesToProcess.empty()) {

                std::cout << "*** Files pulled from server ***\n" << std::endl;

                // Make remote modified time the same as local to enable sync to work.

                for (auto &file : runContext.localFiles) {
                    runContext.remoteFiles[convertFilePath(runContext.optionData, file.first)] = file.second;
                }

                // Save file lists after pull

                saveFilesAfterSynchronise(runContext);

            } else {
                std::cout << "*** No files pulled from server ***\n" << std::endl;
            }

        }

    }

    //
    // Synchronise files with server.
    //

    static void sychroniseFiles(EscapementRunContext &runContext) {

        do {

            std::cout << "*** Sychronizing Files ***" << std::endl;

            // Connect to server

            connectToServer(runContext);

            // If connection successful synchronise

            if (runContext.ftpServer.isConnected()) {

                // Get local and remote file information for synchronise

                std::cout << "*** Getting local/remote file lists... ***" << std::endl;

                loadFilesBeforeSynchronise(runContext);

                // PASS 1) Copy new/updated files to server

                std::cout << "*** Determining new/updated file list..***" << std::endl;

                for (auto &file : runContext.localFiles) {
                    auto remoteFile = runContext.remoteFiles.find(convertFilePath(runContext.optionData, file.first));
                    if ((remoteFile == runContext.remoteFiles.end()) || (remoteFile->second < file.second)) {
                        runContext.filesToProcess.push_back(file.first);
                    }
                }

                // Push non empty list

                if (!runContext.filesToProcess.empty()) {
                    std::cout << "*** Transferring " << runContext.filesToProcess.size() << " new/updated files to server ***" << std::endl;
                    pushFiles(runContext);
                }

                // PASS 2) Remove any deleted local files/directories from server and local cache

                std::cout << "*** Determining local files deleted..***" << std::endl;

                runContext.filesToProcess.clear();
                for (auto &file : runContext.remoteFiles) {
                    if (runContext.localFiles.find(convertFilePath(runContext.optionData, file.first)) == runContext.localFiles.end()) {
                        runContext.filesToProcess.push_back(file.first);
                    }
                }

                // Delete non empty list

                if (!runContext.filesToProcess.empty()) {
                    std::cout << "*** Removing " << runContext.filesToProcess.size() << " deleted local files from server ***" << std::endl;
                    deleteFiles(runContext);
                }

                // Report disparity in number of files

                if (runContext.localFiles.size() != runContext.remoteFiles.size()) {
                    std::cerr << "FTP server seems to be out of sync with local directory." << std::endl;
                    if (!runContext.ftpServer.isConnected()) {
                        std::cerr << "FTP server disconnected unexpectedly." << std::endl;
                    }
                }

                // Disconnect 

                runContext.ftpServer.disconnect();

                // Saved file list after synchronise

                if (runContext.totalFilesProcessed) {
                    saveFilesAfterSynchronise(runContext);
                    std::cout << "*** Files synchronised with server ***\n" << std::endl;
                } else {
                    std::cout << "*** No files synchronised. ***\n" << std::endl;
                }

            }

            // Wait poll interval (pollTime == 0 then one pass)

            if (runContext.optionData.pollTime) {
                std::cout << "*** Waiting " << runContext.optionData.pollTime << " minutes for next synchronise... ***\n" << std::endl;
                std::this_thread::sleep_for(std::chrono::minutes(runContext.optionData.pollTime));
                runContext.localFiles.clear();
                runContext.remoteFiles.clear();
                runContext.filesToProcess.clear();
                runContext.totalFilesProcessed = 0;
            }

        } while (runContext.optionData.pollTime);

    }

    // ================
    // PUBLIC FUNCTIONS
    // ================

    void Escapement(int argc, char** argv) {

        try {

            EscapementRunContext runContext;

            // Read in command line parameters and process

            runContext.optionData = fetchCommandLineOptions(argc, argv);

            // Display run parameters

            std::cout << "Server [" << runContext.optionData.serverName << "]" << " Port [" << runContext.optionData.serverPort << "]" << " User [" << runContext.optionData.userName << "]";
            std::cout << " Remote Directory [" << runContext.optionData.remoteDirectory << "]" << " Local Directory [" << runContext.optionData.localDirectory << "]";
            std::cout << " SSL [" << ((runContext.optionData.noSSL) ? "Off" : "On") << "]\n" << std::endl;

            switch (runContext.optionData.command) {
                case kEscapementSynchronise:
                    sychroniseFiles(runContext);
                    break;
                case kEscapementPullFiles:
                    pullFilesFromServer(runContext);
                    break;
                case kEscapementRefreshCache:
                    refreshFileCache(runContext);
                    break;
            }
        } catch (const std::exception &e) {
            exitWithError(e.what());
        }

    }

} // namespace Escapement

// ============================
// ===== MAIN ENTRY POint =====
// ============================

int main(int argc, char** argv) {

    Escapement::Escapement(argc, argv);
    exit(EXIT_SUCCESS);

}
