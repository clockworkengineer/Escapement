/*
 * File:   Escapement_FileCache.hpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2017, 2:33 PM
 * 
 * Copyright 2017.
 * 
 */

#ifndef ESCAPEMENT_FILECACHE_HPP
#define ESCAPEMENT_FILECACHE_HPP

//
// C++ STL
//

#include <string>

//
// Escapement components
//

#include "Escapement.hpp"

// =========
// NAMESPACE
// =========

namespace Escapement_FileCache {

    void loadEscapmentOptions(Escapement::EscapementOptions &optionData); 
    void loadCachedFiles(const Escapement::EscapementOptions &optionData, Escapement::FileInfoMap &remoteFiles, Escapement::FileInfoMap &localFiles);
    void saveCachedFiles(const Escapement::EscapementOptions &optionData, Escapement::FileInfoMap &remoteFiles, Escapement::FileInfoMap &localFiles); 

} // namespace Escapement_FileCache

#endif /* ESCAPEMENT_FILECACHE_HPP */

