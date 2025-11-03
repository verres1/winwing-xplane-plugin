CMAKE_MINIMUM_REQUIRED(VERSION 3.16)
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
PROJECT(winwing C CXX)

# Use XPLANE_SDK_DIR environment variable
IF(DEFINED ENV{XPLANE_SDK_DIR})
    SET(XPLANE_SDK_ROOT "$ENV{XPLANE_SDK_DIR}")
ELSE()
    MESSAGE(FATAL_ERROR "XPLANE_SDK_DIR environment variable not set. Please set it to the X-Plane SDK directory.")
ENDIF()

MESSAGE(STATUS "Using X-Plane SDK from: ${XPLANE_SDK_ROOT}")

IF(APPLE)
    SET(DIRECTORY_PREFIX "mac_x64")
    SET(XPLANE_LIBRARY_PATH "${XPLANE_SDK_ROOT}/Libraries/Mac/")
ELSEIF(UNIX)
    SET(DIRECTORY_PREFIX "lin_x64")
    SET(XPLANE_LIBRARY_PATH "${XPLANE_SDK_ROOT}/Libraries/Lin/")
ELSEIF(WIN32)
    SET(DIRECTORY_PREFIX "win_x64")
    SET(XPLANE_LIBRARY_PATH "${XPLANE_SDK_ROOT}/Libraries/Win/")
ENDIF()

SET(XPLANE_INCLUDES_PATH "${XPLANE_SDK_ROOT}/CHeaders/" CACHE STRING PATH)

IF(APPLE)
    FIND_LIBRARY(COCOA_LIBRARY Cocoa)
    FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
    FIND_LIBRARY(XPLM_LIBRARY XPLM "${XPLANE_LIBRARY_PATH}")
    FIND_LIBRARY(XPWIDGETS_LIBRARY XPWidgets "${XPLANE_LIBRARY_PATH}")
    
    IF(XPLM_LIBRARY AND XPWIDGETS_LIBRARY)
        ADD_LIBRARY(xplm STATIC IMPORTED GLOBAL)
        ADD_LIBRARY(xpwidgets STATIC IMPORTED GLOBAL)
        SET_PROPERTY(TARGET xplm PROPERTY IMPORTED_LOCATION "${XPLM_LIBRARY}")
        SET_PROPERTY(TARGET xpwidgets PROPERTY IMPORTED_LOCATION "${XPWIDGETS_LIBRARY}")
    ELSE()
        # Cross-compilation: Create interface libraries
        MESSAGE(STATUS "macOS SDK frameworks not found, using interface targets")
        ADD_LIBRARY(xplm INTERFACE)
        ADD_LIBRARY(xpwidgets INTERFACE)
    ENDIF()
ELSEIF(WIN32)
    # Manually specify Windows SDK library paths
    SET(XPLM_LIBRARY "${XPLANE_LIBRARY_PATH}/XPLM_64.lib")
    SET(XPWIDGETS_LIBRARY "${XPLANE_LIBRARY_PATH}/XPWidgets_64.lib")
    
    MESSAGE(STATUS "Windows SDK libraries: ${XPLM_LIBRARY}, ${XPWIDGETS_LIBRARY}")
    
    ADD_LIBRARY(xplm STATIC IMPORTED GLOBAL)
    ADD_LIBRARY(xpwidgets STATIC IMPORTED GLOBAL)
    SET_PROPERTY(TARGET xplm PROPERTY IMPORTED_LOCATION "${XPLM_LIBRARY}")
    SET_PROPERTY(TARGET xpwidgets PROPERTY IMPORTED_LOCATION "${XPWIDGETS_LIBRARY}")
ELSEIF(UNIX)
    FIND_LIBRARY(OPENSSL_LIBRARY NAMES libssl.so PATHS "/usr/lib/x86_64-linux-gnu")
    FIND_LIBRARY(OPENSSL_CRYPTO_LIBRARY NAMES libcrypto.so PATHS "/usr/lib/x86_64-linux-gnu")
    FIND_LIBRARY(XPLM_LIBRARY XPLM_64.so PATHS "${XPLANE_LIBRARY_PATH}")
    FIND_LIBRARY(XPWIDGETS_LIBRARY XPWidgets_64.so PATHS "${XPLANE_LIBRARY_PATH}")
    
    # For native builds, libraries should be found
    IF(XPLM_LIBRARY AND XPWIDGETS_LIBRARY)
        ADD_LIBRARY(xplm SHARED IMPORTED GLOBAL)
        ADD_LIBRARY(xpwidgets SHARED IMPORTED GLOBAL)
        SET_PROPERTY(TARGET xplm PROPERTY IMPORTED_LOCATION "${XPLM_LIBRARY}")
        SET_PROPERTY(TARGET xpwidgets PROPERTY IMPORTED_LOCATION "${XPWIDGETS_LIBRARY}")
    ELSE()
        # Interface libraries for builds without SDK libraries
        ADD_LIBRARY(xplm INTERFACE)
        ADD_LIBRARY(xpwidgets INTERFACE)
    ENDIF()
ENDIF()

IF(EXISTS "${XPLANE_INCLUDES_PATH}/XPLM")
    SET_PROPERTY(TARGET xplm APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${XPLANE_INCLUDES_PATH}/XPLM" "${XPLANE_INCLUDES_PATH}/Wrappers")
    SET_PROPERTY(TARGET xpwidgets APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${XPLANE_INCLUDES_PATH}/XPLM" "${XPLANE_INCLUDES_PATH}/Widgets" "${XPLANE_INCLUDES_PATH}/Wrappers")
ENDIF()

FUNCTION(HEADER_DIRECTORIES return_list base_dir)
    FILE(GLOB_RECURSE new_list "${base_dir}/*.h" "${base_dir}/*.hpp")
    SET(dir_list "")
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        LIST(APPEND dir_list ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list} PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(FIND_SOURCE_FILES return_list base_dir)
    SET(file_patterns "${base_dir}/*.c" "${base_dir}/*.cpp" "${base_dir}/*.hpp")

    IF(APPLE)
        LIST(APPEND file_patterns "${base_dir}/*.mm")
    ENDIF()
    FILE(GLOB_RECURSE new_list ${file_patterns})

    LIST(FIND new_list "${base_dir}/main.cpp" main_index)
    IF(main_index GREATER_EQUAL 0)
        LIST(REMOVE_AT new_list ${main_index})
        LIST(PREPEND new_list "${base_dir}/main.cpp")
    ENDIF()

    LIST(FILTER new_list EXCLUDE REGEX ".*/desktop/.*")

    SET(${return_list} ${new_list} PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(add_xplane_sdk_definitions library_name library_version) 
    IF(APPLE)
        TARGET_COMPILE_DEFINITIONS(${library_name} PUBLIC -DAPL=1 -DIBM=0 -DLIN=0 -DGL_SILENCE_DEPRECATION)
        # Workaround for missing #include in source files - force include wrapper header
        TARGET_COMPILE_OPTIONS(${library_name} PRIVATE "-include" "${CMAKE_SOURCE_DIR}/macos_missing_includes.h")
    ELSEIF(UNIX)
        TARGET_COMPILE_DEFINITIONS(${library_name} PUBLIC -DAPL=0 -DIBM=0 -DLIN=1)
        TARGET_COMPILE_OPTIONS(${library_name} PRIVATE "-fPIC")
    ELSEIF(WIN32)
        TARGET_COMPILE_DEFINITIONS(${library_name} PUBLIC -DAPL=0 -DIBM=1 -DLIN=0)
        TARGET_COMPILE_OPTIONS(${library_name} PRIVATE -pthread)
        TARGET_LINK_OPTIONS(${library_name} PRIVATE -pthread -static-libstdc++ -static-libgcc -Wl,--allow-multiple-definition 
            "-Wl,--defsym,_Z18HidD_GetAttributesPvP16_HIDD_ATTRIBUTES=HidD_GetAttributes"
            "-Wl,--defsym,_Z26HidD_GetManufacturerStringPvS_m=HidD_GetManufacturerString" 
            "-Wl,--defsym,_Z21HidD_GetProductStringPvS_m=HidD_GetProductString"
            "-Wl,--defsym,_Z21HidD_GetPreparsedDataPvPP20_HIDP_PREPARSED_DATA=HidD_GetPreparsedData"
            "-Wl,--defsym,_Z12HidP_GetCapsP20_HIDP_PREPARSED_DATAP10_HIDP_CAPS=HidP_GetCaps"
            "-Wl,--defsym,_Z22HidD_FreePreparsedDataP20_HIDP_PREPARSED_DATA=HidD_FreePreparsedData")
    ENDIF()

    IF(library_version EQUAL 200)
        TARGET_COMPILE_DEFINITIONS(${library_name} PUBLIC -DXPLM200=1)
    ELSEIF(library_version EQUAL 210)
        TARGET_COMPILE_DEFINITIONS(${library_name} PUBLIC -DXPLM200=1 -DXPLM210=1)
    ELSEIF(library_version EQUAL 300)
        TARGET_COMPILE_DEFINITIONS(${library_name} PUBLIC -DXPLM200=1 -DXPLM210=1 -DXPLM300=1)
    ELSEIF(library_version EQUAL 301)
        TARGET_COMPILE_DEFINITIONS(${library_name} PUBLIC -DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1)
    ELSEIF(library_version EQUAL 400)
        TARGET_COMPILE_DEFINITIONS(${library_name} PUBLIC -DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1 -DXPLM400=1)
    ELSEIF(library_version EQUAL 410)
        TARGET_COMPILE_DEFINITIONS(${library_name} PUBLIC -DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1 -DXPLM400=1 -DXPLM410=1)
    ELSEIF(library_version EQUAL 411)
        TARGET_COMPILE_DEFINITIONS(${library_name} PUBLIC -DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1 -DXPLM400=1 -DXPLM410=1 -DXPLM411=1)
    ELSE()
        MESSAGE(FATAL_ERROR "Library version one of: 200, 210, 300, 301, 400, 410, 411")
    ENDIF()
    TARGET_INCLUDE_DIRECTORIES(${library_name} PUBLIC "${XPLANE_INCLUDES_PATH}/XPLM" "${XPLANE_INCLUDES_PATH}/Widgets" "${XPLANE_INCLUDES_PATH}/Wrappers")
ENDFUNCTION(add_xplane_sdk_definitions)

FUNCTION(add_xplane_plugin library_name library_version src_dir)
    MESSAGE(STATUS "Preparing X-Plane plugin '${library_name}' at ${src_dir}")
    FIND_SOURCE_FILES(FILES ${src_dir})

    IF(APPLE)
        ADD_EXECUTABLE(${library_name} ${FILES})
    ELSEIF(UNIX)
        ADD_LIBRARY(${library_name} SHARED ${FILES})
    ELSEIF(WIN32)
        ADD_LIBRARY(${library_name} MODULE ${FILES})
        # TARGET_COMPILE_DEFINITIONS(${library_name} PUBLIC -D_CRT_SECURE_NO_WARNINGS /bigobj)
        TARGET_COMPILE_DEFINITIONS(${library_name} PUBLIC _CRT_SECURE_NO_WARNINGS)
        # TARGET_COMPILE_OPTIONS(${library_name} PUBLIC /bigobj)
    ENDIF()

    IF(${library_version} LESS 300)
        IF(APPLE)
            SET_TARGET_PROPERTIES(${library_name} PROPERTIES OUTPUT_NAME "mac.xpl")
        ELSEIF(UNIX)
            SET_TARGET_PROPERTIES(${library_name} PROPERTIES OUTPUT_NAME "lin.xpl")
        ELSEIF(WIN32)
            SET_TARGET_PROPERTIES(${library_name} PROPERTIES OUTPUT_NAME "win.xpl")
        ENDIF()
    ELSE()
        IF(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${DIRECTORY_PREFIX}")
            FILE(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${DIRECTORY_PREFIX}")
        ENDIF()
        SET_TARGET_PROPERTIES(${library_name} PROPERTIES OUTPUT_NAME "${DIRECTORY_PREFIX}/${library_name}.xpl")
    ENDIF()
    add_xplane_sdk_definitions(${library_name} ${library_version})

    SET_TARGET_PROPERTIES(${library_name} PROPERTIES PREFIX "")
    SET_TARGET_PROPERTIES(${library_name} PROPERTIES SUFFIX "")
    SET_TARGET_PROPERTIES(${library_name} PROPERTIES LINKER_LANGUAGE CXX)

    IF(APPLE)
        SET_TARGET_PROPERTIES(${library_name} PROPERTIES 
            LINK_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIC -fvisibility=hidden -bundle -undefined dynamic_lookup")
    ELSEIF(WIN32)
        # Windows specific flags
    ELSEIF(UNIX)
        SET_TARGET_PROPERTIES(${library_name} PROPERTIES
            LINK_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -rdynamic -nodefaultlibs -fPIC -fvisibility=hidden")
    ENDIF()
        
    # Link libraries (use imported targets if available, otherwise interface)
    TARGET_LINK_LIBRARIES(${library_name} PUBLIC xplm xpwidgets)
    
    # Platform-specific system libraries
    IF(APPLE)
        IF(COCOA_LIBRARY)
            TARGET_LINK_LIBRARIES(${library_name} PUBLIC ${COCOA_LIBRARY})
        ENDIF()
        IF(OPENGL_LIBRARY)
            TARGET_LINK_LIBRARIES(${library_name} PUBLIC ${OPENGL_LIBRARY})
        ENDIF()
    ELSEIF(WIN32)
        # Windows needs OpenGL32
        TARGET_LINK_LIBRARIES(${library_name} PUBLIC opengl32)
    ELSEIF(UNIX)
        IF(OPENSSL_LIBRARY)
            TARGET_LINK_LIBRARIES(${library_name} PUBLIC ${OPENSSL_LIBRARY})
        ENDIF()
        IF(OPENSSL_CRYPTO_LIBRARY)
            TARGET_LINK_LIBRARIES(${library_name} PUBLIC ${OPENSSL_CRYPTO_LIBRARY})
        ENDIF()
    ENDIF()
    
    # Add platform-specific HID libraries
    IF(APPLE)
        FIND_LIBRARY(IOKIT_LIBRARY IOKit)
        FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation)
        IF(IOKIT_LIBRARY AND COREFOUNDATION_LIBRARY)
            TARGET_LINK_LIBRARIES(${library_name} PUBLIC ${IOKIT_LIBRARY} ${COREFOUNDATION_LIBRARY})
        ENDIF()
    ELSEIF(WIN32)
        # Link Windows HID and Setup API libraries
        # Use --start-group/--end-group to resolve circular dependencies
        TARGET_LINK_LIBRARIES(${library_name} PUBLIC "-Wl,--start-group" setupapi hid hidparse "-Wl,--end-group")
    ELSEIF(UNIX)
        FIND_PACKAGE(PkgConfig REQUIRED)
        PKG_CHECK_MODULES(UDEV REQUIRED libudev)
        TARGET_LINK_LIBRARIES(${library_name} PUBLIC ${UDEV_LIBRARIES})
        TARGET_INCLUDE_DIRECTORIES(${library_name} PRIVATE ${UDEV_INCLUDE_DIRS})
    ENDIF()
    
    #TARGET_LINK_LIBRARIES(${library_name} PUBLIC ${XPLM_LIBRARY} ${XPWIDGETS_LIBRARY})
    TARGET_INCLUDE_DIRECTORIES(${library_name} PUBLIC "${XPLANE_INCLUDES_PATH}/XPLM" "${XPLANE_INCLUDES_PATH}/Widgets" "${XPLANE_INCLUDES_PATH}/Wrappers")
    HEADER_DIRECTORIES(header_dir_list ${src_dir})
    TARGET_INCLUDE_DIRECTORIES(${library_name} PRIVATE ${header_dir_list})
        
ENDFUNCTION(add_xplane_plugin)

IF(NOT DEFINED SDK_VERSION)
    SET(SDK_VERSION 411)
    MESSAGE(STATUS "SDK_VERSION not defined, defaulting to ${SDK_VERSION}")
ENDIF()

add_xplane_plugin(winwing ${SDK_VERSION} "${CMAKE_CURRENT_SOURCE_DIR}/src")
