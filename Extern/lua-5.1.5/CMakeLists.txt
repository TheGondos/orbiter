# MIT License
#
# Copyright (c) 2019 Lukas Böger
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# File from https://github.com/lubgr/lua-cmake
# 20220427 - Orbiter : use "src" directory instead of "upstream" submodule

cmake_minimum_required(VERSION 3.12)
project(lua-cmake VERSION 1.0.0 DESCRIPTION "Lua build for a project subdirectory" LANGUAGES C)

option(BUILD_SHARED_LIBS "Build as shared library" ON)

set(srcDir src)

set(srcFiles lapi.c lauxlib.c lbaselib.c lcode.c ldblib.c ldebug.c ldo.c
    ldump.c lfunc.c lgc.c linit.c liolib.c llex.c lmathlib.c lmem.c loadlib.c lobject.c lopcodes.c
    loslib.c lparser.c lstate.c lstring.c lstrlib.c ltable.c ltablib.c ltm.c lundump.c
    lvm.c lzio.c)

set(publicHeaderFiles lauxlib.h lua.h luaconf.h lualib.h)

list(TRANSFORM srcFiles PREPEND ${srcDir}/)
list(TRANSFORM publicHeaderFiles PREPEND ${srcDir}/)

add_library(lua
    ${srcFiles})

target_compile_definitions(lua
    PRIVATE
    $<$<PLATFORM_ID:Linux>:LUA_USE_LINUX LUA_COMPAT_5_2>)

target_compile_options(lua
    PRIVATE
    $<$<OR:$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:GNU>>:
    -Wextra -Wshadow -Wsign-compare -Wundef -Wwrite-strings -Wredundant-decls
    -Wdisabled-optimization -Waggregate-return -Wdouble-promotion -Wdeclaration-after-statement
    -Wmissing-prototypes -Wnested-externs -Wstrict-prototypes -Wc++-compat -Wold-style-definition>)

set(includeDir ${CMAKE_CURRENT_BINARY_DIR}/include)
file(COPY ${publicHeaderFiles} DESTINATION ${includeDir})

add_library(lua-header
    INTERFACE)

target_include_directories(lua-header
    INTERFACE
    ${includeDir})

target_link_libraries(lua
    INTERFACE
    lua-header)

add_library(lua::lib ALIAS lua)
add_library(lua::header ALIAS lua-header)

install(TARGETS lua
#	RUNTIME
	DESTINATION ${ORBITER_INSTALL_MODULE_DIR}
)