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

    void getAllRemoteFiles(Antik::FTP::CFTP &ftpServer, const std::string &remoteDirectory, Escapement::FileInfoMap &remoteFiles);
    std::string convertFilePath(const Escapement::EscapementOptions &optionData, const std::string &filePath);
    Escapement::FileInfoMap getLocalFileListDateTime(const Antik::FTP::FileList &fileList);
    Escapement::FileInfoMap getRemoteFileListDateTime(Antik::FTP::CFTP &ftpServer, const Antik::FTP::FileList &fileList);
    void pullFiles (Antik::FTP::CFTP &ftpServer, const Escapement::EscapementOptions &optionData, Escapement::FileInfoMap &localFiles, Antik::FTP::FileList &filesToTransfer);
    void pushFiles (Antik::FTP::CFTP &ftpServer, const Escapement::EscapementOptions &optionData, Escapement::FileInfoMap &remoteFiles, Antik::FTP::FileList &filesToTransfer);
    void deleteFiles (Antik::FTP::CFTP &ftpServer, const Escapement::EscapementOptions &optionData, Escapement::FileInfoMap &remoteFiles, Antik::FTP::FileList &filesToDelete);
    void loadFilesBeforeSynchronise(Antik::FTP::CFTP  &ftpServer, const Escapement::EscapementOptions &optionData, Escapement::FileInfoMap &remoteFiles, Escapement::FileInfoMap &localFiles);
    void saveFilesAfterSynchronise(Antik::FTP::CFTP &ftpServer, const Escapement::EscapementOptions &optionData, Escapement::FileInfoMap &remoteFiles, Escapement::FileInfoMap &localFiles);   

} // namespace Escapement_Files

#endif /* ESCAPEMENT_FILES_HPP */


