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
// cg_ir_print.c - Deterministic normalized printing of verified Cg IR
//        modules.  Declarations print in module order and functions in
//        module (reachability/source) order with two-space indentation,
//        decimal numerics, canonical CgScalarKindName type spellings,
//        explicit packed/unpacked and sized/unsized array notation, and
//        stable intrinsic spellings.  The printer never emits pointer
//        values, allocator addresses, or traversal-dependent hash
//        order: every byte derives from stable node fields and atom
//        strings.  The whole text is buffered and flushed once after
//        CgIRVerifyModule succeeds, so verification failure, internal
//        allocation failure, or a stream error leaves the output
//        without any partial node text.
//

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "cg_stdlib.h"
#include "cg_ir.h"

//////////////////////// Stable spelling tables //////////////////////////

/*
 * Intrinsic spellings come straight from the pinned catalog rows, so
 * the printed name of an opcode never depends on runtime state.  The
 * def rows carry their names as quoted literals, so the table expands
 * them directly.
 */

static const char * const lIntrinsicNames[] = {
    "<no-intrinsic>",
#define CG_INTRINSIC(id, name, flags) name,
#include "cg_stdlib.def"
#undef CG_INTRINSIC
};

static const char *lIntrinsicName(CgIntrinsic intrinsic)
{
    if (intrinsic > CG_INTRINSIC_NONE && intrinsic < CG_INTRINSIC_COUNT)
        return lIntrinsicNames[intrinsic];
    return "<bad-intrinsic>";
} // lIntrinsicName

/*
 * Canonical sampler spellings indexed by CgSamplerKind.
 */

static const char * const lSamplerNames[] = {
    "sampler", "sampler1D", "sampler2D", "sampler3D",
    "samplerCUBE", "samplerRECT",
};

//////////////////////////////// Print buffer ////////////////////////////////

/*
 * Grow-only output buffer backed by a private memory pool.  All text is
 * staged here so the caller's stream sees one all-or-nothing write; the
 * pool dies with the print call.
 */

typedef struct PrintBuf_Rec {
    MemoryPool *pool;
    char *data;
    size_t size;
    size_t cap;
    int failed;
} PrintBuf;

static void lBufGrow(PrintBuf *buf, size_t extra)
{
    size_t cap;
    char *data;

    if (buf->failed)
        return;
    if (buf->cap - buf->size >= extra)
        return;
    cap = buf->cap ? buf->cap : 256;
    while (cap - buf->size < extra)
        cap *= 2;
    data = (char *) mem_Alloc(buf->pool, cap);
    if (! data) {
        buf->failed = 1;
        return;
    }
    if (buf->size)
        memcpy(data, buf->data, buf->size);
    buf->data = data;
    buf->cap = cap;
} // lBufGrow

static void lBufAppend(PrintBuf *buf, const char *text, size_t len)
{
    if (buf->failed || len == 0)
        return;
    lBufGrow(buf, len);
    if (buf->failed)
        return;
    memcpy(buf->data + buf->size, text, len);
    buf->size += len;
} // lBufAppend

static void lBufPrint(PrintBuf *buf, const char *format, ...)
{
    va_list args;
    int needed;

    if (buf->failed)
        return;
    va_start(args, format);
    needed = vsnprintf(NULL, 0, format, args);
    va_end(args);
    if (needed < 0) {
        buf->failed = 1;
        return;
    }
    lBufGrow(buf, (size_t) needed + 1);
    if (buf->failed)
        return;
    va_start(args, format);
    needed = vsnprintf(buf->data + buf->size, (size_t) needed + 1,
                       format, args);
    va_end(args);
    if (needed < 0) {
        buf->failed = 1;
        return;
    }
    buf->size += (size_t) needed;
} // lBufPrint

static void lPrintIndent(PrintBuf *buf, int depth)
{
    int i;

    for (i = 0; i < depth; i++)
        lBufAppend(buf, "  ", 2);
} // lPrintIndent

//////////////////////////// Type spellings //////////////////////////////

static void lPrintScalarBase(PrintBuf *buf, const Type *type)
{
    CgScalarKind kind;

    if (GetBase(type) == TYPE_BASE_VOID) {
        lBufAppend(buf, "void", 4);
        return;
    }
    kind = GetScalarKind(type);
    lBufPrint(buf, "%s", CgScalarKindName(kind));
} // lPrintScalarBase

static void lPrintPackedPrefix(PrintBuf *buf, const Type *type)
{
    if (type->properties & TYPE_MISC_PACKED_KW)
        lBufAppend(buf, "packed ", 7);
} // lPrintPackedPrefix

static void lPrintType(PrintBuf *buf, const Type *type)
{
    int category, columns, length, rows;
    CgSamplerKind sampler;

    if (! type) {
        lBufAppend(buf, "<type>", 6);
        return;
    }
    category = GetCategory(type);
    switch (category) {
    case TYPE_CATEGORY_SCALAR:
        lPrintScalarBase(buf, type);
        break;
    case TYPE_CATEGORY_ARRAY:
        /* Canonical vectors and matrices are packed arrays of packed
         * rows; they spell like their source forms. */
        if (IsVector(type, &length)) {
            lPrintScalarBase(buf, type);
            lBufPrint(buf, "%d", length);
        } else if (IsMatrix(type, &columns, &rows)) {
            lPrintPackedPrefix(buf, type);
            lPrintScalarBase(buf, type);
            lBufPrint(buf, "%dx%d", rows, columns);
        } else {
            lPrintPackedPrefix(buf, type);
            lPrintType(buf, type->arr.eltype);
            if (type->arr.numels == CG_ARRAY_UNSIZED)
                lBufAppend(buf, "[]", 2);
            else
                lBufPrint(buf, "[%d]", type->arr.numels);
        }
        break;
    case TYPE_CATEGORY_STRUCT:
        lPrintPackedPrefix(buf, type);
        lBufAppend(buf, "struct ", 7);
        lBufPrint(buf, "%s", GetAtomString(atable, type->str.tag));
        break;
    case TYPE_CATEGORY_CONNECTOR:
        lBufAppend(buf, "connector ", 10);
        lBufPrint(buf, "%s", GetAtomString(atable, type->str.tag));
        break;
    case TYPE_CATEGORY_INTERFACE:
        lBufAppend(buf, "interface ", 10);
        lBufPrint(buf, "%s", GetAtomString(atable, type->iface.tag));
        break;
    case TYPE_CATEGORY_SAMPLER:
        sampler = type->samp.samplerKind;
        if (sampler >= CG_SAMPLER_BASE && sampler < CG_SAMPLER_COUNT)
            lBufPrint(buf, "%s", lSamplerNames[sampler]);
        else
            lBufAppend(buf, "<sampler>", 9);
        break;
    default:
        lBufAppend(buf, "<type>", 6);
        break;
    }
} // lPrintType

/////////////////////////// Declaration printing /////////////////////////

/*
 * Leading storage word: what the source declared, falling back to the
 * assigned program-interface domain for entry formals and globals that
 * carry no explicit storage class.
 */

static void lPrintStorageWord(PrintBuf *buf, const CgIRDecl *decl)
{
    const char *word;

    word = NULL;
    switch (decl->storage) {
    case CGIR_STORAGE_CONST:
        word = "const";
        break;
    case CGIR_STORAGE_UNIFORM:
        word = "uniform";
        break;
    case CGIR_STORAGE_VARYING:
        word = "varying";
        break;
    case CGIR_STORAGE_NONE:
    default:
        switch (decl->domain) {
        case CGIR_DOMAIN_UNIFORM:
            word = "uniform";
            break;
        case CGIR_DOMAIN_VARYING:
            word = "varying";
            break;
        case CGIR_DOMAIN_NONE:
        default:
            break;
        }
        break;
    }
    if (word) {
        lBufAppend(buf, word, strlen(word));
        lBufAppend(buf, " ", 1);
    }
} // lPrintStorageWord

static void lPrintDirectionWord(PrintBuf *buf, const Type *type)
{
    int qualifiers;

    qualifiers = GetQualifiers(type);
    if ((qualifiers & TYPE_QUALIFIER_INOUT) == TYPE_QUALIFIER_INOUT)
        lBufAppend(buf, "inout ", 6);
    else if (qualifiers & TYPE_QUALIFIER_OUT)
        lBufAppend(buf, "out ", 4);
} // lPrintDirectionWord

/*
 * Declared name: the resolved symbol's atom, the declaration's own
 * name atom, or a fixed placeholder for fully anonymous temporaries.
 */

static void lPrintDeclName(PrintBuf *buf, const CgIRDecl *decl)
{
    int name;

    name = 0;
    if (decl->symbol && decl->symbol->name)
        name = decl->symbol->name;
    else if (decl->name)
        name = decl->name;
    if (name)
        lBufPrint(buf, "%s", GetAtomString(atable, name));
    else
        lBufAppend(buf, "<unnamed>", 9);
} // lPrintDeclName

/*
 * "<storage> <direction> <type> <name>[: <semantic>]" -- everything a
 * declaration line needs before its initializer or semicolon.
 */

static void lPrintDeclHead(PrintBuf *buf, const CgIRDecl *decl)
{
    lPrintStorageWord(buf, decl);
    lPrintDirectionWord(buf, decl->type);
    lPrintType(buf, decl->type);
    lBufAppend(buf, " ", 1);
    lPrintDeclName(buf, decl);
    if (decl->semantic)
        lBufPrint(buf, " : %s", GetAtomString(atable, decl->semantic));
} // lPrintDeclHead

//////////////////////////// Expression printing ///////////////////////////

static void lPrintExpr(PrintBuf *buf, const CgIRExpr *expr);

/*
 * Postfix chains (members, indices, swizzles, calls) and leaves embed
 * safely without parentheses; operator forms need them when nested.
 */

static int lExprIsAtomic(const CgIRExpr *expr)
{
    switch (expr->kind) {
    case CGIR_EXPR_CONSTANT:
    case CGIR_EXPR_SYMBOL:
    case CGIR_EXPR_MEMBER:
    case CGIR_EXPR_INDEX:
    case CGIR_EXPR_LENGTH:
    case CGIR_EXPR_SWIZZLE:
    case CGIR_EXPR_CONSTRUCT:
    case CGIR_EXPR_CAST:
    case CGIR_EXPR_CALL:
    case CGIR_EXPR_INTERFACE_CALL:
    case CGIR_EXPR_INTRINSIC:
        return 1;
    default:
        return 0;
    }
} // lExprIsAtomic

/*
 * Operand position: atomic expressions print bare, operator forms get
 * exactly one protective pair of parentheses.
 */

static void lPrintOperand(PrintBuf *buf, const CgIRExpr *expr)
{
    if (! expr) {
        lBufAppend(buf, "<missing>", 9);
        return;
    }
    if (lExprIsAtomic(expr)) {
        lPrintExpr(buf, expr);
    } else {
        lBufAppend(buf, "(", 1);
        lPrintExpr(buf, expr);
        lBufAppend(buf, ")", 1);
    }
} // lPrintOperand

static void lPrintCallArguments(PrintBuf *buf, const CgIRExpr *args)
{
    const CgIRExpr *arg;

    lBufAppend(buf, "(", 1);
    for (arg = args; arg != NULL; arg = arg->next) {
        if (arg != args)
            lBufAppend(buf, ", ", 2);
        lPrintOperand(buf, arg);
    }
    lBufAppend(buf, ")", 1);
} // lPrintCallArguments

static void lPrintConstant(PrintBuf *buf, const CgNumericValue *value)
{
    switch (value->kind) {
    case CG_SCALAR_BOOL:
        lBufAppend(buf, value->value.i ? "true" : "false",
                   value->value.i ? 4 : 5);
        break;
    case CG_SCALAR_NONE:
    case CG_SCALAR_UNDEFINED:
        lBufAppend(buf, "<none>", 6);
        break;
    default:
        if (CgScalarIsIntegral(value->kind)) {
            if (CgScalarIsUnsigned(value->kind))
                lBufPrint(buf, "%llu", value->value.u);
            else
                lBufPrint(buf, "%lld", value->value.i);
        } else {
            lBufPrint(buf, "%.6f", value->value.f);
        }
        break;
    }
} // lPrintConstant

static const char *lBinaryOpSpelling(CgIROp op)
{
    switch (op) {
    case CGIR_OP_MULTIPLY:       return "*";
    case CGIR_OP_DIVIDE:         return "/";
    case CGIR_OP_MODULO:         return "%";
    case CGIR_OP_ADD:            return "+";
    case CGIR_OP_SUBTRACT:       return "-";
    case CGIR_OP_SHIFT_LEFT:     return "<<";
    case CGIR_OP_SHIFT_RIGHT:    return ">>";
    case CGIR_OP_LESS:           return "<";
    case CGIR_OP_GREATER:        return ">";
    case CGIR_OP_LESS_EQUAL:     return "<=";
    case CGIR_OP_GREATER_EQUAL:  return ">=";
    case CGIR_OP_EQUAL:          return "==";
    case CGIR_OP_NOT_EQUAL:      return "!=";
    case CGIR_OP_BITWISE_AND:    return "&";
    case CGIR_OP_BITWISE_XOR:    return "^";
    case CGIR_OP_BITWISE_OR:     return "|";
    case CGIR_OP_LOGICAL_AND:    return "&&";
    case CGIR_OP_LOGICAL_OR:     return "||";
    case CGIR_OP_ASSIGN:         return "=";
    case CGIR_OP_ADD_ASSIGN:     return "+=";
    case CGIR_OP_SUBTRACT_ASSIGN:return "-=";
    case CGIR_OP_MULTIPLY_ASSIGN:return "*=";
    case CGIR_OP_DIVIDE_ASSIGN:  return "/=";
    case CGIR_OP_MODULO_ASSIGN:  return "%=";
    default:                     return "<op>";
    }
} // lBinaryOpSpelling

static void lPrintUnaryOperator(PrintBuf *buf, CgIROp op)
{
    const char *spelling;

    switch (op) {
    case CGIR_OP_NEGATE:          spelling = "-";  break;
    case CGIR_OP_POSITIVE:        spelling = "+";  break;
    case CGIR_OP_LOGICAL_NOT:     spelling = "!";  break;
    case CGIR_OP_BITWISE_NOT:     spelling = "~";  break;
    case CGIR_OP_PRE_INCREMENT:
    case CGIR_OP_POST_INCREMENT:  spelling = "++"; break;
    case CGIR_OP_PRE_DECREMENT:
    case CGIR_OP_POST_DECREMENT:  spelling = "--"; break;
    default:                      spelling = "<unary>"; break;
    }
    lBufAppend(buf, spelling, strlen(spelling));
} // lPrintUnaryOperator

static void lPrintSwizzleComponents(PrintBuf *buf, int mask, int count)
{
    static const char components[] = "xyzw";
    int i, select;

    for (i = 0; i < count; i++) {
        select = (mask >> (2 * i)) & 0x3;
        if (select >= 0 && select <= 3)
            lBufAppend(buf, &components[select], 1);
        else
            lBufAppend(buf, "?", 1);
    }
} // lPrintSwizzleComponents

static void lPrintExpr(PrintBuf *buf, const CgIRExpr *expr)
{
    if (! expr) {
        lBufAppend(buf, "<missing>", 9);
        return;
    }
    switch (expr->kind) {
    case CGIR_EXPR_CONSTANT:
        lPrintConstant(buf, &expr->u.constant);
        break;
    case CGIR_EXPR_SYMBOL:
        if (expr->u.symbol && expr->u.symbol->name)
            lBufPrint(buf, "%s", GetAtomString(atable,
                                               expr->u.symbol->name));
        else
            lBufAppend(buf, "<unnamed>", 9);
        break;
    case CGIR_EXPR_MEMBER:
        lPrintOperand(buf, expr->u.member.object);
        lBufAppend(buf, ".", 1);
        if (expr->u.member.member)
            lBufPrint(buf, "%s", GetAtomString(atable,
                            expr->u.member.member->name));
        else
            lBufAppend(buf, "<member>", 8);
        break;
    case CGIR_EXPR_INDEX:
        lPrintOperand(buf, expr->u.index.object);
        lBufAppend(buf, "[", 1);
        if (expr->u.index.index)
            lPrintExpr(buf, expr->u.index.index);
        else
            lBufAppend(buf, "<missing>", 9);
        lBufAppend(buf, "]", 1);
        break;
    case CGIR_EXPR_LENGTH:
        lPrintOperand(buf, expr->u.length.object);
        lBufAppend(buf, ".length", 7);
        break;
    case CGIR_EXPR_SWIZZLE:
        lPrintOperand(buf, expr->u.swizzle.object);
        lBufAppend(buf, ".", 1);
        lPrintSwizzleComponents(buf, expr->u.swizzle.mask,
                                expr->u.swizzle.componentCount);
        break;
    case CGIR_EXPR_CONSTRUCT:
        lPrintType(buf, expr->type);
        lPrintCallArguments(buf, expr->u.construct.arguments);
        break;
    case CGIR_EXPR_CAST:
        lBufAppend(buf, "(", 1);
        lPrintType(buf, expr->type);
        lBufAppend(buf, ")", 1);
        lPrintOperand(buf, expr->u.cast.operand);
        break;
    case CGIR_EXPR_UNARY:
        switch (expr->u.unary.op) {
        case CGIR_OP_POST_INCREMENT:
        case CGIR_OP_POST_DECREMENT:
            lPrintOperand(buf, expr->u.unary.operand);
            lPrintUnaryOperator(buf, expr->u.unary.op);
            break;
        default:
            lPrintUnaryOperator(buf, expr->u.unary.op);
            lPrintOperand(buf, expr->u.unary.operand);
            break;
        }
        break;
    case CGIR_EXPR_BINARY:
        lPrintOperand(buf, expr->u.binary.left);
        lBufAppend(buf, " ", 1);
        lBufAppend(buf, lBinaryOpSpelling(expr->u.binary.op),
                   strlen(lBinaryOpSpelling(expr->u.binary.op)));
        lBufAppend(buf, " ", 1);
        lPrintOperand(buf, expr->u.binary.right);
        break;
    case CGIR_EXPR_ASSIGN:
        lPrintOperand(buf, expr->u.assign.target);
        lBufAppend(buf, " ", 1);
        lBufAppend(buf, lBinaryOpSpelling(expr->u.assign.op),
                   strlen(lBinaryOpSpelling(expr->u.assign.op)));
        lBufAppend(buf, " ", 1);
        lPrintOperand(buf, expr->u.assign.value);
        break;
    case CGIR_EXPR_CONDITIONAL:
        lPrintOperand(buf, expr->u.conditional.condition);
        lBufAppend(buf, " ? ", 3);
        lPrintOperand(buf, expr->u.conditional.trueExpr);
        lBufAppend(buf, " : ", 3);
        lPrintOperand(buf, expr->u.conditional.falseExpr);
        break;
    case CGIR_EXPR_CALL:
        if (expr->u.call.callee)
            lBufPrint(buf, "%s", GetAtomString(atable,
                            expr->u.call.callee->name));
        else
            lBufAppend(buf, "<callee>", 8);
        lPrintCallArguments(buf, expr->u.call.arguments);
        break;
    case CGIR_EXPR_INTERFACE_CALL:
        lPrintOperand(buf, expr->u.interfaceCall.receiver);
        lBufAppend(buf, ".", 1);
        if (expr->u.interfaceCall.method)
            lBufPrint(buf, "%s", GetAtomString(atable,
                            expr->u.interfaceCall.method->name));
        else
            lBufAppend(buf, "<method>", 8);
        lPrintCallArguments(buf, expr->u.interfaceCall.arguments);
        break;
    case CGIR_EXPR_INTRINSIC:
        lBufAppend(buf, lIntrinsicName(expr->u.intrinsicCall.intrinsic),
                   strlen(lIntrinsicName(expr->u.intrinsicCall.intrinsic)));
        lPrintCallArguments(buf, expr->u.intrinsicCall.arguments);
        break;
    default:
        lBufAppend(buf, "<bad-expr>", 10);
        break;
    }
} // lPrintExpr

//////////////////////////// Statement printing ///////////////////////////

static void lPrintStmtList(PrintBuf *buf, const CgIRStmt *stmts, int depth);

static void lPrintStmtInlineHead(PrintBuf *buf, const CgIRStmt *stmt)
{
    if (! stmt)
        return;
    switch (stmt->kind) {
    case CGIR_STMT_DECL:
        if (stmt->u.decl) {
            lPrintDeclHead(buf, stmt->u.decl);
            if (stmt->u.decl->initializer) {
                lBufAppend(buf, " = ", 3);
                lPrintExpr(buf, stmt->u.decl->initializer);
            }
        }
        break;
    case CGIR_STMT_EXPR:
        if (stmt->u.expression)
            lPrintExpr(buf, stmt->u.expression);
        break;
    default:
        break;
    }
} // lPrintStmtInlineHead

static void lPrintStmt(PrintBuf *buf, const CgIRStmt *stmt, int depth);

static void lPrintBodyStmt(PrintBuf *buf, const CgIRStmt *body, int depth)
{
    if (! body)
        return;
    lPrintStmt(buf, body, depth);
} // lPrintBodyStmt

static void lPrintStmt(PrintBuf *buf, const CgIRStmt *stmt, int depth)
{
    if (! stmt) {
        lPrintIndent(buf, depth);
        lBufAppend(buf, "<missing>;\n", 11);
        return;
    }
    switch (stmt->kind) {
    case CGIR_STMT_BLOCK:
        lPrintIndent(buf, depth);
        lBufAppend(buf, "{\n", 2);
        lPrintStmtList(buf, stmt->u.block, depth + 1);
        lPrintIndent(buf, depth);
        lBufAppend(buf, "}\n", 2);
        break;
    case CGIR_STMT_DECL:
        lPrintIndent(buf, depth);
        if (stmt->u.decl) {
            lPrintDeclHead(buf, stmt->u.decl);
            if (stmt->u.decl->initializer) {
                lBufAppend(buf, " = ", 3);
                lPrintExpr(buf, stmt->u.decl->initializer);
            }
        }
        lBufAppend(buf, ";\n", 2);
        break;
    case CGIR_STMT_EXPR:
        lPrintIndent(buf, depth);
        if (stmt->u.expression)
            lPrintExpr(buf, stmt->u.expression);
        lBufAppend(buf, ";\n", 2);
        break;
    case CGIR_STMT_IF:
        lPrintIndent(buf, depth);
        lBufAppend(buf, "if (", 4);
        if (stmt->u.ifStmt.condition)
            lPrintExpr(buf, stmt->u.ifStmt.condition);
        else
            lBufAppend(buf, "<missing>", 9);
        lBufAppend(buf, ")\n", 2);
        lPrintBodyStmt(buf, stmt->u.ifStmt.trueBranch, depth);
        if (stmt->u.ifStmt.falseBranch) {
            lPrintIndent(buf, depth);
            lBufAppend(buf, "else\n", 5);
            lPrintBodyStmt(buf, stmt->u.ifStmt.falseBranch, depth);
        }
        break;
    case CGIR_STMT_WHILE:
        lPrintIndent(buf, depth);
        lBufAppend(buf, "while (", 7);
        if (stmt->u.loop.condition)
            lPrintExpr(buf, stmt->u.loop.condition);
        else
            lBufAppend(buf, "<missing>", 9);
        lBufAppend(buf, ")\n", 2);
        lPrintBodyStmt(buf, stmt->u.loop.body, depth);
        break;
    case CGIR_STMT_DO:
        lPrintIndent(buf, depth);
        lBufAppend(buf, "do\n", 3);
        lPrintBodyStmt(buf, stmt->u.loop.body, depth);
        lPrintIndent(buf, depth);
        lBufAppend(buf, "while (", 7);
        if (stmt->u.loop.condition)
            lPrintExpr(buf, stmt->u.loop.condition);
        else
            lBufAppend(buf, "<missing>", 9);
        lBufAppend(buf, ");\n", 3);
        break;
    case CGIR_STMT_FOR:
        lPrintIndent(buf, depth);
        lBufAppend(buf, "for (", 5);
        lPrintStmtInlineHead(buf, stmt->u.forStmt.init);
        lBufAppend(buf, "; ", 2);
        if (stmt->u.forStmt.condition)
            lPrintExpr(buf, stmt->u.forStmt.condition);
        lBufAppend(buf, "; ", 2);
        if (stmt->u.forStmt.step)
            lPrintExpr(buf, stmt->u.forStmt.step);
        lBufAppend(buf, ")\n", 2);
        lPrintBodyStmt(buf, stmt->u.forStmt.body, depth);
        break;
    case CGIR_STMT_RETURN:
        lPrintIndent(buf, depth);
        lBufAppend(buf, "return", 6);
        if (stmt->u.returnExpr) {
            lBufAppend(buf, " ", 1);
            lPrintExpr(buf, stmt->u.returnExpr);
        }
        lBufAppend(buf, ";\n", 2);
        break;
    case CGIR_STMT_BREAK:
        lPrintIndent(buf, depth);
        lBufAppend(buf, "break;\n", 7);
        break;
    case CGIR_STMT_CONTINUE:
        lPrintIndent(buf, depth);
        lBufAppend(buf, "continue;\n", 10);
        break;
    case CGIR_STMT_DISCARD:
        lPrintIndent(buf, depth);
        if (stmt->u.discard.condition) {
            lBufAppend(buf, "discard (", 9);
            lPrintExpr(buf, stmt->u.discard.condition);
            lBufAppend(buf, ");\n", 3);
        } else {
            lBufAppend(buf, "discard;\n", 9);
        }
        break;
    default:
        lPrintIndent(buf, depth);
        lBufAppend(buf, "<bad-stmt>;\n", 12);
        break;
    }
} // lPrintStmt

static void lPrintStmtList(PrintBuf *buf, const CgIRStmt *stmts, int depth)
{
    const CgIRStmt *stmt;

    for (stmt = stmts; stmt != NULL; stmt = stmt->next)
        lPrintStmt(buf, stmt, depth);
} // lPrintStmtList

///////////////////////////// Module printing /////////////////////////////

static void lPrintFunction(PrintBuf *buf, const CgIRFunction *function)
{
    const CgIRDecl *param;

    if (function->resultType)
        lPrintType(buf, function->resultType);
    else
        lBufAppend(buf, "<type>", 6);
    lBufAppend(buf, " ", 1);
    if (function->symbol && function->symbol->name)
        lBufPrint(buf, "%s", GetAtomString(atable,
                                           function->symbol->name));
    else
        lBufAppend(buf, "<unnamed>", 9);
    lBufAppend(buf, "(", 1);
    for (param = function->parameters; param != NULL; param = param->next) {
        if (param != function->parameters)
            lBufAppend(buf, ", ", 2);
        lPrintDeclHead(buf, param);
    }
    lBufAppend(buf, ")\n{\n", 4);
    lPrintStmtList(buf, function->body ? function->body->u.block : NULL, 1);
    lBufAppend(buf, "}\n", 2);
} // lPrintFunction

static void lPrintModuleText(PrintBuf *buf, const CgIRModule *module)
{
    const CgIRDecl *global;
    const CgIRFunction *function;
    int printedFunction;

    for (global = module->globals; global != NULL; global = global->next) {
        lPrintDeclHead(buf, global);
        if (global->initializer) {
            lBufAppend(buf, " = ", 3);
            lPrintExpr(buf, global->initializer);
        }
        lBufAppend(buf, ";\n", 2);
    }
    if (module->globals)
        lBufAppend(buf, "\n", 1);

    printedFunction = 0;
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (printedFunction)
            lBufAppend(buf, "\n", 1);
        printedFunction = 1;
        lPrintFunction(buf, function);
    }
} // lPrintModuleText

/*
 * CgIRPrintModule() - Verify first, buffer the whole normalized text,
 *          then hand the complete bytes to "out" in one write.  Any
 *          failure along the way reports zero and leaves the stream
 *          without printer-generated text.
 */

int CgIRPrintModule(FILE *out, const CgIRModule *module)
{
    CgIRVerifyDiagnostic diagnostic;
    PrintBuf buf;
    MemoryPool *pool;
    int ok;

    if (out == NULL || module == NULL)
        return 0;
    memset(&diagnostic, 0, sizeof(diagnostic));
    if (! CgIRVerifyModule(module, &diagnostic))
        return 0;
    pool = mem_CreatePool(0, 0);
    if (! pool)
        return 0;
    memset(&buf, 0, sizeof(buf));
    buf.pool = pool;
    lPrintModuleText(&buf, module);
    ok = ! buf.failed;
    if (ok && buf.size != 0) {
        if (fwrite(buf.data, 1, buf.size, out) != buf.size)
            ok = 0;
    }
    if (ok && (fflush(out) != 0 || ferror(out)))
        ok = 0;
    mem_FreePool(pool);
    return ok;
} // CgIRPrintModule
