
-- add modes: debug and release 
add_rules("mode.debug", "mode.release")

-- openssl
add_linkdirs("/usr/local/opt/openssl/lib")
add_includedirs("/usr/local/opt/openssl/include")

-- glog
add_linkdirs("/usr/local/opt/glog/lib", "/usr/local/opt/gflags/lib")
add_includedirs("/usr/local/opt/glog/include", "/usr/local/opt/gflags/include")

add_syslinks("ssl", "crypto", "glog", "gflags", "iconv")

set_languages("cxx14")
add_defines("DEBUG")
add_defines("_HTTP_SUPPORT", "_SSL_SUPPORT", "_ICONV_SUPPORT", "_UDP_SUPPORT")

-- add files
add_files("src/**.cpp|*4C*.cpp", "src/**.c") 
add_includedirs("src/**")

-- add target
target("hpsocket_static_lib")
    -- set kind
    set_kind("static")


-- add target
target("hpsocket_demo")

    -- set kind
    set_kind("binary")

    -- add files
    add_files("main.cpp")


-- unit test
-- add target
target("tcp_client")

    -- set kind
    set_kind("binary")

    -- add files
    add_files("test/client/test1.cpp")
-- unit test
-- add target
target("tcp_server")

    -- set kind
    set_kind("binary")

    -- add files
    add_files("test/server/test1.cpp")


-- unit test
-- add target
target("tcp_pack_client")

    -- set kind
    set_kind("binary")

    -- add files
    add_files("test/client/test2.cpp")
-- unit test
-- add target
target("tcp_pack_server")

    -- set kind
    set_kind("binary")

    -- add files
    add_files("test/server/test2.cpp")

-- unit test
-- add target
target("tcp_agent_client")

    -- set kind
    set_kind("binary")

    -- add files
    add_files("test/client/test3.cpp")

-- unit test
-- add target
target("tcp_pack_agent_client")

    -- set kind
    set_kind("binary")

    -- add files
    add_files("test/client/test4.cpp")


-- unit test
-- add target
target("udp_server")

    -- set kind
    set_kind("binary")

    -- add files
    add_files("test/server/test3.cpp")

-- unit test
-- add target
target("udp_client")

    -- set kind
    set_kind("binary")

    -- add files
    add_files("test/client/test5.cpp")

-- unit test
-- add target
target("http_server")

    -- set kind
    set_kind("binary")
    add_defines("_NEED_HTTP", "_NEED_SSL")

    -- add files
    add_files("test/server/test4.cpp")
    add_files("test/helper.cpp")
    add_includedirs("test/helper.h")
--