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
NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE OR
USE, ARE DISCLAIMED. IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL,
INDIRECT, INCIDENTAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/
// cg_overload_test.c
//
// Unit tests for CgResolveOverload: profile-selector filtering
// (exact > wildcard > open), ranked argument matching (exact beats
// promotion beats implicit conversion), default-argument accounting,
// ambiguity detection, and the no-mutation guarantee on
// FunSymbol.flags during resolution.
//

#if defined(NDEBUG)
#undef NDEBUG
#endif
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slglobals.h"
#include "cg_overload.h"

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

/*
 * lMakeFun() - Build a standalone function symbol "name" with return
 *              type "ret" and the NULL-terminated formal type list.
 *              The selector defaults to the open profile; callers
 *              refine it afterwards.  Only fields read by the resolver
 *              are populated.
 */

static Symbol *lMakeFun(const char *name, Type *ret, Type *form0, ...)
{
    Symbol *lSymb;
    TypeList **tail;
    va_list forms;

    lSymb = (Symbol *) calloc(1, sizeof(Symbol));
    assert(lSymb);
    lSymb->name = LookUpAddString(atable, name);
    lSymb->kind = FUNCTION_S;
    lSymb->type = (Type *) calloc(1, sizeof(Type));
    assert(lSymb->type);
    lSymb->type->properties = TYPE_CATEGORY_FUNCTION;
    lSymb->type->fun.rettype = ret;
    tail = &lSymb->type->fun.paramtypes;
    if (form0) {
        Type *form = form0;

        va_start(forms, form0);
        while (form) {
            *tail = (TypeList *) calloc(1, sizeof(TypeList));
            assert(*tail);
            (*tail)->type = form;
            tail = &(*tail)->next;
            form = va_arg(forms, Type *);
        }
        va_end(forms);
    }
    /* Unqualified functions carry the open profile selector. */
    lSymb->details.fun.profileSelector.isOpen = 1;
    return lSymb;
} // lMakeFun

/*
 * lChainOverloads() - Link "second" onto "first"'s overload chain and
 *                     return "first", mirroring DeclareFunc ordering.
 */

static Symbol *lChainOverloads(Symbol *first, Symbol *second)
{
    second->details.fun.overload = first->details.fun.overload;
    first->details.fun.overload = second;
    return first;
} // lChainOverloads

/*
 * lMakeArgList() - Build a FUN_ARG_OP chain whose nodes expose one
 *                  argument type each, exactly like ArgumentList().
 */

static expr *lMakeArgList(int count, ...)
{
    expr *head = NULL;
    binary *tail = NULL;
    va_list types;
    int ii;

    va_start(types, count);
    for (ii = 0; ii < count; ii++) {
        binary *node = (binary *) calloc(1, sizeof(binary));

        assert(node);
        node->kind = BINARY_N;
        node->op = FUN_ARG_OP;
        node->type = va_arg(types, Type *);
        if (tail) {
            tail->right = (expr *) node;
        } else {
            head = (expr *) node;
        }
        tail = node;
    }
    va_end(types);
    return head;
} // lMakeArgList

/*
 * lSetExactSelector() / lSetWildcardSelector() - Qualify a candidate.
 */

static void lSetExactSelector(Symbol *fSymb, int profileAtom)
{
    fSymb->details.fun.profileSelector.name = profileAtom;
    fSymb->details.fun.profileSelector.specificity =
        CG_PROFILE_EXACT_SPECIFICITY;
    fSymb->details.fun.profileSelector.isOpen = 0;
} // lSetExactSelector

static void lSetWildcardSelector(Symbol *fSymb, int wildcardAtom,
                                 int specificity)
{
    fSymb->details.fun.profileSelector.name = wildcardAtom;
    fSymb->details.fun.profileSelector.specificity = specificity;
    fSymb->details.fun.profileSelector.isOpen = 0;
} // lSetWildcardSelector

/*
 * lAddFormal() - Append one formal parameter symbol carrying an
 *                optional default-value expression to the function's
 *                ordered formal list.
 */

static void lAddFormal(Symbol *fun, const char *name, Type *type,
                       expr *defaultValue)
{
    Symbol *formal, *last;

    formal = (Symbol *) calloc(1, sizeof(Symbol));
    assert(formal);
    formal->name = LookUpAddString(atable, name);
    formal->kind = VARIABLE_S;
    formal->type = type;
    formal->details.var.init = defaultValue;
    formal->properties |= SYMB_IS_PARAMETER;
    if (!fun->details.fun.params) {
        fun->details.fun.params = formal;
        return;
    }
    last = fun->details.fun.params;
    while (last->next)
        last = last->next;
    last->next = formal;
} // lAddFormal

/*
 * lConstExpr() - A minimal constant expression node whose only job is
 *                to be non-NULL and typed.
 */

static expr *lConstExpr(Type *type)
{
    expr *lExpr = (expr *) calloc(1, sizeof(expr));

    assert(lExpr);
    lExpr->common.kind = CONST_N;
    lExpr->common.type = type;
    lExpr->common.IsConst = 1;
    return lExpr;
} // lConstExpr

/*
 * lMarkFlags() / lAssertFlagsUnchanged() - Every overload candidate is
 *          stamped before resolution; the resolver must never use (or
 *          disturb) FunSymbol.flags as its scratch space.
 */

static void lMarkFlags(Symbol *fSymb)
{
    while (fSymb) {
        fSymb->details.fun.flags = 0xDEAD;
        fSymb = fSymb->details.fun.overload;
    }
} // lMarkFlags

static void lAssertFlagsUnchanged(Symbol *fSymb)
{
    while (fSymb) {
        assert(fSymb->details.fun.flags == 0xDEAD);
        fSymb = fSymb->details.fun.overload;
    }
} // lAssertFlagsUnchanged

int main(void)
{
    CgStruct cg;
    slHAL hal;
    CgProfileIdentity idGlslv, idVertexOther, idFragmentBare, idGeneric;
    int atomGlslv, atomVs, atomPs, atomVp40;
    Type *floatType, *halfType;
    Symbol *pickOpen, *pickVs, *pickGlslv;
    expr *pickArgs;
    CgOverloadResult result;

    memset(&cg, 0, sizeof(cg));
    memset(&hal, 0, sizeof(hal));
    hal.GetSizeof = TestGetSizeof;
    hal.RegisterNames = TestRegisterNames;
    cg.theHAL = &hal;
    Cg = &cg;

    assert(InitAtomTable(atable, 0));
    assert(InitSymbolTable(Cg));

    floatType = GetStandardTypeKind(CG_SCALAR_FLOAT, 0, 0);
    halfType = GetStandardTypeKind(CG_SCALAR_HALF, 0, 0);
    assert(floatType && halfType);

    atomVs = AddAtom(atable, "vs");
    atomPs = AddAtom(atable, "ps");
    atomGlslv = LookUpAddString(atable, "glslv");
    atomVp40 = LookUpAddString(atable, "vp40");
    assert(atomVs && atomPs && atomGlslv && atomVp40);

    /* Target identities.  glslv: vertex stage, exact name plus the vs
     * wildcard.  vp40: a different vertex profile sharing only that
     * wildcard.  glslf-as-target-bare: fragment with no wildcards.
     * generic: neutral stage, no selectors beyond open. */

    idGlslv.exactName = atomGlslv;
    idGlslv.stage = CG_PROFILE_STAGE_VERTEX;
    idGlslv.wildcards = &atomVs;
    idGlslv.specificity = NULL;
    idGlslv.wildcardCount = 1;

    idVertexOther.exactName = atomVp40;
    idVertexOther.stage = CG_PROFILE_STAGE_VERTEX;
    idVertexOther.wildcards = &atomVs;
    idVertexOther.specificity = NULL;
    idVertexOther.wildcardCount = 1;

    idFragmentBare.exactName = atomPs;
    idFragmentBare.stage = CG_PROFILE_STAGE_FRAGMENT;
    idFragmentBare.wildcards = NULL;
    idFragmentBare.specificity = NULL;
    idFragmentBare.wildcardCount = 0;

    idGeneric.exactName = LookUpAddString(atable, "generic");
    idGeneric.stage = CG_PROFILE_STAGE_NEUTRAL;
    idGeneric.wildcards = NULL;
    idGeneric.specificity = NULL;
    idGeneric.wildcardCount = 0;

    /* ---- Case 1: pick(float) declared open, vs, and exact glslv. ---- */

    pickOpen = lMakeFun("pick", floatType, floatType, NULL);
    pickVs = lMakeFun("pick", floatType, floatType, NULL);
    lSetWildcardSelector(pickVs, atomVs, 10);
    pickGlslv = lMakeFun("pick", floatType, floatType, NULL);
    lSetExactSelector(pickGlslv, atomGlslv);
    /* Chain open -> vs -> glslv: every candidate stays reachable from
     * the declaration-order head. */
    pickOpen->details.fun.overload = pickVs;
    pickVs->details.fun.overload = pickGlslv;
    pickGlslv->details.fun.overload = NULL;

    /* Exact glslv wins when compiling for glslv. */
    lMarkFlags(pickOpen);
    memset(&result, 0, sizeof(result));
    pickArgs = lMakeArgList(1, floatType);
    assert(CgResolveOverload(&idGlslv, pickOpen, pickArgs, &result) == 1);
    assert(result.symbol == pickGlslv);
    assert(!result.ambiguous);
    assert(result.usedDefaults == 0);
    lAssertFlagsUnchanged(pickOpen);

    /* Only the shared vs wildcard matches another vertex profile. */
    lMarkFlags(pickOpen);
    memset(&result, 0, sizeof(result));
    assert(CgResolveOverload(&idVertexOther, pickOpen, pickArgs,
                             &result) == 1);
    assert(result.symbol == pickVs);
    assert(!result.ambiguous);
    lAssertFlagsUnchanged(pickOpen);

    /* A profile matching no selector falls through to open. */
    lMarkFlags(pickOpen);
    memset(&result, 0, sizeof(result));
    assert(CgResolveOverload(&idFragmentBare, pickOpen, pickArgs,
                             &result) == 1);
    assert(result.symbol == pickOpen);
    assert(!result.ambiguous);
    lAssertFlagsUnchanged(pickOpen);

    /* The neutral generic profile also lands on open. */
    memset(&result, 0, sizeof(result));
    assert(CgResolveOverload(&idGeneric, pickOpen, pickArgs, &result) == 1);
    assert(result.symbol == pickOpen);

    /* ---- Case 2: an exact half candidate beats a float candidate. ---- */

    {
        Symbol *rankHalf = lMakeFun("shade", halfType, halfType, NULL);
        Symbol *rankFloat = lMakeFun("shade", floatType, floatType, NULL);
        expr *args = lMakeArgList(1, halfType);

        lChainOverloads(rankHalf, rankFloat);
        lMarkFlags(rankHalf);
        memset(&result, 0, sizeof(result));
        assert(CgResolveOverload(&idGeneric, rankHalf, args, &result) == 1);
        assert(result.symbol == rankHalf);
        lAssertFlagsUnchanged(rankHalf);
    }

    /* ---- Case 3: trailing defaults fill unsupplied arguments. ---- */

    {
        Symbol *withDefault = lMakeFun("fill", floatType, floatType,
                                       floatType, NULL);
        expr *oneArg = lMakeArgList(1, floatType);

        lAddFormal(withDefault, "a", floatType, NULL);
        lAddFormal(withDefault, "b", floatType, lConstExpr(floatType));
        lMarkFlags(withDefault);
        memset(&result, 0, sizeof(result));
        assert(CgResolveOverload(&idGeneric, withDefault, oneArg,
                                 &result) == 1);
        assert(result.symbol == withDefault);
        assert(result.usedDefaults == 1);
        lAssertFlagsUnchanged(withDefault);
    }

    /* ---- Case 4: identical conversion ranks stay ambiguous. ---- */

    {
        Symbol *ambFloat = lMakeFun("amb", floatType, floatType, NULL);
        Symbol *ambInt = lMakeFun("amb", floatType,
                                  GetStandardTypeKind(CG_SCALAR_INT, 0, 0),
                                  NULL);
        expr *args = lMakeArgList(1, halfType);

        /* half -> float widens inside the floating family; half -> int
         * crosses classes.  Both are plain implicit conversions of the
         * same tier, so neither candidate can outrank the other. */
        lChainOverloads(ambFloat, ambInt);
        lMarkFlags(ambFloat);
        memset(&result, 0, sizeof(result));
        assert(CgResolveOverload(&idGeneric, ambFloat, args, &result) == 0);
        assert(result.ambiguous);
        assert(result.symbol == NULL);
        lAssertFlagsUnchanged(ambFloat);
    }

    /* ---- Case 5: no viable candidate reports failure cleanly. ---- */

    {
        Symbol *solo = lMakeFun("solo", floatType,
                                GetSamplerType(CG_SAMPLER_2D), NULL);
        expr *args = lMakeArgList(1, floatType);

        /* A sampler formal never binds a scalar actual: not exact, not
         * dynamically compatible, not convertible. */
        lMarkFlags(solo);
        memset(&result, 0, sizeof(result));
        assert(CgResolveOverload(&idGeneric, solo, args, &result) == 0);
        assert(!result.ambiguous);
        assert(result.symbol == NULL);

        /* Arity overflow: more actuals than formals kills the sole
         * candidate during the argument walk; a missing argument with
         * no default does the same after the arity filter. */
        args = lMakeArgList(2, floatType, floatType);
        memset(&result, 0, sizeof(result));
        assert(CgResolveOverload(&idGeneric, solo, args, &result) == 0);
        assert(!result.ambiguous);
        memset(&result, 0, sizeof(result));
        assert(CgResolveOverload(&idGeneric, solo, NULL, &result) == 0);
        assert(!result.ambiguous);
    }

    FreeSymbolTable(Cg);
    FreeAtomTable(atable);
    return 0;
}
