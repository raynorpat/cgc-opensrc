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
// cg_ir.h - Backend-neutral Cg IR: stable expression and statement
//        kinds, module-owned declarations, functions, expressions, and
//        statements, plus the typed builders that construct them.
//
// Ownership model: every node belongs to exactly one CgIRModule.  Nodes
// are allocated exclusively through the module's allocator callback and
// never freed individually; releasing the allocator's storage releases
// the whole module graph at once.  Any allocation failure marks the
// module failed (CgIRModuleFailed), every later builder call returns
// NULL without allocating, and no builder ever returns a partially
// initialized node.
//
// Node prefixes are stable: every expression begins with kind, canonical
// Type *, SourceLoc, synthesized flag, lvalue flag, and side-effect
// flag; every statement begins with kind, SourceLoc, synthesized flag,
// and next pointer.  Builders zero-initialize each node first, so a
// NULL location yields an all-zero SourceLoc and absent payloads stay
// NULL/zero even over dirty allocator memory.  Flag policy (which nodes
// are synthesized, lvalues, or side-effecting) belongs to callers and
// later validation, not to builders; builders assert only cheap local
// requirements such as non-NULL operands and types.
//
// Include discipline: this header follows the repository prelude
// convention (see cg_stdlib.h) and consumes SourceLoc, Type, Symbol,
// and TypeList; include "slglobals.h" before it in translation units.
//

#if !defined(__CG_IR_H)
#define __CG_IR_H 1

#include <stddef.h>

#include "cg_stdlib.h"

/*
 * Stable expression kinds, in lowering order.  One builder per kind
 * below; calls carry their resolved identity (ordinary Symbol,
 * declared interface method, or catalog intrinsic signature).
 */

typedef enum CgIRExprKind_Rec {
    CGIR_EXPR_CONSTANT,
    CGIR_EXPR_SYMBOL,
    CGIR_EXPR_MEMBER,
    CGIR_EXPR_INDEX,
    CGIR_EXPR_LENGTH,
    CGIR_EXPR_SWIZZLE,
    CGIR_EXPR_CONSTRUCT,
    CGIR_EXPR_CAST,
    CGIR_EXPR_UNARY,
    CGIR_EXPR_BINARY,
    CGIR_EXPR_ASSIGN,
    CGIR_EXPR_CONDITIONAL,
    CGIR_EXPR_CALL,
    CGIR_EXPR_INTERFACE_CALL,
    CGIR_EXPR_INTRINSIC
} CgIRExprKind;

/*
 * Stable structured statement kinds.  Control flow stays nested: loops
 * keep their condition and body, blocks keep their inner list, and no
 * builder flattens, unrolls, or lowers aggregate operations.
 */

typedef enum CgIRStmtKind_Rec {
    CGIR_STMT_BLOCK,
    CGIR_STMT_DECL,
    CGIR_STMT_EXPR,
    CGIR_STMT_IF,
    CGIR_STMT_WHILE,
    CGIR_STMT_DO,
    CGIR_STMT_FOR,
    CGIR_STMT_RETURN,
    CGIR_STMT_BREAK,
    CGIR_STMT_CONTINUE,
    CGIR_STMT_DISCARD
} CgIRStmtKind;

/*
 * CgIROp - shape-independent operator identity for unary, binary, and
 *          assignment expressions.  Operand and result shapes live in
 *          the canonical types, so the IR needs one opcode per source
 *          operator rather than the frontend's per-shape explosion.
 *          Compound assignments keep their own opcodes so printing and
 *          verification see source-level structure.
 */

typedef enum CgIROp_Rec {
    CGIR_OP_NONE = 0,
    /* Unary */
    CGIR_OP_NEGATE,
    CGIR_OP_POSITIVE,
    CGIR_OP_LOGICAL_NOT,
    CGIR_OP_BITWISE_NOT,
    CGIR_OP_PRE_INCREMENT,
    CGIR_OP_PRE_DECREMENT,
    CGIR_OP_POST_INCREMENT,
    CGIR_OP_POST_DECREMENT,
    /* Binary arithmetic */
    CGIR_OP_MULTIPLY,
    CGIR_OP_DIVIDE,
    CGIR_OP_MODULO,
    CGIR_OP_ADD,
    CGIR_OP_SUBTRACT,
    CGIR_OP_SHIFT_LEFT,
    CGIR_OP_SHIFT_RIGHT,
    /* Relational and equality */
    CGIR_OP_LESS,
    CGIR_OP_GREATER,
    CGIR_OP_LESS_EQUAL,
    CGIR_OP_GREATER_EQUAL,
    CGIR_OP_EQUAL,
    CGIR_OP_NOT_EQUAL,
    /* Bitwise */
    CGIR_OP_BITWISE_AND,
    CGIR_OP_BITWISE_XOR,
    CGIR_OP_BITWISE_OR,
    /* Short-circuit logical */
    CGIR_OP_LOGICAL_AND,
    CGIR_OP_LOGICAL_OR,
    /* Assignment family */
    CGIR_OP_ASSIGN,
    CGIR_OP_ADD_ASSIGN,
    CGIR_OP_SUBTRACT_ASSIGN,
    CGIR_OP_MULTIPLY_ASSIGN,
    CGIR_OP_DIVIDE_ASSIGN,
    CGIR_OP_MODULO_ASSIGN
} CgIROp;

/*
 * Declared storage class and effective program domain of one
 * declaration.  Storage records what the source said (CONST, UNIFORM,
 * VARYING, or NONE for locals, temporaries, and formals); domain
 * records the program-interface role assigned by the language rules
 * (entry formals become VARYING, globals UNIFORM).  Parameter direction
 * (in/out/inout) rides the canonical type's qualifier bits, not here.
 */

typedef enum CgIRStorage_Rec {
    CGIR_STORAGE_NONE = 0,
    CGIR_STORAGE_CONST,
    CGIR_STORAGE_UNIFORM,
    CGIR_STORAGE_VARYING
} CgIRStorage;

typedef enum CgIRDomain_Rec {
    CGIR_DOMAIN_NONE = 0,
    CGIR_DOMAIN_UNIFORM,
    CGIR_DOMAIN_VARYING
} CgIRDomain;

typedef struct CgIRExpr_Rec CgIRExpr;
typedef struct CgIRStmt_Rec CgIRStmt;
typedef struct CgIRDecl_Rec CgIRDecl;
typedef struct CgIRFunction_Rec CgIRFunction;
typedef struct CgIRModule_Rec CgIRModule;

/*
 * CgIRDecl - one declared object: global uniform/varying, formal
 *          parameter, or function local.  "symbol" is the resolved
 *          identity (NULL only for compiler-synthesized temporaries);
 *          "name" repeats the source name atom so synthesized decls can
 *          still spell themselves.  "semantic" is the binding semantic
 *          atom, 0 when none was declared.
 */

struct CgIRDecl_Rec {
    CgIRDecl *next;
    Symbol *symbol;
    int name;
    Type *type;
    CgIRStorage storage;
    CgIRDomain domain;
    int semantic;
    CgIRExpr *initializer;
    SourceLoc loc;
};

/*
 * CgIRFunction - one function definition: ordered parameters, ordered
 *          locals, and the structured body.  "isEntry" marks the
 *          selected entry point; module->entry points at it.
 */

struct CgIRFunction_Rec {
    CgIRFunction *next;
    Symbol *symbol;
    Type *resultType;
    CgIRDecl *parameters;
    CgIRDecl *locals;
    CgIRStmt *body;
    SourceLoc loc;
    int isEntry;
};

/*
 * CgIRExpr - typed expression node.  Swizzle masks use the frontend
 *          encoding: two bits per result element, element i held in
 *          bits 2*i..2*i+1 as its source component index, so ".xz"
 *          stores mask 0x8 with componentCount 2.  Constants carry a
 *          typed CgNumericValue (kind plus value).
 */

struct CgIRExpr_Rec {
    CgIRExprKind kind;
    Type *type;
    SourceLoc loc;
    int synthesized;
    int isLvalue;
    int sideEffects;
    CgIRExpr *next;
    union {
        CgNumericValue constant;
        Symbol *symbol;
        struct {
            CgIRExpr *object;
            Symbol *member;
        } member;
        struct {
            CgIRExpr *object;
            CgIRExpr *index;
        } index;
        struct {
            CgIRExpr *object;
        } length;
        struct {
            CgIRExpr *object;
            int mask;
            int componentCount;
        } swizzle;
        struct {
            CgIRExpr *arguments;
        } construct;
        struct {
            CgIRExpr *operand;
        } cast;
        struct {
            CgIROp op;
            CgIRExpr *operand;
        } unary;
        struct {
            CgIROp op;
            CgIRExpr *left;
            CgIRExpr *right;
        } binary;
        struct {
            CgIROp op;
            CgIRExpr *target;
            CgIRExpr *value;
        } assign;
        struct {
            CgIRExpr *condition;
            CgIRExpr *trueExpr;
            CgIRExpr *falseExpr;
        } conditional;
        struct {
            Symbol *callee;
            CgIRExpr *arguments;
        } call;
        struct {
            Symbol *method;
            CgIRExpr *receiver;
            CgIRExpr *arguments;
        } interfaceCall;
        struct {
            CgIntrinsic intrinsic;
            const CgIntrinsicSignature *signature;
            CgIRExpr *arguments;
        } intrinsicCall;
    } u;
};

/*
 * CgIRStmt - structured statement node.  WHILE and DO share u.loop;
 * FOR keeps its init as a statement (declaration or expression) and
 * its step as an expression; DISCARD carries an optional Boolean
 * predicate (NULL for bare discard).
 */

struct CgIRStmt_Rec {
    CgIRStmtKind kind;
    SourceLoc loc;
    int synthesized;
    CgIRStmt *next;
    union {
        CgIRStmt *block;
        CgIRDecl *decl;
        CgIRExpr *expression;
        struct {
            CgIRExpr *condition;
            CgIRStmt *trueBranch;
            CgIRStmt *falseBranch;
        } ifStmt;
        struct {
            CgIRExpr *condition;
            CgIRStmt *body;
        } loop;
        struct {
            CgIRStmt *init;
            CgIRExpr *condition;
            CgIRExpr *step;
            CgIRStmt *body;
        } forStmt;
        CgIRExpr *returnExpr;
        struct {
            CgIRExpr *condition;
        } discard;
    } u;
};

/*
 * CgIRModule - single owner of one IR graph: allocator callback pair,
 * sticky failed flag, selected entry, ordered globals, ordered
 * functions, and the profile identity being compiled.
 */

struct CgIRModule_Rec {
    void *(*alloc)(void *arg, size_t size);
    void *allocArg;
    int failed;
    CgIRFunction *entry;
    CgIRDecl *globals;
    CgIRFunction *functions;
    const CgProfileIdentity *profile;
};

//////////////////////// Module initialization ////////////////////////

/*
 * CgIRInitModule() - Prepare "module" for building.  Does not
 *          allocate; "alloc"/"allocArg" back every node this module
 *          will own.
 */

void CgIRInitModule(CgIRModule *module, void *(*alloc)(void *, size_t),
                    void *allocArg);

/*
 * CgIRModuleFailed() - Nonzero once any allocation failed for this
 *          module.  Failure is sticky for the module's lifetime.
 */

int CgIRModuleFailed(const CgIRModule *module);

///////////////////// Declarations and functions //////////////////////

/*
 * CgIRNewDecl() - Build one declaration node.  "name" is the source
 *          name atom (0 for anonymous temporaries); "semantic" is the
 *          binding semantic atom (0 when none).  Returns NULL and
 *          marks the module failed on allocation failure.
 */

CgIRDecl *CgIRNewDecl(CgIRModule *module, Symbol *symbol, int name,
                      Type *type, CgIRStorage storage, CgIRDomain domain,
                      int semantic, CgIRExpr *initializer,
                      const SourceLoc *loc);

/*
 * CgIRNewFunction() - Build one empty function node.  Parameters,
 *          locals, body, entry selection, and module registration are
 *          filled in by callers.
 */

CgIRFunction *CgIRNewFunction(CgIRModule *module, Symbol *symbol,
                              Type *resultType, const SourceLoc *loc);

/*
 * List appends preserving insertion order.  No allocation: they link
 * existing nodes through their next fields.
 */

void CgIRAppendDecl(CgIRDecl **list, CgIRDecl *decl);
void CgIRAppendFunction(CgIRFunction **list, CgIRFunction *function);
void CgIRAppendStmt(CgIRStmt **list, CgIRStmt *stmt);
void CgIRAppendExpr(CgIRExpr **list, CgIRExpr *expr);

//////////////////////// Expression builders //////////////////////////

/*
 * One builder per expression kind.  Each takes the explicit canonical
 * result type and source location (NULL location means
 * compiler-synthesized and stores all zeroes).  Encoding authority:
 * the empty (all-zero) location is authoritative, so a node whose
 * location is all zeroes is compiler-synthesized.  Builders leave the
 * synthesized flag clear, making those nodes synthesized by
 * convention; callers that instead mark synthesis through the
 * synthesized flag must keep the flag coherent with the location -
 * setting the flag requires a real (non-empty) location on the same
 * node, and leaving it clear over a zero location still denotes
 * conventionally synthesized.  Argument lists are
 * NULL-terminated CgIRExpr chains built with CgIRAppendExpr; NULL
 * arguments mean an empty list.  Only cheap local assertions apply;
 * complete validation is the verifier's job.
 */

CgIRExpr *CgIRNewConstant(CgIRModule *module, Type *type,
                          const SourceLoc *loc,
                          const CgNumericValue *value);

CgIRExpr *CgIRNewSymbol(CgIRModule *module, Type *type,
                        const SourceLoc *loc, Symbol *symbol);

CgIRExpr *CgIRNewMember(CgIRModule *module, Type *type,
                        const SourceLoc *loc, CgIRExpr *object,
                        Symbol *member);

CgIRExpr *CgIRNewIndex(CgIRModule *module, Type *type,
                       const SourceLoc *loc, CgIRExpr *object,
                       CgIRExpr *index);

CgIRExpr *CgIRNewLength(CgIRModule *module, Type *type,
                        const SourceLoc *loc, CgIRExpr *object);

CgIRExpr *CgIRNewSwizzle(CgIRModule *module, Type *type,
                         const SourceLoc *loc, CgIRExpr *object,
                         int mask, int componentCount);

CgIRExpr *CgIRNewConstruct(CgIRModule *module, Type *type,
                           const SourceLoc *loc, CgIRExpr *arguments);

CgIRExpr *CgIRNewCast(CgIRModule *module, Type *type,
                      const SourceLoc *loc, CgIRExpr *operand);

CgIRExpr *CgIRNewUnary(CgIRModule *module, Type *type,
                       const SourceLoc *loc, CgIROp op, CgIRExpr *operand);

CgIRExpr *CgIRNewBinary(CgIRModule *module, Type *type,
                        const SourceLoc *loc, CgIROp op, CgIRExpr *left,
                        CgIRExpr *right);

CgIRExpr *CgIRNewAssign(CgIRModule *module, Type *type,
                        const SourceLoc *loc, CgIROp op, CgIRExpr *target,
                        CgIRExpr *value);

CgIRExpr *CgIRNewConditional(CgIRModule *module, Type *type,
                             const SourceLoc *loc, CgIRExpr *condition,
                             CgIRExpr *trueExpr, CgIRExpr *falseExpr);

CgIRExpr *CgIRNewCall(CgIRModule *module, Type *type,
                      const SourceLoc *loc, Symbol *callee,
                      CgIRExpr *arguments);

CgIRExpr *CgIRNewInterfaceCall(CgIRModule *module, Type *type,
                               const SourceLoc *loc, Symbol *method,
                               CgIRExpr *receiver, CgIRExpr *arguments);

CgIRExpr *CgIRNewIntrinsicCall(CgIRModule *module, Type *type,
                               const SourceLoc *loc, CgIntrinsic intrinsic,
                               const CgIntrinsicSignature *signature,
                               CgIRExpr *arguments);

//////////////////////// Statement builders ///////////////////////////

/*
 * One builder per statement kind.  Locations follow the same NULL-
 * means-synthesized rule as expressions; bodies and branches are
 * statement lists linked through their next fields.
 */

CgIRStmt *CgIRNewBlockStmt(CgIRModule *module, const SourceLoc *loc);

CgIRStmt *CgIRNewDeclStmt(CgIRModule *module, const SourceLoc *loc,
                          CgIRDecl *decl);

CgIRStmt *CgIRNewExprStmt(CgIRModule *module, const SourceLoc *loc,
                          CgIRExpr *expression);

CgIRStmt *CgIRNewIfStmt(CgIRModule *module, const SourceLoc *loc,
                        CgIRExpr *condition, CgIRStmt *trueBranch,
                        CgIRStmt *falseBranch);

CgIRStmt *CgIRNewWhileStmt(CgIRModule *module, const SourceLoc *loc,
                           CgIRExpr *condition, CgIRStmt *body);

CgIRStmt *CgIRNewDoStmt(CgIRModule *module, const SourceLoc *loc,
                        CgIRExpr *condition, CgIRStmt *body);

CgIRStmt *CgIRNewForStmt(CgIRModule *module, const SourceLoc *loc,
                         CgIRStmt *init, CgIRExpr *condition,
                         CgIRExpr *step, CgIRStmt *body);

CgIRStmt *CgIRNewReturnStmt(CgIRModule *module, const SourceLoc *loc,
                            CgIRExpr *value);

CgIRStmt *CgIRNewBreakStmt(CgIRModule *module, const SourceLoc *loc);

CgIRStmt *CgIRNewContinueStmt(CgIRModule *module, const SourceLoc *loc);

CgIRStmt *CgIRNewDiscardStmt(CgIRModule *module, const SourceLoc *loc,
                             CgIRExpr *condition);

////////////////////////// Module verification /////////////////////////

/*
 * Stable internal classification of the first invariant a module
 * violates.  Tests and callers match on these values, never on
 * user-facing text:
 *
 *   OK        - the module verified; diagnostics are zeroed.
 *   TYPE      - canonical or compatible result types disagree (binary
 *               and unary result shapes, casts, returns, assignments,
 *               constructors, element and component types).
 *   OWNER     - declaration ownership is broken: unresolved symbols,
 *               duplicate declarations, incomplete functions, entry
 *               selection, failed allocation.
 *   OPERAND   - operand counts, presence, or role types are wrong
 *               (conditions, predicates, swizzle masks, constructor
 *               data, member lookup).
 *   LVALUE    - an assignment target, update operand, or out actual is
 *               not an lvalue, or a write mask repeats components.
 *   CALL      - ordinary calls: arity, argument conversions, parameter
 *               directions, resolved callee identity or result type.
 *   INTRINSIC - intrinsic opcode/signature disagreement, argument
 *               mismatch against the signature, or result mismatch.
 *   CONTROL   - break or continue outside any enclosing loop.
 *   INTERFACE - interface dispatch: receiver interface, method owner,
 *               method arguments, or method result type.
 *   LOCATION  - a node claims provenance its location cannot honor: a
 *               set synthesized flag over an all-zero location.
 */

typedef enum CgIRVerifyReason_Rec {
    CGIR_VERIFY_OK = 0,
    CGIR_VERIFY_TYPE,
    CGIR_VERIFY_OWNER,
    CGIR_VERIFY_OPERAND,
    CGIR_VERIFY_LVALUE,
    CGIR_VERIFY_CALL,
    CGIR_VERIFY_INTRINSIC,
    CGIR_VERIFY_CONTROL,
    CGIR_VERIFY_INTERFACE,
    CGIR_VERIFY_LOCATION
} CgIRVerifyReason;

/*
 * One controlled internal diagnostic describing the first invariant
 * failure.  "loc" repeats the failing node's own source location (all
 * zeroes for synthesized nodes); "node" points at the failing CgIRExpr,
 * CgIRStmt, CgIRDecl, CgIRFunction, or the module itself for a failed
 * allocation.
 */

typedef struct CgIRVerifyDiagnostic_Rec {
    CgIRVerifyReason reason;
    SourceLoc loc;
    const void *node;
} CgIRVerifyDiagnostic;

/*
 * CgIRVerifyModule() - Verify every IR invariant before profile
 *          validation: canonical types, declaration ownership and
 *          symbol visibility, operand counts and types, lvalues and
 *          unique write-mask components, call arity/directions/
 *          signatures, intrinsic identity/signature agreement, return
 *          compatibility, control placement, interface compatibility,
 *          and required source locations.  Returns nonzero when the
 *          module verifies; returns zero at the FIRST invariant failure
 *          and fills "diagnostic" (when non-NULL) so release builds get
 *          one controlled internal diagnostic.  Verifying a module with
 *          a sticky allocation failure reports CGIR_VERIFY_OWNER about
 *          the module itself.
 *
 * Visibility authority: module globals, function parameters, and DECL
 * statements.  A function's "locals" list is emission metadata checked
 * for shape only; symbols become visible through their declarations.
 */

int CgIRVerifyModule(const CgIRModule *module,
                     CgIRVerifyDiagnostic *diagnostic);

/*
 * CgIRVerifyReasonName() - Stable lowercase spelling of a verify
 *          reason for internal diagnostics ("<invalid>" for values
 *          outside the enum).
 */

const char *CgIRVerifyReasonName(CgIRVerifyReason reason);

#endif // !defined(__CG_IR_H)
