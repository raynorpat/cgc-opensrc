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
#include <string.h>

#include "slglobals.h"
#include "hlsl_hal.h"

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
         module->stage != HLSL_STAGE_PIXEL) ||
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
    if (type->arraySize > 0 && fprintf(out, "[%d]", type->arraySize) < 0)
        return 0;
    return 1;
} // HlslWriteTypeAndName

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

static int HlslExprPrecedence(const HlslExpr *expression)
{
    if (expression == NULL)
        return 0;
    if (expression->kind != HLSL_EXPR_BINARY)
        return 100;
    switch (expression->u.binary.op) {
    case HLSL_OP_ASSIGN: return 10;
    case HLSL_OP_MULTIPLY: return 70;
    default: return 20;
    }
} // HlslExprPrecedence

static const char *HlslOperatorText(HlslOperator op)
{
    switch (op) {
    case HLSL_OP_ASSIGN: return "=";
    case HLSL_OP_MULTIPLY: return "*";
    default: return NULL;
    }
} // HlslOperatorText

static int HlslWriteExpr(FILE *out, const HlslExpr *expression,
                         int parentPrecedence)
{
    const HlslExpr *argument;
    const char *text;
    int precedence;
    int parentheses;
    int first;

    if (expression == NULL)
        return 0;
    precedence = HlslExprPrecedence(expression);
    parentheses = precedence < parentPrecedence;
    if (parentheses && fputc('(', out) == EOF)
        return 0;
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
    case HLSL_EXPR_BINARY:
        text = HlslOperatorText(expression->u.binary.op);
        if (text == NULL ||
            !HlslWriteExpr(out, expression->u.binary.left, precedence) ||
            fprintf(out, " %s ", text) < 0 ||
            !HlslWriteExpr(out, expression->u.binary.right,
                           precedence + (expression->u.binary.op ==
                                         HLSL_OP_ASSIGN ? 0 : 1)))
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
        for (argument = expression->u.call.arguments; argument != NULL;
             argument = argument->next)
        {
            if ((!first && fputs(", ", out) == EOF) ||
                !HlslWriteExpr(out, argument, 0))
            {
                return 0;
            }
            first = 0;
        }
        if (fputc(')', out) == EOF)
            return 0;
        break;
    case HLSL_EXPR_CONSTRUCT:
        text = HlslTypeName(&expression->type);
        if (text == NULL || fprintf(out, "%s(", text) < 0)
            return 0;
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
        if (fputc(')', out) == EOF)
            return 0;
        break;
    case HLSL_EXPR_MEMBER:
        if (!HlslWriteExpr(out, expression->u.member.object, 100) ||
            fprintf(out, ".%s", expression->u.member.name) < 0)
        {
            return 0;
        }
        break;
    case HLSL_EXPR_INDEX:
        if (!HlslWriteExpr(out, expression->u.index.object, 100) ||
            fputc('[', out) == EOF ||
            !HlslWriteExpr(out, expression->u.index.index, 0) ||
            fputc(']', out) == EOF)
        {
            return 0;
        }
        break;
    default:
        return 0;
    }
    if (parentheses && fputc(')', out) == EOF)
        return 0;
    return 1;
} // HlslWriteExpr

static int HlslWriteDecl(FILE *out, const HlslDecl *decl, int indent,
                         int withSemantic)
{
    if (!HlslWriteIndent(out, indent))
        return 0;
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
    if (!HlslWriteTypeAndName(out, &decl->type, decl->name))
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
    if (decl->initializer != NULL &&
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
        if (!HlslWriteTypeAndName(out, &parameter->type, parameter->name))
            return 0;
        first = 0;
    }
    return 1;
} // HlslWriteParameters

static int HlslWriteStatements(FILE *out, const HlslStmt *statement,
                               int indent)
{
    for (; statement != NULL; statement = statement->next) {
        if (!HlslWriteIndent(out, indent))
            return 0;
        switch (statement->kind) {
        case HLSL_STMT_EXPRESSION:
            if (!HlslWriteExpr(out, statement->u.expression, 0) ||
                fputs(";\n", out) == EOF)
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
    case HLSL_REGISTER_NONE: break;
    }
    return "?";
} // HlslBankText

static int HlslEmitModule(FILE *out, const HlslModule *module,
                          const HlslProfileDesc *profile)
{
    const HlslBinding *binding;
    const HlslDecl *decl;
    const HlslFunction *function;
    int wroteSection;

    if (fprintf(out, "// profile %s\n", profile->name) < 0 ||
        fprintf(out, "// target %s\n", profile->target) < 0)
    {
        return 0;
    }
    for (binding = module->allocatedBindings; binding != NULL;
         binding = binding->allocationNext)
    {
        if (fprintf(out, "// var %s %s : %s : %s[%d] : 1 : %d\n",
                HlslTypeName(&binding->type), binding->publicName,
                binding->semantic != NULL ? binding->semantic : "",
                HlslBankText(binding->physical.bank),
                binding->physical.regno, binding->physical.span) < 0)
        {
            return 0;
        }
    }
    wroteSection = module->allocatedBindings != NULL;
    if (wroteSection && fputc('\n', out) == EOF)
        return 0;
    for (decl = module->globals; decl != NULL; decl = decl->next) {
        if (!HlslWriteDecl(out, decl, 0, 0))
            return 0;
    }
    if (module->globals != NULL && fputc('\n', out) == EOF)
        return 0;
    for (decl = module->structs; decl != NULL; decl = decl->next) {
        if (!HlslWriteStruct(out, decl) || fputc('\n', out) == EOF)
            return 0;
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

int HlslWriteModule(FILE *out, const HlslModule *module,
                    const HlslProfileDesc *profile)
{
    FILE *temporary;
    char buffer[4096];
    size_t count;

    if (out == NULL || !HlslCanWriteModule(module, profile))
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
