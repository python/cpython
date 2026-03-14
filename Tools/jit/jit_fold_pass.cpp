// Tools/jit/jit_fold_pass.cpp
//
// LLVM pass plugin that folds JIT-time constant computations into
// inline assembly markers for the CPython DynASM JIT backend.
//
// The CPython JIT uses extern symbols (_JIT_OPARG, _JIT_OPERAND0, etc.)
// whose addresses encode runtime-patchable values.  In the LLVM IR these
// appear as `ptrtoint (ptr @_JIT_OPARG to i16)` constant expressions.
// Clang compiles arithmetic on these into multi-instruction sequences,
// but since the values are known at JIT emit time, these computations
// can be deferred to the DynASM emitter as C expressions.
//
// This pass traces SSA use-chains from the patch-value symbols through
// type conversions and arithmetic, builds C expression strings, and
// replaces the computation trees with inline assembly markers that
// survive through LLVM codegen into the final .s file.
//
// The markers have the form:
//   nop # @@JIT_MOV_IMM %reg, <C-expression>@@
//
// The _asm_to_dasc.py converter recognizes these and emits:
//   emit_mov_imm(Dst, JREG_X, <C-expression>);
//
// Markers do NOT declare ~{flags} as clobbered.  The downstream
// _asm_to_dasc.py converter emits emit_mov_imm_preserve_flags() which
// avoids the flag-destroying xor-zeroing idiom (uses mov Rd, 0 instead).
// This gives LLVM full freedom to schedule flag-producing instructions
// (cmp, test) before the marker without inserting flag-save/restore code.
//
// Build:
//   clang++-21 -shared -fPIC -o jit_fold_pass.so jit_fold_pass.cpp \
//       $(llvm-config-21 --cxxflags --ldflags) -lLLVM-21
//
// Use:
//   opt-21 --load-pass-plugin=./jit_fold_pass.so -passes=jit-fold \
//       input.ll -S -o output.ll

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/Operator.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"

#include <optional>
#include <string>

using namespace llvm;

namespace {

// ── Helpers ────────────────────────────────────────────────────────────

// C unsigned integer type for a given bit width.
static std::string utype(unsigned bits) {
    switch (bits) {
    case 8:  return "uint8_t";
    case 16: return "uint16_t";
    case 32: return "uint32_t";
    default: return "uintptr_t";
    }
}

// C signed integer type for a given bit width.
static std::string stype(unsigned bits) {
    switch (bits) {
    case 8:  return "int8_t";
    case 16: return "int16_t";
    case 32: return "int32_t";
    default: return "int64_t";
    }
}

// Check if a Constant references any _JIT_* patch global.
static bool usesJITGlobal(
    Constant *C,
    const DenseMap<GlobalVariable *, std::string> &patchBases)
{
    if (auto *GV = dyn_cast<GlobalVariable>(C))
        return patchBases.count(GV);
    for (unsigned i = 0; i < C->getNumOperands(); ++i)
        if (auto *inner = dyn_cast<Constant>(C->getOperand(i)))
            if (usesJITGlobal(inner, patchBases))
                return true;
    return false;
}

// Map a binary opcode to the C operator string.
static const char *binOpStr(unsigned opcode) {
    switch (opcode) {
    case Instruction::Add:  return " + ";
    case Instruction::Sub:  return " - ";
    case Instruction::Mul:  return " * ";
    case Instruction::Shl:  return " << ";
    case Instruction::LShr: return " >> ";
    case Instruction::AShr: return " >> ";
    case Instruction::And:  return " & ";
    case Instruction::Or:   return " | ";
    case Instruction::Xor:  return " ^ ";
    default: return nullptr;
    }
}

// ── Expression builder ─────────────────────────────────────────────────
//
// Recursively builds a C expression string from an SSA value that is
// derived from _JIT_* patch symbols, integer constants, and known
// global addresses.  Returns nullopt when the value depends on
// runtime data.

static std::optional<std::string> buildExpr(
    Value *V,
    const DenseMap<GlobalVariable *, std::string> &patchBases,
    DenseMap<Value *, std::string> &cache,
    const DataLayout &DL)
{
    // Check cache first.
    {
        auto it = cache.find(V);
        if (it != cache.end())
            return it->second;
    }

    std::optional<std::string> result;

    // ── Base: integer constant ──
    if (auto *CI = dyn_cast<ConstantInt>(V)) {
        int64_t sv = CI->getSExtValue();
        uint64_t uv = CI->getZExtValue();
        // Use unsigned representation unless negative.
        if (sv >= 0)
            result = std::to_string(uv);
        else
            result = "(" + std::to_string(sv) + ")";
    }

    // ── GlobalVariable: patch symbols or known globals ──
    // Handles pointer operands (e.g., GEP base pointers).
    else if (auto *GV = dyn_cast<GlobalVariable>(V)) {
        auto it = patchBases.find(GV);
        if (it != patchBases.end())
            result = it->second;
        else if (GV->hasName() && !GV->getName().empty())
            result = "(uintptr_t)&" + GV->getName().str();
    }

    // ── Function pointer ──
    else if (auto *Fn = dyn_cast<Function>(V)) {
        if (Fn->hasName() && !Fn->getName().empty())
            result = "(uintptr_t)&" + Fn->getName().str();
    }

    // ── GEPOperator: handles both GEP instructions and GEP ConstantExprs ──
    // Decomposes into base + constant_offset + sum(variable * stride).
    // This enables folding patterns like &_PyRuntime + 14120 + oparg * 32
    // into a single emit_mov_imm C expression.
    else if (auto *GEP = dyn_cast<GEPOperator>(V)) {
        auto base = buildExpr(GEP->getPointerOperand(), patchBases, cache, DL);
        if (base) {
            // Try all-constant GEP first (fast path).
            APInt constOff(DL.getIndexTypeSizeInBits(
                GEP->getPointerOperandType()), 0);
            if (GEP->accumulateConstantOffset(DL, constOff)) {
                int64_t offset = constOff.getSExtValue();
                if (offset == 0)
                    result = *base;
                else if (offset > 0)
                    result = "(" + *base + " + " + std::to_string(offset) + ")";
                else
                    result = "(" + *base + " + (" +
                             std::to_string(offset) + "))";
            } else {
                // Decompose GEP with variable indices using
                // gep_type_iterator for correct stride computation.
                int64_t constOffset = 0;
                SmallVector<std::pair<std::string, uint64_t>, 2> varTerms;
                bool ok = true;

                for (auto GTI = gep_type_begin(GEP),
                          GTE = gep_type_end(GEP);
                     GTI != GTE; ++GTI) {
                    Value *idx = GTI.getOperand();
                    if (StructType *STy = GTI.getStructTypeOrNull()) {
                        // Struct index — always constant.
                        auto *CIdx = cast<ConstantInt>(idx);
                        constOffset += DL.getStructLayout(STy)
                            ->getElementOffset(CIdx->getZExtValue());
                    } else {
                        // Sequential type (array, pointer).
                        TypeSize elemSize =
                            GTI.getSequentialElementStride(DL);
                        if (elemSize.isScalable()) { ok = false; break; }
                        uint64_t size = elemSize.getFixedValue();

                        if (auto *CIdx = dyn_cast<ConstantInt>(idx)) {
                            constOffset +=
                                CIdx->getSExtValue() * (int64_t)size;
                        } else {
                            auto idxExpr = buildExpr(
                                idx, patchBases, cache, DL);
                            if (!idxExpr) { ok = false; break; }
                            varTerms.push_back({*idxExpr, size});
                        }
                    }
                }

                if (ok) {
                    std::string expr = *base;
                    if (constOffset != 0)
                        expr = "(" + expr + " + " +
                               std::to_string(constOffset) + ")";
                    for (auto &[idxExpr, stride] : varTerms) {
                        if (stride == 1)
                            expr = "(" + expr +
                                   " + (uintptr_t)(" + idxExpr + "))";
                        else
                            expr = "(" + expr +
                                   " + (uintptr_t)(" + idxExpr + ") * " +
                                   std::to_string(stride) + ")";
                    }
                    result = expr;
                }
            }
        }
    }

    // ── ConstantExpr: ptrtoint, binary ops, casts ──
    else if (auto *CE = dyn_cast<ConstantExpr>(V)) {
        if (CE->getOpcode() == Instruction::PtrToInt) {
            auto *op = CE->getOperand(0);
            if (auto *GV = dyn_cast<GlobalVariable>(op)) {
                auto it = patchBases.find(GV);
                if (it != patchBases.end()) {
                    result = it->second;
                }
                // Non-JIT globals: fold as (uintptr_t)&symbol_name.
                // This allows mixed expressions like
                //   &_PyRuntime + 14121 + (oparg + 5) * 32
                // to become a single emit_mov_imm C expression.
                // The global's address is available at JIT compile time
                // because the JIT emitter is part of the CPython binary.
                else if (GV->hasName() && !GV->getName().empty()) {
                    result = "(uintptr_t)&" + GV->getName().str();
                }
            }
            // Function pointers: fold as (uintptr_t)&func_name.
            else if (auto *Fn = dyn_cast<Function>(op)) {
                if (Fn->hasName() && !Fn->getName().empty())
                    result = "(uintptr_t)&" + Fn->getName().str();
            }
        }
        // Binary operations in ConstantExprs (e.g., add(ptrtoint, 2)).
        // Clang emits these for expressions like `oparg + 2` when the
        // result is used in a store or other non-instruction context.
        // LLVM 21 supports Add, Sub, Xor as ConstantExpr binary ops.
        else if (CE->getNumOperands() == 2) {
            const char *ceOp = binOpStr(CE->getOpcode());
            if (ceOp) {
                auto lhs = buildExpr(CE->getOperand(0), patchBases, cache, DL);
                auto rhs = buildExpr(CE->getOperand(1), patchBases, cache, DL);
                if (lhs && rhs) {
                    std::string left = *lhs;
                    if (CE->getOpcode() == Instruction::AShr) {
                        unsigned bits = CE->getType()->getIntegerBitWidth();
                        left = "(" + stype(bits) + ")(" + left + ")";
                    }
                    result = "(" + left + ceOp + *rhs + ")";
                }
            }
        }
        // ZExt/SExt/Trunc ConstantExprs
        else if (CE->getOpcode() == Instruction::ZExt) {
            auto inner = buildExpr(CE->getOperand(0), patchBases, cache, DL);
            if (inner) {
                unsigned srcBits =
                    CE->getOperand(0)->getType()->getIntegerBitWidth();
                if (srcBits < 32)
                    result = "(" + utype(srcBits) + ")(" + *inner + ")";
                else
                    result = *inner;
            }
        }
        else if (CE->getOpcode() == Instruction::SExt) {
            auto inner = buildExpr(CE->getOperand(0), patchBases, cache, DL);
            if (inner) {
                unsigned srcBits =
                    CE->getOperand(0)->getType()->getIntegerBitWidth();
                result = "(" + stype(srcBits) + ")(" + *inner + ")";
            }
        }
        else if (CE->getOpcode() == Instruction::Trunc) {
            auto inner = buildExpr(CE->getOperand(0), patchBases, cache, DL);
            if (inner) {
                unsigned dstBits = CE->getType()->getIntegerBitWidth();
                result = "(" + utype(dstBits) + ")(" + *inner + ")";
            }
        }
    }

    // ── PtrToInt instruction ──
    // Converts pointer to integer — the expression is the same as the
    // pointer operand (which already includes (uintptr_t)& for globals).
    else if (auto *PTI = dyn_cast<PtrToIntInst>(V)) {
        auto inner = buildExpr(PTI->getPointerOperand(), patchBases, cache, DL);
        if (inner)
            result = *inner;
    }

    // ── ZExt: zero-extend ──
    else if (auto *I = dyn_cast<ZExtInst>(V)) {
        auto inner = buildExpr(I->getOperand(0), patchBases, cache, DL);
        if (inner) {
            unsigned srcBits = I->getOperand(0)->getType()->getIntegerBitWidth();
            // Explicit truncation mask for narrow sources to match C semantics.
            if (srcBits < 32)
                result = "(" + utype(srcBits) + ")(" + *inner + ")";
            else
                result = *inner;
        }
    }

    // ── SExt: sign-extend ──
    else if (auto *I = dyn_cast<SExtInst>(V)) {
        auto inner = buildExpr(I->getOperand(0), patchBases, cache, DL);
        if (inner) {
            unsigned srcBits = I->getOperand(0)->getType()->getIntegerBitWidth();
            result = "(" + stype(srcBits) + ")(" + *inner + ")";
        }
    }

    // ── Trunc: truncate ──
    else if (auto *I = dyn_cast<TruncInst>(V)) {
        auto inner = buildExpr(I->getOperand(0), patchBases, cache, DL);
        if (inner) {
            unsigned dstBits = I->getType()->getIntegerBitWidth();
            result = "(" + utype(dstBits) + ")(" + *inner + ")";
        }
    }

    // ── Binary operators ──
    else if (auto *BO = dyn_cast<BinaryOperator>(V)) {
        auto lhs = buildExpr(BO->getOperand(0), patchBases, cache, DL);
        auto rhs = buildExpr(BO->getOperand(1), patchBases, cache, DL);
        if (lhs && rhs) {
            const char *op = binOpStr(BO->getOpcode());
            if (op) {
                std::string left = *lhs;
                if (BO->getOpcode() == Instruction::AShr) {
                    unsigned bits = BO->getType()->getIntegerBitWidth();
                    left = "(" + stype(bits) + ")(" + left + ")";
                }
                result = "(" + left + op + *rhs + ")";
            }
        }
    }

    // ── Select with patch-derived condition and both arms ──
    else if (auto *SI = dyn_cast<SelectInst>(V)) {
        auto cond = buildExpr(SI->getCondition(), patchBases, cache, DL);
        auto tv   = buildExpr(SI->getTrueValue(), patchBases, cache, DL);
        auto fv   = buildExpr(SI->getFalseValue(), patchBases, cache, DL);
        if (cond && tv && fv)
            result = "((" + *cond + ") ? (" + *tv + ") : (" + *fv + "))";
    }

    // NOTE: ICmpInst is NOT handled here.  Making the comparison result
    // a patch-derived value would cause LLVM to emit `test al, 1; jne`
    // which is worse than the original `test rax, rax; je`.  Instead,
    // stencils with provable oparg constraints use __builtin_assume()
    // (injected by _targets.py) so LLVM eliminates dead branches at
    // compile time.

    // Cache the result (only successful ones — nullopt means "not foldable").
    if (result)
        cache[V] = *result;

    return result;
}


// ── Main pass ──────────────────────────────────────────────────────────

class JITFoldPass : public PassInfoMixin<JITFoldPass> {
public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
        // 1. Discover patch-value globals and their base C expressions.
        DenseMap<GlobalVariable *, std::string> patchBases;

        auto tryAdd = [&](const char *name, const char *expr) {
            if (auto *GV = M.getGlobalVariable(name))
                patchBases[GV] = expr;
        };
        tryAdd("_JIT_OPARG",    "(uintptr_t)(instruction->oparg)");
        tryAdd("_JIT_OPERAND0", "(uintptr_t)(instruction->operand0)");
        tryAdd("_JIT_OPERAND1", "(uintptr_t)(instruction->operand1)");
        tryAdd("_JIT_TARGET",   "(uintptr_t)(instruction->jump_target)");

        // Don't bail out early when patchBases is empty.
        // Stencils that don't reference _JIT_* globals may still
        // benefit from OR-distribution and global-address folding
        // (e.g., _MATCH_MAPPING uses _Py_TrueStruct | 1).

        bool changed = false;
        for (Function &F : M) {
            if (F.isDeclaration())
                continue;
            changed |= processFunction(F, patchBases);
        }
        return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }

private:
    // ── Phase 0: Materialize binary ConstantExprs ──
    //
    // Clang can emit `store i16 add(ptrtoint(@_JIT_OPARG to i16), i16 2)`
    // where the add is a ConstantExpr, not an instruction.  buildExpr now
    // handles ConstantExpr binary ops, but boundary detection only looks
    // at Instructions.  Convert these to real instructions so Phase 2
    // can find and replace them.
    void materializeConstantExprs(
        Function &F,
        const DenseMap<GlobalVariable *, std::string> &patchBases)
    {
        SmallVector<std::pair<Instruction *, unsigned>> toFix;
        for (auto &BB : F) {
            for (auto &I : BB) {
                for (unsigned opIdx = 0; opIdx < I.getNumOperands(); ++opIdx) {
                    auto *CE = dyn_cast<ConstantExpr>(I.getOperand(opIdx));
                    if (!CE)
                        continue;
                    // Check if this is a binary op ConstantExpr
                    // (Add, Sub, Xor are the binary ops that survive as
                    // ConstantExpr in LLVM 21).
                    if (!binOpStr(CE->getOpcode()) ||
                        CE->getNumOperands() != 2)
                        continue;
                    if (!usesJITGlobal(CE, patchBases))
                        continue;
                    toFix.push_back({&I, opIdx});
                }
            }
        }
        for (auto &[inst, opIdx] : toFix) {
            auto *CE = dyn_cast<ConstantExpr>(inst->getOperand(opIdx));
            if (!CE)
                continue;
            // Create a real BinaryOperator (avoids constant folding back
            // into a ConstantExpr).
            auto *NewI = BinaryOperator::Create(
                static_cast<Instruction::BinaryOps>(CE->getOpcode()),
                CE->getOperand(0), CE->getOperand(1), "", inst->getIterator());
            inst->setOperand(opIdx, NewI);
        }
    }

    // GEP decomposition comment: The concern about GEP decomposition
    // moving SIB scaling into C arithmetic only applies to GEPs with
    // RUNTIME indices (e.g., [r13 + r8*8 + 80]).  For GEPs where all
    // variable indices are patch-derived, buildExpr uses (uintptr_t)
    // arithmetic which is always 64-bit and cannot overflow.

    // ── Phase 0b: Distribute OR into SELECT arms ──
    //
    // LLVM's instcombine canonicalizes:
    //   select(cond, (A|C), (B|C))  →  (select(cond, A, B)) | C
    //
    // For PyStackRef borrow tagging (| Py_TAG_REFCNT), this prevents
    // folding the tag into compile-time-constant addresses of
    // _Py_TrueStruct, _Py_FalseStruct, _Py_NoneStruct.
    // We reverse it so each arm becomes a foldable constant:
    //   or(select(cond, A, B), C)  →  select(cond, A|C, B|C)
    void distributeOrIntoSelect(Function &F) {
        SmallVector<Instruction *, 8> toRemoveOr;
        SmallVector<SelectInst *, 8> maybeDeadSel;

        for (auto &BB : F) {
            for (auto &I : BB) {
                auto *BO = dyn_cast<BinaryOperator>(&I);
                if (!BO || BO->getOpcode() != Instruction::Or)
                    continue;

                // Match: or(select, C) or or(C, select)
                ConstantInt *C = nullptr;
                SelectInst *Sel = nullptr;
                if ((Sel = dyn_cast<SelectInst>(BO->getOperand(0))) &&
                    (C = dyn_cast<ConstantInt>(BO->getOperand(1)))) {
                } else if ((C = dyn_cast<ConstantInt>(BO->getOperand(0))) &&
                           (Sel = dyn_cast<SelectInst>(BO->getOperand(1)))) {
                } else {
                    continue;
                }

                // Only distribute small tag constants (e.g., Py_TAG_REFCNT=1).
                if (C->getZExtValue() > 7)
                    continue;

                auto *TvOr = BinaryOperator::Create(
                    Instruction::Or, Sel->getTrueValue(), C,
                    "", BO->getIterator());
                auto *FvOr = BinaryOperator::Create(
                    Instruction::Or, Sel->getFalseValue(), C,
                    "", BO->getIterator());
                auto *NewSel = SelectInst::Create(
                    Sel->getCondition(), TvOr, FvOr,
                    "", BO->getIterator());

                BO->replaceAllUsesWith(NewSel);
                toRemoveOr.push_back(BO);
                maybeDeadSel.push_back(Sel);
            }
        }

        for (auto *I : toRemoveOr)
            I->eraseFromParent();
        for (auto *Sel : maybeDeadSel) {
            if (Sel->use_empty())
                Sel->eraseFromParent();
        }
    }

    bool processFunction(
        Function &F,
        const DenseMap<GlobalVariable *, std::string> &patchBases)
    {
        const DataLayout &DL = F.getParent()->getDataLayout();

        // Phase 0: Convert binary ConstantExprs that reference _JIT_*
        // globals into real instructions so the rest of the pass can
        // find and replace them.
        materializeConstantExprs(F, patchBases);

        // Phase 0b: Distribute OR into SELECT arms so borrow tags
        // (| Py_TAG_REFCNT) get folded into constant addresses.
        distributeOrIntoSelect(F);

        // Phase 1: Build expressions for all patch-derived values.
        DenseMap<Value *, std::string> cache;
        for (auto &BB : F)
            for (auto &I : BB)
                buildExpr(&I, patchBases, cache, DL);

        // Phase 2: Find "boundary" instructions — patch-derived values
        // that have at least one user which is NOT patch-derived.
        // These are the points where we inject inline-asm markers.
        //
        // Create boundaries for expressions that reference:
        // - _JIT_* patch values (contain "instruction->"), OR
        // - Global addresses (contain "(uintptr_t)&") — these arise from
        //   OR-distribution of borrow tags into select arms, e.g.
        //   ((uintptr_t)&_Py_TrueStruct | 1).  The combined address+tag
        //   cannot be expressed as a single relocation, so emit_mov_imm
        //   is more efficient than LLVM's mov+or sequence.
        //
        // Pure numeric constants not referencing either are skipped —
        // LLVM handles them better via immediates.
        SmallVector<std::pair<Instruction *, std::string>> boundaries;

        for (auto &BB : F) {
            for (auto &I : BB) {
                auto it = cache.find(&I);
                if (it == cache.end())
                    continue;  // not patch-derived

                // Skip pure constant expressions (no JIT or global ref).
                if (it->second.find("instruction->") == std::string::npos &&
                    it->second.find("(uintptr_t)&") == std::string::npos)
                    continue;

                bool hasBoundaryUse = false;
                for (User *U : I.users()) {
                    if (cache.find(U) == cache.end()) {
                        hasBoundaryUse = true;
                        break;
                    }
                }
                if (hasBoundaryUse)
                    boundaries.push_back({&I, it->second});
            }
        }

        if (boundaries.empty())
            return false;

        // Phase 3: Replace each boundary value with an inline-asm marker.
        for (auto &[inst, expr] : boundaries)
            injectMarker(inst, expr);

        // Phase 4: Remove dead patch-derived instructions.
        // Work backwards to handle use-before-def chains.
        bool progress = true;
        while (progress) {
            progress = false;
            for (auto &BB : F) {
                for (auto it = BB.begin(); it != BB.end(); ) {
                    Instruction &I = *it++;
                    if (I.use_empty() && cache.count(&I) &&
                        !I.isTerminator() && !I.mayHaveSideEffects()) {
                        I.eraseFromParent();
                        progress = true;
                    }
                }
            }
        }

        return true;
    }

    void injectMarker(Instruction *I, const std::string &expr) {
        IRBuilder<> Builder(I);
        Type *origTy = I->getType();
        LLVMContext &Ctx = I->getContext();
        Type *i64Ty = Type::getInt64Ty(Ctx);

        // The inline asm emits a NOP (valid x86) with a comment marker.
        // The NOP ensures the assembler doesn't complain.  The marker
        // is parsed by _asm_to_dasc.py which converts it to an
        // emit_mov_imm_preserve_flags() call.
        //
        // Always use i64 output type so LLVM allocates a 64-bit register
        // (rax, rcx, ...) rather than a 32/16-bit sub-register (eax, ax).
        // The emit_mov_imm_preserve_flags helper handles narrower values
        // correctly.
        //
        // We do NOT clobber ~{flags} here.  The downstream
        // emit_mov_imm_preserve_flags() avoids xor-zeroing (which would
        // clobber flags), using mov Rd, 0 instead.  This gives LLVM full
        // scheduling freedom around flag-producing instructions.
        std::string asmText = "nop # @@JIT_MOV_IMM $0, " + expr + "@@";

        FunctionType *FTy = FunctionType::get(i64Ty, /*isVarArg=*/false);
        InlineAsm *IA = InlineAsm::get(
            FTy, asmText,
            "=r,~{dirflag},~{fpsr}",
            /*hasSideEffects=*/true);

        CallInst *marker = Builder.CreateCall(FTy, IA);

        // If original type was narrower than i64, truncate back.
        // LLVM codegen will use the appropriate sub-register in the
        // consuming instruction without emitting extra instructions.
        Value *result = marker;
        if (origTy != i64Ty && origTy->isIntegerTy()) {
            result = Builder.CreateTrunc(marker, origTy);
        } else if (origTy->isPointerTy()) {
            result = Builder.CreateIntToPtr(marker, origTy);
        }

        I->replaceAllUsesWith(result);
        I->eraseFromParent();
    }
};

} // anonymous namespace

// ── Plugin registration ────────────────────────────────────────────────

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "JITFoldPass",
        LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) -> bool {
                    if (Name == "jit-fold") {
                        MPM.addPass(JITFoldPass());
                        return true;
                    }
                    return false;
                });
        }};
}
