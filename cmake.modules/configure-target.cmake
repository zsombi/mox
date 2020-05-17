#
# Copyright (C) 2017-2020 bitWelder
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see
# <http://www.gnu.org/licenses/>
#

include(configure-platform)

# local function, configure common options
macro(__common_config arg_target)
    # compile defs
    if (MOX_ENABLE_LOGS)
        target_compile_definitions(${arg_target} PUBLIC MOX_ENABLE_LOGS)
    endif()

    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        target_compile_definitions(${arg_target} PUBLIC DEBUG)
    endif()

    if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
        target_compile_definitions(${arg_target} PUBLIC MOX_HOST_LINUX)
    endif()

    if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
        target_compile_definitions(${arg_target} PUBLIC MOX_HOST_MACOSX)
    endif()

    # compile options
    target_compile_options(${arg_target} PUBLIC -std=c++17 -Werror -Wall -W -fPIC -pthread)

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        target_compile_options(${arg_target} PUBLIC -stdlib=libc++ -Winconsistent-missing-override)
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(${arg_target} PUBLIC -Wno-subobject-linkage)
    endif()

    # linker options
    if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
        target_link_libraries(${arg_target} PUBLIC "-framework Foundation -framework CoreFoundation")
        # target_link_options available from 3.13.5
    #    target_link_options(${arg_target} PUBLIC Wl F/Library/Frameworks)
    endif()

    if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)
        target_link_libraries(${arg_target} PRIVATE PkgConfig::deps)
        target_link_libraries(${arg_target} PRIVATE ${LIBGLIB_LIBRARIES})
    endif()

endmacro()

# Configure a library
# arguments
# target: the target to configure
macro(__configure_library arg_target)
    target_compile_definitions(${arg_target} PUBLIC MOX_LIBRARY)
    # symbol visibility
    target_compile_options(${arg_target} PUBLIC -fvisibility=hidden)

    if (BUILD_SHARED_LIBS)
        if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
            target_compile_options(${arg_target} PUBLIC -shared)
        endif()
#        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-s")
#        target_link_options(${arg_target} PUBLIC -s)
    endif()
endmacro()

# Configure an executable
# target: the name of the executable
macro(__configure_exe arg_target)
endmacro()

function(configure_target arg_target)
    __common_config(${arg_target})

    get_target_property(target_type ${arg_target} TYPE)
    if (target_type STREQUAL "EXECUTABLE")
        __configure_exe(${arg_target})
    else()
        __configure_library(${arg_target})
    endif()

    print_config(${arg_target})
endfunction()
