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

