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

    void getAllRemoteFiles(Escapement::EscapementRunContext &runContext);
    std::string convertFilePath(const Escapement::EscapementOptions &optionData, const std::string &filePath);
    Escapement::FileInfoMap getLocalFileListDateTime(const Antik::FTP::FileList &fileList);
    Escapement::FileInfoMap getRemoteFileListDateTime(Antik::FTP::CFTP &ftpServer, const Antik::FTP::FileList &fileList);
    void pullFiles (Escapement::EscapementRunContext &runContext, Antik::FTP::FileList &filesToTransfer);
    void pushFiles (Escapement::EscapementRunContext &runContext, Antik::FTP::FileList &filesToTransfer);
    void deleteFiles (Escapement::EscapementRunContext &runContext, Antik::FTP::FileList &filesToDelete);
    void loadFilesBeforeSynchronise(Escapement::EscapementRunContext &runContext);
    void saveFilesAfterSynchronise(const Escapement::EscapementRunContext &runContext);   

} // namespace Escapement_Files

#endif /* ESCAPEMENT_FILES_HPP */


