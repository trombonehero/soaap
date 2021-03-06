#include "Analysis/InfoFlow/FPTargetsAnalysis.h"
#include "Util/DebugUtils.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "soaap.h"

#include <sstream>

using namespace soaap;

map<Function*,int> FPTargetsAnalysis::funcToIdx;
map<int,Function*> FPTargetsAnalysis::idxToFunc;

void FPTargetsAnalysis::initialise(ValueContextPairList& worklist, Module& M, SandboxVector& sandboxes) {
  // iniitalise funcToIdx and idxToFunc maps (once)
  if (funcToIdx.empty()) {
    int nextIdx = 0;
    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      if (F->isDeclaration()) continue;
      if (F->hasAddressTaken()) { // limit to only those funcs that could be fp targets
        funcToIdx[F] = nextIdx;
        idxToFunc[nextIdx] = F;
        nextIdx++;
      }
    }
  }
}

void FPTargetsAnalysis::postDataFlowAnalysis(Module& M, SandboxVector& sandboxes) {
}

// return the union of from and to
bool FPTargetsAnalysis::performMeet(BitVector from, BitVector& to) {
  return performUnion(from, to);
}

// return the union of from and to
bool FPTargetsAnalysis::performUnion(BitVector from, BitVector& to) {
  //static int counter = 0;
  BitVector oldTo = to;
  to |= from;
  return to != oldTo;
}

FunctionSet FPTargetsAnalysis::getTargets(Value* FP, Context* C) {
  BitVector& vector = state[C][FP];
  return convertBitVectorToFunctionSet(vector);
}

FunctionSet FPTargetsAnalysis::convertBitVectorToFunctionSet(BitVector vector) {
  FunctionSet functions;
  int idx = 0;
  for (int i=0; i<vector.count(); i++) {
    idx = (i == 0) ? vector.find_first() : vector.find_next(idx);
    functions.insert(idxToFunc[idx]);
  }
  return functions;
}

BitVector FPTargetsAnalysis::convertFunctionSetToBitVector(FunctionSet funcs) {
  BitVector vector;
  for (Function* F : funcs) {
    setBitVector(vector, F);
  }
  return vector;
}

void FPTargetsAnalysis::setBitVector(BitVector& vector, Function* F) {
  int idx = funcToIdx[F];
  if (vector.size() <= idx) {
    vector.resize(idx+1);
  }
  vector.set(idx);
}

string FPTargetsAnalysis::stringifyFact(BitVector fact) {
  FunctionSet funcs = convertBitVectorToFunctionSet(fact);
  return CallGraphUtils::stringifyFunctionSet(funcs);
}

void FPTargetsAnalysis::stateChangedForFunctionPointer(CallInst* CI, const Value* FP, Context* C, BitVector& newState) {
  // Filter out those callees that aren't compatible with FP's function type
  SDEBUG("soaap.analysis.infoflow.fp", 3, dbgs() << "bits set (before): " << newState.count() << "\n");
  FunctionType* FT = NULL;
  if (PointerType* PT = dyn_cast<PointerType>(FP->getType())) {
    if (PointerType* PT2 = dyn_cast<PointerType>(PT->getElementType())) {
      FT = dyn_cast<FunctionType>(PT2->getElementType());
    }
    else {
      FT = dyn_cast<FunctionType>(PT->getElementType());
    }
  }
  
  if (FT != NULL) {
    int idx;
    int numSetBits = newState.count();
    for (int i=0; i<numSetBits; i++) {
      idx = (i == 0) ? newState.find_first() : newState.find_next(idx);
      Function* F = idxToFunc[idx];
      SDEBUG("soaap.analysis.infoflow.fp", 3, dbgs() << "F: " << F->getName() << "\n");
      SDEBUG("soaap.analysis.infoflow.fp", 3, dbgs() << "FT: " << *FT << "\n");
      if (!areTypeCompatible(F->getFunctionType(), FT) || F->isDeclaration()) {
        newState.reset(idx);
        if (F->isDeclaration()) {
          SDEBUG("soaap.analysis.infoflow.fp", 3, dbgs() << "Declaration\n");
        }
        else {
          FunctionType* FT2 = F->getFunctionType();
          SDEBUG("soaap.analysis.infoflow.fp", 3, dbgs() << "Function types don't match: " << *FT2 << "\n");
          SDEBUG("soaap.analysis.infoflow.fp", 3, dbgs() << "FT2.return: " << *(FT2->getReturnType()) << "\n");
          SDEBUG("soaap.analysis.infoflow.fp", 3, dbgs() << "FT2.params: " << FT2->getNumParams() << "\n");
          SDEBUG("soaap.analysis.infoflow.fp", 3, dbgs() << "FT2.varargs: " << FT2->isVarArg() << "\n");
          SDEBUG("soaap.analysis.infoflow.fp", 3, dbgs() << "FT.vararg: " << FT->isVarArg() << "\n");
        }
      }
    }
  }
  else {
    dbgs() << "Unrecognised FP: " << *FP->getType() << "\n";
  }
  SDEBUG("soaap.analysis.infoflow.fp", 3, dbgs() << "bits set (after): " << newState.count() << "\n");
  FunctionSet newFuncs = convertBitVectorToFunctionSet(newState);
  CallGraphUtils::addCallees(CI, C, newFuncs);
}

bool FPTargetsAnalysis::areTypeCompatible(FunctionType* FT1, FunctionType* FT2) {
  return FT1 == FT2 || (FT1->getReturnType() == FT2->getReturnType() && FT1->getNumParams() == 0 && FT2->getNumParams() == 0);
}
