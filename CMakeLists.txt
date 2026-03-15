cmake_minimum_required(VERSION 3.16)

# Hot Reload for MSVC (optional)
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
      "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,\
$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,\
$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(TextAnalysisTool LANGUAGES CXX)

# Build the actual Qt app in the subfolder
add_subdirectory(TextAnalysisTool)
