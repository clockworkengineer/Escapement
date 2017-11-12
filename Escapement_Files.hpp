/*
 * File:   Escapement_Files.hpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2017, 2:33 PM
 * 
 * Copyright 2017.
 * 
 */

#ifndef ESCAPEMENT_FILES_HPP
#define ESCAPEMENT_FILES_HPP

//
// C++ STL
//

#include <string>

//
// Escapement components
//

#include "Escapement.hpp"
#include "Escapement_CommandLine.hpp"

// =========
// NAMESPACE
// =========

namespace Escapement_Files {

    std::string localFileToRemote(Escapement_CommandLine::EscapementOptions &optionData, const std::string &localFilePath);
    std::string remoteFileToLocal(Escapement_CommandLine::EscapementOptions &optionData, const std::string &remoteFilePath);
    Escapement::FileInfoMap getRemoteFileListDateTime(Antik::FTP::CFTP &ftpServer, const std::vector<std::string> &fileList);
    void loadFilesBeforeSynchronise(Antik::FTP::CFTP  &ftpServer, Escapement_CommandLine::EscapementOptions &optionData, Escapement::FileInfoMap &remoteFiles, Escapement::FileInfoMap &localFiles);
    void saveFilesAfterSynchronise(Antik::FTP::CFTP &ftpServer, Escapement_CommandLine::EscapementOptions &optionData, Escapement::FileInfoMap &remoteFiles, Escapement::FileInfoMap &localFiles);   

} // namespace Escapement_Files

#endif /* ESCAPEMENT_FILES_HPP */


