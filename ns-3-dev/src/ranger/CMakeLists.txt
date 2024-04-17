build_lib(
  LIBNAME ranger
  SOURCE_FILES
    model/ranger-net-device.cc
    model/ranger-routing-protocol.cc
    model/ranger-routing-nblist.cc
    model/ranger-nwk.cc
    model/ranger-nwk-header.cc
    model/ranger-audio-management.cc
  HEADER_FILES
    model/ranger-net-device.h
    model/ranger-routing-protocol.h
    model/ranger-routing-nblist.cc
    model/ranger-nwk.h
    model/ranger-nwk-header.h
    model/ranger-audio-management.h
  LIBRARIES_TO_LINK ${liblr-wpan}
  TEST_SOURCES
)
