#[[
   Copyright 2020 The SilkRpc Authors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
]]

if(MSVC)
  add_link_options(/STACK:10000000)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  add_link_options(-Wl,-stack_size -Wl,0x1000000)
endif()

find_package(gRPC CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)

if (NOT MSVC)
    add_compile_options(-std=c++2a)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fcoroutines)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-stdlib=libc++)
endif()

add_executable(get_latest_block get_latest_block.cpp)
target_include_directories(get_latest_block PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(get_latest_block absl::flags_parse gRPC::grpc++_unsecure protobuf::libprotobuf silkrpc)
