# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("footipredict")

find_package(game-activity REQUIRED CONFIG)
find_package(oboe REQUIRED CONFIG)

set(ffmpeg_dir ${CMAKE_SOURCE_DIR}/ffmpeg)
set(ffmpeg_libs ${ffmpeg_dir}/${ANDROID_ABI}/lib/)

include_directories(${ffmpeg_dir}/${ANDROID_ABI}/include/)


set(
        # List variable name
        ffmpeg_libs_names
        # Values in the list
        avcodec avdevice avfilter avformat avutil swresample swscale)

foreach (ffmpeg_lib_name ${ffmpeg_libs_names})
    add_library(
            ${ffmpeg_lib_name}
            STATIC
            IMPORTED)
    set_target_properties(
            ${ffmpeg_lib_name}
            PROPERTIES
            IMPORTED_LOCATION
            ${ffmpeg_libs}/lib${ffmpeg_lib_name}.so)
endforeach ()



add_library(footipredict SHARED
        main.cpp
        OboePlayer.cpp)

target_link_libraries(footipredict
        #Oboe
        oboe::oboe
        # The game activity
        game-activity::game-activity
        # EGL and other dependent libraries required for drawing
        # and interacting with Android system
        EGL
        GLESv3
        jnigraphics
        android
        log
        mediandk
        ${ffmpeg_libs_names}
)
