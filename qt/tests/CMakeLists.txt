cmake_minimum_required(VERSION 3.21)
project(AsyncSerialiserTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Enable strict compiler warnings (AC15: No compiler warnings with -Wall -Wextra)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Test Gui Quick)

enable_testing()

# AsyncSerialiser unit tests
qt_add_executable(tst_asyncserialiser
    tst_asyncserialiser.cpp
    ../asyncserialiser.cpp
    ../asyncserialiser.h
)

target_include_directories(tst_asyncserialiser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(tst_asyncserialiser PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME tst_asyncserialiser COMMAND tst_asyncserialiser)

# JsonBridge async operations tests (Story 5.2)
qt_add_executable(tst_jsonbridge_async
    tst_jsonbridge_async.cpp
    ../asyncserialiser.cpp
    ../asyncserialiser.h
    ../jsonbridge.cpp
    ../jsonbridge.h
    ../qjsontreemodel.cpp
    ../qjsontreemodel.h
    ../qjsontreeitem.cpp
    ../qjsontreeitem.h
    ../qxmltreemodel.cpp
    ../qxmltreemodel.h
    ../qxmltreeitem.cpp
    ../qxmltreeitem.h
)

target_include_directories(tst_jsonbridge_async PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(tst_jsonbridge_async PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui
    Qt6::Quick
)

add_test(NAME tst_jsonbridge_async COMMAND tst_jsonbridge_async)

# Mermaid rendering tests (Story 10.2)
qt_add_executable(tst_mermaid
    tst_mermaid.cpp
    ../asyncserialiser.cpp
    ../asyncserialiser.h
    ../jsonbridge.cpp
    ../jsonbridge.h
    ../qjsontreemodel.cpp
    ../qjsontreemodel.h
    ../qjsontreeitem.cpp
    ../qjsontreeitem.h
    ../qxmltreemodel.cpp
    ../qxmltreemodel.h
    ../qxmltreeitem.cpp
    ../qxmltreeitem.h
)

target_include_directories(tst_mermaid PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(tst_mermaid PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui
    Qt6::Quick
)

add_test(NAME tst_mermaid COMMAND tst_mermaid)

# Responsive UI tests (Story 11.0)
qt_add_executable(tst_responsive_ui
    tst_responsive_ui.cpp
)

target_include_directories(tst_responsive_ui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Define QML_PATH for test to locate QML files
target_compile_definitions(tst_responsive_ui PRIVATE
    QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../qml"
)

target_link_libraries(tst_responsive_ui PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
)

add_test(NAME tst_responsive_ui COMMAND tst_responsive_ui)

# Theme tests (Story 10.7)
qt_add_executable(tst_theme
    tst_theme.cpp
)

target_include_directories(tst_theme PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Define QML_PATH for test to locate QML files
target_compile_definitions(tst_theme PRIVATE
    QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../qml"
)

target_link_libraries(tst_theme PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
)

add_test(NAME tst_theme COMMAND tst_theme)

# Markdown Bridge tests (Story 10.3)
qt_add_executable(tst_markdown_bridge
    tst_markdown_bridge.cpp
    ../asyncserialiser.cpp
    ../asyncserialiser.h
    ../jsonbridge.cpp
    ../jsonbridge.h
    ../qjsontreemodel.cpp
    ../qjsontreemodel.h
    ../qjsontreeitem.cpp
    ../qjsontreeitem.h
    ../qxmltreemodel.cpp
    ../qxmltreemodel.h
    ../qxmltreeitem.cpp
    ../qxmltreeitem.h
)

target_include_directories(tst_markdown_bridge PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(tst_markdown_bridge PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui
    Qt6::Quick
)

add_test(NAME tst_markdown_bridge COMMAND tst_markdown_bridge)

# JsonBridge XML operations tests (Story 8.3)
qt_add_executable(tst_jsonbridge_xml
    tst_jsonbridge_xml.cpp
    ../asyncserialiser.cpp
    ../asyncserialiser.h
    ../jsonbridge.cpp
    ../jsonbridge.h
    ../qjsontreemodel.cpp
    ../qjsontreemodel.h
    ../qjsontreeitem.cpp
    ../qjsontreeitem.h
    ../qxmltreemodel.cpp
    ../qxmltreemodel.h
    ../qxmltreeitem.cpp
    ../qxmltreeitem.h
)

target_include_directories(tst_jsonbridge_xml PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(tst_jsonbridge_xml PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui
    Qt6::Quick
)

add_test(NAME tst_jsonbridge_xml COMMAND tst_jsonbridge_xml)

# Format Auto-Detection tests (Story 8.4)
qt_add_executable(tst_format_detection
    tst_format_detection.cpp
    ../asyncserialiser.cpp
    ../asyncserialiser.h
    ../jsonbridge.cpp
    ../jsonbridge.h
    ../qjsontreemodel.cpp
    ../qjsontreemodel.h
    ../qjsontreeitem.cpp
    ../qjsontreeitem.h
    ../qxmltreemodel.cpp
    ../qxmltreemodel.h
    ../qxmltreeitem.cpp
    ../qxmltreeitem.h
)

target_include_directories(tst_format_detection PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(tst_format_detection PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui
    Qt6::Quick
)

add_test(NAME tst_format_detection COMMAND tst_format_detection)

# QXmlTreeModel tests (Story 8.6)
qt_add_executable(tst_qxmltreemodel
    tst_qxmltreemodel.cpp
    ../qxmltreeitem.cpp
    ../qxmltreeitem.h
    ../qxmltreemodel.cpp
    ../qxmltreemodel.h
)

target_include_directories(tst_qxmltreemodel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(tst_qxmltreemodel PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME tst_qxmltreemodel COMMAND tst_qxmltreemodel)

# Format Selector tests (Story 8.5)
qt_add_executable(tst_format_selector
    tst_format_selector.cpp
)

target_include_directories(tst_format_selector PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Define QML_PATH for test to locate QML files
target_compile_definitions(tst_format_selector PRIVATE
    QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../qml"
)

target_link_libraries(tst_format_selector PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
)

add_test(NAME tst_format_selector COMMAND tst_format_selector)

# XML Integration tests (Story 8.7)
qt_add_executable(tst_xml_integration
    integration/tst_xml_integration.cpp
    ../asyncserialiser.cpp
    ../asyncserialiser.h
    ../jsonbridge.cpp
    ../jsonbridge.h
    ../qjsontreemodel.cpp
    ../qjsontreemodel.h
    ../qjsontreeitem.cpp
    ../qjsontreeitem.h
    ../qxmltreemodel.cpp
    ../qxmltreemodel.h
    ../qxmltreeitem.cpp
    ../qxmltreeitem.h
)

target_include_directories(tst_xml_integration PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(tst_xml_integration PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui
    Qt6::Quick
)

add_test(NAME tst_xml_integration COMMAND tst_xml_integration)
