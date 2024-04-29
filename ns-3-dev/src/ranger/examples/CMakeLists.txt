set(base_examples
    ranger-test
    ranger-routing-simple-test
    ranger-mobility-test
    ranger-test-mac
    ranger-comprehensive-test
    ranger-distance-error-test
)

foreach(
  example
  ${base_examples}
)
  build_lib_example(
    NAME ${example}
    SOURCE_FILES ${example}.cc
    LIBRARIES_TO_LINK 
      ${libranger}
      ${libnetanim}
  )
endforeach()

