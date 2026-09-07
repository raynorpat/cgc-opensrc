foreach(required CMAKE_LISTS VALIDATOR)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

file(READ "${CMAKE_LISTS}" cmake_source)
file(READ "${VALIDATOR}" validator_source)

set(expected_mappings
    "hlslv|vs_3_0|TRUE"
    "hlslf|ps_3_0|TRUE"
    "hlslv40|vs_4_0|FALSE"
    "hlslg40|gs_4_0|FALSE"
    "hlslf40|ps_4_0|FALSE"
    "hlslv50|vs_5_0|FALSE"
    "hlslg50|gs_5_0|FALSE"
    "hlslf50|ps_5_0|FALSE")

foreach(mapping IN LISTS expected_mappings)
    string(REPLACE "|" ";" fields "${mapping}")
    list(GET fields 0 profile)
    list(GET fields 1 target)
    list(GET fields 2 legacy_syntax)
    if(NOT cmake_source MATCHES
       "profile STREQUAL \"${profile}\"[^\n]*\n[ \t]*set\\(target ${target}\\)[^\n]*\n[ \t]*set\\(legacy_syntax ${legacy_syntax}\\)")
        message(FATAL_ERROR
            "add_hlsl_validation_case does not map ${profile} to ${target} with LEGACY_SYNTAX=${legacy_syntax}")
    endif()
endforeach()

if(NOT validator_source MATCHES
   "foreach\\(required CGC FXC PROFILE TARGET SOURCE OUTPUT BYTECODE CONFIG LEGACY_SYNTAX\\)")
    message(FATAL_ERROR "validator does not require LEGACY_SYNTAX")
endif()
if(NOT validator_source MATCHES "if\\(LEGACY_SYNTAX\\)")
    message(FATAL_ERROR "validator does not select syntax mode from LEGACY_SYNTAX")
endif()
if(NOT validator_source MATCHES
   "set\\(syntax_mode /Gec\\)[^\n]*\n[ \t]*else\\(\\)[^\n]*\n[ \t]*set\\(syntax_mode /Ges\\)")
    message(FATAL_ERROR "validator does not select exact /Gec and /Ges modes")
endif()
string(FIND "${validator_source}"
    [=[set(fxc_arguments /nologo /WX ${syntax_mode} /E main /T "${TARGET}")]=]
    command_index)
if(command_index EQUAL -1)
    message(FATAL_ERROR "validator command does not retain the public main entry and exact target")
endif()
foreach(required_failure_contract
        "RESULT_VARIABLE fxc_result"
        "if(NOT fxc_result EQUAL 0)"
        "FXC rejected")
    string(FIND "${validator_source}" "${required_failure_contract}"
        failure_contract_index)
    if(failure_contract_index EQUAL -1)
        message(FATAL_ERROR
            "validator does not fail on nonzero fxc status: missing ${required_failure_contract}")
    endif()
endforeach()
