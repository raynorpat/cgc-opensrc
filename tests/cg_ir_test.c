/****************************************************************************\
Copyright (c) 2002, NVIDIA Corporation.

NVIDIA Corporation("NVIDIA") supplies this software to you in
consideration of your agreement to the following terms, and your use,
installation, modification or redistribution of this NVIDIA software
constitutes acceptance of these terms.  If you do not agree with these
terms, please do not use, install, modify or redistribute this NVIDIA
software.

In consideration of your agreement to abide by the following terms, and
subject to these terms, NVIDIA grants you a personal, non-exclusive
license, under NVIDIA's copyrights in this original NVIDIA software (the
"NVIDIA Software"), to use, reproduce, modify and redistribute the
NVIDIA Software, with or without modifications, in source and/or binary
forms; provided that if you redistribute the NVIDIA Software, you must
retain the copyright notice of NVIDIA, this notice and the following
text and disclaimers in all such redistributions of the NVIDIA Software.
Neither the name, trademarks, service marks nor logos of NVIDIA
Corporation may be used to endorse or promote products derived from the
NVIDIA Software without specific prior written permission from NVIDIA.
Except as expressly stated in this notice, no other rights or licenses
express or implied, are granted by NVIDIA herein, including but not
limited to any patent rights that may be infringed by your derivative
works or by other works in which the NVIDIA Software may be
incorporated. No hardware is licensed hereunder.

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OR CONDITIONS OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL,
INDIRECT, INCIDENTAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

//
// cg_ir_test.c - Unit tests for the backend-neutral Cg IR core: module
//        ownership through counting, dirty, budgeted, and failing
//        allocators; stable expression and statement kinds; typed
//        expression builders; structured statement builders; and
//        failed-module semantics that never hand out partially
//        initialized nodes.
//

#if defined(NDEBUG)
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slglobals.h"
#include "cg_stdlib.h"
#include "cg_ir.h"
#include "cg_reach.h"
#include "cg_ir_lower.h"

CgStruct *Cg;
Scope *CurrentScope;

/*
 * TestRegisterNames() - Stub HAL name registration for InitSymbolTable.
 */

static int TestRegisterNames(slHAL *fHAL)
{
    (void) fHAL;
    return 1;
} // TestRegisterNames

/*
 * TestGetSizeof() - Minimal size query mirroring GetSizeof_HAL.
 */

static int TestGetSizeof(Type *fType)
{
    if (!fType)
        return 0;
    return fType->co.size;
} // TestGetSizeof

void SemanticError(SourceLoc *loc, int num, const char *mess, ...)
{
    (void) loc;
    (void) num;
    (void) mess;
}

void InternalError(SourceLoc *loc, int num, const char *mess, ...)
{
    (void) loc;
    (void) num;
    (void) mess;
}

dtype CurrentDeclTypeSpecs = { 0, };

Symbol *DefineTypedef(SourceLoc *loc, Scope *fScope, int atom, Type *fType)
{
    (void) loc;
    (void) fScope;
    (void) atom;
    (void) fType;
    return NULL;
}

////////////////////////////////// Allocators //////////////////////////////////

static int allocationCount;
static int allocationBudget;

/* Normal allocator: plain uninitialized malloc. */

static void *PlainAlloc(void *arg, size_t size)
{
    (void) arg;
    return malloc(size);
} // PlainAlloc

/* Zeroed allocator: every byte starts at zero. */

static void *TestAlloc(void *arg, size_t size)
{
    (void) arg;
    return calloc(1, size);
} // TestAlloc

static void *CountingAlloc(void *arg, size_t size)
{
    (void) arg;
    allocationCount++;
    return calloc(1, size);
} // CountingAlloc

static void *DirtyAlloc(void *arg, size_t size)
{
    void *memory;

    (void) arg;
    memory = malloc(size);
    memset(memory, 0xa5, size);
    return memory;
} // DirtyAlloc

/*
 * BudgetAlloc() - Succeeds until the budget runs out, then fails like
 *          FailingAlloc; every success is dirty-filled so skipped
 *          initialization shows up immediately.
 */

static void *BudgetAlloc(void *arg, size_t size)
{
    void *memory;

    (void) arg;
    allocationBudget--;
    if (allocationBudget < 0)
        return NULL;
    memory = malloc(size);
    memset(memory, 0xa5, size);
    return memory;
} // BudgetAlloc

static void *FailingAlloc(void *arg, size_t size)
{
    (void) arg;
    (void) size;
    return NULL;
} // FailingAlloc

/////////////////////////////////// Fixtures ///////////////////////////////////

/*
 * lDotSignature - Stand-in immutable catalog signature.  The intrinsic
 *          builder only stores the opcode plus the pointer, so a local
 *          signature keeps this unit independent of catalog expansion.
 */

static const CgIntrinsicSignature lDotSignature = {
    CG_INTRINSIC_DOT, "dot", NULL, NULL,
    CG_INTRINSIC_PURE | CG_INTRINSIC_FOLDABLE
};

/*
 * lMakeSymbol() - Minimal standalone symbol carrying a name atom, a
 *          kind, and a canonical type.
 */

static Symbol *lMakeSymbol(symbolkind kind, const char *name, Type *fType)
{
    Symbol *lSymb;

    lSymb = (Symbol *) calloc(1, sizeof(Symbol));
    assert(lSymb != NULL);
    lSymb->name = LookUpAddString(atable, name);
    lSymb->kind = kind;
    lSymb->type = fType;
    return lSymb;
} // lMakeSymbol

///////////////////////////////// Verification /////////////////////////////////

/*
 * lVerifyAccept() - "module" must verify: the diagnostic comes back
 *          zeroed (reason OK, no node, empty location) even when the
 *          caller hands in garbage, and a NULL diagnostic pointer is
 *          tolerated.
 */

static void lVerifyAccept(CgIRModule *module)
{
    CgIRVerifyDiagnostic diagnostic;

    memset(&diagnostic, 0xa5, sizeof(diagnostic));
    assert(CgIRVerifyModule(module, &diagnostic));
    assert(diagnostic.reason == CGIR_VERIFY_OK);
    assert(diagnostic.loc.file == 0 && diagnostic.loc.line == 0);
    assert(diagnostic.node == NULL);
    assert(CgIRVerifyModule(module, NULL));
} // lVerifyAccept

/*
 * lVerifyReject() - "module" must fail verification reporting exactly
 *          "reason" about a concrete failing node.
 */

static void lVerifyReject(CgIRModule *module, CgIRVerifyReason reason)
{
    CgIRVerifyDiagnostic diagnostic;

    memset(&diagnostic, 0, sizeof(diagnostic));
    assert(!CgIRVerifyModule(module, &diagnostic));
    assert(diagnostic.reason == reason);
    assert(diagnostic.node != NULL);
} // lVerifyReject

///////////////////////////////// Reach graph //////////////////////////////////

/*
 * Hand-built frontend nodes: cg_ir_unit does not link the parser's node
 * constructors, so the reach scenario shapes minimal expr/stmt trees
 * straight from support.h's unions.  Only the fields CgReachBuild reads
 * (kind, op, left/right, symbol, statements) are populated.
 */

static expr *lReachSymbNode(Symbol *fSymb)
{
    symb *node;

    node = (symb *) calloc(1, sizeof(symb));
    assert(node != NULL);
    node->kind = SYMB_N;
    node->op = VARIABLE_OP;
    node->type = fSymb->type;
    node->symbol = fSymb;
    return (expr *) node;
}

static expr *lReachArgNode(expr *fActual, expr *fRest)
{
    binary *node;

    node = (binary *) calloc(1, sizeof(binary));
    assert(node != NULL);
    node->kind = BINARY_N;
    node->op = FUN_ARG_OP;
    node->left = fActual;
    node->right = fRest;
    return (expr *) node;
}

static expr *lReachCallNode(Symbol *fCallee, expr *fArgs)
{
    binary *node;

    node = (binary *) calloc(1, sizeof(binary));
    assert(node != NULL);
    node->kind = BINARY_N;
    node->op = FUN_CALL_OP;
    node->left = lReachSymbNode(fCallee);
    node->right = fArgs;
    return (expr *) node;
}

static stmt *lReachExprStmtNode(expr *fExp)
{
    expr_stmt *node;

    node = (expr_stmt *) calloc(1, sizeof(expr_stmt));
    assert(node != NULL);
    node->kind = EXPR_STMT;
    node->exp = fExp;
    return (stmt *) node;
}

static stmt *lReachBlockStmtNode(stmt *fBody)
{
    block_stmt *node;

    node = (block_stmt *) calloc(1, sizeof(block_stmt));
    assert(node != NULL);
    node->kind = BLOCK_STMT;
    node->body = fBody;
    return (stmt *) node;
}

/*
 * lReachFunction() - A defined function symbol whose body is one call
 *          statement; "fSourceOrdinal" fixes the deterministic visit
 *          order.
 */

static Symbol *lReachFunction(const char *name, int fSourceOrdinal,
                              expr *call)
{
    Symbol *symb;

    symb = lMakeSymbol(FUNCTION_S, name, VoidType);
    symb->sourceOrdinal = fSourceOrdinal;
    if (call != NULL) {
        symb->details.fun.statements =
            lReachBlockStmtNode(lReachExprStmtNode(call));
    }
    return symb;
} // lReachFunction

/*
 * lGroupSelectorNode() - A `_m` group selector over "fObject"; the
 *          legacy packing carries the selected components in MASK16
 *          nibbles (row<<2|col each) with their count in T2.
 */

static expr *lGroupSelectorNode(expr *fObject, int fMask16, int fCount,
                                Type *fType)
{
    unary *node;

    node = (unary *) calloc(1, sizeof(unary));
    assert(node != NULL);
    node->kind = UNARY_N;
    node->op = SWIZMAT_Z_OP;
    node->type = fType;
    node->subop = SUBOP_ZM(fMask16, fCount, 0, 0, 0);
    node->arg = fObject;
    return (expr *) node;
}

/*
 * lAssignNode() - One plain assignment expression.
 */

static expr *lAssignNode(expr *fLeft, expr *fRight, Type *fType)
{
    binary *node;

    node = (binary *) calloc(1, sizeof(binary));
    assert(node != NULL);
    node->kind = BINARY_N;
    node->op = ASSIGN_OP;
    node->type = fType;
    node->left = fLeft;
    node->right = fRight;
    return (expr *) node;
}

/* ============================ Geometry fixtures ============================ */

/*
 * CgGeoFixture - One complete geometry module plus handles onto every
 *          node the geometry scenarios poke.  "emitValues" and
 *          "flatValues" point into the STATEMENT-OWNED deep copies, so
 *          malformed-case mutations reach what the verifier walks;
 *          "callerValues"/"callerSecond" keep the builder's input list
 *          for ownership assertions.
 */

typedef struct CgGeoFixture_Rec {
    CgIRModule module;
    CgIRFunction *entryFn;
    CgIRFunction *flatFn;
    CgIRDecl *arrayFormal;
    CgIRDecl *globalUv;
    CgIRDecl *globalColor;
    CgIRExpr *posIndex;
    CgIRExpr *uvRef;
    CgIRExpr *colorRef;
    CgIRStmt *callFlat;
    CgIRStmt *emit;
    CgIRStmt *restart;
    CgIRGeometryValue *callerValues;
    CgIRGeometryValue *callerSecond;
    CgIRGeometryValue *emitValues;
    CgIRGeometryValue *flatValues;
    int atomPosition;
    int atomTexcoord0;
    int atomTexCoordSource;
    int atomColor0;
    int atomInstanceid;
    int atomLayer;
    SourceLoc fnLoc;
    SourceLoc infoInputLoc;
    SourceLoc infoOutputLoc;
    SourceLoc infoMaxLoc;
    SourceLoc opLoc;
    SourceLoc valueLoc;
} CgGeoFixture;

/*
 * lBuildGeometryFixture() - A verified-clean geometry module: triangle
 *          input, triangle-strip output, maximum six, one resolved
 *          AttribArray<float4,3> entry formal, an emit bundle carrying
 *          POSITION then TEXCOORD0, a flat COLOR0 held by a REACHABLE
 *          helper the entry calls, and one restart.
 *          "fWithArrayFormal" drops the attribute-array formal for the
 *          operation-placement cases that would otherwise trip the
 *          array-placement invariant first.
 */

static void lBuildGeometryFixture(CgGeoFixture *fx,
                                  void *(*alloc)(void *, size_t),
                                  int fWithArrayFormal)
{
    CgIRGeometryInfo info;
    Type *attribType;
    Type *float4Local;
    Type *float2Type;
    Type *intLocal;
    CgNumericValue vIndexZero;
    Symbol *entrySymb;
    Symbol *flatSymb;
    Symbol *arraySymb;
    Symbol *uvSymb;
    Symbol *colorSymb;
    CgIRExpr *arrayRef;
    CgIRExpr *indexConst;
    CgIRGeometryValue *vPos;
    CgIRGeometryValue *vUv;
    CgIRGeometryValue *vCol;
    CgIRExpr *flatCall;
    CgIRStmt *entryBody;
    CgIRStmt *flatBody;
    SourceLoc declLoc;

    memset(fx, 0, sizeof(*fx));
    float4Local = GetStandardTypeKind(CG_SCALAR_FLOAT, 4, 0);
    float2Type = GetStandardTypeKind(CG_SCALAR_FLOAT, 2, 0);
    intLocal = GetStandardTypeKind(CG_SCALAR_INT, 0, 0);
    attribType = CgGetAttribArrayType(float4Local, 3);
    assert(float4Local != UndefinedType);
    assert(float2Type != UndefinedType);
    assert(intLocal != UndefinedType);
    assert(attribType != UndefinedType);
    memset(&vIndexZero, 0, sizeof(vIndexZero));
    vIndexZero.kind = CG_SCALAR_INT;

    fx->atomPosition = LookUpAddString(atable, "POSITION");
    fx->atomTexcoord0 = LookUpAddString(atable, "TEXCOORD0");
    fx->atomTexCoordSource = LookUpAddString(atable, "texcoord0");
    fx->atomColor0 = LookUpAddString(atable, "COLOR0");
    fx->atomInstanceid = LookUpAddString(atable, "INSTANCEID");
    fx->atomLayer = LookUpAddString(atable, "LAYER");

    memset(&fx->infoInputLoc, 0, sizeof(fx->infoInputLoc));
    fx->infoInputLoc.file = 7; fx->infoInputLoc.line = 10;
    memset(&fx->infoOutputLoc, 0, sizeof(fx->infoOutputLoc));
    fx->infoOutputLoc.file = 7; fx->infoOutputLoc.line = 11;
    memset(&fx->infoMaxLoc, 0, sizeof(fx->infoMaxLoc));
    fx->infoMaxLoc.file = 7; fx->infoMaxLoc.line = 12;
    memset(&fx->opLoc, 0, sizeof(fx->opLoc));
    fx->opLoc.file = 8; fx->opLoc.line = 20;
    memset(&fx->valueLoc, 0, sizeof(fx->valueLoc));
    fx->valueLoc.file = 8; fx->valueLoc.line = 21;
    memset(&declLoc, 0, sizeof(declLoc));
    declLoc.file = 6; declLoc.line = 5;
    fx->fnLoc = declLoc;

    CgIRInitModule(&fx->module, alloc, NULL);
    assert(CgIRSetStage(&fx->module, CGIR_STAGE_GEOMETRY));

    memset(&info, 0, sizeof(info));
    info.inputTopology = CG_GEOMETRY_INPUT_TRIANGLE;
    info.outputTopology = CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP;
    info.inputVertexCount = 3;
    info.maxOutputVertices = 6;
    info.hasMaxOutputVertices = 1;
    info.inputLoc = fx->infoInputLoc;
    info.outputLoc = fx->infoOutputLoc;
    info.maxVerticesLoc = fx->infoMaxLoc;
    assert(CgIRSetGeometryInfo(&fx->module, &info));

    uvSymb = lMakeSymbol(VARIABLE_S, "g_uv", float2Type);
    colorSymb = lMakeSymbol(VARIABLE_S, "g_color", float4Local);
    entrySymb = lMakeSymbol(FUNCTION_S, "geoMain", VoidType);
    flatSymb = lMakeSymbol(FUNCTION_S, "geoFlat", VoidType);
    arraySymb = lMakeSymbol(VARIABLE_S, "tri", attribType);

    fx->globalUv = CgIRNewDecl(&fx->module, uvSymb, uvSymb->name,
                               float2Type, CGIR_STORAGE_UNIFORM,
                               CGIR_DOMAIN_UNIFORM, 0, NULL, &declLoc);
    fx->globalColor = CgIRNewDecl(&fx->module, colorSymb, colorSymb->name,
                                  float4Local, CGIR_STORAGE_UNIFORM,
                                  CGIR_DOMAIN_UNIFORM, 0, NULL, &declLoc);
    assert(fx->globalUv != NULL && fx->globalColor != NULL);
    CgIRAppendDecl(&fx->module.globals, fx->globalUv);
    CgIRAppendDecl(&fx->module.globals, fx->globalColor);

    fx->entryFn = CgIRNewFunction(&fx->module, entrySymb, VoidType,
                                  &declLoc);
    fx->flatFn = CgIRNewFunction(&fx->module, flatSymb, VoidType,
                                 &declLoc);
    assert(fx->entryFn != NULL && fx->flatFn != NULL);

    if (fWithArrayFormal) {
        fx->arrayFormal = CgIRNewDecl(&fx->module, arraySymb,
                                      arraySymb->name, attribType,
                                      CGIR_STORAGE_NONE, CGIR_DOMAIN_NONE,
                                      0, NULL, &declLoc);
        assert(fx->arrayFormal != NULL);
        CgIRAppendDecl(&fx->entryFn->parameters, fx->arrayFormal);
        arrayRef = CgIRNewSymbol(&fx->module, attribType, &fx->valueLoc,
                                 arraySymb);
        assert(arrayRef != NULL);
        indexConst = CgIRNewConstant(&fx->module, intLocal, &fx->valueLoc,
                                     &vIndexZero);
        assert(indexConst != NULL);
        fx->posIndex = CgIRNewIndex(&fx->module, float4Local,
                                    &fx->valueLoc, arrayRef, indexConst);
    } else {
        fx->posIndex = CgIRNewSymbol(&fx->module, float4Local,
                                     &fx->valueLoc, colorSymb);
    }
    assert(fx->posIndex != NULL);
    fx->uvRef = CgIRNewSymbol(&fx->module, float2Type, &fx->valueLoc,
                              uvSymb);
    fx->colorRef = CgIRNewSymbol(&fx->module, float4Local, &fx->valueLoc,
                                 colorSymb);
    assert(fx->uvRef != NULL && fx->colorRef != NULL);

    /* Caller-side bundle lists: the builders must deep-copy these. */
    vPos = CgIRNewGeometryValue(&fx->module, fx->atomPosition,
                                fx->atomPosition, float4Local,
                                fx->posIndex, fx->valueLoc);
    vUv = CgIRNewGeometryValue(&fx->module, fx->atomTexcoord0,
                               fx->atomTexCoordSource, float2Type,
                               fx->uvRef, fx->valueLoc);
    vCol = CgIRNewGeometryValue(&fx->module, fx->atomColor0,
                                fx->atomColor0, float4Local,
                                fx->colorRef, fx->valueLoc);
    assert(vPos != NULL && vUv != NULL && vCol != NULL);
    vPos->next = vUv;
    fx->callerValues = vPos;
    fx->callerSecond = vUv;

    fx->emit = CgIRNewGeometryEmit(&fx->module, vPos, fx->opLoc);
    fx->restart = CgIRNewGeometryRestart(&fx->module, fx->opLoc);
    assert(fx->emit != NULL && fx->restart != NULL);
    fx->emitValues = fx->emit->u.geometry.values;
    assert(fx->emitValues != NULL);

    flatCall = CgIRNewCall(&fx->module, VoidType, &fx->opLoc, flatSymb,
                           NULL);
    assert(flatCall != NULL);
    fx->callFlat = CgIRNewExprStmt(&fx->module, &fx->opLoc, flatCall);
    assert(fx->callFlat != NULL);

    /* Flat operation lives in the reachable helper. */
    {
        CgIRStmt *flatStmt;

        flatStmt = CgIRNewGeometryFlat(&fx->module, vCol, fx->opLoc);
        assert(flatStmt != NULL);
        fx->flatValues = flatStmt->u.geometry.values;
        assert(fx->flatValues != NULL);
        flatBody = CgIRNewBlockStmt(&fx->module, &fx->opLoc);
        assert(flatBody != NULL);
        CgIRAppendStmt(&flatBody->u.block, flatStmt);
        fx->flatFn->body = flatBody;
    }

    entryBody = CgIRNewBlockStmt(&fx->module, &fx->opLoc);
    assert(entryBody != NULL);
    CgIRAppendStmt(&entryBody->u.block, fx->callFlat);
    CgIRAppendStmt(&entryBody->u.block, fx->emit);
    CgIRAppendStmt(&entryBody->u.block, fx->restart);
    fx->entryFn->body = entryBody;

    fx->entryFn->isEntry = 1;
    fx->module.entry = fx->entryFn;
    CgIRAppendFunction(&fx->module.functions, fx->entryFn);
    CgIRAppendFunction(&fx->module.functions, fx->flatFn);
} /* lBuildGeometryFixture */

int main(int argc, char **argv)
{
    CgStruct cg;
    slHAL hal;
    CgProfileIdentity genericIdentity;
    CgIRModule module;
    CgIRModule coverModule;
    CgIRModule dirtyModule;
    CgIRModule budgetModule;
    CgIRModule failModule;
    Type *floatType;
    Type *float2Type;
    Type *float4Type;
    Type *intType;
    Type *boolType;
    Symbol *shadeSymb;
    Symbol *mainSymb;
    Symbol *positionSymb;
    Symbol *localSymb;
    Symbol *memberSymb;
    Symbol *methodSymb;
    Symbol *calleeSymb;
    CgNumericValue vZero;
    CgNumericValue vOne;
    CgNumericValue vHalf;
    CgNumericValue vTrue;
    CgNumericValue vIntZero;
    SourceLoc fnALoc;
    SourceLoc fnBLoc;
    SourceLoc paramLoc;
    SourceLoc constLoc;
    SourceLoc ctorLoc;
    SourceLoc retLoc;
    SourceLoc blockLoc;
    CgIRFunction *shadeFn;
    CgIRFunction *mainFn;
    CgIRDecl *param;
    CgIRExpr *args;
    CgIRExpr *constX0;
    CgIRExpr *constX1;
    CgIRExpr *constY0;
    CgIRExpr *constY1;
    CgIRExpr *ctorExpr;
    CgIRStmt *retStmt;
    CgIRStmt *bodyBlock;
    CgIRFunction *coverFn;
    CgIRDecl *localDecl;
    CgIRExpr *varRef;
    CgIRExpr *swizExpr;
    CgIRExpr *idxConst;
    CgIRExpr *indexExpr;
    CgIRExpr *memberExpr;
    CgIRExpr *lenExpr;
    CgIRExpr *halfConst;
    CgIRExpr *castExpr;
    CgIRExpr *unaryExpr;
    CgIRExpr *binaryExpr;
    CgIRExpr *boolConst;
    CgIRExpr *condExpr;
    CgIRExpr *assignExpr;
    CgIRExpr *callArgs;
    CgIRExpr *callExpr;
    CgIRExpr *dotArgs;
    CgIRExpr *intrinsicExpr;
    CgIRExpr *interfaceExpr;
    CgIRExpr *stepExpr;
    CgIRStmt *bodyList;
    CgIRStmt *declStmt;
    CgIRStmt *exprStmt;
    CgIRStmt *innerBlock;
    CgIRStmt *breakStmt;
    CgIRStmt *continueStmt;
    CgIRStmt *whileStmt;
    CgIRStmt *doStmt;
    CgIRStmt *forInit;
    CgIRStmt *forStmt;
    CgIRStmt *discardStmt;
    CgIRStmt *ifStmt;
    CgIRFunction *dFn;
    CgIRFunction *dFn2;
    CgIRDecl *dDecl;
    CgIRExpr *dExpr;
    CgIRStmt *dStmt;
    CgIRStmt *dBlock;
    CgIRFunction *budgetFn;
    CgIRDecl *budgetDecl;
    CgIRExpr *budgetC0;
    CgIRExpr *budgetC1;
    CgIRExpr *stickyExpr;

    /* Scenario 5: module verification fixtures. */
    CgIRModule rejectModule;
    CgIRModule verifyModule;
    CgIRVerifyDiagnostic verifyDiagnostic;
    Type ifaceAType;
    Type ifaceBType;
    Type qualifiedOutFloat4;
    TypeList dotFFFirst;
    TypeList dotFFLast;
    CgIntrinsicSignature dotFFSignature;
    Type *sampler2DType;
    Symbol *helperSymb;
    Symbol *outFormalSymb;
    Symbol *inFormalSymb;
    Symbol *texSymb;
    Symbol *tmpSymb;
    Symbol *objSymb;
    Symbol *evalSymb;
    Symbol *otherEvalSymb;
    CgIRFunction *rFn;
    CgIRFunction *rCalleeFn;
    CgIRFunction *vHelperFn;
    CgIRFunction *vMainFn;
    CgIRDecl *rDecl;
    CgIRDecl *vInFormal;
    CgIRDecl *vOutFormal;
    CgIRDecl *vPosFormal;
    CgIRDecl *vTexDecl;
    CgIRDecl *vObjDecl;
    CgIRDecl *vTmpDecl;
    CgIRExpr *rArgs;
    CgIRExpr *rLeft;
    CgIRExpr *rRight;
    CgIRExpr *rTarget;
    CgIRExpr *rValue;
    CgIRExpr *vInit;
    CgIRExpr *vObjRef;
    CgIRExpr *vTmpRef;
    CgIRStmt *rBody;
    CgIRStmt *rInner;
    CgIRStmt *rStmt;
    Type prodFnType;
    TypeList prodParamFirst;
    TypeList prodParamLast;
    Symbol *prodEvalSymb;
    Symbol *prodReceiverSymb;
    Symbol *prodXFormalSymb;

    /* Verifier extension fixtures: connector members and first-class
     * array constructors. */
    Type connType;
    Scope connScope;
    Type array3Type;
    Symbol *connMemberSymb;
    Symbol *connVarSymb;

    /* Scenario 6: reachability graph fixtures. */
    Symbol *reachMainSymb;
    Symbol *reachASymb;
    Symbol *reachBSymb;
    Symbol *reachUnusedSymb;
    Symbol *reachTintSymb;
    Symbol *reachSpareSymb;
    Symbol *cycXSymb;
    Symbol *cycYSymb;
    CgReachGraph reachGraph;
    CgReachGraph cycleGraph;
    const CgReachEdge *reachEdge;

    /*
     * Assertion seam: check_assertions_active.cmake runs this unit with
     * --verify-assertions-active and requires the sentinel below.  The
     * spelling is the checker's fixed contract shared with glsl_ir_unit;
     * this translation unit is built with active assertions (#undef
     * NDEBUG above), while cg_ir_verify.c keeps NDEBUG in Release so a
     * rejected module returns its diagnostic instead of aborting.
     */

    if (argc == 2 && !strcmp(argv[1], "--verify-assertions-active")) {
        int assertionsActive;

        assertionsActive = 0;
        assert((assertionsActive = 1) != 0);
        if (!assertionsActive)
            return 2;
        puts("glsl-ir-assertions-active");
        return 0;
    }

    memset(&cg, 0, sizeof(cg));
    memset(&hal, 0, sizeof(hal));
    hal.GetSizeof = TestGetSizeof;
    hal.RegisterNames = TestRegisterNames;
    cg.theHAL = &hal;
    Cg = &cg;

    assert(InitAtomTable(atable, 0));
    assert(InitSymbolTable(Cg));
    assert(StartGlobalScope(Cg));
    assert(GlobalScope != NULL);

    floatType = GetStandardTypeKind(CG_SCALAR_FLOAT, 0, 0);
    float2Type = GetStandardTypeKind(CG_SCALAR_FLOAT, 2, 0);
    float4Type = GetStandardTypeKind(CG_SCALAR_FLOAT, 4, 0);
    intType = GetStandardTypeKind(CG_SCALAR_INT, 0, 0);
    boolType = GetStandardTypeKind(CG_SCALAR_BOOL, 0, 0);
    assert(floatType != UndefinedType);
    assert(float2Type != UndefinedType);
    assert(float4Type != UndefinedType);
    assert(intType != UndefinedType);
    assert(boolType != UndefinedType);

    memset(&vZero, 0, sizeof(vZero));
    vZero.kind = CG_SCALAR_FLOAT;
    memset(&vOne, 0, sizeof(vOne));
    vOne.kind = CG_SCALAR_FLOAT;
    vOne.value.f = 1.0;
    memset(&vHalf, 0, sizeof(vHalf));
    vHalf.kind = CG_SCALAR_FLOAT;
    vHalf.value.f = 0.5;
    memset(&vTrue, 0, sizeof(vTrue));
    vTrue.kind = CG_SCALAR_BOOL;
    vTrue.value.i = 1;
    memset(&vIntZero, 0, sizeof(vIntZero));
    vIntZero.kind = CG_SCALAR_INT;

    fnALoc.file = 3; fnALoc.line = 40;
    fnBLoc.file = 3; fnBLoc.line = 45;
    paramLoc.file = 4; paramLoc.line = 9;
    constLoc.file = 5; constLoc.line = 11;
    ctorLoc.file = 5; ctorLoc.line = 12;
    retLoc.file = 5; retLoc.line = 13;
    blockLoc.file = 5; blockLoc.line = 14;

    shadeSymb = lMakeSymbol(FUNCTION_S, "shade", float4Type);
    mainSymb = lMakeSymbol(FUNCTION_S, "main", float4Type);
    positionSymb = lMakeSymbol(VARIABLE_S, "position", float4Type);

    /////////////////// Scenario 1: counting allocator //////////////////

    /* float4 main(float4 position : VARYING-domain formal) with a body
     * block holding one return of a float4 constructor built from four
     * float constants.  Exactly ten module allocations: two functions,
     * one declaration, five expressions, two statements. */

    allocationCount = 0;
    CgIRInitModule(&module, CountingAlloc, NULL);
    assert(!CgIRModuleFailed(&module));

    memset(&genericIdentity, 0, sizeof(genericIdentity));
    genericIdentity.stage = CG_PROFILE_STAGE_NEUTRAL;
    module.profile = &genericIdentity;

    shadeFn = CgIRNewFunction(&module, shadeSymb, float4Type, &fnALoc);
    assert(shadeFn != NULL);
    mainFn = CgIRNewFunction(&module, mainSymb, float4Type, &fnBLoc);
    assert(mainFn != NULL);

    param = CgIRNewDecl(&module, positionSymb, positionSymb->name,
                        float4Type, CGIR_STORAGE_NONE, CGIR_DOMAIN_VARYING,
                        0, NULL, &paramLoc);
    assert(param != NULL);
    CgIRAppendDecl(&mainFn->parameters, param);

    args = NULL;
    constX0 = CgIRNewConstant(&module, floatType, &constLoc, &vZero);
    constX1 = CgIRNewConstant(&module, floatType, &constLoc, &vZero);
    constY0 = CgIRNewConstant(&module, floatType, &constLoc, &vZero);
    constY1 = CgIRNewConstant(&module, floatType, &constLoc, &vOne);
    assert(constX0 != NULL && constX1 != NULL);
    assert(constY0 != NULL && constY1 != NULL);
    CgIRAppendExpr(&args, constX0);
    CgIRAppendExpr(&args, constX1);
    CgIRAppendExpr(&args, constY0);
    CgIRAppendExpr(&args, constY1);
    ctorExpr = CgIRNewConstruct(&module, float4Type, &ctorLoc, args);
    assert(ctorExpr != NULL);

    retStmt = CgIRNewReturnStmt(&module, &retLoc, ctorExpr);
    assert(retStmt != NULL);
    bodyBlock = CgIRNewBlockStmt(&module, &blockLoc);
    assert(bodyBlock != NULL);
    CgIRAppendStmt(&bodyBlock->u.block, retStmt);

    mainFn->body = bodyBlock;
    mainFn->isEntry = 1;
    module.entry = mainFn;
    CgIRAppendFunction(&module.functions, shadeFn);
    CgIRAppendFunction(&module.functions, mainFn);

    /* Exact allocation count: one allocation per node, none per link. */
    assert(allocationCount == 10);
    assert(!CgIRModuleFailed(&module));
    assert(module.globals == NULL);
    assert(module.profile == &genericIdentity);

    /* Declaration order: shade first, entry second, nothing trailing. */
    assert(module.functions == shadeFn);
    assert(shadeFn->next == mainFn);
    assert(mainFn->next == NULL);
    assert(module.entry == mainFn);

    assert(shadeFn->symbol == shadeSymb);
    assert(shadeFn->resultType == float4Type);
    assert(shadeFn->loc.file == fnALoc.file);
    assert(shadeFn->loc.line == fnALoc.line);
    assert(shadeFn->isEntry == 0);
    assert(shadeFn->parameters == NULL);
    assert(shadeFn->locals == NULL);
    assert(shadeFn->body == NULL);

    assert(mainFn->symbol == mainSymb);
    assert(mainFn->resultType == float4Type);
    assert(mainFn->loc.file == fnBLoc.file);
    assert(mainFn->loc.line == fnBLoc.line);
    assert(mainFn->isEntry == 1);
    assert(mainFn->locals == NULL);
    assert(mainFn->body == bodyBlock);

    assert(mainFn->parameters == param);
    assert(param->next == NULL);
    assert(param->symbol == positionSymb);
    assert(param->name == positionSymb->name);
    assert(param->type == float4Type);
    assert(param->storage == CGIR_STORAGE_NONE);
    assert(param->domain == CGIR_DOMAIN_VARYING);
    assert(param->semantic == 0);
    assert(param->initializer == NULL);
    assert(param->loc.file == paramLoc.file);
    assert(param->loc.line == paramLoc.line);

    assert(bodyBlock->kind == CGIR_STMT_BLOCK);
    assert(bodyBlock->loc.file == blockLoc.file);
    assert(bodyBlock->loc.line == blockLoc.line);
    assert(bodyBlock->synthesized == 0);
    assert(bodyBlock->next == NULL);
    assert(bodyBlock->u.block == retStmt);

    assert(retStmt->kind == CGIR_STMT_RETURN);
    assert(retStmt->loc.file == retLoc.file);
    assert(retStmt->loc.line == retLoc.line);
    assert(retStmt->next == NULL);
    assert(retStmt->u.returnExpr == ctorExpr);

    assert(ctorExpr->kind == CGIR_EXPR_CONSTRUCT);
    assert(ctorExpr->type == float4Type);
    assert(ctorExpr->loc.file == ctorLoc.file);
    assert(ctorExpr->loc.line == ctorLoc.line);
    assert(ctorExpr->synthesized == 0);
    assert(ctorExpr->isLvalue == 0);
    assert(ctorExpr->sideEffects == 0);
    assert(ctorExpr->u.construct.arguments == constX0);
    assert(constX0->next == constX1);
    assert(constX1->next == constY0);
    assert(constY0->next == constY1);
    assert(constY1->next == NULL);
    assert(constX0->kind == CGIR_EXPR_CONSTANT);
    assert(constX0->type == floatType);
    assert(constX0->loc.file == constLoc.file);
    assert(constX0->loc.line == constLoc.line);
    assert(constX0->u.constant.kind == CG_SCALAR_FLOAT);
    assert(constX0->u.constant.value.f == 0.0);
    assert(constY1->kind == CGIR_EXPR_CONSTANT);
    assert(constY1->u.constant.kind == CG_SCALAR_FLOAT);
    assert(constY1->u.constant.value.f == 1.0);

    /////////////////// Scenario 2: full builder coverage //////////////////

    localSymb = lMakeSymbol(VARIABLE_S, "value", float4Type);
    memberSymb = lMakeSymbol(VARIABLE_S, "z", floatType);
    methodSymb = lMakeSymbol(FUNCTION_S, "eval", floatType);
    calleeSymb = lMakeSymbol(FUNCTION_S, "shade", float4Type);

    CgIRInitModule(&coverModule, PlainAlloc, NULL);
    coverFn = CgIRNewFunction(&coverModule, mainSymb, VoidType, &fnALoc);
    assert(coverFn != NULL);

    localDecl = CgIRNewDecl(&coverModule, localSymb, localSymb->name,
                            float4Type, CGIR_STORAGE_CONST,
                            CGIR_DOMAIN_NONE, 0, NULL, &paramLoc);
    assert(localDecl != NULL);
    CgIRAppendDecl(&coverFn->locals, localDecl);
    assert(coverFn->locals == localDecl);
    assert(localDecl->storage == CGIR_STORAGE_CONST);
    assert(localDecl->domain == CGIR_DOMAIN_NONE);

    varRef = CgIRNewSymbol(&coverModule, float4Type, &constLoc, localSymb);
    assert(varRef != NULL);
    assert(varRef->kind == CGIR_EXPR_SYMBOL);
    assert(varRef->type == float4Type);
    assert(varRef->u.symbol == localSymb);

    /* ".xz" encodes components 0 and 2 as two bits each: 0 | (2 << 2). */
    swizExpr = CgIRNewSwizzle(&coverModule, float2Type, &ctorLoc, varRef,
                              0x8, 2);
    assert(swizExpr != NULL);
    assert(swizExpr->kind == CGIR_EXPR_SWIZZLE);
    assert(swizExpr->type == float2Type);
    assert(swizExpr->u.swizzle.object == varRef);
    assert(swizExpr->u.swizzle.mask == 0x8);
    assert(swizExpr->u.swizzle.componentCount == 2);

    idxConst = CgIRNewConstant(&coverModule, intType, &constLoc, &vIntZero);
    assert(idxConst != NULL);
    assert(idxConst->u.constant.kind == CG_SCALAR_INT);
    indexExpr = CgIRNewIndex(&coverModule, floatType, &retLoc, swizExpr,
                             idxConst);
    assert(indexExpr != NULL);
    assert(indexExpr->kind == CGIR_EXPR_INDEX);
    assert(indexExpr->type == floatType);
    assert(indexExpr->u.index.object == swizExpr);
    assert(indexExpr->u.index.index == idxConst);

    memberExpr = CgIRNewMember(&coverModule, floatType, &blockLoc, varRef,
                               memberSymb);
    assert(memberExpr != NULL);
    assert(memberExpr->kind == CGIR_EXPR_MEMBER);
    assert(memberExpr->u.member.object == varRef);
    assert(memberExpr->u.member.member == memberSymb);

    lenExpr = CgIRNewLength(&coverModule, intType, &fnALoc, varRef);
    assert(lenExpr != NULL);
    assert(lenExpr->kind == CGIR_EXPR_LENGTH);
    assert(lenExpr->type == intType);
    assert(lenExpr->u.length.object == varRef);

    halfConst = CgIRNewConstant(&coverModule, floatType, &constLoc, &vHalf);
    assert(halfConst != NULL);

    castExpr = CgIRNewCast(&coverModule, intType, &fnBLoc, idxConst);
    assert(castExpr != NULL);
    assert(castExpr->kind == CGIR_EXPR_CAST);
    assert(castExpr->type == intType);
    assert(castExpr->u.cast.operand == idxConst);

    unaryExpr = CgIRNewUnary(&coverModule, floatType, &paramLoc,
                             CGIR_OP_NEGATE, halfConst);
    assert(unaryExpr != NULL);
    assert(unaryExpr->kind == CGIR_EXPR_UNARY);
    assert(unaryExpr->u.unary.op == CGIR_OP_NEGATE);
    assert(unaryExpr->u.unary.operand == halfConst);

    binaryExpr = CgIRNewBinary(&coverModule, floatType, &ctorLoc,
                               CGIR_OP_ADD, unaryExpr, castExpr);
    assert(binaryExpr != NULL);
    assert(binaryExpr->kind == CGIR_EXPR_BINARY);
    assert(binaryExpr->u.binary.op == CGIR_OP_ADD);
    assert(binaryExpr->u.binary.left == unaryExpr);
    assert(binaryExpr->u.binary.right == castExpr);

    boolConst = CgIRNewConstant(&coverModule, boolType, &constLoc, &vTrue);
    assert(boolConst != NULL);
    assert(boolConst->u.constant.kind == CG_SCALAR_BOOL);

    condExpr = CgIRNewConditional(&coverModule, floatType, &retLoc,
                                  boolConst, binaryExpr, halfConst);
    assert(condExpr != NULL);
    assert(condExpr->kind == CGIR_EXPR_CONDITIONAL);
    assert(condExpr->u.conditional.condition == boolConst);
    assert(condExpr->u.conditional.trueExpr == binaryExpr);
    assert(condExpr->u.conditional.falseExpr == halfConst);

    assignExpr = CgIRNewAssign(&coverModule, float4Type, &blockLoc,
                               CGIR_OP_ASSIGN, varRef, condExpr);
    assert(assignExpr != NULL);
    assert(assignExpr->kind == CGIR_EXPR_ASSIGN);
    assert(assignExpr->type == float4Type);
    assert(assignExpr->u.assign.op == CGIR_OP_ASSIGN);
    assert(assignExpr->u.assign.target == varRef);
    assert(assignExpr->u.assign.value == condExpr);

    callArgs = NULL;
    CgIRAppendExpr(&callArgs, assignExpr);
    callExpr = CgIRNewCall(&coverModule, float4Type, &fnALoc, calleeSymb,
                           callArgs);
    assert(callExpr != NULL);
    assert(callExpr->kind == CGIR_EXPR_CALL);
    assert(callExpr->u.call.callee == calleeSymb);
    assert(callExpr->u.call.arguments == assignExpr);

    dotArgs = NULL;
    CgIRAppendExpr(&dotArgs, binaryExpr);
    CgIRAppendExpr(&dotArgs, halfConst);
    intrinsicExpr = CgIRNewIntrinsicCall(&coverModule, floatType, &fnBLoc,
                                         CG_INTRINSIC_DOT, &lDotSignature,
                                         dotArgs);
    assert(intrinsicExpr != NULL);
    assert(intrinsicExpr->kind == CGIR_EXPR_INTRINSIC);
    assert(intrinsicExpr->u.intrinsicCall.intrinsic == CG_INTRINSIC_DOT);
    assert(intrinsicExpr->u.intrinsicCall.signature == &lDotSignature);
    assert(intrinsicExpr->u.intrinsicCall.arguments == binaryExpr);

    interfaceExpr = CgIRNewInterfaceCall(&coverModule, floatType, &paramLoc,
                                         methodSymb, varRef, NULL);
    assert(interfaceExpr != NULL);
    assert(interfaceExpr->kind == CGIR_EXPR_INTERFACE_CALL);
    assert(interfaceExpr->u.interfaceCall.method == methodSymb);
    assert(interfaceExpr->u.interfaceCall.receiver == varRef);
    assert(interfaceExpr->u.interfaceCall.arguments == NULL);

    stepExpr = CgIRNewUnary(&coverModule, float4Type, &ctorLoc,
                            CGIR_OP_POST_INCREMENT, varRef);
    assert(stepExpr != NULL);
    assert(stepExpr->u.unary.op == CGIR_OP_POST_INCREMENT);

    bodyList = NULL;
    declStmt = CgIRNewDeclStmt(&coverModule, &blockLoc, localDecl);
    assert(declStmt != NULL);
    assert(declStmt->kind == CGIR_STMT_DECL);
    assert(declStmt->u.decl == localDecl);
    CgIRAppendStmt(&bodyList, declStmt);

    exprStmt = CgIRNewExprStmt(&coverModule, &retLoc, callExpr);
    assert(exprStmt != NULL);
    assert(exprStmt->kind == CGIR_STMT_EXPR);
    assert(exprStmt->u.expression == callExpr);
    CgIRAppendStmt(&bodyList, exprStmt);

    innerBlock = CgIRNewBlockStmt(&coverModule, &blockLoc);
    assert(innerBlock != NULL);
    breakStmt = CgIRNewBreakStmt(&coverModule, &ctorLoc);
    assert(breakStmt != NULL);
    assert(breakStmt->kind == CGIR_STMT_BREAK);
    continueStmt = CgIRNewContinueStmt(&coverModule, &retLoc);
    assert(continueStmt != NULL);
    assert(continueStmt->kind == CGIR_STMT_CONTINUE);
    CgIRAppendStmt(&innerBlock->u.block, breakStmt);
    CgIRAppendStmt(&innerBlock->u.block, continueStmt);
    assert(innerBlock->u.block == breakStmt);
    assert(breakStmt->next == continueStmt);
    assert(continueStmt->next == NULL);

    whileStmt = CgIRNewWhileStmt(&coverModule, &fnALoc, boolConst,
                                 innerBlock);
    assert(whileStmt != NULL);
    assert(whileStmt->kind == CGIR_STMT_WHILE);
    assert(whileStmt->u.loop.condition == boolConst);
    assert(whileStmt->u.loop.body == innerBlock);

    doStmt = CgIRNewDoStmt(&coverModule, &fnBLoc, boolConst, innerBlock);
    assert(doStmt != NULL);
    assert(doStmt->kind == CGIR_STMT_DO);
    assert(doStmt->u.loop.condition == boolConst);
    assert(doStmt->u.loop.body == innerBlock);

    forInit = CgIRNewExprStmt(&coverModule, &paramLoc, stepExpr);
    assert(forInit != NULL);
    forStmt = CgIRNewForStmt(&coverModule, &constLoc, forInit, boolConst,
                             stepExpr, innerBlock);
    assert(forStmt != NULL);
    assert(forStmt->kind == CGIR_STMT_FOR);
    assert(forStmt->u.forStmt.init == forInit);
    assert(forStmt->u.forStmt.condition == boolConst);
    assert(forStmt->u.forStmt.step == stepExpr);
    assert(forStmt->u.forStmt.body == innerBlock);

    discardStmt = CgIRNewDiscardStmt(&coverModule, &ctorLoc, NULL);
    assert(discardStmt != NULL);
    assert(discardStmt->kind == CGIR_STMT_DISCARD);
    assert(discardStmt->u.discard.condition == NULL);

    ifStmt = CgIRNewIfStmt(&coverModule, &retLoc, boolConst, whileStmt, NULL);
    assert(ifStmt != NULL);
    assert(ifStmt->kind == CGIR_STMT_IF);
    assert(ifStmt->u.ifStmt.condition == boolConst);
    assert(ifStmt->u.ifStmt.trueBranch == whileStmt);
    assert(ifStmt->u.ifStmt.falseBranch == NULL);
    CgIRAppendStmt(&bodyList, ifStmt);
    CgIRAppendStmt(&bodyList, whileStmt);
    CgIRAppendStmt(&bodyList, doStmt);
    CgIRAppendStmt(&bodyList, forStmt);
    CgIRAppendStmt(&bodyList, discardStmt);

    /* Source order survives appending: declarations, expressions, then
     * control flow in insertion order. */
    assert(bodyList == declStmt);
    assert(declStmt->next == exprStmt);
    assert(exprStmt->next == ifStmt);
    assert(ifStmt->next == whileStmt);
    assert(whileStmt->next == doStmt);
    assert(doStmt->next == forStmt);
    assert(forStmt->next == discardStmt);
    assert(discardStmt->next == NULL);

    coverFn->body = bodyList;
    coverFn->isEntry = 1;
    coverModule.entry = coverFn;
    CgIRAppendFunction(&coverModule.functions, coverFn);
    assert(!CgIRModuleFailed(&coverModule));

    /////////////////// Scenario 3: dirty allocator //////////////////

    /* Every node byte is initialized by its builder; NULL locations and
     * absent payloads stay zero even over 0xa5-filled memory. */

    CgIRInitModule(&dirtyModule, DirtyAlloc, NULL);

    dFn = CgIRNewFunction(&dirtyModule, shadeSymb, float4Type, NULL);
    assert(dFn != NULL);
    assert(dFn->next == NULL);
    assert(dFn->symbol == shadeSymb);
    assert(dFn->resultType == float4Type);
    assert(dFn->loc.file == 0 && dFn->loc.line == 0);
    assert(dFn->isEntry == 0);
    assert(dFn->parameters == NULL);
    assert(dFn->locals == NULL);
    assert(dFn->body == NULL);

    dDecl = CgIRNewDecl(&dirtyModule, NULL, 0, float4Type,
                        CGIR_STORAGE_UNIFORM, CGIR_DOMAIN_UNIFORM, 0, NULL,
                        NULL);
    assert(dDecl != NULL);
    assert(dDecl->next == NULL);
    assert(dDecl->symbol == NULL);
    assert(dDecl->name == 0);
    assert(dDecl->semantic == 0);
    assert(dDecl->initializer == NULL);
    assert(dDecl->loc.file == 0 && dDecl->loc.line == 0);

    dExpr = CgIRNewConstant(&dirtyModule, floatType, NULL, NULL);
    assert(dExpr != NULL);
    assert(dExpr->kind == CGIR_EXPR_CONSTANT);
    assert(dExpr->type == floatType);
    assert(dExpr->loc.file == 0 && dExpr->loc.line == 0);
    assert(dExpr->synthesized == 0);
    assert(dExpr->isLvalue == 0);
    assert(dExpr->sideEffects == 0);
    assert(dExpr->next == NULL);
    assert(dExpr->u.constant.kind == CG_SCALAR_NONE);
    assert(dExpr->u.constant.value.i == 0);

    dStmt = CgIRNewDiscardStmt(&dirtyModule, NULL, NULL);
    assert(dStmt != NULL);
    assert(dStmt->kind == CGIR_STMT_DISCARD);
    assert(dStmt->loc.file == 0 && dStmt->loc.line == 0);
    assert(dStmt->synthesized == 0);
    assert(dStmt->next == NULL);
    assert(dStmt->u.discard.condition == NULL);

    dBlock = CgIRNewBlockStmt(&dirtyModule, NULL);
    assert(dBlock != NULL);
    assert(dBlock->u.block == NULL);

    CgIRAppendFunction(&dirtyModule.functions, dFn);
    dFn2 = CgIRNewFunction(&dirtyModule, mainSymb, float4Type, NULL);
    assert(dFn2 != NULL);
    CgIRAppendFunction(&dirtyModule.functions, dFn2);
    assert(dirtyModule.functions == dFn);
    assert(dFn->next == dFn2);
    assert(dFn2->next == NULL);
    assert(!CgIRModuleFailed(&dirtyModule));

    /////////////////// Scenario 4: failing allocators //////////////////

    /* A failing allocator yields NULL from the very first builder, marks
     * the module failed, and links nothing into the module. */

    CgIRInitModule(&failModule, FailingAlloc, NULL);
    assert(CgIRNewFunction(&failModule, shadeSymb, float4Type,
                           &fnALoc) == NULL);
    assert(CgIRModuleFailed(&failModule));
    assert(failModule.functions == NULL);
    assert(failModule.entry == NULL);
    assert(CgIRNewConstant(&failModule, floatType, &constLoc,
                           &vZero) == NULL);
    assert(CgIRNewBlockStmt(&failModule, &blockLoc) == NULL);

    /* Mid-construction exhaustion: earlier nodes stay intact, the
     * failing builder returns NULL instead of a partial node, failure
     * is sticky, and sticky builders attempt no allocation. */

    allocationBudget = 3;
    CgIRInitModule(&budgetModule, BudgetAlloc, NULL);
    budgetFn = CgIRNewFunction(&budgetModule, shadeSymb, float4Type,
                               &fnALoc);
    assert(budgetFn != NULL);
    budgetDecl = CgIRNewDecl(&budgetModule, positionSymb,
                             positionSymb->name, float4Type,
                             CGIR_STORAGE_NONE, CGIR_DOMAIN_VARYING, 0, NULL,
                             &paramLoc);
    assert(budgetDecl != NULL);
    budgetC0 = CgIRNewConstant(&budgetModule, floatType, &constLoc, &vZero);
    assert(budgetC0 != NULL);
    assert(budgetC0->kind == CGIR_EXPR_CONSTANT);
    assert(budgetC0->type == floatType);
    budgetC1 = CgIRNewConstant(&budgetModule, floatType, &constLoc, &vOne);
    assert(budgetC1 == NULL);
    assert(CgIRModuleFailed(&budgetModule));
    assert(allocationBudget == -1);

    stickyExpr = CgIRNewConstant(&budgetModule, floatType, &constLoc,
                                 &vZero);
    assert(stickyExpr == NULL);
    assert(allocationBudget == -1);
    assert(budgetFn->next == NULL);
    assert(budgetModule.functions == NULL);

    /* A failed module can never verify: its graph is incomplete by
     * construction. */
    memset(&verifyDiagnostic, 0, sizeof(verifyDiagnostic));
    assert(!CgIRVerifyModule(&failModule, &verifyDiagnostic));
    assert(verifyDiagnostic.reason == CGIR_VERIFY_OWNER);
    assert(verifyDiagnostic.node == &failModule);

    /////////////////// Scenario 5: module verification //////////////////

    /*
     * Verifier contract: one well-formed module verifies with a zeroed
     * diagnostic; each malformed module below is rejected reporting its
     * stable internal reason enum (never user-facing text) about the
     * concrete failing node; verification stops at the first invariant.
     */

    /* Local interfaces: opaque identities whose category bits are their
     * whole story here; method symbols carry their owning interface. */
    memset(&ifaceAType, 0, sizeof(ifaceAType));
    ifaceAType.properties = TYPE_CATEGORY_INTERFACE;
    memset(&ifaceBType, 0, sizeof(ifaceBType));
    ifaceBType.properties = TYPE_CATEGORY_INTERFACE;

    evalSymb = lMakeSymbol(FUNCTION_S, "eval", floatType);
    evalSymb->details.fun.isMethod = 1;
    evalSymb->details.fun.ownerType = &ifaceAType;
    otherEvalSymb = lMakeSymbol(FUNCTION_S, "eval", floatType);
    otherEvalSymb->details.fun.isMethod = 1;
    otherEvalSymb->details.fun.ownerType = &ifaceBType;

    sampler2DType = GetSamplerType(CG_SAMPLER_2D);
    assert(sampler2DType != NULL && sampler2DType != UndefinedType);

    helperSymb = lMakeSymbol(FUNCTION_S, "scale", VoidType);
    outFormalSymb = lMakeSymbol(VARIABLE_S, "written", float4Type);
    inFormalSymb = lMakeSymbol(VARIABLE_S, "read", float4Type);
    texSymb = lMakeSymbol(VARIABLE_S, "tex", sampler2DType);
    tmpSymb = lMakeSymbol(VARIABLE_S, "tmp", float4Type);
    objSymb = lMakeSymbol(VARIABLE_S, "obj", &ifaceAType);

    /* Parameter direction rides type qualifier bits on a qualified
     * copy: mutating the interned canonical float4 is forbidden. */
    qualifiedOutFloat4 = *float4Type;
    qualifiedOutFloat4.properties |= TYPE_QUALIFIER_OUT;

    /* dot(float, float) -> float local signature with parameters. */
    dotFFLast.next = NULL;
    dotFFLast.type = floatType;
    dotFFFirst.next = &dotFFLast;
    dotFFFirst.type = floatType;
    dotFFSignature.intrinsic = CG_INTRINSIC_DOT;
    dotFFSignature.name = "dot";
    dotFFSignature.result = floatType;
    dotFFSignature.parameters = &dotFFFirst;
    dotFFSignature.flags = CG_INTRINSIC_PURE | CG_INTRINSIC_FOLDABLE;

    /* Positive module: every verifiable construct in one valid program,
     * including a bare discard synthesized by convention (all-zero
     * location, clear flag).  Lvalue and write-target flags are set by
     * hand: builders leave them clear by design. */

    CgIRInitModule(&verifyModule, TestAlloc, NULL);
    verifyModule.profile = &genericIdentity;

    vTexDecl = CgIRNewDecl(&verifyModule, texSymb, texSymb->name,
                           sampler2DType, CGIR_STORAGE_UNIFORM,
                           CGIR_DOMAIN_UNIFORM, 0, NULL, &paramLoc);
    vObjDecl = CgIRNewDecl(&verifyModule, objSymb, objSymb->name,
                           &ifaceAType, CGIR_STORAGE_UNIFORM,
                           CGIR_DOMAIN_UNIFORM, 0, NULL, &paramLoc);
    assert(vTexDecl != NULL && vObjDecl != NULL);
    CgIRAppendDecl(&verifyModule.globals, vTexDecl);
    CgIRAppendDecl(&verifyModule.globals, vObjDecl);

    vHelperFn = CgIRNewFunction(&verifyModule, helperSymb, VoidType,
                                &fnALoc);
    vMainFn = CgIRNewFunction(&verifyModule, mainSymb, float4Type, &fnBLoc);
    assert(vHelperFn != NULL && vMainFn != NULL);

    vOutFormal = CgIRNewDecl(&verifyModule, outFormalSymb,
                             outFormalSymb->name, &qualifiedOutFloat4,
                             CGIR_STORAGE_NONE, CGIR_DOMAIN_NONE, 0, NULL,
                             &paramLoc);
    vInFormal = CgIRNewDecl(&verifyModule, inFormalSymb,
                            inFormalSymb->name, float4Type,
                            CGIR_STORAGE_NONE, CGIR_DOMAIN_NONE, 0, NULL,
                            &paramLoc);
    vPosFormal = CgIRNewDecl(&verifyModule, positionSymb,
                             positionSymb->name, float4Type,
                             CGIR_STORAGE_NONE, CGIR_DOMAIN_VARYING, 0,
                             NULL, &paramLoc);
    assert(vOutFormal != NULL && vInFormal != NULL && vPosFormal != NULL);
    CgIRAppendDecl(&vHelperFn->parameters, vOutFormal);
    CgIRAppendDecl(&vHelperFn->parameters, vInFormal);
    CgIRAppendDecl(&vMainFn->parameters, vPosFormal);

    /* Helper body: assigns the out formal from the in formal. */
    rTarget = CgIRNewSymbol(&verifyModule, float4Type, &constLoc,
                            outFormalSymb);
    assert(rTarget != NULL);
    rTarget->isLvalue = 1;
    rValue = CgIRNewSymbol(&verifyModule, float4Type, &constLoc,
                           inFormalSymb);
    assert(rValue != NULL);
    rValue->isLvalue = 1;
    rValue = CgIRNewAssign(&verifyModule, float4Type, &ctorLoc,
                           CGIR_OP_ASSIGN, rTarget, rValue);
    assert(rValue != NULL);
    rStmt = CgIRNewExprStmt(&verifyModule, &retLoc, rValue);
    rBody = CgIRNewBlockStmt(&verifyModule, &blockLoc);
    assert(rStmt != NULL && rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, rStmt);
    vHelperFn->body = rBody;

    /* Entry local: float4 tmp initialized by scalar replication. */
    vTmpDecl = CgIRNewDecl(&verifyModule, tmpSymb, tmpSymb->name,
                           float4Type, CGIR_STORAGE_CONST,
                           CGIR_DOMAIN_NONE, 0, NULL, &paramLoc);
    rLeft = CgIRNewConstant(&verifyModule, floatType, &constLoc, &vHalf);
    vInit = CgIRNewConstruct(&verifyModule, float4Type, &ctorLoc, rLeft);
    assert(vTmpDecl != NULL && rLeft != NULL && vInit != NULL);
    vTmpDecl->initializer = vInit;

    vTmpRef = CgIRNewSymbol(&verifyModule, float4Type, &constLoc, tmpSymb);
    assert(vTmpRef != NULL);
    vTmpRef->isLvalue = 1;

    bodyList = NULL;

    /* 1. Declaration statement registers the local's visibility. */
    declStmt = CgIRNewDeclStmt(&verifyModule, &blockLoc, vTmpDecl);
    assert(declStmt != NULL);
    CgIRAppendStmt(&bodyList, declStmt);

    /* 2. Unique-component write mask through a swizzled lvalue (.xy). */
    rTarget = CgIRNewSwizzle(&verifyModule, float2Type, &ctorLoc, vTmpRef,
                             0x4, 2);
    assert(rTarget != NULL);
    rTarget->isLvalue = 1;
    rLeft = CgIRNewConstant(&verifyModule, floatType, &constLoc, &vZero);
    rRight = CgIRNewConstant(&verifyModule, floatType, &constLoc, &vOne);
    assert(rLeft != NULL && rRight != NULL);
    rArgs = NULL;
    CgIRAppendExpr(&rArgs, rLeft);
    CgIRAppendExpr(&rArgs, rRight);
    rValue = CgIRNewConstruct(&verifyModule, float2Type, &ctorLoc, rArgs);
    assert(rValue != NULL);
    assignExpr = CgIRNewAssign(&verifyModule, float2Type, &blockLoc,
                               CGIR_OP_ASSIGN, rTarget, rValue);
    assert(assignExpr != NULL);
    exprStmt = CgIRNewExprStmt(&verifyModule, &retLoc, assignExpr);
    assert(exprStmt != NULL);
    CgIRAppendStmt(&bodyList, exprStmt);

    /* 3. Scalar arithmetic under the usual conversions. */
    rLeft = CgIRNewConstant(&verifyModule, floatType, &constLoc, &vZero);
    rRight = CgIRNewConstant(&verifyModule, floatType, &constLoc, &vOne);
    assert(rLeft != NULL && rRight != NULL);
    binaryExpr = CgIRNewBinary(&verifyModule, floatType, &ctorLoc,
                               CGIR_OP_ADD, rLeft, rRight);
    assert(binaryExpr != NULL);
    exprStmt = CgIRNewExprStmt(&verifyModule, &retLoc, binaryExpr);
    assert(exprStmt != NULL);
    CgIRAppendStmt(&bodyList, exprStmt);

    /* 4. Unary negate plus an lvalue increment. */
    rLeft = CgIRNewConstant(&verifyModule, floatType, &constLoc, &vHalf);
    assert(rLeft != NULL);
    unaryExpr = CgIRNewUnary(&verifyModule, floatType, &paramLoc,
                             CGIR_OP_NEGATE, rLeft);
    assert(unaryExpr != NULL);
    exprStmt = CgIRNewExprStmt(&verifyModule, &retLoc, unaryExpr);
    assert(exprStmt != NULL);
    CgIRAppendStmt(&bodyList, exprStmt);
    rValue = CgIRNewUnary(&verifyModule, float4Type, &ctorLoc,
                          CGIR_OP_POST_INCREMENT, vTmpRef);
    assert(rValue != NULL);
    exprStmt = CgIRNewExprStmt(&verifyModule, &retLoc, rValue);
    assert(exprStmt != NULL);
    CgIRAppendStmt(&bodyList, exprStmt);

    /* 5. Swizzle read, indexing, and length. */
    swizExpr = CgIRNewSwizzle(&verifyModule, float2Type, &ctorLoc, vTmpRef,
                              0x8, 2);
    idxConst = CgIRNewConstant(&verifyModule, intType, &constLoc,
                               &vIntZero);
    assert(swizExpr != NULL && idxConst != NULL);
    indexExpr = CgIRNewIndex(&verifyModule, floatType, &retLoc, swizExpr,
                             idxConst);
    assert(indexExpr != NULL);
    exprStmt = CgIRNewExprStmt(&verifyModule, &retLoc, indexExpr);
    assert(exprStmt != NULL);
    CgIRAppendStmt(&bodyList, exprStmt);
    lenExpr = CgIRNewLength(&verifyModule, intType, &fnALoc, vTmpRef);
    assert(lenExpr != NULL);
    exprStmt = CgIRNewExprStmt(&verifyModule, &retLoc, lenExpr);
    assert(exprStmt != NULL);
    CgIRAppendStmt(&bodyList, exprStmt);

    /* 6. Conditional between compatible branches. */
    boolConst = CgIRNewConstant(&verifyModule, boolType, &constLoc, &vTrue);
    rLeft = CgIRNewConstant(&verifyModule, floatType, &constLoc, &vZero);
    rRight = CgIRNewConstant(&verifyModule, floatType, &constLoc, &vHalf);
    assert(boolConst != NULL && rLeft != NULL && rRight != NULL);
    condExpr = CgIRNewConditional(&verifyModule, floatType, &retLoc,
                                  boolConst, rLeft, rRight);
    assert(condExpr != NULL);
    exprStmt = CgIRNewExprStmt(&verifyModule, &retLoc, condExpr);
    assert(exprStmt != NULL);
    CgIRAppendStmt(&bodyList, exprStmt);

    /* 7. Explicit cast. */
    rLeft = CgIRNewConstant(&verifyModule, intType, &constLoc, &vIntZero);
    assert(rLeft != NULL);
    castExpr = CgIRNewCast(&verifyModule, floatType, &fnBLoc, rLeft);
    assert(castExpr != NULL);
    exprStmt = CgIRNewExprStmt(&verifyModule, &retLoc, castExpr);
    assert(exprStmt != NULL);
    CgIRAppendStmt(&bodyList, exprStmt);

    /* 8. Intrinsic call matching its catalog-form signature. */
    rLeft = CgIRNewConstant(&verifyModule, floatType, &constLoc, &vZero);
    rRight = CgIRNewConstant(&verifyModule, floatType, &constLoc, &vHalf);
    assert(rLeft != NULL && rRight != NULL);
    dotArgs = NULL;
    CgIRAppendExpr(&dotArgs, rLeft);
    CgIRAppendExpr(&dotArgs, rRight);
    intrinsicExpr = CgIRNewIntrinsicCall(&verifyModule, floatType, &fnALoc,
                                         CG_INTRINSIC_DOT, &dotFFSignature,
                                         dotArgs);
    assert(intrinsicExpr != NULL);
    exprStmt = CgIRNewExprStmt(&verifyModule, &retLoc, intrinsicExpr);
    assert(exprStmt != NULL);
    CgIRAppendStmt(&bodyList, exprStmt);

    /* 9. Interface dispatch through a conforming receiver. */
    vObjRef = CgIRNewSymbol(&verifyModule, &ifaceAType, &constLoc, objSymb);
    assert(vObjRef != NULL);
    interfaceExpr = CgIRNewInterfaceCall(&verifyModule, floatType,
                                         &paramLoc, evalSymb, vObjRef, NULL);
    assert(interfaceExpr != NULL);
    exprStmt = CgIRNewExprStmt(&verifyModule, &retLoc, interfaceExpr);
    assert(exprStmt != NULL);
    CgIRAppendStmt(&bodyList, exprStmt);

    /* 10. User call honoring arity and parameter directions. */
    rLeft = CgIRNewSymbol(&verifyModule, float4Type, &constLoc, tmpSymb);
    rRight = CgIRNewSymbol(&verifyModule, float4Type, &constLoc, tmpSymb);
    assert(rLeft != NULL && rRight != NULL);
    rLeft->isLvalue = 1;
    rRight->isLvalue = 1;
    callArgs = NULL;
    CgIRAppendExpr(&callArgs, rLeft);
    CgIRAppendExpr(&callArgs, rRight);
    callExpr = CgIRNewCall(&verifyModule, VoidType, &fnALoc, helperSymb,
                           callArgs);
    assert(callExpr != NULL);
    exprStmt = CgIRNewExprStmt(&verifyModule, &retLoc, callExpr);
    assert(exprStmt != NULL);
    CgIRAppendStmt(&bodyList, exprStmt);

    /* 11. Loop-carried break and continue inside an if. */
    breakStmt = CgIRNewBreakStmt(&verifyModule, &ctorLoc);
    continueStmt = CgIRNewContinueStmt(&verifyModule, &retLoc);
    assert(breakStmt != NULL && continueStmt != NULL);
    rInner = CgIRNewBlockStmt(&verifyModule, &blockLoc);
    rBody = CgIRNewBlockStmt(&verifyModule, &blockLoc);
    assert(rInner != NULL && rBody != NULL);
    CgIRAppendStmt(&rInner->u.block, breakStmt);
    CgIRAppendStmt(&rBody->u.block, continueStmt);
    ifStmt = CgIRNewIfStmt(&verifyModule, &retLoc, boolConst, rInner,
                           rBody);
    assert(ifStmt != NULL);
    rInner = CgIRNewBlockStmt(&verifyModule, &blockLoc);
    assert(rInner != NULL);
    CgIRAppendStmt(&rInner->u.block, ifStmt);
    whileStmt = CgIRNewWhileStmt(&verifyModule, &fnALoc, boolConst, rInner);
    assert(whileStmt != NULL);
    CgIRAppendStmt(&bodyList, whileStmt);

    /* 12. Bare discard: conventional synthesis (zero location, clear
     * flag) and no predicate. */
    discardStmt = CgIRNewDiscardStmt(&verifyModule, NULL, NULL);
    assert(discardStmt != NULL);
    CgIRAppendStmt(&bodyList, discardStmt);

    /* 13. Return of the advertised result type. */
    rLeft = CgIRNewConstant(&verifyModule, floatType, &constLoc, &vZero);
    rRight = CgIRNewConstant(&verifyModule, floatType, &constLoc, &vOne);
    rTarget = CgIRNewConstant(&verifyModule, floatType, &constLoc, &vZero);
    rValue = CgIRNewConstant(&verifyModule, floatType, &constLoc, &vOne);
    assert(rLeft != NULL && rRight != NULL);
    assert(rTarget != NULL && rValue != NULL);
    rArgs = NULL;
    CgIRAppendExpr(&rArgs, rLeft);
    CgIRAppendExpr(&rArgs, rRight);
    CgIRAppendExpr(&rArgs, rTarget);
    CgIRAppendExpr(&rArgs, rValue);
    retStmt = CgIRNewReturnStmt(&verifyModule, &retLoc,
                                CgIRNewConstruct(&verifyModule, float4Type,
                                                 &ctorLoc, rArgs));
    assert(retStmt != NULL && retStmt->u.returnExpr != NULL);
    CgIRAppendStmt(&bodyList, retStmt);

    bodyBlock = CgIRNewBlockStmt(&verifyModule, &blockLoc);
    assert(bodyBlock != NULL);
    CgIRAppendStmt(&bodyBlock->u.block, bodyList);
    vMainFn->body = bodyBlock;

    vMainFn->isEntry = 1;
    verifyModule.entry = vMainFn;
    CgIRAppendFunction(&verifyModule.functions, vHelperFn);
    CgIRAppendFunction(&verifyModule.functions, vMainFn);

    lVerifyAccept(&verifyModule);

    /*
     * Rejected modules.  Each case isolates exactly one invariant so
     * the reported reason is unambiguous; verification stops at the
     * first failure.
     */

    /* R1: binary result type disagrees with its operands - the builder
     * accepts any caller-supplied result type, so float + float typed
     * as float2 reaches the verifier unchanged. */
    CgIRInitModule(&rejectModule, TestAlloc, NULL);
    rFn = CgIRNewFunction(&rejectModule, mainSymb, float4Type, &fnALoc);
    assert(rFn != NULL);
    rLeft = CgIRNewConstant(&rejectModule, floatType, &constLoc, &vZero);
    rRight = CgIRNewConstant(&rejectModule, floatType, &constLoc, &vOne);
    assert(rLeft != NULL && rRight != NULL);
    rValue = CgIRNewBinary(&rejectModule, float2Type, &ctorLoc,
                           CGIR_OP_ADD, rLeft, rRight);
    assert(rValue != NULL);
    rStmt = CgIRNewExprStmt(&rejectModule, &retLoc, rValue);
    rBody = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rStmt != NULL && rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, rStmt);
    rFn->body = rBody;
    rFn->isEntry = 1;
    rejectModule.entry = rFn;
    CgIRAppendFunction(&rejectModule.functions, rFn);
    memset(&verifyDiagnostic, 0, sizeof(verifyDiagnostic));
    assert(!CgIRVerifyModule(&rejectModule, &verifyDiagnostic));
    assert(verifyDiagnostic.reason == CGIR_VERIFY_TYPE);
    assert(verifyDiagnostic.node == rValue);
    assert(verifyDiagnostic.loc.file == ctorLoc.file);
    assert(verifyDiagnostic.loc.line == ctorLoc.line);

    /* R2: assignment to a non-lvalue - builders leave the lvalue flag
     * clear, so an unadorned symbol reference is the non-lvalue. */
    CgIRInitModule(&rejectModule, TestAlloc, NULL);
    rDecl = CgIRNewDecl(&rejectModule, tmpSymb, tmpSymb->name, float4Type,
                        CGIR_STORAGE_UNIFORM, CGIR_DOMAIN_UNIFORM, 0, NULL,
                        &paramLoc);
    assert(rDecl != NULL);
    CgIRAppendDecl(&rejectModule.globals, rDecl);
    rFn = CgIRNewFunction(&rejectModule, mainSymb, float4Type, &fnALoc);
    assert(rFn != NULL);
    rTarget = CgIRNewSymbol(&rejectModule, float4Type, &constLoc, tmpSymb);
    rValue = CgIRNewConstant(&rejectModule, floatType, &constLoc, &vHalf);
    assert(rTarget != NULL && rValue != NULL);
    assignExpr = CgIRNewAssign(&rejectModule, float4Type, &blockLoc,
                               CGIR_OP_ASSIGN, rTarget, rValue);
    assert(assignExpr != NULL);
    rStmt = CgIRNewExprStmt(&rejectModule, &retLoc, assignExpr);
    rBody = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rStmt != NULL && rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, rStmt);
    rFn->body = rBody;
    rFn->isEntry = 1;
    rejectModule.entry = rFn;
    CgIRAppendFunction(&rejectModule.functions, rFn);
    lVerifyReject(&rejectModule, CGIR_VERIFY_LVALUE);

    /* R3: wrong-arity call - the callee declares one formal, the call
     * site passes two arguments. */
    CgIRInitModule(&rejectModule, TestAlloc, NULL);
    rCalleeFn = CgIRNewFunction(&rejectModule, shadeSymb, floatType,
                                &fnALoc);
    rFn = CgIRNewFunction(&rejectModule, mainSymb, float4Type, &fnBLoc);
    assert(rCalleeFn != NULL && rFn != NULL);
    rDecl = CgIRNewDecl(&rejectModule, inFormalSymb, inFormalSymb->name,
                        float4Type, CGIR_STORAGE_NONE, CGIR_DOMAIN_NONE, 0,
                        NULL, &paramLoc);
    assert(rDecl != NULL);
    CgIRAppendDecl(&rCalleeFn->parameters, rDecl);
    rCalleeFn->body = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rCalleeFn->body != NULL);
    rLeft = CgIRNewConstant(&rejectModule, floatType, &constLoc, &vZero);
    rRight = CgIRNewConstant(&rejectModule, floatType, &constLoc, &vOne);
    assert(rLeft != NULL && rRight != NULL);
    callArgs = NULL;
    CgIRAppendExpr(&callArgs, rLeft);
    CgIRAppendExpr(&callArgs, rRight);
    callExpr = CgIRNewCall(&rejectModule, floatType, &ctorLoc, shadeSymb,
                           callArgs);
    assert(callExpr != NULL);
    rStmt = CgIRNewExprStmt(&rejectModule, &retLoc, callExpr);
    rBody = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rStmt != NULL && rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, rStmt);
    rFn->body = rBody;
    rFn->isEntry = 1;
    rejectModule.entry = rFn;
    CgIRAppendFunction(&rejectModule.functions, rCalleeFn);
    CgIRAppendFunction(&rejectModule.functions, rFn);
    lVerifyReject(&rejectModule, CGIR_VERIFY_CALL);

    /* R4a: intrinsic arity disagrees with the signature's parameter
     * list (one argument against dot(float, float)). */
    CgIRInitModule(&rejectModule, TestAlloc, NULL);
    rFn = CgIRNewFunction(&rejectModule, mainSymb, float4Type, &fnALoc);
    assert(rFn != NULL);
    rLeft = CgIRNewConstant(&rejectModule, floatType, &constLoc, &vZero);
    assert(rLeft != NULL);
    dotArgs = NULL;
    CgIRAppendExpr(&dotArgs, rLeft);
    intrinsicExpr = CgIRNewIntrinsicCall(&rejectModule, floatType, &fnALoc,
                                         CG_INTRINSIC_DOT, &dotFFSignature,
                                         dotArgs);
    assert(intrinsicExpr != NULL);
    rStmt = CgIRNewExprStmt(&rejectModule, &retLoc, intrinsicExpr);
    rBody = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rStmt != NULL && rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, rStmt);
    rFn->body = rBody;
    rFn->isEntry = 1;
    rejectModule.entry = rFn;
    CgIRAppendFunction(&rejectModule.functions, rFn);
    lVerifyReject(&rejectModule, CGIR_VERIFY_INTRINSIC);

    /* R4b: intrinsic identity disagrees with its signature.  Builders
     * assert identity agreement, so this state is only reachable by
     * overwriting the opcode after construction. */
    CgIRInitModule(&rejectModule, TestAlloc, NULL);
    rFn = CgIRNewFunction(&rejectModule, mainSymb, float4Type, &fnALoc);
    assert(rFn != NULL);
    rLeft = CgIRNewConstant(&rejectModule, floatType, &constLoc, &vZero);
    rRight = CgIRNewConstant(&rejectModule, floatType, &constLoc, &vOne);
    assert(rLeft != NULL && rRight != NULL);
    dotArgs = NULL;
    CgIRAppendExpr(&dotArgs, rLeft);
    CgIRAppendExpr(&dotArgs, rRight);
    intrinsicExpr = CgIRNewIntrinsicCall(&rejectModule, floatType, &fnALoc,
                                         CG_INTRINSIC_DOT, &dotFFSignature,
                                         dotArgs);
    assert(intrinsicExpr != NULL);
    intrinsicExpr->u.intrinsicCall.intrinsic = CG_INTRINSIC_LENGTH;
    rStmt = CgIRNewExprStmt(&rejectModule, &retLoc, intrinsicExpr);
    rBody = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rStmt != NULL && rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, rStmt);
    rFn->body = rBody;
    rFn->isEntry = 1;
    rejectModule.entry = rFn;
    CgIRAppendFunction(&rejectModule.functions, rFn);
    lVerifyReject(&rejectModule, CGIR_VERIFY_INTRINSIC);

    /* R5: return value cannot convert to the function result - a
     * sampler never converts to float4. */
    CgIRInitModule(&rejectModule, TestAlloc, NULL);
    rDecl = CgIRNewDecl(&rejectModule, texSymb, texSymb->name,
                        sampler2DType, CGIR_STORAGE_UNIFORM,
                        CGIR_DOMAIN_UNIFORM, 0, NULL, &paramLoc);
    assert(rDecl != NULL);
    CgIRAppendDecl(&rejectModule.globals, rDecl);
    rFn = CgIRNewFunction(&rejectModule, mainSymb, float4Type, &fnALoc);
    assert(rFn != NULL);
    rValue = CgIRNewSymbol(&rejectModule, sampler2DType, &constLoc, texSymb);
    assert(rValue != NULL);
    retStmt = CgIRNewReturnStmt(&rejectModule, &retLoc, rValue);
    assert(retStmt != NULL);
    rBody = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, retStmt);
    rFn->body = rBody;
    rFn->isEntry = 1;
    rejectModule.entry = rFn;
    CgIRAppendFunction(&rejectModule.functions, rFn);
    memset(&verifyDiagnostic, 0, sizeof(verifyDiagnostic));
    assert(!CgIRVerifyModule(&rejectModule, &verifyDiagnostic));
    assert(verifyDiagnostic.reason == CGIR_VERIFY_TYPE);
    assert(verifyDiagnostic.node == retStmt);

    /* R6: break outside any loop. */
    CgIRInitModule(&rejectModule, TestAlloc, NULL);
    rFn = CgIRNewFunction(&rejectModule, mainSymb, float4Type, &fnALoc);
    assert(rFn != NULL);
    breakStmt = CgIRNewBreakStmt(&rejectModule, &ctorLoc);
    assert(breakStmt != NULL);
    rBody = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, breakStmt);
    rFn->body = rBody;
    rFn->isEntry = 1;
    rejectModule.entry = rFn;
    CgIRAppendFunction(&rejectModule.functions, rFn);
    lVerifyReject(&rejectModule, CGIR_VERIFY_CONTROL);

    /* R7: discard carrying a non-Boolean predicate. */
    CgIRInitModule(&rejectModule, TestAlloc, NULL);
    rFn = CgIRNewFunction(&rejectModule, mainSymb, float4Type, &fnALoc);
    assert(rFn != NULL);
    rLeft = CgIRNewConstant(&rejectModule, floatType, &constLoc, &vHalf);
    assert(rLeft != NULL);
    discardStmt = CgIRNewDiscardStmt(&rejectModule, &ctorLoc, rLeft);
    assert(discardStmt != NULL);
    rBody = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, discardStmt);
    rFn->body = rBody;
    rFn->isEntry = 1;
    rejectModule.entry = rFn;
    CgIRAppendFunction(&rejectModule.functions, rFn);
    lVerifyReject(&rejectModule, CGIR_VERIFY_OPERAND);

    /* R8: interface call whose receiver has the wrong interface - the
     * method implements ifaceB while the receiver is an ifaceA. */
    CgIRInitModule(&rejectModule, TestAlloc, NULL);
    rDecl = CgIRNewDecl(&rejectModule, objSymb, objSymb->name, &ifaceAType,
                        CGIR_STORAGE_UNIFORM, CGIR_DOMAIN_UNIFORM, 0, NULL,
                        &paramLoc);
    assert(rDecl != NULL);
    CgIRAppendDecl(&rejectModule.globals, rDecl);
    rFn = CgIRNewFunction(&rejectModule, mainSymb, float4Type, &fnALoc);
    assert(rFn != NULL);
    rValue = CgIRNewSymbol(&rejectModule, &ifaceAType, &constLoc, objSymb);
    assert(rValue != NULL);
    interfaceExpr = CgIRNewInterfaceCall(&rejectModule, floatType,
                                         &paramLoc, otherEvalSymb, rValue,
                                         NULL);
    assert(interfaceExpr != NULL);
    rStmt = CgIRNewExprStmt(&rejectModule, &retLoc, interfaceExpr);
    rBody = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rStmt != NULL && rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, rStmt);
    rFn->body = rBody;
    rFn->isEntry = 1;
    rejectModule.entry = rFn;
    CgIRAppendFunction(&rejectModule.functions, rFn);
    lVerifyReject(&rejectModule, CGIR_VERIFY_INTERFACE);

    /* R9: user-derived node with an empty source location.  The
     * documented convention makes all-zero location + clear flag
     * conventionally synthesized, which verifies; setting the
     * synthesized flag over an empty location claims provenance the
     * encoding cannot honor.  Builders never set the flag, so the test
     * writes it directly. */
    CgIRInitModule(&rejectModule, TestAlloc, NULL);
    rFn = CgIRNewFunction(&rejectModule, mainSymb, float4Type, &fnALoc);
    assert(rFn != NULL);
    rValue = CgIRNewConstant(&rejectModule, floatType, &constLoc, &vHalf);
    assert(rValue != NULL);
    rStmt = CgIRNewExprStmt(&rejectModule, NULL, rValue);
    assert(rStmt != NULL);
    rStmt->synthesized = 1;
    rBody = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, rStmt);
    rFn->body = rBody;
    rFn->isEntry = 1;
    rejectModule.entry = rFn;
    CgIRAppendFunction(&rejectModule.functions, rFn);
    memset(&verifyDiagnostic, 0, sizeof(verifyDiagnostic));
    assert(!CgIRVerifyModule(&rejectModule, &verifyDiagnostic));
    assert(verifyDiagnostic.reason == CGIR_VERIFY_LOCATION);
    assert(verifyDiagnostic.node == rStmt);

    /* Twin of R9: the same statement with the flag left clear stays
     * conventionally synthesized and must verify. */
    CgIRInitModule(&rejectModule, TestAlloc, NULL);
    rFn = CgIRNewFunction(&rejectModule, mainSymb, float4Type, &fnALoc);
    assert(rFn != NULL);
    rValue = CgIRNewConstant(&rejectModule, floatType, &constLoc, &vHalf);
    assert(rValue != NULL);
    rStmt = CgIRNewExprStmt(&rejectModule, NULL, rValue);
    assert(rStmt != NULL);
    assert(rStmt->synthesized == 0);
    rBody = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, rStmt);
    rFn->body = rBody;
    rFn->isEntry = 1;
    rejectModule.entry = rFn;
    CgIRAppendFunction(&rejectModule.functions, rFn);
    lVerifyAccept(&rejectModule);

    /* Connector aggregates are struct-shaped for member access: $vout
     * and friends carry TYPE_CATEGORY_CONNECTOR, and lowering emits
     * member selections on them.  One data member, one selection into
     * an assignment target. */
    memset(&connType, 0, sizeof(connType));
    connType.properties = TYPE_CATEGORY_CONNECTOR;
    memset(&connScope, 0, sizeof(connScope));
    connMemberSymb = lMakeSymbol(VARIABLE_S, "connmember", floatType);
    connScope.params = connMemberSymb;
    connType.str.members = &connScope;
    connVarSymb = lMakeSymbol(VARIABLE_S, "$conn", &connType);

    CgIRInitModule(&rejectModule, TestAlloc, NULL);
    rDecl = CgIRNewDecl(&rejectModule, connVarSymb, connVarSymb->name,
                        &connType, CGIR_STORAGE_NONE, CGIR_DOMAIN_UNIFORM,
                        0, NULL, &paramLoc);
    assert(rDecl != NULL);
    CgIRAppendDecl(&rejectModule.globals, rDecl);
    rFn = CgIRNewFunction(&rejectModule, mainSymb, VoidType, &fnALoc);
    assert(rFn != NULL);
    rLeft = CgIRNewSymbol(&rejectModule, &connType, &constLoc, connVarSymb);
    assert(rLeft != NULL);
    rLeft->isLvalue = 1;
    rValue = CgIRNewMember(&rejectModule, floatType, &constLoc, rLeft,
                           connMemberSymb);
    assert(rValue != NULL);
    rValue->isLvalue = 1;
    rTarget = CgIRNewConstant(&rejectModule, floatType, &constLoc, &vHalf);
    assert(rTarget != NULL);
    rValue = CgIRNewAssign(&rejectModule, floatType, &ctorLoc,
                           CGIR_OP_ASSIGN, rValue, rTarget);
    assert(rValue != NULL);
    rStmt = CgIRNewExprStmt(&rejectModule, &retLoc, rValue);
    assert(rStmt != NULL);
    rBody = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, rStmt);
    rFn->body = rBody;
    rFn->isEntry = 1;
    rejectModule.entry = rFn;
    CgIRAppendFunction(&rejectModule.functions, rFn);
    lVerifyAccept(&rejectModule);

    /* Constructors cover first-class arrays too: an exact element fill
     * verifies, and so does scalar replication; a short fill stays an
     * operand failure. */
    memset(&array3Type, 0, sizeof(array3Type));
    array3Type.properties = TYPE_BASE_FLOAT | TYPE_CATEGORY_ARRAY;
    array3Type.arr.eltype = floatType;
    array3Type.arr.numels = 3;
    array3Type.arr.scalarKind = CG_SCALAR_FLOAT;

    CgIRInitModule(&rejectModule, TestAlloc, NULL);
    rFn = CgIRNewFunction(&rejectModule, mainSymb, VoidType, &fnALoc);
    assert(rFn != NULL);
    rArgs = NULL;
    CgIRAppendExpr(&rArgs, CgIRNewConstant(&rejectModule, floatType,
                                           &constLoc, &vZero));
    CgIRAppendExpr(&rArgs, CgIRNewConstant(&rejectModule, floatType,
                                           &constLoc, &vHalf));
    CgIRAppendExpr(&rArgs, CgIRNewConstant(&rejectModule, floatType,
                                           &constLoc, &vOne));
    assert(rArgs != NULL && rArgs->next != NULL && rArgs->next->next != NULL);
    rValue = CgIRNewConstruct(&rejectModule, &array3Type, &ctorLoc, rArgs);
    assert(rValue != NULL);
    rStmt = CgIRNewExprStmt(&rejectModule, &retLoc, rValue);
    assert(rStmt != NULL);
    rBody = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, rStmt);
    rFn->body = rBody;
    rFn->isEntry = 1;
    rejectModule.entry = rFn;
    CgIRAppendFunction(&rejectModule.functions, rFn);
    lVerifyAccept(&rejectModule);

    /* Short fill: two scalars cannot build a three-element array. */
    CgIRInitModule(&rejectModule, TestAlloc, NULL);
    rFn = CgIRNewFunction(&rejectModule, mainSymb, VoidType, &fnALoc);
    assert(rFn != NULL);
    rArgs = NULL;
    CgIRAppendExpr(&rArgs, CgIRNewConstant(&rejectModule, floatType,
                                           &constLoc, &vZero));
    CgIRAppendExpr(&rArgs, CgIRNewConstant(&rejectModule, floatType,
                                           &constLoc, &vOne));
    rValue = CgIRNewConstruct(&rejectModule, &array3Type, &ctorLoc, rArgs);
    assert(rValue != NULL);
    rStmt = CgIRNewExprStmt(&rejectModule, &retLoc, rValue);
    assert(rStmt != NULL);
    rBody = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, rStmt);
    rFn->body = rBody;
    rFn->isEntry = 1;
    rejectModule.entry = rFn;
    CgIRAppendFunction(&rejectModule.functions, rFn);
    lVerifyReject(&rejectModule, CGIR_VERIFY_OPERAND);

    /*
     * Production-shaped method symbol: FUNCTION-typed with the result
     * in fun.rettype and the implicit receiver prepended to
     * details.fun.params, exactly as support.c's
     * lSynthesizeMethodReceiver builds real methods.  A conforming
     * dispatch -- receiver field carrying the ifaceA object, one
     * declared float formal beyond the receiver, call result equal to
     * fun.rettype -- must verify clean.
     */
    memset(&prodFnType, 0, sizeof(prodFnType));
    prodFnType.properties = TYPE_CATEGORY_FUNCTION;
    prodFnType.fun.rettype = floatType;
    prodParamLast.next = NULL;
    prodParamLast.type = floatType;
    prodParamFirst.next = &prodParamLast;
    prodParamFirst.type = &ifaceAType;
    prodFnType.fun.paramtypes = &prodParamFirst;
    prodEvalSymb = lMakeSymbol(FUNCTION_S, "eval", &prodFnType);
    assert(prodEvalSymb != NULL);
    prodEvalSymb->details.fun.isMethod = 1;
    prodEvalSymb->details.fun.ownerType = &ifaceAType;
    prodReceiverSymb = lMakeSymbol(VARIABLE_S, "$this", &ifaceAType);
    prodXFormalSymb = lMakeSymbol(VARIABLE_S, "x", floatType);
    assert(prodReceiverSymb != NULL && prodXFormalSymb != NULL);
    prodReceiverSymb->next = prodXFormalSymb;
    prodXFormalSymb->next = NULL;
    prodEvalSymb->details.fun.params = prodReceiverSymb;

    CgIRInitModule(&rejectModule, TestAlloc, NULL);
    rDecl = CgIRNewDecl(&rejectModule, objSymb, objSymb->name, &ifaceAType,
                        CGIR_STORAGE_UNIFORM, CGIR_DOMAIN_UNIFORM, 0, NULL,
                        &paramLoc);
    assert(rDecl != NULL);
    CgIRAppendDecl(&rejectModule.globals, rDecl);
    rFn = CgIRNewFunction(&rejectModule, mainSymb, float4Type, &fnALoc);
    assert(rFn != NULL);
    rValue = CgIRNewSymbol(&rejectModule, &ifaceAType, &constLoc, objSymb);
    rLeft = CgIRNewConstant(&rejectModule, floatType, &constLoc, &vZero);
    assert(rValue != NULL && rLeft != NULL);
    rArgs = NULL;
    CgIRAppendExpr(&rArgs, rLeft);
    interfaceExpr = CgIRNewInterfaceCall(&rejectModule, floatType,
                                         &paramLoc, prodEvalSymb, rValue,
                                         rArgs);
    assert(interfaceExpr != NULL);
    rStmt = CgIRNewExprStmt(&rejectModule, &retLoc, interfaceExpr);
    rBody = CgIRNewBlockStmt(&rejectModule, &blockLoc);
    assert(rStmt != NULL && rBody != NULL);
    CgIRAppendStmt(&rBody->u.block, rStmt);
    rFn->body = rBody;
    rFn->isEntry = 1;
    rejectModule.entry = rFn;
    CgIRAppendFunction(&rejectModule.functions, rFn);
    lVerifyAccept(&rejectModule);

    /* The same dispatch advertising a result that disagrees with
     * fun.rettype is rejected at the dispatch node itself. */
    interfaceExpr->type = float2Type;
    memset(&verifyDiagnostic, 0, sizeof(verifyDiagnostic));
    assert(!CgIRVerifyModule(&rejectModule, &verifyDiagnostic));
    assert(verifyDiagnostic.reason == CGIR_VERIFY_INTERFACE);
    assert(verifyDiagnostic.node == interfaceExpr);

    /////////////////// Scenario 6: reachability graph //////////////////

    /*
     * The same shape as tests/cg20/ir/reachable.cg: main calls helperA
     * passing the referenced uniform g_tint, helperA calls helperB, and
     * one helper plus one global are never referenced.  Discovery adds
     * the entry first and then visits in sourceOrdinal order; each
     * node's witness is its first parent edge.
     */

    reachMainSymb = lReachFunction("reachMain", 30, NULL);
    reachASymb = lReachFunction("reachHelperA", 31, NULL);
    /* helperB carries an empty statement body: lReachFunction's third
     * parameter is the one CALL EXPRESSION its body makes, so a bare
     * statement must be installed directly rather than wrapped as if
     * it were an expression. */
    reachBSymb = lReachFunction("reachHelperB", 32, NULL);
    reachBSymb->details.fun.statements =
        lReachBlockStmtNode(lReachExprStmtNode(NULL));
    reachUnusedSymb = lReachFunction("reachUnused", 33, NULL);
    /* Globals must live in the real global scope for reachability to
     * recognize them; AddSymbol inserts into its reversed-atom tree. */
    reachTintSymb = AddSymbol(&fnALoc, GlobalScope,
                              LookUpAddString(atable, "reachTint"),
                              float4Type, VARIABLE_S);
    reachSpareSymb = AddSymbol(&fnALoc, GlobalScope,
                               LookUpAddString(atable, "reachSpare"),
                               float4Type, VARIABLE_S);
    assert(reachMainSymb != NULL && reachASymb != NULL && reachBSymb != NULL);
    assert(reachUnusedSymb != NULL);
    assert(reachTintSymb != NULL && reachSpareSymb != NULL);
    reachTintSymb->sourceOrdinal = 20;
    reachSpareSymb->sourceOrdinal = 21;

    /* main(reachTint): helperA(g_tint) -- the call's argument reads the
     * uniform, so both helperA and the uniform hang off main. */
    {
        expr *call;
        expr *args;

        args = lReachArgNode(lReachSymbNode(reachTintSymb), NULL);
        call = lReachCallNode(reachASymb, args);
        reachMainSymb->details.fun.statements =
            lReachBlockStmtNode(lReachExprStmtNode(call));
    }
    /* helperA: helperB(g_tint) -- the deepest call also reads it. */
    {
        expr *call;
        expr *args;

        args = lReachArgNode(lReachSymbNode(reachTintSymb), NULL);
        call = lReachCallNode(reachBSymb, args);
        reachASymb->details.fun.statements =
            lReachBlockStmtNode(lReachExprStmtNode(call));
    }

    memset(&reachGraph, 0xa5, sizeof(reachGraph));
    assert(CgReachBuild(NULL, &reachGraph) == 0);

    memset(&reachGraph, 0, sizeof(reachGraph));
    assert(CgReachBuild(reachMainSymb, &reachGraph));

    /* Exactly the reachable set: entry, two callees, one uniform. */
    assert(CgReachContainsSymbol(&reachGraph, reachMainSymb));
    assert(CgReachContainsSymbol(&reachGraph, reachASymb));
    assert(CgReachContainsSymbol(&reachGraph, reachBSymb));
    assert(CgReachContainsSymbol(&reachGraph, reachTintSymb));
    assert(!CgReachContainsSymbol(&reachGraph, reachUnusedSymb));
    assert(!CgReachContainsSymbol(&reachGraph, reachSpareSymb));
    assert(!CgReachContainsSymbol(&reachGraph, shadeSymb));

    /* Entry first; nodes appear in discovery order.  Expanding main
     * discovers helperA then the uniform; the next expansion picks the
     * pending symbol with the smallest sourceOrdinal (the uniform,
     * 20, before helperA, 31), whose turn adds helperB. */
    assert(reachGraph.nodeCount == 4);
    assert(reachGraph.nodes[0].symbol == reachMainSymb);
    assert(reachGraph.nodes[1].symbol == reachASymb);
    assert(reachGraph.nodes[2].symbol == reachTintSymb);
    assert(reachGraph.nodes[3].symbol == reachBSymb);

    /* Witnesses: the entry has no parent; everything else records its
     * FIRST discoverer as the parent edge. */
    reachEdge = CgReachWitness(&reachGraph, reachMainSymb);
    assert(reachEdge != NULL && reachEdge->from == NULL &&
           reachEdge->to == reachMainSymb);
    reachEdge = CgReachWitness(&reachGraph, reachASymb);
    assert(reachEdge != NULL && reachEdge->from == reachMainSymb &&
           reachEdge->to == reachASymb);
    reachEdge = CgReachWitness(&reachGraph, reachBSymb);
    assert(reachEdge != NULL && reachEdge->from == reachASymb &&
           reachEdge->to == reachBSymb);
    reachEdge = CgReachWitness(&reachGraph, reachTintSymb);
    assert(reachEdge != NULL && reachEdge->from == reachMainSymb &&
           reachEdge->to == reachTintSymb);
    assert(CgReachWitness(&reachGraph, reachUnusedSymb) == NULL);
    assert(CgReachWitness(&reachGraph, NULL) == NULL);

    CgReachDestroy(&reachGraph);

    /* Recursive cycles terminate without infinite traversal; recursion
     * stays a later profile decision, so both frames stay in the set. */
    cycXSymb = lReachFunction("cycleX", 40, NULL);
    cycYSymb = lReachFunction("cycleY", 41, NULL);
    assert(cycXSymb != NULL && cycYSymb != NULL);
    cycXSymb->details.fun.statements = lReachBlockStmtNode(
        lReachExprStmtNode(lReachCallNode(cycYSymb, NULL)));
    cycYSymb->details.fun.statements = lReachBlockStmtNode(
        lReachExprStmtNode(lReachCallNode(cycXSymb, NULL)));

    memset(&cycleGraph, 0, sizeof(cycleGraph));
    assert(CgReachBuild(cycXSymb, &cycleGraph));
    assert(CgReachContainsSymbol(&cycleGraph, cycXSymb));
    assert(CgReachContainsSymbol(&cycleGraph, cycYSymb));
    assert(cycleGraph.nodeCount == 2);
    assert(cycleGraph.nodes[0].symbol == cycXSymb);
    assert(cycleGraph.nodes[1].symbol == cycYSymb);
    reachEdge = CgReachWitness(&cycleGraph, cycXSymb);
    assert(reachEdge != NULL && reachEdge->from == NULL &&
           reachEdge->to == cycXSymb);
    reachEdge = CgReachWitness(&cycleGraph, cycYSymb);
    assert(reachEdge != NULL && reachEdge->from == cycXSymb &&
           reachEdge->to == cycYSymb);
    CgReachDestroy(&cycleGraph);

    /////////////////// Scenario 7: group-write value temps ////////////////

    /*
     * `m._m00_m11 = f()` fans out into one scalar store per component.
     * A side-effecting VALUE must move through its own synthesized
     * $cglmprN temporary exactly like a side-effecting OBJECT, so no
     * store references an effecting subtree; a pure value keeps today's
     * sharing.  Both lowered modules must verify clean.
     */

    {
        Type *mat2Type;
        Symbol *lowerMainSymb;
        Symbol *gmSymb;
        Symbol *geSymb;
        Scope *fnScope;
        expr *groupWrite;
        CgReachGraph reach;
        CgIRModule module;
        CgIRLowerContext context;
        CgIRFunction *entry;
        CgIRStmt *s0, *s1, *s2, *s3, *s4, *s5, *s6;
        CgIRDecl *tempDecl;
        CgIRExpr *tempRef;
        CgIRExpr *storeValue;
        CgIRExpr *targets[2];
        int coords[2][2];
        int ii;

        mat2Type = GetStandardTypeKind(CG_SCALAR_FLOAT, 2, 2);
        assert(mat2Type != UndefinedType);

        lowerMainSymb = lMakeSymbol(FUNCTION_S, "lowerMain", VoidType);
        assert(lowerMainSymb != NULL);

        /* The function's local scope mirrors parser.y: a pushed scope
         * whose funindex claims the body's locals. */
        PushScope(NewScope());
        CurrentScope->funindex = ++NextFunctionIndex;
        fnScope = CurrentScope;
        gmSymb = AddSymbol(&fnALoc, fnScope,
                           LookUpAddString(atable, "gm"), mat2Type,
                           VARIABLE_S);
        geSymb = AddSymbol(&fnALoc, fnScope,
                           LookUpAddString(atable, "ge"), floatType,
                           VARIABLE_S);
        assert(gmSymb != NULL && geSymb != NULL);
        PopScope();
        lowerMainSymb->details.fun.locals = fnScope;

        /* m._m00_m11 = ge; with ge marked side-effecting: mask16 packs
         * (row<<2|col) per nibble -- 0x00 | (0x05 << 4) -- over two
         * selected components. */
        groupWrite = lAssignNode(
            lGroupSelectorNode(lReachSymbNode(gmSymb), 0x50, 2,
                               float2Type),
            lReachSymbNode(geSymb), floatType);
        assert(groupWrite != NULL);
        groupWrite->bin.right->common.HasSideEffects = 1;
        lowerMainSymb->details.fun.statements =
            lReachBlockStmtNode(lReachExprStmtNode(groupWrite));

        memset(&reach, 0, sizeof(reach));
        assert(CgReachBuild(lowerMainSymb, &reach));
        CgIRInitModule(&module, PlainAlloc, NULL);
        context.module = &module;
        context.reach = &reach;
        memset(&context.verifyDiagnostic, 0,
               sizeof(context.verifyDiagnostic));
        assert(CgIRLowerProgram(&context, GlobalScope, lowerMainSymb));

        entry = module.entry;
        assert(entry != NULL && entry->symbol == lowerMainSymb);

        /* Statement shape: two source-local DECLs, then the function's
         * BLOCK_STMT as one nested IR block holding the temporary's
         * DECL, its one effecting move, and two stores. */
        s0 = entry->body->u.block;
        assert(s0 != NULL && s0->kind == CGIR_STMT_DECL);
        s1 = s0->next;
        assert(s1 != NULL && s1->kind == CGIR_STMT_DECL);
        s2 = s1->next;
        assert(s2 != NULL && s2->kind == CGIR_STMT_BLOCK);
        assert(s2->next == NULL);
        s3 = s2->u.block;
        assert(s3 != NULL && s3->kind == CGIR_STMT_DECL);
        tempDecl = s3->u.decl;
        assert(tempDecl != NULL && tempDecl->symbol != NULL);
        assert(strncmp(GetAtomString(atable, tempDecl->symbol->name),
                       "$cglmpr", 7) == 0);
        s4 = s3->next;
        assert(s4 != NULL && s4->kind == CGIR_STMT_EXPR);
        s5 = s4->next;
        assert(s5 != NULL && s5->kind == CGIR_STMT_EXPR);
        assert(s5->next != NULL &&
               s5->next->kind == CGIR_STMT_EXPR);
        assert(s5->next->next == NULL);

        /* The move evaluates the effecting value exactly once into the
         * temporary. */
        assert(s4->u.expression->kind == CGIR_EXPR_ASSIGN);
        tempRef = s4->u.expression->u.assign.target;
        assert(tempRef != NULL && tempRef->kind == CGIR_EXPR_SYMBOL);
        assert(tempRef->u.symbol == tempDecl->symbol);
        assert(tempRef->isLvalue);
        assert(tempRef->sideEffects == 0);
        storeValue = s4->u.expression->u.assign.value;
        assert(storeValue->kind == CGIR_EXPR_SYMBOL);
        assert(storeValue->u.symbol == geSymb);
        assert(storeValue->sideEffects == 1);

        /* Both stores read the same effect-free temporary. */
        s6 = s5->next;
        assert(s5->u.expression->kind == CGIR_EXPR_ASSIGN);
        assert(s6->u.expression->kind == CGIR_EXPR_ASSIGN);
        assert(s5->u.expression->u.assign.value ==
               s6->u.expression->u.assign.value);
        storeValue = s5->u.expression->u.assign.value;
        assert(storeValue->kind == CGIR_EXPR_SYMBOL);
        assert(storeValue->u.symbol == tempDecl->symbol);
        assert(storeValue->sideEffects == 0);

        /* Store components in mask order: m[0][0], then m[1][1]. */
        targets[0] = s5->u.expression->u.assign.target;
        targets[1] = s6->u.expression->u.assign.target;
        coords[0][0] = 0;
        coords[0][1] = 0;
        coords[1][0] = 1;
        coords[1][1] = 1;
        for (ii = 0; ii < 2; ii++) {
            CgIRExpr *outer = targets[ii];
            CgIRExpr *inner;

            assert(outer != NULL && outer->kind == CGIR_EXPR_INDEX);
            assert(outer->isLvalue);
            inner = outer->u.index.object;
            assert(inner != NULL && inner->kind == CGIR_EXPR_INDEX);
            assert(inner->u.index.object->kind == CGIR_EXPR_SYMBOL);
            assert(inner->u.index.object->u.symbol == gmSymb);
            assert(inner->u.index.index->kind == CGIR_EXPR_CONSTANT);
            assert(inner->u.index.index->u.constant.value.i ==
                   coords[ii][0]);
            assert(outer->u.index.index->kind == CGIR_EXPR_CONSTANT);
            assert(outer->u.index.index->u.constant.value.i ==
                   coords[ii][1]);
        }

        /* Visibility authority: the temporary declares among locals. */
        {
            CgIRDecl *decl;
            int found = 0;

            for (decl = entry->locals; decl != NULL; decl = decl->next) {
                if (decl->symbol == tempDecl->symbol)
                    found = 1;
            }
            assert(found);
        }
        lVerifyAccept(&module);
        CgReachDestroy(&reach);

        /* Control: a PURE value keeps today's shape -- no temporary
         * appears and both stores share one effect-free subtree. */
        {
            Symbol *pureMainSymb;
            Symbol *gpSymb;
            Symbol *gzSymb;
            Scope *pureScope;
            expr *pureWrite;
            stmt *pureBody;
            CgReachGraph pureReach;
            CgIRModule pureModule;
            CgIRLowerContext pureContext;
            CgIRFunction *pureEntry;
            CgIRStmt *p0, *p1, *p2, *p3;
            CgIRDecl *decl;
            int localCount = 0;

            pureMainSymb = lMakeSymbol(FUNCTION_S, "lowerPure", VoidType);
            assert(pureMainSymb != NULL);
            PushScope(NewScope());
            CurrentScope->funindex = ++NextFunctionIndex;
            pureScope = CurrentScope;
            gpSymb = AddSymbol(&fnALoc, pureScope,
                               LookUpAddString(atable, "gp"), mat2Type,
                               VARIABLE_S);
            gzSymb = AddSymbol(&fnALoc, pureScope,
                               LookUpAddString(atable, "gz"), floatType,
                               VARIABLE_S);
            assert(gpSymb != NULL && gzSymb != NULL);
            PopScope();
            pureMainSymb->details.fun.locals = pureScope;

            pureWrite = lAssignNode(
                lGroupSelectorNode(lReachSymbNode(gpSymb), 0x50, 2,
                                   float2Type),
                lReachSymbNode(gzSymb), floatType);
            assert(pureWrite != NULL);
            pureBody = lReachBlockStmtNode(
                lReachExprStmtNode(pureWrite));
            pureMainSymb->details.fun.statements = pureBody;

            memset(&pureReach, 0, sizeof(pureReach));
            assert(CgReachBuild(pureMainSymb, &pureReach));
            CgIRInitModule(&pureModule, PlainAlloc, NULL);
            pureContext.module = &pureModule;
            pureContext.reach = &pureReach;
            memset(&pureContext.verifyDiagnostic, 0,
                   sizeof(pureContext.verifyDiagnostic));
            assert(CgIRLowerProgram(&pureContext, GlobalScope,
                                    pureMainSymb));

            pureEntry = pureModule.entry;
            assert(pureEntry != NULL &&
                   pureEntry->symbol == pureMainSymb);

            /* Exactly the source locals plus the body block, whose
             * inner list holds just the two stores: nothing
             * synthesized. */
            p0 = pureEntry->body->u.block;
            assert(p0 != NULL && p0->kind == CGIR_STMT_DECL);
            p1 = p0->next;
            assert(p1 != NULL && p1->kind == CGIR_STMT_DECL);
            p2 = p1->next;
            assert(p2 != NULL && p2->kind == CGIR_STMT_BLOCK);
            assert(p2->next == NULL);
            p3 = p2->u.block;
            assert(p3 != NULL && p3->kind == CGIR_STMT_EXPR);
            assert(p3->next != NULL &&
                   p3->next->kind == CGIR_STMT_EXPR);
            assert(p3->next->next == NULL);

            p0 = p3;
            p1 = p3->next;
            assert(p0->u.expression->kind == CGIR_EXPR_ASSIGN);
            assert(p1->u.expression->kind == CGIR_EXPR_ASSIGN);
            assert(p0->u.expression->u.assign.value ==
                   p1->u.expression->u.assign.value);
            storeValue = p0->u.expression->u.assign.value;
            assert(storeValue->kind == CGIR_EXPR_SYMBOL);
            assert(storeValue->u.symbol == gzSymb);
            assert(storeValue->sideEffects == 0);

            for (decl = pureEntry->locals; decl != NULL;
                 decl = decl->next)
            {
                localCount++;
                assert(strncmp(GetAtomString(atable, decl->symbol->name),
                               "$cglmpr", 7) != 0);
            }
            assert(localCount == 2);
            lVerifyAccept(&pureModule);
            CgReachDestroy(&pureReach);
        }
    }

    /////////////////// Scenario 8: normalized printing golden /////////////

    /*
     * One uniform, one struct instance read through a member
     * selection, one interface object dispatched by method identity,
     * one helper reached by its resolved callee, one intrinsic, an if
     * with both arms, and a loop carrying a break.  CgIRPrintModule
     * must refuse an unverified module without writing a byte, and
     * otherwise emit the literal golden text below byte-for-byte.
     */

    {
        static const char *lGoldenText =
            "uniform float4 tint : COLOR;\n"
            "uniform struct Material surface;\n"
            "uniform interface Evaluator obj;\n"
            "\n"
            "float4 main(varying float4 position : POSITION)\n"
            "{\n"
            "  float4 acc = float4(0.000000);\n"
            "  int steps = 0;\n"
            "  acc = tint;\n"
            "  surface.gloss = 0.500000;\n"
            "  acc = (acc + obj.eval(acc));\n"
            "  if (true)\n"
            "  {\n"
            "    steps = (steps + 1);\n"
            "  }\n"
            "  else\n"
            "  {\n"
            "    acc = (-acc);\n"
            "  }\n"
            "  while (true)\n"
            "  {\n"
            "    steps = (steps + 1);\n"
            "    break;\n"
            "  }\n"
            "  acc = float4(dot(acc.xy, acc.xy), 0.000000, 0.000000, "
                                                            "1.000000);\n"
            "  scale(acc.x);\n"
            "  return acc;\n"
            "}\n"
            "\n"
            "float scale(float s)\n"
            "{\n"
            "  return s * 0.500000;\n"
            "}\n";
        Type goldMatType;
        Type goldIfaceType;
        Type goldEvalFnType;
        Scope goldMatScope;
        TypeList goldEvalParams;
        TypeList goldDotFirst;
        TypeList goldDotLast;
        CgIntrinsicSignature goldDotSignature;
        CgIRVerifyDiagnostic goldDiag;
        Symbol *goldTintSymb;
        Symbol *goldAlbedoSymb;
        Symbol *goldGlossSymb;
        Symbol *goldSurfaceSymb;
        Symbol *goldObjSymb;
        Symbol *goldEvalSymb;
        Symbol *goldReceiverSymb;
        Symbol *goldVSymb;
        Symbol *goldScaleSymb;
        Symbol *goldSSymb;
        Symbol *goldAccSymb;
        Symbol *goldStepsSymb;
        CgNumericValue goldIntOne;
        CgIRModule goldModule;
        CgIRModule goldBadModule;
        CgIRFunction *goldMainFn;
        CgIRFunction *goldScaleFn;
        CgIRFunction *goldBadFn;
        CgIRDecl *goldTintDecl;
        CgIRDecl *goldSurfaceDecl;
        CgIRDecl *goldObjDecl;
        CgIRDecl *goldPosDecl;
        CgIRDecl *goldSDecl;
        CgIRDecl *goldAccDecl;
        CgIRDecl *goldStepsDecl;
        CgIRExpr *goldTmp;
        CgIRExpr *goldTarget;
        CgIRExpr *goldValue;
        CgIRExpr *goldArgs;
        CgIRExpr *goldDotCall;
        CgIRStmt *goldList;
        CgIRStmt *goldStmt;
        CgIRStmt *goldThenBlock;
        CgIRStmt *goldElseBlock;
        CgIRStmt *goldLoopBody;
        FILE *goldFile;
        char *goldText;
        long goldLength;
        size_t goldRead;

        /* Local types: a two-member struct, an interface whose method
         * returns float4, and a dot(float, float) catalog signature --
         * all hand-built like Scenario 5's fixtures. */
        memset(&goldMatType, 0, sizeof(goldMatType));
        goldMatType.properties = TYPE_CATEGORY_STRUCT;
        goldMatType.str.tag = LookUpAddString(atable, "Material");
        memset(&goldMatScope, 0, sizeof(goldMatScope));
        goldAlbedoSymb = lMakeSymbol(VARIABLE_S, "albedo", float4Type);
        goldGlossSymb = lMakeSymbol(VARIABLE_S, "gloss", floatType);
        assert(goldAlbedoSymb != NULL && goldGlossSymb != NULL);
        goldAlbedoSymb->next = goldGlossSymb;
        goldGlossSymb->next = NULL;
        goldMatScope.params = goldAlbedoSymb;
        goldMatType.str.members = &goldMatScope;

        memset(&goldIfaceType, 0, sizeof(goldIfaceType));
        goldIfaceType.properties = TYPE_CATEGORY_INTERFACE;
        goldIfaceType.iface.tag = LookUpAddString(atable, "Evaluator");

        memset(&goldEvalFnType, 0, sizeof(goldEvalFnType));
        goldEvalFnType.properties = TYPE_CATEGORY_FUNCTION;
        goldEvalFnType.fun.rettype = float4Type;
        goldEvalParams.next = NULL;
        goldEvalParams.type = &goldIfaceType;
        goldEvalFnType.fun.paramtypes = &goldEvalParams;
        goldEvalSymb = lMakeSymbol(FUNCTION_S, "eval", &goldEvalFnType);
        assert(goldEvalSymb != NULL);
        goldEvalSymb->details.fun.isMethod = 1;
        goldEvalSymb->details.fun.ownerType = &goldIfaceType;
        goldReceiverSymb = lMakeSymbol(VARIABLE_S, "$this",
                                       &goldIfaceType);
        goldVSymb = lMakeSymbol(VARIABLE_S, "v", float4Type);
        assert(goldReceiverSymb != NULL && goldVSymb != NULL);
        goldReceiverSymb->next = goldVSymb;
        goldVSymb->next = NULL;
        goldEvalSymb->details.fun.params = goldReceiverSymb;

        goldTintSymb = lMakeSymbol(VARIABLE_S, "tint", float4Type);
        goldSurfaceSymb = lMakeSymbol(VARIABLE_S, "surface", &goldMatType);
        goldObjSymb = lMakeSymbol(VARIABLE_S, "obj", &goldIfaceType);
        goldScaleSymb = lMakeSymbol(FUNCTION_S, "scale", floatType);
        goldSSymb = lMakeSymbol(VARIABLE_S, "s", floatType);
        goldAccSymb = lMakeSymbol(VARIABLE_S, "acc", float4Type);
        goldStepsSymb = lMakeSymbol(VARIABLE_S, "steps", intType);
        assert(goldTintSymb != NULL && goldSurfaceSymb != NULL);
        assert(goldObjSymb != NULL && goldScaleSymb != NULL);
        assert(goldSSymb != NULL && goldAccSymb != NULL);
        assert(goldStepsSymb != NULL);

        memset(&goldIntOne, 0, sizeof(goldIntOne));
        goldIntOne.kind = CG_SCALAR_INT;
        goldIntOne.value.i = 1;

        /* dot(float2, float2) -> float local signature. */
        memset(&goldDiag, 0, sizeof(goldDiag));
        goldDotLast.next = NULL;
        goldDotLast.type = float2Type;
        goldDotFirst.next = &goldDotLast;
        goldDotFirst.type = float2Type;
        goldDotSignature.intrinsic = CG_INTRINSIC_DOT;
        goldDotSignature.name = "dot";
        goldDotSignature.result = floatType;
        goldDotSignature.parameters = &goldDotFirst;
        goldDotSignature.flags = CG_INTRINSIC_PURE | CG_INTRINSIC_FOLDABLE;

        CgIRInitModule(&goldModule, TestAlloc, NULL);
        goldModule.profile = &genericIdentity;

        /* Globals in source order: a bound uniform, a struct instance,
         * and an interface object. */
        goldTintDecl = CgIRNewDecl(&goldModule, goldTintSymb,
                                   goldTintSymb->name, float4Type,
                                   CGIR_STORAGE_UNIFORM,
                                   CGIR_DOMAIN_UNIFORM,
                                   LookUpAddString(atable, "COLOR"), NULL,
                                   &paramLoc);
        goldSurfaceDecl = CgIRNewDecl(&goldModule, goldSurfaceSymb,
                                      goldSurfaceSymb->name, &goldMatType,
                                      CGIR_STORAGE_UNIFORM,
                                      CGIR_DOMAIN_UNIFORM, 0, NULL,
                                      &paramLoc);
        goldObjDecl = CgIRNewDecl(&goldModule, goldObjSymb,
                                  goldObjSymb->name, &goldIfaceType,
                                  CGIR_STORAGE_UNIFORM,
                                  CGIR_DOMAIN_UNIFORM, 0, NULL, &paramLoc);
        assert(goldTintDecl != NULL && goldSurfaceDecl != NULL);
        assert(goldObjDecl != NULL);
        CgIRAppendDecl(&goldModule.globals, goldTintDecl);
        CgIRAppendDecl(&goldModule.globals, goldSurfaceDecl);
        CgIRAppendDecl(&goldModule.globals, goldObjDecl);

        /* Entry first, then the helper -- production lowering order. */
        goldMainFn = CgIRNewFunction(&goldModule, mainSymb, float4Type,
                                     &fnBLoc);
        goldScaleFn = CgIRNewFunction(&goldModule, goldScaleSymb,
                                      floatType, &fnALoc);
        assert(goldMainFn != NULL && goldScaleFn != NULL);
        goldPosDecl = CgIRNewDecl(&goldModule, positionSymb,
                                  positionSymb->name, float4Type,
                                  CGIR_STORAGE_NONE, CGIR_DOMAIN_VARYING,
                                  LookUpAddString(atable, "POSITION"),
                                  NULL, &paramLoc);
        goldSDecl = CgIRNewDecl(&goldModule, goldSSymb, goldSSymb->name,
                                floatType, CGIR_STORAGE_NONE,
                                CGIR_DOMAIN_NONE, 0, NULL, &paramLoc);
        goldAccDecl = CgIRNewDecl(&goldModule, goldAccSymb,
                                  goldAccSymb->name, float4Type,
                                  CGIR_STORAGE_NONE, CGIR_DOMAIN_NONE,
                                  0, NULL, &paramLoc);
        goldStepsDecl = CgIRNewDecl(&goldModule, goldStepsSymb,
                                    goldStepsSymb->name, intType,
                                    CGIR_STORAGE_NONE, CGIR_DOMAIN_NONE,
                                    0, NULL, &paramLoc);
        assert(goldPosDecl != NULL && goldSDecl != NULL);
        assert(goldAccDecl != NULL && goldStepsDecl != NULL);
        CgIRAppendDecl(&goldMainFn->parameters, goldPosDecl);
        CgIRAppendDecl(&goldScaleFn->parameters, goldSDecl);

        /* float4 acc = float4(0.0); */
        goldTmp = CgIRNewConstant(&goldModule, floatType, &constLoc,
                                  &vZero);
        assert(goldTmp != NULL);
        goldAccDecl->initializer = CgIRNewConstruct(&goldModule,
                                                    float4Type, &ctorLoc,
                                                    goldTmp);
        assert(goldAccDecl->initializer != NULL);
        goldStmt = CgIRNewDeclStmt(&goldModule, &blockLoc, goldAccDecl);
        assert(goldStmt != NULL);
        goldList = NULL;
        CgIRAppendStmt(&goldList, goldStmt);

        /* int steps = 0; */
        goldTmp = CgIRNewConstant(&goldModule, intType, &constLoc,
                                  &vIntZero);
        assert(goldTmp != NULL);
        goldStepsDecl->initializer = goldTmp;
        goldStmt = CgIRNewDeclStmt(&goldModule, &blockLoc, goldStepsDecl);
        assert(goldStmt != NULL);
        CgIRAppendStmt(&goldList, goldStmt);

        /* acc = tint; */
        goldTarget = CgIRNewSymbol(&goldModule, float4Type, &constLoc,
                                   goldAccSymb);
        goldValue = CgIRNewSymbol(&goldModule, float4Type, &constLoc,
                                  goldTintSymb);
        assert(goldTarget != NULL && goldValue != NULL);
        goldTarget->isLvalue = 1;
        goldValue->isLvalue = 1;
        goldTarget = CgIRNewAssign(&goldModule, float4Type, &blockLoc,
                                   CGIR_OP_ASSIGN, goldTarget, goldValue);
        assert(goldTarget != NULL);
        goldStmt = CgIRNewExprStmt(&goldModule, &retLoc, goldTarget);
        assert(goldStmt != NULL);
        CgIRAppendStmt(&goldList, goldStmt);

        /* surface.gloss = 0.5; */
        goldValue = CgIRNewSymbol(&goldModule, &goldMatType, &constLoc,
                                  goldSurfaceSymb);
        assert(goldValue != NULL);
        goldValue->isLvalue = 1;
        goldTarget = CgIRNewMember(&goldModule, floatType, &ctorLoc,
                                   goldValue, goldGlossSymb);
        assert(goldTarget != NULL);
        goldTarget->isLvalue = 1;
        goldTmp = CgIRNewConstant(&goldModule, floatType, &constLoc,
                                  &vHalf);
        assert(goldTmp != NULL);
        goldValue = CgIRNewAssign(&goldModule, floatType, &ctorLoc,
                                  CGIR_OP_ASSIGN, goldTarget, goldTmp);
        assert(goldValue != NULL);
        goldStmt = CgIRNewExprStmt(&goldModule, &retLoc, goldValue);
        assert(goldStmt != NULL);
        CgIRAppendStmt(&goldList, goldStmt);

        /* acc = acc + obj.eval(acc); */
        goldValue = CgIRNewSymbol(&goldModule, &goldIfaceType, &constLoc,
                                  goldObjSymb);
        goldTmp = CgIRNewSymbol(&goldModule, float4Type, &constLoc,
                                goldAccSymb);
        assert(goldValue != NULL && goldTmp != NULL);
        goldValue->isLvalue = 1;
        goldTmp->isLvalue = 1;
        goldArgs = NULL;
        CgIRAppendExpr(&goldArgs, goldTmp);
        goldValue = CgIRNewInterfaceCall(&goldModule, float4Type,
                                         &ctorLoc, goldEvalSymb,
                                         goldValue, goldArgs);
        assert(goldValue != NULL);
        goldTmp = CgIRNewSymbol(&goldModule, float4Type, &constLoc,
                                goldAccSymb);
        assert(goldTmp != NULL);
        goldTmp->isLvalue = 1;
        goldValue = CgIRNewBinary(&goldModule, float4Type, &ctorLoc,
                                  CGIR_OP_ADD, goldTmp, goldValue);
        assert(goldValue != NULL);
        goldTarget = CgIRNewSymbol(&goldModule, float4Type, &constLoc,
                                   goldAccSymb);
        assert(goldTarget != NULL);
        goldTarget->isLvalue = 1;
        goldTarget = CgIRNewAssign(&goldModule, float4Type, &blockLoc,
                                   CGIR_OP_ASSIGN, goldTarget, goldValue);
        assert(goldTarget != NULL);
        goldStmt = CgIRNewExprStmt(&goldModule, &retLoc, goldTarget);
        assert(goldStmt != NULL);
        CgIRAppendStmt(&goldList, goldStmt);

        /* if (true) { steps = steps + 1; } else { acc = -acc; } */
        goldThenBlock = CgIRNewBlockStmt(&goldModule, &blockLoc);
        goldElseBlock = CgIRNewBlockStmt(&goldModule, &blockLoc);
        assert(goldThenBlock != NULL && goldElseBlock != NULL);
        goldTarget = CgIRNewSymbol(&goldModule, intType, &constLoc,
                                   goldStepsSymb);
        goldTmp = CgIRNewSymbol(&goldModule, intType, &constLoc,
                                goldStepsSymb);
        goldValue = CgIRNewConstant(&goldModule, intType, &constLoc,
                                    &goldIntOne);
        assert(goldTarget != NULL && goldTmp != NULL && goldValue != NULL);
        goldTarget->isLvalue = 1;
        goldTmp->isLvalue = 1;
        goldValue = CgIRNewBinary(&goldModule, intType, &ctorLoc,
                                  CGIR_OP_ADD, goldTmp, goldValue);
        assert(goldValue != NULL);
        goldValue = CgIRNewAssign(&goldModule, intType, &ctorLoc,
                                  CGIR_OP_ASSIGN, goldTarget, goldValue);
        assert(goldValue != NULL);
        goldStmt = CgIRNewExprStmt(&goldModule, &retLoc, goldValue);
        assert(goldStmt != NULL);
        CgIRAppendStmt(&goldThenBlock->u.block, goldStmt);
        goldTarget = CgIRNewSymbol(&goldModule, float4Type, &constLoc,
                                   goldAccSymb);
        goldTmp = CgIRNewSymbol(&goldModule, float4Type, &constLoc,
                                goldAccSymb);
        assert(goldTarget != NULL && goldTmp != NULL);
        goldTarget->isLvalue = 1;
        goldTmp->isLvalue = 1;
        goldValue = CgIRNewUnary(&goldModule, float4Type, &ctorLoc,
                                 CGIR_OP_NEGATE, goldTmp);
        assert(goldValue != NULL);
        goldValue = CgIRNewAssign(&goldModule, float4Type, &ctorLoc,
                                  CGIR_OP_ASSIGN, goldTarget, goldValue);
        assert(goldValue != NULL);
        goldStmt = CgIRNewExprStmt(&goldModule, &retLoc, goldValue);
        assert(goldStmt != NULL);
        CgIRAppendStmt(&goldElseBlock->u.block, goldStmt);
        goldTmp = CgIRNewConstant(&goldModule, boolType, &constLoc,
                                  &vTrue);
        assert(goldTmp != NULL);
        goldStmt = CgIRNewIfStmt(&goldModule, &retLoc, goldTmp,
                                 goldThenBlock, goldElseBlock);
        assert(goldStmt != NULL);
        CgIRAppendStmt(&goldList, goldStmt);

        /* while (true) { steps = steps + 1; break; } */
        goldLoopBody = CgIRNewBlockStmt(&goldModule, &blockLoc);
        assert(goldLoopBody != NULL);
        goldTarget = CgIRNewSymbol(&goldModule, intType, &constLoc,
                                   goldStepsSymb);
        goldTmp = CgIRNewSymbol(&goldModule, intType, &constLoc,
                                goldStepsSymb);
        goldValue = CgIRNewConstant(&goldModule, intType, &constLoc,
                                    &goldIntOne);
        assert(goldTarget != NULL && goldTmp != NULL && goldValue != NULL);
        goldTarget->isLvalue = 1;
        goldTmp->isLvalue = 1;
        goldValue = CgIRNewBinary(&goldModule, intType, &ctorLoc,
                                  CGIR_OP_ADD, goldTmp, goldValue);
        assert(goldValue != NULL);
        goldValue = CgIRNewAssign(&goldModule, intType, &ctorLoc,
                                  CGIR_OP_ASSIGN, goldTarget, goldValue);
        assert(goldValue != NULL);
        goldStmt = CgIRNewExprStmt(&goldModule, &retLoc, goldValue);
        assert(goldStmt != NULL);
        CgIRAppendStmt(&goldLoopBody->u.block, goldStmt);
        goldStmt = CgIRNewBreakStmt(&goldModule, &retLoc);
        assert(goldStmt != NULL);
        CgIRAppendStmt(&goldLoopBody->u.block, goldStmt);
        goldTmp = CgIRNewConstant(&goldModule, boolType, &constLoc,
                                  &vTrue);
        assert(goldTmp != NULL);
        goldStmt = CgIRNewWhileStmt(&goldModule, &fnALoc, goldTmp,
                                    goldLoopBody);
        assert(goldStmt != NULL);
        CgIRAppendStmt(&goldList, goldStmt);

        /* acc = float4(dot(acc.xy, acc.xy), 0.0, 0.0, 1.0); */
        goldValue = CgIRNewSymbol(&goldModule, float4Type, &constLoc,
                                  goldAccSymb);
        goldTarget = CgIRNewSymbol(&goldModule, float4Type, &constLoc,
                                   goldAccSymb);
        assert(goldValue != NULL && goldTarget != NULL);
        goldValue->isLvalue = 1;
        goldTarget->isLvalue = 1;
        goldValue = CgIRNewSwizzle(&goldModule, float2Type, &ctorLoc,
                                   goldValue, 0x4, 2);
        goldTarget = CgIRNewSwizzle(&goldModule, float2Type, &ctorLoc,
                                    goldTarget, 0x4, 2);
        assert(goldValue != NULL && goldTarget != NULL);
        goldArgs = NULL;
        CgIRAppendExpr(&goldArgs, goldValue);
        CgIRAppendExpr(&goldArgs, goldTarget);
        goldDotCall = CgIRNewIntrinsicCall(&goldModule, floatType,
                                           &fnALoc, CG_INTRINSIC_DOT,
                                           &goldDotSignature, goldArgs);
        assert(goldDotCall != NULL);
        goldArgs = NULL;
        CgIRAppendExpr(&goldArgs, goldDotCall);
        goldTmp = CgIRNewConstant(&goldModule, floatType, &constLoc,
                                  &vZero);
        assert(goldTmp != NULL);
        CgIRAppendExpr(&goldArgs, goldTmp);
        goldTmp = CgIRNewConstant(&goldModule, floatType, &constLoc,
                                  &vZero);
        assert(goldTmp != NULL);
        CgIRAppendExpr(&goldArgs, goldTmp);
        goldTmp = CgIRNewConstant(&goldModule, floatType, &constLoc,
                                  &vOne);
        assert(goldTmp != NULL);
        CgIRAppendExpr(&goldArgs, goldTmp);
        goldValue = CgIRNewConstruct(&goldModule, float4Type, &ctorLoc,
                                     goldArgs);
        assert(goldValue != NULL);
        goldTarget = CgIRNewSymbol(&goldModule, float4Type, &constLoc,
                                   goldAccSymb);
        assert(goldTarget != NULL);
        goldTarget->isLvalue = 1;
        goldTarget = CgIRNewAssign(&goldModule, float4Type, &blockLoc,
                                   CGIR_OP_ASSIGN, goldTarget, goldValue);
        assert(goldTarget != NULL);
        goldStmt = CgIRNewExprStmt(&goldModule, &retLoc, goldTarget);
        assert(goldStmt != NULL);
        CgIRAppendStmt(&goldList, goldStmt);

        /* scale(acc.x); */
        goldValue = CgIRNewSymbol(&goldModule, float4Type, &constLoc,
                                  goldAccSymb);
        assert(goldValue != NULL);
        goldValue->isLvalue = 1;
        goldValue = CgIRNewSwizzle(&goldModule, floatType, &ctorLoc,
                                   goldValue, 0x0, 1);
        assert(goldValue != NULL);
        goldArgs = NULL;
        CgIRAppendExpr(&goldArgs, goldValue);
        goldValue = CgIRNewCall(&goldModule, floatType, &retLoc,
                                goldScaleSymb, goldArgs);
        assert(goldValue != NULL);
        goldStmt = CgIRNewExprStmt(&goldModule, &retLoc, goldValue);
        assert(goldStmt != NULL);
        CgIRAppendStmt(&goldList, goldStmt);

        /* return acc; */
        goldValue = CgIRNewSymbol(&goldModule, float4Type, &constLoc,
                                  goldAccSymb);
        assert(goldValue != NULL);
        goldValue->isLvalue = 1;
        goldStmt = CgIRNewReturnStmt(&goldModule, &retLoc, goldValue);
        assert(goldStmt != NULL);
        CgIRAppendStmt(&goldList, goldStmt);

        goldStmt = CgIRNewBlockStmt(&goldModule, &blockLoc);
        assert(goldStmt != NULL);
        CgIRAppendStmt(&goldStmt->u.block, goldList);
        goldMainFn->body = goldStmt;

        /* float scale(float s) { return s * 0.5; } */
        goldTarget = CgIRNewSymbol(&goldModule, floatType, &constLoc,
                                   goldSSymb);
        goldTmp = CgIRNewConstant(&goldModule, floatType, &constLoc,
                                  &vHalf);
        assert(goldTarget != NULL && goldTmp != NULL);
        goldTarget->isLvalue = 1;
        goldValue = CgIRNewBinary(&goldModule, floatType, &ctorLoc,
                                  CGIR_OP_MULTIPLY, goldTarget, goldTmp);
        assert(goldValue != NULL);
        goldStmt = CgIRNewReturnStmt(&goldModule, &retLoc, goldValue);
        assert(goldStmt != NULL);
        goldThenBlock = CgIRNewBlockStmt(&goldModule, &blockLoc);
        assert(goldThenBlock != NULL);
        CgIRAppendStmt(&goldThenBlock->u.block, goldStmt);
        goldScaleFn->body = goldThenBlock;

        goldMainFn->isEntry = 1;
        goldModule.entry = goldMainFn;
        CgIRAppendFunction(&goldModule.functions, goldMainFn);
        CgIRAppendFunction(&goldModule.functions, goldScaleFn);
        if (! CgIRVerifyModule(&goldModule, &goldDiag)) {
            fprintf(stderr, "golden module rejected: %s at file %u line %d\n",
                    CgIRVerifyReasonName(goldDiag.reason),
                    (unsigned) goldDiag.loc.file, goldDiag.loc.line);
        }
        assert(CgIRVerifyModule(&goldModule, NULL));

        /* Refusal: an unverified module writes nothing at all.  One
         * defined function without entry selection fails OWNER
         * verification before any text is produced. */
        CgIRInitModule(&goldBadModule, TestAlloc, NULL);
        goldBadFn = CgIRNewFunction(&goldBadModule, shadeSymb,
                                    float4Type, &fnALoc);
        assert(goldBadFn != NULL);
        CgIRAppendFunction(&goldBadModule.functions, goldBadFn);
        assert(!CgIRVerifyModule(&goldBadModule, NULL));

        goldFile = tmpfile();
        assert(goldFile != NULL);
        assert(!CgIRPrintModule(goldFile, &goldBadModule));
        assert(fflush(goldFile) == 0);
        assert(ftell(goldFile) == 0);

        /* The verified module emits exactly the golden bytes. */
        assert(CgIRPrintModule(goldFile, &goldModule));
        assert(fflush(goldFile) == 0);
        assert(fseek(goldFile, 0, SEEK_END) == 0);
        goldLength = ftell(goldFile);
        rewind(goldFile);
        goldText = (char *) malloc((size_t) goldLength + 1);
        assert(goldText != NULL);
        goldRead = fread(goldText, 1, (size_t) goldLength, goldFile);
        assert(goldRead == (size_t) goldLength);
        goldText[goldLength] = '\0';
        if (goldLength != (long) strlen(lGoldenText) ||
            strcmp(goldText, lGoldenText) != 0)
        {
            fprintf(stderr,
                    "--- normalized print mismatch (%ld vs %zu bytes) ---\n"
                    "%s"
                    "-----------------------------------------------------\n",
                    goldLength, strlen(lGoldenText), goldText);
        }
        assert(goldLength == (long) strlen(lGoldenText));
        assert(strcmp(goldText, lGoldenText) == 0);
        fclose(goldFile);
    }

    /*
     * Scenario 9: geometry IR.
     *
     * Geometry modules carry a resolved stage, one owned metadata
     * record, and explicit emit/flat/restart statements whose bundles
     * the builders deep-copy while retaining the verified canonical
     * type/semantic/expression references.  The verifier enforces the
     * stage/metadata pairing, exact topology-to-count mapping, positive
     * known maximum, attribute-array placement and extent, reachability
     * of every operation from the entry, nonempty duplicate-free
     * bundles with valid locations, the geometry-only semantic
     * direction/type rules, POSITION's flat ban, and operand-free
     * restarts -- each reported as CGIR_VERIFY_GEOMETRY about the
     * offending node.
     */

    {
        CgGeoFixture geo;
        CgGeoFixture dirtyGeo;
        CgGeoFixture badGeo;
        CgIRModule scratchModule;
        CgIRModule budgetGeoModule;
        Type *wrongExtentType;
        Symbol *straySymb;
        CgIRFunction *strayFn;
        CgIRStmt *strayBody;
        CgIRStmt *strayRestart;
        CgIRGeometryValue *dupValue;
        CgIRVerifyDiagnostic geoDiag;
        CgIRGeometryInfo geoInfo;

        /* Builder contract: out-of-range stages are refused in place. */
        CgIRInitModule(&scratchModule, TestAlloc, NULL);
        assert(!CgIRSetStage(&scratchModule, (CgIRStage) 99));
        assert(scratchModule.stage == CGIR_STAGE_UNKNOWN);
        assert(CgIRSetStage(&scratchModule, CGIR_STAGE_VERTEX));
        assert(scratchModule.stage == CGIR_STAGE_VERTEX);

        lBuildGeometryFixture(&geo, TestAlloc, 1);

        /* Stage and metadata ownership: the record is a module-owned
         * deep copy, distinct from the caller's storage. */
        assert(geo.module.stage == CGIR_STAGE_GEOMETRY);
        assert(geo.module.geometry != NULL);
        assert(geo.module.geometry->inputTopology ==
               CG_GEOMETRY_INPUT_TRIANGLE);
        assert(geo.module.geometry->outputTopology ==
               CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP);
        assert(geo.module.geometry->inputVertexCount == 3);
        assert(geo.module.geometry->maxOutputVertices == 6);
        assert(geo.module.geometry->hasMaxOutputVertices == 1);
        assert(geo.module.geometry->inputLoc.file ==
               geo.infoInputLoc.file);
        assert(geo.module.geometry->inputLoc.line ==
               geo.infoInputLoc.line);
        assert(geo.module.geometry->outputLoc.line ==
               geo.infoOutputLoc.line);
        assert(geo.module.geometry->maxVerticesLoc.line ==
               geo.infoMaxLoc.line);

        /* Bundle ownership: statement-owned copies, ordered, retaining
         * the original type/expression references; the caller list is
         * untouched and stays separately linked. */
        assert(geo.emitValues != geo.callerValues);
        assert(geo.emitValues->next != geo.callerSecond);
        assert(geo.emitValues->next->next == NULL);
        assert(geo.emitValues->canonicalSemantic == geo.atomPosition);
        assert(geo.emitValues->sourceSemantic == geo.atomPosition);
        assert(geo.emitValues->type == float4Type);
        assert(geo.emitValues->value == geo.posIndex);
        assert(geo.emitValues->loc.file == geo.valueLoc.file);
        assert(geo.emitValues->loc.line == geo.valueLoc.line);
        assert(geo.emitValues->next->canonicalSemantic ==
               geo.atomTexcoord0);
        assert(geo.emitValues->next->sourceSemantic ==
               geo.atomTexCoordSource);
        assert(geo.emitValues->next->type == float2Type);
        assert(geo.emitValues->next->value == geo.uvRef);
        assert(geo.flatValues->canonicalSemantic == geo.atomColor0);
        assert(geo.flatValues->type == float4Type);
        assert(geo.flatValues->value == geo.colorRef);
        assert(geo.flatValues->next == NULL);
        assert(geo.callerValues->next == geo.callerSecond);
        assert(geo.callerSecond->next == NULL);

        /* Statement kinds, order, locations, and operand-free restart. */
        assert(geo.emit->kind == CGIR_STMT_GEOMETRY_EMIT);
        assert(geo.emit->loc.file == geo.opLoc.file);
        assert(geo.emit->loc.line == geo.opLoc.line);
        assert(geo.restart->kind == CGIR_STMT_GEOMETRY_RESTART);
        assert(geo.restart->u.geometry.values == NULL);
        assert(geo.entryFn->body->u.block == geo.callFlat);
        assert(geo.callFlat->next == geo.emit);
        assert(geo.emit->next == geo.restart);
        assert(geo.restart->next == NULL);

        /* Clean allocator module verifies... */
        lVerifyAccept(&geo.module);

        /* ...and the identical construction over a dirty allocator
         * verifies too: every node byte is builder-initialized. */
        lBuildGeometryFixture(&dirtyGeo, DirtyAlloc, 1);
        lVerifyAccept(&dirtyGeo.module);

        /* M1: an unknown stage carrying geometry metadata is
         * construction garbage. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        badGeo.module.stage = CGIR_STAGE_UNKNOWN;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == &badGeo.module);

        /* M2: missing geometry metadata on a geometry module reports
         * the module itself anchored at the entry location. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        badGeo.module.geometry = NULL;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == &badGeo.module);
        assert(geoDiag.loc.file == badGeo.fnLoc.file);
        assert(geoDiag.loc.line == badGeo.fnLoc.line);
        assert(!CgIRVerifyModule(&badGeo.module, NULL));

        /* M3: extra geometry metadata on a neutral module. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        badGeo.module.stage = CGIR_STAGE_NEUTRAL;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == &badGeo.module);

        /* M4a: input topology enum outside the valid range. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        badGeo.module.geometry->inputTopology =
            CG_GEOMETRY_INPUT_UNKNOWN;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == (const void *) badGeo.module.geometry);
        assert(geoDiag.loc.line == badGeo.infoInputLoc.line);

        /* M4b: output topology enum outside the valid range. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        badGeo.module.geometry->outputTopology = (CgGeometryOutput) 99;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == (const void *) badGeo.module.geometry);
        assert(geoDiag.loc.line == badGeo.infoOutputLoc.line);

        /* M4c: the vertex count must match the input topology
         * exactly (triangle means three). */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        badGeo.module.geometry->inputVertexCount = 4;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == (const void *) badGeo.module.geometry);
        assert(geoDiag.loc.line == badGeo.infoInputLoc.line);

        /* M5: a declared maximum of zero is never valid. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        badGeo.module.geometry->maxOutputVertices = 0;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == (const void *) badGeo.module.geometry);
        assert(geoDiag.loc.line == badGeo.infoMaxLoc.line);

        /* M6: attribute arrays live only in geometry modules -- here a
         * vertex-stage module keeps its array formal. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        assert(CgIRSetStage(&badGeo.module, CGIR_STAGE_VERTEX));
        badGeo.module.geometry = NULL;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == badGeo.arrayFormal);

        /* M7: attribute arrays carry the resolved module extent;
         * triangle input demands three, not six. */
        wrongExtentType = CgGetAttribArrayType(float4Type, 6);
        assert(wrongExtentType != UndefinedType);
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        badGeo.arrayFormal->type = wrongExtentType;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == badGeo.arrayFormal);

        /* M8a: a geometry operation inside a function no call reaches
         * is rejected at the stray operation itself. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        straySymb = lMakeSymbol(FUNCTION_S, "geoStray", VoidType);
        strayFn = CgIRNewFunction(&badGeo.module, straySymb, VoidType,
                                  &badGeo.opLoc);
        assert(strayFn != NULL);
        strayRestart = CgIRNewGeometryRestart(&badGeo.module,
                                              badGeo.opLoc);
        assert(strayRestart != NULL);
        strayBody = CgIRNewBlockStmt(&badGeo.module, &badGeo.opLoc);
        assert(strayBody != NULL);
        CgIRAppendStmt(&strayBody->u.block, strayRestart);
        strayFn->body = strayBody;
        CgIRAppendFunction(&badGeo.module.functions, strayFn);
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == strayRestart);

        /* M8b: a geometry operation under a fragment stage is rejected
         * at the operation; the attribute-array formal yields to a
         * plain float4 so placement of the OPERATION is isolated. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        assert(CgIRSetStage(&badGeo.module, CGIR_STAGE_FRAGMENT));
        badGeo.module.geometry = NULL;
        badGeo.arrayFormal->type = float4Type;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == badGeo.emit);

        /* M9: an emit bundle cannot be empty. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        badGeo.emit->u.geometry.values = NULL;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == badGeo.emit);

        /* M10: canonical semantics are unique within one bundle. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        dupValue = CgIRNewGeometryValue(&badGeo.module,
                                        badGeo.atomPosition,
                                        badGeo.atomPosition, float4Type,
                                        badGeo.colorRef, badGeo.valueLoc);
        assert(dupValue != NULL && dupValue->next == NULL);
        badGeo.emitValues->next->next = dupValue;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == dupValue);

        /* M11a: input-only semantics cannot ride an output bundle. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        badGeo.emitValues->canonicalSemantic = badGeo.atomInstanceid;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == badGeo.emitValues);

        /* M11b: output-only semantics demand scalar int; LAYER over a
         * float4 value is a type violation. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        badGeo.emitValues->canonicalSemantic = badGeo.atomLayer;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == badGeo.emitValues);

        /* M12: flatAttrib never carries POSITION. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        badGeo.flatValues->canonicalSemantic = badGeo.atomPosition;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == badGeo.flatValues);

        /* M13: restartStrip takes no operands. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        badGeo.restart->u.geometry.values = badGeo.flatValues;
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == badGeo.restart);

        /* M14: every bundle item anchors at a real source location. */
        lBuildGeometryFixture(&badGeo, TestAlloc, 1);
        memset(&badGeo.emitValues->loc, 0, sizeof(badGeo.emitValues->loc));
        memset(&geoDiag, 0, sizeof(geoDiag));
        assert(!CgIRVerifyModule(&badGeo.module, &geoDiag));
        assert(geoDiag.reason == CGIR_VERIFY_GEOMETRY);
        assert(geoDiag.node == badGeo.emitValues);

        /* Allocation failure follows the base module-failed contract:
         * the failing builder returns NULL, marks the module failed,
         * and later calls refuse without allocating. */

        memset(&geoInfo, 0, sizeof(geoInfo));
        geoInfo.inputTopology = CG_GEOMETRY_INPUT_TRIANGLE;
        geoInfo.outputTopology = CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP;
        geoInfo.inputVertexCount = 3;
        geoInfo.maxOutputVertices = 6;
        geoInfo.hasMaxOutputVertices = 1;

        CgIRInitModule(&budgetGeoModule, FailingAlloc, NULL);
        assert(CgIRSetStage(&budgetGeoModule, CGIR_STAGE_GEOMETRY));
        assert(!CgIRSetGeometryInfo(&budgetGeoModule, &geoInfo));
        assert(CgIRModuleFailed(&budgetGeoModule));
        assert(budgetGeoModule.geometry == NULL);
        assert(CgIRNewGeometryValue(&budgetGeoModule, 1, 1, float4Type,
                                    NULL, constLoc) == NULL);
        assert(CgIRModuleFailed(&budgetGeoModule));

        allocationBudget = 4;
        CgIRInitModule(&budgetGeoModule, BudgetAlloc, NULL);
        assert(CgIRSetStage(&budgetGeoModule, CGIR_STAGE_GEOMETRY));
        {
            CgIRGeometryValue *budgetV0;
            CgIRGeometryValue *budgetV1;
            CgIRStmt *budgetEmit;

            assert(CgIRSetGeometryInfo(&budgetGeoModule, &geoInfo));
            budgetV0 = CgIRNewGeometryValue(&budgetGeoModule, 10, 11,
                                            float4Type, NULL, constLoc);
            budgetV1 = CgIRNewGeometryValue(&budgetGeoModule, 12, 12,
                                            float4Type, NULL, constLoc);
            assert(budgetV0 != NULL && budgetV1 != NULL);
            budgetV0->next = budgetV1;
            /* Info plus two value nodes exhaust four allocations; the
             * emit's deep copy fails mid-list and nothing partial
             * escapes. */
            budgetEmit = CgIRNewGeometryEmit(&budgetGeoModule, budgetV0,
                                             constLoc);
            assert(budgetEmit == NULL);
            assert(CgIRModuleFailed(&budgetGeoModule));
            assert(allocationBudget == -1);
            assert(CgIRNewGeometryRestart(&budgetGeoModule,
                                          constLoc) == NULL);
            assert(allocationBudget == -1);
        }
    }

    FreeSymbolTable(Cg);
    FreeAtomTable(atable);
    return 0;
}

