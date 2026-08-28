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
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN
ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
OF THE NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF
CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE,
EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/
// hlsl_ir.h
//

#if !defined(__HLSL_IR_H)
#define __HLSL_IR_H 1

#include <stddef.h>
#include <stdio.h>

typedef enum HlslStage_Enum {
    HLSL_STAGE_VERTEX,
    HLSL_STAGE_PIXEL
} HlslStage;

typedef enum HlslBase_Enum {
    HLSL_BASE_VOID,
    HLSL_BASE_FLOAT,
    HLSL_BASE_INT,
    HLSL_BASE_BOOL,
    HLSL_BASE_SAMPLER1D,
    HLSL_BASE_SAMPLER2D,
    HLSL_BASE_SAMPLER3D,
    HLSL_BASE_SAMPLERCUBE,
    HLSL_BASE_STRUCT
} HlslBase;

typedef enum HlslRegisterBank_Enum {
    HLSL_REGISTER_NONE,
    HLSL_REGISTER_C,
    HLSL_REGISTER_I,
    HLSL_REGISTER_B,
    HLSL_REGISTER_S
} HlslRegisterBank;

typedef enum HlslErrorKind_Enum {
    HLSL_ERROR_NONE,
    HLSL_ERROR_UNSUPPORTED_TYPE,
    HLSL_ERROR_UNSUPPORTED_OPERATION,
    HLSL_ERROR_STAGE_OPERATION,
    HLSL_ERROR_SEMANTIC,
    HLSL_ERROR_INTERFACE_CONFLICT,
    HLSL_ERROR_REQUIRED_POSITION,
    HLSL_ERROR_ENTRY_ABI,
    HLSL_ERROR_REGISTER_COLLISION,
    HLSL_ERROR_RESOURCE_LIMIT,
    HLSL_ERROR_SAMPLER,
    HLSL_ERROR_INTRINSIC,
    HLSL_ERROR_NAME_COLLISION,
    HLSL_ERROR_INVALID_IR
} HlslErrorKind;

typedef enum HlslStorage_Enum {
    HLSL_STORAGE_NONE,
    HLSL_STORAGE_STATIC,
    HLSL_STORAGE_CONST,
    HLSL_STORAGE_INPUT,
    HLSL_STORAGE_OUTPUT,
    HLSL_STORAGE_UNIFORM,
    HLSL_STORAGE_SAMPLER,
    HLSL_STORAGE_BUILTIN
} HlslStorage;

typedef enum HlslParameterQualifier_Enum {
    HLSL_PARAMETER_IN,
    HLSL_PARAMETER_OUT,
    HLSL_PARAMETER_INOUT
} HlslParameterQualifier;

typedef enum HlslExprKind_Enum {
    HLSL_EXPR_SYMBOL,
    HLSL_EXPR_INT,
    HLSL_EXPR_FLOAT,
    HLSL_EXPR_BOOL,
    HLSL_EXPR_UNARY,
    HLSL_EXPR_BINARY,
    HLSL_EXPR_CONDITIONAL,
    HLSL_EXPR_CALL,
    HLSL_EXPR_CONSTRUCT,
    HLSL_EXPR_CAST,
    HLSL_EXPR_MEMBER,
    HLSL_EXPR_INDEX,
    HLSL_EXPR_SWIZZLE
} HlslExprKind;

typedef enum HlslOperator_Enum {
    HLSL_OP_NONE,
    HLSL_OP_ASSIGN,
    HLSL_OP_ADD_ASSIGN,
    HLSL_OP_SUBTRACT_ASSIGN,
    HLSL_OP_MULTIPLY_ASSIGN,
    HLSL_OP_DIVIDE_ASSIGN,
    HLSL_OP_REMAINDER_ASSIGN,
    HLSL_OP_BITWISE_OR_ASSIGN,
    HLSL_OP_BITWISE_XOR_ASSIGN,
    HLSL_OP_BITWISE_AND_ASSIGN,
    HLSL_OP_SHIFT_LEFT_ASSIGN,
    HLSL_OP_SHIFT_RIGHT_ASSIGN,
    HLSL_OP_LOGICAL_OR,
    HLSL_OP_LOGICAL_AND,
    HLSL_OP_BITWISE_OR,
    HLSL_OP_BITWISE_XOR,
    HLSL_OP_BITWISE_AND,
    HLSL_OP_EQUAL,
    HLSL_OP_NOT_EQUAL,
    HLSL_OP_LESS,
    HLSL_OP_GREATER,
    HLSL_OP_LESS_EQUAL,
    HLSL_OP_GREATER_EQUAL,
    HLSL_OP_SHIFT_LEFT,
    HLSL_OP_SHIFT_RIGHT,
    HLSL_OP_ADD,
    HLSL_OP_SUBTRACT,
    HLSL_OP_MULTIPLY,
    HLSL_OP_DIVIDE,
    HLSL_OP_REMAINDER,
    HLSL_OP_NEGATE,
    HLSL_OP_POSITIVE,
    HLSL_OP_LOGICAL_NOT,
    HLSL_OP_BITWISE_NOT,
    HLSL_OP_PRE_INCREMENT,
    HLSL_OP_PRE_DECREMENT,
    HLSL_OP_POST_INCREMENT,
    HLSL_OP_POST_DECREMENT
} HlslOperator;

typedef enum HlslStmtKind_Enum {
    HLSL_STMT_DECLARATION,
    HLSL_STMT_EXPRESSION,
    HLSL_STMT_IF,
    HLSL_STMT_WHILE,
    HLSL_STMT_DO,
    HLSL_STMT_FOR,
    HLSL_STMT_BLOCK,
    HLSL_STMT_RETURN,
    HLSL_STMT_DISCARD,
    HLSL_STMT_BREAK,
    HLSL_STMT_CONTINUE
} HlslStmtKind;

typedef struct HlslLoc_Rec {
    int file;
    int line;
} HlslLoc;

typedef struct HlslType_Rec HlslType;
typedef struct HlslName_Rec HlslName;
typedef struct HlslDecl_Rec HlslDecl;
typedef struct HlslExpr_Rec HlslExpr;
typedef struct HlslStmt_Rec HlslStmt;
typedef struct HlslFunction_Rec HlslFunction;
typedef struct HlslBinding_Rec HlslBinding;
typedef struct HlslProfileDesc_Rec HlslProfileDesc;
typedef struct HlslModule_Rec HlslModule;
typedef void *(*HlslAllocFn)(void *arg, size_t size);

struct HlslType_Rec {
    HlslBase base;
    int len;
    int rows;
    int cols;
    int arraySize;
    const char *structName;
    HlslType *elementType;
    HlslDecl *members;
};

typedef struct HlslPhysicalBinding_Rec {
    HlslRegisterBank bank;
    int regno;
    int span;
    int component;
} HlslPhysicalBinding;

struct HlslName_Rec {
    HlslName *next;
    const void *nameSpace;
    const void *identity;
    const char *source;
    const char *emitted;
};

struct HlslExpr_Rec {
    HlslExpr *next;
    HlslExprKind kind;
    HlslType type;
    HlslLoc loc;
    union {
        HlslDecl *symbol;
        int literalInt;
        float literalFloat;
        int literalBool;
        struct {
            HlslOperator op;
            HlslExpr *operand;
        } unary;
        struct {
            HlslOperator op;
            HlslExpr *left;
            HlslExpr *right;
        } binary;
        struct {
            HlslExpr *condition;
            HlslExpr *trueExpr;
            HlslExpr *falseExpr;
        } conditional;
        struct {
            HlslFunction *function;
            const char *name;
            HlslExpr *arguments;
        } call;
        struct {
            HlslExpr *arguments;
        } construct;
        struct {
            HlslExpr *expression;
        } cast;
        struct {
            HlslExpr *object;
            HlslDecl *decl;
            const char *name;
        } member;
        struct {
            HlslExpr *object;
            HlslExpr *index;
        } index;
        struct {
            HlslExpr *object;
            const char *mask;
        } swizzle;
    } u;
};

struct HlslStmt_Rec {
    HlslStmt *next;
    HlslStmtKind kind;
    HlslLoc loc;
    union {
        HlslDecl *declaration;
        HlslExpr *expression;
        struct {
            HlslExpr *condition;
            HlslStmt *trueBranch;
            HlslStmt *falseBranch;
        } ifStmt;
        struct {
            HlslExpr *condition;
            HlslStmt *body;
        } loop;
        struct {
            HlslStmt *init;
            HlslExpr *condition;
            HlslStmt *step;
            HlslStmt *body;
        } forStmt;
        HlslStmt *block;
        HlslExpr *returnExpr;
    } u;
};

struct HlslDecl_Rec {
    HlslDecl *next;
    HlslStorage storage;
    HlslType type;
    const char *name;
    const char *semantic;
    HlslLoc loc;
    int sourceOrdinal;
    HlslExpr *initializer;
    const void *identity;
    HlslDecl *members;
    HlslParameterQualifier parameterQualifier;
    HlslPhysicalBinding physical;
};

struct HlslFunction_Rec {
    HlslFunction *next;
    HlslType result;
    const char *name;
    HlslLoc loc;
    const void *identity;
    HlslDecl *parameters;
    HlslDecl *locals;
    HlslStmt *body;
    int isEntry;
    int needsPrototype;
    int visitState;
};

struct HlslBinding_Rec {
    HlslBinding *next;
    HlslStorage storage;
    HlslType type;
    const char *name;
    const char *semantic;
    HlslLoc loc;
    int sourceOrdinal;
    HlslDecl *declaration;
    int isOutput;
    int defaultCount;
    float *defaultValues;
    int sourceBase;
    HlslPhysicalBinding physical;
};

struct HlslModule_Rec {
    HlslStage stage;
    HlslAllocFn alloc;
    void *allocArg;
    HlslName *names;
    HlslDecl *structs;
    HlslDecl *globals;
    HlslFunction *functions;
    HlslFunction *entry;
    HlslFunction *wrapper;
    HlslBinding *bindings;
    HlslLoc errorLoc;
    HlslErrorKind errorKind;
    const char *errorReason;
    const char *resourceName;
    int resourceUsed;
    int resourceAvailable;
    int errors;
};

void HlslInitModule(HlslModule *module, HlslStage stage,
    HlslAllocFn alloc, void *allocArg);

const char *HlslAllocateName(HlslModule *module, const char *source);
const char *HlslAllocateSymbolName(HlslModule *module,
    const void *identity, const char *source);
const char *HlslAllocateScopedSymbolName(HlslModule *module,
    const void *nameSpace, const void *identity, const char *source);
const char *HlslAllocateDistinctName(HlslModule *module,
    const char *source);

HlslType HlslNumericType(HlslBase base, int len);
HlslType HlslMatrixType(int rows, int cols);
const char *HlslTypeName(const HlslType *type);
int HlslTypeRegisterSpan(const HlslType *type);
int HlslIsReservedName(const char *name);
int HlslReservedNameCount(void);
const char *HlslReservedNameAt(int index);

HlslDecl *HlslNewDecl(HlslModule *module, HlslStorage storage,
    HlslType type, const char *name);
HlslExpr *HlslNewExpr(HlslModule *module, HlslExprKind kind,
    HlslType type);
HlslStmt *HlslNewStmt(HlslModule *module, HlslStmtKind kind);
HlslFunction *HlslNewFunction(HlslModule *module, HlslType result,
    const char *name);
HlslBinding *HlslNewBinding(HlslModule *module, HlslStorage storage,
    HlslType type, const char *name, const char *semantic);

void HlslAppendDecl(HlslDecl **list, HlslDecl *decl);
void HlslAppendExpr(HlslExpr **list, HlslExpr *expr);
void HlslAppendStmt(HlslStmt **list, HlslStmt *stmt);
void HlslAppendFunction(HlslFunction **list, HlslFunction *function);
void HlslAppendBinding(HlslBinding **list, HlslBinding *binding);

int HlslWriteModule(FILE *out, const HlslModule *module,
    const HlslProfileDesc *profile);

#endif // !defined(__HLSL_IR_H)
