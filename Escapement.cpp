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
    // Exit with error message/status
    //

    static void exitWithError(string errMsg) {

        // Display error and exit.

        cerr << errMsg << endl;
        exit(EXIT_FAILURE);

    }

    // ================
    // PUBLIC FUNCTIONS
    // ================

    void SychronizeFiles(int argc, char** argv) {

        try {

            EscapementOptions optionData;
            CFTP ftpServer;
            FileInfoMap localFiles;
            FileInfoMap remoteFiles;

            // Read in command line parameters and process

            optionData = fetchCommandLineOptions(argc, argv);

            cout << "Server [" << optionData.serverName << "]" << " Port [" << optionData.serverPort << "]" << " User [" << optionData.userName << "]";
            cout << " Remote Directory [" << optionData.remoteDirectory << "]" << " Local Directory [" << optionData.localDirectory << "]\n" << endl;

            // Set server and port

            ftpServer.setServerAndPort(optionData.serverName, optionData.serverPort);

            // Set FTP account user name and password

            ftpServer.setUserAndPassword(optionData.userName, optionData.userPassword);

            // Enable SSL

            ftpServer.setSslEnabled(true);

            do {

                // Connect

                if (ftpServer.connect() != 230) {
                    throw CFTP::Exception("Unable to connect status returned = " + ftpServer.getCommandResponse());
                }

                // Create remote directory

                if (!ftpServer.fileExists(optionData.remoteDirectory)) {
                    makeRemotePath(ftpServer, optionData.remoteDirectory);
                    if (!ftpServer.fileExists(optionData.remoteDirectory)) {
                        throw CFTP::Exception("Remote FTP server directory " + optionData.remoteDirectory + " could not created.");
                    }
                }

                ftpServer.changeWorkingDirectory(optionData.remoteDirectory);
                ftpServer.getCurrentWoringDirectory(optionData.remoteDirectory);

                cout << "Current Working Direcory [" << optionData.remoteDirectory << "]\n" << endl;

                // Get local and remote file information for synchronise

                loadFilesBeforeSynchronise(ftpServer, optionData, remoteFiles, localFiles);

                // PASS 1) Copy new/updated files to server

                cout << "*** Transferring any new/updated files to server ***" << endl;

                vector<string> filesToTransfer;
                for (auto &file : localFiles) {
                    auto remoteFile = remoteFiles.find(localFileToRemote(optionData, file.first));
                    if ( (remoteFile == remoteFiles.end()) || (remoteFile->second < file.second)) {
                        filesToTransfer.push_back(file.first);
                    } 
                }

                if (!filesToTransfer.empty()) {
                    sort(filesToTransfer.begin(), filesToTransfer.end()); // Putfiles() requires list to be sorted
                    FileInfoMap filesTransfered{ getRemoteFileListDateTime(ftpServer, putFiles(ftpServer, optionData.localDirectory, filesToTransfer))};
                    if (!filesTransfered.empty()) {
                        cout << "Number of files to transfer [" << filesTransfered.size() << "]" << endl;
                        for (auto &file : filesTransfered) {
                            remoteFiles[file.first] = file.second;
                            cout << "File [" << file.first << "]" << endl;
                        }
                    } else {
                        cerr << "None of the selected files transferred." << endl;
                    }
                }

                // PASS 2) Remove any deleted local files/directories from server and local cache

                cout << "*** Removing any deleted local files from server ***" << endl;

                for (auto fileIter = remoteFiles.begin(); fileIter != remoteFiles.end();) {
                    if (localFiles.find(remoteFileToLocal(optionData, fileIter->first)) == localFiles.end()) {
                        if (ftpServer.deleteFile(fileIter->first) == 250) {
                            cout << "File [" << fileIter->first << " ] removed from server." << endl;
                            fileIter = remoteFiles.erase(fileIter);
                        } else if (ftpServer.removeDirectory(fileIter->first) == 250) {
                            cout << "Directory [" << fileIter->first << " ] removed from server." << endl;
                            fileIter = remoteFiles.erase(fileIter);
                        } else {
                            cerr << "File [" << fileIter->first << " ] could not be removed from server." << endl;
                            fileIter++;
                        }
                    } else {
                        fileIter++;
                    }
                }

                if (localFiles.size() != remoteFiles.size()) {
                    cerr << "FTP server seems to be out of sync with local directory." << endl;
                }

                // Disconnect 

                ftpServer.disconnect();

                cout << "*** Files synchronized with server ***\n" << endl;

                // Saved file list after synchronise

                saveFilesAfterSynchronise(ftpServer, optionData, remoteFiles, localFiles);

                // Wait poll interval (pollTime == 0 then one pass)

                this_thread::sleep_for(chrono::minutes(optionData.pollTime));

            } while (optionData.pollTime);

            //
            // Catch any errors
            //    

        } catch (CFTP::Exception &e) {
            exitWithError(e.what());
        } catch (exception &e) {
            exitWithError(string("Standard exception occured: [") + e.what() + "]");
        }

        exit(EXIT_SUCCESS);

    }

} // namespace Escapement

// ============================
// ===== MAIN ENTRY POint =====
// ============================

int main(int argc, char** argv) {
    
    Escapement::SychronizeFiles(argc, argv);
    exit(EXIT_SUCCESS);

}
