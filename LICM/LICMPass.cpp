#include <unordered_set>
#include <vector>

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"

using namespace llvm;

namespace
{

struct LICMPass : public PassInfoMixin<LICMPass>
{
    static std::unordered_set<unsigned int> GoodInstructions;

    std::unordered_set<Instruction*> findHoistableLoads(Loop* L)
    {
        std::unordered_set<Value*> UnhoistableLoads;

        Module* M = L->getHeader()->getModule();
        for (GlobalVariable& GV : M->globals())
        {
            if (!GV.isConstant()) UnhoistableLoads.insert(&GV);
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
                        if (Arg->getType()->isPointerTy()) UnhoistableLoads.insert(Arg);
                }
                else if (GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(&I))
                {
                    UnhoistableLoads.insert(GEP);
                }
            }
        }

        std::unordered_set<Instruction*> HoistableLoads;
        for (BasicBlock* BB : L->blocks())
            for (Instruction& I : *BB)
                if (LoadInst* LI = dyn_cast<LoadInst>(&I))
                    if (UnhoistableLoads.count(LI->getPointerOperand()) == 0) HoistableLoads.insert(LI);

        return HoistableLoads;
    }

    bool isGoodInstruction(const Instruction* I) { return GoodInstructions.count(I->getOpcode()); }

    bool isHoistableInstruction(Instruction* I, std::unordered_set<Instruction*>& HoistableLoads,
                                const std::unordered_set<Instruction*>& NotHoisted)
    {
        if (LoadInst* LI = dyn_cast<LoadInst>(I)) return HoistableLoads.count(LI) == 1;

        if (!isGoodInstruction(I)) return false;

        for (Value* Op : I->operands())
            if (Instruction* OpInst = dyn_cast<Instruction>(Op))
                if (NotHoisted.count(OpInst) == 1) return false;

        return true;
    }

    void findHoistableInstructions(const Loop* L, std::unordered_set<Instruction*>& HoistableLoads,
                                   std::unordered_set<Instruction*>& NotHoisted,
                                   std::vector<Instruction*>& ToHoist)
    {
        for (BasicBlock* BB : L->blocks())
            for (Instruction& I : *BB)
                if (isHoistableInstruction(&I, HoistableLoads, NotHoisted))
                    ToHoist.push_back(&I);
                else
                    NotHoisted.insert(&I);
    }

    void moveHoistableLoads(BasicBlock* Preheader, const std::vector<Instruction*>& ToHoist)
    {
        for (Instruction* I : ToHoist) I->moveBefore(Preheader->getTerminator()->getIterator());
    }

    PreservedAnalyses run(Loop& L, LoopAnalysisManager& LAM, LoopStandardAnalysisResults& AR, LPMUpdater&)
    {
        BasicBlock* Preheader = L.getLoopPreheader();
        if (!Preheader) return PreservedAnalyses::all();

        auto HoistableLoads = findHoistableLoads(&L);
        std::unordered_set<Instruction*> NotHoisted;
        std::vector<Instruction*> ToHoist;

        findHoistableInstructions(&L, HoistableLoads, NotHoisted, ToHoist);
        moveHoistableLoads(Preheader, ToHoist);

        if (ToHoist.empty()) return PreservedAnalyses::all();

        PreservedAnalyses PA;
        PA.preserveSet<CFGAnalyses>();
        return PA;
    }
};

std::unordered_set<unsigned int> LICMPass::GoodInstructions = {
    Instruction::Add,     Instruction::Sub,    Instruction::Mul,    Instruction::UDiv,
    Instruction::SDiv,    Instruction::URem,   Instruction::SRem,   Instruction::FAdd,
    Instruction::FSub,    Instruction::FMul,   Instruction::FDiv,   Instruction::FRem,
    Instruction::And,     Instruction::Or,     Instruction::Xor,    Instruction::Shl,
    Instruction::LShr,    Instruction::AShr,   Instruction::FNeg,   Instruction::ZExt,
    Instruction::SExt,    Instruction::Trunc,  Instruction::FPExt,  Instruction::FPTrunc,
    Instruction::FPToUI,  Instruction::FPToSI, Instruction::UIToFP, Instruction::SIToFP,
    Instruction::BitCast, Instruction::ICmp,   Instruction::FCmp,   Instruction::GetElementPtr};

}  // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo()
{
    return {LLVM_PLUGIN_API_VERSION, "LICMPass", LLVM_VERSION_STRING, [](PassBuilder& PB)
            {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, LoopPassManager& LPM, ArrayRef<PassBuilder::PipelineElement>)
                    {
                        if (Name == "licm-pass")
                        {
                            LPM.addPass(LICMPass());
                            return true;
                        }
                        return false;
                    });
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager& FPM, ArrayRef<PassBuilder::PipelineElement>)
                    {
                        if (Name == "licm-pass")
                        {
                            FPM.addPass(PromotePass());
                            LoopPassManager LPM;
                            LPM.addPass(LICMPass());
                            FPM.addPass(createFunctionToLoopPassAdaptor(std::move(LPM)));
                            return true;
                        }
                        return false;
                    });
            }};
}
