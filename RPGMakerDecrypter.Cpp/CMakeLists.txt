cmake_minimum_required(VERSION 3.21)

project(RPGMakerDecrypter VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Widgets)

# Core decryption library. Depends on Qt Core only, no GUI dependencies.
add_library(rpgmaker_core STATIC
    src/core/RGSSADArchive.cpp
    src/core/RGSSADV1Archive.cpp
    src/core/RGSSADV3Archive.cpp
    src/core/ArchiveFileNameUtils.cpp
    src/core/VersionFinder.cpp
    src/core/EncryptionKeyFinder.cpp
    src/core/FileDecrypter.cpp
    src/core/DirectoryFilesDecrypter.cpp
    src/core/ProjectReconstructor.cpp
    src/core/MVProjectReconstructor.cpp
    src/core/MZProjectReconstructor.cpp
    src/core/ProjectGenerator.cpp
)

target_link_libraries(rpgmaker_core PUBLIC Qt6::Core)

target_include_directories(rpgmaker_core PUBLIC src/core)

# GUI application.
add_executable(RPGMakerDecrypter
    src/app/main.cpp
    src/app/MainWindow.cpp
    src/app/DecryptWorker.cpp
)

# WIN32 / MACOSX_BUNDLE produce a windowed application without a console window.
set_target_properties(RPGMakerDecrypter PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(RPGMakerDecrypter PRIVATE rpgmaker_core Qt6::Widgets)

# Core self-tests.
add_executable(RPGMakerDecrypterTests
    tests/CoreTests.cpp
)

target_link_libraries(RPGMakerDecrypterTests PRIVATE rpgmaker_core Qt6::Core)

enable_testing()
add_test(NAME RPGMakerDecrypterTests COMMAND RPGMakerDecrypterTests)
