/*
 * File:   Escapement.hpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2017, 2:33 PM
 * 
 * Copyright 2017.
 * 
 */

#ifndef ESCAPEMENT_HPP
#define ESCAPEMENT_HPP

//
// C++ STL
//

#include <string>
#include <unordered_map>

//
// Antik Classes
//

#include "CFTP.hpp"

// =========
// NAMESPACE
// =========

namespace Escapement {

   // File information map (indexed by filename, value last modified date/time)

   typedef std::unordered_map<std::string, Antik::FTP::CFTP::DateTime> FileInfoMap;

} // namespace Escapement

#endif /* ESCAPEMENT_HPP */

