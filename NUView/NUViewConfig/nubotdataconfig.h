/*! @file nubotdataconfig.h
    @brief A configuration file that controls the configuration of the where nubot data files
    
    The following preprocessor variables are set here:
        - DATA_DIR
    
    This file is automatically generated by CMake. Do NOT modify this file. Seriously, don't modify
    this file. If you really need to put something here, then you want to modify ./Make/config.in.

    @author Jason Kulk
 */
#ifndef NUBOTDATACONFIG_H
#define NUBOTDATACONFIG_H

#include <stdlib.h>
#include <string>

#ifdef WIN32
    #include <wchar.h>
#endif

#ifndef WIN32
    #define DATA_DIR (std::string(getenv("HOME")) + std::string("/nubot/"))
#endif

#ifdef WIN32
    #define DATA_DIR (std::string(getenv("HOMEPATH")) + std::string("/nubot/"))
#endif


#define CONFIG_DIR (DATA_DIR + std::string("/Config/NUview/"))

#endif // !NUBOTCONFIG_H
