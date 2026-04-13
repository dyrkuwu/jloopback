cmake_minimum_required(VERSION 3.21)

project(jloopback LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(JNI REQUIRED)
find_package(Java REQUIRED)

add_library(jloopback SHARED
    audio_processor.cpp
    entry.cpp
)

target_include_directories(jloopback PRIVATE ${JNI_INCLUDE_DIRS})

if (TARGET JNI::JNI)
    target_link_libraries(jloopback PRIVATE JNI::JNI)
elseif (JNI_LIBRARIES)
    target_link_libraries(jloopback PRIVATE ${JNI_LIBRARIES})
endif()

if (WIN32)
    target_compile_definitions(jloopback PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

set(JLOOPBACK_JAVA_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/java/me/ley/jloopback/AudioProcessor.java
    ${CMAKE_CURRENT_SOURCE_DIR}/java/me/ley/jloopback/AudioProcessorNative.java
    ${CMAKE_CURRENT_SOURCE_DIR}/java/me/ley/jloopback/NativeLibraryLoader.java
)

set(JLOOPBACK_JAVA_CLASSES_DIR ${CMAKE_CURRENT_BINARY_DIR}/java/classes)
set(JLOOPBACK_JAR_RESOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/java/resources/natives/windows-x86_64)
set(JLOOPBACK_JAR_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/jloopback.jar)

add_custom_target(jar ALL
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${JLOOPBACK_JAVA_CLASSES_DIR}
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_CURRENT_BINARY_DIR}/java/resources
    COMMAND ${CMAKE_COMMAND} -E make_directory ${JLOOPBACK_JAVA_CLASSES_DIR}
    COMMAND ${Java_JAVAC_EXECUTABLE} --release 21 -d ${JLOOPBACK_JAVA_CLASSES_DIR} ${JLOOPBACK_JAVA_SOURCES}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${JLOOPBACK_JAR_RESOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:jloopback> ${JLOOPBACK_JAR_RESOURCE_DIR}/jloopback.dll
    COMMAND ${Java_JAR_EXECUTABLE} --create --file ${JLOOPBACK_JAR_OUTPUT}
            -C ${JLOOPBACK_JAVA_CLASSES_DIR} .
            -C ${CMAKE_CURRENT_BINARY_DIR}/java/resources .
    DEPENDS jloopback ${JLOOPBACK_JAVA_SOURCES}
    BYPRODUCTS ${JLOOPBACK_JAR_OUTPUT}
    VERBATIM
)
