cmake_minimum_required(VERSION 3.20)
project(CineIris VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Svg)

# FFmpeg — each platform has its own self-contained include/ + lib/
set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/ffmpeg")

if(WIN32)
    set(FFMPEG_INCLUDE "${FFMPEG_ROOT}/windows/include")
    set(FFMPEG_LIB_DIR "${FFMPEG_ROOT}/windows/lib")
    set(FFMPEG_LIBS
        ${FFMPEG_LIB_DIR}/libavcodec.dll.a
        ${FFMPEG_LIB_DIR}/libavformat.dll.a
        ${FFMPEG_LIB_DIR}/libavutil.dll.a
        ${FFMPEG_LIB_DIR}/libswscale.dll.a
    )
elseif(APPLE)
    set(FFMPEG_INCLUDE "${FFMPEG_ROOT}/macos/include")
    set(FFMPEG_LIB_DIR "${FFMPEG_ROOT}/macos/lib")
    set(FFMPEG_LIBS avcodec avformat avutil swscale)
else()
    set(FFMPEG_INCLUDE "${FFMPEG_ROOT}/linux/include")
    set(FFMPEG_LIB_DIR "${FFMPEG_ROOT}/linux/lib")
    set(FFMPEG_LIBS avcodec avformat avutil swscale swresample)
endif()

# FluentUI — dynamic plugin
set(FLUENTUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FLUENTUI_BUILD_STATIC_LIB OFF CACHE BOOL "" FORCE)
set(FLUENTUI_QML_PLUGIN_DIRECTORY "${CMAKE_BINARY_DIR}/qml/FluentUI" CACHE PATH "" FORCE)
add_subdirectory(third_party/FluentUI)

# Sources
set(SOURCES
    src/main.cpp
    src/TaskController.cpp
    src/VideoProber.cpp
    src/BarcodeGenerator.cpp
    src/IrisTransformer.cpp
)

set(HEADERS
    src/TaskController.h
    src/VideoProber.h
    src/BarcodeGenerator.h
    src/IrisTransformer.h
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    qml.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE}
)

target_link_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_LIB_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    ${FFMPEG_LIBS}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    FLUENTUI_QML_DIR="${CMAKE_BINARY_DIR}/qml"
)

# Platform-specific settings
if(WIN32)
    # Copy FFmpeg DLLs to output
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${FFMPEG_ROOT}/windows/bin" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUILD_RPATH "${FFMPEG_LIB_DIR}"
        INSTALL_RPATH "@executable_path/../Frameworks"
    )
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUILD_RPATH "${FFMPEG_LIB_DIR}"
        INSTALL_RPATH "$ORIGIN/../lib"
    )
endif()
