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
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN ANY WAY
OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE
NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT,
TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF
NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

//
// cg_types.c
//

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "cg_types.h"

typedef struct CgScalarTraits_Rec {
    const char *name;
    unsigned isCompileTime : 1;
    unsigned isIntegral : 1;
    unsigned isUnsigned : 1;
    unsigned isFloating : 1;
} CgScalarTraits;

static const CgScalarTraits traits[CG_SCALAR_COUNT] = {
    { "<none>", 0, 0, 0, 0 },
    { "<undefined>", 0, 0, 0, 0 },
    { "cfloat", 1, 0, 0, 1 },
    { "cint", 1, 1, 0, 0 },
    { "bool", 0, 0, 0, 0 },
    { "char", 0, 1, 0, 0 },
    { "unsigned char", 0, 1, 1, 0 },
    { "short", 0, 1, 0, 0 },
    { "unsigned short", 0, 1, 1, 0 },
    { "int", 0, 1, 0, 0 },
    { "unsigned int", 0, 1, 1, 0 },
    { "long", 0, 1, 0, 0 },
    { "unsigned long", 0, 1, 1, 0 },
    { "fixed", 0, 0, 0, 1 },
    { "half", 0, 0, 0, 1 },
    { "float", 0, 0, 0, 1 },
    { "double", 0, 0, 0, 1 }
};

static Type *standardTypes[CG_SCALAR_COUNT][5][5];

/*
 * GetScalarKind() - Return the canonical scalar kind of a type.
 *
 */

CgScalarKind GetScalarKind(const Type *type)
{
    if (type) {
        return type->co.scalarKind;
    } else {
        return CG_SCALAR_NONE;
    }
} // GetScalarKind

/*
 * SetScalarKind() - Set the canonical scalar kind of a type.
 *
 */

void SetScalarKind(Type *type, CgScalarKind kind)
{
    if (type) {
        type->co.scalarKind = kind;
    }
} // SetScalarKind

/*
 * CgTypeIsPoison() - Nonzero for the shared UndefinedType recovery
 *          sentinel: an operand of that type already produced its one
 *          diagnostic, so downstream conversion, overload, interface,
 *          lowering, and profile layers must return early instead of
 *          reporting again.
 */

int CgTypeIsPoison(const Type *type)
{
    return type == UndefinedType;
} // CgTypeIsPoison

/*
 * CgScalarIsCompileTime() - Returns TRUE if kinds of this class are
 *                          compile-time constants.
 *
 */

int CgScalarIsCompileTime(CgScalarKind kind)
{
    if (kind >= CG_SCALAR_NONE && kind < CG_SCALAR_COUNT) {
        return traits[kind].isCompileTime;
    } else {
        return 0;
    }
} // CgScalarIsCompileTime

/*
 * CgScalarIsIntegral() - Returns TRUE if kinds of this class are integral.
 *
 */

int CgScalarIsIntegral(CgScalarKind kind)
{
    if (kind >= CG_SCALAR_NONE && kind < CG_SCALAR_COUNT) {
        return traits[kind].isIntegral;
    } else {
        return 0;
    }
} // CgScalarIsIntegral

/*
 * CgScalarIsUnsigned() - Returns TRUE if kinds of this class are unsigned.
 *
 */

int CgScalarIsUnsigned(CgScalarKind kind)
{
    if (kind >= CG_SCALAR_NONE && kind < CG_SCALAR_COUNT) {
        return traits[kind].isUnsigned;
    } else {
        return 0;
    }
} // CgScalarIsUnsigned

/*
 * CgScalarIsFloating() - Returns TRUE if kinds of this class are floating
 *                        point.
 *
 */

int CgScalarIsFloating(CgScalarKind kind)
{
    if (kind >= CG_SCALAR_NONE && kind < CG_SCALAR_COUNT) {
        return traits[kind].isFloating;
    } else {
        return 0;
    }
} // CgScalarIsFloating

/*
 * CgScalarKindName() - Return a pointer to a string representation of a
 *                      canonical scalar kind.
 *
 */

const char *CgScalarKindName(CgScalarKind kind)
{
    if (kind >= CG_SCALAR_NONE && kind < CG_SCALAR_COUNT) {
        return traits[kind].name;
    } else {
        return traits[CG_SCALAR_NONE].name;
    }
} // CgScalarKindName

/*
 * CgScalarLegacyBase() - Return the legacy four-bit base that backs a
 *                        canonical scalar kind, or TYPE_BASE_NO_TYPE for
 *                        kinds with no legacy representation.
 *
 */

int CgScalarLegacyBase(CgScalarKind kind)
{
    switch (kind) {
    case CG_SCALAR_CFLOAT:
        return TYPE_BASE_CFLOAT;
    case CG_SCALAR_CINT:
        return TYPE_BASE_CINT;
    case CG_SCALAR_BOOL:
        return TYPE_BASE_BOOLEAN;
    case CG_SCALAR_CHAR:
    case CG_SCALAR_UCHAR:
    case CG_SCALAR_SHORT:
    case CG_SCALAR_USHORT:
    case CG_SCALAR_INT:
    case CG_SCALAR_UINT:
    case CG_SCALAR_LONG:
    case CG_SCALAR_ULONG:
        return TYPE_BASE_INT;
    case CG_SCALAR_FIXED:
    case CG_SCALAR_HALF:
    case CG_SCALAR_FLOAT:
    case CG_SCALAR_DOUBLE:
        return TYPE_BASE_FLOAT;
    default:
        return TYPE_BASE_NO_TYPE;
    }
} // CgScalarLegacyBase

///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Normative Conversion Matrix: /////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * lValidConversionKind() - TRUE if a kind names a real scalar identity.
 *                          Kinds the canonical table does not model (NONE,
 *                          UNDEFINED) never convert; this covers void,
 *                          undefined types, and profile extension bases such
 *                          as samplers without consulting the HAL.
 */

static int lValidConversionKind(CgScalarKind kind)
{
    return kind > CG_SCALAR_UNDEFINED && kind < CG_SCALAR_COUNT;
} // lValidConversionKind

/*
 * lNumericRank() - Capacity of a scalar within its class: integral widths
 *                  and floating precision both order 1 (narrowest) to 4.
 *
 */

static int lNumericRank(CgScalarKind kind)
{
    switch (kind) {
    case CG_SCALAR_CHAR:
    case CG_SCALAR_UCHAR:
    case CG_SCALAR_FIXED:
        return 1;
    case CG_SCALAR_SHORT:
    case CG_SCALAR_USHORT:
    case CG_SCALAR_HALF:
        return 2;
    case CG_SCALAR_INT:
    case CG_SCALAR_UINT:
    case CG_SCALAR_FLOAT:
        return 3;
    case CG_SCALAR_LONG:
    case CG_SCALAR_ULONG:
    case CG_SCALAR_DOUBLE:
        return 4;
    default:
        return 0;
    }
} // lNumericRank

/*
 * lMapExplicitRank() - An explicit cast downgrades a warning-ranked implicit
 *                      conversion to a plain explicit one; every other rank
 *                      is returned unchanged so promotions and exact matches
 *                      keep their stronger classification under casts too.
 *
 */

static CgConversionRank lMapExplicitRank(CgConversionRank rank, int explicitCast)
{
    if (explicitCast && rank == CG_CONVERSION_IMPLICIT_WARN) {
        return CG_CONVERSION_EXPLICIT;
    }
    return rank;
} // lMapExplicitRank

/*
 * CgClassifyScalarConversion() - Rank a conversion between two scalar kinds
 *                                following the specification's ordered rules:
 *
 *   1. Unknown identities convert to nothing.
 *   2. A kind converts to itself exactly.
 *   3. Compile-time cint/cfloat constants promote to any numeric target.
 *   4. Boolean interconverts implicitly with every numeric kind.
 *   5. Within a class, narrowing loses information: warned when implicit,
 *      plain explicit under a cast; widening and equal-width conversions
 *      (including signed/unsigned pairs) are silent implicits.
 *   6. Across classes (integral vs floating family) conversions are silent
 *      implicits.
 *
 */

CgConversionRank CgClassifyScalarConversion(CgScalarKind from,
                                            CgScalarKind to,
                                            int explicitCast)
{
    if (!lValidConversionKind(from) || !lValidConversionKind(to)) {
        return CG_CONVERSION_NONE;
    }
    if (from == to) {
        return CG_CONVERSION_EXACT;
    }
    if (from == CG_SCALAR_CINT || from == CG_SCALAR_CFLOAT) {
        return CG_CONVERSION_PROMOTION;
    }
    if (to == CG_SCALAR_CINT || to == CG_SCALAR_CFLOAT) {
        /* No surface syntax names these targets; treat them as their
         * runtime equivalents for the remaining rules. */
        to = to == CG_SCALAR_CINT ? CG_SCALAR_INT : CG_SCALAR_FLOAT;
        if (to == from) {
            return CG_CONVERSION_IMPLICIT;
        }
    }
    if (from == CG_SCALAR_BOOL || to == CG_SCALAR_BOOL) {
        return CG_CONVERSION_IMPLICIT;
    }
    if ((CgScalarIsIntegral(from) && CgScalarIsIntegral(to)) ||
        (CgScalarIsFloating(from) && CgScalarIsFloating(to)))
    {
        if (lNumericRank(to) >= lNumericRank(from)) {
            return CG_CONVERSION_IMPLICIT;
        }
        return lMapExplicitRank(CG_CONVERSION_IMPLICIT_WARN, explicitCast);
    }
    return CG_CONVERSION_IMPLICIT;
} // CgClassifyScalarConversion

/*
 * lClassifyArrayConversion() - Rank array-shape conversions:
 *
 *   - vector -> same-length vector: silent element conversion;
 *   - vector -> shorter vector: prefix element selection under a cast;
 *   - matrix -> same-dims matrix: silent element conversion;
 *   - matrix -> smaller matrix: upper-left submatrix selection under a cast;
 *   - vector <-> matrix: reshape under a cast when element counts match;
 *   - conversions touching an unpacked dimension: equal element counts,
 *     available only through an explicit cast.
 *
 */

static CgConversionRank lClassifyArrayConversion(const Type *from,
                                                 const Type *to, int explicitCast)
{
    int flen = 0, tlen = 0, fcols = 0, frows = 0, tcols = 0, trows = 0;
    int fromVector = IsVector(from, &flen);
    int toVector = IsVector(to, &tlen);
    int fromMatrix = IsMatrix(from, &fcols, &frows);
    int toMatrix = IsMatrix(to, &tcols, &trows);
    CgConversionRank rank;

    if (fromVector && toVector) {
        if (tlen > flen || (tlen != flen && !explicitCast)) {
            return CG_CONVERSION_NONE;
        }
    } else if (fromMatrix && toMatrix) {
        if (trows > frows || tcols > fcols ||
            ((trows != frows || tcols != fcols) && !explicitCast))
        {
            return CG_CONVERSION_NONE;
        }
    } else if ((fromVector || fromMatrix) && (toVector || toMatrix)) {
        int fromCount = fromVector ? flen : frows * fcols;
        int toCount = toVector ? tlen : trows * tcols;

        if (fromCount != toCount || !explicitCast) {
            return CG_CONVERSION_NONE;
        }
    } else {
        if (IsUnsizedArray(to) && IsPacked(to) == IsPacked(from) &&
            IsSameUnqualifiedType(to->arr.eltype, from->arr.eltype))
        {
            /* A concrete array binds identically to an unsized array
             * with the same element shape: the runtime length travels
             * with the value, so this is exact-shape compatibility. */
            return CG_CONVERSION_EXACT;
        }
        if (from->arr.numels == to->arr.numels &&
            IsSameUnqualifiedType(from->arr.eltype, to->arr.eltype))
        {
            /* Identical aggregate shapes convert exactly; no cast is
             * needed to recognize a type as itself. */
            return CG_CONVERSION_EXACT;
        }
        if (from->arr.numels != to->arr.numels || !explicitCast) {
            /* Unpacked arrays convert element-wise only through a cast. */
            return CG_CONVERSION_NONE;
        }
    }
    rank = CgClassifyConversion(from->arr.eltype, to->arr.eltype, explicitCast);
    if (rank == CG_CONVERSION_NONE) {
        return CG_CONVERSION_NONE;
    }
    return rank;
} // lClassifyArrayConversion

/*
 * lClassifyScalarToArray() - Scalar replication fills every element of a
 *                            vector or matrix target with the scalar's
 *                            element classification.
 *
 */

static CgConversionRank lClassifyScalarToArray(const Type *from,
                                               const Type *to, int explicitCast)
{
    if (!IsVector(to, NULL) && !IsMatrix(to, NULL, NULL)) {
        return CG_CONVERSION_NONE;
    }
    return CgClassifyScalarConversion(GetScalarKind(from),
                                      GetScalarKind(to->arr.eltype),
                                      explicitCast);
} // lClassifyScalarToArray

/*
 * lClassifyArrayToScalar() - Vector or matrix to scalar reads the first
 *                            element; dropping the remaining components is
 *                            warned about when it happens implicitly.
 *
 */

static CgConversionRank lClassifyArrayToScalar(const Type *from,
                                               const Type *to, int explicitCast)
{
    CgConversionRank rank;

    if (!IsVector(from, NULL) && !IsMatrix(from, NULL, NULL)) {
        return CG_CONVERSION_NONE;
    }
    rank = CgClassifyScalarConversion(GetScalarKind(from->arr.eltype),
                                      GetScalarKind(to), explicitCast);
    if (rank == CG_CONVERSION_NONE) {
        return CG_CONVERSION_NONE;
    }
    return explicitCast ? CG_CONVERSION_EXPLICIT : CG_CONVERSION_IMPLICIT_WARN;
} // lClassifyArrayToScalar

/*
 * lStructMembers() - Return the ordered data-member symbol list of a
 *                    struct type.  Methods live only in the name-lookup
 *                    tree, never in this list.
 *
 */

static Symbol *lStructMembers(const Type *structType)
{
    if (!structType->str.members) {
        return NULL;
    }
    return structType->str.members->params;
} // lStructMembers

/*
 * lImplementsInterface() - TRUE when the struct's implemented interface
 *                         is exactly "to".  Structs inherit a single
 *                         interface and interface identity is pointer
 *                         uniqueness of the declared type.
 *
 */

static int lImplementsInterface(const Type *from, const Type *to)
{
    return from->str.implementedInterface != NULL &&
           from->str.implementedInterface == to;
} // lImplementsInterface

/*
 * lNextDataMember() - Advance to the next data member; methods are not
 *                     data and never participate in conversions.
 *
 */

static Symbol *lNextDataMember(Symbol *member)
{
    while (member && IsFunction(member)) {
        member = member->next;
    }
    return member;
} // lNextDataMember

/*
 * lClassifyStructConversion() - Structure casts are explicit-only and take
 *                               three forms: pairwise member conversion with
 *                               equal member counts, extraction of the first
 *                               member's value, and wrapping a value into a
 *                               one-member structure.  An identical structure
 *                               type converts exactly in any context.
 *
 */

static CgConversionRank lClassifyStructConversion(const Type *from,
                                                  const Type *to,
                                                  int explicitCast)
{
    Symbol *fmember, *tmember;

    if (IsSameUnqualifiedType(from, to)) {
        return CG_CONVERSION_EXACT;
    }
    if (!explicitCast) {
        return CG_CONVERSION_NONE;
    }
    fmember = lNextDataMember(lStructMembers(from));
    tmember = lNextDataMember(lStructMembers(to));
    while (fmember && tmember) {
        if (CgClassifyConversion(fmember->type, tmember->type, 1) ==
            CG_CONVERSION_NONE)
        {
            return CG_CONVERSION_NONE;
        }
        fmember = lNextDataMember(fmember->next);
        tmember = lNextDataMember(tmember->next);
    }
    if (fmember || tmember) {
        return CG_CONVERSION_NONE;
    }
    return CG_CONVERSION_EXPLICIT;
} // lClassifyStructConversion

/*
 * lClassifyStructFrom() - Explicit cast of a structure to another category:
 *                         the value of its first data member converts to
 *                         the target type.
 *
 */

static CgConversionRank lClassifyStructFrom(const Type *from,
                                            const Type *to, int explicitCast)
{
    Symbol *first;

    if (!explicitCast) {
        return CG_CONVERSION_NONE;
    }
    first = lNextDataMember(lStructMembers(from));
    if (!first) {
        return CG_CONVERSION_NONE;
    }
    return CgClassifyConversion(first->type, to, 1);
} // lClassifyStructFrom

/*
 * lClassifyStructTo() - Explicit cast into a structure: only structures
 *                       with a single data member can be formed, from a
 *                       value convertible to that member's type.
 *
 */

static CgConversionRank lClassifyStructTo(const Type *from,
                                          const Type *to, int explicitCast)
{
    Symbol *first;

    if (!explicitCast) {
        return CG_CONVERSION_NONE;
    }
    first = lNextDataMember(lStructMembers(to));
    if (!first || lNextDataMember(first->next)) {
        return CG_CONVERSION_NONE;
    }
    return CgClassifyConversion(from, first->type, 1);
} // lClassifyStructTo

/*
 * CgClassifyConversion() - Rank converting a value of "from" to "to",
 *                          consulting only language-level type information.
 *
 */

CgConversionRank CgClassifyConversion(const Type *from, const Type *to,
                                      int explicitCast)
{
    int fromCategory, toCategory;

    if (!from || !to) {
        return CG_CONVERSION_NONE;
    }
    fromCategory = GetCategory(from);
    toCategory = GetCategory(to);
    if (fromCategory == toCategory) {
        switch (fromCategory) {
        case TYPE_CATEGORY_SCALAR:
            return CgClassifyScalarConversion(GetScalarKind(from),
                                              GetScalarKind(to), explicitCast);
        case TYPE_CATEGORY_ARRAY:
            return lClassifyArrayConversion(from, to, explicitCast);
        case TYPE_CATEGORY_STRUCT:
            return lClassifyStructConversion(from, to, explicitCast);
        case TYPE_CATEGORY_INTERFACE:
            /* Interface identity is the declared type itself. */
            return from == to ? CG_CONVERSION_EXACT : CG_CONVERSION_NONE;
        case TYPE_CATEGORY_SAMPLER: {
            /* Samplers are opaque: the same object converts exactly,
             * and a specific kind binds to the deprecated base sampler
             * when the family rules say they are compatible. */
            CgSamplerKind fromKind, toKind;

            if (from == to)
                return CG_CONVERSION_EXACT;
            if (IsSampler(from, &fromKind) && IsSampler(to, &toKind) &&
                CgSamplerCompatible(toKind, fromKind))
            {
                return CG_CONVERSION_EXACT;
            }
            return CG_CONVERSION_NONE;
        }
        default:
            return CG_CONVERSION_NONE;
        }
    }
    switch (fromCategory) {
    case TYPE_CATEGORY_STRUCT:
        /* A struct converts dynamically to the interface it implements;
         * no other struct conversion is implicit. */
        if (toCategory == TYPE_CATEGORY_INTERFACE &&
            lImplementsInterface(from, to))
        {
            return CG_CONVERSION_DYNAMIC;
        }
        return lClassifyStructFrom(from, to, explicitCast);
    case TYPE_CATEGORY_ARRAY:
        if (toCategory == TYPE_CATEGORY_SCALAR) {
            return lClassifyArrayToScalar(from, to, explicitCast);
        }
        return CG_CONVERSION_NONE;
    case TYPE_CATEGORY_SCALAR:
        if (toCategory == TYPE_CATEGORY_ARRAY) {
            return lClassifyScalarToArray(from, to, explicitCast);
        }
        if (toCategory == TYPE_CATEGORY_STRUCT) {
            return lClassifyStructTo(from, to, explicitCast);
        }
        return CG_CONVERSION_NONE;
    default:
        return CG_CONVERSION_NONE;
    }
} // CgClassifyConversion

/*
 * lResolveCompileTimeKind() - The concrete kind a compile-time constant
 *                             adopts when the other operand carries it.
 *
 */

static CgScalarKind lResolveCompileTimeKind(CgScalarKind constant,
                                            CgScalarKind other)
{
    if (constant == CG_SCALAR_CINT && CgScalarIsIntegral(other)) {
        return other;
    }
    if (CgScalarIsFloating(other)) {
        return other;
    }
    return constant == CG_SCALAR_CINT ? CG_SCALAR_INT : CG_SCALAR_FLOAT;
} // lResolveCompileTimeKind

/*
 * lUsualArithmeticKind() - The result kind of an arithmetic operation on two
 *                          numeric operand kinds, following the usual
 *                          arithmetic conversions: compile-time kinds adapt
 *                          to their partners, floating precision dominates
 *                          integral width (double > float > half > fixed),
 *                          wider integrals beat narrower ones, and
 *                          unsigned wins at equal width.
 *
 */

static CgScalarKind lUsualArithmeticKind(CgScalarKind left, CgScalarKind right)
{
    if (!lValidConversionKind(left) || !lValidConversionKind(right)) {
        return CG_SCALAR_NONE;
    }
    if (left == CG_SCALAR_BOOL || right == CG_SCALAR_BOOL) {
        return CG_SCALAR_NONE;
    }
    if (CgScalarIsCompileTime(left)) {
        if (CgScalarIsCompileTime(right)) {
            if (left == right) {
                return left;
            }
            return CG_SCALAR_CFLOAT;
        }
        left = lResolveCompileTimeKind(left, right);
    } else if (CgScalarIsCompileTime(right)) {
        right = lResolveCompileTimeKind(right, left);
    } else if ((!CgScalarIsIntegral(left) && !CgScalarIsFloating(left)) ||
               (!CgScalarIsIntegral(right) && !CgScalarIsFloating(right)))
    {
        return CG_SCALAR_NONE;
    }
    if (CgScalarIsFloating(left) && CgScalarIsFloating(right)) {
        return lNumericRank(left) >= lNumericRank(right) ? left : right;
    }
    if (CgScalarIsFloating(left)) {
        return left;
    }
    if (CgScalarIsFloating(right)) {
        return right;
    }
    if (lNumericRank(left) != lNumericRank(right)) {
        return lNumericRank(left) > lNumericRank(right) ? left : right;
    }
    if (CgScalarIsUnsigned(right)) {
        return right;
    }
    return left;
} // lUsualArithmeticKind

/*
 * CgUsualArithmeticType() - The interned scalar type arithmetic on "left"
 *                           and "right" produces, or NULL when either operand
 *                           is not numeric.
 *
 */

Type *CgUsualArithmeticType(const Type *left, const Type *right)
{
    CgScalarKind kind = lUsualArithmeticKind(GetScalarKind(left),
                                             GetScalarKind(right));

    if (!lValidConversionKind(kind)) {
        return NULL;
    }
    return GetStandardTypeKind(kind, 0, 0);
} // CgUsualArithmeticType

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// Standard Type Interning: ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * lKindProperties() - Legacy property bits backing a canonical scalar kind.
 *
 * Kinds the canonical enum does not model yet (CG_SCALAR_NONE,
 * CG_SCALAR_UNDEFINED) get no properties and are never interned.  New
 * integral kinds share the legacy int base and new floating kinds share
 * the legacy float base until later tasks retire the four-bit bases;
 * cfloat/cint keep their const qualifiers.
 */

static int lKindProperties(CgScalarKind kind)
{
    switch (kind) {
    case CG_SCALAR_CFLOAT:
        return TYPE_BASE_CFLOAT | TYPE_CATEGORY_SCALAR | TYPE_QUALIFIER_CONST;
    case CG_SCALAR_CINT:
        return TYPE_BASE_CINT | TYPE_CATEGORY_SCALAR | TYPE_QUALIFIER_CONST;
    case CG_SCALAR_BOOL:
        return TYPE_BASE_BOOLEAN | TYPE_CATEGORY_SCALAR;
    case CG_SCALAR_CHAR:
    case CG_SCALAR_UCHAR:
    case CG_SCALAR_SHORT:
    case CG_SCALAR_USHORT:
    case CG_SCALAR_INT:
    case CG_SCALAR_UINT:
    case CG_SCALAR_LONG:
    case CG_SCALAR_ULONG:
        return TYPE_BASE_INT | TYPE_CATEGORY_SCALAR;
    case CG_SCALAR_FIXED:
    case CG_SCALAR_HALF:
    case CG_SCALAR_FLOAT:
    case CG_SCALAR_DOUBLE:
        return TYPE_BASE_FLOAT | TYPE_CATEGORY_SCALAR;
    default:
        return 0;
    }
} // lKindProperties

/*
 * GetStandardTypeKind() - Return the interned standard type for a canonical
 *                         scalar kind and shape.
 *
 * Indices are (0,0) for a scalar, (length,0) for a vector, and
 * (rows,columns) for a matrix.  Shapes outside these ranges, and kinds
 * with no interned representation, map to UndefinedType.
 */

Type *GetStandardTypeKind(CgScalarKind kind, int rows, int columns)
{
    if (kind >= CG_SCALAR_NONE && kind < CG_SCALAR_COUNT &&
        rows >= 0 && rows <= 4 && columns >= 0 && columns <= 4 &&
        (rows > 0 || columns == 0))
    {
        if (standardTypes[kind][rows][columns])
            return standardTypes[kind][rows][columns];
    }
    return UndefinedType;
} // GetStandardTypeKind

/*
 * InitCgStandardTypes() - Intern every supported standard scalar, vector,
 *                         and matrix type.
 *
 * Vectors are constructed as packed arrays of the scalar type and matrices
 * as packed arrays of packed row vectors; the scalar kind is stored on
 * every layer.
 */

int InitCgStandardTypes(void)
{
    CgScalarKind kind;

    for (kind = CG_SCALAR_NONE; kind < CG_SCALAR_COUNT; kind++) {
        int properties = lKindProperties(kind);
        int qualifiers = properties & TYPE_QUALIFIER_MASK;
        int rows, columns, length;
        Type *scalar, *row;

        if (!properties)
            continue;
        scalar = NewType(properties, 1);
        SetScalarKind(scalar, kind);
        standardTypes[kind][0][0] = scalar;
        for (length = 1; length <= 4; length++) {
            standardTypes[kind][length][0] =
                NewPackedArrayType(scalar, length, qualifiers);
        }
        for (rows = 1; rows <= 4; rows++) {
            for (columns = 1; columns <= 4; columns++) {
                row = NewPackedArrayType(scalar, columns, qualifiers);
                standardTypes[kind][rows][columns] =
                    NewPackedArrayType(row, rows, qualifiers);
            }
        }
    }
    return 1;
} // InitCgStandardTypes

/*
 * FreeCgStandardTypes() - Reset the interned standard-type registry.
 *
 * The interned Type structs stay allocated, exactly like every other
 * symbol-table type in this compiler; only the registry entries are
 * cleared so a later InitCgStandardTypes() rebuilds cleanly.
 */

void FreeCgStandardTypes(void)
{
    memset(standardTypes, 0, sizeof(standardTypes));
} // FreeCgStandardTypes

///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Sampler Type Registry: ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * CgSamplerLegacyBase() - The four-bit texture-object base that backs a
 *                         canonical sampler kind inside backend validation
 *                         and lowering.  The first four values deliberately
 *                         equal the GLSL profiles' historical user bases
 *                         (TYPE_BASE_GLSL_SAMPLER1D..CUBE in glsl_hal.h) so
 *                         backend base decoding keeps working unchanged;
 *                         samplerRECT and the deprecated base kind occupy
 *                         the next free user slots.
 */

int CgSamplerLegacyBase(CgSamplerKind kind)
{
    switch (kind) {
    case CG_SAMPLER_1D:
        return TYPE_BASE_FIRST_USER + 0;
    case CG_SAMPLER_2D:
        return TYPE_BASE_FIRST_USER + 1;
    case CG_SAMPLER_3D:
        return TYPE_BASE_FIRST_USER + 2;
    case CG_SAMPLER_CUBE:
        return TYPE_BASE_FIRST_USER + 3;
    case CG_SAMPLER_RECT:
        return TYPE_BASE_FIRST_USER + 4;
    case CG_SAMPLER_BASE:
        return TYPE_BASE_FIRST_USER + 5;
    default:
        return TYPE_BASE_NO_TYPE;
    }
} // CgSamplerLegacyBase

static Type *samplerTypes[CG_SAMPLER_COUNT];

/*
 * IsSampler() - TRUE if "type" is a canonical sampler type; its family
 *               member is reported through "kind" when non-NULL.
 *
 */

int IsSampler(const Type *type, CgSamplerKind *kind)
{
    CgSamplerKind sampled;

    if (!type || GetCategory(type) != TYPE_CATEGORY_SAMPLER) {
        return 0;
    }
    sampled = type->samp.samplerKind;
    if (sampled < CG_SAMPLER_BASE || sampled >= CG_SAMPLER_COUNT) {
        return 0;
    }
    if (kind) {
        *kind = sampled;
    }
    return 1;
} // IsSampler

/*
 * GetSamplerType() - Return the interned canonical sampler type for
 *                    "kind", creating it on first use.  Invalid kinds map
 *                    to UndefinedType, like unsupported standard shapes.
 *
 */

Type *GetSamplerType(CgSamplerKind kind)
{
    Type *type;

    if (kind < CG_SAMPLER_BASE || kind >= CG_SAMPLER_COUNT) {
        return UndefinedType;
    }
    if (!samplerTypes[kind]) {
        type = NewType(TYPE_CATEGORY_SAMPLER | CgSamplerLegacyBase(kind), 1);
        SetScalarKind(type, CG_SCALAR_NONE);
        type->samp.samplerKind = kind;
        samplerTypes[kind] = type;
    }
    return samplerTypes[kind];
} // GetSamplerType

/*
 * CgSamplerCompatible() - TRUE when an actual sampler of "actualKind" may
 *                         bind to a formal of "formalKind": any specific
 *                         kind binds to the deprecated base sampler, and
 *                         only identical kinds bind to each other.  A base
 *                         sampler value never binds to a specific formal,
 *                         which keeps the deprecated spelling from
 *                         masquerading as a concrete kind.
 *
 */

int CgSamplerCompatible(CgSamplerKind formalKind, CgSamplerKind actualKind)
{
    if (formalKind < CG_SAMPLER_BASE || formalKind >= CG_SAMPLER_COUNT ||
        actualKind < CG_SAMPLER_BASE || actualKind >= CG_SAMPLER_COUNT)
    {
        return 0;
    }
    if (formalKind == CG_SAMPLER_BASE) {
        return 1;
    }
    return formalKind == actualKind;
} // CgSamplerCompatible

/*
 * FreeCgSamplerTypes() - Reset the interned sampler-type registry.  The
 *                        interned Type structs stay allocated; only the
 *                        registry entries are cleared.
 */

void FreeCgSamplerTypes(void)
{
    memset(samplerTypes, 0, sizeof(samplerTypes));
} // FreeCgSamplerTypes

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// Attrib Array Type Registry: ////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * The interned attribute-array identities, most recent first.  Entries
 * live as long as the registry; the Type structs themselves stay
 * allocated for the symbol-table lifetime like every other type here.
 */

typedef struct CgAttribArrayEntry_Rec {
    struct CgAttribArrayEntry_Rec *next;
    Type *type;
} CgAttribArrayEntry;

static CgAttribArrayEntry *attribArrayEntries;

/*
 * lValidAttribArrayExtent() - TRUE for the unresolved source shape (0)
 *         and the resolved topology shapes (1, 2, 3, 4, 6); no other
 *         count names a canonical identity.
 */

static int lValidAttribArrayExtent(unsigned int extent)
{
    return extent == 0 || extent == 1 || extent == 2 ||
           extent == 3 || extent == 4 || extent == 6;
} // lValidAttribArrayExtent

/*
 * CgGetAttribArrayType() - Return the interned AttribArray type for an
 *          element pointer and extent, creating it on first use.
 *          Identical (element, extent) pairs return the same Type, so
 *          pointer equality is the whole equality story downstream.
 */

Type *CgGetAttribArrayType(Type *element, unsigned int extent)
{
    CgAttribArrayEntry *entry;
    Type *type;

    if (!element || CgTypeIsPoison(element) ||
        !lValidAttribArrayExtent(extent))
    {
        return UndefinedType;
    }
    for (entry = attribArrayEntries; entry; entry = entry->next) {
        if (entry->type->attrarr.eltype == element &&
            entry->type->attrarr.extent == (int)extent)
        {
            return entry->type;
        }
    }
    /* NewType stamps a legacy scalar kind from the base bits into the
     * first payload word; assigning eltype afterwards overwrites that
     * slot with the real payload, and nothing may write scalarKind on
     * an attribute array again. */
    type = NewType(TYPE_CATEGORY_ATTRIB_ARRAY | GetBase(element), 0);
    type->attrarr.eltype = element;
    type->attrarr.extent = (int)extent;
    entry = (CgAttribArrayEntry *) malloc(sizeof(CgAttribArrayEntry));
    entry->type = type;
    entry->next = attribArrayEntries;
    attribArrayEntries = entry;
    return type;
} // CgGetAttribArrayType

/*
 * CgIsAttribArray() - TRUE if "type" is a canonical attribute array;
 *          never true for ordinary arrays of any shape.
 */

int CgIsAttribArray(const Type *type)
{
    if (type && GetCategory(type) == TYPE_CATEGORY_ATTRIB_ARRAY) {
        return 1;
    } else {
        return 0;
    }
} // CgIsAttribArray

/*
 * CgAttribArrayElement() - The interned element type of an attribute
 *          array, or NULL for anything else.
 */

Type *CgAttribArrayElement(const Type *type)
{
    if (!CgIsAttribArray(type)) {
        return NULL;
    }
    return type->attrarr.eltype;
} // CgAttribArrayElement

/*
 * CgAttribArrayExtent() - The source (0) or resolved topology extent
 *          of an attribute array; 0 for anything else.
 */

unsigned int CgAttribArrayExtent(const Type *type)
{
    if (!CgIsAttribArray(type)) {
        return 0;
    }
    return (unsigned int) type->attrarr.extent;
} // CgAttribArrayExtent

/*
 * FreeCgAttribArrayTypes() - Reset the interned attribute-array
 *                            registry.  The interned Type structs stay
 *                            allocated; only the registry entries are
 *                            freed so a later symbol table rebuilds
 *                            cleanly.
 */

void FreeCgAttribArrayTypes(void)
{
    CgAttribArrayEntry *entry;
    CgAttribArrayEntry *next;

    entry = attribArrayEntries;
    while (entry) {
        next = entry->next;
        free(entry);
        entry = next;
    }
    attribArrayEntries = NULL;
} // FreeCgAttribArrayTypes
