if(NOT DEFINED PRIMESTAGE_SOURCE_DIR)
  message(FATAL_ERROR "PRIMESTAGE_SOURCE_DIR is required")
endif()
if(NOT DEFINED PRIMESTAGE_BINARY_DIR)
  message(FATAL_ERROR "PRIMESTAGE_BINARY_DIR is required")
endif()
if(NOT DEFINED PRIMESTAGE_CXX_COMPILER)
  message(FATAL_ERROR "PRIMESTAGE_CXX_COMPILER is required")
endif()

set(install_prefix "${PRIMESTAGE_BINARY_DIR}/find-package-prefix")
set(consumer_binary_dir "${PRIMESTAGE_BINARY_DIR}/find-package-smoke-build")
set(consumer_source_dir "${PRIMESTAGE_SOURCE_DIR}/tests/cmake/find_package_smoke")

file(REMOVE_RECURSE "${install_prefix}" "${consumer_binary_dir}")

execute_process(
  COMMAND "${CMAKE_COMMAND}" --install "${PRIMESTAGE_BINARY_DIR}" --prefix "${install_prefix}"
  RESULT_VARIABLE installResult
  OUTPUT_VARIABLE installStdout
  ERROR_VARIABLE installStderr
)
if(NOT installResult EQUAL 0)
  message(FATAL_ERROR
    "PrimeStage install step failed.\nstdout:\n${installStdout}\nstderr:\n${installStderr}")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}"
    -S "${consumer_source_dir}"
    -B "${consumer_binary_dir}"
    -DCMAKE_CXX_COMPILER=${PRIMESTAGE_CXX_COMPILER}
    -DCMAKE_BUILD_TYPE=Debug
    -DCMAKE_PREFIX_PATH=${install_prefix}
  RESULT_VARIABLE configureResult
  OUTPUT_VARIABLE configureStdout
  ERROR_VARIABLE configureStderr
)
if(NOT configureResult EQUAL 0)
  message(FATAL_ERROR
    "find_package configure step failed.\nstdout:\n${configureStdout}\nstderr:\n${configureStderr}")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${consumer_binary_dir}"
  RESULT_VARIABLE buildResult
  OUTPUT_VARIABLE buildStdout
  ERROR_VARIABLE buildStderr
)
if(NOT buildResult EQUAL 0)
  message(FATAL_ERROR
    "find_package consumer build failed.\nstdout:\n${buildStdout}\nstderr:\n${buildStderr}")
endif()
