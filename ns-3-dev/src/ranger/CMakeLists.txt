build_lib(
  LIBNAME ranger
  SOURCE_FILES
    model/ranger-net-device.cc
    model/ranger-routing-protocol.cc
    model/ranger-routing-nblist.cc
    model/ranger-nwk.cc
    model/ranger-nwk-header.cc
    model/ranger-audio-management.cc
    model/ranger-mac.cc
    model/ranger-mac-header.cc
    helper/ranger-recorder.cc
  HEADER_FILES
    model/ranger-net-device.h
    model/ranger-routing-protocol.h
    model/ranger-routing-nblist.h
    model/ranger-nwk.h
    model/ranger-nwk-header.h
    model/ranger-audio-management.h
    model/ranger-mac.h
    model/ranger-mac-header.h
    model/ranger-mac-constants.h
    helper/ranger-recorder.h
  LIBRARIES_TO_LINK ${libspectrum}
                    ${liblr-wpan}
  TEST_SOURCES
)
