add_library (TEST_dll_dll_target SHARED "dll_test.cpp")
target_link_libraries (TEST_dll_dll_target ${Boost_LIBRARIES} )

add_executable (TEST_dll_dll_importer "dll_import_main.cpp")
target_link_libraries (TEST_dll_dll_importer ${Boost_LIBRARIES} libdl.so)