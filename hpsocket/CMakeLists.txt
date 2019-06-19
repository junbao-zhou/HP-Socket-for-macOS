cmake_minimum_required(VERSION 3.14)
project(hpsocket)

set(CMAKE_CXX_STANDARD 17)

include_directories(/opt/local/include)

link_directories(/opt/local/lib)
link_libraries(ssl iconv z crypto)

add_definitions(-D_UDP_SUPPORT -D_NEED_HTTP -D_NEED_SSL -D_HTTP_SUPPORT -D_SSL_SUPPORT -D_ZLIB_SUPPORT)

set(HPSOCKET_SOURCE_PATH src)
set(HPSOCKET_SOURCE_COMMON_PATH src/common)
set(HPSOCKET_SOURCE_COMMON_CRYPTO_PATH src/common/crypto)
set(HPSOCKET_SOURCE_COMMON_HTTP_PATH src/common/http)
set(HPSOCKET_SOURCE_COMMON_KCP_PATH src/common/kcp)

aux_source_directory(src HPSOCKET_SOURCE_PATH)
aux_source_directory(src/common HPSOCKET_SOURCE_COMMON_PATH)
aux_source_directory(src/common/crypto HPSOCKET_SOURCE_COMMON_CRYPTO_PATH)
aux_source_directory(src/common/http HPSOCKET_SOURCE_COMMON_HTTP_PATH)
aux_source_directory(src/common/kcp HPSOCKET_SOURCE_COMMON_KCP_PATH)

set(TEST_HELPER_CPP test/helper.cpp)
set(TEST_HELPER_H test/helper.h)
set(ARQ_SERVER test/server/test4.cpp)
# set(ARQ_CLIENT test/client/test6.cpp)

add_executable(test_server
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${ARQ_SERVER}
        ${HPSOCKET_SOURCE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_executable(test_client
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${ARQ_CLIENT}
        ${HPSOCKET_SOURCE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})
