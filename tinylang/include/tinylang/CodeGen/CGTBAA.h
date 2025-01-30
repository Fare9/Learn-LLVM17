#ifndef TINYLANG_CODEGEN_CGTBAA
#define TINYLANG_CODEGEN_CGTBAA

#include "tinylang/AST/AST.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Metadata.h"

namespace tinylang
{

    class CGModule;
    
    class CGTBAA
    {
        CGModule &CGM;
        
        /// @brief Root of the type hierarchy
        llvm::MDNode * Root;

        /// @brief MDHelper - Helper for creating metadata.
        llvm::MDBuilder MDHelper;

        /// @brief The root node of the TBAA hierarchy
        llvm::DenseMap<TypeDeclaration *, llvm::MDNode *>
            MetadataCache;
        
        /// @brief We need to create a new type node for scalar types, we will
        /// use this function
        /// @param Ty declaration of the type
        /// @param Name name of the scalar type
        /// @param Parent node parent of the scalar type in the tree
        /// @return new node in scalar type tree
        llvm::MDNode * createScalarTypeNode(
            TypeDeclaration * Ty,
            StringRef Name,
            llvm::MDNode * Parent
        );

        /// @brief Create the type node for a record, we need to enumerate all the
        /// for that we will need to enumerate all the fields.
        /// @param Ty 
        /// @param Name 
        /// @param Fields 
        /// @return 
        llvm::MDNode * createStructTypeNode(
            TypeDeclaration * Ty,
            StringRef Name,
            llvm::ArrayRef<std::pair<llvm::MDNode *, uint64_t>> Fields
        );

    public:
        /// @brief Constructor of the class used to create
        /// metadata in load and store for pointer alias analysis
        /// @param CGM 
        CGTBAA(CGModule &CGM);

        /// @brief Initialize the root node of the hierarchy and return it
        /// @return root of type hierarchy
        llvm::MDNode *getRoot();
        /// @brief Get a node from the tree hierarchy, or insert it in case
        /// it doesn't exist, and return the node
        /// @param Ty type declaration
        /// @return node in the hierarchy tree
        llvm::MDNode *getTypeInfo(TypeDeclaration * Ty);

        /// @brief Get the node from hierarchy declaration tree for pointer declarations.
        /// @param Ty declaration of type
        /// @return node in the declaration tree
        llvm::MDNode *getAccessTagInfo(TypeDeclaration *Ty);

    };
} // namespace tinylang

#endif