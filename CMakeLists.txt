cmake_minimum_required(VERSION 3.16)

project(SpectralLoop VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)
find_package(Qt6 6.8 REQUIRED COMPONENTS Multimedia)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appSpectralLoop
    src/main.cpp
)

qt_add_qml_module(appSpectralLoop
    URI SpectralLoop
    VERSION 1.0
    QML_FILES
        qml/Main.qml
    SOURCES
        src/Interface/BackendInterface.h src/Interface/BackendInterface.cpp
        src/Model/SpectrogramConverter.h src/Model/SpectrogramConverter.cpp
        src/Model/SpectrogramObject.h src/Model/SpectrogramObject.cpp
        src/Model/SpectrogramPlayer.h src/Model/SpectrogramPlayer.cpp
)

target_include_directories(appSpectralLoop PRIVATE src/Interface)
target_include_directories(appSpectralLoop PRIVATE src/Model)

set_target_properties(appSpectralLoop PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appSpectralLoop
    PRIVATE Qt6::Quick
    PRIVATE Qt6::Multimedia
)

include(GNUInstallDirs)
install(TARGETS appSpectralLoop
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
