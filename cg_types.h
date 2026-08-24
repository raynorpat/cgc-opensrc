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
// cg_types.h
//

#if !defined(__CG_TYPES_H)
#define __CG_TYPES_H 1

typedef union Type_Rec Type;

/*
 * Sentinel stored in TypeArray::numels for a dynamically sized (unsized)
 * array.  Zero remains reserved for invalid/recovery types so that "no
 * elements" is never confused with "unknown number of elements":
 */

#define CG_ARRAY_UNSIZED (-1)

typedef enum CgScalarKind_Rec {
    CG_SCALAR_NONE = 0,
    CG_SCALAR_UNDEFINED,
    CG_SCALAR_CFLOAT,
    CG_SCALAR_CINT,
    CG_SCALAR_BOOL,
    CG_SCALAR_CHAR,
    CG_SCALAR_UCHAR,
    CG_SCALAR_SHORT,
    CG_SCALAR_USHORT,
    CG_SCALAR_INT,
    CG_SCALAR_UINT,
    CG_SCALAR_LONG,
    CG_SCALAR_ULONG,
    CG_SCALAR_FIXED,
    CG_SCALAR_HALF,
    CG_SCALAR_FLOAT,
    CG_SCALAR_DOUBLE,
    CG_SCALAR_COUNT
} CgScalarKind;

CgScalarKind GetScalarKind(const Type *type);
void SetScalarKind(Type *type, CgScalarKind kind);
int CgScalarIsCompileTime(CgScalarKind kind);
int CgScalarIsIntegral(CgScalarKind kind);
int CgScalarIsUnsigned(CgScalarKind kind);
int CgScalarIsFloating(CgScalarKind kind);
const char *CgScalarKindName(CgScalarKind kind);
int CgScalarLegacyBase(CgScalarKind kind);
Type *GetStandardTypeKind(CgScalarKind kind, int rows, int columns);
int InitCgStandardTypes(void);
void FreeCgStandardTypes(void);

/*
 * Normative conversion classification.  Ranks are ordered so that better
 * conversions compare greater: EXACT needs no cast, DYNAMIC is reserved for
 * interface types, PROMOTION adapts a compile-time constant, IMPLICIT is a
 * silent value-preserving conversion, IMPLICIT_WARN is lossy when implicit,
 * EXPLICIT is only available through a cast, and NONE is not a conversion.
 */

typedef enum CgConversionRank_Rec {
    CG_CONVERSION_NONE = 0,
    CG_CONVERSION_EXPLICIT,
    CG_CONVERSION_IMPLICIT_WARN,
    CG_CONVERSION_IMPLICIT,
    CG_CONVERSION_PROMOTION,
    CG_CONVERSION_DYNAMIC,
    CG_CONVERSION_EXACT
} CgConversionRank;

CgConversionRank CgClassifyConversion(const Type *from, const Type *to,
                                      int explicitCast);
CgConversionRank CgClassifyScalarConversion(CgScalarKind from,
                                            CgScalarKind to,
                                            int explicitCast);
Type *CgUsualArithmeticType(const Type *left, const Type *right);

#endif
