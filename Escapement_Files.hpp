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
    void getAllLocalFiles(Escapement::EscapementRunContext &runContext);
    std::string convertFilePath(const Escapement::EscapementOptions &optionData, const std::string &filePath);
    void pullFiles (Escapement::EscapementRunContext &runContext);
    void pushFiles (Escapement::EscapementRunContext &runContext);
    void deleteFiles (Escapement::EscapementRunContext &runContext);
    void loadFilesBeforeSynchronise(Escapement::EscapementRunContext &runContext);
    void saveFilesAfterSynchronise(const Escapement::EscapementRunContext &runContext);   

} // namespace Escapement_Files

#endif /* ESCAPEMENT_FILES_HPP */


