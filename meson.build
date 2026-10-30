project('MyMemoryPool', 'c',
        version : '0.1.0',
        default_options : ['c_std=c11', 'warning_level=2'])

mempool_lib = static_library('mempool',
                             ['mempool.c', 'mempool.h'],
                             install : false)

test_exe = executable('test_mempool',
                      'test_mempool.c',
                      link_with : mempool_lib)

test('mempool-tests', test_exe)
