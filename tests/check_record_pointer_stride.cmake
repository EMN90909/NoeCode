if(NOT DEFINED NOQERI OR NOT DEFINED SOURCE)
  message(FATAL_ERROR "NOQERI and SOURCE are required")
endif()

execute_process(
  COMMAND "${NOQERI}" nir "${SOURCE}" -O1
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE nir
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "Noqeri NIR generation failed: ${err}")
endif()

# Pair is laid out as u32 + 4 bytes padding + u64, so pointer indexing must
# advance by 16 bytes. A stride of one previously corrupted adjacent records.
if(NOT nir MATCHES "ptr_offset[^\n]*\\* 16")
  message(FATAL_ERROR "record pointer stride regression: expected 16-byte ptr_offset\n${nir}")
endif()
