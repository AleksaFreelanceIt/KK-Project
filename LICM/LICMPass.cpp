#include <unordered_set>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils.h"

using namespace llvm;

namespace
{

struct LICMPass : public LoopPass
{
    static char ID;
    static std::unordered_set<unsigned int> GoodInstructions;

    LICMPass() : LoopPass(ID) {}

    std::unordered_set<Instruction*> findHoistableLoads(Loop* L)
    {
        std::unordered_set<Value*> UnhoistableLoads;

        Module* M = L->getHeader()->getModule();
        for (GlobalVariable& GV : M->globals())
        {
            if (!GV.isConstant())
            {
                UnhoistableLoads.insert(&GV);
            }
        }

        for (BasicBlock* BB : L->blocks())
        {
            for (Instruction& I : *BB)
            {
                if (StoreInst* SI = dyn_cast<StoreInst>(&I))
                {
                    UnhoistableLoads.insert(SI->getPointerOperand());
                }
                else if (CallInst* CI = dyn_cast<CallInst>(&I))
                {
                    for (Value* Arg : CI->args())
                    {
                        if (Arg->getType()->isPointerTy())
                        {
                            UnhoistableLoads.insert(Arg);
                        }
                    }
                }
                else if (GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(&I))
                {
                    UnhoistableLoads.insert(GEP);
                }
            }
        }

        std::unordered_set<Instruction*> HoistableLoads;

        for (BasicBlock* BB : L->blocks())
        {
            for (Instruction& I : *BB)
            {
                if (LoadInst* LI = dyn_cast<LoadInst>(&I))
                {
                    if (UnhoistableLoads.count(LI->getPointerOperand()) == 0)
                    {
                        HoistableLoads.insert(LI);
                    }
                }
            }
        }

        return HoistableLoads;
    }

    bool isGoodInstruction(const Instruction* I) { return GoodInstructions.count(I->getOpcode()); }

    bool isHoistableInstruction(Instruction* I, std::unordered_set<Instruction*>& HoistableLoads,
                                const std::unordered_set<Instruction*>& NotHoisted)
    {
        if (LoadInst* LI = dyn_cast<LoadInst>(I))
        {
            return HoistableLoads.count(LI) == 1;
        }

        if (!isGoodInstruction(I))
        {
            return false;
        }

        for (Value* Op : I->operands())
        {
            bool ShouldAdd = true;

            if (Instruction* OpInst = dyn_cast<Instruction>(Op))
            {
                if (NotHoisted.count(OpInst) == 1)
                {
                    ShouldAdd = false;
                }
            }

            if (!ShouldAdd)
            {
                return false;
            }
        }

        return true;
    }

    void findHoistableInstructions(const Loop* L, std::unordered_set<Instruction*>& HoistableLoads,
                                   std::unordered_set<Instruction*>& NotHoisted,
                                   std::vector<Instruction*>& ToHoist)
    {
        for (BasicBlock* BB : L->blocks())
        {
            for (Instruction& I : *BB)
            {
                if (isHoistableInstruction(&I, HoistableLoads, NotHoisted))
                {
                    ToHoist.push_back(&I);
                }
                else
                {
                    NotHoisted.insert(&I);
                }
            }
        }
    }

    void moveHoistableLoads(BasicBlock* LoopPreheader, const std::vector<Instruction*>& ToHoist)
    {
        for (Instruction* I : ToHoist)
        {
            I->moveBefore(LoopPreheader->getTerminator());
        }
    }

    bool runOnLoop(Loop* L, LPPassManager& LPM) override
    {
        BasicBlock* LoopPreheader = L->getLoopPreheader();
        if (LoopPreheader == nullptr)
        {
            return false;
        }

        std::unordered_set<Instruction*> HoistableLoads(findHoistableLoads(L));
        std::unordered_set<Instruction*> NotHoisted;
        std::vector<Instruction*> ToHoist;

        findHoistableInstructions(L, HoistableLoads, NotHoisted, ToHoist);

        moveHoistableLoads(LoopPreheader, ToHoist);

        return !ToHoist.empty();
    }

    void getAnalysisUsage(AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequiredID(LoopSimplifyID);
    }
};
}  // namespace

char LICMPass::ID = 0;
std::unordered_set<unsigned int> LICMPass::GoodInstructions = {
    Instruction::Add,     Instruction::Sub,    Instruction::Mul,    Instruction::UDiv,
    Instruction::SDiv,    Instruction::URem,   Instruction::SRem,   Instruction::FAdd,
    Instruction::FSub,    Instruction::FMul,   Instruction::FDiv,   Instruction::FRem,
    Instruction::And,     Instruction::Or,     Instruction::Xor,    Instruction::Shl,
    Instruction::LShr,    Instruction::AShr,   Instruction::FNeg,   Instruction::ZExt,
    Instruction::SExt,    Instruction::Trunc,  Instruction::FPExt,  Instruction::FPTrunc,
    Instruction::FPToUI,  Instruction::FPToSI, Instruction::UIToFP, Instruction::SIToFP,
    Instruction::BitCast, Instruction::ICmp,   Instruction::FCmp,   Instruction::GetElementPtr};

static RegisterPass<LICMPass> X("licm-pass", "The implementation of the LICM pass for the KK project");
