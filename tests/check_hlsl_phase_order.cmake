if(NOT DEFINED SOURCE)
    message(FATAL_ERROR "SOURCE is required")
endif()

file(READ "${SOURCE}/hlsl_hal.c" source_text)
string(FIND "${source_text}" "HlslLowerProgram(&module" lower_index)
string(FIND "${source_text}" "HlslBuildEntryWrapper(&module" wrapper_index)
string(FIND "${source_text}" "HlslLegalizeModule(&module" legalize_index)
string(FIND "${source_text}" "HlslAllocateBindings(&module" binding_index)
string(FIND "${source_text}" "HlslValidateModule(&module" validate_index)

if(lower_index LESS 0 OR wrapper_index LESS 0 OR legalize_index LESS 0 OR
   binding_index LESS 0 OR validate_index LESS 0)
    message(FATAL_ERROR "HLSL pipeline phase call is missing")
endif()
if(NOT lower_index LESS wrapper_index OR
   NOT wrapper_index LESS legalize_index OR
   NOT legalize_index LESS binding_index OR
   NOT binding_index LESS validate_index)
    message(FATAL_ERROR
        "HLSL pipeline must be lower, wrapper, legalize, bind, validate")
endif()
