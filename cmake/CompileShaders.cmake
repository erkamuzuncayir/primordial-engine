# ==============================================================================
# Incremental shader compilation directly to config target directory
# ==============================================================================
function(PE_CompileShaders targetName shaderInputDir)
  find_program(glslcExecutable NAMES glslc)
  if(NOT glslcExecutable)
    message(WARNING "glslc is not available. GLSL shaders won't be compiled.")
    return()
  endif()

  file(GLOB_RECURSE GLSL_FILES CONFIGURE_DEPENDS "${shaderInputDir}/*.glsl")
  if(NOT GLSL_FILES)
    message(WARNING "There is no GLSL shader file.")
    return()
  endif()

  set(compiledShaders "")

  set(targetShaderDir "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/shaders")

  foreach(shaderFile ${GLSL_FILES})
    cmake_path(GET shaderFile STEM fileName)

    # Vertex Shader
    set(vertOut "${targetShaderDir}/${fileName}_vert.spv")
    add_custom_command(
      OUTPUT "${vertOut}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${targetShaderDir}"
      COMMAND ${glslcExecutable} -fshader-stage=vert -DVERTEX_SHADER
          -o "${vertOut}" "${shaderFile}"
      DEPENDS "${shaderFile}"
      COMMENT "Compiling vertex shader: ${fileName}.glsl"
      VERBATIM
    )
    list(APPEND compiledShaders "${vertOut}")

    # Fragment Shader
    set(fragOut "${targetShaderDir}/${fileName}_frag.spv")
    add_custom_command(
      OUTPUT "${fragOut}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${targetShaderDir}"
      COMMAND ${glslcExecutable} -fshader-stage=frag -DFRAGMENT_SHADER
          -o "${fragOut}" "${shaderFile}"
      DEPENDS "${shaderFile}"
      COMMENT "Compiling fragment shader: ${fileName}.glsl"
      VERBATIM
    )
    list(APPEND compiledShaders "${fragOut}")
  endforeach()

  # Custom Target
  if(compiledShaders)
    set(shaderTargetName "${targetName}_CompileShaders")
    add_custom_target(${shaderTargetName} DEPENDS ${compiledShaders})
    add_dependencies(${targetName} ${shaderTargetName})
  endif()
endfunction()
