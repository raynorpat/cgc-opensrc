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
// cg_types_test.c
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
#include "cg_types.h"
#include "cg_numeric.h"

CgStruct *Cg;
Scope *CurrentScope;

/*
 * TestRegisterNames() - Stub HAL name registration for InitSymbolTable.
 *
 */

static int TestRegisterNames(slHAL *fHAL)
{
    (void) fHAL;
    return 1;
}

/*
 * TestGetSizeof() - Minimal size query mirroring GetSizeof_HAL for the
 *                   scalar, vector, and matrix shapes built here.
 *
 */

static int TestGetSizeof(Type *fType)
{
    int len, len2;

    if (!fType)
        return 0;
    switch (GetCategory(fType)) {
    case TYPE_CATEGORY_ARRAY:
        if (IsVector(fType, &len))
            return len;
        if (IsMatrix(fType, &len, &len2))
            return len2 > len ? len * 4 : len2 * 4;
        return fType->co.size;
    default:
        return fType->co.size;
    }
}

typedef struct ConversionCase_Rec {
    CgScalarKind from;
    CgScalarKind to;
    CgConversionRank implicitRank;
    CgConversionRank explicitRank;
} ConversionCase;

static const ConversionCase cases[] = {
    { CG_SCALAR_CINT, CG_SCALAR_HALF, CG_CONVERSION_PROMOTION, CG_CONVERSION_PROMOTION },
    { CG_SCALAR_INT, CG_SCALAR_FLOAT, CG_CONVERSION_IMPLICIT, CG_CONVERSION_IMPLICIT },
    { CG_SCALAR_DOUBLE, CG_SCALAR_HALF, CG_CONVERSION_IMPLICIT_WARN, CG_CONVERSION_EXPLICIT },
    { CG_SCALAR_BOOL, CG_SCALAR_FLOAT, CG_CONVERSION_IMPLICIT, CG_CONVERSION_IMPLICIT },
    { CG_SCALAR_FLOAT, CG_SCALAR_BOOL, CG_CONVERSION_IMPLICIT, CG_CONVERSION_IMPLICIT }
};

int main(void)
{
    CgStruct cg;
    slHAL hal;
    Type type;

    memset(&cg, 0, sizeof(cg));
    memset(&hal, 0, sizeof(hal));
    hal.GetSizeof = TestGetSizeof;
    hal.RegisterNames = TestRegisterNames;
    cg.theHAL = &hal;
    Cg = &cg;

    assert(InitAtomTable(atable, 0));
    assert(InitSymbolTable(Cg));

    InitType(&type);
    SetScalarKind(&type, CG_SCALAR_FLOAT);
    assert(GetScalarKind(&type) == CG_SCALAR_FLOAT);
    assert(CgScalarIsFloating(CG_SCALAR_FLOAT));
    assert(!CgScalarIsIntegral(CG_SCALAR_FLOAT));
    assert(CG_SCALAR_DOUBLE > 15);

    assert(GetScalarKind(GetStandardTypeKind(CG_SCALAR_CHAR, 0, 0)) == CG_SCALAR_CHAR);
    assert(GetScalarKind(GetStandardTypeKind(CG_SCALAR_ULONG, 4, 0)) == CG_SCALAR_ULONG);
    assert(GetScalarKind(GetStandardTypeKind(CG_SCALAR_HALF, 3, 2)) == CG_SCALAR_HALF);
    assert(IsVector(GetStandardTypeKind(CG_SCALAR_FIXED, 4, 0), NULL));
    assert(IsMatrix(GetStandardTypeKind(CG_SCALAR_DOUBLE, 4, 4), NULL, NULL));
    assert(GetStandardTypeKind(CG_SCALAR_FLOAT, 4, 4) ==
           GetStandardTypeKind(CG_SCALAR_FLOAT, 4, 4));

    {
        size_t ii;

        for (ii = 0; ii < sizeof(cases) / sizeof(cases[0]); ii++) {
            assert(CgClassifyScalarConversion(cases[ii].from, cases[ii].to, 0) ==
                   cases[ii].implicitRank);
            assert(CgClassifyScalarConversion(cases[ii].from, cases[ii].to, 1) ==
                   cases[ii].explicitRank);
        }
        assert(CgClassifyScalarConversion(CG_SCALAR_NONE, CG_SCALAR_FLOAT, 0) ==
               CG_CONVERSION_NONE);
        assert(CgClassifyScalarConversion(CG_SCALAR_INT, CG_SCALAR_INT, 0) ==
               CG_CONVERSION_EXACT);

        assert(GetScalarKind(CgUsualArithmeticType(
                   GetStandardTypeKind(CG_SCALAR_CINT, 0, 0),
                   GetStandardTypeKind(CG_SCALAR_HALF, 0, 0))) == CG_SCALAR_HALF);
        assert(GetScalarKind(CgUsualArithmeticType(
                   GetStandardTypeKind(CG_SCALAR_CFLOAT, 0, 0),
                   GetStandardTypeKind(CG_SCALAR_FIXED, 0, 0))) == CG_SCALAR_FIXED);
        assert(GetScalarKind(CgUsualArithmeticType(
                   GetStandardTypeKind(CG_SCALAR_INT, 0, 0),
                   GetStandardTypeKind(CG_SCALAR_UINT, 0, 0))) == CG_SCALAR_UINT);
        assert(GetScalarKind(CgUsualArithmeticType(
                   GetStandardTypeKind(CG_SCALAR_SHORT, 0, 0),
                   GetStandardTypeKind(CG_SCALAR_USHORT, 0, 0))) == CG_SCALAR_USHORT);
        assert(GetScalarKind(CgUsualArithmeticType(
                   GetStandardTypeKind(CG_SCALAR_LONG, 0, 0),
                   GetStandardTypeKind(CG_SCALAR_ULONG, 0, 0))) == CG_SCALAR_ULONG);
        assert(GetScalarKind(CgUsualArithmeticType(
                   GetStandardTypeKind(CG_SCALAR_INT, 0, 0),
                   GetStandardTypeKind(CG_SCALAR_FLOAT, 0, 0))) == CG_SCALAR_FLOAT);
    }

    /* First-class arrays: an identical aggregate converts exactly
     * without a cast; differing elements keep needing the explicit
     * form even at equal lengths. */
    {
        Type ints;
        Type intsAgain;
        Type floats;
        Type unsized;

        InitType(&ints);
        ints.properties = TYPE_BASE_INT | TYPE_CATEGORY_ARRAY;
        ints.arr.eltype = GetStandardTypeKind(CG_SCALAR_INT, 0, 0);
        ints.arr.numels = 4;
        ints.arr.scalarKind = CG_SCALAR_INT;
        intsAgain = ints;
        InitType(&floats);
        floats.properties = TYPE_BASE_FLOAT | TYPE_CATEGORY_ARRAY;
        floats.arr.eltype = GetStandardTypeKind(CG_SCALAR_FLOAT, 0, 0);
        floats.arr.numels = 4;
        floats.arr.scalarKind = CG_SCALAR_FLOAT;

        assert(CgClassifyConversion(&ints, &ints, 0) ==
               CG_CONVERSION_EXACT);
        assert(CgClassifyConversion(&ints, &intsAgain, 0) ==
               CG_CONVERSION_EXACT);
        assert(CgClassifyConversion(&floats, &ints, 0) ==
               CG_CONVERSION_NONE);
        assert(CgClassifyConversion(&floats, &ints, 1) !=
               CG_CONVERSION_NONE);

        /* A concrete array binds exactly to an unsized array of the
         * same element shape; the runtime length travels with the
         * value. */
        InitType(&unsized);
        unsized.properties = ints.properties;
        unsized.arr.eltype = ints.arr.eltype;
        unsized.arr.numels = CG_ARRAY_UNSIZED;
        unsized.arr.scalarKind = CG_SCALAR_INT;
        assert(CgClassifyConversion(&ints, &unsized, 0) ==
               CG_CONVERSION_EXACT);
        assert(CgClassifyConversion(&intsAgain, &unsized, 0) ==
               CG_CONVERSION_EXACT);
        assert(CgClassifyConversion(&floats, &unsized, 0) ==
               CG_CONVERSION_NONE);

        /* Opaque samplers convert only within their own identity or
         * through the compatible base-sampler binding. */
        {
            Type *samp2D = GetSamplerType(CG_SAMPLER_2D);
            Type *sampBase = GetSamplerType(CG_SAMPLER_BASE);
            Type *sampCube = GetSamplerType(CG_SAMPLER_CUBE);

            assert(samp2D != NULL && sampBase != NULL &&
                   sampCube != NULL);
            assert(CgClassifyConversion(samp2D, samp2D, 0) ==
                   CG_CONVERSION_EXACT);
            assert(CgClassifyConversion(samp2D, sampBase, 0) ==
                   CG_CONVERSION_EXACT);
            assert(CgClassifyConversion(samp2D, sampCube, 0) ==
                   CG_CONVERSION_NONE);
        }
    }

    {
        CgNumericValue input;
        CgNumericValue output;

        CgNumericSetSigned(&input, CG_SCALAR_LONG, -2);
        assert(CgNumericConvert(&output, CG_SCALAR_ULONG, &input));
        assert(output.kind == CG_SCALAR_ULONG);
        CgNumericSetFloat(&input, CG_SCALAR_FIXED, 3.0);
        assert(CgNumericNormalize(&output, &input));
        assert(output.value.f < 2.0);
        CgNumericSetFloat(&input, CG_SCALAR_HALF, 1.0 / 3.0);
        assert(CgNumericNormalize(&output, &input));
        assert(output.kind == CG_SCALAR_HALF);
    }

    FreeSymbolTable(Cg);
    FreeAtomTable(atable);
    return 0;
}

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
