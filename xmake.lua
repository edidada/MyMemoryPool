set_project("MyMemoryPool")
set_version("0.1.0")
set_languages("c11")

target("mempool")
    set_kind("static")
    add_files("mempool.c")
    add_headerfiles("mempool.h")

target("test_mempool")
    set_kind("binary")
    add_deps("mempool")
    add_files("test_mempool.c")
