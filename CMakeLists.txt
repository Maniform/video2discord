cmake_minimum_required(VERSION 3.19)
project(video2discord LANGUAGES CXX VERSION 1.0)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Concurrent)

qt_standard_project_setup()

qt_add_executable(video2discord
    WIN32 MACOSX_BUNDLE
    main.cpp
    MainWindow.cpp
    MainWindow.h
    MainWindow.ui
)

target_link_libraries(video2discord
    PRIVATE
        Qt::Core
        Qt::Widgets
		Qt::Concurrent
)

if(APPLE)
	set(FFMPEG_EXECUTABLE Resources/MacOS/ffmpeg)
	set_source_files_properties(
			${FFMPEG_EXECUTABLE}
			PROPERTIES
			MACOSX_PACKAGE_LOCATION "Resources"
	)
	target_sources(video2discord PRIVATE ${FFMPEG_EXECUTABLE})
endif()

include(GNUInstallDirs)

install(TARGETS video2discord
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET video2discord
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

if(WIN32)
    install(FILES Resources/Windows/ffmpeg.exe DESTINATION bin)

    add_custom_command(TARGET video2discord POST_BUILD
		    COMMAND windeployqt --release $<TARGET_FILE:video2discord>
		    COMMENT "Running windeployqt..."
    )

    install(DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/ DESTINATION bin)
    if(WIN32)
        set(CPACK_PACKAGE_NAME "video2discord")
        set(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
        set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
        set(CPACK_PACKAGE_EXECUTABLES "video2discord" "Video 2 Discord")
        set(CPACK_NSIS_INSTALLED_ICON_NAME "bin/video2discord.exe")
    endif()
    include(CPack)
endif()