add_executable(inception-cl
  DummyObjectFile.cpp
  FunctionsHelperWriter.cpp
  inception-cl.cpp
  IRMerger.cpp
)

set(INCEPTION_LIBS
  CodeInv Target Transforms Utils
)

# set(LLVM_COMPONENTS
#   mc
# )

target_link_libraries(inception-cl CodeInv Target Transforms Utils ${INCEPTION_LIBS})
link_directories(${LLVM_LIBRARY_DIRS})

install(TARGETS inception-cl RUNTIME DESTINATION bin)

# The INCEPTION binary depends on the runtimes
#add_dependencies(inception-cl)
