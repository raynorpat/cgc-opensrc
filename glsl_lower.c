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
license, under NVIDIA's copyrights in this original NVIDIA software
(the "NVIDIA Software"), to use, reproduce, modify and redistribute the
NVIDIA Software, with or without modifications, in source and/or binary
forms; provided that if you redistribute the NVIDIA Software, you must
retain the copyright notice of NVIDIA, this notice and the following
text and disclaimers in all such redistributions of the NVIDIA Software.
Neither the name, trademarks, service marks nor logos of NVIDIA
Corporation may be used to endorse or promote products derived from
this NVIDIA Software without specific prior written permission from
NVIDIA. Except as expressly stated in this notice, no other rights or
licenses express or implied, are granted by NVIDIA herein, including
but not limited to any patent rights that may be infringed by your
derivative works or by other works in which the NVIDIA Software may be
incorporated. No hardware is licensed hereunder.

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OR CONDITIONS OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
OR ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER
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

#include "slglobals.h"
#include "glsl_hal.h"

typedef struct GlslLowerContext_Rec {
    GlslModule *module;
    const GlslProfileDesc *profile;
    Scope *scope;
    GlslFunction *function;
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

    decl = GlslFindDeclList(context->function->parameters, identity);
    if (decl == NULL)
        decl = GlslFindDeclList(context->function->locals, identity);
    if (decl == NULL)
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
    int category;
    int len;

    if (source == NULL || target == NULL)
        return 0;
    category = GetCategory(source);
    if (category == TYPE_CATEGORY_SCALAR &&
        GetBase(source) == TYPE_BASE_FLOAT)
    {
        *target = GlslNumericType(GLSL_BASE_FLOAT, 1);
        return 1;
    }
    if (category == TYPE_CATEGORY_ARRAY && IsVector(source, &len) &&
        GetBase(source) == TYPE_BASE_FLOAT && len >= 1 && len <= 4)
    {
        *target = GlslNumericType(GLSL_BASE_FLOAT, len);
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
                                   Symbol *symbol)
{
    GlslDecl *decl;
    GlslType type;
    const char *sourceName;
    const char *name;

    if (!GlslLowerType(context, symbol->type, &type))
        return NULL;
    sourceName = GetAtomString(atable, symbol->name);
    name = sourceName;
    if (name == NULL)
        return NULL;
    decl = GlslNewDecl(context->module, GLSL_STORAGE_NONE, type, name);
    if (decl != NULL) {
        decl->identity = symbol;
        GlslSetLoc(&decl->loc, &symbol->loc);
    }
    return decl;
}

static int GlslCollectMembers(GlslLowerContext *context, Symbol *symbol,
                              GlslDecl **members)
{
    GlslDecl *decl;

    if (symbol == NULL)
        return 1;
    if (!GlslCollectMembers(context, symbol->left, members))
        return 0;
    if (symbol->kind == VARIABLE_S) {
        decl = GlslNewSourceDecl(context, symbol);
        if (decl == NULL)
            return 0;
        GlslInsertDecl(members, decl);
    }
    if (!GlslCollectMembers(context, symbol->right, members))
        return 0;
    return 1;
}

static int GlslBuildEntryStruct(GlslLowerContext *context, Type *type)
{
    GlslDecl *decl;
    GlslType structType;
    Symbol *tag;
    const char *sourceName;
    const char *name;

    if (GetCategory(type) != TYPE_CATEGORY_STRUCT)
        return 0;
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
    if (!GlslCollectMembers(context, type->str.members->symbols,
                            &decl->members))
    {
        return 0;
    }
    return 1;
}

static int GlslCollectFormals(GlslLowerContext *context, Symbol *formal)
{
    GlslDecl *decl;

    for (; formal != NULL; formal = formal->next) {
        if (GetDomain(formal->type) == TYPE_DOMAIN_UNIFORM)
            return 0;
        decl = GlslNewSourceDecl(context, formal);
        if (decl == NULL)
            return 0;
        GlslInsertDecl(&context->function->locals, decl);
    }
    return 1;
}

static int GlslCollectLocals(GlslLowerContext *context, Symbol *symbol)
{
    const char *name;
    GlslDecl *decl;

    if (symbol == NULL)
        return 1;
    if (!GlslCollectLocals(context, symbol->left))
        return 0;
    if (symbol->kind == VARIABLE_S &&
        GlslFindDecl(context, symbol) == NULL)
    {
        name = GetAtomString(atable, symbol->name);
        if (name == NULL || name[0] == '$')
            return 0;
        decl = GlslNewSourceDecl(context, symbol);
        if (decl == NULL)
            return 0;
        GlslInsertDecl(&context->function->locals, decl);
    }
    if (!GlslCollectLocals(context, symbol->right))
        return 0;
    return 1;
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
    char attributeName[256];
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
        ((sourceBinding->none.properties & BIND_INPUT) != 0))
    {
        return NULL;
    }
    canonical = GetAtomString(atable, sourceBinding->conn.rname);
    interfaceName = GlslCanonicalInterfaceName(context->profile,
                                                sourceBinding->conn.rname,
                                                isOutput);
    if (canonical == NULL || interfaceName == NULL ||
        !GlslLowerType(context, member->type, &type))
    {
        return NULL;
    }
    if (!strncmp(interfaceName, "gl_", 3)) {
        storage = GLSL_STORAGE_BUILTIN;
        name = interfaceName;
    } else if (!isOutput &&
               context->profile->stage == GLSL_STAGE_VERTEX) {
        storage = GLSL_STORAGE_ATTRIBUTE;
        if (strlen(interfaceName) + 4 > sizeof(attributeName))
            return NULL;
        sprintf(attributeName, "cg_%s", interfaceName);
        name = GlslAllocateSymbolName(context->module, member,
                                      attributeName);
    } else {
        return NULL;
    }
    if (name == NULL)
        return NULL;
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
    if (context->module->bindings == NULL) {
        context->module->bindings = binding;
    } else {
        GlslBinding *last;

        for (last = context->module->bindings; last->next != NULL;
             last = last->next)
            ;
        last->next = binding;
    }
    return decl;
}

static GlslExpr *GlslLowerExpr(GlslLowerContext *context, expr *source)
{
    GlslExpr *target;
    GlslDecl *decl;
    GlslType type;
    Symbol *member;

    if (source == NULL || !GlslLowerType(context, source->common.type, &type))
        return NULL;
    if (source->common.kind == SYMB_N && source->sym.op == VARIABLE_OP) {
        decl = GlslFindDecl(context, source->sym.symbol);
        if (decl == NULL)
            return NULL;
        target = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, type);
        if (target != NULL)
            target->u.symbol = decl;
        return target;
    }
    if (source->common.kind == BINARY_N &&
        source->bin.op == MEMBER_SELECTOR_OP)
    {
        if (source->bin.right == NULL ||
            source->bin.right->common.kind != SYMB_N ||
            source->bin.right->sym.op != MEMBER_OP)
        {
            return NULL;
        }
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
        target->u.member.object = GlslLowerExpr(context,
                                                source->bin.left);
        if (target->u.member.object == NULL)
            return NULL;
        target->u.member.decl = decl;
        target->u.member.name = decl->name;
        return target;
    }
    if (source->common.kind == BINARY_N &&
        (source->bin.op == ASSIGN_OP ||
         source->bin.op == ASSIGN_V_OP ||
         source->bin.op == ASSIGN_GEN_OP))
    {
        target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, type);
        if (target == NULL)
            return NULL;
        target->u.binary.op = '=';
        target->u.binary.left = GlslLowerExpr(context, source->bin.left);
        target->u.binary.right = GlslLowerExpr(context, source->bin.right);
        if (target->u.binary.left == NULL ||
            target->u.binary.right == NULL)
        {
            return NULL;
        }
        return target;
    }
    return NULL;
}

static int GlslLowerStatements(GlslLowerContext *context, stmt *source)
{
    GlslStmt *target;

    for (; source != NULL; source = source->commonst.next) {
        if (source->commonst.kind != EXPR_STMT ||
            source->exprst.exp == NULL)
        {
            return 0;
        }
        target = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
        if (target == NULL)
            return 0;
        GlslSetLoc(&target->loc, &source->commonst.loc);
        target->u.expression = GlslLowerExpr(context, source->exprst.exp);
        if (target->u.expression == NULL)
            return 0;
        GlslAppendStmt(&context->function->body, target);
    }
    return 1;
}

int GlslLowerProgram(GlslModule *module, const GlslProfileDesc *profile,
                     SourceLoc *loc, Scope *scope, Symbol *program)
{
    GlslLowerContext context;
    GlslFunction *function;
    GlslType voidType;
    Type *result;

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
    result = program->type->fun.rettype;
    if (!GlslBuildEntryStruct(&context, result))
        return GlslLowerError(&context);
    voidType = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(module, voidType, "main");
    if (function == NULL)
        return GlslLowerError(&context);
    function->identity = program;
    function->isEntry = 1;
    GlslSetLoc(&function->loc, loc != NULL ? loc : &program->loc);
    context.function = function;
    module->entry = function;
    GlslAppendFunction(&module->functions, function);
    if (!GlslCollectFormals(&context, program->details.fun.params) ||
        !GlslCollectLocals(&context,
                           program->details.fun.locals->symbols) ||
        !GlslLowerStatements(&context, program->details.fun.statements))
    {
        return GlslLowerError(&context);
    }
    return module->errors == 0;
}
