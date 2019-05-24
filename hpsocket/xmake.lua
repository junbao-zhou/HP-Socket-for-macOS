
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
--
-- FAQ
--
-- You can enter the project directory firstly before building project.
--   
--   $ cd projectdir
-- 
-- 1. How to build project?
--   
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install 
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code 
--    -- add macro defination
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "cxx11")
--
--    -- set optimization: none, faster, fastest, smallest 
--    set_optimize("fastest")
--    
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox", "z", "pthread")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--
-- 7. If you want to known more usage about xmake, please see http://xmake.io/#/home
--
    
