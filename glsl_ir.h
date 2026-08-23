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
// glsl_ir.h
//

#if !defined(__GLSL_IR_H)
#define __GLSL_IR_H 1

#include <stddef.h>

enum GlslStage_Enum {
    GLSL_STAGE_VERTEX,
    GLSL_STAGE_FRAGMENT
};

typedef enum GlslStage_Enum GlslStage;

typedef void *(*GlslAllocFn)(void *arg, size_t size);

typedef enum GlslBase_Enum {
    GLSL_BASE_VOID,
    GLSL_BASE_FLOAT,
    GLSL_BASE_INT,
    GLSL_BASE_BOOL,
    GLSL_BASE_SAMPLER1D,
    GLSL_BASE_SAMPLER2D,
    GLSL_BASE_SAMPLER3D,
    GLSL_BASE_SAMPLERCUBE,
    GLSL_BASE_STRUCT
} GlslBase;

typedef enum GlslBuiltin_Enum {
    GLSL_BUILTIN_NONE,
    GLSL_BUILTIN_MUL,
    GLSL_BUILTIN_DOT,
    GLSL_BUILTIN_CROSS,
    GLSL_BUILTIN_NORMALIZE,
    GLSL_BUILTIN_REFLECT,
    GLSL_BUILTIN_REFRACT,
    GLSL_BUILTIN_LENGTH,
    GLSL_BUILTIN_DISTANCE,
    GLSL_BUILTIN_MIN,
    GLSL_BUILTIN_MAX,
    GLSL_BUILTIN_CLAMP,
    GLSL_BUILTIN_ABS,
    GLSL_BUILTIN_SIGN,
    GLSL_BUILTIN_FLOOR,
    GLSL_BUILTIN_CEIL,
    GLSL_BUILTIN_SQRT,
    GLSL_BUILTIN_EXP,
    GLSL_BUILTIN_EXP2,
    GLSL_BUILTIN_LOG,
    GLSL_BUILTIN_LOG2,
    GLSL_BUILTIN_SIN,
    GLSL_BUILTIN_COS,
    GLSL_BUILTIN_TAN,
    GLSL_BUILTIN_ASIN,
    GLSL_BUILTIN_ACOS,
    GLSL_BUILTIN_ATAN,
    GLSL_BUILTIN_RSQRT,
    GLSL_BUILTIN_LERP,
    GLSL_BUILTIN_FRAC,
    GLSL_BUILTIN_SATURATE,
    GLSL_BUILTIN_TEX1D,
    GLSL_BUILTIN_TEX2D,
    GLSL_BUILTIN_TEX3D,
    GLSL_BUILTIN_TEXCUBE
} GlslBuiltin;

typedef struct GlslDecl_Rec GlslDecl;

typedef struct GlslType_Rec {
    GlslBase base;
    int len;
    int rows;
    int cols;
    int arraySize;
    const char *structName;
    struct GlslType_Rec *elementType;
    GlslDecl *members;
} GlslType;

typedef struct GlslName_Rec {
    struct GlslName_Rec *next;
    const void *nameSpace;
    const void *identity;
    const char *source;
    const char *emitted;
} GlslName;

typedef enum GlslStorage_Enum {
    GLSL_STORAGE_NONE,
    GLSL_STORAGE_CONST,
    GLSL_STORAGE_ATTRIBUTE,
    GLSL_STORAGE_VARYING,
    GLSL_STORAGE_UNIFORM,
    GLSL_STORAGE_SAMPLER,
    GLSL_STORAGE_BUILTIN
} GlslStorage;

typedef enum GlslParameterQualifier_Enum {
    GLSL_PARAMETER_IN,
    GLSL_PARAMETER_OUT,
    GLSL_PARAMETER_INOUT
} GlslParameterQualifier;

typedef enum GlslExprKind_Enum {
    GLSL_EXPR_SYMBOL, GLSL_EXPR_INT, GLSL_EXPR_FLOAT, GLSL_EXPR_BOOL,
    GLSL_EXPR_UNARY, GLSL_EXPR_BINARY, GLSL_EXPR_CONDITIONAL,
    GLSL_EXPR_CALL, GLSL_EXPR_CONSTRUCT, GLSL_EXPR_MEMBER,
    GLSL_EXPR_INDEX, GLSL_EXPR_SWIZZLE
} GlslExprKind;

typedef enum GlslOperator_Enum {
    GLSL_OP_NONE,
    GLSL_OP_ASSIGN,
    GLSL_OP_LOGICAL_OR,
    GLSL_OP_LOGICAL_AND,
    GLSL_OP_EQUAL,
    GLSL_OP_NOT_EQUAL,
    GLSL_OP_LESS,
    GLSL_OP_GREATER,
    GLSL_OP_LESS_EQUAL,
    GLSL_OP_GREATER_EQUAL,
    GLSL_OP_ADD,
    GLSL_OP_SUBTRACT,
    GLSL_OP_MULTIPLY,
    GLSL_OP_DIVIDE,
    GLSL_OP_NEGATE,
    GLSL_OP_POSITIVE,
    GLSL_OP_LOGICAL_NOT
} GlslOperator;

typedef enum GlslStmtKind_Enum {
    GLSL_STMT_EXPRESSION, GLSL_STMT_IF, GLSL_STMT_WHILE, GLSL_STMT_DO,
    GLSL_STMT_FOR, GLSL_STMT_BLOCK, GLSL_STMT_RETURN, GLSL_STMT_DISCARD,
    GLSL_STMT_BREAK, GLSL_STMT_CONTINUE
} GlslStmtKind;

typedef struct GlslLoc_Rec {
    int file;
    int line;
} GlslLoc;

typedef struct GlslExpr_Rec GlslExpr;
typedef struct GlslStmt_Rec GlslStmt;
typedef struct GlslFunction_Rec GlslFunction;
typedef struct GlslBinding_Rec GlslBinding;

struct GlslExpr_Rec {
    GlslExpr *next;
    GlslExprKind kind;
    GlslType type;
    GlslLoc loc;
    union {
        GlslDecl *symbol;
        int literalInt;
        float literalFloat;
        int literalBool;
        struct {
            GlslOperator op;
            GlslExpr *operand;
        } unary;
        struct {
            GlslOperator op;
            GlslExpr *left;
            GlslExpr *right;
        } binary;
        struct {
            GlslExpr *condition;
            GlslExpr *trueExpr;
            GlslExpr *falseExpr;
        } conditional;
        struct {
            GlslExpr *target;
            const char *name;
            GlslExpr *arguments;
            GlslBuiltin builtin;
        } call;
        struct {
            GlslExpr *arguments;
        } construct;
        struct {
            GlslExpr *object;
            GlslDecl *decl;
            const char *name;
        } member;
        struct {
            GlslExpr *object;
            GlslExpr *index;
        } index;
        struct {
            GlslExpr *object;
            const char *mask;
        } swizzle;
    } u;
};

struct GlslStmt_Rec {
    GlslStmt *next;
    GlslStmtKind kind;
    GlslLoc loc;
    union {
        GlslExpr *expression;
        struct {
            GlslExpr *condition;
            GlslStmt *trueBranch;
            GlslStmt *falseBranch;
        } ifStmt;
        struct {
            GlslExpr *condition;
            GlslStmt *body;
        } loop;
        struct {
            GlslStmt *init;
            GlslExpr *condition;
            GlslStmt *step;
            GlslStmt *body;
        } forStmt;
        GlslStmt *block;
        GlslExpr *returnExpr;
    } u;
};

struct GlslDecl_Rec {
    GlslDecl *next;
    GlslStorage storage;
    GlslType type;
    const char *name;
    GlslLoc loc;
    int sourceOrdinal;
    GlslExpr *initializer;
    const void *identity;
    GlslDecl *members;
    GlslParameterQualifier parameterQualifier;
};

struct GlslFunction_Rec {
    GlslFunction *next;
    GlslType result;
    const char *name;
    GlslLoc loc;
    const void *identity;
    GlslDecl *parameters;
    GlslDecl *locals;
    GlslStmt *body;
    int isEntry;
    int needsPrototype;
    int visitState;
};

struct GlslBinding_Rec {
    GlslBinding *next;
    GlslStorage storage;
    const char *name;
    const char *semantic;
    const char *interfaceKey;
    GlslLoc loc;
    int sourceOrdinal;
    GlslDecl *declaration;
    int isOutput;
    int defaultCount;
    float *defaultValues;
};

typedef struct GlslModule_Rec {
    GlslStage stage;
    GlslAllocFn alloc;
    void *allocArg;
    GlslName *names;
    GlslDecl *structs;
    GlslDecl *globals;
    GlslFunction *functions;
    GlslFunction *entry;
    GlslBinding *bindings;
    GlslLoc errorLoc;
    const char *errorReason;
    const char *resourceName;
    int resourceUsed;
    int resourceAvailable;
    int errors;
} GlslModule;

void GlslInitModule(GlslModule *module, GlslStage stage,
    GlslAllocFn alloc, void *allocArg);
const char *GlslAllocateName(GlslModule *module, const char *source);
const char *GlslAllocateSymbolName(GlslModule *module, const void *identity,
    const char *source);
const char *GlslAllocateScopedSymbolName(GlslModule *module,
    const void *nameSpace, const void *identity, const char *source);
const char *GlslAllocateDistinctName(GlslModule *module, const char *source);
GlslType GlslNumericType(GlslBase base, int len);
GlslType GlslMatrixType(int size);
const char *GlslTypeName(const GlslType *type);
GlslBuiltin GlslLookupBuiltin(const char *name, const GlslType *result,
    const GlslType *params, int paramCount);
const char *GlslBuiltinSpelling(GlslBuiltin builtin);
int GlslTypeComponentCount(const GlslType *type);
int GlslIsReservedName(const char *name);
GlslDecl *GlslNewDecl(GlslModule *module, GlslStorage storage,
    GlslType type, const char *name);
GlslExpr *GlslNewExpr(GlslModule *module, GlslExprKind kind, GlslType type);
GlslStmt *GlslNewStmt(GlslModule *module, GlslStmtKind kind);
GlslFunction *GlslNewFunction(GlslModule *module, GlslType result,
    const char *name);
GlslBinding *GlslNewBinding(GlslModule *module, GlslStorage storage,
    const char *name, const char *semantic);
void GlslAppendDecl(GlslDecl **list, GlslDecl *decl);
void GlslAppendStmt(GlslStmt **list, GlslStmt *stmt);
void GlslAppendFunction(GlslFunction **list, GlslFunction *function);

#endif // !defined(__GLSL_IR_H)
