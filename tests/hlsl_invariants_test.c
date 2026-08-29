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
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN
ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
OF THE NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF
CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE,
EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/
// hlsl_invariants_test.c
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hlsl_ir.h"
#include "slglobals.h"
#include "hlsl_hal.h"

#undef malloc
#undef calloc

Scope *CurrentScope = NULL;

static HlslProfileDesc profile;

static void *TestAlloc(void *arg, size_t size)
{
    (void) arg;
    return calloc(1, size);
}

static int Require(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        return 0;
    }
    return 1;
}

static HlslFunction *InitModule(HlslModule *module, HlslType result)
{
    HlslFunction *entry;

    HlslInitModule(module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    entry = HlslNewFunction(module, result, "cg_entry");
    if (entry != NULL) {
        entry->isEntry = 1;
        module->entry = entry;
        module->functions = entry;
    }
    return entry;
}

static HlslExpr *SymbolExpr(HlslModule *module, HlslDecl *decl)
{
    HlslExpr *expression;

    expression = HlslNewExpr(module, HLSL_EXPR_SYMBOL, decl->type);
    if (expression != NULL)
        expression->u.symbol = decl;
    return expression;
}

static HlslStmt *ExpressionStmt(HlslModule *module, HlslExpr *expression)
{
    HlslStmt *statement;

    statement = HlslNewStmt(module, HLSL_STMT_EXPRESSION);
    if (statement != NULL)
        statement->u.expression = expression;
    return statement;
}

static int RejectedAsInvalid(HlslModule *module, const char *message)
{
    return Require(!HlslLegalizeModule(module, &profile), message) &&
           Require(module->errorKind == HLSL_ERROR_INVALID_IR,
                   "malformed IR did not use invalid-IR diagnostic");
}

static int TestAssignmentTypes(void)
{
    HlslModule module;
    HlslFunction *entry;
    HlslDecl *left;
    HlslDecl *right;
    HlslExpr *assignment;
    HlslType floatType;
    HlslType intType;
    HlslType voidType;

    floatType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    intType = HlslNumericType(HLSL_BASE_INT, 1);
    voidType = HlslNumericType(HLSL_BASE_VOID, 0);
    entry = InitModule(&module, voidType);
    left = HlslNewDecl(&module, HLSL_STORAGE_NONE, floatType, "left");
    right = HlslNewDecl(&module, HLSL_STORAGE_NONE, intType, "right");
    assignment = HlslNewExpr(&module, HLSL_EXPR_BINARY, floatType);
    if (entry == NULL || left == NULL || right == NULL || assignment == NULL)
        return 0;
    HlslAppendDecl(&entry->locals, left);
    HlslAppendDecl(&entry->locals, right);
    assignment->u.binary.op = HLSL_OP_ASSIGN;
    assignment->u.binary.left = SymbolExpr(&module, left);
    assignment->u.binary.right = SymbolExpr(&module, right);
    entry->body = ExpressionStmt(&module, assignment);
    return RejectedAsInvalid(&module,
                             "assignment type mismatch was accepted");
}

static int TestMultiplyShapes(void)
{
    HlslModule module;
    HlslFunction *entry;
    HlslDecl *left;
    HlslDecl *right;
    HlslExpr *multiply;
    HlslType float2;
    HlslType float3;
    HlslType voidType;

    float2 = HlslNumericType(HLSL_BASE_FLOAT, 2);
    float3 = HlslNumericType(HLSL_BASE_FLOAT, 3);
    voidType = HlslNumericType(HLSL_BASE_VOID, 0);
    entry = InitModule(&module, voidType);
    left = HlslNewDecl(&module, HLSL_STORAGE_NONE, float2, "left");
    right = HlslNewDecl(&module, HLSL_STORAGE_NONE, float3, "right");
    multiply = HlslNewExpr(&module, HLSL_EXPR_BINARY, float3);
    if (entry == NULL || left == NULL || right == NULL || multiply == NULL)
        return 0;
    HlslAppendDecl(&entry->locals, left);
    HlslAppendDecl(&entry->locals, right);
    multiply->u.binary.op = HLSL_OP_MULTIPLY;
    multiply->u.binary.left = SymbolExpr(&module, left);
    multiply->u.binary.right = SymbolExpr(&module, right);
    entry->body = ExpressionStmt(&module, multiply);
    return RejectedAsInvalid(&module,
                             "incompatible multiply shapes were accepted");
}

static int TestMultiplyResult(void)
{
    HlslModule module;
    HlslFunction *entry;
    HlslDecl *left;
    HlslDecl *right;
    HlslExpr *multiply;
    HlslType floatType;
    HlslType float2;
    HlslType float3;
    HlslType voidType;

    floatType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    float2 = HlslNumericType(HLSL_BASE_FLOAT, 2);
    float3 = HlslNumericType(HLSL_BASE_FLOAT, 3);
    voidType = HlslNumericType(HLSL_BASE_VOID, 0);
    entry = InitModule(&module, voidType);
    left = HlslNewDecl(&module, HLSL_STORAGE_NONE, float2, "left");
    right = HlslNewDecl(&module, HLSL_STORAGE_NONE, floatType, "right");
    multiply = HlslNewExpr(&module, HLSL_EXPR_BINARY, float3);
    if (entry == NULL || left == NULL || right == NULL || multiply == NULL)
        return 0;
    HlslAppendDecl(&entry->locals, left);
    HlslAppendDecl(&entry->locals, right);
    multiply->u.binary.op = HLSL_OP_MULTIPLY;
    multiply->u.binary.left = SymbolExpr(&module, left);
    multiply->u.binary.right = SymbolExpr(&module, right);
    entry->body = ExpressionStmt(&module, multiply);
    return RejectedAsInvalid(&module,
                             "incorrect multiply result was accepted");
}

static int TestCallArity(void)
{
    HlslModule module;
    HlslFunction *entry;
    HlslFunction *helper;
    HlslDecl *first;
    HlslDecl *second;
    HlslDecl *argumentDecl;
    HlslExpr *call;
    HlslType floatType;

    floatType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    entry = InitModule(&module, floatType);
    helper = HlslNewFunction(&module, floatType, "cg_helper");
    first = HlslNewDecl(&module, HLSL_STORAGE_NONE, floatType, "first");
    second = HlslNewDecl(&module, HLSL_STORAGE_NONE, floatType, "second");
    argumentDecl = HlslNewDecl(&module, HLSL_STORAGE_NONE, floatType,
                               "argument");
    call = HlslNewExpr(&module, HLSL_EXPR_CALL, floatType);
    if (entry == NULL || helper == NULL || first == NULL || second == NULL ||
        argumentDecl == NULL || call == NULL)
    {
        return 0;
    }
    helper->parameters = first;
    first->next = second;
    helper->next = entry;
    module.functions = helper;
    entry->locals = argumentDecl;
    call->u.call.function = helper;
    call->u.call.name = helper->name;
    call->u.call.arguments = SymbolExpr(&module, argumentDecl);
    entry->body = HlslNewStmt(&module, HLSL_STMT_RETURN);
    entry->body->u.returnExpr = call;
    return RejectedAsInvalid(&module, "call arity mismatch was accepted");
}

static int TestCallParameterType(void)
{
    HlslModule module;
    HlslFunction *entry;
    HlslFunction *helper;
    HlslDecl *parameter;
    HlslDecl *argumentDecl;
    HlslExpr *call;
    HlslType floatType;
    HlslType intType;

    floatType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    intType = HlslNumericType(HLSL_BASE_INT, 1);
    entry = InitModule(&module, floatType);
    helper = HlslNewFunction(&module, floatType, "cg_helper");
    parameter = HlslNewDecl(&module, HLSL_STORAGE_NONE, intType, "value");
    argumentDecl = HlslNewDecl(&module, HLSL_STORAGE_NONE, floatType,
                               "argument");
    call = HlslNewExpr(&module, HLSL_EXPR_CALL, floatType);
    if (entry == NULL || helper == NULL || parameter == NULL ||
        argumentDecl == NULL || call == NULL)
    {
        return 0;
    }
    helper->parameters = parameter;
    helper->next = entry;
    module.functions = helper;
    entry->locals = argumentDecl;
    call->u.call.function = helper;
    call->u.call.name = helper->name;
    call->u.call.arguments = SymbolExpr(&module, argumentDecl);
    entry->body = HlslNewStmt(&module, HLSL_STMT_RETURN);
    entry->body->u.returnExpr = call;
    return RejectedAsInvalid(&module,
                             "call parameter type mismatch was accepted");
}

static int TestMemberOwnership(void)
{
    HlslModule module;
    HlslFunction *entry;
    HlslDecl *objectDecl;
    HlslDecl *firstMember;
    HlslDecl *otherMember;
    HlslExpr *member;
    HlslType floatType;
    HlslType structType;
    HlslType voidType;

    floatType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    voidType = HlslNumericType(HLSL_BASE_VOID, 0);
    entry = InitModule(&module, voidType);
    structType = HlslNumericType(HLSL_BASE_STRUCT, 0);
    structType.structName = "cg_Value";
    firstMember = HlslNewDecl(&module, HLSL_STORAGE_NONE, floatType, "first");
    otherMember = HlslNewDecl(&module, HLSL_STORAGE_NONE, floatType, "other");
    if (entry == NULL || firstMember == NULL || otherMember == NULL)
        return 0;
    structType.members = firstMember;
    objectDecl = HlslNewDecl(&module, HLSL_STORAGE_NONE, structType, "object");
    member = HlslNewExpr(&module, HLSL_EXPR_MEMBER, floatType);
    if (objectDecl == NULL || member == NULL)
        return 0;
    entry->locals = objectDecl;
    member->u.member.object = SymbolExpr(&module, objectDecl);
    member->u.member.decl = otherMember;
    member->u.member.name = otherMember->name;
    entry->body = ExpressionStmt(&module, member);
    return RejectedAsInvalid(&module,
                             "foreign aggregate member was accepted");
}

static int TestIndexLegality(void)
{
    HlslModule module;
    HlslFunction *entry;
    HlslDecl *objectDecl;
    HlslDecl *indexDecl;
    HlslExpr *index;
    HlslType floatType;
    HlslType float2;
    HlslType voidType;

    floatType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    float2 = HlslNumericType(HLSL_BASE_FLOAT, 2);
    voidType = HlslNumericType(HLSL_BASE_VOID, 0);
    entry = InitModule(&module, voidType);
    objectDecl = HlslNewDecl(&module, HLSL_STORAGE_NONE, float2, "object");
    indexDecl = HlslNewDecl(&module, HLSL_STORAGE_NONE, floatType, "index");
    index = HlslNewExpr(&module, HLSL_EXPR_INDEX, floatType);
    if (entry == NULL || objectDecl == NULL || indexDecl == NULL ||
        index == NULL)
    {
        return 0;
    }
    objectDecl->next = indexDecl;
    entry->locals = objectDecl;
    index->u.index.object = SymbolExpr(&module, objectDecl);
    index->u.index.index = SymbolExpr(&module, indexDecl);
    entry->body = ExpressionStmt(&module, index);
    return RejectedAsInvalid(&module, "non-integer index was accepted");
}

static int TestReturnType(void)
{
    HlslModule module;
    HlslFunction *entry;
    HlslDecl *value;
    HlslType floatType;
    HlslType intType;

    floatType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    intType = HlslNumericType(HLSL_BASE_INT, 1);
    entry = InitModule(&module, floatType);
    value = HlslNewDecl(&module, HLSL_STORAGE_NONE, intType, "value");
    if (entry == NULL || value == NULL)
        return 0;
    entry->locals = value;
    entry->body = HlslNewStmt(&module, HLSL_STMT_RETURN);
    entry->body->u.returnExpr = SymbolExpr(&module, value);
    return RejectedAsInvalid(&module, "return type mismatch was accepted");
}

static int TestInitializerAndBodyEntryCalls(void)
{
    HlslModule module;
    HlslFunction *entry;
    HlslFunction *wrapper;
    HlslDecl *inputStruct;
    HlslDecl *outputStruct;
    HlslDecl *inputMember;
    HlslDecl *outputMember;
    HlslDecl *local;
    HlslExpr *initializerCall;
    HlslExpr *bodyCall;
    HlslType float4Type;
    HlslType inputType;
    HlslType outputType;
    HlslType voidType;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    float4Type = HlslNumericType(HLSL_BASE_FLOAT, 4);
    voidType = HlslNumericType(HLSL_BASE_VOID, 0);
    inputType = HlslNumericType(HLSL_BASE_STRUCT, 0);
    inputType.structName = "cg_VertexIn";
    outputType = HlslNumericType(HLSL_BASE_STRUCT, 0);
    outputType.structName = "cg_VertexOut";
    inputStruct = HlslNewDecl(&module, HLSL_STORAGE_INPUT, inputType,
                              inputType.structName);
    outputStruct = HlslNewDecl(&module, HLSL_STORAGE_OUTPUT, outputType,
                               outputType.structName);
    inputMember = HlslNewDecl(&module, HLSL_STORAGE_NONE, float4Type,
                              "position");
    outputMember = HlslNewDecl(&module, HLSL_STORAGE_NONE, float4Type,
                               "position");
    entry = HlslNewFunction(&module, voidType, "cg_entry");
    wrapper = HlslNewFunction(&module, outputType, "main");
    local = HlslNewDecl(&module, HLSL_STORAGE_NONE, voidType, "local");
    initializerCall = HlslNewExpr(&module, HLSL_EXPR_CALL, voidType);
    bodyCall = HlslNewExpr(&module, HLSL_EXPR_CALL, voidType);
    if (inputStruct == NULL || outputStruct == NULL || inputMember == NULL ||
        outputMember == NULL || entry == NULL || wrapper == NULL ||
        local == NULL || initializerCall == NULL || bodyCall == NULL)
    {
        return 0;
    }
    inputMember->semantic = "POSITION0";
    outputMember->semantic = "POSITION0";
    inputStruct->members = inputMember;
    outputStruct->members = outputMember;
    inputStruct->type.members = inputMember;
    outputStruct->type.members = outputMember;
    inputStruct->next = outputStruct;
    module.structs = inputStruct;
    entry->isEntry = 1;
    entry->next = wrapper;
    module.functions = entry;
    module.entry = entry;
    module.wrapper = wrapper;
    initializerCall->u.call.function = entry;
    initializerCall->u.call.name = entry->name;
    local->initializer = initializerCall;
    wrapper->locals = local;
    bodyCall->u.call.function = entry;
    bodyCall->u.call.name = entry->name;
    wrapper->body = ExpressionStmt(&module, bodyCall);
    return Require(!HlslValidateModule(&module, &profile),
                   "initializer entry call was omitted from wrapper count") &&
           Require(module.errorKind == HLSL_ERROR_ENTRY_ABI,
                   "double entry call did not report entry ABI failure");
}

int main(void)
{
    memset(&profile, 0, sizeof(profile));
    profile.stage = HLSL_STAGE_VERTEX;
    profile.name = "hlslv";
    profile.target = "vs_3_0";
    return TestAssignmentTypes() &&
           TestMultiplyShapes() &&
           TestMultiplyResult() &&
           TestCallArity() &&
           TestCallParameterType() &&
           TestMemberOwnership() &&
           TestIndexLegality() &&
           TestReturnType() &&
           TestInitializerAndBodyEntryCalls() ? 0 : 1;
}
