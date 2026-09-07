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
license, under NVIDIA's copyrights in this original NVIDIA software,
to use, reproduce, modify and redistribute the NVIDIA Software, with or
without modifications, in source and/or binary forms; provided that if
you redistribute the NVIDIA Software, you must retain the copyright
notice of NVIDIA, this notice and the following text and disclaimers in
all such redistributions of the NVIDIA Software.  Neither the name,
trademarks, service marks nor logos of NVIDIA Corporation may be used
to endorse or promote products derived from the NVIDIA Software without
specific prior written permission from NVIDIA.  Except as expressly
stated in this notice, no other rights or licenses express or implied,
are granted by NVIDIA herein, including but not limited to any patent
rights that may be infringed by your derivative works or by other works
in which the NVIDIA Software may be incorporated. No hardware is
licensed hereunder.

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OR CONDITIONS OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
OR ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER
PRODUCTS.

IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN
ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
OF THE NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF
CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE,
EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "hlsl_lower_internal.h"

void HlslSetLoc(HlslLoc *target, const SourceLoc *source)
{
    if (target != NULL && source != NULL) {
        target->file = source->file;
        target->line = source->line;
    }
} // HlslSetLoc

HlslExpr *HlslNewSourceExpr(HlslLowerContext *context,
                                   HlslExprKind kind, HlslType type)
{
    HlslLoc loc;

    loc.file = 0;
    loc.line = 0;
    if (context != NULL)
        HlslSetLoc(&loc, &context->statementLoc);
    return context != NULL ?
           HlslNewLocatedExpr(context->module, kind, type, &loc) : NULL;
} // HlslNewSourceExpr

int HlslLowerFailure(HlslLowerContext *context, HlslErrorKind kind,
                            const char *reason, const SourceLoc *loc)
{
    HlslModule *module;

    module = context != NULL ? context->module : NULL;
    if (module != NULL && module->errors == 0) {
        module->errorKind = kind;
        module->errorReason = reason;
        HlslSetLoc(&module->errorLoc, loc != NULL ? loc :
                   &context->statementLoc);
    }
    if (module != NULL)
        module->errors++;
    return 0;
} // HlslLowerFailure

void *HlslLowerAlloc(HlslLowerContext *context, size_t size)
{
    void *memory;

    if (context == NULL || context->module == NULL ||
        context->module->alloc == NULL)
    {
        return NULL;
    }
    memory = (*context->module->alloc)(context->module->allocArg, size);
    if (memory != NULL)
        memset(memory, 0, size);
    return memory;
} // HlslLowerAlloc

char *HlslGeneratedSource(HlslLowerContext *context,
                                 const char *source)
{
    char *name;
    size_t length;

    if (source == NULL)
        return NULL;
    length = strlen(source);
    name = (char *) HlslLowerAlloc(context, length + 4);
    if (name == NULL)
        return NULL;
    memcpy(name, "cg_", 3);
    memcpy(name + 3, source, length + 1);
    return name;
} // HlslGeneratedSource

HlslDecl *HlslFindDeclList(HlslDecl *list, const void *identity)
{
    HlslDecl *decl;

    for (decl = list; decl != NULL; decl = decl->next) {
        if (decl->identity == identity)
            return decl;
    }
    return NULL;
} // HlslFindDeclList

HlslDecl *HlslFindDecl(HlslLowerContext *context,
                              const void *identity)
{
    HlslDecl *decl;
    HlslBinding *binding;

    if (context->function != NULL) {
        decl = HlslFindDeclList(context->function->parameters, identity);
        if (decl == NULL)
            decl = HlslFindDeclList(context->function->locals, identity);
        if (decl != NULL)
            return decl;
    }
    decl = HlslFindDeclList(context->module->globals, identity);
    if (decl != NULL)
        return decl;
    for (decl = context->module->structs; decl != NULL; decl = decl->next) {
        HlslDecl *member;

        member = HlslFindDeclList(decl->members, identity);
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
} // HlslFindDecl

HlslFunction *HlslFindFunction(HlslModule *module,
                                      const void *identity)
{
    HlslFunction *function;

    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function->identity == identity)
            return function;
    }
    return NULL;
} // HlslFindFunction

char *HlslCopyText(HlslLowerContext *context, const char *text)
{
    char *copy;
    size_t length;

    length = strlen(text) + 1;
    copy = (char *) HlslLowerAlloc(context, length);
    if (copy != NULL)
        memcpy(copy, text, length);
    return copy;
} // HlslCopyText
