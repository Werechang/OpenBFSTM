cmake_minimum_required(VERSION 3.28)
project(OpenBFSTM)

set(CMAKE_CXX_STANDARD 23)

add_executable(OpenBFSTM main.cpp
        InMemoryResource.h
        BfstmFile.cpp
        BfstmFile.h
        AudioPlaybackManager.cpp
        AudioPlaybackManager.h
        bfgrp/BfgrpFile.h
        BfFile.h
        bfgrp/BfgrpReader.cpp
        bfgrp/BfgrpReader.h
        DSPADPCMCodec.h)

target_link_libraries(OpenBFSTM libasound.so)
