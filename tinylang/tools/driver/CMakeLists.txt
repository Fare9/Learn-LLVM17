# set to the list of LLVM components 
# we need to link our tool against.
# here the Support components
set(LLVM_LINK_COMPONENTS ${LLVM_TARGETS_TO_BUILD}
  AggressiveInstCombine Analysis AsmParser
  BitWriter CodeGen Core Coroutines IPO IRReader
  InstCombine Instrumentation MC ObjCARCOpts Remarks
  ScalarOpts Support Target TransformUtils Vectorize
  Passes)

# the name of the tool we want to compile
# and the source files of it, we have to
# announce that we support plugins from the
# LLVM passes
add_tinylang_tool(tinylang
    Driver.cpp

    DEPENDS
    SUPPORT_PLUGINS
)

export_executable_symbols_for_plugins(tinylang)

# to link against our own library
# we specify it with target_link_libraries
target_link_libraries(tinylang
  PRIVATE tinylangBasic tinylangCodeGen
  tinylangLexer tinylangParser tinylangSema)