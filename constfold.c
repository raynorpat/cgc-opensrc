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
// constfold.c
//

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
//#include <string.h>

#include "slglobals.h"

#if 0
#define DB(X)   X
#else
#define DB(X)
#endif

/*
 * Folding is indexed by canonical scalar kind: every operation below
 * delegates to cg_numeric, which normalizes by the value's stored kind.
 */

static int lValidFoldKind(CgScalarKind kind);
static CgScalarKind FoldKindOf(CgScalarKind stored, int base);
static operations *KindOps(CgScalarKind kind);
static operations *BaseOps(int base);
static int BinOpAvailable(operations *ops, CgNumericOp numop);
static int (*CmpFn(operations *ops, opcode op))
    (const scalar_constant *, const scalar_constant *);
static operations *kind_ops[CG_SCALAR_COUNT];

// IsConstant(expr *)
// return true iff the argument is a constant
static int IsConstant(expr *fexpr)
{
    return fexpr && fexpr->common.kind == CONST_N;
} // IsConstant

// IsConstList(expr *)
// return true iff the argument is a constant or a list of constants
static int IsConstList(expr *fexpr)
{
    while (fexpr &&
           fexpr->common.kind == BINARY_N &&
           fexpr->bin.op == EXPR_LIST_OP
    ) {
        if (!IsConstant(fexpr->bin.left)) return 0;
        if (!(fexpr = fexpr->bin.right)) return 1;
    }
    return fexpr && fexpr->common.kind == CONST_N;
} // IsConstList

// NewConst
// create a new blank constant node we can fill in
// This is a bit of a hack, as the opcode field is really redundant with
// the type info in the node, so we just create an opcode that's as close
// as possible to what we want -- which sometimes may not exist (vectors
// other than float
static expr *NewConst(int base, int len)
{
    float       tmp[4] = { 0, 0, 0, 0 };
    opcode      op = FCONST_OP;
    operations  *ops;

    // This is a bit of a hack -- we use NewFConstNodeV to allocate a node
    // even for non-FCONST_V nodes.  It turns out that it works out ok.
    ops = BaseOps(base);
    if (ops) op = ops->const_opcode;
    if (len) op++;
    return (expr *)NewFConstNodeV(op, tmp, len, base);
}

// GetConstVal
// get the actual value from a constant node
static scalar_constant *GetConstVal(expr *constexpr) {
    if (!constexpr || constexpr->common.kind != CONST_N) return 0;
    return constexpr->co.val;
} // GetConstVal

typedef struct constlist_iter {
    expr        *curr, *rest;
    int         i, lim;
} constlist_iter;

static void InitConstList_iter(constlist_iter *iter, expr *fexpr)
{
    iter->curr = 0;
    iter->rest = fexpr;
    iter->i = iter->lim = 0;
} // InitConstList_iter

static scalar_constant *ConstListNext(constlist_iter *iter) {
    if (iter->i >= iter->lim) {
        do {
            if (!iter->rest) return 0;
            if (iter->rest->common.kind == BINARY_N &&
                iter->rest->bin.op == EXPR_LIST_OP
            ) {
                iter->curr = iter->rest->bin.left;
                iter->rest = iter->rest->bin.right;
            } else if (iter->rest->common.kind == CONST_N) {
                iter->curr = iter->rest;
                iter->rest = 0;
            } else {
                iter->rest = 0;
                return 0;
            }
        } while (!iter->curr || iter->curr->common.kind != CONST_N);
        iter->i = 0;
        iter->lim = SUBOP_GET_S(iter->curr->co.subop);
    }
    return &iter->curr->co.val[iter->i++];
} // ConstListNext

DB(
static void pconst(scalar_constant *v, int type) {
    switch(type) {
    case TYPE_BASE_BOOLEAN:
        printf("%c", v->value.i ? 'T' : 'F');
        break;
    case TYPE_BASE_INT:
    case TYPE_BASE_CINT:
        printf("%d", (int)v->value.i);
        break;
    default:
        printf("%g", v->value.f);
        break;
    }
}

static void DumpConstList(expr *fexpr, int type) {
    constlist_iter      iter;
    int                 first = 1;
    scalar_constant     *v;

    InitConstList_iter(&iter, fexpr);
    printf("(");
    while ((v = ConstListNext(&iter))) {
        if (first) first = 0;
        else printf(", ");
        pconst(v, type);
    }
    printf(")");
}
)

/*
 * FoldConstants() - Fold this expression if possible.
 *
 */

expr *FoldConstants(expr *fexpr)
{
    return PostApplyToNodes(ConstantFoldNode, fexpr, 0, 0);
}


// ConstantFoldNode
// all the real work is done here
expr *ConstantFoldNode(expr *fexpr, void *_arg1, int arg2)
{
    expr *rv = fexpr;
    int base, target;
    int len;
    int i;
    scalar_constant *a1, *a2;
    scalar_constant tmp[4];
    operations *ops;
    CgScalarKind kind, tkind;
    CgNumericOp numop;
    int failed;
    int op_offset, a1_mask, a2_mask;
    if (!fexpr) return fexpr;
    switch(fexpr->common.kind) {
    case UNARY_N:
        base = SUBOP_GET_T(fexpr->un.subop);
        len = SUBOP_GET_S(fexpr->un.subop);
        if (fexpr->un.op == VECTOR_V_OP &&
            (!IsMatrix(fexpr->common.type, NULL, NULL) ||
             !Cg->theHAL->GetCapsBit(CAPS_MATRIX_CONSTRUCTOR_AST)) &&
            (!Cg->theHAL->GetCapsBit(CAPS_AGGREGATE_DEFAULT_BINDINGS) ||
             GetCategory(fexpr->common.type) != TYPE_CATEGORY_ARRAY ||
             IsVector(fexpr->common.type, NULL) ||
             IsMatrix(fexpr->common.type, NULL, NULL)) &&
            IsConstList(fexpr->un.arg)) {
            constlist_iter      iter;
            DB (printf("fold VECTOR_V_OP[%d:%d]", len, base);
                DumpConstList(fexpr->un.arg, base);
                printf(" -> ");)
            InitConstList_iter(&iter, fexpr->un.arg);
            rv = NewConst(base, len);
            for (i=0; i<len; i++) {
                rv->co.val[i] = *ConstListNext(&iter);
                DB (if (i) printf(", ");
                    pconst(&rv->co.val[i], base);)
            }
            DB(printf("\n");)
            return rv;
        }
        if (!IsConstant(fexpr->un.arg)) break;
        a1 = GetConstVal(fexpr->un.arg);
        kind = FoldKindOf(a1[0].kind, base);
        ops = KindOps(kind);
        if (!ops) break;
        switch (fexpr->un.op) {
        case SWIZZLE_Z_OP: {
            int mask = SUBOP_GET_MASK(fexpr->un.subop);
            len = SUBOP_GET_S2(fexpr->un.subop);
            DB (printf("fold SWIZ[%d:%d].", len, base);
                for (i=0; i<len; i++)
                    putchar("xyzw"[(mask>>(i*2))&3]);
                DumpConstList(fexpr->un.arg, base);
                printf(" -> ");)
            rv = NewConst(base, len);
            for (i=0; i==0 || i<len; i++) {
                rv->co.val[i] = a1[mask&3];
                mask >>= 2;
                DB (if (i) printf(", ");
                    pconst(&rv->co.val[i], base);)
            }
            DB(printf("\n");)
            break; }
        case SWIZMAT_Z_OP:
            break;
        case CAST_CS_OP:
        case CAST_CV_OP:
            target = SUBOP_GET_T2(fexpr->un.subop);
            tkind = FoldKindOf(CG_SCALAR_UNDEFINED, target);
            DB (printf("fold CAST[%d:%d->%d]", len, base, target);
                DumpConstList(fexpr->un.arg, base);
                printf(" -> ");)
            if (!lValidFoldKind(tkind)) {
                DB(printf("no target kind, abort\n");)
                break;
            }
            failed = 0;
            for (i=0; i==0 || i<len; i++)
                if (!CgNumericConvert(&tmp[i], tkind, &a1[i]))
                    failed = 1;
            if (failed) {
                DB(printf("conversion failed, abort\n");)
                break;
            }
            rv = NewConst(target, len);
            for (i=0; i==0 || i<len; i++) {
                rv->co.val[i] = tmp[i];
                DB (if (i) printf(", ");
                    pconst(&rv->co.val[i], target);)
            }
            DB(printf("\n");)
            break;
        case CAST_CM_OP:
            break;
        case NEG_OP:
        case NEG_V_OP:
            DB (printf("fold NEG[%d:%d]", len, base);
                DumpConstList(fexpr->un.arg, base);
                printf(" -> ");)
            if (!ops->op_neg) {
                DB(printf("no function, abort\n");)
                break;
            }
            rv = NewConst(base, len);
            for (i=0; i==0 || i<len; i++) {
                ops->op_neg(&rv->co.val[i], &a1[i]);
                DB (if (i) printf(", ");
                    pconst(&rv->co.val[i], base);)
            }
            DB(printf("\n");)
            break;
        case POS_OP:
        case POS_V_OP:
            rv = fexpr->un.arg;
            break;
        case NOT_OP:
        case NOT_V_OP:
            DB (printf("fold NOT[%d:%d]", len, base);
                DumpConstList(fexpr->un.arg, base);
                printf(" -> ");)
            if (!ops->op_not) {
                DB(printf("no function, abort\n");)
                break;
            }
            rv = NewConst(base, len);
            for (i=0; i==0 || i<len; i++) {
                ops->op_not(&rv->co.val[i], &a1[i]);
                DB (if (i) printf(", ");
                    pconst(&rv->co.val[i], base);)
            }
            DB(printf("\n");)
            break;
        case BNOT_OP:
        case BNOT_V_OP:
            DB (printf("fold BNOT[%d:%d]", len, base);
                DumpConstList(fexpr->un.arg, base);
                printf(" -> ");)
            if (!ops->op_bnot) {
                DB(printf("no function, abort\n");)
                break;
            }
            rv = NewConst(base, len);
            for (i=0; i==0 || i<len; i++) {
                ops->op_bnot(&rv->co.val[i], &a1[i]);
                DB (if (i) printf(", ");
                    pconst(&rv->co.val[i], base);)
            }
            DB(printf("\n");)
            break;
        default:
            break;
        }
        break;
    case BINARY_N:
        if (!IsConstant(fexpr->bin.left) ||
            !IsConstant(fexpr->bin.right)
        )
            break;
        base = SUBOP_GET_T(fexpr->bin.subop);
        len = SUBOP_GET_S(fexpr->bin.subop);
        a1 = GetConstVal(fexpr->bin.left);
        a2 = GetConstVal(fexpr->bin.right);
        kind = FoldKindOf(a1[0].kind, base);
        ops = KindOps(kind);
        switch(fexpr->bin.op) {
        case MEMBER_SELECTOR_OP:
        case ARRAY_INDEX_OP:
        case FUN_CALL_OP:
        case FUN_BUILTIN_OP:
        case FUN_ARG_OP:
        case EXPR_LIST_OP:
            break;
        case MUL_OP:
        case MUL_V_OP:
        case MUL_SV_OP:
        case MUL_VS_OP:
            DB (printf("fold MUL");)
            numop = CG_NUMERIC_MUL;
            op_offset = fexpr->bin.op - MUL_OP;
            goto normal_binop;
        normal_binop:
            DB (printf("[%d:%d]", len, base);
                DumpConstList(fexpr->bin.left, base);
                DumpConstList(fexpr->bin.right, base);
                printf(" -> ");)
            if (!BinOpAvailable(ops, numop)) {
                DB(printf("no function, abort\n");)
                break;
            }
            failed = 0;
            a1_mask = 0 - (op_offset&1);
            a2_mask = 0 - (((op_offset>>1)^op_offset)&1);
            for (i=0; i==0 || i<len; i++)
                if (!CgNumericBinary(&tmp[i], numop,
                                     &a1[i & a1_mask],
                                     &a2[i & a2_mask]))
                    failed = 1;
            if (failed) {
                DB(printf("operation refused, abort\n");)
                break;
            }
            rv = NewConst(base, len);
            for (i=0; i==0 || i<len; i++) {
                rv->co.val[i] = tmp[i];
                DB (if (i) printf(", ");
                    pconst(&rv->co.val[i], base);)
            }
            DB(printf("\n");)
            break;
        case DIV_OP:
        case DIV_V_OP:
        case DIV_SV_OP:
        case DIV_VS_OP:
            DB (printf("fold DIV");)
            numop = CG_NUMERIC_DIV;
            op_offset = fexpr->bin.op - DIV_OP;
            goto normal_binop;
        case MOD_OP:
        case MOD_V_OP:
        case MOD_SV_OP:
        case MOD_VS_OP:
            DB (printf("fold MOD");)
            numop = CG_NUMERIC_MOD;
            op_offset = fexpr->bin.op - MOD_OP;
            goto normal_binop;
        case ADD_OP:
        case ADD_V_OP:
        case ADD_SV_OP:
        case ADD_VS_OP:
            DB (printf("fold ADD");)
            numop = CG_NUMERIC_ADD;
            op_offset = fexpr->bin.op - ADD_OP;
            goto normal_binop;
        case SUB_OP:
        case SUB_V_OP:
        case SUB_SV_OP:
        case SUB_VS_OP:
            DB (printf("fold SUB");)
            numop = CG_NUMERIC_SUB;
            op_offset = fexpr->bin.op - SUB_OP;
            goto normal_binop;
        case SHL_OP:
        case SHL_V_OP:
            DB (printf("fold SHL");)
            goto normal_shiftop;
        normal_shiftop:
            DB (printf("[%d:%d]", len, base);
                DumpConstList(fexpr->bin.left, base);
                DumpConstList(fexpr->bin.right, TYPE_BASE_CINT);
                printf(" -> ");)
            if (!ops || !(fexpr->bin.op == SHL_OP ||
                          fexpr->bin.op == SHL_V_OP
                          ? ops->op_shl : ops->op_shr))
            {
                DB(printf("no function, abort\n");)
                break;
            }
            failed = 0;
            for (i=0; i==0 || i<len; i++) {
                if (fexpr->bin.op == SHL_OP || fexpr->bin.op == SHL_V_OP)
                    failed |= !ops->op_shl(&tmp[i], &a1[i],
                                           (int) a2->value.i);
                else
                    failed |= !ops->op_shr(&tmp[i], &a1[i],
                                           (int) a2->value.i);
            }
            if (failed) {
                DB(printf("invalid shift, abort\n");)
                break;
            }
            rv = NewConst(base, len);
            for (i=0; i==0 || i<len; i++) {
                rv->co.val[i] = tmp[i];
                DB (if (i) printf(", ");
                    pconst(&rv->co.val[i], base);)
            }
            DB(printf("\n");)
            break;
        case SHR_OP:
        case SHR_V_OP:
            DB (printf("fold SHR");)
            goto normal_shiftop;
        case LT_OP:
        case LT_V_OP:
        case LT_SV_OP:
        case LT_VS_OP:
            DB (printf("fold LT");)
            if (!ops || !ops->op_lt) {
                DB(printf("no function, abort\n");)
                break;
            }
            op_offset = fexpr->bin.op - LT_OP;
        normal_cmpop:
            DB (printf("[%d:%d]", len, base);
                DumpConstList(fexpr->bin.left, base);
                DumpConstList(fexpr->bin.right, TYPE_BASE_CINT);
                printf(" -> ");)
            if (!ops) {
                DB(printf("no kind table, abort\n");)
                break;
            }
            rv = NewConst(TYPE_BASE_BOOLEAN, len);
            // set a1_mask to all 0s or all 1s, depending on whether a1
            // (left arg) is scalar (all 0s) or vector (all 1s).  a2_mask
            // is set according to a2.  This is dependent on the ordering
            // of the OFFSET_ tags in supprt.h
            a1_mask = 0 - (op_offset&1);
            a2_mask = 0 - (((op_offset>>1)^op_offset)&1);
            for (i=0; i==0 || i<len; i++) {
                rv->co.val[i].kind = CG_SCALAR_BOOL;
                rv->co.val[i].value.i =
                    CmpFn(ops, fexpr->bin.op)(&a1[i & a1_mask],
                                              &a2[i & a2_mask]);
                DB (if (i) printf(", ");
                    pconst(&rv->co.val[i], TYPE_BASE_BOOLEAN);)
            }
            DB(printf("\n");)
            break;
        case GT_OP:
        case GT_V_OP:
        case GT_SV_OP:
        case GT_VS_OP:
            if (!ops || !ops->op_gt) {
                DB(printf("no function, abort\n");)
                break;
            }
            op_offset = fexpr->bin.op - GT_OP;
            goto normal_cmpop;
        case LE_OP:
        case LE_V_OP:
        case LE_SV_OP:
        case LE_VS_OP:
            if (!ops || !ops->op_le) {
                DB(printf("no function, abort\n");)
                break;
            }
            op_offset = fexpr->bin.op - LE_OP;
            goto normal_cmpop;
        case GE_OP:
        case GE_V_OP:
        case GE_SV_OP:
        case GE_VS_OP:
            if (!ops || !ops->op_ge) {
                DB(printf("no function, abort\n");)
                break;
            }
            op_offset = fexpr->bin.op - GE_OP;
            goto normal_cmpop;
        case EQ_OP:
        case EQ_V_OP:
        case EQ_SV_OP:
        case EQ_VS_OP:
            if (!ops || !ops->op_eq) {
                DB(printf("no function, abort\n");)
                break;
            }
            op_offset = fexpr->bin.op - EQ_OP;
            goto normal_cmpop;
        case NE_OP:
        case NE_V_OP:
        case NE_SV_OP:
        case NE_VS_OP:
            if (!ops || !ops->op_ne) {
                DB(printf("no function, abort\n");)
                break;
            }
            op_offset = fexpr->bin.op - NE_OP;
            goto normal_cmpop;
        case AND_OP:
        case AND_V_OP:
        case AND_SV_OP:
        case AND_VS_OP:
            DB (printf("fold AND");)
            if (!ops || !ops->op_and) {
                DB(printf("no function, abort\n");)
                break;
            }
            numop = CG_NUMERIC_BIT_AND;
            op_offset = fexpr->bin.op - AND_OP;
            goto normal_binop2;
        case XOR_OP:
        case XOR_V_OP:
        case XOR_SV_OP:
        case XOR_VS_OP:
            DB (printf("fold XOR");)
            if (!ops || !ops->op_xor) {
                DB(printf("no function, abort\n");)
                break;
            }
            numop = CG_NUMERIC_BIT_XOR;
            op_offset = fexpr->bin.op - XOR_OP;
            goto normal_binop2;
        case OR_OP:
        case OR_V_OP:
        case OR_SV_OP:
        case OR_VS_OP:
            DB (printf("fold OR");)
            if (!ops || !ops->op_or) {
                DB(printf("no function, abort\n");)
                break;
            }
            numop = CG_NUMERIC_BIT_OR;
            op_offset = fexpr->bin.op - OR_OP;
            goto normal_binop2;
        normal_binop2:
            DB (printf("[%d:%d]", len, base);
                DumpConstList(fexpr->bin.left, base);
                DumpConstList(fexpr->bin.right, base);
                printf(" -> ");)
            failed = 0;
            a1_mask = 0 - (op_offset&1);
            a2_mask = 0 - (((op_offset>>1)^op_offset)&1);
            for (i=0; i==0 || i<len; i++)
                if (!CgNumericBinary(&tmp[i], numop,
                                     &a1[i & a1_mask],
                                     &a2[i & a2_mask]))
                    failed = 1;
            if (failed) {
                DB(printf("operation refused, abort\n");)
                break;
            }
            rv = NewConst(base, len);
            for (i=0; i==0 || i<len; i++) {
                rv->co.val[i] = tmp[i];
                DB (if (i) printf(", ");
                    pconst(&rv->co.val[i], base);)
            }
            DB(printf("\n");)
            break;
        case BAND_OP:
        case BAND_V_OP:
        case BAND_SV_OP:
        case BAND_VS_OP:
            DB (printf("fold BAND");)
            if (!ops || !ops->op_band) {
                DB(printf("no function, abort\n");)
                break;
            }
            op_offset = fexpr->bin.op - BAND_OP;
            goto normal_logop;
        case BOR_OP:
        case BOR_V_OP:
        case BOR_SV_OP:
        case BOR_VS_OP:
            DB (printf("fold BOR");)
            if (!ops || !ops->op_bor) {
                DB(printf("no function, abort\n");)
                break;
            }
            op_offset = fexpr->bin.op - BOR_OP;
            goto normal_logop;
        normal_logop:
            DB (printf("[%d:%d]", len, base);
                DumpConstList(fexpr->bin.left, base);
                DumpConstList(fexpr->bin.right, base);
                printf(" -> ");)
            rv = NewConst(base, len);
            a1_mask = 0 - (op_offset&1);
            a2_mask = 0 - (((op_offset>>1)^op_offset)&1);
            for (i=0; i==0 || i<len; i++) {
                if (fexpr->bin.op == BAND_OP ||
                    fexpr->bin.op == BAND_V_OP ||
                    fexpr->bin.op == BAND_SV_OP ||
                    fexpr->bin.op == BAND_VS_OP)
                    ops->op_band(&rv->co.val[i], &a1[i & a1_mask],
                                 &a2[i & a2_mask]);
                else
                    ops->op_bor(&rv->co.val[i], &a1[i & a1_mask],
                                &a2[i & a2_mask]);
                DB (if (i) printf(", ");
                    pconst(&rv->co.val[i], base);)
            }
            DB(printf("\n");)
            break;
        case ASSIGN_OP:
        case ASSIGN_V_OP:
        case ASSIGN_GEN_OP:
        case ASSIGN_MASKED_KV_OP:
        default:
            break;
        }
        break;
    case TRINARY_N:
        switch(fexpr->tri.op) {
        case COND_OP:
        case COND_V_OP:
        case COND_SV_OP:
        case COND_GEN_OP:
        case ASSIGN_COND_OP:
        case ASSIGN_COND_V_OP:
        case ASSIGN_COND_SV_OP:
        case ASSIGN_COND_GEN_OP:
        default:
            break;
        }
        break;
    default:
        break;
    }
    return rv;
} // ConstantFoldNode

/*
 * lValidFoldKind() - TRUE if the canonical kind participates in folding.
 *
 */

static int lValidFoldKind(CgScalarKind kind)
{
    return kind > CG_SCALAR_UNDEFINED && kind < CG_SCALAR_COUNT;
}

/*
 * FoldKindOf() - Pick the kind governing a fold: the stored kind of the
 *         left operand when it carries one, else the default kind of the
 *         legacy base recorded in the node's subop.
 *
 */

static CgScalarKind FoldKindOf(CgScalarKind stored, int base)
{
    if (lValidFoldKind(stored))
        return stored;
    switch (base) {
    case TYPE_BASE_CFLOAT:
        return CG_SCALAR_CFLOAT;
    case TYPE_BASE_CINT:
        return CG_SCALAR_CINT;
    case TYPE_BASE_BOOLEAN:
        return CG_SCALAR_BOOL;
    case TYPE_BASE_INT:
        return CG_SCALAR_INT;
    case TYPE_BASE_FLOAT:
        return CG_SCALAR_FLOAT;
    default:
        return CG_SCALAR_UNDEFINED;
    }
}

/*
 * KindOps() - Operation table for a canonical scalar kind.
 *
 */

static operations *KindOps(CgScalarKind kind)
{
    if (!lValidFoldKind(kind))
        return 0;
    return kind_ops[kind];
}

/*
 * BaseOps() - Operation table for a legacy four-bit base.
 *
 */

static operations *BaseOps(int base)
{
    return KindOps(FoldKindOf(CG_SCALAR_UNDEFINED, base));
}

/*
 * num_* wrappers - every arithmetic primitive funnels through
 * CgNumericBinary, which picks the working precision and the result
 * normalization from the left operand's canonical kind.
 *
 */

static void num_neg(scalar_constant *r, const scalar_constant *a)
{
    CgNumericBinary(r, CG_NUMERIC_NEG, a, 0);
}
static void num_not(scalar_constant *r, const scalar_constant *a)
{
    CgNumericBinary(r, CG_NUMERIC_BIT_NOT, a, 0);
}
static void num_lnot(scalar_constant *r, const scalar_constant *a)
{
    CgNumericBinary(r, CG_NUMERIC_NOT, a, 0);
}
static void num_add(scalar_constant *r, const scalar_constant *a, const scalar_constant *b)
{
    CgNumericBinary(r, CG_NUMERIC_ADD, a, b);
}
static void num_sub(scalar_constant *r, const scalar_constant *a, const scalar_constant *b)
{
    CgNumericBinary(r, CG_NUMERIC_SUB, a, b);
}
static void num_mul(scalar_constant *r, const scalar_constant *a, const scalar_constant *b)
{
    CgNumericBinary(r, CG_NUMERIC_MUL, a, b);
}
static void num_div(scalar_constant *r, const scalar_constant *a, const scalar_constant *b)
{
    CgNumericBinary(r, CG_NUMERIC_DIV, a, b);
}
static void num_mod(scalar_constant *r, const scalar_constant *a, const scalar_constant *b)
{
    CgNumericBinary(r, CG_NUMERIC_MOD, a, b);
}
static void num_and(scalar_constant *r, const scalar_constant *a, const scalar_constant *b)
{
    CgNumericBinary(r, CG_NUMERIC_BIT_AND, a, b);
}
static void num_or(scalar_constant *r, const scalar_constant *a, const scalar_constant *b)
{
    CgNumericBinary(r, CG_NUMERIC_BIT_OR, a, b);
}
static void num_xor(scalar_constant *r, const scalar_constant *a, const scalar_constant *b)
{
    CgNumericBinary(r, CG_NUMERIC_BIT_XOR, a, b);
}

/*
 * num_shr/num_shl - Shift counts stay full-width compile-time ints; they
 * are never converted down to the left operand's kind.
 *
 */

static int num_shr(scalar_constant *r, const scalar_constant *a, int b)
{
    CgNumericValue count;

    CgNumericSetSigned(&count, CG_SCALAR_CINT, b);
    return CgNumericBinary(r, CG_NUMERIC_SHIFT_RIGHT, a, &count);
}
static int num_shl(scalar_constant *r, const scalar_constant *a, int b)
{
    CgNumericValue count;

    CgNumericSetSigned(&count, CG_SCALAR_CINT, b);
    return CgNumericBinary(r, CG_NUMERIC_SHIFT_LEFT, a, &count);
}

/*
 * Boolean logic stays local: cg_numeric has no logical-AND/OR primitives.
 *
 */

static void bool_and(scalar_constant *r, const scalar_constant *a, const scalar_constant *b)
{
    r->value.i = a->value.i && b->value.i;
}
static void bool_or(scalar_constant *r, const scalar_constant *a, const scalar_constant *b)
{
    r->value.i = a->value.i || b->value.i;
}

/*
 * num_lt..num_ne - Comparisons run in the operands' own domain: floating
 * kinds compare as doubles, unsigned kinds compare modularly, and signed
 * kinds compare in 64 bits.
 *
 */

static int num_lt(const scalar_constant *a, const scalar_constant *b)
{
    if (CgScalarIsFloating(a->kind))
        return a->value.f < b->value.f;
    if (CgScalarIsUnsigned(a->kind))
        return a->value.u < b->value.u;
    return a->value.i < b->value.i;
}
static int num_gt(const scalar_constant *a, const scalar_constant *b)
{
    if (CgScalarIsFloating(a->kind))
        return a->value.f > b->value.f;
    if (CgScalarIsUnsigned(a->kind))
        return a->value.u > b->value.u;
    return a->value.i > b->value.i;
}
static int num_le(const scalar_constant *a, const scalar_constant *b)
{
    if (CgScalarIsFloating(a->kind))
        return a->value.f <= b->value.f;
    if (CgScalarIsUnsigned(a->kind))
        return a->value.u <= b->value.u;
    return a->value.i <= b->value.i;
}
static int num_ge(const scalar_constant *a, const scalar_constant *b)
{
    if (CgScalarIsFloating(a->kind))
        return a->value.f >= b->value.f;
    if (CgScalarIsUnsigned(a->kind))
        return a->value.u >= b->value.u;
    return a->value.i >= b->value.i;
}
static int num_eq(const scalar_constant *a, const scalar_constant *b)
{
    if (CgScalarIsFloating(a->kind))
        return a->value.f == b->value.f;
    if (CgScalarIsUnsigned(a->kind))
        return a->value.u == b->value.u;
    return a->value.i == b->value.i;
}
static int num_ne(const scalar_constant *a, const scalar_constant *b)
{
    if (CgScalarIsFloating(a->kind))
        return a->value.f != b->value.f;
    if (CgScalarIsUnsigned(a->kind))
        return a->value.u != b->value.u;
    return a->value.i != b->value.i;
}


/*
 * BinOpAvailable() - TRUE when this kind's table folds the primitive.
 *
 */

static int BinOpAvailable(operations *ops, CgNumericOp numop)
{
    if (!ops)
        return 0;
    switch (numop) {
    case CG_NUMERIC_ADD:
        return ops->op_add != 0;
    case CG_NUMERIC_SUB:
        return ops->op_sub != 0;
    case CG_NUMERIC_MUL:
        return ops->op_mul != 0;
    case CG_NUMERIC_DIV:
        return ops->op_div != 0;
    case CG_NUMERIC_MOD:
        return ops->op_mod != 0;
    case CG_NUMERIC_BIT_AND:
        return ops->op_and != 0;
    case CG_NUMERIC_BIT_OR:
        return ops->op_or != 0;
    case CG_NUMERIC_BIT_XOR:
        return ops->op_xor != 0;
    default:
        return 0;
    }
}

/*
 * CmpFn() - Comparison entry for a frontend comparison opcode.
 *
 */

static int (*CmpFn(operations *ops, opcode op))
    (const scalar_constant *, const scalar_constant *)
{
    switch (op) {
    case LT_OP: case LT_V_OP: case LT_SV_OP: case LT_VS_OP:
        return ops->op_lt;
    case GT_OP: case GT_V_OP: case GT_SV_OP: case GT_VS_OP:
        return ops->op_gt;
    case LE_OP: case LE_V_OP: case LE_SV_OP: case LE_VS_OP:
        return ops->op_le;
    case GE_OP: case GE_V_OP: case GE_SV_OP: case GE_VS_OP:
        return ops->op_ge;
    case EQ_OP: case EQ_V_OP: case EQ_SV_OP: case EQ_VS_OP:
        return ops->op_eq;
    case NE_OP: case NE_V_OP: case NE_SV_OP: case NE_VS_OP:
        return ops->op_ne;
    default:
        return 0;
    }
}

static operations int_fam_ops = {
    ICONST_OP,
    num_neg, num_not, 0,
    num_add, num_sub, num_mul, num_div, num_mod,
    num_and, num_or, num_xor, 0, 0,
    num_shr, num_shl,
    num_lt, num_gt, num_le, num_ge, num_eq, num_ne,
};

static operations float_fam_ops = {
    FCONST_OP,
    num_neg, 0, 0,
    num_add, num_sub, num_mul, num_div, 0,
    0, 0, 0, 0, 0,
    0, 0,
    num_lt, num_gt, num_le, num_ge, num_eq, num_ne,
};

static operations bool_fam_ops = {
    BCONST_OP,
    0, 0, num_lnot,
    0, 0, 0, 0, 0,
    0, 0, 0, bool_and, bool_or,
    0, 0,
    0, 0, 0, 0, num_eq, num_ne,
};

static operations half_fam_ops = {
    HCONST_OP,
    num_neg, 0, 0,
    num_add, num_sub, num_mul, num_div, 0,
    0, 0, 0, 0, 0,
    0, 0,
    num_lt, num_gt, num_le, num_ge, num_eq, num_ne,
};

static operations fixed_fam_ops = {
    XCONST_OP,
    num_neg, 0, 0,
    num_add, num_sub, num_mul, num_div, 0,
    0, 0, 0, 0, 0,
    0, 0,
    num_lt, num_gt, num_le, num_ge, num_eq, num_ne,
};

/*
 * kind_ops - Folding tables indexed by CgScalarKind.  The integer-width
 * families share one set of primitives; each operation reads the working
 * width and signedness from the value's own kind.  Double shares the
 * float family since CgNumericBinary keeps double precision intact.
 */

static operations *kind_ops[CG_SCALAR_COUNT] = {
    0,              /* none */
    0,              /* undefined */
    &float_fam_ops, /* cfloat */
    &int_fam_ops,   /* cint */
    &bool_fam_ops,  /* boolean */
    &int_fam_ops,   /* char */
    &int_fam_ops,   /* unsigned char */
    &int_fam_ops,   /* short */
    &int_fam_ops,   /* unsigned short */
    &int_fam_ops,   /* int */
    &int_fam_ops,   /* unsigned int */
    &int_fam_ops,   /* long */
    &int_fam_ops,   /* unsigned long */
    &fixed_fam_ops, /* fixed */
    &half_fam_ops,  /* half */
    &float_fam_ops, /* float */
    &float_fam_ops, /* double */
};
