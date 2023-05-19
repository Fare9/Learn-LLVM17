set(LLVM_LINK_COMPONENTS support)

add_tinylang_library(tinylangCodeGen
    CGModule.cpp
    CGProcedure.cpp
    CodeGenerator.cpp 
    CGTBAA.cpp
    CGDebugInfo.cpp

    LINK_LIBS 
    tinylangSema
)