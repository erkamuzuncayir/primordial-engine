# ==============================================================================
# Code formatting using clang-format
# ==============================================================================
function(PE_FormatProject)
  list(APPEND filesToFormat ${PE_SOURCE_FILES} ${PE_HEADER_FILES})
  list(FILTER filesToFormat EXCLUDE REGEX ".*/src/Vendor/.*")
  set(targetName "FormatProject")
  find_program(clangFormatExe clang-format)
  if(clangFormatExe)
    add_custom_target(${targetName}
      COMMAND ${clangFormatExe} -style=file -i ${filesToFormat}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Running clang-format on project's files..."
    )
  else()
    message(WARNING
        "clang-format executable not found. The target ${targetName} is disabled.")
  endif()
endfunction()
