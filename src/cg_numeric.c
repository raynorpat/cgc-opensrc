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
// cg_numeric.c
//

#include <math.h>

#include "cg_numeric.h"

/*
 * lIsIntegralKind() - Return TRUE for canonical kinds stored as integers.
 *
 */

static int lIsIntegralKind(CgScalarKind kind)
{
    switch (kind) {
    case CG_SCALAR_CINT:
    case CG_SCALAR_BOOL:
    case CG_SCALAR_CHAR:
    case CG_SCALAR_UCHAR:
    case CG_SCALAR_SHORT:
    case CG_SCALAR_USHORT:
    case CG_SCALAR_INT:
    case CG_SCALAR_UINT:
    case CG_SCALAR_LONG:
    case CG_SCALAR_ULONG:
        return 1;
    default:
        return 0;
    }
} // lIsIntegralKind

/*
 * lIsUnsignedKind() - Return TRUE for canonical unsigned integer kinds.
 *
 */

static int lIsUnsignedKind(CgScalarKind kind)
{
    switch (kind) {
    case CG_SCALAR_UCHAR:
    case CG_SCALAR_USHORT:
    case CG_SCALAR_UINT:
    case CG_SCALAR_ULONG:
        return 1;
    default:
        return 0;
    }
} // lIsUnsignedKind

/*
 * lIsFloatingKind() - Return TRUE for canonical floating point kinds.
 *
 */

static int lIsFloatingKind(CgScalarKind kind)
{
    switch (kind) {
    case CG_SCALAR_CFLOAT:
    case CG_SCALAR_FIXED:
    case CG_SCALAR_HALF:
    case CG_SCALAR_FLOAT:
    case CG_SCALAR_DOUBLE:
        return 1;
    default:
        return 0;
    }
} // lIsFloatingKind

/*
 * lKindBits() - Exact width in bits of an integral canonical kind.
 *
 */

static int lKindBits(CgScalarKind kind)
{
    switch (kind) {
    case CG_SCALAR_CHAR:
    case CG_SCALAR_UCHAR:
        return 8;
    case CG_SCALAR_SHORT:
    case CG_SCALAR_USHORT:
        return 16;
    case CG_SCALAR_CINT:
    case CG_SCALAR_BOOL:
    case CG_SCALAR_INT:
    case CG_SCALAR_UINT:
        return 32;
    case CG_SCALAR_LONG:
    case CG_SCALAR_ULONG:
        return 64;
    default:
        return 0;
    }
} // lKindBits

/*
 * lValidKind() - Return TRUE if the kind carries numeric values.
 *
 */

static int lValidKind(CgScalarKind kind)
{
    return lIsIntegralKind(kind) || lIsFloatingKind(kind);
} // lValidKind

/*
 * lTruncateSigned() - Two's-complement truncation to the kind's width.
 *
 */

static CgInt64 lTruncateSigned(CgInt64 data, int bits)
{
    int shift = 64 - bits;

    return (CgInt64)((CgUInt64)data << shift) >> shift;
} // lTruncateSigned

/*
 * lMaskUnsigned() - Mask an unsigned value to the kind's width.
 *
 */

static CgUInt64 lMaskUnsigned(CgUInt64 data, int bits)
{
    if (bits >= 64)
        return data;
    return data & (((CgUInt64)1 << bits) - 1);
} // lMaskUnsigned

/*
 * lGetDouble() - Host double value of any numeric kind.
 *
 */

static double lGetDouble(const CgNumericValue *value)
{
    if (lIsFloatingKind(value->kind))
        return value->value.f;
    if (lIsUnsignedKind(value->kind))
        return (double)value->value.u;
    return (double)value->value.i;
} // lGetDouble

/*
 * NormalizeFixed() - Round/clamp to fixed (signed 2.10) precision.
 *
 */

static double NormalizeFixed(double value)
{
    const double maximum = 2.0 - 1.0 / 1024.0;
    if (value < -2.0)
        return -2.0;
    if (value > maximum)
        return maximum;
    return floor(value * 1024.0 + (value >= 0.0 ? 0.5 : -0.5)) / 1024.0;
} // NormalizeFixed

/*
 * CgNumericRoundHalf() - Round a value to half (S5.10, bias=14) precision.
 *
 */

float CgNumericRoundHalf(double v)
{
    int exp;
    double mant = frexp(v, &exp);
    int rndm = (int)(mant * 2048 + 0.5);
    if (exp > 17) {
        // overflow -- build the appropriately signed infinity
        v = ldexp(mant, 500) * 2;
    } else if (exp < -23) {
        // full underflow -- build appropraitely signed zero
        v = ldexp(mant, -500);
    } else {
        if (exp < -13) {
            // underflow -- round more to show denorm
            rndm >>= -(exp - 13);
            rndm <<= -(exp - 13);
        }
        v = ldexp(rndm/2048.0, exp);
    }
    return (float)v;
} // CgNumericRoundHalf

/*
 * CgNumericSetSigned() - Store a signed integer datum of the given kind.
 *
 */

void CgNumericSetSigned(CgNumericValue *value, CgScalarKind kind, CgInt64 data)
{
    value->kind = kind;
    value->value.i = data;
} // CgNumericSetSigned

/*
 * CgNumericSetUnsigned() - Store an unsigned integer datum of the given kind.
 *
 */

void CgNumericSetUnsigned(CgNumericValue *value, CgScalarKind kind, CgUInt64 data)
{
    value->kind = kind;
    value->value.u = data;
} // CgNumericSetUnsigned

/*
 * CgNumericSetFloat() - Store a floating point datum of the given kind.
 *
 */

void CgNumericSetFloat(CgNumericValue *value, CgScalarKind kind, double data)
{
    value->kind = kind;
    value->value.f = data;
} // CgNumericSetFloat

/*
 * CgNumericNormalize() - Clamp/truncate a value to its own kind's range.
 *
 * Signed integers are truncated two's-complement style, unsigned integers
 * are masked, cfloat/float pass through a float temporary, half rounds to
 * half precision, fixed rounds/clamps to signed 2.10, and double is kept
 * directly.  Returns FALSE only for kinds that carry no numeric value.
 *
 */

int CgNumericNormalize(CgNumericValue *result, const CgNumericValue *value)
{
    CgNumericValue tmp;
    int bits;

    if (!value || !lValidKind(value->kind))
        return 0;

    tmp.kind = value->kind;
    if (lIsIntegralKind(value->kind)) {
        bits = lKindBits(value->kind);
        if (lIsUnsignedKind(value->kind))
            tmp.value.u = lMaskUnsigned(value->value.u, bits);
        else
            tmp.value.i = lTruncateSigned(value->value.i, bits);
    } else {
        switch (value->kind) {
        case CG_SCALAR_HALF:
            tmp.value.f = CgNumericRoundHalf(value->value.f);
            break;
        case CG_SCALAR_FIXED:
            tmp.value.f = NormalizeFixed(value->value.f);
            break;
        case CG_SCALAR_DOUBLE:
            tmp.value.f = value->value.f;
            break;
        default: // cfloat and float pass through a float temporary:
            tmp.value.f = (double)(float)value->value.f;
            break;
        }
    }
    *result = tmp;
    return 1;
} // CgNumericNormalize

/*
 * CgNumericConvert() - Convert a value between canonical scalar kinds.
 *
 * Conversions to bool test the source for a nonzero value in its own
 * domain.  Floating to integral conversions fail when the datum lies
 * outside the 64-bit host range or is NaN; everything else wraps or
 * rounds through CgNumericNormalize.  Returns TRUE on success.
 *
 */

int CgNumericConvert(CgNumericValue *result, CgScalarKind kind,
                     const CgNumericValue *value)
{
    CgNumericValue tmp;
    int nonzero;

    if (!value || !lValidKind(kind) || !lValidKind(value->kind))
        return 0;

    if (kind == CG_SCALAR_BOOL) {
        if (lIsFloatingKind(value->kind))
            nonzero = (value->value.f != 0.0);
        else if (lIsUnsignedKind(value->kind))
            nonzero = (value->value.u != 0);
        else
            nonzero = (value->value.i != 0);
        CgNumericSetSigned(&tmp, kind, nonzero);
        return CgNumericNormalize(result, &tmp);
    }

    if (lIsFloatingKind(kind)) {
        CgNumericSetFloat(&tmp, kind, lGetDouble(value));
        return CgNumericNormalize(result, &tmp);
    }

    if (lIsFloatingKind(value->kind)) {
        double d = value->value.f;

        if (d != d || d < -9223372036854775808.0 ||
            d >= 18446744073709551616.0)
        {
            return 0;
        }
        CgNumericSetSigned(&tmp, kind, (CgInt64)d);
        return CgNumericNormalize(result, &tmp);
    }

    tmp.kind = kind;
    tmp.value.u = value->value.u;
    return CgNumericNormalize(result, &tmp);
} // CgNumericConvert

/*
 * CgNumericBinary() - Evaluate one primitive operation on typed values.
 *
 * The operation runs in the domain of the left operand's kind: doubles for
 * floating kinds, exact 64-bit host arithmetic for integers.  Division by
 * zero, INT64_MIN / -1, and out-of-range shift counts are rejected before
 * the host operation so no undefined behavior can occur; unary operations
 * pass NULL for "right".  Returns TRUE when "*result" holds the outcome.
 *
 */

int CgNumericBinary(CgNumericValue *result, CgNumericOp op,
                    const CgNumericValue *left,
                    const CgNumericValue *right)
{
    CgNumericValue a, b;
    CgScalarKind kind;
    int bits;

    if (!left || !lValidKind(left->kind))
        return 0;
    if (op != CG_NUMERIC_NEG && op != CG_NUMERIC_NOT &&
        op != CG_NUMERIC_BIT_NOT)
    {
        if (!right || !lValidKind(right->kind))
            return 0;
    }
    kind = left->kind;
    a = *left;
    if (right && op != CG_NUMERIC_SHIFT_LEFT &&
        op != CG_NUMERIC_SHIFT_RIGHT &&
        right->kind != kind)
    {
        // Shift counts stay full-width ints instead of converting down
        // to the left operand's kind:
        if (!CgNumericConvert(&b, kind, right))
            return 0;
    } else if (right) {
        b = *right;
    }

    if (lIsFloatingKind(kind)) {
        double da = lGetDouble(&a);
        double db = right ? lGetDouble(&b) : 0.0;
        double dr;

        switch (op) {
        case CG_NUMERIC_NEG:
            dr = -da;
            break;
        case CG_NUMERIC_NOT:
            dr = (da == 0.0);
            break;
        case CG_NUMERIC_ADD:
            dr = da + db;
            break;
        case CG_NUMERIC_SUB:
            dr = da - db;
            break;
        case CG_NUMERIC_MUL:
            dr = da * db;
            break;
        case CG_NUMERIC_DIV:
            dr = da / db;
            break;
        case CG_NUMERIC_MOD:
            dr = fmod(da, db);
            break;
        default: // bitwise and shift operations are integral-only:
            return 0;
        }
        CgNumericSetFloat(&a, kind, dr);
        return CgNumericNormalize(result, &a);
    }

    bits = lKindBits(kind);
    {
        CgUInt64 ua = a.value.u;
        CgUInt64 ub = right ? b.value.u : 0;
        CgInt64 ia = a.value.i;
        CgInt64 ib = right ? b.value.i : 0;
        CgUInt64 ur;
        CgInt64 ir;
        int scount;

        switch (op) {
        case CG_NUMERIC_NEG:
            ur = 0 - ua; // modular negate, well-defined for all widths
            CgNumericSetUnsigned(&a, kind, ur);
            return CgNumericNormalize(result, &a);
        case CG_NUMERIC_NOT:
            CgNumericSetSigned(&a, kind, ia == 0);
            return CgNumericNormalize(result, &a);
        case CG_NUMERIC_BIT_NOT:
            ur = ~ua;
            CgNumericSetUnsigned(&a, kind, ur);
            return CgNumericNormalize(result, &a);
        case CG_NUMERIC_ADD:
            ur = ua + ub;
            break;
        case CG_NUMERIC_SUB:
            ur = ua - ub;
            break;
        case CG_NUMERIC_MUL:
            ur = ua * ub;
            break;
        case CG_NUMERIC_DIV:
        case CG_NUMERIC_MOD:
            if (ub == 0)
                return 0;
            if (!lIsUnsignedKind(kind) && ib == -1 &&
                ua == ((CgUInt64)1 << 63))
            {
                return 0;
            }
            if (lIsUnsignedKind(kind))
                ur = (op == CG_NUMERIC_DIV) ? ua / ub : ua % ub;
            else {
                ir = (op == CG_NUMERIC_DIV) ? ia / ib : ia % ib;
                CgNumericSetSigned(&a, kind, ir);
                return CgNumericNormalize(result, &a);
            }
            break;
        case CG_NUMERIC_BIT_AND:
            ur = ua & ub;
            break;
        case CG_NUMERIC_BIT_OR:
            ur = ua | ub;
            break;
        case CG_NUMERIC_BIT_XOR:
            ur = ua ^ ub;
            break;
        case CG_NUMERIC_SHIFT_LEFT:
        case CG_NUMERIC_SHIFT_RIGHT:
            if (ub > (CgUInt64)0x7fffffff)
                return 0;
            scount = (int)ub;
            if (scount >= bits)
                return 0;
            if (op == CG_NUMERIC_SHIFT_LEFT) {
                ur = ua << scount;
            } else {
                if (lIsUnsignedKind(kind)) {
                    ur = ua >> scount;
                } else {
                    CgNumericSetSigned(&a, kind, ia >> scount);
                    return CgNumericNormalize(result, &a);
                }
            }
            break;
        default:
            return 0;
        }
        CgNumericSetUnsigned(&a, kind, ur);
        return CgNumericNormalize(result, &a);
    }
} // CgNumericBinary
