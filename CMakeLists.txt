# Copyright © 2026 Peter Adrianov
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.21)
project(hunk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.9 REQUIRED COMPONENTS Core Concurrent Widgets Svg WebEngineWidgets)

include(FetchContent)
set(QLEMENTINE_SANDBOX OFF CACHE BOOL "" FORCE)
set(QLEMENTINE_SHOWCASE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(Qlementine
  GIT_REPOSITORY https://github.com/oclero/qlementine.git
  GIT_TAG v1.5.2
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Qlementine)

set(CMARK_TESTS OFF CACHE BOOL "" FORCE)
set(CMARK_SHARED OFF CACHE BOOL "" FORCE)
set(CMARK_STATIC ON CACHE BOOL "" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_AUTOMOC OFF)
FetchContent_Declare(cmark_gfm
  GIT_REPOSITORY https://github.com/github/cmark-gfm.git
  GIT_TAG 0.29.0.gfm.13
  GIT_SHALLOW TRUE
  PATCH_COMMAND sed -i.bak "/ext_scanners\\.re/d" extensions/CMakeLists.txt
)
FetchContent_MakeAvailable(cmark_gfm)
set(CMAKE_AUTOMOC ON)

add_library(hunk_core STATIC
  src/DiffDoc.cpp
  src/DiffParse.cpp
  src/DiffGap.cpp
  src/DiffPath.cpp
  src/WordDiff.cpp
  src/DiffZip.cpp
  src/DiffFold.cpp
  src/SyntaxLang.cpp
  src/SyntaxScan.cpp
  src/SyntaxText.cpp
  src/SyntaxWord.cpp
  src/SyntaxMd.cpp
  src/SyntaxEmph.cpp
  src/ReviewExport.cpp
)
target_include_directories(hunk_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(hunk_core PUBLIC Qt6::Core)

add_executable(hunk
  src/main.cpp
  src/Smoke.cpp
  src/MainWindow.cpp
  src/MainActions.cpp
  src/MainRecent.cpp
  src/BarIcon.cpp
  src/MainChrome.cpp
  src/MainAbout.cpp
  src/MainFilePane.cpp
  src/MainTree.cpp
  src/MainHide.cpp
  src/MainSeen.cpp
  src/MainFileRow.cpp
  src/MainHeat.cpp
  src/MainNotes.cpp
  src/MainNoteList.cpp
  src/MainNoteKeep.cpp
  src/MainReview.cpp
  src/MainLoad.cpp
  src/MainPath.cpp
  src/MainRef.cpp
  src/MainRefFilter.cpp
  src/MainWatch.cpp
  src/MainDisk.cpp
  src/MainDiskWalk.cpp
  src/Theme.cpp
  src/MainTheme.cpp
  src/DiffCanvas.cpp
  src/MdView.cpp
  src/MdChrome.cpp
  src/MdRead.cpp
  src/MdHtml.cpp
  src/DiffScroll.cpp
  src/DiffInput.cpp
  src/DiffTitle.cpp
  src/DiffSplit.cpp
  src/DiffSelect.cpp
  src/DiffMark.cpp
  src/DiffHeader.cpp
  src/DiffPaint.cpp
  src/DiffFoldView.cpp
  src/DiffLayout.cpp
  src/DiffSide.cpp
  src/DiffSyntax.cpp
  src/DiffColors.cpp
  src/GitRepo.cpp
  src/GitBlob.cpp
  src/GitCmd.cpp
  src/GitStamp.cpp
  src/GitDrift.cpp
  src/GitRef.cpp
  src/GitRange.cpp
  src/GitPoint.cpp
  src/GitFork.cpp
  src/ReviewStore.cpp
  src/ReviewSync.cpp
  src/ReviewShift.cpp
  src/ReviewSpan.cpp
)
target_include_directories(hunk PRIVATE
  ${cmark_gfm_SOURCE_DIR}/src
  ${cmark_gfm_BINARY_DIR}/src
  ${cmark_gfm_SOURCE_DIR}/extensions
)
target_compile_definitions(hunk PRIVATE CMARK_GFM_STATIC_DEFINE CMARK_GFM_EXTENSIONS_STATIC_DEFINE)
target_link_libraries(hunk PRIVATE
  hunk_core Qt6::Widgets Qt6::Concurrent Qt6::WebEngineWidgets qlementine
  libcmark-gfm-extensions_static libcmark-gfm_static
)
target_compile_definitions(hunk PRIVATE HUNK_VERSION="${PROJECT_VERSION}")

set_source_files_properties(icons/hunk.png PROPERTIES QT_RESOURCE_ALIAS hunk.png)
qt_add_resources(hunk hunk_icons PREFIX "/icons" FILES icons/hunk.png)

set_source_files_properties(third_party/markdown/github-light.css PROPERTIES QT_RESOURCE_ALIAS github-light.css)
set_source_files_properties(third_party/markdown/github-dark.css PROPERTIES QT_RESOURCE_ALIAS github-dark.css)
set_source_files_properties(third_party/markdown/highlight.js PROPERTIES QT_RESOURCE_ALIAS highlight.js)
set_source_files_properties(third_party/markdown/hl-light.css PROPERTIES QT_RESOURCE_ALIAS hl-light.css)
set_source_files_properties(third_party/markdown/hl-dark.css PROPERTIES QT_RESOURCE_ALIAS hl-dark.css)
qt_add_resources(hunk hunk_md PREFIX "/md" FILES
  third_party/markdown/github-light.css
  third_party/markdown/github-dark.css
  third_party/markdown/highlight.js
  third_party/markdown/hl-light.css
  third_party/markdown/hl-dark.css
)

if(APPLE)
  configure_file(icons/Info.plist.in ${CMAKE_BINARY_DIR}/Info.plist @ONLY)
  target_sources(hunk PRIVATE src/MacIcon.mm icons/hunk.icns)
  set_source_files_properties(icons/hunk.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  target_link_libraries(hunk PRIVATE "-framework AppKit")
  set_target_properties(hunk PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_BINARY_DIR}/Info.plist
    MACOSX_BUNDLE_ICON_FILE hunk.icns
  )
  add_custom_command(TARGET hunk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/hunk
    COMMAND ${CMAKE_COMMAND} -E create_symlink hunk.app/Contents/MacOS/hunk ${CMAKE_BINARY_DIR}/hunk
  )
endif()

include(GNUInstallDirs)
install(TARGETS hunk
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(UNIX AND NOT APPLE)
  install(FILES icons/hunk.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
  install(DIRECTORY icons/hicolor DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons)
endif()

add_executable(hunk_test
  tests/parse_test.cpp
  tests/file_test.cpp
  tests/fold_test.cpp
  tests/gap_test.cpp
  tests/review_test.cpp
  tests/shift_test.cpp
  src/ReviewStore.cpp
  src/ReviewSync.cpp
  src/ReviewShift.cpp
  src/ReviewSpan.cpp
)
target_link_libraries(hunk_test PRIVATE hunk_core Qt6::Core)

add_executable(hunk_syntax tests/syntax_test.cpp tests/md_test.cpp tests/emph_test.cpp)
target_link_libraries(hunk_syntax PRIVATE hunk_core Qt6::Core)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(hunk_core PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(hunk PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(hunk_test PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(hunk_syntax PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME parse COMMAND hunk_test)
add_test(NAME syntax COMMAND hunk_syntax)
