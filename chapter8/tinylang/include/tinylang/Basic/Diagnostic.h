#ifndef TINYLANG_BASIC_DIAGNOSTIC_H
#define TINYLANG_BASIC_DIAGNOSTIC_H

#include "tinylang/Basic/LLVM.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include <utility>

namespace tinylang
{
    namespace diag
    {
        /// @brief All the diagnostics level and the error message
        enum
        {
#define DIAG(ID, Level, Msg) ID,
#include "tinylang/Basic/Diagnostic.def"
        };
    } //! namespace diag

    class DiagnosticsEngine
    {
        /// @brief Obtain the error text
        /// @param DiagID id of the error
        /// @return error text
        static const char *getDiagnosticText(unsigned DiagID);

        /// @brief Obtain the Kind of Diagnostic given the ID
        /// @param DiagID id of the error
        /// @return kind of error
        static SourceMgr::DiagKind getDiagnosticKind(unsigned DiagID);

        /// @brief Source manager
        SourceMgr &SrcMgr;

        /// @brief Number of errors
        unsigned NumErrors;
    public:
        DiagnosticsEngine(SourceMgr &SrcMgr) : SrcMgr(SrcMgr), NumErrors(0) {}

        /// @brief Get the number of errors
        /// @return number of errors
        unsigned numErrors() const
        {
            return NumErrors;
        }

        /// @brief Report an error on compilation time
        /// @tparam ...Args type of the arguments to show
        /// @param Loc location of the error
        /// @param DiagID ID of the error
        /// @param ...Arguments arguments to print with the error
        template <typename... Args>
        void report(SMLoc Loc, unsigned DiagID, Args &&... Arguments)
        {
            std::string Msg = llvm::formatv(getDiagnosticText(DiagID), std::forward<Args>(Arguments)...).str();
            SourceMgr::DiagKind Kind = getDiagnosticKind(DiagID);
            SrcMgr.PrintMessage(Loc, Kind, Msg);
            NumErrors += (Kind == SourceMgr::DK_Error);
        }
    };

} //! namespace tinylang

#endif