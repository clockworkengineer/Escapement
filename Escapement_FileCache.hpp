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
    void loadCachedFiles(Escapement::EscapementRunContext &runContext);
    void saveCachedFiles(const Escapement::EscapementRunContext &runContext); 

} // namespace Escapement_FileCache

#endif /* ESCAPEMENT_FILECACHE_HPP */

