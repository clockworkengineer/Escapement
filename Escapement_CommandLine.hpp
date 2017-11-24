/*
 * File:   Escapement_CommandLine.hpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2017, 2:33 PM
 * 
 * Copyright 2017.
 * 
 */

#ifndef ESCAPEMENT_COMMANDLINE_HPP
#define ESCAPEMENT_COMMANDLINE_HPP

//
// C++ STL
//

#include <string>

//
// Escapement
//

#include "Escapement.hpp"

// =========
// NAMESPACE
// =========

namespace Escapement_CommandLine {

    Escapement::EscapementOptions fetchCommandLineOptions(int argc, char** argv);

} // namespace Escapement_CommandLine

#endif /* ESCAPEMENT_COMMANDLINE_HPP */

