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
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT
LIABILITY OR OTHERWISE, EVEN IF NVIDIA HAS BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

//
// cg_numeric.h
//

#if !defined(__CG_NUMERIC_H)
#define __CG_NUMERIC_H 1

#include "cg_types.h"

#if defined(CGC_HAVE_STDINT_H)
#include <stdint.h>
typedef int64_t CgInt64;
typedef uint64_t CgUInt64;
#elif defined(_MSC_VER)
typedef __int64 CgInt64;
typedef unsigned __int64 CgUInt64;
#else
typedef long long CgInt64;
typedef unsigned long long CgUInt64;
#endif

typedef struct CgNumericValue_Rec {
    CgScalarKind kind;
    union {
        CgInt64 i;
        CgUInt64 u;
        double f;
    } value;
} CgNumericValue;

typedef enum CgNumericOp_Rec {
    CG_NUMERIC_NEG,
    CG_NUMERIC_NOT,
    CG_NUMERIC_BIT_NOT,
    CG_NUMERIC_ADD,
    CG_NUMERIC_SUB,
    CG_NUMERIC_MUL,
    CG_NUMERIC_DIV,
    CG_NUMERIC_MOD,
    CG_NUMERIC_BIT_AND,
    CG_NUMERIC_BIT_OR,
    CG_NUMERIC_BIT_XOR,
    CG_NUMERIC_SHIFT_LEFT,
    CG_NUMERIC_SHIFT_RIGHT
} CgNumericOp;

void CgNumericSetSigned(CgNumericValue *value, CgScalarKind kind, CgInt64 data);
void CgNumericSetUnsigned(CgNumericValue *value, CgScalarKind kind, CgUInt64 data);
void CgNumericSetFloat(CgNumericValue *value, CgScalarKind kind, double data);
int CgNumericNormalize(CgNumericValue *result, const CgNumericValue *value);
int CgNumericConvert(CgNumericValue *result, CgScalarKind kind,
                     const CgNumericValue *value);
int CgNumericBinary(CgNumericValue *result, CgNumericOp op,
                    const CgNumericValue *left,
                    const CgNumericValue *right);
float CgNumericRoundHalf(double value);

#endif
