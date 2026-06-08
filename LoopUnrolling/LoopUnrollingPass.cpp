#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"

#include <vector>
#include <unordered_map>

using namespace llvm;

namespace {

static const int UnrollFactor = 3;

struct LoopUnrollingPass : public PassInfoMixin<LoopUnrollingPass> {
    std::vector<BasicBlock *> LoopBasicBlocks;
    std::unordered_map<Value *, Value *> VariablesMap;
    Value *LoopCounter;
    bool isLoopBoundConst;
    int BoundValue;

    LoopUnrollingPass() : LoopCounter(nullptr), isLoopBoundConst(false), BoundValue(0) {}

    void MapVariables(Loop *L) {
        VariablesMap.clear();
        Function *F = L->getHeader()->getParent();

        for (BasicBlock &BB : *F)
            for (Instruction &I : BB)
                if (isa<LoadInst>(&I))
                    VariablesMap[&I] = I.getOperand(0);
    }

    void findLoopCounterAndBound(Loop *L) {
        LoopCounter = nullptr;
        isLoopBoundConst = false;
        BoundValue = 0;

        auto inspectICmp = [&](ICmpInst *CI) {
            ConstantInt *ConstInt = nullptr;
            Value *VarOp = nullptr;

            for (unsigned op = 0; op < 2; ++op) {
                if ((ConstInt = dyn_cast<ConstantInt>(CI->getOperand(op)))) {
                    VarOp = CI->getOperand(1 - op);
                    break;
                }
            }

            if (ConstInt) {
                isLoopBoundConst = true;
                BoundValue = ConstInt->getSExtValue();
            }

            if (!VarOp) {
                for (unsigned op = 0; op < 2; ++op) {
                    Value *Candidate = CI->getOperand(op);
                    if (isa<LoadInst>(Candidate) || VariablesMap.find(Candidate) != VariablesMap.end()) {
                        VarOp = Candidate;
                        break;
                    }
                }
            }

            if (VarOp) {
                if (LoadInst *LI = dyn_cast<LoadInst>(VarOp))
                    LoopCounter = LI->getOperand(0);
                else if (VariablesMap.find(VarOp) != VariablesMap.end())
                    LoopCounter = VariablesMap[VarOp];
                else
                    LoopCounter = VarOp;
            }
        };

        for (Instruction &I : *L->getHeader()) {
            if (ICmpInst *CI = dyn_cast<ICmpInst>(&I)) {
                inspectICmp(CI);
                if (LoopCounter) return;
            }
        }

        if (BasicBlock *Latch = L->getLoopLatch()) {
            for (Instruction &I : *Latch) {
                if (ICmpInst *CI = dyn_cast<ICmpInst>(&I)) {
                    inspectICmp(CI);
                    if (LoopCounter) return;
                }
            }
        }

        for (BasicBlock *BB : L->blocks()) {
            for (Instruction &I : *BB) {
                if (ICmpInst *CI = dyn_cast<ICmpInst>(&I)) {
                    inspectICmp(CI);
                    if (LoopCounter) return;
                }
            }
        }
    }

    void fullUnrolling1(Loop *L) {
        std::vector<Instruction *> LoopInstructions;
        std::unordered_map<Value *, Value *> Mapping;
        Instruction *Copy;
        LoadInst *CounterLoad = nullptr;

        BasicBlock *LoopBody = LoopBasicBlocks[1];
        for (Instruction &I : *LoopBody) {
            if (!I.isTerminator()) {
                LoopInstructions.push_back(&I);
                if (!CounterLoad) {
                    if (auto *LI = dyn_cast<LoadInst>(&I))
                        if (LI->getOperand(0) == LoopCounter)
                            CounterLoad = LI;
                }
            }
        }

        Value *BaseCounter = nullptr;
        if (CounterLoad) {
            IRBuilder<> Builder(L->getLoopPreheader(),L->getLoopPreheader()->getTerminator()->getIterator());
            BaseCounter = Builder.CreateLoad(CounterLoad->getType(), LoopCounter);
        }

        for (int i = 1; i < BoundValue; i++) {
            Value *IterationCounter = nullptr;
            Mapping.clear();

            for (Instruction *I : LoopInstructions) {
                if (auto *LI = dyn_cast<LoadInst>(I)) {
                    if (LI->getOperand(0) == LoopCounter && BaseCounter) {
                        if (!IterationCounter) {
                            IterationCounter = BinaryOperator::CreateAdd(
                                BaseCounter,
                                ConstantInt::get(BaseCounter->getType(), i));
                            cast<Instruction>(IterationCounter)->insertBefore(LoopBody->getTerminator()->getIterator());
                        }
                        Mapping[I] = IterationCounter;
                        continue;
                    }
                }

                Copy = I->clone();
                Copy->insertBefore(LoopBody->getTerminator()->getIterator());
                Mapping[I] = Copy;

                for (size_t j = 0; j < Copy->getNumOperands(); j++)
                    if (Mapping.find(Copy->getOperand(j)) != Mapping.end())
                        Copy->setOperand(j, Mapping[Copy->getOperand(j)]);
            }
        }

        LoopBody->getTerminator()->eraseFromParent();
        L->getLoopPreheader()->splice(L->getLoopPreheader()->getTerminator()->getIterator(), LoopBody);
        L->getLoopPreheader()->getTerminator()->setSuccessor(0, L->getExitBlock());

        for (BasicBlock *BB : LoopBasicBlocks)
            BB->eraseFromParent();
    }

    void duplicateLoopBody(std::vector<BasicBlock *> LoopBodyBasicBlocks, int numOfTimes, BasicBlock *InsertBefore) {
        std::unordered_map<Value *, Value *> Mapping;
        std::unordered_map<Value *, Value *> LoadMapping;
        std::unordered_map<BasicBlock *, BasicBlock *> BlocksMapping;

        IRBuilder<> Builder(InsertBefore->getContext());
        Instruction *Copy;
        BasicBlock *LastFromPreviousCopy = LoopBodyBasicBlocks.back();
        std::vector<BasicBlock *> LoopBodyBasicBlockCopy;

        for (int i = 0; i < numOfTimes; i++) {
            LoopBodyBasicBlockCopy.clear();
            Mapping.clear();
            LoadMapping.clear();
            BlocksMapping.clear();

            for (size_t j = 0; j < LoopBodyBasicBlocks.size(); j++) {
                BasicBlock *NewBasicBlock = BasicBlock::Create(InsertBefore->getContext(), "", InsertBefore->getParent(), InsertBefore);
                LoopBodyBasicBlockCopy.push_back(NewBasicBlock);
                BlocksMapping[LoopBodyBasicBlocks[j]] = NewBasicBlock;
            }

            for (size_t j = 0; j < LoopBodyBasicBlocks.size(); j++) {
                Builder.SetInsertPoint(LoopBodyBasicBlockCopy[j]);

                for (Instruction &I : *LoopBodyBasicBlocks[j]) {
                    Copy = I.clone();
                    Builder.Insert(Copy);

                    if (isa<LoadInst>(Copy) && Copy->getOperand(0) == LoopCounter) {
                        Instruction *Add = (Instruction *) BinaryOperator::CreateAdd(
                            Copy, ConstantInt::get(Type::getInt32Ty(Copy->getContext()), i + 1));
                        Add->insertAfter(Copy);
                        LoadMapping[Copy] = Add;
                    }

                    Mapping[&I] = Copy;

                    for (size_t k = 0; k < Copy->getNumOperands(); k++) {
                        if (Mapping.find(Copy->getOperand(k)) != Mapping.end())
                            Copy->setOperand(k, Mapping[Copy->getOperand(k)]);
                        if (LoadMapping.find(Copy->getOperand(k)) != LoadMapping.end())
                            Copy->setOperand(k, LoadMapping[Copy->getOperand(k)]);
                    }
                }
            }

            for (size_t j = 0; j < LoopBodyBasicBlocks.size(); j++) {
                if (!LoopBodyBasicBlockCopy[j]->getTerminator()) continue;
                for (size_t k = 0; k < LoopBodyBasicBlockCopy[j]->getTerminator()->getNumSuccessors(); k++) {
                    BasicBlock *Succ = LoopBodyBasicBlocks[j]->getTerminator()->getSuccessor(k);
                    if (BlocksMapping.find(Succ) != BlocksMapping.end())
                        LoopBodyBasicBlockCopy[j]->getTerminator()->setSuccessor(k, BlocksMapping[Succ]);
                }
            }

            LastFromPreviousCopy->getTerminator()->setSuccessor(0, LoopBodyBasicBlockCopy.front());
            LastFromPreviousCopy = LoopBodyBasicBlockCopy.back();
        }

        if (!LoopBodyBasicBlockCopy.empty())
            LoopBodyBasicBlockCopy.back()->getTerminator()->setSuccessor(0, InsertBefore);
    }

    void fullUnrolling(Loop *L) {
        BasicBlock *Exit = L->getExitBlock();

        std::vector<BasicBlock *> LoopBodyBlocks(LoopBasicBlocks.size() - 2);
        std::copy(LoopBasicBlocks.begin() + 1, LoopBasicBlocks.end() - 1, LoopBodyBlocks.begin());

        L->getLoopPreheader()->getTerminator()->setSuccessor(0, LoopBodyBlocks.front());
        LoopBodyBlocks.back()->getTerminator()->setSuccessor(0, Exit);

        duplicateLoopBody(LoopBodyBlocks, BoundValue - 1, Exit);

        LoopBasicBlocks.front()->eraseFromParent();
        LoopBasicBlocks.back()->eraseFromParent();
    }

    void partialUnrolling1(Loop *L) {
        std::vector<Instruction *> LoopInstructions;
        std::unordered_map<Value *, Value *> Mapping;
        std::unordered_map<Value *, Value *> LoadMapping;
        BasicBlock *LoopBody = LoopBasicBlocks[1];

        for (Instruction &I : *LoopBody)
            if (!I.isTerminator())
                LoopInstructions.push_back(&I);

        Instruction *Copy;

        for (int i = 0; i < UnrollFactor - 1; i++) {
            Mapping.clear();
            LoadMapping.clear();

            for (Instruction *I : LoopInstructions) {
                Copy = I->clone();
                Copy->insertBefore(LoopBody->getTerminator()->getIterator());

                if (isa<LoadInst>(Copy) && Copy->getOperand(0) == LoopCounter) {
                    Instruction *Add = (Instruction *) BinaryOperator::CreateAdd(
                        Copy, ConstantInt::get(Type::getInt32Ty(Copy->getContext()), i + 1));
                    Add->insertAfter(Copy);
                    LoadMapping[Copy] = Add;
                }

                Mapping[I] = Copy;

                for (size_t j = 0; j < Copy->getNumOperands(); j++) {
                    if (Mapping.find(Copy->getOperand(j)) != Mapping.end())
                        Copy->setOperand(j, Mapping[Copy->getOperand(j)]);
                    if (LoadMapping.find(Copy->getOperand(j)) != LoadMapping.end())
                        Copy->setOperand(j, LoadMapping[Copy->getOperand(j)]);
                }
            }
        }

        if (BasicBlock *Latch = L->getLoopLatch()) {
            for (Instruction &I : *Latch) {
                if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
                    if (BO->getOpcode() == Instruction::Add) {
                        if (auto *LI = dyn_cast<LoadInst>(BO->getOperand(0))) {
                            if (LI->getOperand(0) == LoopCounter) {
                                if (auto *C = dyn_cast<ConstantInt>(BO->getOperand(1))) {
                                    if (C->getSExtValue() == 1)
                                        BO->setOperand(1, ConstantInt::get(C->getType(), UnrollFactor));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    BasicBlock* copyLoop(Loop *L) {
        BasicBlock *Exit = L->getExitBlock();
        std::unordered_map<Value *, Value *> Mapping;
        std::unordered_map<BasicBlock *, BasicBlock *> BlocksMapping;
        IRBuilder<> Builder(Exit->getContext());
        Instruction *Copy;
        std::vector<BasicBlock *> LoopBasicBlockCopy;

        for (size_t j = 0; j < LoopBasicBlocks.size(); j++) {
            BasicBlock *NewBasicBlock = BasicBlock::Create(Exit->getContext(), "", Exit->getParent(), Exit);
            LoopBasicBlockCopy.push_back(NewBasicBlock);
            BlocksMapping[LoopBasicBlocks[j]] = NewBasicBlock;
        }

        for (size_t j = 0; j < LoopBasicBlocks.size(); j++) {
            Builder.SetInsertPoint(LoopBasicBlockCopy[j]);

            for (Instruction &I : *LoopBasicBlocks[j]) {
                Copy = I.clone();
                Builder.Insert(Copy);
                Mapping[&I] = Copy;

                for (size_t k = 0; k < Copy->getNumOperands(); k++)
                    if (Mapping.find(Copy->getOperand(k)) != Mapping.end())
                        Copy->setOperand(k, Mapping[Copy->getOperand(k)]);
            }
        }

        for (size_t j = 0; j < LoopBasicBlocks.size(); j++) {
            if (!LoopBasicBlockCopy[j]->getTerminator()) continue;
            for (size_t k = 0; k < LoopBasicBlocks[j]->getTerminator()->getNumSuccessors(); k++)
                if (BlocksMapping.find(LoopBasicBlocks[j]->getTerminator()->getSuccessor(k)) != BlocksMapping.end())
                    LoopBasicBlockCopy[j]->getTerminator()->setSuccessor(k, BlocksMapping[LoopBasicBlocks[j]->getTerminator()->getSuccessor(k)]);
        }

        return LoopBasicBlockCopy.front();
    }

    void partialUnrolling(Loop *L) {
        BasicBlock *JumpTo = copyLoop(L);

        std::vector<BasicBlock *> LoopBodyBasicBlocks(LoopBasicBlocks.size() - 2);
        std::copy(LoopBasicBlocks.begin() + 1, LoopBasicBlocks.end() - 1, LoopBodyBasicBlocks.begin());

        duplicateLoopBody(LoopBodyBasicBlocks, UnrollFactor - 1, LoopBasicBlocks.back());

        LoopBasicBlocks.front()->getTerminator()->setSuccessor(1, JumpTo);

        for (Instruction &I : *LoopBasicBlocks.front()) {
            if (isa<LoadInst>(&I) && I.getOperand(0) == LoopCounter) {
                Instruction *Add = (Instruction *) BinaryOperator::CreateAdd(
                    &I, ConstantInt::get(Type::getInt32Ty(I.getContext()), UnrollFactor - 1));
                Add->insertAfter(&I);
                I.replaceUsesWithIf(Add, [Add](Use &U) {
                    return U.getUser() != Add;
                });
            }
        }

        for (Instruction &I : *LoopBasicBlocks.back()) {
            if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
                if (BO->getOpcode() == Instruction::Add) {
                    if (auto *LI = dyn_cast<LoadInst>(BO->getOperand(0))) {
                        if (LI->getOperand(0) == LoopCounter)
                            BO->setOperand(1, ConstantInt::get(Type::getInt32Ty(I.getContext()), UnrollFactor));
                    }
                }
            }
        }
    }

    void unrollLoop(Loop *L) {
        if (isLoopBoundConst) {
            if (LoopBasicBlocks.size() == 3)
                fullUnrolling1(L);
            else
                fullUnrolling(L);
        } else {
            if (LoopBasicBlocks.size() == 3)
                partialUnrolling1(L);
            else
                partialUnrolling(L);
        }
    }

    void debugPrintLoopInfo(Loop *L) {
        errs() << "[OurUnroll] Running on loop in function: "
               << L->getHeader()->getParent()->getName() << "\n";
        errs() << "[OurUnroll] Header: " << L->getHeader()->getName()
               << ", blocks: " << L->getBlocks().size() << "\n";
        errs() << "[OurUnroll] isLoopBoundConst=" << isLoopBoundConst
               << " BoundValue=" << BoundValue << "\n";
        errs() << "[OurUnroll] LoopCounter: ";
        if (LoopCounter)
            LoopCounter->print(errs());
        else
            errs() << "<null>";
        errs() << "\n";
    }

    bool isLoopCounterModifiedInBody(Loop *L, Value *LoopCounter) {
        if (!LoopCounter) return false;

        BasicBlock *Header = L->getHeader();
        BasicBlock *Latch = L->getLoopLatch();

        for (BasicBlock *BB : L->blocks()) {
            if (BB == Header || BB == Latch) continue;

            for (Instruction &I : *BB) {
                if (auto *Store = dyn_cast<StoreInst>(&I))
                    if (Store->getPointerOperand() == LoopCounter)
                        return true;
            }
        }
        return false;
    }

    PreservedAnalyses run(Loop &L, LoopAnalysisManager &LAM,
                          LoopStandardAnalysisResults &AR, LPMUpdater &) {
        LoopBasicBlocks = L.getBlocksVector();
        MapVariables(&L);
        findLoopCounterAndBound(&L);

        if (isLoopCounterModifiedInBody(&L, LoopCounter)) {
            errs() << "Loop counter is modified inside loop body. Loop unrolling won't be done!\n";
            return PreservedAnalyses::all();
        }

        unrollLoop(&L);
        return PreservedAnalyses::none();
    }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo()
{
    return {LLVM_PLUGIN_API_VERSION, "LoopUnrollingPass", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, LoopPassManager &LPM,
                       ArrayRef<PassBuilder::PipelineElement>) {
                        if (Name == "loop-unrolling-pass") {
                            LPM.addPass(LoopUnrollingPass());
                            return true;
                        }
                        return false;
                    });
            }};
}
