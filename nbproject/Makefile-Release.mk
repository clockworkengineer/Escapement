#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/Escapement.o \
	${OBJECTDIR}/Escapement_CommandLine.o \
	${OBJECTDIR}/Escapement_FileCache.o \
	${OBJECTDIR}/Escapement_Files.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-Wall -Werror -std=c++17
CXXFLAGS=-Wall -Werror -std=c++17

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=../Antik/dist/Release/GNU-Linux/libantik.a -lboost_system -lboost_filesystem -lboost_program_options `pkg-config --libs libcurl` `pkg-config --libs zlib` `pkg-config --libs openssl` -lpthread   

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/escapement

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/escapement: ../Antik/dist/Release/GNU-Linux/libantik.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/escapement: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/escapement ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/Escapement.o: Escapement.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I../Antik/include -I../Antik/classes/implementation `pkg-config --cflags libcurl` `pkg-config --cflags zlib` `pkg-config --cflags openssl`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Escapement.o Escapement.cpp

${OBJECTDIR}/Escapement_CommandLine.o: Escapement_CommandLine.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I../Antik/include -I../Antik/classes/implementation `pkg-config --cflags libcurl` `pkg-config --cflags zlib` `pkg-config --cflags openssl`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Escapement_CommandLine.o Escapement_CommandLine.cpp

${OBJECTDIR}/Escapement_FileCache.o: Escapement_FileCache.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I../Antik/include -I../Antik/classes/implementation `pkg-config --cflags libcurl` `pkg-config --cflags zlib` `pkg-config --cflags openssl`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Escapement_FileCache.o Escapement_FileCache.cpp

${OBJECTDIR}/Escapement_Files.o: Escapement_Files.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I../Antik/include -I../Antik/classes/implementation `pkg-config --cflags libcurl` `pkg-config --cflags zlib` `pkg-config --cflags openssl`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Escapement_Files.o Escapement_Files.cpp

# Subprojects
.build-subprojects:
	cd ../Antik && ${MAKE}  -f Makefile CONF=Release

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:
	cd ../Antik && ${MAKE}  -f Makefile CONF=Release clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
