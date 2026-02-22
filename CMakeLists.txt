cmake_minimum_required(VERSION 3.16)
project(FaceFling VERSION 0.1.0 LANGUAGES CXX)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# macOS deployment target
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
)

# Auto-generate MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find dlib
find_package(dlib REQUIRED)

# Find SQLite3
find_package(SQLite3 REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    
    # App layer (Qt UI)
    src/app/MainWindow.cpp
    src/app/MainWindow.h
    src/app/FaceGridWidget.cpp
    src/app/FaceGridWidget.h
    src/app/PersonListWidget.cpp
    src/app/PersonListWidget.h
    src/app/ScanProgressDialog.cpp
    src/app/ScanProgressDialog.h
    
    # Core engine
    src/core/Scanner.cpp
    src/core/Scanner.h
    src/core/Indexer.cpp
    src/core/Indexer.h
    src/core/Clusterer.cpp
    src/core/Clusterer.h
    src/core/Exporter.cpp
    src/core/Exporter.h
    
    # Services
    src/services/FaceService.cpp
    src/services/FaceService.h
    src/services/Database.cpp
    src/services/Database.h
    src/services/ImageLoader.cpp
    src/services/ImageLoader.h
    
    # Models
    src/models/Photo.h
    src/models/Face.h
    src/models/Cluster.h
    src/models/Person.h
)

# Resource files
set(RESOURCES
    resources/face-fling.qrc
)

# Create executable
add_executable(${PROJECT_NAME} MACOSX_BUNDLE
    ${SOURCES}
    ${RESOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    dlib::dlib
    SQLite::SQLite3
)

# macOS bundle settings
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.facefling.app"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist
)

# Copy dlib models to bundle
set(MODEL_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/models/shape_predictor_68_face_landmarks.dat
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/models/dlib_face_recognition_resnet_model_v1.dat
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/models/mmod_human_face_detector.dat
)

# Install models to Resources folder in bundle
foreach(MODEL ${MODEL_FILES})
    if(EXISTS ${MODEL})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${MODEL}
                $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/models/
            COMMENT "Copying ${MODEL} to bundle"
        )
    endif()
endforeach()

# Enable testing
enable_testing()
add_subdirectory(tests EXCLUDE_FROM_ALL)
