set(base_examples
    ranger-test
    ranger-routing-simple-test
    ranger-mobility-test
    ranger-test-mac
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

