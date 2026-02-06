cmake_minimum_required(VERSION 3.21)
project(AirgapJsonFormatter VERSION 0.1.8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Enable strict compiler warnings (AC15: No compiler warnings with -Wall -Wextra)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)

qt_add_executable(airgap_formatter
    main.cpp
    jsonbridge.cpp
    jsonbridge.h
    asyncserialiser.cpp
    asyncserialiser.h
    qjsontreeitem.cpp
    qjsontreeitem.h
    qjsontreemodel.cpp
    qjsontreemodel.h
    theme.cpp
    theme.h
)

qt_add_qml_module(airgap_formatter
    URI AirgapFormatter
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Header.qml
        qml/Toolbar.qml
        qml/InputPane.qml
        qml/OutputPane.qml
        qml/JsonTreeView.qml
        qml/StatusBar.qml
        qml/HistoryPanel.qml
    SOURCES
        theme.h
        theme.cpp
)

target_link_libraries(airgap_formatter PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
)

# JSPI build option (experimental, requires Chrome 137+ or Firefox 130+ with flag)
# JSPI (JavaScript Promise Integration) allows WebAssembly to suspend/resume
# with multiple concurrent suspensions, eliminating Asyncify overhead.
# Default: OFF (uses Asyncify for broad browser compatibility)
option(ENABLE_JSPI "Enable JSPI instead of Asyncify (experimental, Chrome 137+ only)" OFF)

# WebAssembly-specific settings
if(EMSCRIPTEN)
    # Base Emscripten link options for ES6 module export
    target_link_options(airgap_formatter PRIVATE
        -sMODULARIZE=1
        -sEXPORT_ES6=1
        -sEXPORT_NAME=createQtAppInstance
        -sENVIRONMENT=web
        -sALLOW_MEMORY_GROWTH=1
        --bind
    )

    # Choose between JSPI and Asyncify based on build option
    if(ENABLE_JSPI)
        message(STATUS "Building with JSPI (experimental) - requires Chrome 137+ or Firefox 130+ with flag")
        message(STATUS "  Note: JSPI eliminates Asyncify overhead but has limited browser support")
        target_link_options(airgap_formatter PRIVATE
            -sJSPI=1
            -sASYNCIFY=0
        )
        # Add compile definition so C++ code can detect JSPI build
        target_compile_definitions(airgap_formatter PRIVATE ENABLE_JSPI_BUILD=1)
    else()
        message(STATUS "Building with Asyncify (production default)")
        target_link_options(airgap_formatter PRIVATE
            -sASYNCIFY
            -sASYNCIFY_STACK_SIZE=131072
        )
    endif()
endif()

# Copy PWA assets to build directory for deployment
if(EMSCRIPTEN)
    file(COPY ${CMAKE_SOURCE_DIR}/../public/icon-192.png DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_SOURCE_DIR}/../public/icon-512.png DESTINATION ${CMAKE_BINARY_DIR})
endif()
