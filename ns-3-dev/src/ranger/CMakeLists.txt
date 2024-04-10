build_lib(
  LIBNAME ranger
  SOURCE_FILES
    model/ranger-net-device.cc
  HEADER_FILES
    model/ranger-net-device.h
  LIBRARIES_TO_LINK ${liblr-wpan}
  TEST_SOURCES
)
