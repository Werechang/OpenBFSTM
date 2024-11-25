cmake_minimum_required(VERSION 3.28)
project(OpenBFSTM)

set(CMAKE_CXX_STANDARD 23)

find_package(OpenGL REQUIRED)
find_package(imgui REQUIRED)
find_package(ALSA REQUIRED)

add_executable(OpenBFSTM main.cpp
        MemoryResource.h
        BfstmFile.cpp
        BfstmFile.h
        playback/AudioPlayback.h
        bfgrp/BfgrpFile.h
        BfFile.h
        bfgrp/BfgrpReader.cpp
        bfgrp/BfgrpReader.h
        DSPADPCMCodec.h
        playback/PipeWirePlayback.cpp
        playback/PipeWirePlayback.h
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
        implot/implot_items.cpp)


target_link_libraries(OpenBFSTM PRIVATE ALSA::ALSA glfw OpenGL imgui::imgui)
