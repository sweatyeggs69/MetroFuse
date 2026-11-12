cmake_minimum_required(VERSION 3.24)
# Windows: static MSVC runtime so MetroFuse.exe runs without installing the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

project(metrofuse LANGUAGES C CXX VERSION 1.3.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(MSVC)
    add_compile_options(/utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()

option(METROFUSE_FETCH_SDL "Download and build SDL3 if it is not installed" ON)
option(METROFUSE_BUILD_TESTS "Build headless tests" ON)
option(METROFUSE_BUILD_TOOLS "Build analysis tools (metrofuse_disasm, needs Capstone)" ON)
option(METROFUSE_MACOS_BUNDLE "Build MetroFuse.app (and the `dmg` target) on macOS" OFF)
option(METROFUSE_VENDOR_SDL "Always build SDL3 from source (static) instead of using a system install" OFF)
option(METROFUSE_PACKAGE "Add packaging targets (Windows zip, Linux AppDir install)" OFF)

# App icon renderer (original procedural artwork); used for the macOS .icns, Windows .ico and
# the Linux icon.
add_executable(metrofuse_icon tools/metrofuse_icon/main.cpp)

# Core: ROM loading and verification. No SDL dependency, so it can be tested headless.
add_library(metrofuse_core STATIC
    src/rom.cpp
    src/sha1.cpp
)
target_include_directories(metrofuse_core PUBLIC src)
if(MSVC)
    target_compile_options(metrofuse_core PRIVATE /W4)
else()
    target_compile_options(metrofuse_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Translations of the launcher and menu text (src/app/i18n.h). Each assets/lang/<code>.po is
# compiled in; tools/i18n.py checks them against the code. No SDL.
set(metrofuse_lang_cpp "")
foreach(pair "fr;fr" "de;de" "es;es" "it;it" "pt_br;pt-BR")
    list(GET pair 0 name)
    list(GET pair 1 file)
    set(out "${CMAKE_CURRENT_BINARY_DIR}/lang_${name}.cpp")
    add_custom_command(OUTPUT "${out}"
        COMMAND "${CMAKE_COMMAND}" -DINPUT=${PROJECT_SOURCE_DIR}/assets/lang/${file}.po
                -DOUTPUT=${out} -DSYMBOL=metrofuse_lang_${name} -P ${PROJECT_SOURCE_DIR}/cmake/embed.cmake
        DEPENDS assets/lang/${file}.po cmake/embed.cmake
        COMMENT "Embedding ${file}.po")
    list(APPEND metrofuse_lang_cpp "${out}")
endforeach()
add_library(metrofuse_i18n STATIC src/app/i18n.cpp ${metrofuse_lang_cpp})
target_include_directories(metrofuse_i18n PUBLIC src)
if(MSVC)
    target_compile_options(metrofuse_i18n PRIVATE /W4)
else()
    target_compile_options(metrofuse_i18n PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Save import/export and backups (no SDL).
add_library(metrofuse_saves STATIC src/app/save_import.cpp src/app/fsutil.cpp)
target_include_directories(metrofuse_saves PUBLIC src)
target_link_libraries(metrofuse_saves PUBLIC metrofuse_i18n)
if(WIN32)
    target_link_libraries(metrofuse_saves PUBLIC shell32 ole32 uuid)
endif()
if(NOT MSVC)
    target_compile_options(metrofuse_saves PRIVATE -Wall -Wextra)
endif()

# Save state slots (no SDL). Uses the system zlib when present (macOS/Linux); otherwise states
# are stored uncompressed.
add_library(metrofuse_states STATIC src/app/states.cpp)
target_include_directories(metrofuse_states PUBLIC src)
target_link_libraries(metrofuse_states PUBLIC metrofuse_gba metrofuse_saves)
target_compile_definitions(metrofuse_states PRIVATE METROFUSE_VERSION="${PROJECT_VERSION}")
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
    target_link_libraries(metrofuse_states PRIVATE ZLIB::ZLIB)
    target_compile_definitions(metrofuse_states PRIVATE METROFUSE_HAVE_ZLIB=1)
endif()
if(NOT MSVC)
    target_compile_options(metrofuse_states PRIVATE -Wall -Wextra)
endif()

# Updates from GitHub Releases: versions, release JSON, SHA-256, zip unpacking. No SDL.
add_library(metrofuse_update_core STATIC src/update/sha256.cpp src/update/update_core.cpp)
target_include_directories(metrofuse_update_core PUBLIC src)
if(ZLIB_FOUND)
    target_link_libraries(metrofuse_update_core PRIVATE ZLIB::ZLIB)
    target_compile_definitions(metrofuse_update_core PRIVATE METROFUSE_HAVE_ZLIB=1)
endif()
if(MSVC)
    target_compile_options(metrofuse_update_core PRIVATE /W4)
else()
    target_compile_options(metrofuse_update_core PRIVATE -Wall -Wextra)
endif()

# Display post-processing (scaling layout, GBA colors, frame blending, overlays). No SDL.
add_library(metrofuse_display STATIC src/options/display.cpp)
target_include_directories(metrofuse_display PUBLIC src)
if(MSVC)
    target_compile_options(metrofuse_display PRIVATE /W4)
else()
    target_compile_options(metrofuse_display PRIVATE -Wall -Wextra -Wpedantic)
endif()

# GBA runtime (interpreter, PPU, APU, BIOS HLE). No SDL dependency.
add_library(metrofuse_gba STATIC
    src/gba/apu.cpp
    src/gba/bios.cpp
    src/gba/bus.cpp
    src/gba/cpu.cpp
    src/gba/ppu.cpp
    src/gba/state.cpp
    src/gba/system.cpp
    src/gba/wide_level.cpp
)
target_include_directories(metrofuse_gba PUBLIC src)
if(MSVC)
    target_compile_options(metrofuse_gba PRIVATE /W4)
else()
    target_compile_options(metrofuse_gba PRIVATE -Wall -Wextra)
endif()

# SDL3: use a system install if present (brew install sdl3, pacman -S sdl3, vcpkg), else fetch it.
if(NOT METROFUSE_VENDOR_SDL)
    find_package(SDL3 CONFIG QUIET)
endif()
if(NOT SDL3_FOUND AND METROFUSE_FETCH_SDL)
    include(FetchContent)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.26
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(SDL3)
endif()

if(TARGET SDL3::SDL3)
    # Dear ImGui (core + SDL3 platform + SDL_Renderer3 backends), pinned.
    include(FetchContent)
    FetchContent_Declare(imgui_src
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.8
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(imgui_src)
    add_library(imgui STATIC
        ${imgui_src_SOURCE_DIR}/imgui.cpp
        ${imgui_src_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_src_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_src_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_src_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_src_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_src_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
    )
    target_include_directories(imgui PUBLIC ${imgui_src_SOURCE_DIR} ${imgui_src_SOURCE_DIR}/backends)
    target_link_libraries(imgui PUBLIC SDL3::SDL3)

    # RetroAchievements: rcheevos (MIT) for the rc_client runtime, and stb_image (public domain)
    # to decode PNG badges. Both pinned; built from source as plain C/C++ files.
    FetchContent_Declare(rcheevos
        GIT_REPOSITORY https://github.com/RetroAchievements/rcheevos.git
        GIT_TAG v12.5.0
        GIT_SHALLOW TRUE)
    FetchContent_Declare(stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG 4e9b368bfc071e97206ea5c6adba68dd0cbf8897)
    FetchContent_MakeAvailable(rcheevos stb)
    set(rc ${rcheevos_SOURCE_DIR}/src)
    add_library(rcheevos STATIC
        ${rc}/rc_client.c ${rc}/rc_compat.c ${rc}/rc_util.c ${rc}/rc_version.c
        ${rc}/rapi/rc_api_common.c ${rc}/rapi/rc_api_info.c ${rc}/rapi/rc_api_runtime.c ${rc}/rapi/rc_api_user.c
        ${rc}/rcheevos/alloc.c ${rc}/rcheevos/condition.c ${rc}/rcheevos/condset.c ${rc}/rcheevos/consoleinfo.c
        ${rc}/rcheevos/format.c ${rc}/rcheevos/lboard.c ${rc}/rcheevos/memref.c ${rc}/rcheevos/operand.c
        ${rc}/rcheevos/rc_validate.c ${rc}/rcheevos/richpresence.c ${rc}/rcheevos/runtime.c
        ${rc}/rcheevos/runtime_progress.c ${rc}/rcheevos/trigger.c ${rc}/rcheevos/value.c
        ${rc}/rhash/md5.c)
    target_include_directories(rcheevos PUBLIC ${rcheevos_SOURCE_DIR}/include ${rc})
    target_include_directories(rcheevos SYSTEM PUBLIC ${stb_SOURCE_DIR})
    set_target_properties(rcheevos PROPERTIES C_STANDARD 99)

    # Options panel (owned by the options work; display.* / input.* settings).
    add_library(metrofuse_options STATIC
        src/options/options_panel.cpp
        src/options/presenter.cpp
        src/options/input.cpp
    )
    target_include_directories(metrofuse_options PUBLIC src)
    target_link_libraries(metrofuse_options PUBLIC imgui metrofuse_display metrofuse_app metrofuse_states SDL3::SDL3)

    # App layer: settings, launcher UI. Uses SDL (pref path, dialogs).
    add_library(metrofuse_app STATIC
        src/app/config.cpp
    )
    target_include_directories(metrofuse_app PUBLIC src)
    target_link_libraries(metrofuse_app PUBLIC metrofuse_i18n SDL3::SDL3)

    # Launcher: first-run ROM setup, themed menu, shortcuts, game session. The DM Sans font
    # (SIL OFL) and a few Lucide icons (ISC), both in assets/fonts, are compiled in.
    set(metrofuse_font_cpp "")
    foreach(pair "dmsans_regular;DMSans-Regular.ttf" "dmsans_semibold;DMSans-SemiBold.ttf"
                 "dmsans_bold;DMSans-Bold.ttf" "lucide_icons;lucide-icons.ttf")
        list(GET pair 0 name)
        list(GET pair 1 file)
        set(out "${CMAKE_CURRENT_BINARY_DIR}/font_${name}.cpp")
        add_custom_command(OUTPUT "${out}"
            COMMAND "${CMAKE_COMMAND}" -DINPUT=${PROJECT_SOURCE_DIR}/assets/fonts/${file}
                    -DOUTPUT=${out} -DSYMBOL=metrofuse_font_${name} -P ${PROJECT_SOURCE_DIR}/cmake/embed.cmake
            DEPENDS assets/fonts/${file} cmake/embed.cmake
            COMMENT "Embedding ${file}")
        list(APPEND metrofuse_font_cpp "${out}")
    endforeach()
    # The MetroFuse wordmark on the launcher card (assets/logo/metrofuse-logo.png, original artwork).
    # Always embedded: an optional EXISTS check was only evaluated at configure time, so build
    # folders configured before the file was added never picked it up.
    set(out "${CMAKE_CURRENT_BINARY_DIR}/image_logo.cpp")
    add_custom_command(OUTPUT "${out}"
        COMMAND "${CMAKE_COMMAND}" -DINPUT=${PROJECT_SOURCE_DIR}/assets/logo/metrofuse-logo.png -DOUTPUT=${out}
                -DSYMBOL=metrofuse_image_logo -P ${PROJECT_SOURCE_DIR}/cmake/embed.cmake
        DEPENDS assets/logo/metrofuse-logo.png cmake/embed.cmake
        COMMENT "Embedding the MetroFuse logo")
    list(APPEND metrofuse_font_cpp "${out}")
    # The app icon (512x512, rendered by metrofuse_icon), written into the shortcuts the launcher
    # creates so they carry the MetroFuse icon even from a plain build/metrofuse.
    set(icon_png "${CMAKE_CURRENT_BINARY_DIR}/metrofuse-icon-512.png")
    set(out "${CMAKE_CURRENT_BINARY_DIR}/image_icon.cpp")
    add_custom_command(OUTPUT "${out}"
        COMMAND metrofuse_icon --png 512 "${icon_png}"
        COMMAND "${CMAKE_COMMAND}" -DINPUT=${icon_png} -DOUTPUT=${out}
                -DSYMBOL=metrofuse_image_icon -P ${PROJECT_SOURCE_DIR}/cmake/embed.cmake
        DEPENDS metrofuse_icon cmake/embed.cmake
        COMMENT "Embedding the MetroFuse app icon")
    list(APPEND metrofuse_font_cpp "${out}")
    # License texts compiled in (shown in F1 > About and by --licenses).
    foreach(pair "license;LICENSE" "notices;THIRD_PARTY_NOTICES.md" "dmsans_ofl;assets/fonts/DMSans-OFL.txt")
        list(GET pair 0 name)
        list(GET pair 1 file)
        set(out "${CMAKE_CURRENT_BINARY_DIR}/text_${name}.cpp")
        add_custom_command(OUTPUT "${out}"
            COMMAND "${CMAKE_COMMAND}" -DINPUT=${PROJECT_SOURCE_DIR}/${file} -DOUTPUT=${out}
                    -DSYMBOL=metrofuse_text_${name} -P ${PROJECT_SOURCE_DIR}/cmake/embed.cmake
            DEPENDS ${file} cmake/embed.cmake
            COMMENT "Embedding ${file}")
        list(APPEND metrofuse_font_cpp "${out}")
    endforeach()
    add_library(metrofuse_launcher STATIC
        src/app/achievements.cpp
        src/app/credentials.cpp
        src/app/http.cpp
        src/app/app.cpp
        src/app/platform.cpp
        src/app/theme.cpp
        src/app/theme_colors.cpp
        src/app/licenses.cpp
        src/app/toast.cpp
        ${metrofuse_font_cpp}
    )
    target_include_directories(metrofuse_launcher PUBLIC src)
    target_link_libraries(metrofuse_launcher PUBLIC metrofuse_options metrofuse_gba metrofuse_core metrofuse_saves metrofuse_states imgui SDL3::SDL3)
    target_link_libraries(metrofuse_launcher PRIVATE rcheevos)
    target_compile_definitions(metrofuse_launcher PRIVATE METROFUSE_VERSION="${PROJECT_VERSION}")
    if(APPLE)
        target_link_libraries(metrofuse_launcher PRIVATE "-framework Security" "-framework CoreFoundation")
    elseif(WIN32)
        target_link_libraries(metrofuse_launcher PRIVATE winhttp advapi32)
    else()
        find_package(Threads REQUIRED)
        target_link_libraries(metrofuse_launcher PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
    endif()
    if(NOT MSVC)
        target_compile_options(metrofuse_launcher PRIVATE -Wall -Wextra)
    endif()

    # In-app updater: downloads (WinHTTP / curl), install per OS, and its settings tab.
    add_library(metrofuse_updater STATIC src/update/http.cpp src/update/updater.cpp src/update/update_ui.cpp)
    target_include_directories(metrofuse_updater PUBLIC src)
    target_link_libraries(metrofuse_updater PUBLIC metrofuse_update_core metrofuse_app imgui SDL3::SDL3)
    target_compile_definitions(metrofuse_updater PUBLIC METROFUSE_VERSION="${PROJECT_VERSION}")
    if(WIN32)
        target_link_libraries(metrofuse_updater PRIVATE winhttp)
    endif()
    if(NOT MSVC)
        target_compile_options(metrofuse_updater PRIVATE -Wall -Wextra)
    endif()
    target_link_libraries(metrofuse_launcher PUBLIC metrofuse_updater)

    add_executable(metrofuse src/main.cpp)
    if(WIN32)
        # GUI program (no console window), MetroFuse.exe with the app icon and version info.
        set(metrofuse_ico "${CMAKE_CURRENT_BINARY_DIR}/metrofuse.ico")
        add_custom_command(OUTPUT "${metrofuse_ico}" COMMAND metrofuse_icon --ico "${metrofuse_ico}" DEPENDS metrofuse_icon
                           COMMENT "Rendering metrofuse.ico")
        configure_file(packaging/windows/metrofuse.rc.in "${CMAKE_CURRENT_BINARY_DIR}/metrofuse.rc" @ONLY)
        target_sources(metrofuse PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/metrofuse.rc" "${metrofuse_ico}")
        set_target_properties(metrofuse PROPERTIES WIN32_EXECUTABLE TRUE OUTPUT_NAME MetroFuse)
        target_link_libraries(metrofuse PRIVATE ole32 shell32 uuid)
    endif()
    target_link_libraries(metrofuse PRIVATE metrofuse_launcher metrofuse_core metrofuse_gba metrofuse_options imgui SDL3::SDL3)
    if(APPLE AND METROFUSE_MACOS_BUNDLE)
        include(cmake/MacBundle.cmake)
        metrofuse_configure_bundle(metrofuse)
    elseif(APPLE)
        # Plain (unbundled) build/metrofuse: embed an Info.plist so the menu bar says MetroFuse, not metrofuse.
        set(metrofuse_plist "${CMAKE_CURRENT_BINARY_DIR}/metrofuse-embedded-Info.plist")
        file(WRITE "${metrofuse_plist}" "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
            "<plist version=\"1.0\"><dict>\n"
            "<key>CFBundleName</key><string>MetroFuse</string>\n"
            "<key>CFBundleDisplayName</key><string>MetroFuse</string>\n"
            "<key>CFBundleShortVersionString</key><string>${PROJECT_VERSION}</string>\n"
            "</dict></plist>\n")
        target_link_options(metrofuse PRIVATE "LINKER:-sectcreate,__TEXT,__info_plist,${metrofuse_plist}")
        set_property(TARGET metrofuse APPEND PROPERTY LINK_DEPENDS "${metrofuse_plist}")
    endif()
    if(METROFUSE_PACKAGE AND WIN32)
        # Release file: the single self-contained MetroFuse.exe (static runtime, SDL, zlib; licenses
        # embedded, see F1 > About and --licenses), copied under its release name.
        set(metrofuse_release_exe "${CMAKE_CURRENT_BINARY_DIR}/MetroFuse-${PROJECT_VERSION}-Windows-x64.exe")
        add_custom_target(package_exe
            COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:metrofuse>" "${metrofuse_release_exe}"
            DEPENDS metrofuse
            COMMENT "Building ${metrofuse_release_exe}"
            VERBATIM)
    endif()
    if(METROFUSE_PACKAGE AND UNIX AND NOT APPLE)
        # Portable Linux binary: static libstdc++/libgcc; SDL is static and loads X11/Wayland/
        # ALSA/PulseAudio/PipeWire at runtime. Installed into an AppDir by the workflow.
        target_link_options(metrofuse PRIVATE -static-libstdc++ -static-libgcc)
        set(metrofuse_png "${CMAKE_CURRENT_BINARY_DIR}/metrofuse.png")
        add_custom_command(OUTPUT "${metrofuse_png}" COMMAND metrofuse_icon --png 256 "${metrofuse_png}" DEPENDS metrofuse_icon
                           COMMENT "Rendering metrofuse.png")
        add_custom_target(metrofuse_linux_icon ALL DEPENDS "${metrofuse_png}")
        install(TARGETS metrofuse RUNTIME DESTINATION bin)
        install(FILES packaging/linux/metrofuse.desktop DESTINATION share/applications)
        install(FILES "${metrofuse_png}" DESTINATION share/icons/hicolor/256x256/apps)
        install(FILES LICENSE THIRD_PARTY_NOTICES.md packaging/README-Linux.txt DESTINATION share/doc/metrofuse)
        install(FILES assets/fonts/DMSans-OFL.txt assets/fonts/Lucide-LICENSE.txt DESTINATION share/doc/metrofuse)
    endif()
else()
    message(WARNING "SDL3 not found and METROFUSE_FETCH_SDL is OFF: only building metrofuse_core")
endif()

if(METROFUSE_BUILD_TESTS)
    enable_testing()
    add_executable(metrofuse_romcheck tests/romcheck.cpp)
    target_link_libraries(metrofuse_romcheck PRIVATE metrofuse_core)
    add_test(NAME sha1_selftest COMMAND metrofuse_romcheck --selftest)
    add_executable(metrofuse_display_test tests/display_test.cpp)
    target_link_libraries(metrofuse_display_test PRIVATE metrofuse_display)
    add_test(NAME display_options COMMAND metrofuse_display_test)
    add_executable(metrofuse_save_test tests/save_import_test.cpp)
    target_link_libraries(metrofuse_save_test PRIVATE metrofuse_saves)
    add_test(NAME save_import COMMAND metrofuse_save_test)
    add_executable(metrofuse_states_test tests/states_test.cpp)
    target_link_libraries(metrofuse_states_test PRIVATE metrofuse_states)
    add_test(NAME save_state_slots COMMAND metrofuse_states_test)
    add_executable(metrofuse_cheats_test tests/cheats_test.cpp)
    add_executable(metrofuse_i18n_test tests/i18n_test.cpp)
    target_link_libraries(metrofuse_i18n_test PRIVATE metrofuse_i18n)
    target_compile_definitions(metrofuse_i18n_test PRIVATE METROFUSE_LANG_DIR="${PROJECT_SOURCE_DIR}/assets/lang")
    add_test(NAME translations COMMAND metrofuse_i18n_test)
    target_include_directories(metrofuse_cheats_test PRIVATE src)
    add_test(NAME cheats COMMAND metrofuse_cheats_test)
    add_executable(metrofuse_hold_morph_test tests/hold_morph_test.cpp)
    target_include_directories(metrofuse_hold_morph_test PRIVATE src)
    add_test(NAME hold_morph COMMAND metrofuse_hold_morph_test)
    add_executable(metrofuse_redux_test tests/redux_test.cpp)
    target_include_directories(metrofuse_redux_test PRIVATE src)
    add_test(NAME redux_mode COMMAND metrofuse_redux_test)
    add_executable(metrofuse_update_test tests/update_test.cpp)
    target_link_libraries(metrofuse_update_test PRIVATE metrofuse_update_core)
    target_compile_definitions(metrofuse_update_test PRIVATE METROFUSE_TEST_DATA="${PROJECT_SOURCE_DIR}/tests/data")
    add_test(NAME updates COMMAND metrofuse_update_test)
    add_executable(metrofuse_state_test tests/state_test.cpp)
    target_link_libraries(metrofuse_state_test PRIVATE metrofuse_gba metrofuse_core)
    # Uses the user's own ROM when it sits in the source folder (it is never committed).
    target_compile_definitions(metrofuse_state_test PRIVATE
        METROFUSE_TEST_ROM="${PROJECT_SOURCE_DIR}/Metroid Fusion.gba"
        METROFUSE_TEST_BIOS="${PROJECT_SOURCE_DIR}/gba_bios.bin")
    add_test(NAME save_state_determinism COMMAND metrofuse_state_test)
    set_tests_properties(save_state_determinism PROPERTIES SKIP_RETURN_CODE 77)
    if(TARGET metrofuse_launcher)
        # RetroAchievements glue against canned server replies (offline).
        add_executable(metrofuse_achievements_test tests/achievements_test.cpp)
        target_link_libraries(metrofuse_achievements_test PRIVATE metrofuse_launcher rcheevos)
        add_test(NAME achievements COMMAND metrofuse_achievements_test)
        # Controller bindings driven through a virtual SDL gamepad.
        add_executable(metrofuse_input_test tests/input_test.cpp)
        target_link_libraries(metrofuse_input_test PRIVATE metrofuse_launcher)
        add_test(NAME input COMMAND metrofuse_input_test)
    endif()
endif()

# Analysis tools. Their output is derived from the ROM and goes to generated/ (gitignored).
if(METROFUSE_BUILD_TOOLS)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(CAPSTONE IMPORTED_TARGET capstone>=5)
    endif()
    if(TARGET PkgConfig::CAPSTONE)
        add_executable(metrofuse_disasm tools/metrofuse_disasm/main.cpp)
        target_link_libraries(metrofuse_disasm PRIVATE metrofuse_core PkgConfig::CAPSTONE)
        if(NOT MSVC)
            target_compile_options(metrofuse_disasm PRIVATE -Wall -Wextra)
        endif()
    else()
        message(WARNING "Capstone not found via pkg-config: skipping metrofuse_disasm (brew install capstone pkgconf)")
    endif()
endif()
