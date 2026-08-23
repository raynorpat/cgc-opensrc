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
