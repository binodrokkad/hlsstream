cmake_minimum_required(VERSION 3.4.1)
project(hlsstream)

set (CMAKE_CXX_STANDARD 14)
set(CMAKE_VERBOSE_MAKEFILE on)
MESSAGE(${PROJECT_NAME} " Build type: " ${CMAKE_BUILD_TYPE})

set(PATH_TO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src")

if (${CMAKE_SYSTEM_NAME} MATCHES "Android")
    set(CURL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/android/libcurl_ssl)
    set(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/android/ffmpeg)
    set(OPENSSL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/android/openssl-1.1.1)
endif ()
include_directories(${CURL_DIR}/include)
include_directories(${OPENSSL_DIR}/include)
include_directories(${FFMPEG_DIR}/${ANDROID_ABI}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(${PROJECT_NAME} SHARED
        "${PATH_TO_SRC}/HLSReader.cpp"
        "${PATH_TO_SRC}/HLSStreamer.cpp"
        "${PATH_TO_SRC}/HTTPHandler.cpp"
        "${PATH_TO_SRC}/LiveManager.cpp"
        "${PATH_TO_SRC}/PlaylistParser.cpp"
        "${PATH_TO_SRC}/SegmentManager.cpp"
        "${PATH_TO_SRC}/Demuxer.cpp"
        )

if (${CMAKE_SYSTEM_NAME} MATCHES "Android")
    target_sources(${PROJECT_NAME} PRIVATE
            "${PATH_TO_SRC}/android/HLSStreamJNI.cpp"
            )

    add_library(avutil SHARED IMPORTED)
    set_target_properties(avutil PROPERTIES IMPORTED_LOCATION
            ${FFMPEG_DIR}/${ANDROID_ABI}/libavutil.so)

    add_library(avformat SHARED IMPORTED)
    set_target_properties(avformat PROPERTIES IMPORTED_LOCATION
            ${FFMPEG_DIR}/${ANDROID_ABI}/libavformat.so)

    add_library(avcodec SHARED IMPORTED)
    set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION
            ${FFMPEG_DIR}/${ANDROID_ABI}/libavcodec.so)

    add_library(swresample SHARED IMPORTED)
    set_target_properties(swresample PROPERTIES IMPORTED_LOCATION
            ${FFMPEG_DIR}/${ANDROID_ABI}/libswresample.so)

    add_library(swscale SHARED IMPORTED)
    set_target_properties(swscale PROPERTIES IMPORTED_LOCATION
            ${FFMPEG_DIR}/${ANDROID_ABI}/libswscale.so)
endif ()


target_link_libraries(${PROJECT_NAME}
        swscale
        swresample
        avcodec
        avutil
        avformat
        )

if (${CMAKE_SYSTEM_NAME} MATCHES "Android")
    target_link_libraries(${PROJECT_NAME}
            z
            ${CURL_DIR}/${ANDROID_ABI}/libcurl.a
            ${OPENSSL_DIR}/${ANDROID_ABI}/lib/libcrypto.a
            ${OPENSSL_DIR}/${ANDROID_ABI}/lib/libssl.a
            )
endif()