#include "tinylang/CodeGen/CGTBAA.h"
#include "tinylang/CodeGen/CGModule.h"
#include "llvm/IR/DataLayout.h"

using namespace tinylang;

CGTBAA::CGTBAA(CGModule &CGM) : 
    CGM(CGM), MDHelper(llvm::MDBuilder(CGM.getLLVMCtx())), Root(nullptr)
{
}


llvm::MDNode * CGTBAA::getRoot()
{
    if (!Root)
        Root = MDHelper.createTBAARoot("Simple tinylang TBAA");
    return Root;
}

llvm::MDNode* CGTBAA::createScalarTypeNode(TypeDeclaration *Ty, StringRef Name, llvm::MDNode *Parent)
{
    /// create a scalar type node from given type
    llvm::MDNode * N = MDHelper.createTBAAScalarTypeNode(Name, Parent);
    return MetadataCache[Ty] = N;
}

llvm::MDNode * CGTBAA::createStructTypeNode(
            TypeDeclaration * Ty,
            StringRef Name,
            llvm::ArrayRef<std::pair<llvm::MDNode *, uint64_t>> Fields
        )
{
    /// for a structure, the type node must contain the different
    /// fields in the structure, we have to specify the type of a node
    /// and the offset of the node
    llvm::MDNode * N = MDHelper.createTBAAStructTypeNode(Name, Fields);
    return MetadataCache[Ty] = N;
}

llvm::MDNode *CGTBAA::getTypeInfo(TypeDeclaration * Ty)
{
    // check first if the type is in the cache
    if (llvm::MDNode * N = MetadataCache[Ty])
        return N;
    
    if (auto * Pervasive = llvm::dyn_cast<PervasiveTypeDeclaration>(Ty)) // declaration of a type
    {
        StringRef Name = Pervasive->getName();
        /// create the scalar type giving Pervasive as key for cache map
        return createScalarTypeNode(Pervasive, Name, getRoot());
    }

    if (auto * Pointer = llvm::dyn_cast<PointerTypeDeclaration>(Ty))
    {
        StringRef Name = "any pointer"; // just generate a name for pointer, we do not analyze a lot
        return createScalarTypeNode(Pointer, Name, getRoot()); // pointers have same root as types
    }

    if (auto * Record = llvm::dyn_cast<RecordTypeDeclaration>(Ty))
    {
        // records need to be analyzed for obtaining all their fields
        llvm::SmallVector<std::pair<llvm::MDNode *, uint64_t>, 4> Fields;

        auto * Rec = llvm::cast<llvm::StructType>(CGM.convertType(Record));
        const llvm::StructLayout * Layout = CGM.getModule()->getDataLayout().getStructLayout(Rec);

        unsigned Idx = 0;
        for (const auto &F : Record->getFields())
        {
            // obtain offset of the field
            uint64_t Offset = Layout->getElementOffset(Idx); 
            // put the type of the field in the vector together with the offset
            Fields.emplace_back(getTypeInfo(F.getType()), Offset); 
            ++Idx;
        }

        StringRef Name = CGM.mangleName(Record);
        /// create the type of a struct type node
        return createStructTypeNode(Record, Name, Fields);
    }

    // in other case it is not a type nor a pointer nor a struct
    return nullptr;
}

llvm::MDNode *CGTBAA::getAccessTagInfo(TypeDeclaration *Ty)
{
    if (auto * Pointer = llvm::dyn_cast<PointerTypeDeclaration>(Ty))
    {
        return getTypeInfo(Pointer->getType());
    }
    
    return nullptr;
}