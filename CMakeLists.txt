cmake_minimum_required(VERSION 3.28)
project(OpenBFSTM)

set(CMAKE_CXX_STANDARD 23)

find_package(OpenGL REQUIRED)
find_package(imgui REQUIRED)
find_package(ALSA QUIET)

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
        codec/DspADPCM.h
        format/bfsar/BfsarWriter.cpp
        format/bfsar/BfsarWriter.h
        format/bfgrp/BfgrpWriter.cpp
        format/bfgrp/BfgrpWriter.h
        format/bfstp/BfstpReader.cpp
        format/bfstp/BfstpReader.h
        format/bfwar/BfwarReader.cpp
        format/bfwar/BfwarReader.h
        format/bfwar/BfwarStructs.h
        format/bfwar/BfwarWriter.cpp
        format/bfwar/BfwarWriter.h
        playback/DummyPlayback.h
        format/bfwav/BfwavStructs.h
        format/bfwav/BfwavReader.cpp
        format/bfwav/BfwavReader.h
        format/bfwsd/BfwsdReader.cpp
        format/bfwsd/BfwsdReader.h
        format/bfwsd/BfwsdStructs.h)


target_link_libraries(OpenBFSTM PRIVATE glfw OpenGL imgui::imgui)
if (ALSA_FOUND)
    target_link_libraries(OpenBFSTM PRIVATE ALSA::ALSA)
endif ()
