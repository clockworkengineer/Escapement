#include "HOST.hpp"
/*
 * File:   Escapement_CommandLine.cpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2017, 2:33 PM
 * 
 * Copyright 2017.
 * 
 */

//
// Module: Escapement_CommandLine
//
// Description: Escapement program options processing functionality.
// 
// Dependencies: 
// 
// C11++              : Use of C11++ features.
// Boost              : File system, program option,.
//

// =============
// INCLUDE FILES
// =============

//
// C++ STL
//

#include <iostream>

//
// Escapement command line
//

#include "Escapement_CommandLine.hpp"

//
// Boost file system & program options
//

#include "boost/program_options.hpp" 
#include <boost/filesystem.hpp>

// =========
// NAMESPACE
// =========

namespace Escapement_CommandLine {

    // =======
    // IMPORTS
    // =======

    using namespace std;
        
    namespace po = boost::program_options;
    namespace fs = boost::filesystem;

    // ===============
    // LOCAL FUNCTIONS
    // ===============

    //
    // Add options common to both command line and config file
    //

  static void addCommonOptions(po::options_description& commonOptions, EscapementOptions & optionData) {

        commonOptions.add_options()
                ("server,s", po::value<std::string>(&optionData.serverName)->required(), "FTP Server name")
                ("port,o", po::value<std::string>(&optionData.serverPort)->required(), "FTP Server port")
                ("user,u", po::value<std::string>(&optionData.userName)->required(), "Account username")
                ("password,p", po::value<std::string>(&optionData.userPassword)->required(), "User password")
                ("remote,r", po::value<std::string>(&optionData.remoteDirectory)->required(), "Remote directory to restore")
                ("local,l", po::value<std::string>(&optionData.localDirectory)->required(), "Local directory as base for restore")
                ("cache,c", po::value<std::string>(&optionData.fileCache), "JSON file cache")
                ("polltime,t", po::value<int>(&optionData.pollTime), "Server poll time in minutes")
                ("pull", "Pull files from server to local directory");

    }

    // ================
    // PUBLIC FUNCTIONS
    // ================

    //
    // Read in and process command line options using boost.
    //

    EscapementOptions fetchCommandLineOptions(int argc, char** argv) {

        EscapementOptions optionData;

        // Define and parse the program options

        po::options_description commandLine("Program Options");
        commandLine.add_options()
                ("help", "Print help messages")
                ("config,c", po::value<std::string>(&optionData.configFileName), "Config File Name");

        addCommonOptions(commandLine, optionData);

        po::options_description configFile("Config Files Options");

        addCommonOptions(configFile, optionData);

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

            if (vm.count("pull")) {
                optionData.pullFromServer=true;
            }
            
            po::notify(vm);

        } catch (po::error& e) {
            std::cerr << "Escapement Error: " << e.what() << std::endl << std::endl;
            std::cerr << commandLine << std::endl;
            exit(EXIT_FAILURE);
        }

        return(optionData);
        
    }

} // namespace Escapement_CommandLine

