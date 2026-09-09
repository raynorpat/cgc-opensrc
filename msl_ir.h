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

#ifndef __MSL_IR_H
#define __MSL_IR_H 1
#include <stddef.h>
#include <stdio.h>
/* Self-contained target nodes. No frontend tree or type is retained. */
typedef enum MslStage_Rec { MSL_VERTEX = 1, MSL_FRAGMENT = 2 } MslStage;
typedef enum MslBase_Rec { MSL_VOID, MSL_BOOL, MSL_INT, MSL_UINT, MSL_FLOAT, MSL_MATRIX, MSL_RECORD, MSL_ARRAY, MSL_TEXTURE2D, MSL_TEXTURECUBE } MslBase;
typedef enum MslAddressSpace_Rec { MSL_THREAD, MSL_CONSTANT } MslAddressSpace;
typedef enum MslDeclRole_Rec { MSL_VALUE_DECL, MSL_UNIFORM_DECL, MSL_RESOURCE_DECL } MslDeclRole;
typedef enum MslDirection_Rec { MSL_IN, MSL_OUT, MSL_INOUT } MslDirection;
typedef enum MslInterpolation_Rec { MSL_PERSPECTIVE, MSL_FLAT } MslInterpolation;
typedef struct MslRecord_Rec MslRecord;
typedef struct MslType_Rec { MslBase base; int lanes; MslRecord *record; int promotion; } MslType;
typedef struct MslDecl_Rec MslDecl;
typedef struct MslExpr_Rec MslExpr;
typedef struct MslStmt_Rec MslStmt;
typedef struct MslFunction_Rec MslFunction;
typedef enum MslExprKind_Rec {
    MSL_LITERAL, MSL_SYMBOL, MSL_CONSTRUCT, MSL_UNARY, MSL_BINARY,
    MSL_ASSIGN, MSL_SELECT, MSL_SWIZZLE, MSL_INDEX, MSL_CALL, MSL_MEMBER, MSL_ROW, MSL_SEQUENCE, MSL_SAMPLE
} MslExprKind;
typedef enum MslStmtKind_Rec {
    MSL_BLOCK, MSL_DECL, MSL_EXPR, MSL_IF, MSL_WHILE, MSL_DO,
    MSL_FOR, MSL_RETURN, MSL_BREAK, MSL_CONTINUE, MSL_DISCARD
} MslStmtKind;
struct MslDecl_Rec {
    MslDecl *next, *bindingNext;
    const char *name;
    MslType type;
    SourceLoc loc;
    int uniformSlot;
    int resourceSlot;
    MslDirection direction;
    MslDeclRole role;
    MslAddressSpace addressSpace;
    int readOnly, constant;
    MslExpr *defaultValue;
    const char *sourceName;
    const char *semantic;
    int attribute;
};
struct MslRecord_Rec {
    MslRecord *next;
    const char *name;
    MslDecl *members;
    int arrayCount;
};
typedef struct MslInterface_Rec {
    struct MslInterface_Rec *next;
    MslType type;
    const char *semantic, *path;
    int attribute;
    int builtin;
    MslInterpolation interpolation;
} MslInterface;
struct MslExpr_Rec {
    MslExprKind kind;
    MslType type;
    SourceLoc loc;
    MslExpr *a, *b, *c, *next;
    MslDecl *decl;
    MslFunction *function;
    const char *text;
    int postfix;
};
struct MslStmt_Rec {
    MslStmtKind kind;
    SourceLoc loc;
    MslStmt *next, *body, *other, *init;
    MslExpr *value, *step;
    MslDecl *decl;
};
typedef struct MslDependency_Rec {
    struct MslDependency_Rec *next; MslDecl *decl;
} MslDependency;
struct MslFunction_Rec {
    MslFunction *next;
    const char *name;
    MslType result;
    MslDecl *parameters, *locals;
    MslDependency *globals;
    MslStmt *body;
    SourceLoc loc;
    int entry;
    int visit;
};
typedef struct MslDiagnostic_Rec { int code; SourceLoc loc; const char *reason; const void *symbol; } MslDiagnostic;
typedef struct MslModule_Rec {
    void *(*alloc)(void *, size_t);
    void *allocArg;
    int failed;
    MslStage stage;
    MslFunction *functions, *entry;
    const char *exportName, *sourceEntry;
    int uniformSlots;
    MslAddressSpace uniformAddressSpace;
    unsigned rowSetters;
    MslRecord *records;
    MslDecl *globals, *bindings;
    MslInterface *inputs, *outputs;
    MslDiagnostic diagnostic;
} MslModule;
void MslInitModule(MslModule *, MslStage, void *(*)(void *, size_t), void *);
void *MslAlloc(MslModule *, size_t);
const char *MslString(MslModule *, const char *);
int MslFail(MslModule *, int, const SourceLoc *, const char *);
int MslVerifyModule(const MslModule *, MslDiagnostic *);
int MslWriteModule(FILE *, const MslModule *);
int MslTypeSlots(MslType);
#endif
