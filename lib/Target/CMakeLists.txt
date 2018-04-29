#===------------------------------------------------------------------------===#
#
#                     The Inception compiler
#
#===------------------------------------------------------------------------===#
inception_compiler_add_component(Target
	AddLifter.cpp
	BranchLifter.cpp
	DummyLifter.cpp
	ITLifter.cpp
	MiscLifter.cpp
	StoreLifter.cpp
	ARMLifter.cpp
	CompareLifter.cpp
	ExtendLifter.cpp
	LoadLifter.cpp
	MoveDataLifter.cpp
	SubtractLifter.cpp
	ARMLifterManager.cpp
	CoprocLifter.cpp
	FlagsLifter.cpp
	LogicalLifter.cpp
	MulLifter.cpp
	SVCallLifter.cpp
	BarrierLifter.cpp
	DivLifter.cpp
	HintLifter.cpp
	ShiftLifter.cpp
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
  list(APPEND LLVM_COMPONENTS mcjit executionengine native)
else()
  list(APPEND LLVM_COMPONENTS jit engine)
endif()


inception_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(Target PUBLIC ${LLVM_LIBS})
target_link_libraries(Target PRIVATE
	Utils
)
