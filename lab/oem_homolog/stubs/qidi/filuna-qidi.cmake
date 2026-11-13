# Filuna — passado por -DCMAKE_PROJECT_INCLUDE na régua de paridade do QIDIStudio (ver README.md).
# Ao fim da configuração, acrescenta "-include filuna_forca.hpp" aos alvos da interface e da CLI.
if(NOT FILUNA_QIDI_FORCA_REGISTRADO)
  set(FILUNA_QIDI_FORCA_REGISTRADO ON)
  set(_filuna_forca "${CMAKE_CURRENT_LIST_DIR}/filuna_forca.hpp")
  function(_filuna_qidi_forca)
    foreach(_alvo libslic3r_gui QIDIStudio)
      if(TARGET ${_alvo})
        # só C++: o alvo também compila C (gstQIDIsrc.c) e um cabeçalho pré-compilado de C
        target_compile_options(${_alvo} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-include>" "$<$<COMPILE_LANGUAGE:CXX>:${_filuna_forca}>")
      endif()
    endforeach()
  endfunction()
  cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL _filuna_qidi_forca)
endif()
