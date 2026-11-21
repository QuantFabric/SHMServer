cmake_minimum_required(VERSION 3.16)
project(SHMServerPythonSwig LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(SWIG 4.0 REQUIRED)

execute_process(
    COMMAND ${Python3_EXECUTABLE} -c
        "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX') or '')"
    OUTPUT_VARIABLE SHMSERVER_PYTHON_EXT_SUFFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE SHMSERVER_EXT_SUFFIX_RESULT
)
if(NOT SHMSERVER_EXT_SUFFIX_RESULT EQUAL 0
   OR NOT SHMSERVER_PYTHON_EXT_SUFFIX)
    set(SHMSERVER_PYTHON_EXT_SUFFIX ${CMAKE_SHARED_MODULE_SUFFIX})
endif()
message(STATUS "Python extension suffix: ${SHMSERVER_PYTHON_EXT_SUFFIX}")

set(SHMSERVER_SWIG_OUTPUT_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)
set(SHMSERVER_HEADER_ROOT
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
set(SHMSERVER_UTILS_ROOT
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utils
)
if(NOT EXISTS ${SHMSERVER_HEADER_ROOT}/Common.hpp)
    set(SHMSERVER_HEADER_ROOT
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SHMServer
    )
    set(SHMSERVER_UTILS_ROOT
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/Utils
    )
endif()

set(SHMSERVER_SWIG_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/SHMSwig.hpp
    ${SHMSERVER_HEADER_ROOT}/Common.hpp
    ${SHMSERVER_HEADER_ROOT}/MPMCQueue.hpp
    ${SHMSERVER_HEADER_ROOT}/SHMConnection.hpp
    ${SHMSERVER_HEADER_ROOT}/SHMServer.hpp
    ${SHMSERVER_HEADER_ROOT}/SPSCQueue.hpp
    ${SHMSERVER_UTILS_ROOT}/MarketData.hpp
    ${SHMSERVER_UTILS_ROOT}/PackMessage.hpp
    ${SHMSERVER_UTILS_ROOT}/Util.hpp
)

function(add_swig_python_module module_name)
    set(interface_file
        ${CMAKE_CURRENT_SOURCE_DIR}/${module_name}.i
    )
    set(generated_source
        ${SHMSERVER_SWIG_OUTPUT_DIR}/${module_name}_wrap.cxx
    )
    set(generated_python
        ${SHMSERVER_SWIG_OUTPUT_DIR}/${module_name}.py
    )
    set(generated_outputs
        ${generated_source}
        ${generated_python}
    )
    if(module_name STREQUAL "shm_server")
        list(APPEND generated_outputs
            ${SHMSERVER_SWIG_OUTPUT_DIR}/shm_server_wrap.h
        )
    endif()

    add_custom_command(
        OUTPUT ${generated_outputs}
        COMMAND ${CMAKE_COMMAND} -E make_directory
            ${SHMSERVER_SWIG_OUTPUT_DIR}
        COMMAND ${SWIG_EXECUTABLE}
            -python
            -c++
            -w312,401
            -I${CMAKE_CURRENT_SOURCE_DIR}
            -I${SHMSERVER_HEADER_ROOT}
            -I${SHMSERVER_UTILS_ROOT}
            -outdir ${SHMSERVER_SWIG_OUTPUT_DIR}
            -o ${generated_source}
            ${interface_file}
        DEPENDS
            ${interface_file}
            ${SHMSERVER_SWIG_DEPENDS}
        COMMENT "Generating SWIG Python bindings for ${module_name}"
        VERBATIM
    )

    add_library(${module_name} MODULE ${generated_source})
    target_compile_features(${module_name} PRIVATE cxx_std_17)
    target_include_directories(${module_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Utils
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SHMServer
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/Utils
    )
    target_link_libraries(${module_name} PRIVATE
        Python3::Module
        pthread
        rt
    )
    set_target_properties(${module_name} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "_${module_name}"
        SUFFIX "${SHMSERVER_PYTHON_EXT_SUFFIX}"
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endfunction()

set(SHMSERVER_PYTHON_MODULES
    pack_message
    shm_server
    shm_connection
    spscqueue
)

foreach(module_name IN LISTS SHMSERVER_PYTHON_MODULES)
    add_swig_python_module(${module_name})
endforeach()

target_sources(
    shm_server
    PRIVATE ${SHMSERVER_SWIG_OUTPUT_DIR}/shm_server_wrap.h
)

install(
    TARGETS ${SHMSERVER_PYTHON_MODULES}
    LIBRARY DESTINATION .
    RUNTIME DESTINATION .
)
install(
    FILES
        ${SHMSERVER_SWIG_OUTPUT_DIR}/pack_message.py
        ${SHMSERVER_SWIG_OUTPUT_DIR}/shm_server.py
        ${SHMSERVER_SWIG_OUTPUT_DIR}/shm_connection.py
        ${SHMSERVER_SWIG_OUTPUT_DIR}/spscqueue.py
    DESTINATION .
)

add_custom_target(
    pip_wheel
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/dist
    COMMAND ${Python3_EXECUTABLE} -m pip wheel
        ${CMAKE_CURRENT_SOURCE_DIR}
        --no-deps
        --wheel-dir ${CMAKE_CURRENT_SOURCE_DIR}/dist
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "Building shmserver pip wheel"
)

include(CTest)
if(BUILD_TESTING)
    add_test(
        NAME SHMServerSwigBindings
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_bindings.py
    )
    set_tests_properties(SHMServerSwigBindings PROPERTIES
        ENVIRONMENT
            "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}:${SHMSERVER_SWIG_OUTPUT_DIR}"
        TIMEOUT 30
    )
endif()
