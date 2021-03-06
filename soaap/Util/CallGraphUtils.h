#ifndef SOAAP_UTILS_CALLGRAPHUTILS_H
#define SOAAP_UTILS_CALLGRAPHUTILS_H

#include "llvm/IR/Module.h"
#include "llvm/Support/GraphWriter.h"

#include "Common/Sandbox.h"
#include "Common/Typedefs.h"

using namespace llvm;

namespace soaap {
  class FPTargetsAnalysis;
  class CallGraphUtils {
    public:
      static void buildBasicCallGraph(Module& M, SandboxVector& sandboxes);
      static void loadAnnotatedInferredCallGraphEdges(Module& M, SandboxVector& sandboxes);
      static void listFPCalls(Module& M, SandboxVector& sandboxes);
      static void listFPTargets(Module& M, SandboxVector& sandboxes);
      static void listAllFuncs(Module& M);
      static bool isIndirectCall(CallInst* C);
      static Function* getDirectCallee(CallInst* C);
      static FunctionSet getCallees(const CallInst* C, Context* Ctx, Module& M);
      static FunctionSet getCallees(const Function* F, Context* Ctx, Module& M);
      static set<CallGraphEdge> getCallGraphEdges(const Function* F, Context* Ctx, Module& M);
      static CallInstSet getCallers(const Function* F, Context* Ctx, Module& M);
      static bool isExternCall(CallInst* C);
      static void addCallees(CallInst* C, Context* Ctx, FunctionSet& callees);
      static string stringifyFunctionSet(FunctionSet& funcs);
      static void dumpDOTGraph();
      static InstTrace findPrivilegedPathToFunction(Function* Target, Module& M);
      static InstTrace findSandboxedPathToFunction(Function* Target, Sandbox* S, Module& M);
      static bool isReachableFrom(Function* Source, Function* Dest, Sandbox* Ctx, Module& M);
      /**
       * emits a call trace to @p Target for the given sandbox @p S.
       * If @p S is null then a privileged call graph will be emitted instead.
       */
      static void emitCallTrace(Function* Target, Sandbox* S, Module& M);
    private:
      static map<const CallInst*, map<Context*, FunctionSet> > callToCallees;
      static map<const Function*, map<Context*, FunctionSet> > funcToCallees;
      static map<const Function*, map<Context*, set<CallGraphEdge> > > funcToCallEdges;
      static map<const Function*, map<Context*, CallInstSet> > calleeToCalls;
      static map<Function*, map<Function*,InstTrace> > funcToShortestCallPaths; //TODO: check
      static bool caching;
      static void populateCallCalleeCaches(Module& M);
      static void calculateShortestCallPathsFromFunc(Function* F, bool privileged, Sandbox* S, Module& M);
      static bool isReachableFromHelper(Function* Source, Function* Curr, Function* Dest, Sandbox* Ctx, set<Function*>& visited, Module& M);
      static FPTargetsAnalysis& getFPAnnotatedTargetsAnalysis();
      static FPTargetsAnalysis& getFPInferredTargetsAnalysis();
      
  };
}
namespace llvm {
  template<>
  struct DOTGraphTraits<CallGraph*> : public DefaultDOTGraphTraits {
    DOTGraphTraits (bool isSimple=false) : DefaultDOTGraphTraits(isSimple) {}
  };
}
#endif
