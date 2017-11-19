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

    //
    // Pull files from server.
    //
    
    static void PullFiles(EscapementOptions &optionData) {

        CFTP ftpServer;
        FileInfoMap localFiles;
        FileInfoMap remoteFiles;
        vector<string> filesToProcess;

        cout << "*** Pulling remote Files ***" << endl;

        // Set server and port

        ftpServer.setServerAndPort(optionData.serverName, optionData.serverPort);

        // Set FTP account user name and password

        ftpServer.setUserAndPassword(optionData.userName, optionData.userPassword);

        // Enable SSL

        ftpServer.setSslEnabled(false);

        // Connect

        if (ftpServer.connect() != 230) {
            throw CFTP::Exception("Unable to connect status returned = " + ftpServer.getCommandResponse());
        }

        // Binary transfer more on
        
        ftpServer.setBinaryTransfer(true);
                    
        // Remote directory does not exist.

        if (!ftpServer.fileExists(optionData.remoteDirectory)) {
            throw CFTP::Exception("Remote FTP server directory " + optionData.remoteDirectory + " does  not exist.");
        }

        ftpServer.changeWorkingDirectory(optionData.remoteDirectory);
        ftpServer.getCurrentWoringDirectory(optionData.remoteDirectory);

        cout << "*** Current Working Directory [" << optionData.remoteDirectory << "] ***" << endl;

        // Get all remote file information for pull
        
        cout << "*** Getting file list from remote directory... ***" << endl;

        getAllRemoteFiles(ftpServer, optionData.remoteDirectory, remoteFiles);
        
        for (auto &file : remoteFiles) {
             filesToProcess.push_back(file.first);
        }
        
        // Get non empty list
        
        if (!filesToProcess.empty()) {
            cout << "*** Pulling " << filesToProcess.size() << " files from server. ***" << endl;
            pullFiles(ftpServer, optionData, localFiles, filesToProcess);
        }

        // Report disparity in number of files
        
        if (localFiles.size() != remoteFiles.size()) {
            cerr << "Not all files pulled from FTP server." << endl;
        }

        // Disconnect 

        ftpServer.disconnect();

        cout << "*** Files pulled from server ***\n" << endl;

        // Make remote modified time the same as local so enable sync to work.
        
        for (auto &file : localFiles) {
             remoteFiles[convertFilePath(optionData, file.first)] = file.second;
        }

        // Saved file list after synchronise
  
        saveFilesAfterSynchronise(ftpServer, optionData, remoteFiles, localFiles);


    }
    
    //
    // Synchronise files with server.
    //
    
    static void SychroniseFiles(EscapementOptions &optionData) {

        CFTP ftpServer;
        FileInfoMap localFiles;
        FileInfoMap remoteFiles;
        vector<string> filesToProcess;

        cout << "*** Sychronizing Files ***" << endl;

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

            // Binary transfer more on

            ftpServer.setBinaryTransfer(true);
            
            // Create remote directory

            if (!ftpServer.fileExists(optionData.remoteDirectory)) {
                makeRemotePath(ftpServer, optionData.remoteDirectory);
                if (!ftpServer.fileExists(optionData.remoteDirectory)) {
                    throw CFTP::Exception("Remote FTP server directory " + optionData.remoteDirectory + " could not created.");
                }
            }

            ftpServer.changeWorkingDirectory(optionData.remoteDirectory);
            ftpServer.getCurrentWoringDirectory(optionData.remoteDirectory);

            cout << "*** Current Working Directory [" << optionData.remoteDirectory << "] ***" << endl;

            // Get local and remote file information for synchronise
            
            cout << "*** Getting local/remote file lists... ***" << endl;

            loadFilesBeforeSynchronise(ftpServer, optionData, remoteFiles, localFiles);

            // PASS 1) Copy new/updated files to server

            for (auto &file : localFiles) {
                auto remoteFile = remoteFiles.find(convertFilePath(optionData, file.first));
                if ((remoteFile == remoteFiles.end()) || (remoteFile->second < file.second)) {
                    filesToProcess.push_back(file.first);
                }
            }

            // Push non empty list
            
            if (!filesToProcess.empty()) {
                cout << "*** Transferring " << filesToProcess.size() << " new/updated files to server ***" << endl;
                pushFiles(ftpServer, optionData, remoteFiles, filesToProcess);
            }

            // PASS 2) Remove any deleted local files/directories from server and local cache
 
            filesToProcess.clear();
            for (auto &file : remoteFiles) {
                if (localFiles.find(convertFilePath(optionData, file.first)) == localFiles.end()) {
                    filesToProcess.push_back(file.first);
                }
            }

            // Delete non empty list
            
            if (!filesToProcess.empty()) {
                cout << "*** Removing " << filesToProcess.size() << " deleted local files from server ***" << endl;
                deleteFiles(ftpServer, optionData, remoteFiles, filesToProcess);
            }

            // Report disparity in number of files
            
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

    }

    // ================
    // PUBLIC FUNCTIONS
    // ================

    void Escapement(int argc, char** argv) {

        try {

            EscapementOptions optionData;

            // Read in command line parameters and process

            optionData = fetchCommandLineOptions(argc, argv);

            // Display run parameters

            cout << "Server [" << optionData.serverName << "]" << " Port [" << optionData.serverPort << "]" << " User [" << optionData.userName << "]";
            cout << " Remote Directory [" << optionData.remoteDirectory << "]" << " Local Directory [" << optionData.localDirectory << "]\n" << endl;

            if (optionData.pullFromServer) {
                PullFiles(optionData);       // Pull files
            } else {                    
                SychroniseFiles(optionData); // Synchronise files
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
