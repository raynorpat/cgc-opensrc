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
// glsl_lower.c
//

#include <stdio.h>
#include <string.h>
#include <float.h>

#include "slglobals.h"
#include "glsl_hal.h"

typedef struct GlslLowerContext_Rec {
    GlslModule *module;
    const GlslProfileDesc *profile;
    Scope *scope;
    GlslFunction *function;
    SourceLoc statementLoc;
    int loopDepth;
} GlslLowerContext;

static void GlslSetLoc(GlslLoc *target, const SourceLoc *source)
{
    if (source != NULL) {
        target->file = source->file;
        target->line = source->line;
    }
}

static int GlslLowerError(GlslLowerContext *context)
{
    context->module->errors++;
    return 0;
}

static void GlslRecordFailure(GlslLowerContext *context, const char *reason)
{
    if (context->module->errorReason != NULL)
        return;
    context->module->errorLoc.file = context->statementLoc.file;
    context->module->errorLoc.line = context->statementLoc.line;
    context->module->errorReason = reason;
}

static const char *GlslUnsupportedExprReason(const expr *source)
{
    if (source != NULL && source->common.kind == BINARY_N) {
        switch (source->bin.op) {
        case MOD_OP: case MOD_V_OP: case MOD_SV_OP: case MOD_VS_OP:
            return "remainder (%)";
        case SHL_OP: case SHL_V_OP: case SHR_OP: case SHR_V_OP:
            return "shift operator";
        case AND_OP: case AND_V_OP: case AND_SV_OP: case AND_VS_OP:
        case XOR_OP: case XOR_V_OP: case XOR_SV_OP: case XOR_VS_OP:
        case OR_OP: case OR_V_OP: case OR_SV_OP: case OR_VS_OP:
            return "bitwise operator";
        default:
            break;
        }
    }
    return "GLSL 1.10 expression";
}

static char *GlslCopyText(GlslModule *module, const char *text)
{
    char *copy;
    size_t size;

    size = strlen(text) + 1;
    copy = (char *) module->alloc(module->allocArg, size);
    if (copy != NULL)
        memcpy(copy, text, size);
    return copy;
}

static int GlslTypesEqual(const GlslType *left, const GlslType *right)
{
    const char *leftName;
    const char *rightName;

    leftName = GlslTypeName(left);
    rightName = GlslTypeName(right);
    return leftName != NULL && rightName != NULL &&
           !strcmp(leftName, rightName);
}

static GlslDecl *GlslFindDeclList(GlslDecl *list, const void *identity)
{
    GlslDecl *decl;

    for (decl = list; decl != NULL; decl = decl->next) {
        if (decl->identity == identity)
            return decl;
    }
    return NULL;
}

static GlslDecl *GlslFindDecl(GlslLowerContext *context,
                              const void *identity)
{
    GlslDecl *decl;
    GlslBinding *binding;

    if (context->function != NULL) {
        decl = GlslFindDeclList(context->function->parameters, identity);
        if (decl == NULL)
            decl = GlslFindDeclList(context->function->locals, identity);
        if (decl != NULL)
            return decl;
    }
    decl = GlslFindDeclList(context->module->globals, identity);
    if (decl != NULL)
        return decl;
    for (decl = context->module->structs; decl != NULL; decl = decl->next) {
        GlslDecl *member;

        member = GlslFindDeclList(decl->members, identity);
        if (member != NULL)
            return member;
    }
    for (binding = context->module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (binding->declaration != NULL &&
            binding->declaration->identity == identity)
        {
            return binding->declaration;
        }
    }
    return NULL;
}

static GlslFunction *GlslFindFunction(GlslModule *module,
                                      const void *identity)
{
    GlslFunction *function;

    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function->identity == identity)
            return function;
    }
    return NULL;
}

static Symbol *GlslFindTag(Scope *scope, Type *type)
{
    Symbol *tag;

    tag = LookUpTag(scope, type->str.tag);
    if (tag != NULL && tag->type == type)
        return tag;
    return NULL;
}

static GlslDecl *GlslFindStruct(GlslLowerContext *context, Type *type)
{
    GlslDecl *decl;
    Symbol *tag;

    tag = GlslFindTag(context->scope, type);
    if (tag == NULL)
        return NULL;
    for (decl = context->module->structs; decl != NULL; decl = decl->next) {
        if (decl->identity == tag)
            return decl;
    }
    return NULL;
}

static int GlslLowerType(GlslLowerContext *context, Type *source,
                         GlslType *target)
{
    GlslDecl *structDecl;
    GlslBase glslBase;
    int base;
    int category;
    int len;

    if (source == NULL || target == NULL)
        return 0;
    if (IsVoid(source)) {
        *target = GlslNumericType(GLSL_BASE_VOID, 0);
        return 1;
    }
    base = GetBase(source);
    switch (base) {
    case TYPE_BASE_FLOAT:
    case TYPE_BASE_CFLOAT:
        glslBase = GLSL_BASE_FLOAT;
        break;
    case TYPE_BASE_INT:
    case TYPE_BASE_CINT:
        glslBase = GLSL_BASE_INT;
        break;
    case TYPE_BASE_BOOLEAN:
        glslBase = GLSL_BASE_BOOL;
        break;
    default:
        glslBase = GLSL_BASE_VOID;
        break;
    }
    category = GetCategory(source);
    if (category == TYPE_CATEGORY_SCALAR && glslBase != GLSL_BASE_VOID) {
        *target = GlslNumericType(glslBase, 1);
        return 1;
    }
    if (category == TYPE_CATEGORY_ARRAY && IsVector(source, &len) &&
        glslBase != GLSL_BASE_VOID && len >= 1 && len <= 4)
    {
        *target = GlslNumericType(glslBase, len);
        return 1;
    }
    if (category == TYPE_CATEGORY_STRUCT) {
        structDecl = GlslFindStruct(context, source);
        if (structDecl == NULL)
            return 0;
        *target = GlslNumericType(GLSL_BASE_STRUCT, 0);
        target->structName = structDecl->name;
        return 1;
    }
    return 0;
}

static int GlslDeclComesBefore(const GlslDecl *left, const GlslDecl *right)
{
    if (left->loc.file != right->loc.file)
        return left->loc.file < right->loc.file;
    if (left->loc.line != right->loc.line)
        return left->loc.line < right->loc.line;
    return strcmp(left->name, right->name) < 0;
}

static void GlslInsertDecl(GlslDecl **list, GlslDecl *decl)
{
    GlslDecl **place;

    place = list;
    while (*place != NULL && !GlslDeclComesBefore(decl, *place))
        place = &(*place)->next;
    decl->next = *place;
    *place = decl;
}

static GlslDecl *GlslNewSourceDecl(GlslLowerContext *context,
    Symbol *symbol, const void *nameSpace)
{
    GlslDecl *decl;
    GlslType type;
    const char *sourceName;
    const char *name;

    if (!GlslLowerType(context, symbol->type, &type))
        return NULL;
    sourceName = GetAtomString(atable, symbol->name);
    if (nameSpace != NULL) {
        name = GlslAllocateScopedSymbolName(context->module, nameSpace,
                                            symbol, sourceName);
    } else {
        name = GlslAllocateSymbolName(context->module, symbol, sourceName);
    }
    if (name == NULL)
        return NULL;
    decl = GlslNewDecl(context->module, GLSL_STORAGE_NONE, type, name);
    if (decl != NULL) {
        decl->identity = symbol;
        GlslSetLoc(&decl->loc, &symbol->loc);
    }
    return decl;
}

static int GlslCollectMembers(GlslLowerContext *context, Scope *memberScope,
    Symbol *symbol, GlslDecl **members)
{
    GlslDecl *decl;

    if (symbol == NULL)
        return 1;
    if (!GlslCollectMembers(context, memberScope, symbol->left, members))
        return 0;
    if (symbol->kind == VARIABLE_S) {
        decl = GlslNewSourceDecl(context, symbol, memberScope);
        if (decl == NULL)
            return 0;
        GlslInsertDecl(members, decl);
    }
    return GlslCollectMembers(context, memberScope, symbol->right, members);
}

static int GlslEnsureType(GlslLowerContext *context, Type *type);

static int GlslEnsureSymbolTypes(GlslLowerContext *context, Symbol *symbol)
{
    const char *name;

    if (symbol == NULL)
        return 1;
    if (!GlslEnsureSymbolTypes(context, symbol->left))
        return 0;
    if (symbol->kind == VARIABLE_S) {
        name = GetAtomString(atable, symbol->name);
        if (name != NULL && name[0] != '$' &&
            !GlslEnsureType(context, symbol->type)) return 0;
    }
    return GlslEnsureSymbolTypes(context, symbol->right);
}

static int GlslEnsureParameterTypes(GlslLowerContext *context,
                                    Symbol *formal)
{
    for (; formal != NULL; formal = formal->next) {
        if (!GlslEnsureType(context, formal->type))
            return 0;
    }
    return 1;
}

static int GlslEnsureType(GlslLowerContext *context, Type *type)
{
    GlslDecl *decl;
    GlslType structType;
    Symbol *tag;
    const char *sourceName;
    const char *name;

    if (type == NULL)
        return 0;
    if (GetCategory(type) != TYPE_CATEGORY_STRUCT)
        return 1;
    if (GlslFindStruct(context, type) != NULL)
        return 1;
    tag = GlslFindTag(context->scope, type);
    if (tag == NULL)
        return 0;
    sourceName = GetAtomString(atable, type->str.tag);
    name = GlslAllocateSymbolName(context->module, tag, sourceName);
    if (name == NULL)
        return 0;
    structType = GlslNumericType(GLSL_BASE_STRUCT, 0);
    structType.structName = name;
    decl = GlslNewDecl(context->module, GLSL_STORAGE_NONE,
                       structType, name);
    if (decl == NULL)
        return 0;
    decl->identity = tag;
    GlslSetLoc(&decl->loc, &type->str.loc);
    GlslAppendDecl(&context->module->structs, decl);
    if (type->str.members == NULL ||
        !GlslEnsureSymbolTypes(context, type->str.members->symbols))
    {
        return 0;
    }
    return GlslCollectMembers(context, type->str.members,
        type->str.members->symbols, &decl->members);
}

static int GlslStructReady(const GlslDecl *decl,
                           const GlslDecl *remaining)
{
    const GlslDecl *member;
    const GlslDecl *other;

    for (member = decl->members; member != NULL; member = member->next) {
        if (member->type.base != GLSL_BASE_STRUCT)
            continue;
        for (other = remaining; other != NULL; other = other->next) {
            if (other != decl &&
                !strcmp(member->type.structName, other->name)) return 0;
        }
    }
    return 1;
}

static int GlslSortStructs(GlslLowerContext *context)
{
    GlslDecl *remaining;
    GlslDecl *ordered;
    GlslDecl **tail;
    GlslDecl **place;
    GlslDecl **best;
    GlslDecl *decl;

    remaining = context->module->structs;
    ordered = NULL;
    tail = &ordered;
    while (remaining != NULL) {
        best = NULL;
        for (place = &remaining; *place != NULL; place = &(*place)->next) {
            if (GlslStructReady(*place, remaining) &&
                (best == NULL || GlslDeclComesBefore(*place, *best)))
            {
                best = place;
            }
        }
        if (best == NULL) {
            GlslRecordFailure(context, "recursive GLSL structure");
            return 0;
        }
        decl = *best;
        *best = decl->next;
        decl->next = NULL;
        *tail = decl;
        tail = &decl->next;
    }
    context->module->structs = ordered;
    return 1;
}

static int GlslCollectParameters(GlslLowerContext *context, Symbol *formal,
                                 int entry)
{
    GlslDecl *decl;
    int qualifiers;

    for (; formal != NULL; formal = formal->next) {
        if (GetDomain(formal->type) == TYPE_DOMAIN_UNIFORM)
            return 0;
        decl = GlslNewSourceDecl(context, formal,
                                 entry ? NULL : context->function->identity);
        if (decl == NULL)
            return 0;
        if (entry) {
            GlslAppendDecl(&context->function->locals, decl);
        } else {
            qualifiers = GetQualifiers(formal->type);
            if ((qualifiers & TYPE_QUALIFIER_INOUT) ==
                TYPE_QUALIFIER_INOUT)
            {
                decl->parameterQualifier = GLSL_PARAMETER_INOUT;
            } else if (qualifiers & TYPE_QUALIFIER_OUT) {
                decl->parameterQualifier = GLSL_PARAMETER_OUT;
            }
            GlslAppendDecl(&context->function->parameters, decl);
        }
    }
    return 1;
}

static int GlslCollectLocals(GlslLowerContext *context, Symbol *symbol,
                             int entry)
{
    const char *name;
    const void *nameSpace;
    GlslDecl *decl;

    if (symbol == NULL)
        return 1;
    if (!GlslCollectLocals(context, symbol->left, entry))
        return 0;
    if (symbol->kind == VARIABLE_S && GlslFindDecl(context, symbol) == NULL) {
        name = GetAtomString(atable, symbol->name);
        if (name == NULL || name[0] == '$')
            return 0;
        nameSpace = entry ? NULL : context->function->identity;
        decl = GlslNewSourceDecl(context, symbol, nameSpace);
        if (decl == NULL)
            return 0;
        GlslInsertDecl(&context->function->locals, decl);
    }
    return GlslCollectLocals(context, symbol->right, entry);
}

static int GlslBindingComesBefore(const GlslBinding *left,
                                  const GlslBinding *right)
{
    if (left->storage != right->storage)
        return left->storage < right->storage;
    if (left->loc.file != right->loc.file)
        return left->loc.file < right->loc.file;
    if (left->loc.line != right->loc.line)
        return left->loc.line < right->loc.line;
    return strcmp(left->name, right->name) < 0;
}

static void GlslInsertBinding(GlslBinding **list, GlslBinding *binding)
{
    GlslBinding **place;

    place = list;
    while (*place != NULL && !GlslBindingComesBefore(binding, *place))
        place = &(*place)->next;
    binding->next = *place;
    *place = binding;
}

static GlslDecl *GlslLowerInterface(GlslLowerContext *context,
                                    Symbol *member)
{
    Binding *sourceBinding;
    GlslBinding *binding;
    GlslDecl *decl;
    GlslType type;
    GlslStorage storage;
    const char *canonical;
    const char *interfaceName;
    const char *name;
    char generatedName[256];
    int isOutput;

    decl = GlslFindDecl(context, member);
    if (decl != NULL)
        return decl;
    sourceBinding = member->details.var.bind;
    if (sourceBinding == NULL || sourceBinding->none.kind != BK_CONNECTOR ||
        !(sourceBinding->none.properties & BIND_IS_BOUND) ||
        sourceBinding->conn.rname == 0)
    {
        return NULL;
    }
    isOutput = (sourceBinding->none.properties & BIND_OUTPUT) != 0;
    if (isOutput ==
        ((sourceBinding->none.properties & BIND_INPUT) != 0)) return NULL;
    canonical = GetAtomString(atable, sourceBinding->conn.rname);
    interfaceName = GlslCanonicalInterfaceName(context->profile,
        sourceBinding->conn.rname, isOutput);
    if (canonical == NULL || interfaceName == NULL ||
        !GlslLowerType(context, member->type, &type)) return NULL;
    if (!strncmp(interfaceName, "gl_", 3)) {
        storage = GLSL_STORAGE_BUILTIN;
        name = interfaceName;
    } else {
        if (strlen(interfaceName) + 4 > sizeof(generatedName))
            return NULL;
        sprintf(generatedName, "cg_%s", interfaceName);
        name = GlslAllocateSymbolName(context->module, member,
                                      generatedName);
        if (name == NULL)
            return NULL;
        if (!isOutput && context->profile->stage == GLSL_STAGE_VERTEX)
            storage = GLSL_STORAGE_ATTRIBUTE;
        else
            storage = GLSL_STORAGE_VARYING;
    }
    decl = GlslNewDecl(context->module, storage, type, name);
    binding = GlslNewBinding(context->module, storage, name, canonical);
    if (decl == NULL || binding == NULL)
        return NULL;
    decl->identity = member;
    GlslSetLoc(&decl->loc, &member->loc);
    binding->declaration = decl;
    GlslSetLoc(&binding->loc, &member->loc);
    if (storage != GLSL_STORAGE_BUILTIN)
        GlslAppendDecl(&context->module->globals, decl);
    GlslInsertBinding(&context->module->bindings, binding);
    return decl;
}

static int GlslFunctionComesBefore(const GlslFunction *left,
                                   const GlslFunction *right)
{
    const Symbol *leftSymbol;
    const Symbol *rightSymbol;
    const char *leftName;
    const char *rightName;

    if (left->loc.file != right->loc.file)
        return left->loc.file < right->loc.file;
    if (left->loc.line != right->loc.line)
        return left->loc.line < right->loc.line;
    leftSymbol = (const Symbol *) left->identity;
    rightSymbol = (const Symbol *) right->identity;
    leftName = GetAtomString(atable, leftSymbol->name);
    rightName = GetAtomString(atable, rightSymbol->name);
    return strcmp(leftName, rightName) < 0;
}

static void GlslInsertFunction(GlslFunction **list, GlslFunction *function)
{
    GlslFunction **place;

    place = list;
    while (*place != NULL && !GlslFunctionComesBefore(function, *place))
        place = &(*place)->next;
    function->next = *place;
    *place = function;
}

static int GlslCollectCallsInExpr(GlslLowerContext *context, expr *source);

static int GlslCollectCallsInStatements(GlslLowerContext *context,
                                        stmt *source)
{
    for (; source != NULL; source = source->commonst.next) {
        context->statementLoc = source->commonst.loc;
        switch (source->commonst.kind) {
        case EXPR_STMT:
            if (!GlslCollectCallsInExpr(context, source->exprst.exp)) return 0;
            break;
        case IF_STMT:
            if (!GlslCollectCallsInExpr(context, source->ifst.cond) ||
                !GlslCollectCallsInStatements(context, source->ifst.thenstmt) ||
                !GlslCollectCallsInStatements(context, source->ifst.elsestmt)) return 0;
            break;
        case WHILE_STMT:
        case DO_STMT:
            if (!GlslCollectCallsInExpr(context, source->whilest.cond) ||
                !GlslCollectCallsInStatements(context, source->whilest.body)) return 0;
            break;
        case FOR_STMT:
            if (!GlslCollectCallsInStatements(context, source->forst.init) ||
                !GlslCollectCallsInExpr(context, source->forst.cond) ||
                !GlslCollectCallsInStatements(context, source->forst.step) ||
                !GlslCollectCallsInStatements(context, source->forst.body)) return 0;
            break;
        case BLOCK_STMT:
            if (!GlslCollectCallsInStatements(context, source->blockst.body)) return 0;
            break;
        case RETURN_STMT:
            if (!GlslCollectCallsInExpr(context, source->returnst.exp)) return 0;
            break;
        case DISCARD_STMT:
            if (!GlslCollectCallsInExpr(context, source->discardst.cond)) return 0;
            break;
        case COMMENT_STMT:
        case BREAK_STMT:
        case CONTINUE_STMT:
            break;
        default:
            return 0;
        }
    }
    return 1;
}

static int GlslCollectHelper(GlslLowerContext *context, Symbol *symbol)
{
    GlslFunction *function;
    GlslType result;
    const char *fileName;

    if (symbol == NULL || symbol->kind != FUNCTION_S)
        return 0;
    if (symbol->properties & SYMB_IS_BUILTIN)
        return 1;
    fileName = GetAtomString(atable, symbol->loc.file);
    if (fileName != NULL && !strcmp(fileName, "<stdlib>")) {
        GlslRecordFailure(context, "GLSL standard-library helper");
        return 0;
    }
    function = GlslFindFunction(context->module, symbol);
    if (function != NULL) {
        if (function->visitState == 1) {
            GlslRecordFailure(context, "recursive GLSL helper");
            return 0;
        }
        return 1;
    }
    if (!GlslEnsureType(context, symbol->type->fun.rettype) ||
        !GlslEnsureParameterTypes(context, symbol->details.fun.params) ||
        symbol->details.fun.locals == NULL ||
        !GlslEnsureSymbolTypes(context,
                               symbol->details.fun.locals->symbols) ||
        !GlslLowerType(context, symbol->type->fun.rettype, &result))
    {
        GlslRecordFailure(context, "GLSL helper type");
        return 0;
    }
    function = GlslNewFunction(context->module, result, NULL);
    if (function == NULL)
        return 0;
    function->identity = symbol;
    function->visitState = 1;
    if (symbol->details.fun.statements != NULL) {
        GlslSetLoc(&function->loc,
                   &symbol->details.fun.statements->commonst.loc);
    } else {
        GlslSetLoc(&function->loc, &symbol->loc);
    }
    GlslInsertFunction(&context->module->functions, function);
    if (!GlslCollectCallsInStatements(context,
                                      symbol->details.fun.statements))
    {
        return 0;
    }
    function->visitState = 2;
    return 1;
}

static int GlslCollectCallsInExpr(GlslLowerContext *context, expr *source)
{
    Symbol *symbol;

    if (source == NULL)
        return 1;
    switch (source->common.kind) {
    case DECL_N:
    case SYMB_N:
    case CONST_N:
        return 1;
    case UNARY_N:
        return GlslCollectCallsInExpr(context, source->un.arg);
    case BINARY_N:
        if (source->bin.op == FUN_CALL_OP && source->bin.left != NULL &&
            source->bin.left->common.kind == SYMB_N)
        {
            symbol = source->bin.left->sym.symbol;
            if (!GlslCollectHelper(context, symbol))
                return 0;
        }
        return GlslCollectCallsInExpr(context, source->bin.left) &&
               GlslCollectCallsInExpr(context, source->bin.right);
    case TRINARY_N:
        return GlslCollectCallsInExpr(context, source->tri.arg1) &&
               GlslCollectCallsInExpr(context, source->tri.arg2) &&
               GlslCollectCallsInExpr(context, source->tri.arg3);
    default:
        return 0;
    }
}

static int GlslMappedSignatureEqual(GlslLowerContext *context,
    const Symbol *left, const Symbol *right)
{
    Symbol *leftParam;
    Symbol *rightParam;
    GlslType leftType;
    GlslType rightType;

    leftParam = left->details.fun.params;
    rightParam = right->details.fun.params;
    while (leftParam != NULL && rightParam != NULL) {
        if (!GlslLowerType(context, leftParam->type, &leftType) ||
            !GlslLowerType(context, rightParam->type, &rightType) ||
            !GlslTypesEqual(&leftType, &rightType)) return 0;
        leftParam = leftParam->next;
        rightParam = rightParam->next;
    }
    return leftParam == NULL && rightParam == NULL;
}

static int GlslBuildSignature(GlslLowerContext *context,
    const Symbol *symbol, char *buffer, size_t size)
{
    Symbol *parameter;
    GlslType type;
    const char *typeName;
    size_t used;

    used = 0;
    buffer[0] = '\0';
    for (parameter = symbol->details.fun.params; parameter != NULL;
         parameter = parameter->next)
    {
        if (!GlslLowerType(context, parameter->type, &type)) return 0;
        typeName = GlslTypeName(&type);
        if (typeName == NULL || used + strlen(typeName) + 2 > size) return 0;
        if (used != 0)
            buffer[used++] = '_';
        strcpy(buffer + used, typeName);
        used += strlen(typeName);
    }
    if (used == 0) {
        if (size < 5) return 0;
        strcpy(buffer, "void");
    }
    return 1;
}

static int GlslAssignHelperNames(GlslLowerContext *context)
{
    GlslFunction *function;
    GlslFunction *previous;
    GlslFunction *sameName;
    GlslFunction *sameSignature;
    Symbol *symbol;
    Symbol *previousSymbol;
    const char *sourceName;
    char signature[256];
    char candidate[512];
    int collapsedIndex;

    for (function = context->module->functions; function != NULL;
         function = function->next)
    {
        symbol = (Symbol *) function->identity;
        sourceName = GetAtomString(atable, symbol->name);
        sameName = NULL;
        sameSignature = NULL;
        collapsedIndex = 0;
        for (previous = context->module->functions; previous != function;
             previous = previous->next)
        {
            previousSymbol = (Symbol *) previous->identity;
            if (previousSymbol->name == symbol->name) {
                if (sameName == NULL)
                    sameName = previous;
                if (GlslMappedSignatureEqual(context, previousSymbol,
                                             symbol))
                {
                    sameSignature = previous;
                    collapsedIndex++;
                }
            }
        }
        if (sameName == NULL) {
            function->name = GlslAllocateSymbolName(context->module,
                                                    symbol, sourceName);
        } else if (sameSignature == NULL) {
            function->name = sameName->name;
        } else {
            if (!GlslBuildSignature(context, symbol, signature,
                                    sizeof(signature)) ||
                strlen(sourceName) + strlen(signature) + 24 >
                    sizeof(candidate)) return 0;
            sprintf(candidate, "%s_%s_%d", sourceName, signature,
                    collapsedIndex);
            function->name = GlslAllocateSymbolName(context->module,
                                                    symbol, candidate);
        }
        if (function->name == NULL)
            return 0;
    }
    return 1;
}

static int GlslFunctionIsAfter(const GlslModule *module,
    const GlslFunction *caller, const GlslFunction *callee)
{
    const GlslFunction *function;
    int sawCaller;

    sawCaller = 0;
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function == caller)
            sawCaller = 1;
        if (function == callee)
            return sawCaller;
    }
    return 0;
}

static void GlslMarkForwardCallsInExpr(GlslLowerContext *context,
    GlslFunction *caller, expr *source)
{
    GlslFunction *callee;

    if (source == NULL)
        return;
    switch (source->common.kind) {
    case UNARY_N:
        GlslMarkForwardCallsInExpr(context, caller, source->un.arg);
        break;
    case BINARY_N:
        if (source->bin.op == FUN_CALL_OP && source->bin.left != NULL &&
            source->bin.left->common.kind == SYMB_N)
        {
            callee = GlslFindFunction(context->module,
                                      source->bin.left->sym.symbol);
            if (callee != NULL &&
                GlslFunctionIsAfter(context->module, caller, callee))
            {
                callee->needsPrototype = 1;
            }
        }
        GlslMarkForwardCallsInExpr(context, caller, source->bin.left);
        GlslMarkForwardCallsInExpr(context, caller, source->bin.right);
        break;
    case TRINARY_N:
        GlslMarkForwardCallsInExpr(context, caller, source->tri.arg1);
        GlslMarkForwardCallsInExpr(context, caller, source->tri.arg2);
        GlslMarkForwardCallsInExpr(context, caller, source->tri.arg3);
        break;
    default:
        break;
    }
}

static void GlslMarkForwardCallsInStatements(GlslLowerContext *context,
    GlslFunction *caller, stmt *source)
{
    for (; source != NULL; source = source->commonst.next) {
        switch (source->commonst.kind) {
        case EXPR_STMT:
            GlslMarkForwardCallsInExpr(context, caller, source->exprst.exp);
            break;
        case IF_STMT:
            GlslMarkForwardCallsInExpr(context, caller, source->ifst.cond);
            GlslMarkForwardCallsInStatements(context, caller,
                                              source->ifst.thenstmt);
            GlslMarkForwardCallsInStatements(context, caller,
                                              source->ifst.elsestmt);
            break;
        case WHILE_STMT:
        case DO_STMT:
            GlslMarkForwardCallsInExpr(context, caller, source->whilest.cond);
            GlslMarkForwardCallsInStatements(context, caller,
                                              source->whilest.body);
            break;
        case FOR_STMT:
            GlslMarkForwardCallsInStatements(context, caller,
                                              source->forst.init);
            GlslMarkForwardCallsInExpr(context, caller, source->forst.cond);
            GlslMarkForwardCallsInStatements(context, caller,
                                              source->forst.step);
            GlslMarkForwardCallsInStatements(context, caller,
                                              source->forst.body);
            break;
        case BLOCK_STMT:
            GlslMarkForwardCallsInStatements(context, caller,
                                              source->blockst.body);
            break;
        case RETURN_STMT:
            GlslMarkForwardCallsInExpr(context, caller, source->returnst.exp);
            break;
        case DISCARD_STMT:
            GlslMarkForwardCallsInExpr(context, caller,
                                       source->discardst.cond);
            break;
        default:
            break;
        }
    }
}

static void GlslMarkForwardCalls(GlslLowerContext *context)
{
    GlslFunction *function;
    Symbol *symbol;

    for (function = context->module->functions; function != NULL;
         function = function->next)
    {
        symbol = (Symbol *) function->identity;
        GlslMarkForwardCallsInStatements(context, function,
                                         symbol->details.fun.statements);
    }
}

static void GlslAppendExpr(GlslExpr **list, GlslExpr *expression)
{
    GlslExpr *last;

    if (*list == NULL) {
        *list = expression;
        return;
    }
    for (last = *list; last->next != NULL; last = last->next)
        ;
    last->next = expression;
}

static GlslExpr *GlslLowerExpr(GlslLowerContext *context, expr *source);

static GlslExpr *GlslLowerExprChain(GlslLowerContext *context, expr *source,
                                    opcode listOp)
{
    GlslExpr *list;
    GlslExpr *item;

    list = NULL;
    for (; source != NULL; source = source->bin.right) {
        if (source->common.kind != BINARY_N || source->bin.op != listOp) {
            GlslRecordFailure(context, "GLSL expression list");
            return NULL;
        }
        item = GlslLowerExpr(context, source->bin.left);
        if (item == NULL)
            return NULL;
        GlslAppendExpr(&list, item);
    }
    return list;
}

static GlslExpr *GlslNewLiteral(GlslLowerContext *context, GlslBase base,
    int intValue, float floatValue)
{
    GlslExprKind kind;
    GlslExpr *target;
    GlslType type;

    type = GlslNumericType(base, 1);
    if (base == GLSL_BASE_FLOAT)
        kind = GLSL_EXPR_FLOAT;
    else if (base == GLSL_BASE_BOOL)
        kind = GLSL_EXPR_BOOL;
    else
        kind = GLSL_EXPR_INT;
    target = GlslNewExpr(context->module, kind, type);
    if (target == NULL)
        return NULL;
    if (kind == GLSL_EXPR_FLOAT)
        target->u.literalFloat = floatValue;
    else if (kind == GLSL_EXPR_BOOL)
        target->u.literalBool = intValue != 0;
    else
        target->u.literalInt = intValue;
    return target;
}

static GlslExpr *GlslLowerConstant(GlslLowerContext *context, expr *source,
                                   const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *item;
    int i;

    if (type->base == GLSL_BASE_FLOAT) {
        for (i = 0; i < type->len; i++) {
            if (source->co.val[i].f != source->co.val[i].f ||
                source->co.val[i].f > FLT_MAX ||
                source->co.val[i].f < -FLT_MAX)
            {
                GlslRecordFailure(context,
                                  "non-finite floating-point constant");
                return NULL;
            }
        }
    }

    if (type->len == 1) {
        if (type->base == GLSL_BASE_FLOAT)
            return GlslNewLiteral(context, type->base, 0, source->co.val[0].f);
        return GlslNewLiteral(context, type->base, source->co.val[0].i, 0.0f);
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    for (i = 0; i < type->len; i++) {
        if (type->base == GLSL_BASE_FLOAT)
            item = GlslNewLiteral(context, type->base, 0, source->co.val[i].f);
        else
            item = GlslNewLiteral(context, type->base, source->co.val[i].i, 0.0f);
        if (item == NULL)
            return NULL;
        GlslAppendExpr(&target->u.construct.arguments, item);
    }
    return target;
}

static GlslExpr *GlslNewSwizzle(GlslLowerContext *context, GlslExpr *object,
    const GlslType *type, const char *mask)
{
    GlslExpr *target;

    target = GlslNewExpr(context->module, GLSL_EXPR_SWIZZLE, *type);
    if (target != NULL) {
        target->u.swizzle.object = object;
        target->u.swizzle.mask = GlslCopyText(context->module, mask);
        if (target->u.swizzle.mask == NULL)
            return NULL;
    }
    return target;
}

static GlslExpr *GlslLowerSwizzle(GlslLowerContext *context, expr *source,
                                  const GlslType *type)
{
    GlslExpr *object;
    GlslExpr *target;
    char maskText[5];
    int count;
    int i;
    int mask;

    object = GlslLowerExpr(context, source->un.arg);
    if (object == NULL)
        return NULL;
    count = SUBOP_GET_S2(source->un.subop);
    if (count == 0)
        count = 1;
    if (count < 1 || count > 4)
        return NULL;
    mask = SUBOP_GET_MASK(source->un.subop);
    for (i = count - 1; i >= 0; i--)
        maskText[i] = "xyzw"[(mask >> (i * 2)) & 3];
    maskText[count] = '\0';
    if (object->type.len == 1) {
        if (type->len == 1)
            return object;
        target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
        if (target != NULL)
            target->u.construct.arguments = object;
        return target;
    }
    return GlslNewSwizzle(context, object, type, maskText);
}

static GlslOperator GlslUnaryOperator(opcode op)
{
    switch (op) {
    case NEG_OP:
    case NEG_V_OP: return GLSL_OP_NEGATE;
    case POS_OP:
    case POS_V_OP: return GLSL_OP_POSITIVE;
    case BNOT_OP:
    case BNOT_V_OP: return GLSL_OP_LOGICAL_NOT;
    default: return GLSL_OP_NONE;
    }
}

static GlslOperator GlslBinaryOperator(opcode op)
{
    switch (op) {
    case ASSIGN_OP:
    case ASSIGN_V_OP:
    case ASSIGN_GEN_OP: return GLSL_OP_ASSIGN;
    case MUL_OP: case MUL_V_OP: case MUL_SV_OP: case MUL_VS_OP:
        return GLSL_OP_MULTIPLY;
    case DIV_OP: case DIV_V_OP: case DIV_SV_OP: case DIV_VS_OP:
        return GLSL_OP_DIVIDE;
    case ADD_OP: case ADD_V_OP: case ADD_SV_OP: case ADD_VS_OP:
        return GLSL_OP_ADD;
    case SUB_OP: case SUB_V_OP: case SUB_SV_OP: case SUB_VS_OP:
        return GLSL_OP_SUBTRACT;
    case LT_OP: return GLSL_OP_LESS;
    case GT_OP: return GLSL_OP_GREATER;
    case LE_OP: return GLSL_OP_LESS_EQUAL;
    case GE_OP: return GLSL_OP_GREATER_EQUAL;
    case EQ_OP: return GLSL_OP_EQUAL;
    case NE_OP: return GLSL_OP_NOT_EQUAL;
    case BAND_OP: return GLSL_OP_LOGICAL_AND;
    case BOR_OP: return GLSL_OP_LOGICAL_OR;
    default: return GLSL_OP_NONE;
    }
}

static const char *GlslVectorComparisonName(opcode op)
{
    switch (op) {
    case LT_V_OP: case LT_SV_OP: case LT_VS_OP: return "lessThan";
    case GT_V_OP: case GT_SV_OP: case GT_VS_OP: return "greaterThan";
    case LE_V_OP: case LE_SV_OP: case LE_VS_OP: return "lessThanEqual";
    case GE_V_OP: case GE_SV_OP: case GE_VS_OP: return "greaterThanEqual";
    case EQ_V_OP: case EQ_SV_OP: case EQ_VS_OP: return "equal";
    case NE_V_OP: case NE_SV_OP: case NE_VS_OP: return "notEqual";
    default: return NULL;
    }
}

static GlslExpr *GlslLowerCall(GlslLowerContext *context, expr *source,
                               const GlslType *type)
{
    GlslExpr *target;
    GlslFunction *function;
    Symbol *symbol;
    const char *name;

    if (source->bin.left == NULL ||
        source->bin.left->common.kind != SYMB_N) return NULL;
    symbol = source->bin.left->sym.symbol;
    function = GlslFindFunction(context->module, symbol);
    if (function != NULL) {
        name = function->name;
    } else if (symbol != NULL && symbol->kind == FUNCTION_S &&
               (symbol->properties & SYMB_IS_BUILTIN)) {
        name = GetAtomString(atable, symbol->name);
    } else {
        return NULL;
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CALL, *type);
    if (target == NULL)
        return NULL;
    target->u.call.name = name;
    if (source->bin.right != NULL) {
        target->u.call.arguments = GlslLowerExprChain(context,
            source->bin.right, FUN_ARG_OP);
        if (target->u.call.arguments == NULL)
            return NULL;
    }
    return target;
}

static GlslExpr *GlslLowerVectorComparison(GlslLowerContext *context,
    expr *source, const GlslType *type, const char *name)
{
    GlslExpr *target;
    GlslExpr *constructor;
    GlslExpr *left;
    GlslExpr *right;
    GlslType vectorType;

    left = GlslLowerExpr(context, source->bin.left);
    right = GlslLowerExpr(context, source->bin.right);
    if (left == NULL || right == NULL)
        return NULL;
    if (left->type.len == 1 && type->len > 1) {
        vectorType = GlslNumericType(left->type.base, type->len);
        constructor = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT,
                                  vectorType);
        if (constructor == NULL)
            return NULL;
        constructor->u.construct.arguments = left;
        left = constructor;
    }
    if (right->type.len == 1 && type->len > 1) {
        vectorType = GlslNumericType(right->type.base, type->len);
        constructor = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT,
                                  vectorType);
        if (constructor == NULL)
            return NULL;
        constructor->u.construct.arguments = right;
        right = constructor;
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CALL, *type);
    if (target == NULL)
        return NULL;
    target->u.call.name = name;
    target->u.call.arguments = left;
    left->next = right;
    return target;
}

static GlslExpr *GlslLowerMaskedAssignment(GlslLowerContext *context,
    expr *source, const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *left;
    GlslExpr *right;
    GlslType maskedType;
    char maskText[5];
    int mask;
    int i;
    int count;

    left = GlslLowerExpr(context, source->bin.left);
    right = GlslLowerExpr(context, source->bin.right);
    if (left == NULL || right == NULL)
        return NULL;
    mask = SUBOP_GET_MASK(source->bin.subop);
    count = 0;
    for (i = 0; i < 4; i++) {
        if (mask & (1 << i))
            maskText[count++] = "xyzw"[i];
    }
    maskText[count] = '\0';
    if (count == 0)
        return NULL;
    maskedType = GlslNumericType(left->type.base, count);
    left = GlslNewSwizzle(context, left, &maskedType, maskText);
    if (left == NULL)
        return NULL;
    target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, *type);
    if (target != NULL) {
        target->u.binary.op = GLSL_OP_ASSIGN;
        target->u.binary.left = left;
        target->u.binary.right = right;
    }
    return target;
}

static GlslExpr *GlslLowerComponent(GlslLowerContext *context, expr *source,
    int component, GlslBase base)
{
    GlslExpr *target;
    GlslType type;
    int len;
    char mask[2];

    target = GlslLowerExpr(context, source);
    if (target == NULL)
        return NULL;
    if (!IsVector(source->common.type, &len) || len <= 1)
        return target;
    type = GlslNumericType(base, 1);
    mask[0] = "xyzw"[component];
    mask[1] = '\0';
    return GlslNewSwizzle(context, target, &type, mask);
}

static GlslExpr *GlslLowerConditional(GlslLowerContext *context,
    expr *source, const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *componentExpr;
    GlslExpr *condition;
    GlslExpr *trueExpr;
    GlslExpr *falseExpr;
    GlslType componentType;
    int conditionLen;
    int i;

    conditionLen = 0;
    IsVector(source->tri.arg1->common.type, &conditionLen);
    if (conditionLen <= 1) {
        target = GlslNewExpr(context->module, GLSL_EXPR_CONDITIONAL, *type);
        if (target == NULL)
            return NULL;
        target->u.conditional.condition = GlslLowerExpr(context,
                                                        source->tri.arg1);
        target->u.conditional.trueExpr = GlslLowerExpr(context,
                                                       source->tri.arg2);
        target->u.conditional.falseExpr = GlslLowerExpr(context,
                                                        source->tri.arg3);
        if (target->u.conditional.condition == NULL ||
            target->u.conditional.trueExpr == NULL ||
            target->u.conditional.falseExpr == NULL) return NULL;
        return target;
    }
    if (type->len < 2 || type->len > 4 || conditionLen != type->len)
        return NULL;
    target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    componentType = GlslNumericType(type->base, 1);
    for (i = 0; i < type->len; i++) {
        condition = GlslLowerComponent(context, source->tri.arg1, i,
                                       GLSL_BASE_BOOL);
        trueExpr = GlslLowerComponent(context, source->tri.arg2, i,
                                      type->base);
        falseExpr = GlslLowerComponent(context, source->tri.arg3, i,
                                       type->base);
        if (condition == NULL || trueExpr == NULL || falseExpr == NULL)
            return NULL;
        componentExpr = GlslNewExpr(context->module,
            GLSL_EXPR_CONDITIONAL, componentType);
        if (componentExpr == NULL)
            return NULL;
        componentExpr->u.conditional.condition = condition;
        componentExpr->u.conditional.trueExpr = trueExpr;
        componentExpr->u.conditional.falseExpr = falseExpr;
        GlslAppendExpr(&target->u.construct.arguments, componentExpr);
    }
    return target;
}

static GlslExpr *GlslLowerExpr(GlslLowerContext *context, expr *source)
{
    GlslExpr *target;
    GlslExpr *operand;
    GlslDecl *decl;
    GlslType type;
    GlslOperator op;
    Symbol *member;
    const char *comparison;

    if (source == NULL || !GlslLowerType(context, source->common.type, &type)) {
        GlslRecordFailure(context, "GLSL 1.10 expression type");
        return NULL;
    }
    if (source->common.kind == SYMB_N && source->sym.op == VARIABLE_OP) {
        decl = GlslFindDecl(context, source->sym.symbol);
        if (decl == NULL)
            return NULL;
        target = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, type);
        if (target != NULL)
            target->u.symbol = decl;
        return target;
    }
    if (source->common.kind == CONST_N)
        return GlslLowerConstant(context, source, &type);
    if (source->common.kind == UNARY_N) {
        if (source->un.op == SWIZZLE_Z_OP)
            return GlslLowerSwizzle(context, source, &type);
        if (source->un.op == VECTOR_V_OP) {
            target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, type);
            if (target == NULL)
                return NULL;
            target->u.construct.arguments = GlslLowerExprChain(context,
                source->un.arg, EXPR_LIST_OP);
            if (target->u.construct.arguments == NULL)
                return NULL;
            return target;
        }
        if (source->un.op == CAST_CS_OP || source->un.op == CAST_CV_OP) {
            operand = GlslLowerExpr(context, source->un.arg);
            if (operand == NULL)
                return NULL;
            if (GlslTypesEqual(&operand->type, &type))
                return operand;
            target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, type);
            if (target != NULL)
                target->u.construct.arguments = operand;
            return target;
        }
        if (source->un.op == BNOT_V_OP) {
            operand = GlslLowerExpr(context, source->un.arg);
            if (operand == NULL)
                return NULL;
            target = GlslNewExpr(context->module, GLSL_EXPR_CALL, type);
            if (target != NULL) {
                target->u.call.name = "not";
                target->u.call.arguments = operand;
            }
            return target;
        }
        op = GlslUnaryOperator(source->un.op);
        if (op != GLSL_OP_NONE) {
            target = GlslNewExpr(context->module, GLSL_EXPR_UNARY, type);
            if (target == NULL)
                return NULL;
            target->u.unary.op = op;
            target->u.unary.operand = GlslLowerExpr(context, source->un.arg);
            if (target->u.unary.operand == NULL)
                return NULL;
            return target;
        }
    }
    if (source->common.kind == BINARY_N) {
        if (source->bin.op == MEMBER_SELECTOR_OP) {
            if (source->bin.right == NULL ||
                source->bin.right->common.kind != SYMB_N ||
                source->bin.right->sym.op != MEMBER_OP) return NULL;
            member = source->bin.right->sym.symbol;
            if (source->bin.left != NULL &&
                source->bin.left->common.kind == SYMB_N &&
                source->bin.left->sym.op == VARIABLE_OP &&
                (source->bin.left->sym.symbol == Cg->theHAL->varyingIn ||
                 source->bin.left->sym.symbol == Cg->theHAL->varyingOut))
            {
                decl = GlslLowerInterface(context, member);
                if (decl == NULL)
                    return NULL;
                target = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, type);
                if (target != NULL)
                    target->u.symbol = decl;
                return target;
            }
            decl = GlslFindDecl(context, member);
            if (decl == NULL)
                return NULL;
            target = GlslNewExpr(context->module, GLSL_EXPR_MEMBER, type);
            if (target == NULL)
                return NULL;
            target->u.member.object = GlslLowerExpr(context, source->bin.left);
            if (target->u.member.object == NULL)
                return NULL;
            target->u.member.decl = decl;
            target->u.member.name = decl->name;
            return target;
        }
        if (source->bin.op == ARRAY_INDEX_OP) {
            target = GlslNewExpr(context->module, GLSL_EXPR_INDEX, type);
            if (target == NULL)
                return NULL;
            target->u.index.object = GlslLowerExpr(context, source->bin.left);
            target->u.index.index = GlslLowerExpr(context, source->bin.right);
            if (target->u.index.object == NULL || target->u.index.index == NULL)
                return NULL;
            return target;
        }
        if (source->bin.op == FUN_CALL_OP ||
            source->bin.op == FUN_BUILTIN_OP)
            return GlslLowerCall(context, source, &type);
        if (source->bin.op == ASSIGN_MASKED_KV_OP)
            return GlslLowerMaskedAssignment(context, source, &type);
        comparison = GlslVectorComparisonName(source->bin.op);
        if (comparison != NULL)
            return GlslLowerVectorComparison(context, source, &type,
                                              comparison);
        op = GlslBinaryOperator(source->bin.op);
        if (op != GLSL_OP_NONE) {
            target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, type);
            if (target == NULL)
                return NULL;
            target->u.binary.op = op;
            target->u.binary.left = GlslLowerExpr(context, source->bin.left);
            target->u.binary.right = GlslLowerExpr(context, source->bin.right);
            if (target->u.binary.left == NULL ||
                target->u.binary.right == NULL) return NULL;
            return target;
        }
    }
    if (source->common.kind == TRINARY_N &&
        (source->tri.op == COND_OP || source->tri.op == COND_V_OP ||
         source->tri.op == COND_SV_OP || source->tri.op == COND_GEN_OP))
    {
        return GlslLowerConditional(context, source, &type);
    }
    GlslRecordFailure(context, GlslUnsupportedExprReason(source));
    return NULL;
}

static int GlslLowerStatementList(GlslLowerContext *context, stmt *source,
                                  GlslStmt **list);

static int GlslLowerBranch(GlslLowerContext *context, stmt *source,
                           GlslStmt **list)
{
    if (source != NULL && source->commonst.next == NULL &&
        source->commonst.kind == BLOCK_STMT)
    {
        source = source->blockst.body;
    }
    return GlslLowerStatementList(context, source, list);
}

static int GlslLowerForPart(GlslLowerContext *context, stmt *source,
                            GlslStmt **list)
{
    GlslStmt *target;

    for (; source != NULL; source = source->commonst.next) {
        context->statementLoc = source->commonst.loc;
        if (source->commonst.kind != EXPR_STMT || source->exprst.exp == NULL) {
            GlslRecordFailure(context, "GLSL for expression");
            return 0;
        }
        target = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
        if (target == NULL)
            return 0;
        GlslSetLoc(&target->loc, &source->commonst.loc);
        target->u.expression = GlslLowerExpr(context, source->exprst.exp);
        if (target->u.expression == NULL)
            return 0;
        GlslAppendStmt(list, target);
    }
    return 1;
}

static int GlslLowerStatementList(GlslLowerContext *context, stmt *source,
                                  GlslStmt **list)
{
    GlslStmt *target;

    for (; source != NULL; source = source->commonst.next) {
        context->statementLoc = source->commonst.loc;
        if (source->commonst.kind == COMMENT_STMT)
            continue;
        switch (source->commonst.kind) {
        case EXPR_STMT:
            if (source->exprst.exp == NULL)
                continue;
            target = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
            if (target == NULL)
                return 0;
            target->u.expression = GlslLowerExpr(context, source->exprst.exp);
            if (target->u.expression == NULL)
                return 0;
            break;
        case IF_STMT:
            target = GlslNewStmt(context->module, GLSL_STMT_IF);
            if (target == NULL)
                return 0;
            target->u.ifStmt.condition = GlslLowerExpr(context,
                                                       source->ifst.cond);
            if (target->u.ifStmt.condition == NULL ||
                !GlslLowerBranch(context, source->ifst.thenstmt,
                                 &target->u.ifStmt.trueBranch) ||
                !GlslLowerBranch(context, source->ifst.elsestmt,
                                 &target->u.ifStmt.falseBranch)) return 0;
            break;
        case WHILE_STMT:
        case DO_STMT:
            target = GlslNewStmt(context->module,
                source->commonst.kind == WHILE_STMT ?
                GLSL_STMT_WHILE : GLSL_STMT_DO);
            if (target == NULL)
                return 0;
            target->u.loop.condition = GlslLowerExpr(context,
                                                     source->whilest.cond);
            if (target->u.loop.condition == NULL)
                return 0;
            context->loopDepth++;
            if (!GlslLowerBranch(context, source->whilest.body,
                                 &target->u.loop.body)) {
                context->loopDepth--;
                return 0;
            }
            context->loopDepth--;
            break;
        case FOR_STMT:
            target = GlslNewStmt(context->module, GLSL_STMT_FOR);
            if (target == NULL)
                return 0;
            if (!GlslLowerForPart(context, source->forst.init,
                                  &target->u.forStmt.init) ||
                (source->forst.cond != NULL &&
                 (target->u.forStmt.condition = GlslLowerExpr(
                    context, source->forst.cond)) == NULL) ||
                !GlslLowerForPart(context, source->forst.step,
                                  &target->u.forStmt.step)) return 0;
            context->loopDepth++;
            if (!GlslLowerBranch(context, source->forst.body,
                                 &target->u.forStmt.body)) {
                context->loopDepth--;
                return 0;
            }
            context->loopDepth--;
            break;
        case BLOCK_STMT:
            target = GlslNewStmt(context->module, GLSL_STMT_BLOCK);
            if (target == NULL)
                return 0;
            if (!GlslLowerStatementList(context, source->blockst.body,
                                        &target->u.block)) return 0;
            break;
        case RETURN_STMT:
            target = GlslNewStmt(context->module, GLSL_STMT_RETURN);
            if (target == NULL)
                return 0;
            if (source->returnst.exp != NULL) {
                target->u.returnExpr = GlslLowerExpr(context,
                                                     source->returnst.exp);
                if (target->u.returnExpr == NULL)
                    return 0;
            }
            break;
        case BREAK_STMT:
            if (context->loopDepth == 0) {
                GlslRecordFailure(context, "break outside loop");
                return 0;
            }
            target = GlslNewStmt(context->module, GLSL_STMT_BREAK);
            break;
        case CONTINUE_STMT:
            if (context->loopDepth == 0) {
                GlslRecordFailure(context, "continue outside loop");
                return 0;
            }
            target = GlslNewStmt(context->module, GLSL_STMT_CONTINUE);
            break;
        default:
            GlslRecordFailure(context, "GLSL 1.10 statement");
            return 0;
        }
        if (target == NULL)
            return 0;
        GlslSetLoc(&target->loc, &source->commonst.loc);
        GlslAppendStmt(list, target);
    }
    return 1;
}

static int GlslLowerHelper(GlslLowerContext *context,
                           GlslFunction *function)
{
    Symbol *symbol;

    symbol = (Symbol *) function->identity;
    context->function = function;
    context->statementLoc = symbol->loc;
    if (!GlslCollectParameters(context, symbol->details.fun.params, 0) ||
        !GlslCollectLocals(context, symbol->details.fun.locals->symbols, 0) ||
        !GlslLowerStatementList(context, symbol->details.fun.statements,
                                &function->body)) return 0;
    return 1;
}

int GlslLowerProgram(GlslModule *module, const GlslProfileDesc *profile,
                     SourceLoc *loc, Scope *scope, Symbol *program)
{
    GlslLowerContext context;
    GlslFunction *function;
    GlslFunction *helper;
    GlslFunction *next;
    GlslType voidType;
    Type *result;
    const char *functionName;

    if (module == NULL || profile == NULL || scope == NULL ||
        program == NULL || program->kind != FUNCTION_S ||
        profile->stage != GLSL_STAGE_VERTEX)
    {
        if (module != NULL)
            module->errors++;
        return 0;
    }
    memset(&context, 0, sizeof(context));
    context.module = module;
    context.profile = profile;
    context.scope = scope;
    context.statementLoc = program->loc;
    result = program->type->fun.rettype;
    if (GetCategory(result) != TYPE_CATEGORY_STRUCT ||
        !GlslEnsureType(&context, result) ||
        !GlslEnsureParameterTypes(&context,
                                  program->details.fun.params) ||
        program->details.fun.locals == NULL ||
        !GlslEnsureSymbolTypes(&context,
                               program->details.fun.locals->symbols) ||
        !GlslCollectCallsInStatements(&context,
                                      program->details.fun.statements) ||
        !GlslSortStructs(&context) ||
        !GlslAssignHelperNames(&context)) return GlslLowerError(&context);
    GlslMarkForwardCalls(&context);
    for (helper = module->functions; helper != NULL; helper = next) {
        next = helper->next;
        if (!GlslLowerHelper(&context, helper))
            return GlslLowerError(&context);
    }
    functionName = GlslAllocateSymbolName(module, program, "main");
    if (functionName == NULL)
        return GlslLowerError(&context);
    voidType = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(module, voidType, functionName);
    if (function == NULL)
        return GlslLowerError(&context);
    function->identity = program;
    function->isEntry = 1;
    GlslSetLoc(&function->loc, loc != NULL ? loc : &program->loc);
    context.function = function;
    module->entry = function;
    GlslAppendFunction(&module->functions, function);
    if (!GlslCollectParameters(&context, program->details.fun.params, 1) ||
        !GlslCollectLocals(&context, program->details.fun.locals->symbols, 1) ||
        !GlslLowerStatementList(&context, program->details.fun.statements,
                                &function->body))
    {
        return GlslLowerError(&context);
    }
    return module->errors == 0;
}
