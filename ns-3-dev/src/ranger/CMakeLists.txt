build_lib(
  LIBNAME ranger
  SOURCE_FILES
    model/ranger-net-device.cc
    model/ranger-routing-protocol.cc
    model/ranger-nwk.cc
    model/ranger-nwk-nodeinfo.cc
    model/ranger-nwk-header.cc
  HEADER_FILES
    model/ranger-repositories.h
    model/ranger-net-device.h
    model/ranger-routing-protocol.h
    model/ranger-nwk.h
    model/ranger-nwk-nodeinfo.h
    model/ranger-nwk-header.h
  LIBRARIES_TO_LINK ${liblr-wpan}
  TEST_SOURCES
)
