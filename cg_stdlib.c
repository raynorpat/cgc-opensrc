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
// cg_stdlib.c - Expansion and installation of the declarative Cg 2.0
//        intrinsic catalog.  Every cg_stdlib.def row names one stable
//        intrinsic identity; the builders below expand each identity
//        into its documented overload family over the canonical scalar
//        kinds (fixed/half/float), vector lengths, matrix shapes, and
//        documented integer/bool forms, then install the signatures as
//        ordinary internal function symbols whose FunSymbol carries the
//        immutable CgIntrinsicSignature pointer.
//

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slglobals.h"
#include "cg_stdlib.h"
#include "language.h"

////////////////////////////// Catalog name table //////////////////////////////

static const char *catalogNames[] = {
#define CG_INTRINSIC(id, name, flags) name,
#define CG_STDLIB_SPECIAL(intrinsic, name, flags) name,
#include "cg_stdlib.def"
#undef CG_STDLIB_SPECIAL
#undef CG_INTRINSIC
};

#define CATALOG_NAME_COUNT \
    ((int) (sizeof(catalogNames) / sizeof(catalogNames[0])))

////////////////////////////// Signature storage ///////////////////////////////

/*
 * Signatures are built once per process and then treated as immutable;
 * repeated installations over different scopes reuse the same records
 * so identity stays pointer-stable everywhere.
 */

static CgIntrinsicSignature *catalogSigs = NULL;
static int catalogSigCount = 0;
static int catalogSigCap = 0;
static int catalogBuilt = 0;

/*
 * lParamList() - Allocate one immutable parameter-type chain for an
 *          array of "count" types.
 */

static TypeList *lParamList(const Type * const *types, int count)
{
    TypeList *head = NULL;
    TypeList **tail = &head;
    int i;

    for (i = 0; i < count; i++) {
        TypeList *node = (TypeList *) malloc(sizeof(TypeList));

        assert(node);
        node->next = NULL;
        node->type = (Type *) types[i];
        *tail = node;
        tail = &node->next;
    }
    return head;
} // lParamList

/*
 * lAddNamedSignature() - Append one expanded signature to the catalog.
 */

static void lAddNamedSignature(CgIntrinsic intrinsic, const char *name,
                               Type *result, const Type * const *types,
                               int count, unsigned flags)
{
    CgIntrinsicSignature *sig;

    assert(intrinsic > CG_INTRINSIC_NONE &&
           intrinsic < CG_INTRINSIC_COUNT);
    assert(result != NULL && result != UndefinedType);
    if (catalogSigCount == catalogSigCap) {
        /* The catalog outlives any scope pool, so it grows with the
         * raw allocator; parenthesization bypasses memory.h's
         * CurrentScope-pool macros. */
        int newCap = catalogSigCap ? catalogSigCap * 2 : 256;
        CgIntrinsicSignature *grown = (CgIntrinsicSignature *)
            (malloc)(newCap * sizeof(CgIntrinsicSignature));

        assert(grown);
        if (catalogSigs != NULL) {
            memcpy(grown, catalogSigs,
                   catalogSigCount * sizeof(CgIntrinsicSignature));
            (free)(catalogSigs);
        }
        catalogSigs = grown;
        catalogSigCap = newCap;
    }
    sig = &catalogSigs[catalogSigCount++];
    sig->intrinsic = intrinsic;
    sig->name = name;
    sig->result = result;
    sig->parameters = lParamList(types, count);
    sig->flags = flags;
} // lAddNamedSignature

/*
 * lAddSignature() - The common form: the signature carries its own
 *          catalog-row spelling.
 */

static void lAddSignature(CgIntrinsic intrinsic, Type *result,
                          const Type * const *types, int count,
                          unsigned flags)
{
    lAddNamedSignature(intrinsic, catalogNames[intrinsic - 1], result,
                       types, count, flags);
} // lAddSignature

////////////////////////////// Type helpers ////////////////////////////////////

/*
 * The floating family every ordinary numeric family expands over:
 */

static const CgScalarKind floatKinds[] = {
    CG_SCALAR_FIXED, CG_SCALAR_HALF, CG_SCALAR_FLOAT
};

#define FLOAT_KIND_COUNT \
    ((int) (sizeof(floatKinds) / sizeof(floatKinds[0])))

#define FLTF (CG_INTRINSIC_PURE | CG_INTRINSIC_FOLDABLE)

static const CgScalarKind intKind = CG_SCALAR_INT;

/*
 * lKindShape() - The interned scalar (len 0) or packed vector shape.
 */

static Type *lKindShape(CgScalarKind kind, int len)
{
    return GetStandardTypeKind(kind, len, 0);
} // lKindShape

/*
 * lQualified() - A shallow copy of "type" carrying extra qualifier
 *          bits; used for out-parameter formals of interned shapes.
 */

static Type *lQualified(Type *type, int quals)
{
    Type *copy;

    if (!quals)
        return type;
    copy = DupType(type);
    copy->co.properties |= quals;
    return copy;
} // lQualified

////////////////////////////// Family expanders ////////////////////////////////

/*
 * Shapes follow the standard library's replication rule: vector
 * operands share one length, and any remaining operand may be a scalar
 * instead.  Length 0 denotes the scalar itself.
 */

static void lExpandUnary(CgIntrinsic id, unsigned flags,
                         const CgScalarKind *kinds, int nkinds)
{
    Type *types[1];
    int k, len;

    for (k = 0; k < nkinds; k++) {
        for (len = 0; len <= 4; len++) {
            types[0] = lKindShape(kinds[k], len);
            lAddSignature(id, types[0], types, 1, flags);
        }
    }
} // lExpandUnary

/*
 * lExpandUnaryTo() - Unary family whose result kind differs from the
 *          argument kind (the boolean classification predicates).
 */

static void lExpandUnaryTo(CgIntrinsic id, unsigned flags,
                           const CgScalarKind *kinds, int nkinds,
                           CgScalarKind resultKind)
{
    Type *types[1];
    int k, len;

    for (k = 0; k < nkinds; k++) {
        for (len = 0; len <= 4; len++) {
            types[0] = lKindShape(kinds[k], len);
            lAddSignature(id, lKindShape(resultKind, len), types, 1,
                          flags);
        }
    }
} // lExpandUnaryTo

/*
 * lBuildBoolReductions() - all/any reduce every admitted boolean shape to
 *          one scalar boolean.  They are not component-wise unary families.
 */

static void lBuildBoolReductions(void)
{
    Type *types[1];
    Type *result;
    int len;

    result = lKindShape(CG_SCALAR_BOOL, 0);
    for (len = 0; len <= 4; len++) {
        types[0] = lKindShape(CG_SCALAR_BOOL, len);
        lAddSignature(CG_INTRINSIC_ALL, result, types, 1, FLTF);
        lAddSignature(CG_INTRINSIC_ANY, result, types, 1, FLTF);
    }
} // lBuildBoolReductions

static void lExpandBinaryReplicated(CgIntrinsic id, unsigned flags,
                                    const CgScalarKind *kinds, int nkinds)
{
    Type *types[2];
    int k, len;

    for (k = 0; k < nkinds; k++) {
        /* (scalar, scalar) */
        types[0] = lKindShape(kinds[k], 0);
        types[1] = lKindShape(kinds[k], 0);
        lAddSignature(id, types[0], types, 2, flags);
        for (len = 1; len <= 4; len++) {
            /* (vector, vector), (scalar, vector), (vector, scalar) */
            types[1] = lKindShape(kinds[k], len);
            types[0] = lKindShape(kinds[k], len);
            lAddSignature(id, types[0], types, 2, flags);
            types[0] = lKindShape(kinds[k], 0);
            lAddSignature(id, types[1], types, 2, flags);
            types[0] = lKindShape(kinds[k], len);
            types[1] = lKindShape(kinds[k], 0);
            lAddSignature(id, types[0], types, 2, flags);
        }
    }
} // lExpandBinaryReplicated

static void lExpandTernaryReplicated(CgIntrinsic id, unsigned flags,
                                     const CgScalarKind *kinds, int nkinds)
{
    Type *types[3];
    int k, pattern, len;

    for (k = 0; k < nkinds; k++) {
        /* Each operand is either the scalar or a length-L vector; the
         * result takes the widest participating shape. */
        for (pattern = 0; pattern < 8; pattern++) {
            int v0 = pattern & 1;
            int v1 = pattern & 2;
            int v2 = pattern & 4;
            int hasVector = v0 || v1 || v2;

            if (!hasVector) {
                types[0] = lKindShape(kinds[k], 0);
                types[1] = lKindShape(kinds[k], 0);
                types[2] = lKindShape(kinds[k], 0);
                lAddSignature(id, types[0], types, 3, flags);
                continue;
            }
            for (len = 1; len <= 4; len++) {
                Type *vec = lKindShape(kinds[k], len);

                types[0] = v0 ? vec : lKindShape(kinds[k], 0);
                types[1] = v1 ? vec : lKindShape(kinds[k], 0);
                types[2] = v2 ? vec : lKindShape(kinds[k], 0);
                lAddSignature(id, vec, types, 3, flags);
            }
        }
    }
} // lExpandTernaryReplicated

/*
 * lExpandOutPair() - x paired with an out-parameter of the same shape
 *          and kind (frexp, modf).
 */

static void lExpandOutPair(CgIntrinsic id, unsigned flags,
                           const CgScalarKind *kinds, int nkinds)
{
    Type *types[2];
    int k, len;

    for (k = 0; k < nkinds; k++) {
        for (len = 0; len <= 4; len++) {
            types[0] = lKindShape(kinds[k], len);
            types[1] = lQualified(lKindShape(kinds[k], len),
                                  TYPE_QUALIFIER_OUT);
            lAddSignature(id, types[0], types, 2, flags);
        }
    }
} // lExpandOutPair

////////////////////////////// Explicit builders ///////////////////////////////

static void lBuildDot(void)
{
    Type *types[2];
    int k, len;

    for (k = 0; k < FLOAT_KIND_COUNT; k++) {
        for (len = 0; len <= 4; len++) {
            types[0] = lKindShape(floatKinds[k], len);
            types[1] = lKindShape(floatKinds[k], len);
            lAddSignature(CG_INTRINSIC_DOT, lKindShape(floatKinds[k], 0),
                          types, 2, FLTF);
        }
    }
} // lBuildDot

/*
 * lBuildLdexp() - ldexp(x, n) over matching float and int shapes.
 */

static void lBuildLdexp(void)
{
    Type *types[2];
    int k, len;

    for (k = 0; k < FLOAT_KIND_COUNT; k++) {
        for (len = 0; len <= 4; len++) {
            types[0] = lKindShape(floatKinds[k], len);
            types[1] = lKindShape(intKind, len);
            lAddSignature(CG_INTRINSIC_LDEXP, types[0], types, 2, FLTF);
        }
    }
} // lBuildLdexp

/*
 * lBuildVectorOps() - length/distance reduce any float shape to a
 *          float scalar; normalize keeps its argument's shape and
 *          reflect follows the replicated binary rule.
 */

static void lBuildVectorOps(void)
{
    Type *types[2];
    int k, len;

    for (k = 0; k < FLOAT_KIND_COUNT; k++) {
        for (len = 0; len <= 4; len++) {
            types[0] = lKindShape(floatKinds[k], len);
            lAddSignature(CG_INTRINSIC_LENGTH,
                          lKindShape(floatKinds[k], 0), types, 1, FLTF);
            types[1] = lKindShape(floatKinds[k], len);
            lAddSignature(CG_INTRINSIC_DISTANCE,
                          lKindShape(floatKinds[k], 0), types, 2, FLTF);
            lAddSignature(CG_INTRINSIC_NORMALIZE, types[0], types, 1,
                          CG_INTRINSIC_PURE);
            if (len > 0) {
                /* reflect(I, N): (vL, vL), (vL, s), (s, vL). */
                lAddSignature(CG_INTRINSIC_REFLECT, types[0], types, 2,
                              CG_INTRINSIC_PURE);
                types[0] = lKindShape(floatKinds[k], 0);
                lAddSignature(CG_INTRINSIC_REFLECT, types[1], types, 2,
                              CG_INTRINSIC_PURE);
                types[0] = lKindShape(floatKinds[k], len);
                types[1] = lKindShape(floatKinds[k], 0);
                lAddSignature(CG_INTRINSIC_REFLECT, types[0], types, 2,
                              CG_INTRINSIC_PURE);
            } else {
                types[0] = lKindShape(floatKinds[k], 0);
                types[1] = lKindShape(floatKinds[k], 0);
                lAddSignature(CG_INTRINSIC_REFLECT, types[0], types, 2,
                              CG_INTRINSIC_PURE);
            }
        }
    }
} // lBuildVectorOps

/*
 * lBuildMul() - Every documented matrix product: matrix*matrix where
 *          inner dimensions agree, matrix*vector, vector*matrix, and
 *          the scalar product.  Shape (rows, columns) follows the
 *          interned registry layout: an array of rows of column-wide
 *          packed rows, matching the floatRxC spellings.
 */

static void lBuildMul(void)
{
    Type *types[2];
    int r, c, inner;

    types[0] = lKindShape(CG_SCALAR_FLOAT, 0);
    types[1] = lKindShape(CG_SCALAR_FLOAT, 0);
    lAddSignature(CG_INTRINSIC_MUL, types[0], types, 2, FLTF);

    for (r = 1; r <= 4; r++) {
        for (c = 1; c <= 4; c++) {
            Type *matrix = GetStandardTypeKind(CG_SCALAR_FLOAT, r, c);

            /* mul(matrix(r,c), vector(c)) -> vector(r) */
            types[0] = matrix;
            types[1] = lKindShape(CG_SCALAR_FLOAT, c);
            lAddSignature(CG_INTRINSIC_MUL,
                          lKindShape(CG_SCALAR_FLOAT, r), types, 2, FLTF);

            /* mul(vector(r), matrix(r,c)) -> vector(c) */
            types[0] = lKindShape(CG_SCALAR_FLOAT, r);
            types[1] = matrix;
            lAddSignature(CG_INTRINSIC_MUL,
                          lKindShape(CG_SCALAR_FLOAT, c), types, 2, FLTF);

            /* mul(matrix(r,inner), matrix(inner,c)) -> matrix(r,c) */
            for (inner = 1; inner <= 4; inner++) {
                types[0] = GetStandardTypeKind(CG_SCALAR_FLOAT, r, inner);
                types[1] = GetStandardTypeKind(CG_SCALAR_FLOAT, inner, c);
                lAddSignature(CG_INTRINSIC_MUL, matrix, types, 2, FLTF);
            }
        }
    }
} // lBuildMul

/*
 * lBuildMatrixOps() - transpose over all sixteen float shapes and
 *          determinant over the squares from 2x2 up.
 */

static void lBuildMatrixOps(void)
{
    Type *types[1];
    int rows, cols;

    for (rows = 1; rows <= 4; rows++) {
        for (cols = 1; cols <= 4; cols++) {
            types[0] = GetStandardTypeKind(CG_SCALAR_FLOAT, rows, cols);
            lAddSignature(CG_INTRINSIC_TRANSPOSE,
                          GetStandardTypeKind(CG_SCALAR_FLOAT, cols, rows),
                          types, 1, CG_INTRINSIC_PURE | CG_INTRINSIC_FOLDABLE);
            if (rows == cols && rows >= 2) {
                lAddSignature(CG_INTRINSIC_DETERMINANT,
                              lKindShape(CG_SCALAR_FLOAT, 0), types, 1,
                              CG_INTRINSIC_PURE | CG_INTRINSIC_FOLDABLE);
            }
        }
    }
} // lBuildMatrixOps

/*
 * lBuildGeometry() - cross, lit, noise, and refract have exactly their
 *          documented shapes.
 */

static void lBuildGeometry(void)
{
    Type *types[3];
    int k, len;

    /* cross(float3, float3) -> float3 */
    types[0] = lKindShape(CG_SCALAR_FLOAT, 3);
    types[1] = lKindShape(CG_SCALAR_FLOAT, 3);
    lAddSignature(CG_INTRINSIC_CROSS, types[0], types, 2,
                  CG_INTRINSIC_PURE);

    /* lit(float ndotl, float ndoth, float m) -> float4 */
    types[0] = lKindShape(CG_SCALAR_FLOAT, 0);
    types[1] = lKindShape(CG_SCALAR_FLOAT, 0);
    types[2] = lKindShape(CG_SCALAR_FLOAT, 0);
    lAddSignature(CG_INTRINSIC_LIT, lKindShape(CG_SCALAR_FLOAT, 4),
                  types, 3, FLTF);

    /* noise(p) over the documented float2/float3 domains */
    types[0] = lKindShape(CG_SCALAR_FLOAT, 2);
    lAddSignature(CG_INTRINSIC_NOISE, lKindShape(CG_SCALAR_FLOAT, 0),
                  types, 1, CG_INTRINSIC_PURE);
    types[0] = lKindShape(CG_SCALAR_FLOAT, 3);
    lAddSignature(CG_INTRINSIC_NOISE, lKindShape(CG_SCALAR_FLOAT, 0),
                  types, 1, CG_INTRINSIC_PURE);

    /* refract(I, N, eta): incident and normal share one shape, eta is
     * always a scalar. */
    for (k = 0; k < FLOAT_KIND_COUNT; k++) {
        for (len = 0; len <= 4; len++) {
            types[0] = lKindShape(floatKinds[k], len);
            types[1] = lKindShape(floatKinds[k], len);
            types[2] = lKindShape(CG_SCALAR_FLOAT, 0);
            lAddSignature(CG_INTRINSIC_REFRACT, types[0], types, 3,
                          CG_INTRINSIC_PURE);
        }
    }
} // lBuildGeometry

/*
 * lBuildDerivatives() - ddx/ddy are pure derivative instructions over
 *          the float family.
 */

static void lBuildDerivatives(void)
{
    lExpandUnary(CG_INTRINSIC_DDX, CG_INTRINSIC_DERIVATIVE,
                 floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_DDY, CG_INTRINSIC_DERIVATIVE,
                 floatKinds, FLOAT_KIND_COUNT);
} // lBuildDerivatives

/*
 * lBuildSincos() - sincos(x, out s, out c) over the scalar, the fully
 *          replicated vector, and the vector-with-scalar-outs forms.
 */

static void lBuildSincos(void)
{
    Type *types[3];
    unsigned flags = CG_INTRINSIC_OUT_PARAMS;
    int k, len;

    for (k = 0; k < FLOAT_KIND_COUNT; k++) {
        /* (x, out s, out c) all scalars. */
        types[0] = lKindShape(floatKinds[k], 0);
        types[1] = lQualified(types[0], TYPE_QUALIFIER_OUT);
        types[2] = lQualified(types[0], TYPE_QUALIFIER_OUT);
        lAddSignature(CG_INTRINSIC_SINCOS, VoidType, types, 3, flags);
        for (len = 1; len <= 4; len++) {
            /* (vector, out vector, out vector) */
            types[0] = lKindShape(floatKinds[k], len);
            types[1] = lQualified(types[0], TYPE_QUALIFIER_OUT);
            types[2] = lQualified(types[0], TYPE_QUALIFIER_OUT);
            lAddSignature(CG_INTRINSIC_SINCOS, VoidType, types, 3, flags);
            /* (vector, out scalar, out scalar) */
            types[1] = lQualified(lKindShape(floatKinds[k], 0),
                                  TYPE_QUALIFIER_OUT);
            types[2] = types[1];
            lAddSignature(CG_INTRINSIC_SINCOS, VoidType, types, 3, flags);
        }
    }
} // lBuildSincos

static void lBuildDebug(void)
{
    Type *types[1];

    types[0] = lKindShape(CG_SCALAR_FLOAT, 4);
    lAddSignature(CG_INTRINSIC_DEBUG, VoidType, types, 1,
                  CG_INTRINSIC_SIDE_EFFECTS);
} // lBuildDebug

/*
 * Texture families: base coordinate forms plus an explicit-gradient
 * overload under the base identity, projected forms through a float4
 * q-coordinate, and the half4/fixed4 h4/x4 identities generated from
 * the same rows with typed results.
 */

typedef struct TexFamily_Rec {
    CgIntrinsic id;
    CgIntrinsic projId;
    CgIntrinsic h4Id;
    CgIntrinsic x4Id;
    CgSamplerKind sampler;
    int coordLen;
} TexFamily;

static const TexFamily texFamilies[] = {
    /* coordLen 0 denotes the scalar coordinate documented for tex1D. */
    { CG_INTRINSIC_TEX1D,       CG_INTRINSIC_TEX1DPROJ,
      CG_INTRINSIC_H4TEX1D,     CG_INTRINSIC_X4TEX1D,
      CG_SAMPLER_1D,   0 },
    { CG_INTRINSIC_TEX2D,       CG_INTRINSIC_TEX2DPROJ,
      CG_INTRINSIC_H4TEX2D,     CG_INTRINSIC_X4TEX2D,
      CG_SAMPLER_2D,   2 },
    { CG_INTRINSIC_TEX3D,       CG_INTRINSIC_TEX3DPROJ,
      CG_INTRINSIC_H4TEX3D,     CG_INTRINSIC_X4TEX3D,
      CG_SAMPLER_3D,   3 },
    { CG_INTRINSIC_TEXCUBE,     CG_INTRINSIC_TEXCUBEPROJ,
      CG_INTRINSIC_H4TEXCUBE,   CG_INTRINSIC_X4TEXCUBE,
      CG_SAMPLER_CUBE, 3 },
    { CG_INTRINSIC_TEXRECT,     CG_INTRINSIC_TEXRECTPROJ,
      CG_INTRINSIC_H4TEXRECT,   CG_INTRINSIC_X4TEXRECT,
      CG_SAMPLER_RECT, 2 },
};

#define TEX_FAMILY_COUNT \
    ((int) (sizeof(texFamilies) / sizeof(texFamilies[0])))

static const TexFamily projTexFamilies[] = {
    { CG_INTRINSIC_H4TEX1DPROJ,  CG_INTRINSIC_NONE,
      CG_INTRINSIC_NONE,         CG_INTRINSIC_NONE,
      CG_SAMPLER_1D,   4 },
    { CG_INTRINSIC_X4TEX1DPROJ,  CG_INTRINSIC_NONE,
      CG_INTRINSIC_NONE,         CG_INTRINSIC_NONE,
      CG_SAMPLER_1D,   4 },
    { CG_INTRINSIC_H4TEX2DPROJ,  CG_INTRINSIC_NONE,
      CG_INTRINSIC_NONE,         CG_INTRINSIC_NONE,
      CG_SAMPLER_2D,   4 },
    { CG_INTRINSIC_X4TEX2DPROJ,  CG_INTRINSIC_NONE,
      CG_INTRINSIC_NONE,         CG_INTRINSIC_NONE,
      CG_SAMPLER_2D,   4 },
    { CG_INTRINSIC_H4TEX3DPROJ,  CG_INTRINSIC_NONE,
      CG_INTRINSIC_NONE,         CG_INTRINSIC_NONE,
      CG_SAMPLER_3D,   4 },
    { CG_INTRINSIC_X4TEX3DPROJ,  CG_INTRINSIC_NONE,
      CG_INTRINSIC_NONE,         CG_INTRINSIC_NONE,
      CG_SAMPLER_3D,   4 },
    { CG_INTRINSIC_H4TEXCUBEPROJ, CG_INTRINSIC_NONE,
      CG_INTRINSIC_NONE,          CG_INTRINSIC_NONE,
      CG_SAMPLER_CUBE, 4 },
    { CG_INTRINSIC_X4TEXCUBEPROJ, CG_INTRINSIC_NONE,
      CG_INTRINSIC_NONE,          CG_INTRINSIC_NONE,
      CG_SAMPLER_CUBE, 4 },
    { CG_INTRINSIC_H4TEXRECTPROJ, CG_INTRINSIC_NONE,
      CG_INTRINSIC_NONE,          CG_INTRINSIC_NONE,
      CG_SAMPLER_RECT, 4 },
    { CG_INTRINSIC_X4TEXRECTPROJ, CG_INTRINSIC_NONE,
      CG_INTRINSIC_NONE,          CG_INTRINSIC_NONE,
      CG_SAMPLER_RECT, 4 },
};

#define PROJ_TEX_FAMILY_COUNT \
    ((int) (sizeof(projTexFamilies) / sizeof(projTexFamilies[0])))

static void lBuildBaseTextureFamily(const TexFamily *family)
{
    const Type *types[4];
    Type *sampler = GetSamplerType(family->sampler);
    Type *coord = lKindShape(CG_SCALAR_FLOAT, family->coordLen);
    Type *result = lKindShape(CG_SCALAR_FLOAT, 4);

    /* Base coordinate fetch. */
    types[0] = sampler;
    types[1] = coord;
    lAddSignature(family->id, result, types, 2, CG_INTRINSIC_TEXTURE);

    /* Same row carries the explicit-gradient form. */
    types[2] = coord;
    types[3] = coord;
    lAddSignature(family->id, result, types, 4, CG_INTRINSIC_TEXTURE);

    /* Projection through a float4 q-coordinate. */
    types[0] = sampler;
    types[1] = lKindShape(CG_SCALAR_FLOAT, 4);
    lAddSignature(family->projId, result, types, 2, CG_INTRINSIC_TEXTURE);
} // lBuildBaseTextureFamily

static void lBuildTypedTextureFamily(const TexFamily *family,
                                     CgScalarKind kind)
{
    Type *types[2];
    CgIntrinsic id;
    Type *result = lKindShape(kind, 4);

    id = kind == CG_SCALAR_HALF ? family->h4Id : family->x4Id;
    types[0] = GetSamplerType(family->sampler);
    types[1] = lKindShape(CG_SCALAR_FLOAT, family->coordLen);
    lAddSignature(id, result, types, 2, CG_INTRINSIC_TEXTURE);
} // lBuildTypedTextureFamily

static void lBuildProjectedTextureFamily(const TexFamily *family,
                                         CgScalarKind kind)
{
    Type *types[2];
    Type *result = lKindShape(kind, 4);

    types[0] = GetSamplerType(family->sampler);
    types[1] = lKindShape(CG_SCALAR_FLOAT, 4);
    lAddSignature(family->id, result, types, 2, CG_INTRINSIC_TEXTURE);
} // lBuildProjectedTextureFamily

////////////////////////////// Catalog assembly ////////////////////////////////

static void lBuildCatalog(void)
{
    int i;

    lExpandUnary(CG_INTRINSIC_ABS, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_ABS, FLTF, &intKind, 1);
    lExpandUnary(CG_INTRINSIC_ACOS, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lBuildBoolReductions();
    lExpandUnary(CG_INTRINSIC_ASIN, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_ATAN, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandBinaryReplicated(CG_INTRINSIC_ATAN2, FLTF,
                            floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_CEIL, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandTernaryReplicated(CG_INTRINSIC_CLAMP, FLTF,
                             floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_COS, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_COSH, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_DEGREES, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lBuildDot();
    lExpandUnary(CG_INTRINSIC_EXP, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_EXP2, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_FLOOR, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandBinaryReplicated(CG_INTRINSIC_FMOD, FLTF,
                            floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_FRAC, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandOutPair(CG_INTRINSIC_FREXP, CG_INTRINSIC_OUT_PARAMS,
                   floatKinds, FLOAT_KIND_COUNT);
    lExpandTernaryReplicated(CG_INTRINSIC_FACEFORWARD,
                             CG_INTRINSIC_PURE,
                             floatKinds, FLOAT_KIND_COUNT);
    lExpandUnaryTo(CG_INTRINSIC_ISFINITE, FLTF, floatKinds,
                   FLOAT_KIND_COUNT, CG_SCALAR_BOOL);
    lExpandUnaryTo(CG_INTRINSIC_ISINF, FLTF, floatKinds,
                   FLOAT_KIND_COUNT, CG_SCALAR_BOOL);
    lExpandUnaryTo(CG_INTRINSIC_ISNAN, FLTF, floatKinds,
                   FLOAT_KIND_COUNT, CG_SCALAR_BOOL);
    lBuildLdexp();
    lExpandTernaryReplicated(CG_INTRINSIC_LERP, FLTF,
                             floatKinds, FLOAT_KIND_COUNT);
    lBuildGeometry();     /* cross, lit, noise, refract */
    lExpandUnary(CG_INTRINSIC_LOG, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_LOG10, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_LOG2, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandBinaryReplicated(CG_INTRINSIC_MAX, FLTF,
                            floatKinds, FLOAT_KIND_COUNT);
    lExpandBinaryReplicated(CG_INTRINSIC_MIN, FLTF,
                            floatKinds, FLOAT_KIND_COUNT);
    lExpandOutPair(CG_INTRINSIC_MODF, CG_INTRINSIC_OUT_PARAMS,
                   floatKinds, FLOAT_KIND_COUNT);
    lBuildMul();
    lBuildMatrixOps();    /* transpose, determinant */
    lExpandBinaryReplicated(CG_INTRINSIC_POW, FLTF,
                            floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_RADIANS, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_ROUND, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_RSQRT, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_SATURATE, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_SIGN, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_SIGN, FLTF, &intKind, 1);
    lExpandUnary(CG_INTRINSIC_SIN, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lBuildDerivatives();  /* ddx, ddy */
    lBuildSincos();
    lExpandUnary(CG_INTRINSIC_SINH, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandTernaryReplicated(CG_INTRINSIC_SMOOTHSTEP, FLTF,
                             floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_SQRT, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandBinaryReplicated(CG_INTRINSIC_STEP, FLTF,
                            floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_TAN, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lExpandUnary(CG_INTRINSIC_TANH, FLTF, floatKinds, FLOAT_KIND_COUNT);
    lBuildVectorOps();    /* length, distance, normalize, reflect */
    lBuildDebug();

    for (i = 0; i < TEX_FAMILY_COUNT; i++) {
        lBuildBaseTextureFamily(&texFamilies[i]);
        lBuildTypedTextureFamily(&texFamilies[i], CG_SCALAR_HALF);
        lBuildTypedTextureFamily(&texFamilies[i], CG_SCALAR_FIXED);
    }
    for (i = 0; i < PROJ_TEX_FAMILY_COUNT; i++) {
        lBuildProjectedTextureFamily(&projTexFamilies[i],
                                     i % 2 ? CG_SCALAR_FIXED :
                                             CG_SCALAR_HALF);
    }
} // lBuildCatalog

////////////////////////////// Installation ////////////////////////////////////

/*
 * lInstallSignature() - Create the internal function symbol for one
 *          expanded signature, chaining it onto any same-name
 *          installation already present in "scope".
 */

static void lInstallSignature(Scope *scope, SourceLoc *loc,
                              const CgIntrinsicSignature *sig)
{
    Symbol *head, *member, *symbol;
    Type *funType;
    int atom;

    atom = LookUpAddString(atable, sig->name);
    head = LookUpLocalSymbol(scope, atom);
    if (head != NULL) {
        for (member = head; member != NULL;
             member = member->details.fun.overload)
        {
            if (member->kind == FUNCTION_S &&
                member->details.fun.intrinsic == sig)
            {
                return; // Idempotent re-installation.
            }
        }
    }
    funType = NewType(TYPE_CATEGORY_FUNCTION | TYPE_MISC_INTERNAL, 0);
    funType->fun.rettype = sig->result;
    funType->fun.paramtypes = sig->parameters;
    if (head != NULL) {
        symbol = NewSymbol(loc, scope, atom, funType, FUNCTION_S);
        symbol->details.fun.overload = head->details.fun.overload;
        head->details.fun.overload = symbol;
    } else {
        symbol = AddSymbol(loc, scope, atom, funType, FUNCTION_S);
    }
    symbol->properties |= SYMB_IS_BUILTIN;
    symbol->details.fun.profileSelector.name = 0;
    symbol->details.fun.profileSelector.specificity =
        CG_PROFILE_OPEN_SPECIFICITY;
    symbol->details.fun.profileSelector.isOpen = 1;
    symbol->details.fun.intrinsic = sig;
} // lInstallSignature

/*
 * Helper-structure rows, expanded from the declarative section of
 * cg_stdlib.def.  They introduce predefined output types selected by
 * the current profile family, so they live entirely apart from the
 * intrinsic opcode generation.
 */

#define CG_STDLIB_HELPERS 1
static const struct {
    const char *name;
    CgProfileStage stage;
} helperRows[] = {
#define CG_INTRINSIC(id, name, flags)
#define CG_STDLIB_SPECIAL(intrinsic, name, flags)
#define CG_STDLIB_HELPER(id, name, stage) { name, stage },
#include "cg_stdlib.def"
#undef CG_STDLIB_HELPER
#undef CG_STDLIB_SPECIAL
#undef CG_INTRINSIC
};
#undef CG_STDLIB_HELPERS

#define HELPER_ROW_COUNT \
    ((int) (sizeof(helperRows) / sizeof(helperRows[0])))

int CgStdlibHelperCount(void)
{
    return HELPER_ROW_COUNT;
} // CgStdlibHelperCount

const char *CgStdlibHelperName(int index)
{
    if (index < 0 || index >= HELPER_ROW_COUNT)
        return NULL;
    return helperRows[index].name;
} // CgStdlibHelperName

CgProfileStage CgStdlibHelperStage(int index)
{
    if (index < 0 || index >= HELPER_ROW_COUNT)
        return CG_PROFILE_STAGE_NEUTRAL;
    return helperRows[index].stage;
} // CgStdlibHelperStage

////////////////////////////// Geometry special rows ///////////////////////////

/*
 * The signature-less geometry operations.  Their rows carry immutable
 * flags like every catalog entry, but they expand into exactly one
 * installed symbol each -- never into an overload family -- so the
 * frontend's special-call path can own arity, argument annotations,
 * types, and placement.
 */

typedef struct CgStdlibSpecial_Rec {
    CgIntrinsic intrinsic;
    const char *name;
    unsigned flags;
} CgStdlibSpecial;

#define CG_INTRINSIC(id, name, flags)
#define CG_STDLIB_SPECIAL(intrinsic, name, flags) \
    { intrinsic, name, flags },
static const CgStdlibSpecial specialRows[] = {
#include "cg_stdlib.def"
};
#undef CG_STDLIB_SPECIAL
#undef CG_INTRINSIC

#define SPECIAL_ROW_COUNT \
    ((int) (sizeof(specialRows) / sizeof(specialRows[0])))

/* One immutable minimal signature per special row, built once beside
 * the catalog so every installed symbol carries a uniform
 * details.fun.intrinsic pointer. */

static CgIntrinsicSignature specialSigs[SPECIAL_ROW_COUNT];

static void lBuildSpecialSignatures(void)
{
    int i;

    for (i = 0; i < SPECIAL_ROW_COUNT; i++) {
        specialSigs[i].intrinsic = specialRows[i].intrinsic;
        specialSigs[i].name = specialRows[i].name;
        specialSigs[i].result = VoidType;
        specialSigs[i].parameters = NULL;
        specialSigs[i].flags = specialRows[i].flags;
    }
} // lBuildSpecialSignatures

/*
 * lInstallSpecialSymbol() - Create the reserved function symbol for
 *          one geometry operation.  The empty parameter list is a
 *          placeholder: ordinary parameter binding never runs for a
 *          selected special call.
 */

static void lInstallSpecialSymbol(Scope *scope, SourceLoc *loc,
                                  const CgStdlibSpecial *row)
{
    Symbol *symbol;
    Type *funType;
    int atom;

    atom = LookUpAddString(atable, row->name);
    if (LookUpLocalSymbol(scope, atom) != NULL)
        return; /* Idempotent re-installation. */
    funType = NewType(TYPE_CATEGORY_FUNCTION | TYPE_MISC_INTERNAL, 0);
    funType->fun.rettype = VoidType;
    funType->fun.paramtypes = NULL;
    symbol = AddSymbol(loc, scope, atom, funType, FUNCTION_S);
    symbol->properties |= SYMB_IS_BUILTIN;
    symbol->details.fun.profileSelector.name = 0;
    symbol->details.fun.profileSelector.specificity =
        CG_PROFILE_OPEN_SPECIFICITY;
    symbol->details.fun.profileSelector.isOpen = 1;
    symbol->details.fun.intrinsic =
        &specialSigs[(int) (row - specialRows)];
} // lInstallSpecialSymbol

/*
 * lHelperMemberType() - fragout carries a float4 color member and
 *          fragout_float a plain float; the row spelling decides.
 */

static Type *lHelperMemberType(const char *name)
{
    if (!strcmp(name, "fragout_float"))
        return FloatType;
    return Float4Type;
} // lHelperMemberType

static void lInstallHelperStruct(Scope *scope, SourceLoc *loc,
                                 const char *name)
{
    Type *structType;
    Scope *members;
    Symbol *member;

    structType = NewType(TYPE_CATEGORY_STRUCT, 0);
    members = NewScopeInPool(scope->pool);
    member = AddSymbol(loc, members, LookUpAddString(atable, "col"),
                       lHelperMemberType(name), VARIABLE_S);
    member->details.var.semantics = LookUpAddString(atable, "COLOR0");
    AddParameter(members, member);
    members->HasSemantics = 1;
    SetStructMembers(loc, structType, members);
    AddSymbol(loc, scope, LookUpAddString(atable, name), structType,
              TYPEDEF_S);
} // lInstallHelperStruct

////////////////////////////// Public API //////////////////////////////////////

int InitCgStdlib(Scope *scope)
{
    SourceLoc dummyLoc = { 0, 0 };
    int i;

    if (scope == NULL)
        return 0;
    if (!catalogBuilt) {
        lBuildCatalog();
        lBuildSpecialSignatures();
        catalogBuilt = 1;
    }
    for (i = 0; i < catalogSigCount; i++)
        lInstallSignature(scope, &dummyLoc, &catalogSigs[i]);

    /* Geometry operation symbols exist only under the Cg 2.0 language
     * gate: explicit Cg 1.1 can never resolve a geometry intrinsic
     * through the catalog. */
    if (CgLanguageAllowsGeometry(Cg->options.languageVersion)) {
        for (i = 0; i < SPECIAL_ROW_COUNT; i++)
            lInstallSpecialSymbol(scope, &dummyLoc, &specialRows[i]);
    }

    /* Helper structures follow the current profile family; profiles
     * outside a row's stage simply never see that type, and any use is
     * diagnosed as an unknown name during semantic analysis. */
    if (Cg->theHAL != NULL) {
        for (i = 0; i < HELPER_ROW_COUNT; i++) {
            if (helperRows[i].stage ==
                Cg->theHAL->profileIdentity.stage)
            {
                lInstallHelperStruct(scope, &dummyLoc,
                                     helperRows[i].name);
            }
        }
    }
    return 1;
} // InitCgStdlib

const CgIntrinsicSignature *CgIntrinsicSignatureForSymbol(
    const Symbol *symbol)
{
    if (symbol == NULL || symbol->kind != FUNCTION_S)
        return NULL;
    return (const CgIntrinsicSignature *) symbol->details.fun.intrinsic;
} // CgIntrinsicSignatureForSymbol

int CgStdlibCatalogCount(void)
{
    return CATALOG_NAME_COUNT;
} // CgStdlibCatalogCount

const char *CgStdlibCatalogName(int index)
{
    if (index < 0 || index >= CATALOG_NAME_COUNT)
        return NULL;
    return catalogNames[index];
} // CgStdlibCatalogName

int CgFindIntrinsicByName(const char *name)
{
    int i;

    if (name == NULL)
        return CG_INTRINSIC_NONE;
    for (i = 0; i < CATALOG_NAME_COUNT; i++) {
        if (!strcmp(catalogNames[i], name))
            return i + 1;
    }
    for (i = 0; i < SPECIAL_ROW_COUNT; i++) {
        if (!strcmp(specialRows[i].name, name))
            return specialRows[i].intrinsic;
    }
    return CG_INTRINSIC_NONE;
} // CgFindIntrinsicByName

int CgIntrinsicIsGeometrySpecial(CgIntrinsic intrinsic)
{
    int i;

    for (i = 0; i < SPECIAL_ROW_COUNT; i++) {
        if (specialRows[i].intrinsic == intrinsic)
            return (specialRows[i].flags &
                    CG_INTRINSIC_FLAG_GEOMETRY) != 0;
    }
    return 0;
} // CgIntrinsicIsGeometrySpecial
