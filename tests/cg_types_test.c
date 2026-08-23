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
