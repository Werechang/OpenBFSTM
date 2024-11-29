cmake_minimum_required(VERSION 3.28)
project(OpenBFSTM)

set(CMAKE_CXX_STANDARD 23)

find_package(OpenGL REQUIRED)
find_package(imgui REQUIRED)
find_package(ALSA REQUIRED)

add_executable(OpenBFSTM main.cpp
        MemoryResource.h
        format/bfstm/BfstmFile.cpp
        format/bfstm/BfstmFile.h
        format/bfstm/BfstmReader.cpp
        format/bfstm/BfstmReader.h
        playback/AudioPlayback.h
        format/bfgrp/BfgrpStructs.h
        BfFile.h
        format/bfgrp/BfgrpReader.cpp
        format/bfgrp/BfgrpReader.h
        DSPADPCMCodec.h
        playback/ALSAPlayback.cpp
        playback/ALSAPlayback.h
        playback/PlaybackFunctions.cpp
        playback/PlaybackFunctions.h
        playback/AudioPlayback.cpp
        Window.h
        Window.cpp
        implot/implot_demo.cpp
        implot/implot.cpp
        implot/implot.h
        implot/implot_internal.h
        implot/implot_items.cpp
        BfFile.cpp
        format/bfsar/BfsarStructs.h
        format/bfsar/BfsarReader.cpp
        format/bfsar/BfsarReader.h
        codec/DspADPCM.cpp
        codec/DspADPCM.h)


target_link_libraries(OpenBFSTM PRIVATE ALSA::ALSA glfw OpenGL imgui::imgui)
