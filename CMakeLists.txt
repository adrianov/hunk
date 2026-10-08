cmake_minimum_required(VERSION 3.21)
project(hunk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Concurrent Widgets)

add_library(hunk_core STATIC
  src/DiffDoc.cpp
  src/DiffParse.cpp
  src/DiffPath.cpp
  src/WordDiff.cpp
  src/DiffZip.cpp
  src/DiffFold.cpp
  src/SyntaxLang.cpp
  src/SyntaxScan.cpp
  src/SyntaxText.cpp
  src/SyntaxWord.cpp
  src/ReviewExport.cpp
)
target_include_directories(hunk_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(hunk_core PUBLIC Qt6::Core)

add_executable(hunk
  src/main.cpp
  src/Smoke.cpp
  src/MainWindow.cpp
  src/MainActions.cpp
  src/MainChrome.cpp
  src/MainTree.cpp
  src/MainNotes.cpp
  src/MainReview.cpp
  src/MainLoad.cpp
  src/MainWatch.cpp
  src/MainDisk.cpp
  src/MainDiskWalk.cpp
  src/Theme.cpp
  src/DiffCanvas.cpp
  src/DiffScroll.cpp
  src/DiffInput.cpp
  src/DiffHeader.cpp
  src/DiffPaint.cpp
  src/DiffFoldView.cpp
  src/DiffLayout.cpp
  src/DiffSide.cpp
  src/DiffSyntax.cpp
  src/DiffColors.cpp
  src/GitRepo.cpp
  src/GitCmd.cpp
  src/GitStamp.cpp
  src/GitRange.cpp
  src/ReviewStore.cpp
)
target_link_libraries(hunk PRIVATE hunk_core Qt6::Widgets Qt6::Concurrent)

add_executable(hunk_test tests/parse_test.cpp)
target_link_libraries(hunk_test PRIVATE hunk_core Qt6::Core)

add_executable(hunk_syntax tests/syntax_test.cpp)
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
