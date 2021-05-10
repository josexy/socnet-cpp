cmake_minimum_required(VERSION 3.10)

project(socnet)
set(CMAKE_CXX_STANDARD 20)

aux_source_directory(soc/libjson/src LIBJSON_SRC_FILES)
aux_source_directory(soc/net/src NET_SRC_FILES)
aux_source_directory(soc/http/src HTTP_SRC_FILES)
aux_source_directory(soc/utility/src UTILITY_SRC_FILES)

add_executable(socnet
        test/simple-server-upload.cc
        # test/simple-server-full.cc
        # test/simple-server.cc

        ${LIBJSON_SRC_FILES}
        ${NET_SRC_FILES}
        ${HTTP_SRC_FILES}
        ${UTILITY_SRC_FILES}
        )

target_link_libraries(socnet 
        pthread
        z
        ssl
        crypto 
        # mariadb
)