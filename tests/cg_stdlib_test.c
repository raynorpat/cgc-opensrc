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
INCLUDING WITHOUT LIMITATION, WARRANTIES OF CONDITIONS OF TITLE,
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
// cg_stdlib_test.c - Unit tests for the declarative Cg 2.0 standard
//        library catalog: every entry carries a nonempty name, a stable
//        nonzero intrinsic opcode, at least one expanded signature, a
//        unique (name, signature) identity, and consistent flags.  The
//        name audit pins Tables 1-5 of the archived Cg 2.0.0010 User's
//        Manual so the catalog cannot silently drift toward Cg 3.1.
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
#include "language.h"

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

/*
 * TestGetAlignment() - Minimal alignment query mirroring GetAlignment_HAL.
 */

static int TestGetAlignment(Type *fType)
{
    if (!fType)
        return 1;
    return 1;
} // TestGetAlignment

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

//////////////////////////////////// Catalog walk ////////////////////////////////////

#define MAX_TEST_SIGS 4096

typedef struct SigSet_Rec {
    const CgIntrinsicSignature *sigs[MAX_TEST_SIGS];
    int count;
} SigSet;

/*
 * lCollectOverloads() - Follow one symbol's overload chain, collecting
 *          every catalog-backed signature.
 */

static void lCollectOverloads(SigSet *set, Symbol *fSymb)
{
    for (; fSymb; fSymb = fSymb->details.fun.overload) {
        const CgIntrinsicSignature *sig = fSymb->details.fun.intrinsic;

        if (sig != NULL) {
            assert(set->count < MAX_TEST_SIGS);
            set->sigs[set->count++] = sig;
        }
    }
} // lCollectOverloads

/*
 * lCollectFromTree() - Recursively walk a scope's symbol binary tree,
 *          gathering every catalog-backed signature.
 */

static void lCollectFromTree(SigSet *set, Symbol *symb)
{
    if (symb == NULL)
        return;
    lCollectFromTree(set, symb->left);
    if (symb->kind == FUNCTION_S)
        lCollectOverloads(set, symb);
    lCollectFromTree(set, symb->right);
} // lCollectFromTree

/*
 * lCollectCatalog() - Walk a scope's function symbols and gather
 *          every installed catalog signature.
 */

static void lCollectCatalog(SigSet *set, Scope *fScope)
{
    if (fScope == NULL)
        return;
    lCollectFromTree(set, fScope->symbols);
} // lCollectCatalog

static int lSignatureEquals(const CgIntrinsicSignature *a,
                            const CgIntrinsicSignature *b)
{
    TypeList *pa, *pb;

    if (a == b)
        return 1;
    if (a->intrinsic != b->intrinsic || a->result != b->result ||
        strcmp(a->name, b->name))
    {
        return 0;
    }
    pa = a->parameters;
    pb = b->parameters;
    while (pa && pb) {
        if (pa->type != pb->type)
            return 0;
        pa = pa->next;
        pb = pb->next;
    }
    return pa == NULL && pb == NULL;
} // lSignatureEquals

/*
 * lFindName() - Return the first signature of "set" named "name", or
 *          NULL when absent.
 */

static const CgIntrinsicSignature *lFindName(SigSet *set, const char *name)
{
    int i;

    for (i = 0; i < set->count; i++) {
        if (!strcmp(set->sigs[i]->name, name))
            return set->sigs[i];
    }
    return NULL;
} // lFindName

/*
 * lParamCount() - Number of formal parameters of a signature.
 */

static int lParamCount(const CgIntrinsicSignature *sig)
{
    TypeList *param;
    int count = 0;

    for (param = sig->parameters; param; param = param->next)
        count++;
    return count;
} // lParamCount

/*
 * lAssertVector() - "type" is an interned vector of "kind" with the
 *          given length.
 */

static void lAssertVector(Type *type, CgScalarKind kind, int len)
{
    int vlen = 0;

    assert(type != NULL);
    assert(IsVector(type, &vlen));
    assert(vlen == len);
    assert(GetScalarKind(type) == kind);
} // lAssertVector

//////////////////////////////////// Audits ////////////////////////////////////

/*
 * Pinned Tables 1-5 names from the archived Cg 2.0.0010 manual:
 */

static const char *pinnedNames[] = {
    "abs", "acos", "all", "any", "asin", "atan", "atan2", "ceil",
    "clamp", "cos", "cosh", "cross", "degrees",
    "determinant", "dot", "exp", "exp2", "floor", "fmod", "frac",
    "frexp", "isfinite", "isinf", "isnan",
    "ldexp", "lerp", "lit", "log", "log10", "log2", "max", "min",
    "modf", "mul", "noise", "pow", "radians", "round",
    "rsqrt", "saturate", "sign", "sin", "sincos", "sinh", "smoothstep",
    "sqrt", "step", "tan", "tanh",
    "transpose", "distance", "faceforward", "length", "normalize",
    "reflect", "refract", "ddx", "ddy",
    "debug",
    "tex1D", "tex1Dproj", "tex2D", "tex2Dproj", "tex3D", "tex3Dproj",
    "texCUBE", "texCUBEproj", "texRECT", "texRECTproj"
};

static const char *textureNames[] = {
    "tex1D", "tex1Dproj", "tex2D", "tex2Dproj", "tex3D", "tex3Dproj",
    "texCUBE", "texCUBEproj", "texRECT", "texRECTproj"
};

/*
 * Later Cg names that must NOT appear (drift guard):
 */

static const char *forbiddenNames[] = {
    "clip", "fwidth", "tex2Dbias", "tex2Dlod", "tex2Dfetch",
    "tex2Dsize"
};

int main(void)
{
    CgStruct cg;
    slHAL hal;
    SigSet set;
    Scope *scope;
    int counts[CG_INTRINSIC_COUNT];
    int i, j;

    memset(&cg, 0, sizeof(cg));
    memset(&hal, 0, sizeof(hal));
    hal.GetSizeof = TestGetSizeof;
    hal.GetAlignment = TestGetAlignment;
    hal.RegisterNames = TestRegisterNames;
    /* Neutral stage: no profile helper structures on this pass. */
    hal.profileIdentity.stage = CG_PROFILE_STAGE_NEUTRAL;
    cg.theHAL = &hal;
    Cg = &cg;

    assert(InitAtomTable(atable, 0));
    assert(InitSymbolTable(Cg));

    /* ---- Audit 1: every installed entry is well-formed. ---- */

    set.count = 0;
    scope = CurrentScope;
    lCollectCatalog(&set, scope);
    assert(set.count > 0);

    memset(counts, 0, sizeof(counts));
    for (i = 0; i < set.count; i++) {
        const CgIntrinsicSignature *sig = set.sigs[i];

        assert(sig->name != NULL && sig->name[0] != '\0');
        assert(sig->intrinsic > CG_INTRINSIC_NONE &&
               sig->intrinsic < CG_INTRINSIC_COUNT);
        assert(sig->result != NULL && sig->result != UndefinedType);
        counts[sig->intrinsic]++;
    }

    /* Every catalog row expanded at least one signature.  Geometry
     * special rows never expand overload families, so their absence
     * here is the contract, not a defect. */
    for (i = 1; i < CG_INTRINSIC_COUNT; i++) {
        if (counts[i] < 1) {
            if (CgIntrinsicIsGeometrySpecial((CgIntrinsic) i))
                continue;
            printf("row with no signatures: %s\n",
                   CgStdlibCatalogName(i - 1));
            fflush(stdout);
        }
        assert(counts[i] >= 1 ||
               CgIntrinsicIsGeometrySpecial((CgIntrinsic) i));
    }

    /* Unique (name, signature) identity across the whole expansion,
     * and a bijective name-to-opcode mapping: same name always shares
     * one stable opcode, distinct names never collide on one. */
    for (i = 0; i < set.count; i++) {
        for (j = i + 1; j < set.count; j++) {
            if (!strcmp(set.sigs[i]->name, set.sigs[j]->name)) {
                assert(!lSignatureEquals(set.sigs[i], set.sigs[j]));
            } else {
                assert(set.sigs[i]->intrinsic != set.sigs[j]->intrinsic);
            }
        }
    }

    /* ---- Audit 2: pinned Cg 2.0 names are present. ---- */

    assert(CgStdlibCatalogCount() > 0);
    for (i = 0; i < (int) (sizeof(pinnedNames) / sizeof(pinnedNames[0])); i++) {
        int found = 0;

        for (j = 0; j < CgStdlibCatalogCount(); j++) {
            if (!strcmp(CgStdlibCatalogName(j), pinnedNames[i])) {
                found = 1;
                break;
            }
        }
        assert(found);
    }

    /* h4/x4 prefixed forms exist as real signatures with half4 and
     * fixed4 results over the same sampler kinds. */
    for (i = 0; i < (int) (sizeof(textureNames) / sizeof(textureNames[0])); i++) {
        char spelling[32];
        const CgIntrinsicSignature *halfSig;
        const CgIntrinsicSignature *fixedSig;

        sprintf(spelling, "h4%s", textureNames[i]);
        halfSig = lFindName(&set, spelling);
        assert(halfSig != NULL);
        lAssertVector(halfSig->result, CG_SCALAR_HALF, 4);

        sprintf(spelling, "x4%s", textureNames[i]);
        fixedSig = lFindName(&set, spelling);
        assert(fixedSig != NULL);
        lAssertVector(fixedSig->result, CG_SCALAR_FIXED, 4);
    }

    /* Base texture rows carry the texture flag and take a leading
     * canonical sampler parameter. */
    {
        const CgIntrinsicSignature *tex2d = lFindName(&set, "tex2D");

        assert(tex2d != NULL);
        assert(tex2d->flags & CG_INTRINSIC_TEXTURE);
        assert(lParamCount(tex2d) >= 2);
        assert(GetSamplerType(CG_SAMPLER_2D) ==
               tex2d->parameters->type);

        /* The projected form projects through a float4 coordinate. */
        tex2d = lFindName(&set, "tex2Dproj");
        assert(tex2d != NULL);
        assert(lParamCount(tex2d) == 2);
        lAssertVector(tex2d->parameters->next->type, CG_SCALAR_FLOAT, 4);
    }

    /* Derivative, out-parameter, and side-effect identities keep their
     * documented flags. */
    {
        const CgIntrinsicSignature *ddxSig = lFindName(&set, "ddx");
        const CgIntrinsicSignature *sincosSig = lFindName(&set, "sincos");
        const CgIntrinsicSignature *debugSig = lFindName(&set, "debug");
        const CgIntrinsicSignature *frexpSig = lFindName(&set, "frexp");
        const CgIntrinsicSignature *modfSig = lFindName(&set, "modf");
        const CgIntrinsicSignature *absSig = lFindName(&set, "abs");
        const CgIntrinsicSignature *litSig = lFindName(&set, "lit");
        const CgIntrinsicSignature *mulSig = lFindName(&set, "mul");
        const CgIntrinsicSignature *crossSig = lFindName(&set, "cross");

        assert(ddxSig != NULL && (ddxSig->flags & CG_INTRINSIC_DERIVATIVE));
        assert(sincosSig != NULL &&
               (sincosSig->flags & CG_INTRINSIC_OUT_PARAMS));
        assert(frexpSig != NULL &&
               (frexpSig->flags & CG_INTRINSIC_OUT_PARAMS));
        assert(modfSig != NULL &&
               (modfSig->flags & CG_INTRINSIC_OUT_PARAMS));
        assert(debugSig != NULL &&
               (debugSig->flags & CG_INTRINSIC_SIDE_EFFECTS));
        assert(absSig != NULL && (absSig->flags & CG_INTRINSIC_FOLDABLE));
        assert(litSig != NULL);
        assert(mulSig != NULL && crossSig != NULL);

        /* sincos(x, out s, out c): the out-parameters are marked OUT. */
        assert(lParamCount(sincosSig) == 3);
        assert((sincosSig->parameters->next->type->properties &
                TYPE_QUALIFIER_OUT));
        assert((sincosSig->parameters->next->next->type->properties &
                TYPE_QUALIFIER_OUT));

        /* debug takes one float4 argument and returns nothing. */
        assert(IsVoid(debugSig->result));
        assert(lParamCount(debugSig) == 1);
        lAssertVector(debugSig->parameters->type, CG_SCALAR_FLOAT, 4);

        /* cross is float3 x float3 -> float3. */
        lAssertVector(crossSig->result, CG_SCALAR_FLOAT, 3);
        lAssertVector(crossSig->parameters->type, CG_SCALAR_FLOAT, 3);

        /* Integer/bool families: abs(int), sign(int), all/any(bool). */
        {
            const CgIntrinsicSignature *absInt = NULL;
            const CgIntrinsicSignature *allBool = NULL;
            const CgIntrinsicSignature *anyBool = NULL;
            const CgIntrinsicSignature *allBool3 = NULL;
            const CgIntrinsicSignature *anyBool3 = NULL;

            for (i = 0; i < set.count; i++) {
                const CgIntrinsicSignature *sig = set.sigs[i];

                if (!strcmp(sig->name, "abs") &&
                    GetScalarKind(sig->parameters->type) == CG_SCALAR_INT &&
                    IsScalar(sig->parameters->type))
                {
                    absInt = sig;
                }
                if (!strcmp(sig->name, "all") && IsScalar(sig->result))
                    allBool = sig;
                if (!strcmp(sig->name, "any") && IsScalar(sig->result))
                    anyBool = sig;
                if (!strcmp(sig->name, "all") && IsScalar(sig->result) &&
                    IsVector(sig->parameters->type, NULL) &&
                    sig->parameters->type->arr.numels == 3)
                {
                    allBool3 = sig;
                }
                if (!strcmp(sig->name, "any") && IsScalar(sig->result) &&
                    IsVector(sig->parameters->type, NULL) &&
                    sig->parameters->type->arr.numels == 3)
                {
                    anyBool3 = sig;
                }
            }
            assert(absInt != NULL);
            assert(allBool != NULL);
            assert(anyBool != NULL);
            assert(allBool3 != NULL);
            assert(anyBool3 != NULL);
        }
    }

    /* ---- Audit 3: later Cg names are absent. ---- */

    for (i = 0; i < CgStdlibCatalogCount(); i++) {
        const char *name = CgStdlibCatalogName(i);

        for (j = 0;
             j < (int) (sizeof(forbiddenNames) / sizeof(forbiddenNames[0]));
             j++)
        {
            assert(strcmp(name, forbiddenNames[j]));
        }
    }

    /* ---- Audit 4: helper structures are profile-qualified rows. ---- */

    assert(CgStdlibHelperCount() == 2);
    {
        int sawFragout = 0;
        int sawFragoutFloat = 0;

        for (i = 0; i < CgStdlibHelperCount(); i++) {
            const char *name = CgStdlibHelperName(i);
            CgProfileStage stage = CgStdlibHelperStage(i);

            assert(stage == CG_PROFILE_STAGE_FRAGMENT);
            if (!strcmp(name, "fragout"))
                sawFragout = 1;
            if (!strcmp(name, "fragout_float"))
                sawFragoutFloat = 1;
        }
        assert(sawFragout && sawFragoutFloat);
    }

    /* Neutral profiles install neither helper structure... */
    {
        Scope *neutralScope = NewScopeInPool(mem_CreatePool(0, 0));

        hal.profileIdentity.stage = CG_PROFILE_STAGE_NEUTRAL;
        assert(InitCgStdlib(neutralScope));
        assert(LookUpLocalSymbol(neutralScope,
                                 LookUpAddString(atable, "abs")) != NULL);
        assert(LookUpLocalSymbol(neutralScope,
                                 LookUpAddString(atable, "fragout")) == NULL);
        assert(LookUpLocalSymbol(
                   neutralScope,
                   LookUpAddString(atable, "fragout_float")) == NULL);
    }

    /* ...while the fragment family installs both. */
    {
        Scope *fragmentScope = NewScopeInPool(mem_CreatePool(0, 0));
        Symbol *fragout;

        hal.profileIdentity.stage = CG_PROFILE_STAGE_FRAGMENT;
        assert(InitCgStdlib(fragmentScope));
        fragout = LookUpLocalSymbol(fragmentScope,
                                    LookUpAddString(atable, "fragout"));
        assert(fragout != NULL);
        assert(fragout->kind == TYPEDEF_S);
        assert(IsCategory(fragout->type, TYPE_CATEGORY_STRUCT));
        assert(fragout->type->str.HasSemantics);
        fragout = LookUpLocalSymbol(fragmentScope,
                                    LookUpAddString(atable, "fragout_float"));
        assert(fragout != NULL);
        assert(IsCategory(fragout->type, TYPE_CATEGORY_STRUCT));
    }

    /* ---- Audit 5: geometry special identities. ---- */

    /* Name lookup spans ordinary and special rows, is exact, and
     * misses cleanly. */
    assert(CgFindIntrinsicByName("abs") == CG_INTRINSIC_ABS);
    assert(CgFindIntrinsicByName("emitVertex") ==
           CG_INTRINSIC_EMIT_VERTEX);
    assert(CgFindIntrinsicByName("flatAttrib") ==
           CG_INTRINSIC_FLAT_ATTRIB);
    assert(CgFindIntrinsicByName("restartStrip") ==
           CG_INTRINSIC_RESTART_STRIP);
    assert(CgFindIntrinsicByName("emitvertex") == CG_INTRINSIC_NONE);
    assert(CgFindIntrinsicByName("nosuch") == CG_INTRINSIC_NONE);
    assert(CgFindIntrinsicByName("") == CG_INTRINSIC_NONE);

    /* The geometry flag sits exactly on the three special identities. */
    assert(CgIntrinsicIsGeometrySpecial(CG_INTRINSIC_EMIT_VERTEX));
    assert(CgIntrinsicIsGeometrySpecial(CG_INTRINSIC_FLAT_ATTRIB));
    assert(CgIntrinsicIsGeometrySpecial(CG_INTRINSIC_RESTART_STRIP));
    assert(!CgIntrinsicIsGeometrySpecial(CG_INTRINSIC_ABS));
    assert(!CgIntrinsicIsGeometrySpecial(CG_INTRINSIC_DEBUG));
    assert(!CgIntrinsicIsGeometrySpecial(CG_INTRINSIC_NONE));

    /* Cg 2.0 installs one reserved symbol per special identity with a
     * minimal void signature; explicit Cg 1.1 installs none. */
    {
        Scope *geomScope = NewScopeInPool(mem_CreatePool(0, 0));
        Symbol *op;

        cg.options.languageVersion = CG_LANGUAGE_2_0;
        hal.profileIdentity.stage = CG_PROFILE_STAGE_NEUTRAL;
        assert(InitCgStdlib(geomScope));
        op = LookUpLocalSymbol(geomScope,
                               LookUpAddString(atable, "emitVertex"));
        assert(op != NULL && op->kind == FUNCTION_S);
        assert(op->details.fun.intrinsic != NULL);
        assert(op->details.fun.intrinsic->intrinsic ==
               CG_INTRINSIC_EMIT_VERTEX);
        assert((op->details.fun.intrinsic->flags &
                CG_INTRINSIC_FLAG_GEOMETRY) != 0);
        assert(IsVoid(op->details.fun.intrinsic->result));
        assert(op->details.fun.intrinsic->parameters == NULL);
        assert(LookUpLocalSymbol(
                   geomScope,
                   LookUpAddString(atable, "flatAttrib")) != NULL);
        assert(LookUpLocalSymbol(
                   geomScope,
                   LookUpAddString(atable, "restartStrip")) != NULL);
        assert(LookUpLocalSymbol(geomScope,
                                 LookUpAddString(atable, "abs")) != NULL);

        cg.options.languageVersion = CG_LANGUAGE_1_1;
        {
            Scope *legacyScope = NewScopeInPool(mem_CreatePool(0, 0));

            assert(InitCgStdlib(legacyScope));
            assert(LookUpLocalSymbol(
                       legacyScope,
                       LookUpAddString(atable, "emitVertex")) == NULL);
            assert(LookUpLocalSymbol(
                       legacyScope,
                       LookUpAddString(atable, "flatAttrib")) == NULL);
            assert(LookUpLocalSymbol(
                       legacyScope,
                       LookUpAddString(atable, "restartStrip")) == NULL);
            assert(LookUpLocalSymbol(
                       legacyScope,
                       LookUpAddString(atable, "abs")) != NULL);
        }
        (void) 0;
    }

    FreeSymbolTable(Cg);
    FreeAtomTable(atable);
    return 0;
}
