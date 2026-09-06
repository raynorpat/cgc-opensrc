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
// hlsl_codegen.c
//

#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "slglobals.h"
#include "hlsl_hal.h"
#include "hlsl_modern.h"

static int HlslIsIdentifier(const char *name)
{
    const char *current;

    if (name == NULL ||
        !((name[0] >= 'A' && name[0] <= 'Z') ||
          (name[0] >= 'a' && name[0] <= 'z') || name[0] == '_'))
    {
        return 0;
    }
    for (current = name + 1; *current != '\0'; current++) {
        if (!((*current >= 'A' && *current <= 'Z') ||
              (*current >= 'a' && *current <= 'z') ||
              (*current >= '0' && *current <= '9') || *current == '_'))
        {
            return 0;
        }
    }
    return !HlslIsReservedName(name) || !strcmp(name, "main") ||
           !strncmp(name, "cg_", 3);
}

static int HlslCanWriteModule(const HlslModule *module,
                              const HlslProfileDesc *profile)
{
    const HlslFunction *function;
    int sawEntry;
    int sawWrapper;

    if (module == NULL || profile == NULL ||
        (module->stage != HLSL_STAGE_VERTEX &&
         module->stage != HLSL_STAGE_PIXEL &&
         module->stage != HLSL_STAGE_GEOMETRY) ||
        module->stage != profile->stage || profile->name == NULL ||
        profile->name[0] == '\0' || profile->target == NULL ||
        profile->target[0] == '\0' || module->entry == NULL ||
        module->errors != 0 || !HlslIsIdentifier(module->entry->name))
    {
        return 0;
    }
    if (module->wrapper == NULL) {
        if (module->structs != NULL || module->globals != NULL ||
            module->bindings != NULL || module->functions != module->entry ||
            module->entry->next != NULL ||
            strcmp(module->entry->name, "main") ||
            !module->entry->isEntry ||
            module->entry->parameters != NULL ||
            module->entry->locals != NULL || module->entry->body != NULL ||
            module->entry->result.base != HLSL_BASE_VOID ||
            module->entry->result.len != 0 ||
            module->entry->result.rows != 0 ||
            module->entry->result.cols != 0 ||
            module->entry->result.arraySize != 0 ||
            module->entry->result.structName != NULL ||
            module->entry->result.elementType != NULL ||
            module->entry->result.members != NULL)
        {
            return 0;
        }
    } else if (module->wrapper == module->entry ||
               module->wrapper->name == NULL ||
               strcmp(module->wrapper->name, "main") ||
               !module->entry->isEntry)
    {
        return 0;
    }
    sawEntry = 0;
    sawWrapper = module->wrapper == NULL;
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (!HlslIsIdentifier(function->name) ||
            HlslTypeName(&function->result) == NULL)
        {
            return 0;
        }
        if (function == module->entry)
            sawEntry = 1;
        if (function == module->wrapper)
            sawWrapper = 1;
    }
    return sawEntry && sawWrapper;
} // HlslCanWriteModule

static int HlslWriteIndent(FILE *out, int indent)
{
    while (indent-- > 0) {
        if (fputs("    ", out) == EOF)
            return 0;
    }
    return 1;
} // HlslWriteIndent

static int HlslWriteArrayDimensions(FILE *out, const HlslType *type,
                                    int depth)
{
    if (type == NULL || depth > 128 || type->arraySize < 0)
        return 0;
    if (type->arraySize == 0)
        return 1;
    return type->elementType != NULL &&
           fprintf(out, "[%d]", type->arraySize) >= 0 &&
           HlslWriteArrayDimensions(out, type->elementType, depth + 1);
} // HlslWriteArrayDimensions

static int HlslWriteTypeAndName(FILE *out, const HlslType *type,
                                const char *name)
{
    const char *typeName;

    typeName = HlslTypeName(type);
    if (typeName == NULL || name == NULL ||
        fprintf(out, "%s %s", typeName, name) < 0)
    {
        return 0;
    }
    if (!HlslWriteArrayDimensions(out, type, 0))
        return 0;
    return 1;
} // HlslWriteTypeAndName

static int HlslIsScalarBooleanAggregate(const HlslType *type)
{
    const HlslDecl *member;

    if (type == NULL)
        return 0;
    if (type->arraySize > 0)
        return HlslIsScalarBooleanAggregate(type->elementType);
    if (type->base == HLSL_BASE_STRUCT) {
        if (type->members == NULL)
            return 0;
        for (member = type->members; member != NULL; member = member->next) {
            if (!HlslIsScalarBooleanAggregate(&member->type))
                return 0;
        }
        return 1;
    }
    return type->base == HLSL_BASE_BOOL && type->rows == 0 &&
           type->cols == 0 && type->len >= 1 && type->len <= 4;
} // HlslIsScalarBooleanAggregate

static int HlslIsIntegerAggregate(const HlslType *type)
{
    const HlslDecl *member;

    if (type == NULL)
        return 0;
    if (type->arraySize > 0)
        return HlslIsIntegerAggregate(type->elementType);
    if (type->base == HLSL_BASE_STRUCT) {
        if (type->members == NULL)
            return 0;
        for (member = type->members; member != NULL; member = member->next) {
            if (!HlslIsIntegerAggregate(&member->type))
                return 0;
        }
        return 1;
    }
    return type->base == HLSL_BASE_INT && type->rows == 0 &&
           type->cols == 0 && type->len >= 1 && type->len <= 4;
} // HlslIsIntegerAggregate

static int HlslIsScalarBooleanType(const HlslType *type)
{
    return type != NULL && type->arraySize == 0 &&
           type->base == HLSL_BASE_BOOL && type->len == 1 &&
           type->rows == 0 && type->cols == 0;
} // HlslIsScalarBooleanType

static int HlslWriteDeclTypeAndName(FILE *out, const HlslDecl *decl)
{
    const HlslType *element;

    if (decl->type.arraySize == 0 && decl->type.len == 1 &&
        (decl->type.base == HLSL_BASE_TEXTURE1D ||
         decl->type.base == HLSL_BASE_TEXTURE2D ||
         decl->type.base == HLSL_BASE_TEXTURE3D ||
         decl->type.base == HLSL_BASE_TEXTURECUBE))
    {
        return fprintf(out, "%s<float4> %s",
                       HlslTypeName(&decl->type), decl->name) >= 0;
    }

    if (decl->storage == HLSL_STORAGE_UNIFORM &&
        decl->physical.bank == HLSL_REGISTER_I)
    {
        if (!HlslIsIntegerAggregate(&decl->type) ||
            decl->physical.span <= 0 ||
            fprintf(out, "int4 %s", decl->name) < 0)
        {
            return 0;
        }
        if (decl->physical.span > 1 &&
            fprintf(out, "[%d]", decl->physical.span) < 0)
        {
            return 0;
        }
        return 1;
    }
    if (decl->storage == HLSL_STORAGE_UNIFORM &&
        decl->physical.bank == HLSL_REGISTER_B &&
        !HlslIsScalarBooleanType(&decl->type))
    {
        /* Preserve one logical binding/default record for the runtime
           setter, but expose its contiguous SM3 b# span as scalar storage. */
        element = &decl->type;
        if (!HlslIsScalarBooleanAggregate(element))
            return 0;
        if (decl->physical.span == 1)
            return fprintf(out, "bool %s", decl->name) >= 0;
        if (decl->physical.span > 1)
            return fprintf(out, "bool %s[%d]", decl->name,
                           decl->physical.span) >= 0;
        return 0;
    }
    return HlslWriteTypeAndName(out, &decl->type, decl->name);
} // HlslWriteDeclTypeAndName

static int HlslWriteFloat(FILE *out, float value)
{
    char buffer[64];

    sprintf(buffer, "%.9g", value);
    if (fputs(buffer, out) == EOF)
        return 0;
    if (strchr(buffer, '.') == NULL && strchr(buffer, 'e') == NULL &&
        strchr(buffer, 'E') == NULL && fputs(".0", out) == EOF)
    {
        return 0;
    }
    return 1;
} // HlslWriteFloat

static int HlslWriteDefaultLiteral(FILE *out,
                                   const HlslDefaultLiteral *value)
{
    if (value == NULL)
        return 0;
    switch (value->base) {
    case HLSL_BASE_FLOAT:
        return HlslWriteFloat(out, value->value.floating);
    case HLSL_BASE_INT:
        return fprintf(out, "%d.0", value->value.integer) >= 0;
    case HLSL_BASE_BOOL:
        return fputs(value->value.boolean ? "1.0" : "0.0", out) != EOF;
    default:
        return 0;
    }
} // HlslWriteDefaultLiteral

static const char *HlslValueTypeName(const HlslType *type)
{
    const char *name;

    name = HlslTypeName(type);
    if (name != NULL && !strncmp(name, "row_major ", 10))
        name += 10;
    return name;
} // HlslValueTypeName

static int HlslExprPrecedence(const HlslExpr *expression)
{
    if (expression == NULL)
        return 0;
    if (expression->kind == HLSL_EXPR_CONDITIONAL)
        return 2;
    if (expression->kind == HLSL_EXPR_UNARY ||
        expression->kind == HLSL_EXPR_CAST)
    {
        return 13;
    }
    if (expression->kind != HLSL_EXPR_BINARY)
        return 14;
    switch (expression->u.binary.op) {
    case HLSL_OP_ASSIGN:
    case HLSL_OP_ADD_ASSIGN:
    case HLSL_OP_SUBTRACT_ASSIGN:
    case HLSL_OP_MULTIPLY_ASSIGN:
    case HLSL_OP_DIVIDE_ASSIGN:
    case HLSL_OP_REMAINDER_ASSIGN:
    case HLSL_OP_BITWISE_OR_ASSIGN:
    case HLSL_OP_BITWISE_XOR_ASSIGN:
    case HLSL_OP_BITWISE_AND_ASSIGN:
    case HLSL_OP_SHIFT_LEFT_ASSIGN:
    case HLSL_OP_SHIFT_RIGHT_ASSIGN: return 1;
    case HLSL_OP_LOGICAL_OR: return 3;
    case HLSL_OP_LOGICAL_AND: return 4;
    case HLSL_OP_BITWISE_OR: return 5;
    case HLSL_OP_BITWISE_XOR: return 6;
    case HLSL_OP_BITWISE_AND: return 7;
    case HLSL_OP_EQUAL:
    case HLSL_OP_NOT_EQUAL: return 8;
    case HLSL_OP_LESS:
    case HLSL_OP_GREATER:
    case HLSL_OP_LESS_EQUAL:
    case HLSL_OP_GREATER_EQUAL: return 9;
    case HLSL_OP_SHIFT_LEFT:
    case HLSL_OP_SHIFT_RIGHT: return 10;
    case HLSL_OP_ADD:
    case HLSL_OP_SUBTRACT: return 11;
    case HLSL_OP_MULTIPLY:
    case HLSL_OP_DIVIDE:
    case HLSL_OP_REMAINDER: return 12;
    default: return 0;
    }
} // HlslExprPrecedence

static const char *HlslOperatorText(HlslOperator op)
{
    switch (op) {
    case HLSL_OP_ASSIGN: return "=";
    case HLSL_OP_ADD_ASSIGN: return "+=";
    case HLSL_OP_SUBTRACT_ASSIGN: return "-=";
    case HLSL_OP_MULTIPLY_ASSIGN: return "*=";
    case HLSL_OP_DIVIDE_ASSIGN: return "/=";
    case HLSL_OP_REMAINDER_ASSIGN: return "%=";
    case HLSL_OP_BITWISE_OR_ASSIGN: return "|=";
    case HLSL_OP_BITWISE_XOR_ASSIGN: return "^=";
    case HLSL_OP_BITWISE_AND_ASSIGN: return "&=";
    case HLSL_OP_SHIFT_LEFT_ASSIGN: return "<<=";
    case HLSL_OP_SHIFT_RIGHT_ASSIGN: return ">>=";
    case HLSL_OP_LOGICAL_OR: return "||";
    case HLSL_OP_LOGICAL_AND: return "&&";
    case HLSL_OP_BITWISE_OR: return "|";
    case HLSL_OP_BITWISE_XOR: return "^";
    case HLSL_OP_BITWISE_AND: return "&";
    case HLSL_OP_EQUAL: return "==";
    case HLSL_OP_NOT_EQUAL: return "!=";
    case HLSL_OP_LESS: return "<";
    case HLSL_OP_GREATER: return ">";
    case HLSL_OP_LESS_EQUAL: return "<=";
    case HLSL_OP_GREATER_EQUAL: return ">=";
    case HLSL_OP_SHIFT_LEFT: return "<<";
    case HLSL_OP_SHIFT_RIGHT: return ">>";
    case HLSL_OP_ADD: return "+";
    case HLSL_OP_SUBTRACT: return "-";
    case HLSL_OP_MULTIPLY: return "*";
    case HLSL_OP_DIVIDE: return "/";
    case HLSL_OP_REMAINDER: return "%";
    case HLSL_OP_NEGATE: return "-";
    case HLSL_OP_POSITIVE: return "+";
    case HLSL_OP_LOGICAL_NOT: return "!";
    case HLSL_OP_BITWISE_NOT: return "~";
    case HLSL_OP_PRE_INCREMENT:
    case HLSL_OP_POST_INCREMENT: return "++";
    case HLSL_OP_PRE_DECREMENT:
    case HLSL_OP_POST_DECREMENT: return "--";
    default: return NULL;
    }
} // HlslOperatorText

static int HlslIsAssignmentOperator(HlslOperator op)
{
    return op >= HLSL_OP_ASSIGN && op <= HLSL_OP_SHIFT_RIGHT_ASSIGN;
} // HlslIsAssignmentOperator

static const HlslDecl *HlslPhysicalIntegerDecl(
    const HlslExpr *expression, int *offset)
{
    const HlslDecl *member;
    const HlslDecl *declaration;
    int memberSpan;
    int nestedOffset;

    if (expression == NULL || offset == NULL)
        return NULL;
    if (expression->kind == HLSL_EXPR_SYMBOL) {
        declaration = expression->u.symbol;
        if (declaration == NULL ||
            declaration->physical.bank != HLSL_REGISTER_I)
        {
            return NULL;
        }
        *offset = 0;
        return declaration;
    }
    if (expression->kind == HLSL_EXPR_INDEX &&
        expression->u.index.object != NULL &&
        expression->u.index.object->type.arraySize > 0 &&
        expression->u.index.object->type.elementType != NULL &&
        expression->u.index.index != NULL &&
        expression->u.index.index->kind == HLSL_EXPR_INT &&
        expression->u.index.index->u.literalInt >= 0 &&
        expression->u.index.index->u.literalInt <
            expression->u.index.object->type.arraySize)
    {
        declaration = HlslPhysicalIntegerDecl(
            expression->u.index.object, &nestedOffset);
        memberSpan = HlslTypeRegisterSpan(
            expression->u.index.object->type.elementType);
        if (declaration == NULL || memberSpan <= 0)
            return NULL;
        *offset = nestedOffset +
            expression->u.index.index->u.literalInt * memberSpan;
        return declaration;
    }
    if (expression->kind == HLSL_EXPR_MEMBER &&
        expression->u.member.object != NULL &&
        expression->u.member.object->type.base == HLSL_BASE_STRUCT)
    {
        declaration = HlslPhysicalIntegerDecl(
            expression->u.member.object, &nestedOffset);
        if (declaration == NULL)
            return NULL;
        for (member = expression->u.member.object->type.members;
             member != NULL; member = member->next)
        {
            if (member == expression->u.member.decl ||
                (expression->u.member.name != NULL && member->name != NULL &&
                 !strcmp(member->name, expression->u.member.name)))
            {
                *offset = nestedOffset;
                return declaration;
            }
            memberSpan = HlslTypeRegisterSpan(&member->type);
            if (memberSpan <= 0)
                return NULL;
            nestedOffset += memberSpan;
        }
    }
    return NULL;
} // HlslPhysicalIntegerDecl

static int HlslWritePhysicalIntegerValue(FILE *out,
                                         const HlslExpr *expression)
{
    const HlslDecl *declaration;
    const char *swizzle;
    int offset;

    if (expression == NULL || expression->type.arraySize != 0 ||
        expression->type.base != HLSL_BASE_INT ||
        expression->type.rows != 0 || expression->type.cols != 0 ||
        expression->type.len < 1 || expression->type.len > 4)
    {
        return 0;
    }
    declaration = HlslPhysicalIntegerDecl(expression, &offset);
    if (declaration == NULL || offset < 0 ||
        offset >= declaration->physical.span)
    {
        return 0;
    }
    if (declaration->physical.span == 1) {
        if (fputs(declaration->name, out) == EOF)
            return -1;
    } else if (fprintf(out, "%s[%d]", declaration->name, offset) < 0) {
        return -1;
    }
    swizzle = NULL;
    switch (expression->type.len) {
    case 1: swizzle = "x"; break;
    case 2: swizzle = "xy"; break;
    case 3: swizzle = "xyz"; break;
    default: break;
    }
    if (swizzle != NULL && fprintf(out, ".%s", swizzle) < 0)
        return -1;
    return 1;
} // HlslWritePhysicalIntegerValue

static const HlslDecl *HlslPhysicalBooleanDecl(
    const HlslExpr *expression, int *offset)
{
    const HlslDecl *member;
    const HlslDecl *declaration;
    int memberSpan;
    int nestedOffset;

    if (expression == NULL || offset == NULL)
        return NULL;
    if (expression->kind == HLSL_EXPR_SYMBOL) {
        declaration = expression->u.symbol;
        if (declaration == NULL ||
            declaration->physical.bank != HLSL_REGISTER_B ||
            (declaration->physical.span <= 1 &&
             HlslIsScalarBooleanType(&declaration->type)))
        {
            return NULL;
        }
        *offset = 0;
        return declaration;
    }
    if (expression->kind == HLSL_EXPR_INDEX &&
        expression->u.index.object != NULL &&
        expression->u.index.object->type.arraySize > 0 &&
        expression->u.index.object->type.elementType != NULL &&
        expression->u.index.index != NULL &&
        expression->u.index.index->kind == HLSL_EXPR_INT &&
        expression->u.index.index->u.literalInt >= 0 &&
        expression->u.index.index->u.literalInt <
            expression->u.index.object->type.arraySize)
    {
        declaration = HlslPhysicalBooleanDecl(
            expression->u.index.object, &nestedOffset);
        memberSpan = HlslTypeRegisterSpan(
            expression->u.index.object->type.elementType);
        if (declaration == NULL || memberSpan <= 0)
            return NULL;
        *offset = nestedOffset +
            expression->u.index.index->u.literalInt * memberSpan;
        return declaration;
    }
    if (expression->kind == HLSL_EXPR_MEMBER &&
        expression->u.member.object != NULL &&
        expression->u.member.object->type.base == HLSL_BASE_STRUCT)
    {
        declaration = HlslPhysicalBooleanDecl(
            expression->u.member.object, &nestedOffset);
        if (declaration == NULL)
            return NULL;
        for (member = expression->u.member.object->type.members;
             member != NULL; member = member->next)
        {
            if (member == expression->u.member.decl ||
                (expression->u.member.name != NULL && member->name != NULL &&
                 !strcmp(member->name, expression->u.member.name)))
            {
                *offset = nestedOffset;
                return declaration;
            }
            memberSpan = HlslTypeRegisterSpan(&member->type);
            if (memberSpan <= 0)
                return NULL;
            nestedOffset += memberSpan;
        }
    }
    return NULL;
} // HlslPhysicalBooleanDecl

static int HlslWritePhysicalBooleanValue(FILE *out,
                                         const HlslExpr *expression)
{
    const HlslDecl *declaration;
    const char *typeName;
    int offset;
    int i;

    if (expression == NULL || expression->type.arraySize != 0 ||
        expression->type.base != HLSL_BASE_BOOL ||
        expression->type.rows != 0 || expression->type.cols != 0 ||
        expression->type.len < 1 || expression->type.len > 4)
    {
        return 0;
    }
    declaration = HlslPhysicalBooleanDecl(expression, &offset);
    if (declaration == NULL || offset < 0 ||
        offset > declaration->physical.span - expression->type.len)
    {
        return 0;
    }
    if (expression->type.len == 1) {
        if (declaration->physical.span == 1)
            return fputs(declaration->name, out) == EOF ? -1 : 1;
        return fprintf(out, "%s[%d]", declaration->name, offset) >= 0 ?
               1 : -1;
    }
    typeName = HlslValueTypeName(&expression->type);
    if (typeName == NULL || fprintf(out, "%s(", typeName) < 0)
        return -1;
    for (i = 0; i < expression->type.len; i++) {
        if ((i > 0 && fputs(", ", out) == EOF) ||
            fprintf(out, "%s[%d]", declaration->name, offset + i) < 0)
        {
            return -1;
        }
    }
    return fputc(')', out) == EOF ? -1 : 1;
} // HlslWritePhysicalBooleanValue

static int HlslWriteExprImpl(FILE *out, const HlslExpr *expression,
                             int parentPrecedence,
                             int writePhysicalSwizzle);

static int HlslWriteExpr(FILE *out, const HlslExpr *expression,
                         int parentPrecedence)
{
    return HlslWriteExprImpl(out, expression, parentPrecedence, 1);
} // HlslWriteExpr

static int HlslWriteExprImpl(FILE *out, const HlslExpr *expression,
                             int parentPrecedence,
                             int writePhysicalSwizzle)
{
    const HlslDecl *parameter;
    const HlslExpr *argument;
    const char *text;
    int precedence;
    int parentheses;
    int physicalInteger;
    int physicalBoolean;
    int first;

    if (expression == NULL)
        return 0;
    precedence = HlslExprPrecedence(expression);
    parentheses = precedence < parentPrecedence;
    if (parentheses && fputc('(', out) == EOF)
        return 0;
    physicalInteger = writePhysicalSwizzle ?
        HlslWritePhysicalIntegerValue(out, expression) : 0;
    if (physicalInteger < 0)
        return 0;
    if (physicalInteger > 0)
        goto wrote_expression;
    physicalBoolean = writePhysicalSwizzle ?
        HlslWritePhysicalBooleanValue(out, expression) : 0;
    if (physicalBoolean < 0)
        return 0;
    if (physicalBoolean > 0)
        goto wrote_expression;
    switch (expression->kind) {
    case HLSL_EXPR_SYMBOL:
        if (expression->u.symbol == NULL ||
            fputs(expression->u.symbol->name, out) == EOF)
        {
            return 0;
        }
        break;
    case HLSL_EXPR_INT:
        if (fprintf(out, "%d", expression->u.literalInt) < 0)
            return 0;
        break;
    case HLSL_EXPR_FLOAT:
        if (!HlslWriteFloat(out, expression->u.literalFloat))
            return 0;
        break;
    case HLSL_EXPR_BOOL:
        if (fputs(expression->u.literalBool ? "true" : "false", out) == EOF)
            return 0;
        break;
    case HLSL_EXPR_UNARY:
        text = HlslOperatorText(expression->u.unary.op);
        if (text == NULL)
            return 0;
        if (expression->u.unary.op == HLSL_OP_POST_INCREMENT ||
            expression->u.unary.op == HLSL_OP_POST_DECREMENT)
        {
            if (!HlslWriteExpr(out, expression->u.unary.operand,
                               precedence) || fputs(text, out) == EOF)
            {
                return 0;
            }
        } else if (fputs(text, out) == EOF ||
                   !HlslWriteExpr(out, expression->u.unary.operand,
                                  precedence + 1))
        {
            return 0;
        }
        break;
    case HLSL_EXPR_BINARY:
        text = HlslOperatorText(expression->u.binary.op);
        if (text == NULL ||
            !HlslWriteExpr(out, expression->u.binary.left, precedence) ||
            fprintf(out, " %s ", text) < 0 ||
            !HlslWriteExpr(out, expression->u.binary.right,
                           precedence +
                           (HlslIsAssignmentOperator(
                                expression->u.binary.op) ? 0 : 1)))
        {
            return 0;
        }
        break;
    case HLSL_EXPR_CONDITIONAL:
        if (!HlslWriteExpr(out, expression->u.conditional.condition,
                           precedence + 1) ||
            fputs(" ? ", out) == EOF ||
            !HlslWriteExpr(out, expression->u.conditional.trueExpr,
                           precedence) ||
            fputs(" : ", out) == EOF ||
            !HlslWriteExpr(out, expression->u.conditional.falseExpr,
                           precedence))
        {
            return 0;
        }
        break;
    case HLSL_EXPR_CALL:
        if (expression->u.call.name == NULL ||
            fprintf(out, "%s(", expression->u.call.name) < 0)
        {
            return 0;
        }
        first = 1;
        parameter = expression->u.call.function != NULL ?
                    expression->u.call.function->parameters : NULL;
        for (argument = expression->u.call.arguments; argument != NULL;
             argument = argument->next)
        {
            if ((!first && fputs(", ", out) == EOF) ||
                !HlslWriteExprImpl(out, argument, 0,
                    parameter == NULL ||
                    parameter->physical.bank != HLSL_REGISTER_I))
            {
                return 0;
            }
            first = 0;
            if (parameter != NULL)
                parameter = parameter->next;
        }
        if (fputc(')', out) == EOF)
            return 0;
        break;
    case HLSL_EXPR_CONSTRUCT:
        if (expression->type.arraySize > 0 ||
            expression->type.base == HLSL_BASE_STRUCT)
        {
            if (fputs("{ ", out) == EOF)
                return 0;
        } else {
            text = HlslValueTypeName(&expression->type);
            if (text == NULL || fprintf(out, "%s(", text) < 0)
                return 0;
        }
        first = 1;
        for (argument = expression->u.construct.arguments;
             argument != NULL; argument = argument->next)
        {
            if ((!first && fputs(", ", out) == EOF) ||
                !HlslWriteExpr(out, argument, 0))
            {
                return 0;
            }
            first = 0;
        }
        if (fputs(expression->type.arraySize > 0 ||
                  expression->type.base == HLSL_BASE_STRUCT ?
                  " }" : ")", out) == EOF)
            return 0;
        break;
    case HLSL_EXPR_CAST:
        text = HlslValueTypeName(&expression->type);
        if (text == NULL || fprintf(out, "(%s) ", text) < 0 ||
            !HlslWriteExpr(out, expression->u.cast.expression,
                           precedence))
        {
            return 0;
        }
        break;
    case HLSL_EXPR_MEMBER:
        if (!HlslWriteExpr(out, expression->u.member.object, 14) ||
            fprintf(out, ".%s", expression->u.member.name) < 0)
        {
            return 0;
        }
        break;
    case HLSL_EXPR_INDEX:
        if (!HlslWriteExprImpl(out, expression->u.index.object, 14, 0) ||
            fputc('[', out) == EOF ||
            !HlslWriteExpr(out, expression->u.index.index, 0) ||
            fputc(']', out) == EOF)
        {
            return 0;
        }
        break;
    case HLSL_EXPR_SWIZZLE:
        if (!HlslWriteExprImpl(out, expression->u.swizzle.object, 14, 0) ||
            expression->u.swizzle.mask == NULL ||
            fprintf(out, ".%s", expression->u.swizzle.mask) < 0)
        {
            return 0;
        }
        break;
    case HLSL_EXPR_TEXTURE_METHOD:
        text = expression->u.textureMethod.method ==
                   HLSL_TEXTURE_METHOD_SAMPLE ? "Sample" :
               expression->u.textureMethod.method ==
                   HLSL_TEXTURE_METHOD_SAMPLE_LEVEL ? "SampleLevel" :
               expression->u.textureMethod.method ==
                   HLSL_TEXTURE_METHOD_SAMPLE_BIAS ? "SampleBias" :
               expression->u.textureMethod.method ==
                   HLSL_TEXTURE_METHOD_SAMPLE_GRAD ? "SampleGrad" : NULL;
        if (text == NULL ||
            !HlslWriteExpr(out, expression->u.textureMethod.texture, 14) ||
            fprintf(out, ".%s(", text) < 0 ||
            !HlslWriteExpr(out, expression->u.textureMethod.sampler, 0) ||
            fputs(", ", out) == EOF ||
            !HlslWriteExpr(out,
                expression->u.textureMethod.coordinates, 0) ||
            (expression->u.textureMethod.argument1 != NULL &&
             (fputs(", ", out) == EOF ||
              !HlslWriteExpr(out,
                  expression->u.textureMethod.argument1, 0))) ||
            (expression->u.textureMethod.argument2 != NULL &&
             (fputs(", ", out) == EOF ||
              !HlslWriteExpr(out,
                  expression->u.textureMethod.argument2, 0))) ||
            fputc(')', out) == EOF)
        {
            return 0;
        }
        break;
    default:
        return 0;
    }
wrote_expression:
    if (parentheses && fputc(')', out) == EOF)
        return 0;
    return 1;
} // HlslWriteExprImpl

static int HlslWriteDecl(FILE *out, const HlslDecl *decl, int indent,
                         int withSemantic)
{
    if (!HlslWriteIndent(out, indent))
        return 0;
    /* FXC treats SM3 i#/b# globals without uniform as local constants. */
    if (decl->storage == HLSL_STORAGE_UNIFORM &&
        (decl->physical.bank == HLSL_REGISTER_I ||
         decl->physical.bank == HLSL_REGISTER_B) &&
        fputs("uniform ", out) == EOF)
    {
        return 0;
    }
    if (decl->storageClass == HLSL_STORAGE_CLASS_STATIC &&
        fputs("static ", out) == EOF)
    {
        return 0;
    }
    if (decl->storageClass == HLSL_STORAGE_CLASS_EXTERN &&
        fputs("extern ", out) == EOF)
    {
        return 0;
    }
    if (decl->typeQualifier == HLSL_TYPE_QUALIFIER_CONST &&
        fputs("const ", out) == EOF)
    {
        return 0;
    }
    if (decl->interpolation != HLSL_INTERPOLATION_DEFAULT &&
        (HlslInterpolationName(decl->interpolation) == NULL ||
         fprintf(out, "%s ",
                 HlslInterpolationName(decl->interpolation)) < 0))
    {
        return 0;
    }
    if (!HlslWriteDeclTypeAndName(out, decl))
        return 0;
    if (withSemantic && decl->semantic != NULL &&
        fprintf(out, " : %s", decl->semantic) < 0)
    {
        return 0;
    }
    if (decl->physical.bank != HLSL_REGISTER_NONE &&
        fprintf(out, " : register(%c%d)",
                "?cibs"[decl->physical.bank],
                decl->physical.regno) < 0)
    {
        return 0;
    }
    /* SM3 i#/b# uniforms cannot carry source initializers.  The stable
       cgc-default metadata above remains the runtime default contract. */
    if (decl->initializer != NULL &&
        decl->physical.bank != HLSL_REGISTER_I &&
        decl->physical.bank != HLSL_REGISTER_B &&
        (fputs(" = ", out) == EOF ||
         !HlslWriteExpr(out, decl->initializer, 0)))
    {
        return 0;
    }
    return fputs(";\n", out) != EOF;
} // HlslWriteDecl

static int HlslWriteStruct(FILE *out, const HlslDecl *structure)
{
    const HlslDecl *member;

    if (fprintf(out, "struct %s\n{\n", structure->name) < 0)
        return 0;
    for (member = structure->members; member != NULL; member = member->next) {
        if (!HlslWriteDecl(out, member, 1,
                           structure->storage == HLSL_STORAGE_INPUT ||
                           structure->storage == HLSL_STORAGE_OUTPUT))
        {
            return 0;
        }
    }
    return fputs("};\n", out) != EOF;
} // HlslWriteStruct

static int HlslWriteParameters(FILE *out, const HlslDecl *parameter)
{
    int first;

    first = 1;
    for (; parameter != NULL; parameter = parameter->next) {
        if ((!first && fputs(", ", out) == EOF))
            return 0;
        if (parameter->parameterQualifier == HLSL_PARAMETER_OUT &&
            fputs("out ", out) == EOF)
        {
            return 0;
        }
        if (parameter->parameterQualifier == HLSL_PARAMETER_INOUT &&
            fputs("inout ", out) == EOF)
        {
            return 0;
        }
        if (parameter->canonicalSemantic != NULL &&
            parameter->interpolation != HLSL_INTERPOLATION_DEFAULT &&
            (HlslInterpolationName(parameter->interpolation) == NULL ||
             fprintf(out, "%s ",
                HlslInterpolationName(parameter->interpolation)) < 0))
        {
            return 0;
        }
        if (!HlslWriteDeclTypeAndName(out, parameter))
            return 0;
        if (parameter->canonicalSemantic != NULL &&
            (parameter->semantic == NULL ||
             fprintf(out, " : %s", parameter->semantic) < 0))
        {
            return 0;
        }
        first = 0;
    }
    return 1;
} // HlslWriteParameters

static int HlslWriteStatements(FILE *out, const HlslStmt *statement,
                               int indent);

static int HlslWriteBracedBody(FILE *out, const HlslStmt *body, int indent)
{
    return HlslWriteIndent(out, indent) && fputs("{\n", out) != EOF &&
           HlslWriteStatements(out, body, indent + 1) &&
           HlslWriteIndent(out, indent) && fputs("}\n", out) != EOF;
} // HlslWriteBracedBody

static int HlslWriteForPart(FILE *out, const HlslStmt *statement)
{
    int first;

    first = 1;
    for (; statement != NULL; statement = statement->next) {
        if (statement->kind != HLSL_STMT_EXPRESSION ||
            (!first && fputs(", ", out) == EOF) ||
            !HlslWriteExpr(out, statement->u.expression, 0))
        {
            return 0;
        }
        first = 0;
    }
    return 1;
} // HlslWriteForPart

static int HlslStatementsAreEmpty(const HlslStmt *statement)
{
    for (; statement != NULL; statement = statement->next) {
        if (statement->kind != HLSL_STMT_BLOCK ||
            !HlslStatementsAreEmpty(statement->u.block))
        {
            return 0;
        }
    }
    return 1;
} // HlslStatementsAreEmpty

static int HlslStatementsBreakImmediately(const HlslStmt *statement)
{
    while (statement != NULL && statement->next == NULL &&
           statement->kind == HLSL_STMT_BLOCK)
    {
        statement = statement->u.block;
    }
    return statement != NULL && statement->next == NULL &&
           statement->kind == HLSL_STMT_BREAK;
} // HlslStatementsBreakImmediately

static int HlslExpressionIsConstantFalse(const HlslExpr *expression)
{
    return expression != NULL && expression->kind == HLSL_EXPR_BOOL &&
           !expression->u.literalBool;
} // HlslExpressionIsConstantFalse

static int HlslWriteLoopAttribute(FILE *out, const HlslExpr *condition,
                                  const HlslStmt *body, int indent)
{
    if (!(HlslStatementsAreEmpty(body) &&
          HlslExpressionIsConstantFalse(condition)) &&
        !HlslStatementsBreakImmediately(body))
    {
        return 1;
    }
    return fputs("[unroll]\n", out) != EOF &&
           HlslWriteIndent(out, indent);
} // HlslWriteLoopAttribute

static int HlslWriteStatements(FILE *out, const HlslStmt *statement,
                               int indent)
{
    for (; statement != NULL; statement = statement->next) {
        if (!HlslWriteIndent(out, indent))
            return 0;
        switch (statement->kind) {
        case HLSL_STMT_DECLARATION:
            if (statement->u.declaration == NULL ||
                !HlslWriteDecl(out, statement->u.declaration, 0, 0))
            {
                return 0;
            }
            break;
        case HLSL_STMT_EXPRESSION:
            if (!HlslWriteExpr(out, statement->u.expression, 0) ||
                fputs(";\n", out) == EOF)
            {
                return 0;
            }
            break;
        case HLSL_STMT_IF:
            if (fputs("if (", out) == EOF ||
                !HlslWriteExpr(out, statement->u.ifStmt.condition, 0) ||
                fputs(")\n", out) == EOF ||
                !HlslWriteBracedBody(out,
                    statement->u.ifStmt.trueBranch, indent))
            {
                return 0;
            }
            if (statement->u.ifStmt.falseBranch != NULL &&
                (!HlslWriteIndent(out, indent) ||
                 fputs("else\n", out) == EOF ||
                 !HlslWriteBracedBody(out,
                    statement->u.ifStmt.falseBranch, indent)))
            {
                return 0;
            }
            break;
        case HLSL_STMT_WHILE:
            if (!HlslWriteLoopAttribute(out, statement->u.loop.condition,
                                        statement->u.loop.body, indent) ||
                fputs("while (", out) == EOF ||
                !HlslWriteExpr(out, statement->u.loop.condition, 0) ||
                fputs(")\n", out) == EOF ||
                !HlslWriteBracedBody(out, statement->u.loop.body, indent))
            {
                return 0;
            }
            break;
        case HLSL_STMT_DO:
            if (!HlslWriteLoopAttribute(out, statement->u.loop.condition,
                                        statement->u.loop.body, indent) ||
                fputs("do\n", out) == EOF ||
                !HlslWriteBracedBody(out, statement->u.loop.body, indent) ||
                !HlslWriteIndent(out, indent) ||
                fputs("while (", out) == EOF ||
                !HlslWriteExpr(out, statement->u.loop.condition, 0) ||
                fputs(");\n", out) == EOF)
            {
                return 0;
            }
            break;
        case HLSL_STMT_FOR:
            if (!HlslWriteLoopAttribute(out,
                    statement->u.forStmt.condition,
                    statement->u.forStmt.body, indent) ||
                fputs("for (", out) == EOF ||
                !HlslWriteForPart(out, statement->u.forStmt.init) ||
                fputs("; ", out) == EOF ||
                (statement->u.forStmt.condition != NULL &&
                 !HlslWriteExpr(out,
                    statement->u.forStmt.condition, 0)) ||
                fputs("; ", out) == EOF ||
                !HlslWriteForPart(out, statement->u.forStmt.step) ||
                fputs(")\n", out) == EOF ||
                !HlslWriteBracedBody(out,
                    statement->u.forStmt.body, indent))
            {
                return 0;
            }
            break;
        case HLSL_STMT_RETURN:
            if (fputs("return", out) == EOF)
                return 0;
            if (statement->u.returnExpr != NULL &&
                (fputc(' ', out) == EOF ||
                 !HlslWriteExpr(out, statement->u.returnExpr, 0)))
            {
                return 0;
            }
            if (fputs(";\n", out) == EOF)
                return 0;
            break;
        case HLSL_STMT_BLOCK:
            if (fputs("{\n", out) == EOF ||
                !HlslWriteStatements(out, statement->u.block, indent + 1) ||
                !HlslWriteIndent(out, indent) || fputs("}\n", out) == EOF)
            {
                return 0;
            }
            break;
        case HLSL_STMT_DISCARD:
            if (fputs("discard;\n", out) == EOF)
                return 0;
            break;
        case HLSL_STMT_BREAK:
            if (fputs("break;\n", out) == EOF)
                return 0;
            break;
        case HLSL_STMT_CONTINUE:
            if (fputs("continue;\n", out) == EOF)
                return 0;
            break;
        default:
            return 0;
        }
    }
    return 1;
} // HlslWriteStatements

static int HlslWriteFunctionHeader(FILE *out,
                                   const HlslFunction *function)
{
    if (fprintf(out, "%s %s(", HlslTypeName(&function->result),
                function->name) < 0 ||
        !HlslWriteParameters(out, function->parameters) ||
        fputc(')', out) == EOF)
    {
        return 0;
    }
    return 1;
} // HlslWriteFunctionHeader

static int HlslWriteFunction(FILE *out, const HlslFunction *function)
{
    const HlslDecl *local;

    if (!HlslWriteFunctionHeader(out, function) ||
        fputs("\n{\n", out) == EOF)
    {
        return 0;
    }
    for (local = function->locals; local != NULL; local = local->next) {
        if (!HlslWriteDecl(out, local, 1, 0))
            return 0;
    }
    if (!HlslWriteStatements(out, function->body, 1))
        return 0;
    return fputs("}\n", out) != EOF;
} // HlslWriteFunction

static const char *HlslBankText(HlslRegisterBank bank)
{
    switch (bank) {
    case HLSL_REGISTER_C: return "c";
    case HLSL_REGISTER_I: return "i";
    case HLSL_REGISTER_B: return "b";
    case HLSL_REGISTER_S: return "s";
    case HLSL_REGISTER_T: return "t";
    case HLSL_REGISTER_CB: return "b";
    case HLSL_REGISTER_NONE: break;
    }
    return "?";
} // HlslBankText

static int HlslWriteInterfaceMetadata(FILE *out, const HlslModule *module,
                                      const HlslProfileDesc *profile)
{
    const HlslDecl *structure;
    const HlslDecl *member;
    const char *direction;
    const char *publicName;
    const char *typeName;
    int wroteRecord;

    wroteRecord = 0;
    for (structure = module->structs; structure != NULL;
         structure = structure->next)
    {
        if (structure->storage == HLSL_STORAGE_INPUT)
            direction = "in";
        else if (structure->storage == HLSL_STORAGE_OUTPUT)
            direction = "out";
        else
            continue;
        for (member = structure->members; member != NULL;
             member = member->next)
        {
            publicName = member->publicName != NULL &&
                         member->publicName[0] != '\0' ?
                         member->publicName : member->name;
            typeName = HlslTypeName(&member->type);
            if (publicName == NULL || typeName == NULL ||
                member->semantic == NULL)
            {
                return -1;
            }
            if (profile->semanticPolicy == HLSL_SEMANTIC_POLICY_MODERN) {
                const char *canonical;
                const char *classification;
                const char *interpolation;

                canonical = member->canonicalSemantic;
                classification = member->semanticKind ==
                                 HLSL_SEMANTIC_USER ? "user" : "system";
                interpolation = HlslInterpolationName(
                                    member->interpolation);
                if (canonical == NULL || interpolation == NULL ||
                    fprintf(out,
                        "// cgc-bind interface %s %s %s %s %s %s %s\n",
                        direction, publicName, typeName, member->semantic,
                        canonical, classification, interpolation) < 0)
                {
                    return -1;
                }
            } else if (fprintf(out,
                    "// cgc-bind interface %s %s %s %s\n",
                    direction, publicName, typeName, member->semantic) < 0)
            {
                return -1;
            }
            wroteRecord = 1;
        }
    }
    if (profile->semanticPolicy == HLSL_SEMANTIC_POLICY_MODERN &&
        module->stage == HLSL_STAGE_GEOMETRY && module->wrapper != NULL)
    {
        for (member = module->wrapper->parameters; member != NULL;
             member = member->next)
        {
            const char *canonical;
            const char *classification;
            const char *interpolation;

            if (member->canonicalSemantic == NULL)
                continue;
            publicName = member->publicName != NULL &&
                         member->publicName[0] != '\0' ?
                         member->publicName : member->name;
            typeName = HlslTypeName(&member->type);
            canonical = member->canonicalSemantic;
            classification = member->semanticKind ==
                             HLSL_SEMANTIC_USER ? "user" : "system";
            interpolation = HlslInterpolationName(member->interpolation);
            if (publicName == NULL || typeName == NULL ||
                member->semantic == NULL || interpolation == NULL ||
                fprintf(out,
                    "// cgc-bind interface in %s %s %s %s %s %s\n",
                    publicName, typeName, member->semantic, canonical,
                    classification, interpolation) < 0)
            {
                return -1;
            }
            wroteRecord = 1;
        }
    }
    return wroteRecord;
} // HlslWriteInterfaceMetadata

static int HlslWriteBindingDefault(FILE *out,
                                   const HlslBinding *binding)
{
    int i;

    if (binding->defaultCount <= 0)
        return 1;
    if (fprintf(out, "// cgc-default %s", binding->publicName) < 0)
        return 0;
    for (i = 0; i < binding->defaultCount; i++) {
        if (fputc(' ', out) == EOF ||
            (binding->defaultLiterals != NULL ?
             !HlslWriteDefaultLiteral(out, &binding->defaultLiterals[i]) :
             !HlslWriteFloat(out, binding->defaultValues[i])))
        {
            return 0;
        }
    }
    return fputc('\n', out) != EOF;
} // HlslWriteBindingDefault

static int HlslWriteBindingMetadata(FILE *out,
                                    const HlslBinding *binding)
{
    if (fprintf(out, "// var %s %s : %s : %s[%d] : 1 : %d\n",
            HlslTypeName(&binding->type), binding->publicName,
            binding->semantic != NULL ? binding->semantic : "",
            HlslBankText(binding->physical.bank),
            binding->physical.regno, binding->physical.span) < 0)
    {
        return 0;
    }
    return HlslWriteBindingDefault(out, binding);
} // HlslWriteBindingMetadata

static const HlslResource *HlslBindingCbuffer(const HlslModule *module,
                                              const HlslBinding *binding)
{
    const HlslResource *resource;
    const HlslDecl *member;

    for (resource = module->resources; resource != NULL;
         resource = resource->next)
    {
        if (resource->kind != HLSL_RESOURCE_CBUFFER)
            continue;
        for (member = resource->members; member != NULL;
             member = member->next)
        {
            if (binding->declaration == member)
                return resource;
        }
    }
    return NULL;
} // HlslBindingCbuffer

/* Stable modern runtime contract:
 * // cgc-bind uniform NAME TYPE bN cV.C byte=B size=S span=V semantic=SEM
 * byte and size are physical bytes; span is the full 16-byte-vector span.
 */
static int HlslWriteModernBindingRecord(FILE *out,
                                        const HlslModule *module,
                                        const HlslBinding *binding)
{
    static const char components[] = "xyzw";
    const HlslBinding *leaf;
    const HlslDecl *declaration;
    const HlslResource *resource;
    int byteOffset;
    int byteSize;

    leaf = binding->leafBindings;
    declaration = leaf != NULL ? leaf->declaration : NULL;
    resource = leaf != NULL ? HlslBindingCbuffer(module, leaf) : NULL;
    if (binding->publicName == NULL || binding->logicalTypeName == NULL ||
        declaration == NULL || resource == NULL ||
        resource->binding.slot < 0 || !declaration->hasPackOffset ||
        declaration->packOffset.vector < 0 ||
        declaration->packOffset.vector > (INT_MAX - 12) / 16 ||
        declaration->packOffset.component < 0 ||
        declaration->packOffset.component > 3 ||
        declaration->packOffset.componentCount <= 0 ||
        declaration->packOffset.componentCount > INT_MAX / 4 ||
        leaf->physical.span <= 0)
    {
        return 0;
    }
    byteOffset = declaration->packOffset.vector * 16 +
                 declaration->packOffset.component * 4;
    if (binding->type.arraySize > 0 ||
        binding->type.base == HLSL_BASE_STRUCT ||
        binding->type.rows > 0 || binding->type.cols > 0)
    {
        if (binding->physical.span <= 0 ||
            binding->physical.span > INT_MAX / 16)
        {
            return 0;
        }
        byteSize = binding->physical.span * 16;
    } else {
        byteSize = declaration->packOffset.componentCount * 4;
    }
    if (fprintf(out,
            "// cgc-bind uniform %s %s b%d c%d.%c byte=%d size=%d "
            "span=%d semantic=%s\n",
            binding->publicName, binding->logicalTypeName,
            resource->binding.slot, declaration->packOffset.vector,
            components[declaration->packOffset.component], byteOffset,
            byteSize, binding->physical.span,
            binding->semantic != NULL && binding->semantic[0] != '\0' ?
                binding->semantic : "-") < 0)
    {
        return 0;
    }
    return HlslWriteBindingDefault(out, binding);
} // HlslWriteModernBindingRecord

static int HlslWriteModernSamplerRecord(FILE *out,
                                        const HlslModule *module,
                                        const HlslBinding *binding)
{
    const HlslResource *resource;
    int textureSlot;
    int samplerSlot;

    textureSlot = -1;
    samplerSlot = -1;
    for (resource = module->resources; resource != NULL;
         resource = resource->next)
    {
        if (resource->sourceDeclaration == binding->declaration &&
            resource->kind == HLSL_RESOURCE_TEXTURE)
        {
            textureSlot = resource->binding.slot;
        }
        if (binding->declaration != NULL &&
            resource->sourceDeclaration ==
                binding->declaration->resourcePair &&
            resource->kind == HLSL_RESOURCE_SAMPLER)
        {
            samplerSlot = resource->binding.slot;
        }
    }
    if (binding->publicName == NULL || binding->logicalTypeName == NULL ||
        textureSlot < 0 || samplerSlot < 0 || textureSlot != samplerSlot)
    {
        return 0;
    }
    return fprintf(out, "// cgc-bind sampler %s %s t%d s%d\n",
                   binding->publicName, binding->logicalTypeName,
                   textureSlot, samplerSlot) >= 0;
} // HlslWriteModernSamplerRecord

static int HlslWriteModernBindingMetadata(FILE *out,
                                          const HlslModule *module)
{
    const HlslBinding *binding;
    const HlslBinding *next;
    int lastOrdinal;
    int wrote;

    lastOrdinal = -1;
    wrote = 0;
    for (;;) {
        next = NULL;
        for (binding = module->bindings; binding != NULL;
             binding = binding->next)
        {
            if ((binding->storage != HLSL_STORAGE_UNIFORM &&
                 binding->storage != HLSL_STORAGE_SAMPLER) ||
                binding->sourceOrdinal <= lastOrdinal ||
                (next != NULL &&
                 binding->sourceOrdinal >= next->sourceOrdinal))
            {
                continue;
            }
            next = binding;
        }
        if (next == NULL)
            break;
        if (!(next->storage == HLSL_STORAGE_SAMPLER ?
              HlslWriteModernSamplerRecord(out, module, next) :
              HlslWriteModernBindingRecord(out, module, next)))
            return -1;
        lastOrdinal = next->sourceOrdinal;
        wrote = 1;
    }
    return wrote;
} // HlslWriteModernBindingMetadata

static int HlslPackOffsetUsesComponent(const HlslType *type)
{
    return type != NULL && type->arraySize == 0 &&
           type->base != HLSL_BASE_STRUCT && type->rows == 0 &&
           type->cols == 0;
} // HlslPackOffsetUsesComponent

static int HlslWriteCbuffer(FILE *out, const HlslResource *resource)
{
    static const char components[] = "xyzw";
    const HlslDecl *member;

    if (fprintf(out, "cbuffer %s : register(b%d)\n{\n",
                resource->name, resource->binding.slot) < 0)
    {
        return 0;
    }
    for (member = resource->members; member != NULL;
         member = member->next)
    {
        if (!HlslWriteIndent(out, 1) ||
            !HlslWriteTypeAndName(out, &member->type, member->name) ||
            fprintf(out, " : packoffset(c%d",
                    member->packOffset.vector) < 0)
        {
            return 0;
        }
        if (HlslPackOffsetUsesComponent(&member->type) &&
            fprintf(out, ".%c", components[member->packOffset.component]) < 0)
        {
            return 0;
        }
        if (fputs(");\n", out) == EOF)
            return 0;
    }
    return fputs("};\n", out) != EOF;
} // HlslWriteCbuffer

static int HlslWriteModernResource(FILE *out,
                                   const HlslResource *resource)
{
    const char *typeName;

    typeName = HlslTypeName(&resource->type);
    if (typeName == NULL)
        return 0;
    if (resource->kind == HLSL_RESOURCE_TEXTURE) {
        return fprintf(out, "%s<float4> %s : register(t%d);\n",
                       typeName, resource->name,
                       resource->binding.slot) >= 0;
    }
    if (resource->kind == HLSL_RESOURCE_SAMPLER) {
        return fprintf(out, "%s %s : register(s%d);\n",
                       typeName, resource->name,
                       resource->binding.slot) >= 0;
    }
    return 0;
} // HlslWriteModernResource

static int HlslEmitModule(FILE *out, const HlslModule *module,
                          const HlslProfileDesc *profile)
{
    const HlslBinding *binding;
    const HlslDecl *decl;
    const HlslFunction *function;
    const HlslResource *resource;
    int modern;
    int wroteInterface;
    int wroteSection;

    if (fprintf(out, "// profile %s\n", profile->name) < 0 ||
        fprintf(out, "// target %s\n", profile->target) < 0)
    {
        return 0;
    }
    wroteInterface = HlslWriteInterfaceMetadata(out, module, profile);
    if (wroteInterface < 0 ||
        (wroteInterface && fputc('\n', out) == EOF))
    {
        return 0;
    }
    modern = profile->resourcePolicy == HLSL_RESOURCE_POLICY_MODERN;
    if (modern) {
        wroteSection = HlslWriteModernBindingMetadata(out, module);
        if (wroteSection < 0)
            return 0;
    } else {
        for (binding = module->allocatedBindings; binding != NULL;
             binding = binding->allocationNext)
        {
            if (!HlslWriteBindingMetadata(out, binding))
                return 0;
        }
        wroteSection = module->allocatedBindings != NULL;
    }
    if (wroteSection && fputc('\n', out) == EOF)
        return 0;
    if (modern) {
        for (resource = module->resources; resource != NULL;
             resource = resource->next)
        {
            if (resource->kind == HLSL_RESOURCE_CBUFFER &&
                (!HlslWriteCbuffer(out, resource) ||
                 fputc('\n', out) == EOF))
            {
                return 0;
            }
        }
        for (resource = module->resources; resource != NULL;
             resource = resource->next)
        {
            if (resource->kind != HLSL_RESOURCE_CBUFFER &&
                !HlslWriteModernResource(out, resource))
            {
                return 0;
            }
        }
        for (resource = module->resources; resource != NULL;
             resource = resource->next)
        {
            if (resource->kind != HLSL_RESOURCE_CBUFFER)
                break;
        }
        if (resource != NULL && fputc('\n', out) == EOF)
            return 0;
        for (decl = module->structs; decl != NULL; decl = decl->next) {
            if (!HlslWriteStruct(out, decl) || fputc('\n', out) == EOF)
                return 0;
        }
    }
    for (decl = module->globals; decl != NULL; decl = decl->next) {
        if (!HlslWriteDecl(out, decl, 0, 0))
            return 0;
    }
    if (module->globals != NULL && fputc('\n', out) == EOF)
        return 0;
    if (!modern) {
        for (decl = module->structs; decl != NULL; decl = decl->next) {
            if (!HlslWriteStruct(out, decl) || fputc('\n', out) == EOF)
                return 0;
        }
    }
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function->needsPrototype &&
            (!HlslWriteFunctionHeader(out, function) ||
             fputs(";\n\n", out) == EOF))
        {
            return 0;
        }
    }
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (!HlslWriteFunction(out, function))
            return 0;
        if (function->next != NULL && fputc('\n', out) == EOF)
            return 0;
    }
    return 1;
} // HlslEmitModule

int HlslWriteModule(FILE *out, HlslModule *module,
                    const HlslProfileDesc *profile)
{
    FILE *temporary;
    char buffer[4096];
    size_t count;

    if (out == NULL || !HlslValidateModule(module, profile) ||
        !HlslCanWriteModule(module, profile))
        return 0;
    temporary = tmpfile();
    if (temporary == NULL)
        return 0;
    if (!HlslEmitModule(temporary, module, profile) ||
        fflush(temporary) != 0 || fseek(temporary, 0, SEEK_SET) != 0)
    {
        fclose(temporary);
        return 0;
    }
    while ((count = fread(buffer, 1, sizeof(buffer), temporary)) != 0) {
        if (fwrite(buffer, 1, count, out) != count) {
            fclose(temporary);
            return 0;
        }
    }
    if (ferror(temporary)) {
        fclose(temporary);
        return 0;
    }
    return fclose(temporary) == 0;
} // HlslWriteModule
