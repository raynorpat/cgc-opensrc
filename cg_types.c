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
