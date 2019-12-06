cmake_minimum_required(VERSION 3.14)
project(hpsocket)

set(CMAKE_CXX_STANDARD 17)

include_directories(/opt/local/include)
link_directories(/opt/local/lib)

link_libraries(ssl iconv z crypto)
add_definitions(-D_NEED_HTTP -D_SSL_SUPPORT -D_NEED_SSL)

set(HPSOCKET_SOURCE_BASE_PATH src)
set(HPSOCKET_SOURCE_COMMON_PATH src/common)
set(HPSOCKET_SOURCE_COMMON_CRYPTO_PATH src/common/crypto)
set(HPSOCKET_SOURCE_COMMON_HTTP_PATH src/common/http)
set(HPSOCKET_SOURCE_COMMON_KCP_PATH src/common/kcp)

aux_source_directory(src HPSOCKET_SOURCE_BASE_PATH)
aux_source_directory(src/common HPSOCKET_SOURCE_COMMON_PATH)
aux_source_directory(src/common/crypto HPSOCKET_SOURCE_COMMON_CRYPTO_PATH)
aux_source_directory(src/common/http HPSOCKET_SOURCE_COMMON_HTTP_PATH)
aux_source_directory(src/common/kcp HPSOCKET_SOURCE_COMMON_KCP_PATH)

set(HTTP_SERVER_4C test/server/test6.cpp)

set(TEST_HELPER_CPP test/helper.cpp)
set(TEST_HELPER_H test/helper.h)

set(ARQ_SERVER test/server/test5.cpp)
set(ARQ_CLIENT test/client/test6.cpp)

set(TCP_SERVER test/server/test1.cpp)
set(TCP_CLIENT test/client/test1.cpp)
set(TCP_AGENT test/client/test3.cpp)

set(TCP_SERVER_PACK test/server/test2.cpp)
set(TCP_CLIENT_PACK test/client/test2.cpp)
set(TCP_AGENT_PACK test/client/test4.cpp)

set(TCP_SERVER_PULL test/server/test7.cpp)
set(TCP_CLIENT_PULL test/client/test9.cpp)
set(TCP_AGENT_PULL test/client/testA.cpp)

set(UDP_CLIENT test/client/test5.cpp)
set(UDP_SEVER test/server/test3.cpp)
set(UDP_CAST test/client/test7.cpp)

set(HTTP_SERVER test/server/test4.cpp)
set(HTTP_CLIENT test/client/test8.cpp)

set(UDP_NODE test/client/testB.cpp)

add_executable(test_tcp_agent_pull
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${TCP_AGENT_PULL}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_executable(test_tcp_client_pull
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${TCP_CLIENT_PULL}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_executable(test_tcp_server_pull
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${TCP_SERVER_PULL}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_executable(test_http_client
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${HTTP_CLIENT}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_executable(test_http_server
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${HTTP_SERVER}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_executable(test_udp_cast
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${UDP_CAST}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_executable(test_udp_client
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${UDP_CLIENT}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})


add_executable(test_udp_server
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${UDP_SEVER}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})


add_executable(test_tcp_server_pack
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${TCP_SERVER_PACK}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_executable(test_tcp_client_pack
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${TCP_CLIENT_PACK}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_executable(test_tcp_agent_pack
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${TCP_AGENT_PACK}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_executable(test_tcp_server
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${TCP_SERVER}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_executable(test_tcp_client
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${TCP_CLIENT}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_executable(test_tcp_agent
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${TCP_AGENT}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_executable(test_arq_server
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${ARQ_SERVER}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_executable(test_arq_client
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${ARQ_CLIENT}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_library(hpsocket STATIC
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH})

add_executable(test_server_4c
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${HTTP_SERVER_4C}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH}
        )

add_executable(test_udp_node
        ${TEST_HELPER_CPP}
        ${TEST_HELPER_H}
        ${UDP_NODE}
        ${HPSOCKET_SOURCE_BASE_PATH}
        ${HPSOCKET_SOURCE_COMMON_PATH}
        ${HPSOCKET_SOURCE_COMMON_CRYPTO_PATH}
        ${HPSOCKET_SOURCE_COMMON_HTTP_PATH}
        ${HPSOCKET_SOURCE_COMMON_KCP_PATH}
        )