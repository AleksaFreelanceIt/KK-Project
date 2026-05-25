//
// Created by andjela375 on 5/24/26.
//
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/LoopPeel.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/SizeOpts.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/IRBuilder.h"

#include<vector>
#include<unordered_map>

using namespace llvm;

namespace {

struct OurLoopUnrollingPass : public LoopPass {
    std::vector<BasicBlock *> LoopBasicBlocks;
    std::unordered_map<Value *, Value *> VariablesMap;
    Value *LoopCounter, *LoopBound;
    bool isLoopBoundConst;
    int BoundValue;
    static char ID; // Pass identification, replacement for typeid
    OurLoopUnrollingPass() : LoopPass(ID), LoopCounter(nullptr), LoopBound(nullptr), isLoopBoundConst(false), BoundValue(0) {}

    void MapVariables(Loop *L) {
        VariablesMap.clear();
        Function *F = L->getHeader()->getParent();

        for (BasicBlock &BB : *F) {
            for (Instruction &I : BB) {
                if (isa<LoadInst>(&I)) {
                    VariablesMap[&I] = I.getOperand(0);
                }
            }
        }
    }

    void findLoopCounterAndBound(Loop *L) {
        // Reset the state before scanning.
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
                if (LoadInst *LI = dyn_cast<LoadInst>(VarOp)) {
                    LoopCounter = LI->getOperand(0);
                } else if (VariablesMap.find(VarOp) != VariablesMap.end()) {
                    LoopCounter = VariablesMap[VarOp];
                } else {
                    LoopCounter = VarOp;
                }
            }
        };

        // Search the header, the latch, and all loop blocks for the compare.
        for (Instruction &I : *L->getHeader()) {
            if (ICmpInst *CI = dyn_cast<ICmpInst>(&I)) {
                inspectICmp(CI);
                if (LoopCounter)
                    return;
            }
        }

        if (BasicBlock *Latch = L->getLoopLatch()) {
            for (Instruction &I : *Latch) {
                if (ICmpInst *CI = dyn_cast<ICmpInst>(&I)) {
                    inspectICmp(CI);
                    if (LoopCounter)
                        return;
                }
            }
        }

        for (BasicBlock *BB : L->blocks()) {
            for (Instruction &I : *BB) {
                if (ICmpInst *CI = dyn_cast<ICmpInst>(&I)) {
                    inspectICmp(CI);
                    if (LoopCounter)
                        return;
                }
            }
        }
    }


// za jedan basic block
// instrukcije ubacujemo u preheader, onda ih multipliciramo i mapiramo da imaju odgovarajuce argumente
void fullUnrolling1(Loop *L) {

    std::vector<Instruction *> LoopInstructions;
    std::unordered_map<Value *, Value *> Mapping;
    Instruction *Copy;
    LoadInst *CounterLoad = nullptr;

    BasicBlock *LoopBody = LoopBasicBlocks[1]; // kada radimo sa samo jednim basic blockom

    for (Instruction &I : *LoopBody) {
        if (!I.isTerminator()) {
            LoopInstructions.push_back(&I);
            if (!CounterLoad) {
                if (auto *LI = dyn_cast<LoadInst>(&I)) {
                    if (LI->getOperand(0) == LoopCounter)
                        CounterLoad = LI;
                }
            }
        }
    }

    Value *BaseCounter = nullptr;
    if (CounterLoad) {
        IRBuilder<> Builder(L->getLoopPreheader()->getTerminator());
        BaseCounter = Builder.CreateLoad(CounterLoad->getType(), LoopCounter);
    }

    for (int i = 0; i < BoundValue; i++) {
        Value *IterationCounter = nullptr;
        for (Instruction *I : LoopInstructions) {
            if (auto *LI = dyn_cast<LoadInst>(I)) {
                if (LI->getOperand(0) == LoopCounter && BaseCounter) {
                    if (!IterationCounter) {
                        if (i == 0) {
                            IterationCounter = BaseCounter;
                        } else {
                            IterationCounter = BinaryOperator::CreateAdd(
                                BaseCounter,
                                ConstantInt::get(BaseCounter->getType(), i)
                            );
                            cast<Instruction>(IterationCounter)->insertBefore(LoopBody->getTerminator());
                        }
                    }
                    Mapping[I] = IterationCounter;
                    continue;
                }
            }

            Copy = I->clone();
            Copy->insertBefore(LoopBody->getTerminator());
            Mapping[I] = Copy;

            for (size_t j = 0; j < Copy->getNumOperands(); j++) {
                if (Mapping.find(Copy->getOperand(j)) != Mapping.end()) {
                    Copy->setOperand(j, Mapping[Copy->getOperand(j)]);
                }
            }
        }
    }

    LoopBody->getTerminator()->eraseFromParent(); // moramo da je obrisemo zbog splice-a
    L->getLoopPreheader()->splice(L->getLoopPreheader()->getTerminator()->getIterator(), LoopBody); // stavlja(kopira) sve instrukcije iz LoopBody prije terminirajuce instrukcije
    L->getLoopPreheader()->getTerminator()->setSuccessor(0, L->getExitBlock()); // preheader sad treba da pokazuje na prvi basic block van granica pretlje

    // brisemo petlju jer nam ne treba vise
    for (BasicBlock *BB : LoopBasicBlocks) {
        BB->eraseFromParent();
    }
}

// prvo brisemo basic block koji predstavlja provjeru uslova i basic block koji skace na pocetak petlje i azurira brojac
// ostaju nam oni koji predstavljaju tijelo petlje i n-1 put ga kopiramo
// poslednji basic block treba da pokazuje na exit basic block i svaki block treba da pokazuje na pocetak narednog

void duplicateLoopBody(std::vector<BasicBlock*> LoopBodyBasicBlocks, int numOfTimes, BasicBlock *InsertBefore) {
    std::unordered_map<Value *, Value *> Mapping;
    std::unordered_map<Value *, Value *> LoadMapping;
    std::unordered_map<BasicBlock *, BasicBlock *> BlocksMapping;
    IRBuilder<>Builder(InsertBefore->getContext());
    Instruction *Copy;
    BasicBlock *LastFromPreviousCopy = LoopBodyBasicBlocks.back(); // kako bi znali na sta da prebacimo da pokazuje
    std::vector<BasicBlock *> LoopBodyBasicBlockCopy;

    for(int i = 0; i < numOfTimes; i++) {
        LoopBodyBasicBlockCopy.clear();
        for(size_t j = 0; j < LoopBodyBasicBlocks.size(); j++) { // pravimo prazne basic blockove onoliko koliko ih je bilo u telu petlje(prazne jer ne mozemo da kopiramo basic blockove vec napravimo prazne i kopiramo instrukcije u njih)
            BasicBlock *NewBasicBlock = BasicBlock::Create(InsertBefore->getContext());
            NewBasicBlock->insertInto(InsertBefore->getParent(), InsertBefore);
            LoopBodyBasicBlockCopy.push_back(NewBasicBlock);
            BlocksMapping[LoopBodyBasicBlocks[j]] = NewBasicBlock; // sluzi za kasnije prevezivanje basic blockova (ono sa A->B i A'->B)
        }

        // svaku originalnu instrukciju iz originalnog basic blocka iskopiramo u copy block
        for(size_t j = 0; j < LoopBodyBasicBlocks.size(); j++) {
            Builder.SetInsertPoint(LoopBodyBasicBlockCopy[j]); // smjestaju se na kraj basic blocka

            for(Instruction &I : *LoopBodyBasicBlocks[j]) {
                Copy = I.clone();
                Builder.Insert(Copy);

                if(isa<LoadInst>(Copy) && Copy->getOperand(0) == LoopCounter) {
                    Instruction *Add = (Instruction *) BinaryOperator::CreateAdd(Copy, ConstantInt::get(Type::getInt32Ty(Copy->getContext()), i+1));

                    Add->insertAfter(Copy);
                    LoadMapping[Copy] = Add;
                }
                Mapping[&I] = Copy;

                for(size_t k = 0; k < Copy->getNumOperands(); k++) {
                    if(Mapping.find(Copy->getOperand(k)) != Mapping.end()) {
                        Copy->setOperand(k, Mapping[Copy->getOperand(k)]);
                    }
                    if(LoadMapping.find(Copy->getOperand(k)) != LoadMapping.end()) {
                        Copy->setOperand(k, LoadMapping[Copy->getOperand(k)]);
                    }
                }
            }
        }
        for(size_t j = 0; j < LoopBodyBasicBlocks.size(); j++) {
            for(size_t k = 0; k < LoopBodyBasicBlockCopy[j]->getTerminator()->getNumSuccessors(); k++) {
                if(BlocksMapping.find(LoopBodyBasicBlocks[j]->getTerminator()->getSuccessor(k)) != BlocksMapping.end()) { // gledamo da li postoji mapiranje uopste jer ako ne postoji onda je to poslednji basic block tijela petlje i on nije mapiran ni u sta
                // k-tog suksesora postavlja na ono u sta se mapira dati suksesor originalnog basic blocka
                    LoopBodyBasicBlockCopy[j]->getTerminator()->setSuccessor(k, BlocksMapping[LoopBodyBasicBlocks[j]->getTerminator()->getSuccessor(k)]);
                }
            }
        }

        LastFromPreviousCopy->getTerminator()->setSuccessor(0, LoopBodyBasicBlockCopy.front()); // prevezali smo prvi
        LastFromPreviousCopy = LoopBodyBasicBlockCopy.back(); // poslednji je prevezan
    }
    LoopBodyBasicBlockCopy.back()->getTerminator()->setSuccessor(0, InsertBefore); // prevezan je poslednji basic block da pokazuje na exit basic block
}

// kada imamo vise basic blockova
void fullUnrolling(Loop *L) {
    BasicBlock *Preheader = L->getLoopPreheader();
    BasicBlock *Exit = L->getExitBlock();
    Instruction *OldTerm = Preheader->getTerminator();
    Instruction *InsertPt = OldTerm;

    std::vector<BasicBlock*> LoopBodyBlocks;
    if (LoopBasicBlocks.size() == 1) {
        LoopBodyBlocks.push_back(LoopBasicBlocks[0]);
    } else {
        std::copy(LoopBasicBlocks.begin() + 1, LoopBasicBlocks.end(), std::back_inserter(LoopBodyBlocks));
    }

    for (int i = 0; i < BoundValue; ++i) {
        std::unordered_map<Value *, Value *> Mapping;

        for (BasicBlock *BB : LoopBodyBlocks) {
            for (Instruction &I : *BB) {
                if (I.isTerminator())
                    continue;

                Instruction *Copy = I.clone();
                Copy->insertBefore(InsertPt);

                for (unsigned j = 0; j < Copy->getNumOperands(); ++j) {
                    Value *Op = Copy->getOperand(j);
                    if (Mapping.find(Op) != Mapping.end()) {
                        Copy->setOperand(j, Mapping[Op]);
                    }
                }

                Mapping[&I] = Copy;
            }
        }
    }

    BranchInst::Create(Exit, Preheader);
    OldTerm->eraseFromParent();

    for (BasicBlock *BB : LoopBasicBlocks) {
        BB->eraseFromParent();
    }
}

// kada imamo jedan basic block
void partialUnrolling1(Loop *L) {
    std::vector<Instruction *> LoopInstructions;
    std::unordered_map<Value *, Value *> Mapping;
    std::unordered_map<Value *, Value *> LoadMapping; // treba nam da zapamtimo gdje koristimo counter i da ga sacuvamo (ako imamo a[i]=i treba nam da imamo +1, +2,...)
    BasicBlock *LoopBody = LoopBasicBlocks[1];

    for(Instruction &I : *LoopBody) {
        if(!I.isTerminator()) {
            LoopInstructions.push_back(&I);
        }
    }

    int Factor = 3;

    Instruction *Copy;

    for(int i = 0; i < Factor; i++) {
        for(Instruction *I : LoopInstructions) {
            Copy = I->clone();
            Copy->insertBefore(LoopBody->getTerminator());

            if(isa<LoadInst>(Copy) && Copy->getOperand(0) == LoopCounter) {
                Instruction *Add = (Instruction*) BinaryOperator::CreateAdd(Copy, ConstantInt::get(Type::getInt32Ty(Copy->getContext()), i+1));
                Add->insertAfter(Copy);
                LoadMapping[Copy] = Add;
            }
            Mapping[I] = Copy;

            for(size_t j = 0; j < Copy->getNumOperands(); j++) {
                if(Mapping.find(Copy->getOperand(j)) != Mapping.end()) {
                    Copy->setOperand(j, Mapping[Copy->getOperand(j)]);
                }
                if(LoadMapping.find(Copy->getOperand(j)) != LoadMapping.end()) {
                    Copy->setOperand(j, LoadMapping[Copy->getOperand(j)]);
                }
            }
        }
    }
}

BasicBlock* copyLoop(Loop *L) {
    BasicBlock *Exit = L->getExitBlock();
    std::unordered_map<Value *, Value *> Mapping;
    std::unordered_map<BasicBlock *, BasicBlock *> BlocksMapping;
    IRBuilder<> Builder(Exit->getContext()); // uzimamo neki basic block za koji znamo da sigurno postoji
    Instruction *Copy;
    std::vector<BasicBlock *> LoopBasicBlockCopy;

    for(size_t j = 0; j < LoopBasicBlocks.size(); j++) { // pravimo prazne basic blockove onoliko koliko ih je bilo u telu petlje(prazne jer ne mozemo da kopiramo basic blockove vec napravimo prazne i kopiramo instrukcije u njih)
        BasicBlock *NewBasicBlock = BasicBlock::Create(Exit->getContext(), "", Exit->getParent(), Exit);
        LoopBasicBlockCopy.push_back(NewBasicBlock);
        BlocksMapping[LoopBasicBlocks[j]] = NewBasicBlock; // sluzi za kasnije prevezivanje basic blockova (ono sa A->B i A'->B)
    }

    for(size_t j = 0; j < LoopBasicBlocks.size(); j++) {
        Builder.SetInsertPoint(LoopBasicBlockCopy[j]);

        for(Instruction &I : *LoopBasicBlocks[j]) {
            Copy = I.clone();
            Builder.Insert(Copy);
            Mapping[&I] = Copy;

            for(size_t k = 0; k < Copy->getNumOperands(); k++) {
                if(Mapping.find(Copy->getOperand(k)) != Mapping.end()) {
                    Copy->setOperand(k, Mapping[Copy->getOperand(k)]);
                }
            }
        }
    }
    for(size_t j = 0; j < LoopBasicBlocks.size(); j++) {
        for(size_t k = 0; k < LoopBasicBlocks[j]->getTerminator()->getNumSuccessors(); k++) {
            if(BlocksMapping.find(LoopBasicBlocks[j]->getTerminator()->getSuccessor(k)) != BlocksMapping.end()) {
                LoopBasicBlockCopy[j]->getTerminator()->setSuccessor(k, BlocksMapping[LoopBasicBlocks[j]->getTerminator()->getSuccessor(k)]);
            }
        }
    }
    return LoopBasicBlockCopy.front();
}

void partialUnrolling(Loop *L) {
    int Factor = 3;

    BasicBlock *JumpTo = copyLoop(L);

    std::vector<BasicBlock *> LoopBodyBasicBlocks(LoopBasicBlocks.size()-2);
    std::copy(LoopBasicBlocks.begin()+1, LoopBasicBlocks.end()-1, LoopBodyBasicBlocks.begin());

    duplicateLoopBody(LoopBodyBasicBlocks, Factor-1, LoopBasicBlocks.back());

    LoopBasicBlocks.front()->getTerminator()->setSuccessor(1, JumpTo);

    for(Instruction &I : *LoopBasicBlocks.front()) {
        // mi pretpostavljamo da je i uvijek prvi operand
        if(isa<LoadInst>(&I) && I.getOperand(0) == LoopCounter) {
            Instruction *Add = (Instruction *) BinaryOperator::CreateAdd(&I, ConstantInt::get(Type::getInt32Ty(I.getContext()), Factor-1));
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
                if (LI->getOperand(0) == LoopCounter) {
                    BO->setOperand(
                        1,
                        ConstantInt::get(Type::getInt32Ty(I.getContext()), Factor)
                    );
                }
            }
        }
    }
}
}

void unrollLoop(Loop *L) {
    if(isLoopBoundConst) {
        fullUnrolling(L);
    } else {
        partialUnrolling(L);
    }
}
//Debug f-ja
void debugPrintLoopInfo(Loop *L) {
    errs() << "[OurUnroll] Running on loop in function: "
           << L->getHeader()->getParent()->getName() << "\n";
    errs() << "[OurUnroll] Header: " << L->getHeader()->getName()
           << ", blocks: " << L->getBlocks().size() << "\n";
    errs() << "[OurUnroll] isLoopBoundConst=" << isLoopBoundConst
           << " BoundValue=" << BoundValue << "\n";
    errs() << "[OurUnroll] LoopCounter: ";
    if (LoopCounter) {
        LoopCounter->print(errs());
        errs() << "\n";
    } else {
        errs() << "<null>\n";
    }
}

    // LPM mi necemo koristiti ali se prosledjuje kao parametar
    bool runOnLoop(Loop *L, LPPassManager &LPM) override {

        LoopBasicBlocks = L->getBlocksVector();
        MapVariables(L);
        findLoopCounterAndBound(L);
        unrollLoop(L);
        return true;
    }
  };
}

char OurLoopUnrollingPass::ID = 0;
static RegisterPass<OurLoopUnrollingPass> X("loop-unrolling", "Our simple loop unrolling pass", false, false);
