#===------------------------------------------------------------------------===#
#
#                     The Inception compiler
#
#===------------------------------------------------------------------------===#
inception_compiler_add_component(Utils
  ABIAdapter.cpp
  Builder.cpp
  ErrorHandling.cpp
  IContext.cpp
  utils.cpp
)

# TODO: Work out what the correct LLVM components are for
# inceptionCore.
set(LLVM_COMPONENTS
	core
  support
	analysis
	arm
	armasmparser
	armasmprinter
	armcodegen
	armdesc
	armdisassembler
	arminfo
	asmparser
	asmprinter
	bitreader
	bitwriter
	codegen
	mc
	mcdisassembler
	mcparser
	target
	all-targets
)

if ("${LLVM_PACKAGE_VERSION}" VERSION_EQUAL "3.6" OR
    "${LLVM_PACKAGE_VERSION}" VERSION_GREATER "3.6")
  list(APPEND LLVM_COMPONENTS)
else()
  list(APPEND LLVM_COMPONENTS)
endif()


inception_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(Utils PUBLIC ${LLVM_LIBS})
target_link_libraries(Utils PRIVATE)
