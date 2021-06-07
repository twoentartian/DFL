# https://github.com/bro/cmake/blob/master/FindJeMalloc.cmake
# - Try to find google test headers and libraries.
#
# Usage of this module as follows:
#
#     find_package(Rocksdb)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  ROCKSDB_ROOT_DIR Set this variable to the root installation of
#                    rocksdb if the module has problems finding
#                    the proper installation path.
#
# Variables defined by this module:
#
#  ROCKSDB_FOUND             System has rocksdb libs/headers
#  ROCKSDB_LIBRARIES         The rocksdb library/libraries
#  ROCKSDB_INCLUDE_DIR       The location of rocksdb headers

find_path(ROCKSDB_ROOT_DIR
        NAMES include/rocksdb/db.h
        )

find_library(ROCKSDB_LIBRARIES
        NAMES rocksdb
        HINTS ${ROCKSDB_ROOT_DIR}
        )

find_path(ROCKSDB_INCLUDE_DIR
        NAMES rocksdb/db.h
        HINTS ${ROCKSDB_ROOT_DIR}/include
        )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RocksDB DEFAULT_MSG
        ROCKSDB_LIBRARIES
        ROCKSDB_INCLUDE_DIR
        )

mark_as_advanced(
        ROCKSDB_ROOT_DIR
        ROCKSDB_LIBRARIES
        ROCKSDB_INCLUDE_DIR
)