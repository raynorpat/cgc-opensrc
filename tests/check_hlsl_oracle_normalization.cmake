if(NOT DEFINED WORK_DIR)
    message(FATAL_ERROR "WORK_DIR must be defined")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/hlsl_oracle_records.cmake")
file(MAKE_DIRECTORY "${WORK_DIR}")
set(under "${WORK_DIR}/under.hlsl")
set(reference "${WORK_DIR}/reference.hlsl")
file(REMOVE "${under}" "${reference}")

file(WRITE "${under}"
    "// cgc-bind interface in position float4 POSITION0\n")
file(WRITE "${reference}"
    "//var float4 position : $vin.POSITION : POSITION : 0 : 1\n")
hlsl_read_public_records("${under}" under_records)
hlsl_read_public_records("${reference}" reference_records)
if(NOT under_records STREQUAL reference_records)
    message(FATAL_ERROR
        "input directions did not normalize equally:\n"
        "under=${under_records}\nreference=${reference_records}")
endif()
if(NOT under_records MATCHES "semantic[|]in[|]")
    message(FATAL_ERROR "normalized input record lost its direction")
endif()

file(WRITE "${reference}"
    "//var float4 position : $vout.POSITION : POSITION : 0 : 1\n")
hlsl_read_public_records("${reference}" reference_records)
if(under_records STREQUAL reference_records)
    message(FATAL_ERROR
        "opposite input/output directions normalized as equal")
endif()
if(NOT reference_records MATCHES "semantic[|]out[|]")
    message(FATAL_ERROR "NVIDIA $vout record lost its output direction")
endif()

file(WRITE "${under}"
    "// cgc-bind interface out result float4 COLOR0\n")
file(WRITE "${reference}"
    "//var float4 main : $vout.COLOR0 : COLOR0 : -1 : 1\n")
hlsl_read_public_records("${under}" under_records)
hlsl_read_public_records("${reference}" reference_records)
if(NOT under_records STREQUAL reference_records)
    message(FATAL_ERROR
        "result output records did not normalize equally:\n"
        "under=${under_records}\nreference=${reference_records}")
endif()
