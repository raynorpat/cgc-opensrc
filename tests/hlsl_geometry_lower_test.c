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
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER
PRODUCTS.

IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
\****************************************************************************/

#if defined(NDEBUG)
#undef NDEBUG
#endif
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slglobals.h"
#include "cg_ir.h"
#include "hlsl_hal.h"

static void *TestAlloc(void *arg, size_t size)
{
    (void) arg;
    return calloc(1, size);
}

void SemanticError(SourceLoc *loc, int number, const char *message, ...)
{
    (void) loc; (void) number; (void) message;
}

void SemanticWarning(SourceLoc *loc, int number, const char *message, ...)
{
    (void) loc; (void) number; (void) message;
}

void SemanticNote(SourceLoc *loc, int number, const char *message, ...)
{
    (void) loc; (void) number; (void) message;
}

void InternalError(SourceLoc *loc, int number, const char *message, ...)
{
    (void) loc; (void) number; (void) message;
    abort();
}

void FatalError(const char *message, ...)
{
    (void) message;
    abort();
}

dtype CurrentDeclTypeSpecs = { 0, };

Symbol *DefineTypedef(SourceLoc *loc, Scope *scope, int atom, Type *type)
{
    (void) loc; (void) scope; (void) atom; (void) type;
    return NULL;
}

const SourceLoc *GetExprCallSite(const expr *call)
{
    (void) call;
    return NULL;
}

static Symbol *NewTestSymbol(symbolkind kind, const char *name, Type *type,
                             SourceLoc loc)
{
    Symbol *symbol;

    symbol = (Symbol *) calloc(1, sizeof(*symbol));
    assert(symbol != NULL);
    symbol->kind = kind;
    symbol->name = LookUpAddString(atable, name);
    symbol->type = type;
    symbol->loc = loc;
    symbol->storageClass = SC_AUTO;
    return symbol;
}

static CgIRExpr *NewOrderedVector(CgIRModule *module, Type *scalar,
                                  Type *vector, SourceLoc loc,
                                  Symbol *bump, Symbol *value)
{
    CgNumericValue zero;
    CgIRExpr *arguments;
    CgIRExpr *callArguments;
    CgIRExpr *item;
    int i;

    arguments = NULL;
    for (i = 0; i < 2; i++) {
        callArguments = CgIRNewSymbol(module, scalar, &loc, value);
        assert(callArguments != NULL);
        callArguments->isLvalue = 1;
        item = CgIRNewCall(module, scalar, &loc, bump, callArguments);
        assert(item != NULL);
        item->sideEffects = 1;
        CgIRAppendExpr(&arguments, item);
    }
    item = CgIRNewSymbol(module, scalar, &loc, value);
    assert(item != NULL);
    CgIRAppendExpr(&arguments, item);
    memset(&zero, 0, sizeof(zero));
    zero.kind = CG_SCALAR_FLOAT;
    item = CgIRNewConstant(module, scalar, &loc, &zero);
    assert(item != NULL);
    CgIRAppendExpr(&arguments, item);
    item = CgIRNewConstruct(module, vector, &loc, arguments);
    assert(item != NULL);
    item->sideEffects = 1;
    return item;
}

static CgIRExpr *NewOrderedPair(CgIRModule *module, Type *scalar,
                                Type *pair, SourceLoc loc,
                                Symbol *bump, Symbol *value)
{
    CgIRExpr *arguments;
    CgIRExpr *callArguments;
    CgIRExpr *item;
    int i;

    arguments = NULL;
    for (i = 0; i < 2; i++) {
        callArguments = CgIRNewSymbol(module, scalar, &loc, value);
        assert(callArguments != NULL);
        callArguments->isLvalue = 1;
        item = CgIRNewCall(module, scalar, &loc, bump, callArguments);
        assert(item != NULL);
        item->sideEffects = 1;
        CgIRAppendExpr(&arguments, item);
    }
    item = CgIRNewConstruct(module, pair, &loc, arguments);
    assert(item != NULL);
    item->sideEffects = 1;
    return item;
}

static CgIRExpr *NewBumpCondition(CgIRModule *module, Type *scalar,
                                  Type *booleanType, SourceLoc loc,
                                  Symbol *bump, Symbol *value,
                                  float limit)
{
    CgNumericValue number;
    CgIRExpr *left;
    CgIRExpr *right;

    left = NewOrderedPair(module, scalar,
        GetStandardTypeKind(CG_SCALAR_FLOAT, 2, 0), loc, bump, value);
    right = left->u.construct.arguments;
    left->u.construct.arguments = NULL;
    left = CgIRNewBinary(module, scalar, &loc, CGIR_OP_ADD,
                         right, right->next);
    assert(left != NULL);
    left->u.binary.left->next = NULL;
    left->sideEffects = 1;
    memset(&number, 0, sizeof(number));
    number.kind = CG_SCALAR_FLOAT;
    number.value.f = limit;
    right = CgIRNewConstant(module, scalar, &loc, &number);
    left = CgIRNewBinary(module, booleanType, &loc, CGIR_OP_LESS,
                         left, right);
    assert(left != NULL);
    left->sideEffects = 1;
    return left;
}

static int CountKind(const HlslStmt *statement, HlslStmtKind kind)
{
    int count;

    count = 0;
    for (; statement != NULL; statement = statement->next) {
        if (statement->kind == kind)
            count++;
        switch (statement->kind) {
        case HLSL_STMT_IF:
            count += CountKind(statement->u.ifStmt.trueBranch, kind);
            count += CountKind(statement->u.ifStmt.falseBranch, kind);
            break;
        case HLSL_STMT_WHILE:
        case HLSL_STMT_DO:
            count += CountKind(statement->u.loop.body, kind);
            break;
        case HLSL_STMT_FOR:
            count += CountKind(statement->u.forStmt.init, kind);
            count += CountKind(statement->u.forStmt.body, kind);
            break;
        case HLSL_STMT_BLOCK:
            count += CountKind(statement->u.block, kind);
            break;
        default:
            break;
        }
    }
    return count;
}

static int CountIRKind(const CgIRStmt *statement, CgIRStmtKind kind)
{
    int count;

    count = 0;
    while (statement != NULL) {
        if (statement->kind == kind)
            count++;
        switch (statement->kind) {
        case CGIR_STMT_IF:
            count += CountIRKind(statement->u.ifStmt.trueBranch, kind);
            count += CountIRKind(statement->u.ifStmt.falseBranch, kind);
            break;
        case CGIR_STMT_WHILE:
        case CGIR_STMT_DO:
            count += CountIRKind(statement->u.loop.body, kind);
            break;
        case CGIR_STMT_FOR:
            count += CountIRKind(statement->u.forStmt.init, kind);
            count += CountIRKind(statement->u.forStmt.body, kind);
            break;
        case CGIR_STMT_BLOCK:
            count += CountIRKind(statement->u.block, kind);
            break;
        default:
            break;
        }
        statement = statement->next;
    }
    return count;
}

static HlslStmt *FindAssignment(const HlslStmt *statement,
                                const HlslDecl *target)
{
    HlslExpr *expression;

    while (statement != NULL) {
        if (statement->kind == HLSL_STMT_EXPRESSION) {
            expression = statement->u.expression;
            if (expression != NULL && expression->kind == HLSL_EXPR_BINARY &&
                expression->u.binary.op == HLSL_OP_ASSIGN &&
                expression->u.binary.left != NULL &&
                expression->u.binary.left->kind == HLSL_EXPR_SYMBOL &&
                expression->u.binary.left->u.symbol == target)
            {
                return (HlslStmt *) statement;
            }
        }
        statement = statement->next;
    }
    return NULL;
}

static void AssertOrderedBundlePrefix(const HlslStmt *statement,
                                      const Symbol *bump,
                                      const Symbol *value)
{
    const HlslExpr *assignment;
    const HlslExpr *arguments;
    const HlslDecl *first;
    const HlslDecl *second;

    assert(statement != NULL && statement->kind == HLSL_STMT_EXPRESSION);
    assignment = statement->u.expression;
    assert(assignment != NULL && assignment->kind == HLSL_EXPR_BINARY &&
           assignment->u.binary.op == HLSL_OP_ASSIGN);
    assert(assignment->u.binary.left->kind == HLSL_EXPR_SYMBOL);
    first = assignment->u.binary.left->u.symbol;
    assert(assignment->u.binary.right->kind == HLSL_EXPR_CALL &&
           assignment->u.binary.right->u.call.function != NULL &&
           assignment->u.binary.right->u.call.function->identity == bump);

    statement = statement->next;
    assert(statement != NULL && statement->kind == HLSL_STMT_EXPRESSION);
    assignment = statement->u.expression;
    assert(assignment != NULL && assignment->kind == HLSL_EXPR_BINARY &&
           assignment->u.binary.op == HLSL_OP_ASSIGN);
    assert(assignment->u.binary.left->kind == HLSL_EXPR_SYMBOL);
    second = assignment->u.binary.left->u.symbol;
    assert(assignment->u.binary.right->kind == HLSL_EXPR_CALL &&
           assignment->u.binary.right->u.call.function != NULL &&
           assignment->u.binary.right->u.call.function->identity == bump);

    statement = statement->next;
    assert(statement != NULL && statement->kind == HLSL_STMT_EXPRESSION);
    assignment = statement->u.expression;
    assert(assignment != NULL && assignment->kind == HLSL_EXPR_BINARY &&
           assignment->u.binary.op == HLSL_OP_ASSIGN &&
           assignment->u.binary.right->kind == HLSL_EXPR_CONSTRUCT);
    arguments = assignment->u.binary.right->u.construct.arguments;
    assert(arguments != NULL && arguments->kind == HLSL_EXPR_SYMBOL &&
           arguments->u.symbol == first);
    arguments = arguments->next;
    assert(arguments != NULL && arguments->kind == HLSL_EXPR_SYMBOL &&
           arguments->u.symbol == second);
    arguments = arguments->next;
    assert(arguments != NULL && arguments->kind == HLSL_EXPR_SYMBOL &&
           arguments->u.symbol->identity == value);
}

static HlslStmt *AssertIndexedSwizzleStore(HlslStmt *statement,
                                           const Symbol *next,
                                           const Symbol *values,
                                           const Symbol *observed)
{
    const HlslExpr *assignment;
    const HlslExpr *target;
    const HlslExpr *read;
    const HlslDecl *indexTemp;

    assert(statement != NULL && statement->kind == HLSL_STMT_EXPRESSION);
    assignment = statement->u.expression;
    assert(assignment != NULL && assignment->kind == HLSL_EXPR_BINARY &&
           assignment->u.binary.op == HLSL_OP_ASSIGN &&
           assignment->u.binary.left->kind == HLSL_EXPR_SYMBOL &&
           assignment->u.binary.right->kind == HLSL_EXPR_CALL &&
           assignment->u.binary.right->u.call.function->identity == next);
    indexTemp = assignment->u.binary.left->u.symbol;

    statement = statement->next;
    assert(statement != NULL && statement->kind == HLSL_STMT_EXPRESSION);
    assignment = statement->u.expression;
    target = assignment != NULL ? assignment->u.binary.left : NULL;
    assert(assignment != NULL && assignment->kind == HLSL_EXPR_BINARY &&
           assignment->u.binary.op == HLSL_OP_ASSIGN && target != NULL &&
           target->kind == HLSL_EXPR_SWIZZLE &&
           target->u.swizzle.object->kind == HLSL_EXPR_INDEX &&
           target->u.swizzle.object->u.index.object->kind ==
               HLSL_EXPR_SYMBOL &&
           target->u.swizzle.object->u.index.object->u.symbol->identity ==
               values &&
           target->u.swizzle.object->u.index.index->kind ==
               HLSL_EXPR_SYMBOL &&
           target->u.swizzle.object->u.index.index->u.symbol == indexTemp);

    statement = statement->next;
    assert(statement != NULL && statement->kind == HLSL_STMT_EXPRESSION);
    assignment = statement->u.expression;
    read = assignment != NULL ? assignment->u.binary.right : NULL;
    assert(assignment != NULL && assignment->kind == HLSL_EXPR_BINARY &&
           assignment->u.binary.left->kind == HLSL_EXPR_SYMBOL &&
           assignment->u.binary.left->u.symbol->identity == observed &&
           read != NULL && read->kind == HLSL_EXPR_SWIZZLE &&
           read->u.swizzle.object->kind == HLSL_EXPR_INDEX &&
           read->u.swizzle.object->u.index.object->kind == HLSL_EXPR_SYMBOL &&
           read->u.swizzle.object->u.index.object->u.symbol->identity ==
               values);
    return statement->next;
}

static HlslStmt *AssertVectorConditional(HlslStmt *statement,
                                         const Symbol *selected)
{
    const HlslExpr *assignment;
    const HlslExpr *item;

    for (; statement != NULL; statement = statement->next) {
        assert(statement->kind != HLSL_STMT_IF);
        if (statement->kind != HLSL_STMT_EXPRESSION)
            continue;
        assignment = statement->u.expression;
        if (assignment == NULL || assignment->kind != HLSL_EXPR_BINARY ||
            assignment->u.binary.op != HLSL_OP_ASSIGN ||
            assignment->u.binary.left->kind != HLSL_EXPR_SYMBOL ||
            assignment->u.binary.left->u.symbol->identity != selected)
        {
            continue;
        }
        assert(assignment->u.binary.right->kind == HLSL_EXPR_CONSTRUCT);
        item = assignment->u.binary.right->u.construct.arguments;
        assert(item != NULL && item->kind == HLSL_EXPR_CONDITIONAL);
        item = item->next;
        assert(item != NULL && item->kind == HLSL_EXPR_CONDITIONAL &&
               item->next == NULL);
        return statement->next;
    }
    assert(0);
    return NULL;
}

static HlslStmt *AssertLoopPrefixes(HlslStmt *statement,
                                    const Symbol *bump)
{
    const HlslStmt *body;
    const HlslExpr *assignment;

    assert(statement != NULL && statement->kind == HLSL_STMT_WHILE);
    assert(statement->u.loop.condition->kind == HLSL_EXPR_BOOL &&
           statement->u.loop.condition->u.literalBool);
    body = statement->u.loop.body;
    assignment = body != NULL ? body->u.expression : NULL;
    assert(body != NULL && body->kind == HLSL_STMT_EXPRESSION &&
           assignment->u.binary.right->kind == HLSL_EXPR_CALL &&
           assignment->u.binary.right->u.call.function->identity == bump);

    statement = statement->next;
    assert(statement != NULL && statement->kind == HLSL_STMT_EXPRESSION);
    statement = statement->next;
    assert(statement != NULL && statement->kind == HLSL_STMT_WHILE &&
           statement->u.loop.body != NULL &&
           statement->u.loop.body->kind == HLSL_STMT_IF &&
           statement->u.loop.body->u.ifStmt.trueBranch != NULL);
    body = statement->u.loop.body->u.ifStmt.trueBranch;
    assert(body->kind == HLSL_STMT_EXPRESSION &&
           body->u.expression->u.binary.right->kind == HLSL_EXPR_CALL &&
           body->u.expression->u.binary.right->u.call.function->identity ==
               bump);

    statement = statement->next;
    assert(statement != NULL && statement->kind == HLSL_STMT_FOR &&
           statement->u.forStmt.condition == NULL &&
           statement->u.forStmt.body != NULL &&
           statement->u.forStmt.step != NULL);
    body = statement->u.forStmt.body;
    assert(body->kind == HLSL_STMT_EXPRESSION &&
           body->u.expression->u.binary.right->kind == HLSL_EXPR_CALL &&
           body->u.expression->u.binary.right->u.call.function->identity ==
               bump);
    body = statement->u.forStmt.step;
    assert(body->kind == HLSL_STMT_EXPRESSION &&
           body->u.expression->u.binary.right->kind == HLSL_EXPR_CALL &&
           body->u.expression->u.binary.right->u.call.function->identity ==
               bump);
    assert(CountKind(statement->u.forStmt.body, HLSL_STMT_BREAK) > 0 &&
           CountKind(statement->u.forStmt.body, HLSL_STMT_CONTINUE) > 0);
    return statement->next;
}

int main(int argc, char **argv)
{
    CgIRModule source;
    CgIRFunction *sourceEntry;
    CgIRFunction *sourceBump;
    CgIRFunction *sourceNext;
    CgIRDecl *sourceValue;
    CgIRDecl *sourceBumpValue;
    CgIRDecl *sourceValues;
    CgIRDecl *sourceIndex;
    CgIRDecl *sourceNextIndex;
    CgIRDecl *sourceChoose;
    CgIRDecl *sourceSelected;
    CgIRGeometryInfo geometry;
    CgIRGeometryValue *value;
    CgIRStmt *root;
    CgIRStmt *ifStmt;
    CgIRStmt *trueBlock;
    CgIRStmt *falseBlock;
    CgIRStmt *whileStmt;
    CgIRStmt *whileBlock;
    CgIRStmt *operation;
    CgIRExpr *condition;
    CgIRExpr *lvalueTarget;
    CgIRExpr *lvalueValue;
    CgIRExpr *arguments;
    CgIRExpr *arrayRef;
    CgIRExpr *indexExpr;
    CgIRStmt *lvalueStmt;
    CgIRStmt *readStmt;
    CgIRStmt *vectorStmt;
    CgIRStmt *loopWhileStmt;
    CgIRStmt *loopDoStmt;
    CgIRStmt *loopForStmt;
    CgIRStmt *loopBody;
    CgNumericValue boolean;
    CgNumericValue initialValue;
    CgNumericValue intZero;
    CgNumericValue floatOne;
    CgNumericValue floatTwo;
    CgIRVerifyDiagnostic diagnostic;
    HlslModule target;
    HlslFunction *entry;
    HlslStmt *hIf;
    HlslStmt *hWhile;
    HlslStmt *flatBlock;
    HlslStmt *restart;
    HlslStmt *emitBlock;
    HlslStmt *append;
    HlslStmt *assignment;
    CgStruct compiler;
    Type functionType;
    Type bumpFunctionType;
    Type nextFunctionType;
    Type inoutFloatType;
    Type inoutIntType;
    Type valuesArrayType;
    TypeList bumpParameterType;
    TypeList nextParameterType;
    Type *floatType;
    Type *float2Type;
    Type *float4Type;
    Type *intType;
    Type *boolType;
    Type *bool2Type;
    Symbol *program;
    Symbol *bump;
    Symbol *next;
    Symbol *valueSymbol;
    Symbol *bumpValueSymbol;
    Symbol *valuesSymbol;
    Symbol *indexSymbol;
    Symbol *nextIndexSymbol;
    Symbol *chooseSymbol;
    Symbol *selectedSymbol;
    Scope locals;
    Scope bumpLocals;
    Scope nextLocals;
    SourceLoc entryLoc;
    SourceLoc ifLoc;
    SourceLoc flatLoc;
    SourceLoc restartLoc;
    SourceLoc whileLoc;
    SourceLoc emitLoc;
    SourceLoc bumpLoc;
    SourceLoc lvalueLoc;

    if (argc == 2 && !strcmp(argv[1], "--verify-assertions-active")) {
        int assertionsActive;

        assertionsActive = 0;
        assert((assertionsActive = 1) != 0);
        if (!assertionsActive)
            return 2;
        puts("glsl-ir-assertions-active");
        return 0;
    }

    memset(&compiler, 0, sizeof(compiler));
    Cg = &compiler;
    assert(InitAtomTable(atable, 0));
    assert(RegisterProfiles_hlsl());
    assert(InitHAL("hlslg40", "main"));
    assert(InitSymbolTable(Cg));
    assert(StartGlobalScope(Cg));

    floatType = GetStandardTypeKind(CG_SCALAR_FLOAT, 0, 0);
    float2Type = GetStandardTypeKind(CG_SCALAR_FLOAT, 2, 0);
    float4Type = GetStandardTypeKind(CG_SCALAR_FLOAT, 4, 0);
    intType = GetStandardTypeKind(CG_SCALAR_INT, 0, 0);
    boolType = GetStandardTypeKind(CG_SCALAR_BOOL, 0, 0);
    bool2Type = GetStandardTypeKind(CG_SCALAR_BOOL, 2, 0);
    assert(floatType != UndefinedType && float2Type != UndefinedType &&
           float4Type != UndefinedType && intType != UndefinedType);
    assert(boolType != UndefinedType && bool2Type != UndefinedType);

    memset(&entryLoc, 0, sizeof(entryLoc)); entryLoc.file = 41; entryLoc.line = 3;
    memset(&ifLoc, 0, sizeof(ifLoc)); ifLoc.file = 41; ifLoc.line = 7;
    memset(&flatLoc, 0, sizeof(flatLoc)); flatLoc.file = 41; flatLoc.line = 8;
    memset(&restartLoc, 0, sizeof(restartLoc)); restartLoc.file = 41; restartLoc.line = 10;
    memset(&whileLoc, 0, sizeof(whileLoc)); whileLoc.file = 41; whileLoc.line = 12;
    memset(&emitLoc, 0, sizeof(emitLoc)); emitLoc.file = 41; emitLoc.line = 13;
    memset(&bumpLoc, 0, sizeof(bumpLoc)); bumpLoc.file = 41; bumpLoc.line = 2;
    memset(&lvalueLoc, 0, sizeof(lvalueLoc)); lvalueLoc.file = 41; lvalueLoc.line = 4;

    memset(&functionType, 0, sizeof(functionType));
    functionType.fun.properties = TYPE_CATEGORY_FUNCTION | TYPE_MISC_PROGRAM;
    functionType.fun.rettype = VoidType;
    program = NewTestSymbol(FUNCTION_S, "geometryMain", &functionType,
                            entryLoc);
    program->properties |= SYMB_IS_DEFINED;
    memset(&locals, 0, sizeof(locals));
    program->details.fun.locals = &locals;

    inoutFloatType = *floatType;
    inoutFloatType.properties |= TYPE_QUALIFIER_INOUT;
    memset(&bumpParameterType, 0, sizeof(bumpParameterType));
    bumpParameterType.type = &inoutFloatType;
    memset(&bumpFunctionType, 0, sizeof(bumpFunctionType));
    bumpFunctionType.fun.properties = TYPE_CATEGORY_FUNCTION;
    bumpFunctionType.fun.rettype = floatType;
    bumpFunctionType.fun.paramtypes = &bumpParameterType;
    bump = NewTestSymbol(FUNCTION_S, "bump", &bumpFunctionType, bumpLoc);
    valueSymbol = NewTestSymbol(VARIABLE_S, "value", floatType, entryLoc);
    bumpValueSymbol = NewTestSymbol(VARIABLE_S, "value", &inoutFloatType,
                                    bumpLoc);
    assert(bump != NULL && valueSymbol != NULL && bumpValueSymbol != NULL);
    bump->properties |= SYMB_IS_DEFINED;
    bump->details.fun.params = bumpValueSymbol;
    memset(&bumpLocals, 0, sizeof(bumpLocals));
    bump->details.fun.locals = &bumpLocals;

    memset(&valuesArrayType, 0, sizeof(valuesArrayType));
    valuesArrayType.properties = TYPE_BASE_FLOAT | TYPE_CATEGORY_ARRAY;
    valuesArrayType.arr.eltype = float4Type;
    valuesArrayType.arr.numels = 2;
    valuesArrayType.arr.scalarKind = CG_SCALAR_FLOAT;
    inoutIntType = *intType;
    inoutIntType.properties |= TYPE_QUALIFIER_INOUT;
    memset(&nextParameterType, 0, sizeof(nextParameterType));
    nextParameterType.type = &inoutIntType;
    memset(&nextFunctionType, 0, sizeof(nextFunctionType));
    nextFunctionType.fun.properties = TYPE_CATEGORY_FUNCTION;
    nextFunctionType.fun.rettype = intType;
    nextFunctionType.fun.paramtypes = &nextParameterType;
    next = NewTestSymbol(FUNCTION_S, "next", &nextFunctionType, lvalueLoc);
    valuesSymbol = NewTestSymbol(VARIABLE_S, "values", &valuesArrayType,
                                 lvalueLoc);
    indexSymbol = NewTestSymbol(VARIABLE_S, "index", intType, lvalueLoc);
    nextIndexSymbol = NewTestSymbol(VARIABLE_S, "index", &inoutIntType,
                                    lvalueLoc);
    chooseSymbol = NewTestSymbol(VARIABLE_S, "choose", bool2Type,
                                 lvalueLoc);
    selectedSymbol = NewTestSymbol(VARIABLE_S, "selected", float2Type,
                                   lvalueLoc);
    assert(next != NULL && valuesSymbol != NULL && indexSymbol != NULL &&
           nextIndexSymbol != NULL && chooseSymbol != NULL &&
           selectedSymbol != NULL);
    next->properties |= SYMB_IS_DEFINED;
    next->details.fun.params = nextIndexSymbol;
    memset(&nextLocals, 0, sizeof(nextLocals));
    next->details.fun.locals = &nextLocals;
    locals.symbols = valueSymbol;
    valueSymbol->right = valuesSymbol;
    valuesSymbol->right = indexSymbol;
    indexSymbol->right = chooseSymbol;
    chooseSymbol->right = selectedSymbol;

    CgIRInitModule(&source, TestAlloc, NULL);
    assert(CgIRSetStage(&source, CGIR_STAGE_GEOMETRY));
    memset(&geometry, 0, sizeof(geometry));
    geometry.inputTopology = CG_GEOMETRY_INPUT_POINT;
    geometry.outputTopology = CG_GEOMETRY_OUTPUT_POINTS;
    geometry.inputVertexCount = 1;
    geometry.maxOutputVertices = 1;
    geometry.hasMaxOutputVertices = 1;
    geometry.inputLoc = entryLoc;
    geometry.outputLoc = entryLoc;
    geometry.maxVerticesLoc = entryLoc;
    assert(CgIRSetGeometryInfo(&source, &geometry));
    sourceEntry = CgIRNewFunction(&source, program, VoidType, &entryLoc);
    assert(sourceEntry != NULL);
    sourceEntry->isEntry = 1;
    source.entry = sourceEntry;
    CgIRAppendFunction(&source.functions, sourceEntry);

    sourceBump = CgIRNewFunction(&source, bump, floatType, &bumpLoc);
    sourceBumpValue = CgIRNewDecl(&source, bumpValueSymbol,
        bumpValueSymbol->name, &inoutFloatType, CGIR_STORAGE_NONE,
        CGIR_DOMAIN_NONE, 0, NULL, &bumpLoc);
    assert(sourceBump != NULL && sourceBumpValue != NULL);
    CgIRAppendDecl(&sourceBump->parameters, sourceBumpValue);
    sourceBump->body = CgIRNewReturnStmt(&source, &bumpLoc,
        CgIRNewSymbol(&source, floatType, &bumpLoc, bumpValueSymbol));
    assert(sourceBump->body != NULL);
    CgIRAppendFunction(&source.functions, sourceBump);

    sourceNext = CgIRNewFunction(&source, next, intType, &lvalueLoc);
    sourceNextIndex = CgIRNewDecl(&source, nextIndexSymbol,
        nextIndexSymbol->name, &inoutIntType, CGIR_STORAGE_NONE,
        CGIR_DOMAIN_NONE, 0, NULL, &lvalueLoc);
    assert(sourceNext != NULL && sourceNextIndex != NULL);
    CgIRAppendDecl(&sourceNext->parameters, sourceNextIndex);
    sourceNext->body = CgIRNewReturnStmt(&source, &lvalueLoc,
        CgIRNewSymbol(&source, intType, &lvalueLoc, nextIndexSymbol));
    assert(sourceNext->body != NULL);
    CgIRAppendFunction(&source.functions, sourceNext);

    memset(&initialValue, 0, sizeof(initialValue));
    initialValue.kind = CG_SCALAR_FLOAT;
    sourceValue = CgIRNewDecl(&source, valueSymbol, valueSymbol->name,
        floatType, CGIR_STORAGE_NONE, CGIR_DOMAIN_NONE, 0,
        CgIRNewConstant(&source, floatType, &entryLoc, &initialValue),
        &entryLoc);
    assert(sourceValue != NULL && sourceValue->initializer != NULL);
    CgIRAppendDecl(&sourceEntry->locals, sourceValue);
    memset(&intZero, 0, sizeof(intZero));
    intZero.kind = CG_SCALAR_INT;
    sourceValues = CgIRNewDecl(&source, valuesSymbol, valuesSymbol->name,
        &valuesArrayType, CGIR_STORAGE_NONE, CGIR_DOMAIN_NONE, 0,
        NULL, &lvalueLoc);
    sourceIndex = CgIRNewDecl(&source, indexSymbol, indexSymbol->name,
        intType, CGIR_STORAGE_NONE, CGIR_DOMAIN_NONE, 0,
        CgIRNewConstant(&source, intType, &lvalueLoc, &intZero),
        &lvalueLoc);
    assert(sourceValues != NULL && sourceIndex != NULL &&
           sourceIndex->initializer != NULL);
    CgIRAppendDecl(&sourceEntry->locals, sourceValues);
    CgIRAppendDecl(&sourceEntry->locals, sourceIndex);
    memset(&boolean, 0, sizeof(boolean));
    boolean.kind = CG_SCALAR_BOOL;
    boolean.value.i = 1;
    arguments = NULL;
    CgIRAppendExpr(&arguments, CgIRNewConstant(&source, boolType,
                                                &lvalueLoc, &boolean));
    CgIRAppendExpr(&arguments, CgIRNewConstant(&source, boolType,
                                                &lvalueLoc, &boolean));
    sourceChoose = CgIRNewDecl(&source, chooseSymbol, chooseSymbol->name,
        bool2Type, CGIR_STORAGE_NONE, CGIR_DOMAIN_NONE, 0,
        CgIRNewConstruct(&source, bool2Type, &lvalueLoc, arguments),
        &lvalueLoc);
    sourceSelected = CgIRNewDecl(&source, selectedSymbol,
        selectedSymbol->name, float2Type, CGIR_STORAGE_NONE,
        CGIR_DOMAIN_NONE, 0, NULL, &lvalueLoc);
    assert(sourceChoose != NULL && sourceChoose->initializer != NULL &&
           sourceSelected != NULL);
    CgIRAppendDecl(&sourceEntry->locals, sourceChoose);
    CgIRAppendDecl(&sourceEntry->locals, sourceSelected);

    memset(&boolean, 0, sizeof(boolean));
    boolean.kind = CG_SCALAR_BOOL;
    boolean.value.i = 1;
    condition = CgIRNewConstant(&source, boolType, &ifLoc, &boolean);
    trueBlock = CgIRNewBlockStmt(&source, &ifLoc);
    falseBlock = CgIRNewBlockStmt(&source, &ifLoc);
    value = CgIRNewGeometryValue(&source,
        LookUpAddString(atable, "COLOR0"),
        LookUpAddString(atable, "COLOR0"), float4Type,
        NewOrderedVector(&source, floatType, float4Type, flatLoc,
                         bump, valueSymbol), flatLoc);
    operation = CgIRNewGeometryFlat(&source, value, flatLoc);
    assert(condition != NULL && trueBlock != NULL && falseBlock != NULL);
    assert(operation != NULL);
    CgIRAppendStmt(&trueBlock->u.block, operation);
    operation = CgIRNewGeometryRestart(&source, restartLoc);
    assert(operation != NULL);
    CgIRAppendStmt(&falseBlock->u.block, operation);
    ifStmt = CgIRNewIfStmt(&source, &ifLoc, condition,
                           trueBlock, falseBlock);
    assert(ifStmt != NULL);

    condition = CgIRNewConstant(&source, boolType, &whileLoc, &boolean);
    whileBlock = CgIRNewBlockStmt(&source, &whileLoc);
    value = CgIRNewGeometryValue(&source,
        LookUpAddString(atable, "POSITION"),
        LookUpAddString(atable, "POSITION"), float4Type,
        NewOrderedVector(&source, floatType, float4Type, emitLoc,
                         bump, valueSymbol), emitLoc);
    operation = CgIRNewGeometryEmit(&source, value, emitLoc);
    assert(condition != NULL && whileBlock != NULL && operation != NULL);
    CgIRAppendStmt(&whileBlock->u.block, operation);
    CgIRAppendStmt(&whileBlock->u.block,
                   CgIRNewBreakStmt(&source, &emitLoc));
    whileStmt = CgIRNewWhileStmt(&source, &whileLoc, condition, whileBlock);
    assert(whileStmt != NULL);

    arguments = CgIRNewSymbol(&source, intType, &lvalueLoc, indexSymbol);
    assert(arguments != NULL);
    arguments->isLvalue = 1;
    indexExpr = CgIRNewCall(&source, intType, &lvalueLoc, next, arguments);
    assert(indexExpr != NULL);
    indexExpr->sideEffects = 1;
    arrayRef = CgIRNewSymbol(&source, &valuesArrayType, &lvalueLoc,
                             valuesSymbol);
    assert(arrayRef != NULL);
    arrayRef->isLvalue = 1;
    lvalueTarget = CgIRNewIndex(&source, float4Type, &lvalueLoc,
                                arrayRef, indexExpr);
    assert(lvalueTarget != NULL);
    lvalueTarget->isLvalue = 1;
    lvalueTarget = CgIRNewSwizzle(&source, float2Type, &lvalueLoc,
                                  lvalueTarget, 0x4, 2);
    assert(lvalueTarget != NULL);
    lvalueTarget->isLvalue = 1;
    memset(&floatOne, 0, sizeof(floatOne));
    floatOne.kind = CG_SCALAR_FLOAT;
    floatOne.value.f = 1.0f;
    memset(&floatTwo, 0, sizeof(floatTwo));
    floatTwo.kind = CG_SCALAR_FLOAT;
    floatTwo.value.f = 2.0f;
    arguments = NULL;
    CgIRAppendExpr(&arguments, CgIRNewConstant(&source, floatType,
                                                &lvalueLoc, &floatOne));
    CgIRAppendExpr(&arguments, CgIRNewConstant(&source, floatType,
                                                &lvalueLoc, &floatTwo));
    lvalueValue = CgIRNewConstruct(&source, float2Type, &lvalueLoc,
                                   arguments);
    lvalueValue = CgIRNewAssign(&source, float2Type, &lvalueLoc,
        CGIR_OP_ASSIGN, lvalueTarget, lvalueValue);
    lvalueStmt = CgIRNewExprStmt(&source, &lvalueLoc, lvalueValue);
    assert(lvalueValue != NULL && lvalueStmt != NULL);

    arrayRef = CgIRNewSymbol(&source, &valuesArrayType, &lvalueLoc,
                             valuesSymbol);
    indexExpr = CgIRNewConstant(&source, intType, &lvalueLoc, &intZero);
    lvalueValue = CgIRNewIndex(&source, float4Type, &lvalueLoc,
                               arrayRef, indexExpr);
    lvalueValue = CgIRNewSwizzle(&source, floatType, &lvalueLoc,
                                 lvalueValue, 0, 1);
    lvalueTarget = CgIRNewSymbol(&source, floatType, &lvalueLoc,
                                 valueSymbol);
    assert(lvalueTarget != NULL);
    lvalueTarget->isLvalue = 1;
    lvalueValue = CgIRNewAssign(&source, floatType, &lvalueLoc,
        CGIR_OP_ASSIGN, lvalueTarget, lvalueValue);
    readStmt = CgIRNewExprStmt(&source, &lvalueLoc, lvalueValue);
    assert(lvalueValue != NULL && readStmt != NULL);

    condition = CgIRNewSymbol(&source, bool2Type, &lvalueLoc,
                              chooseSymbol);
    lvalueValue = CgIRNewConditional(&source, float2Type, &lvalueLoc,
        condition,
        NewOrderedPair(&source, floatType, float2Type, lvalueLoc,
                       bump, valueSymbol),
        NewOrderedPair(&source, floatType, float2Type, lvalueLoc,
                       bump, valueSymbol));
    lvalueTarget = CgIRNewSymbol(&source, float2Type, &lvalueLoc,
                                 selectedSymbol);
    assert(lvalueTarget != NULL);
    lvalueTarget->isLvalue = 1;
    lvalueValue = CgIRNewAssign(&source, float2Type, &lvalueLoc,
        CGIR_OP_ASSIGN, lvalueTarget, lvalueValue);
    vectorStmt = CgIRNewExprStmt(&source, &lvalueLoc, lvalueValue);
    assert(condition != NULL && lvalueValue != NULL && vectorStmt != NULL);

    loopBody = CgIRNewBlockStmt(&source, &lvalueLoc);
    CgIRAppendStmt(&loopBody->u.block,
                   CgIRNewBreakStmt(&source, &lvalueLoc));
    loopWhileStmt = CgIRNewWhileStmt(&source, &lvalueLoc,
        NewBumpCondition(&source, floatType, boolType, lvalueLoc,
                         bump, valueSymbol, 4.0f), loopBody);
    loopBody = CgIRNewBlockStmt(&source, &lvalueLoc);
    CgIRAppendStmt(&loopBody->u.block,
                   CgIRNewBreakStmt(&source, &lvalueLoc));
    loopDoStmt = CgIRNewDoStmt(&source, &lvalueLoc,
        NewBumpCondition(&source, floatType, boolType, lvalueLoc,
                         bump, valueSymbol, 8.0f), loopBody);
    loopBody = CgIRNewBlockStmt(&source, &lvalueLoc);
    CgIRAppendStmt(&loopBody->u.block,
                   CgIRNewBreakStmt(&source, &lvalueLoc));
    CgIRAppendStmt(&loopBody->u.block,
                   CgIRNewContinueStmt(&source, &lvalueLoc));
    lvalueValue = NewOrderedPair(&source, floatType, float2Type,
                                 lvalueLoc, bump, valueSymbol);
    arguments = lvalueValue->u.construct.arguments;
    lvalueValue = CgIRNewBinary(&source, floatType, &lvalueLoc,
        CGIR_OP_ADD, arguments, arguments->next);
    assert(lvalueValue != NULL);
    lvalueValue->u.binary.left->next = NULL;
    lvalueValue->sideEffects = 1;
    lvalueTarget = CgIRNewSymbol(&source, floatType, &lvalueLoc,
                                 valueSymbol);
    lvalueTarget->isLvalue = 1;
    lvalueValue = CgIRNewAssign(&source, floatType, &lvalueLoc,
        CGIR_OP_ASSIGN, lvalueTarget, lvalueValue);
    loopForStmt = CgIRNewForStmt(&source, &lvalueLoc, NULL,
        NewBumpCondition(&source, floatType, boolType, lvalueLoc,
                         bump, valueSymbol, 12.0f),
        lvalueValue, loopBody);
    assert(loopWhileStmt != NULL && loopDoStmt != NULL &&
           loopForStmt != NULL);

    root = CgIRNewBlockStmt(&source, &entryLoc);
    assert(root != NULL);
    CgIRAppendStmt(&root->u.block,
                   CgIRNewDeclStmt(&source, &entryLoc, sourceValue));
    CgIRAppendStmt(&root->u.block,
                   CgIRNewDeclStmt(&source, &lvalueLoc, sourceValues));
    CgIRAppendStmt(&root->u.block,
                   CgIRNewDeclStmt(&source, &lvalueLoc, sourceIndex));
    CgIRAppendStmt(&root->u.block,
                   CgIRNewDeclStmt(&source, &lvalueLoc, sourceChoose));
    CgIRAppendStmt(&root->u.block,
                   CgIRNewDeclStmt(&source, &lvalueLoc, sourceSelected));
    CgIRAppendStmt(&root->u.block, lvalueStmt);
    CgIRAppendStmt(&root->u.block, readStmt);
    CgIRAppendStmt(&root->u.block, vectorStmt);
    CgIRAppendStmt(&root->u.block, loopWhileStmt);
    CgIRAppendStmt(&root->u.block, loopDoStmt);
    CgIRAppendStmt(&root->u.block, loopForStmt);
    CgIRAppendStmt(&root->u.block, ifStmt);
    CgIRAppendStmt(&root->u.block, whileStmt);
    sourceEntry->body = root;
    memset(&diagnostic, 0, sizeof(diagnostic));
    assert(CgIRVerifyModule(&source, &diagnostic));

    HlslInitModule(&target, HLSL_STAGE_GEOMETRY, TestAlloc, NULL);
    assert(HlslLowerProgramWithIR(&target, &HlslProfile_hlslg40,
                                  &entryLoc, GlobalScope, program,
                                  &source));
    entry = target.entry;
    assert(entry != NULL && entry->geometryEffect);
    assert(CountIRKind(sourceEntry->body,
                       CGIR_STMT_GEOMETRY_EMIT) == 1);
    assert(CountIRKind(sourceEntry->body,
                       CGIR_STMT_GEOMETRY_RESTART) == 1);
    assert(CountKind(entry->body, HLSL_STMT_APPEND) ==
           CountIRKind(sourceEntry->body, CGIR_STMT_GEOMETRY_EMIT));
    assert(CountKind(entry->body, HLSL_STMT_RESTART_STRIP) ==
           CountIRKind(sourceEntry->body, CGIR_STMT_GEOMETRY_RESTART));

    hIf = AssertIndexedSwizzleStore(entry->body, next, valuesSymbol,
                                    valueSymbol);
    hIf = AssertVectorConditional(hIf, selectedSymbol);
    hIf = AssertLoopPrefixes(hIf, bump);
    hWhile = hIf != NULL ? hIf->next : NULL;
    assert(hIf != NULL && hIf->kind == HLSL_STMT_IF);
    assert(hWhile != NULL && hWhile->kind == HLSL_STMT_WHILE);
    assert(hWhile->next == NULL);
    assert(hIf->loc.file == ifLoc.file && hIf->loc.line == ifLoc.line);
    assert(hWhile->loc.file == whileLoc.file &&
           hWhile->loc.line == whileLoc.line);
    assert(entry->geometryFlatState != NULL &&
           entry->geometryFlatState->next == NULL);
    assert(entry->geometryFlatState->shadow->geometryRole ==
           HLSL_GEOMETRY_DECL_FLAT_SHADOW);

    assert(hIf->u.ifStmt.trueBranch != NULL &&
           hIf->u.ifStmt.trueBranch->kind == HLSL_STMT_BLOCK);
    flatBlock = hIf->u.ifStmt.trueBranch->u.block;
    assert(flatBlock != NULL && flatBlock->kind == HLSL_STMT_BLOCK);
    assert(flatBlock->loc.file == flatLoc.file &&
           flatBlock->loc.line == flatLoc.line);
    AssertOrderedBundlePrefix(flatBlock->u.block, bump, valueSymbol);
    assignment = FindAssignment(flatBlock->u.block,
                                entry->geometryFlatState->shadow);
    assert(assignment != NULL && assignment->kind == HLSL_STMT_EXPRESSION);
    assert(assignment->u.expression->kind == HLSL_EXPR_BINARY &&
           assignment->u.expression->u.binary.op == HLSL_OP_ASSIGN);
    assert(assignment->u.expression->u.binary.left->kind ==
           HLSL_EXPR_SYMBOL);
    assert(assignment->u.expression->u.binary.left->u.symbol ==
           entry->geometryFlatState->shadow);
    assert(assignment->u.expression->u.binary.right != NULL &&
           assignment->u.expression->u.binary.right->kind !=
           HLSL_EXPR_CALL);
    assert(FindAssignment(flatBlock->u.block,
                          entry->geometryFlatState->defined) != NULL);
    assert(CountKind(flatBlock, HLSL_STMT_APPEND) == 0);
    assert(CountKind(flatBlock, HLSL_STMT_RESTART_STRIP) == 0);

    assert(hIf->u.ifStmt.falseBranch != NULL &&
           hIf->u.ifStmt.falseBranch->kind == HLSL_STMT_BLOCK);
    restart = hIf->u.ifStmt.falseBranch->u.block;
    assert(restart != NULL && restart->kind == HLSL_STMT_RESTART_STRIP);
    assert(restart->loc.file == restartLoc.file &&
           restart->loc.line == restartLoc.line);

    assert(hWhile->u.loop.body != NULL &&
           hWhile->u.loop.body->kind == HLSL_STMT_BLOCK);
    emitBlock = hWhile->u.loop.body->u.block;
    assert(emitBlock != NULL && emitBlock->kind == HLSL_STMT_BLOCK);
    assert(emitBlock->loc.file == emitLoc.file &&
           emitBlock->loc.line == emitLoc.line);
    AssertOrderedBundlePrefix(emitBlock->u.block, bump, valueSymbol);
    append = emitBlock->u.block;
    while (append != NULL && append->kind != HLSL_STMT_APPEND)
        append = append->next;
    assert(append != NULL && append->next == NULL);
    assert(append->loc.file == emitLoc.file &&
           append->loc.line == emitLoc.line);
    assert(append->u.append.replay == entry->geometryFlatState);
    return 0;
}
