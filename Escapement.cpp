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
//
// Dependencies:
//
// C11++              : Use of C11++ features.
// Antikythera Classes: CFTP, CSocket. 
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
// Program components.
//

#include "Escapement.hpp"
#include "Escapement_CommandLine.hpp"
#include "Escapement_Files.hpp"

//
// Antik Classes
//

#include "FTPUtil.hpp"

//
// Boost file system library
//

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

// =========
// NAMESPACE
// =========

namespace Escapement {

    // =======
    // IMPORTS
    // =======

    using namespace std;

    using namespace Escapement_CommandLine;
    using namespace Escapement_Files;

    using namespace Antik::FTP;

    // ===============
    // LOCAL FUNCTIONS
    // ===============

    //
    // Exit with error message/status.
    //

    static void exitWithError(string errMsg) {

        // Display error and exit.

        cerr << errMsg << endl;
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

            cout << "*** Current Working Directory [" << runContext.optionData.remoteDirectory << "] ***" << endl;

        } catch (...) {
            cerr << "Escapement error: Failed to connect to server." << endl;
        }

    }

    //
    // Refresh local file cache from local directory and local server.
    //

    static void refreshFileCache(EscapementRunContext &runContext) {

        cout << "*** Refresh file cache ***" << endl;

        // Connect to server

        connectToServer(runContext);

        // If connection successful refresh file list cache

        if (runContext.ftpServer.isConnected()) {

            // Get all remote file information for pull

            cout << "*** Refreshing file list from remote directory... ***" << endl;

            getAllRemoteFiles(runContext);

            cout << "*** Refreshing file list from local directory... ***" << endl;

            getAllLocalFiles(runContext);

            // Report disparity in number of files

            if (runContext.localFiles.size() != runContext.remoteFiles.size()) {
                cerr << "Not all files pulled from FTP server." << endl;
                if (!runContext.ftpServer.isConnected()) {
                    cerr << "FTP server disconnected unexpectedly." << endl;
                }
            }

            // Disconnect from server

            runContext.ftpServer.disconnect();

            cout << "*** File cache refreshed ***\n" << endl;

            // Save refreshed file lists

            saveFilesAfterSynchronise(runContext);

        }

    }

    //
    // Pull files from server.
    //

    static void pullFilesFromServer(EscapementRunContext &runContext) {

        cout << "*** Pulling remote Files ***" << endl;

        // Connect to server

        connectToServer(runContext);

        // If connection successful pull files from server

        if (runContext.ftpServer.isConnected()) {

            // Get all remote file information for pull

            cout << "*** Getting file list from remote directory... ***" << endl;

            getAllRemoteFiles(runContext);

            for (auto &file : runContext.remoteFiles) {
                runContext.filesToProcess.push_back(file.first);
            }

            // Get non empty list

            if (!runContext.filesToProcess.empty()) {
                cout << "*** Pulling " << runContext.filesToProcess.size() << " files from server. ***" << endl;
                pullFiles(runContext);
            }

            // Report disparity in number of files

            if (runContext.localFiles.size() != runContext.remoteFiles.size()) {
                cerr << "Not all files pulled from FTP server." << endl;
                if (!runContext.ftpServer.isConnected()) {
                    cerr << "FTP server disconnected unexpectedly." << endl;
                }
            }

            // Disconnect 

            runContext.ftpServer.disconnect();

            if (!runContext.filesToProcess.empty()) {

                cout << "*** Files pulled from server ***\n" << endl;

                // Make remote modified time the same as local to enable sync to work.

                for (auto &file : runContext.localFiles) {
                    runContext.remoteFiles[convertFilePath(runContext.optionData, file.first)] = file.second;
                }

                // Saved file list after pull

                saveFilesAfterSynchronise(runContext);

            } else {
                cout << "*** No files pulled from server ***\n" << endl;
            }

        }

    }

    //
    // Synchronise files with server.
    //

    static void sychroniseFiles(EscapementRunContext &runContext) {

        do {

            cout << "*** Sychronizing Files ***" << endl;

            // Connect to server

            connectToServer(runContext);

            // If connection successful synchronise

            if (runContext.ftpServer.isConnected()) {

                // Get local and remote file information for synchronise

                cout << "*** Getting local/remote file lists... ***" << endl;

                loadFilesBeforeSynchronise(runContext);

                // PASS 1) Copy new/updated files to server

                cout << "*** Determining new/updated file list..***" << endl;

                for (auto &file : runContext.localFiles) {
                    auto remoteFile = runContext.remoteFiles.find(convertFilePath(runContext.optionData, file.first));
                    if ((remoteFile == runContext.remoteFiles.end()) || (remoteFile->second < file.second)) {
                        runContext.filesToProcess.push_back(file.first);
                    }
                }

                // Push non empty list

                if (!runContext.filesToProcess.empty()) {
                    runContext.totalFilesProcessed += runContext.filesToProcess.size();
                    cout << "*** Transferring " << runContext.filesToProcess.size() << " new/updated files to server ***" << endl;
                    pushFiles(runContext);
                }

                // PASS 2) Remove any deleted local files/directories from server and local cache

                cout << "*** Determining local files deleted..***" << endl;

                runContext.filesToProcess.clear();
                for (auto &file : runContext.remoteFiles) {
                    if (runContext.localFiles.find(convertFilePath(runContext.optionData, file.first)) == runContext.localFiles.end()) {
                        runContext.filesToProcess.push_back(file.first);
                    }
                }

                // Delete non empty list

                if (!runContext.filesToProcess.empty()) {
                    runContext.totalFilesProcessed += runContext.filesToProcess.size();
                    cout << "*** Removing " << runContext.filesToProcess.size() << " deleted local files from server ***" << endl;
                    deleteFiles(runContext);
                }

                // Report disparity in number of files

                if (runContext.localFiles.size() != runContext.remoteFiles.size()) {
                    cerr << "FTP server seems to be out of sync with local directory." << endl;
                    if (!runContext.ftpServer.isConnected()) {
                        cerr << "FTP server disconnected unexpectedly." << endl;
                    }
                }

                // Disconnect 

                runContext.ftpServer.disconnect();

                // Saved file list after synchronise

                if (runContext.totalFilesProcessed) {
                    saveFilesAfterSynchronise(runContext);
                    cout << "*** Files synchronised with server ***\n" << endl;
                } else {
                    cout << "*** No files synchronised. ***\n" << endl;
                }

            }

            // Wait poll interval (pollTime == 0 then one pass)

            if (runContext.optionData.pollTime) {
                cout << "*** Waiting " << runContext.optionData.pollTime << " minutes for next synchronise... ***\n" << endl;
                this_thread::sleep_for(chrono::minutes(runContext.optionData.pollTime));
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

            cout << "Server [" << runContext.optionData.serverName << "]" << " Port [" << runContext.optionData.serverPort << "]" << " User [" << runContext.optionData.userName << "]";
            cout << " Remote Directory [" << runContext.optionData.remoteDirectory << "]" << " Local Directory [" << runContext.optionData.localDirectory << "]";
            cout << " SSL [" << ((runContext.optionData.noSSL) ? "Off" : "On") << "]\n" << endl;

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

        //
        // Catch any errors
        //    

        } catch (const CFTP::Exception &e) {
            exitWithError(e.what());
        } catch (const boost::filesystem::filesystem_error & e) {
            exitWithError(string("BOOST file system exception occured: [") + e.what() + "]");
        } catch (const exception &e) {
            exitWithError(string("Standard exception occured: [") + e.what() + "]");
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
