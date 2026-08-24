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

    FreeSymbolTable(Cg);
    FreeAtomTable(atable);
    return 0;
}
