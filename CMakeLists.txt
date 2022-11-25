cmake_minimum_required(VERSION 3.22)
project(MyUtil)

set(CMAKE_CXX_STANDARD 20)

include_directories(my-logger/include)

add_subdirectory(my-logger)
add_subdirectory(json-parser)
add_subdirectory(elf-reader)
add_subdirectory(xml-parser)
add_subdirectory(threadsafe_queue)
add_subdirectory(thread_pool)
add_subdirectory(any-lite)
add_subdirectory(aop)
add_executable(
        MyUtil main.cpp
        ini-parser/IniFile.cpp ini-parser/IniFile.h
        http-parser/Parser.cpp http-parser/Parser.h
        http-parser/Response.cpp http-parser/Response.h
        http-parser/Request.cpp http-parser/Request.h
        http-parser/Url.cpp http-parser/Url.h
        http-parser/PostForm.cpp http-parser/PostForm.h http-parser/common.h)