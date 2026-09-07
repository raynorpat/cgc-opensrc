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
// glsl_lower_internal.h
//

#ifndef __GLSL_LOWER_INTERNAL_H
#define __GLSL_LOWER_INTERNAL_H

#include "slglobals.h"
#include "glsl_hal.h"
#include "cg_stdlib.h"
#include "cg_ir.h"

#define GLSL_MATRIX_MAX_ARGUMENTS 16

typedef struct GlslMatrixHelper_Rec {
    struct GlslMatrixHelper_Rec *next;
    GlslFunction *function;
    GlslType result;
    GlslType parameters[GLSL_MATRIX_MAX_ARGUMENTS];
    int parameterCount;
} GlslMatrixHelper;

typedef enum GlslMatrixSelectorHelperKind_Enum {
    GLSL_MATRIX_SELECTOR_GET,
    GLSL_MATRIX_SELECTOR_SET
} GlslMatrixSelectorHelperKind;

typedef struct GlslMatrixSelectorHelper_Rec {
    struct GlslMatrixSelectorHelper_Rec *next;
    GlslFunction *function;
    GlslMatrixSelectorHelperKind kind;
    GlslType matrixType;
    GlslType valueType;
    int count;
    int mask;
} GlslMatrixSelectorHelper;

typedef struct GlslInterfaceSource_Rec {
    struct GlslInterfaceSource_Rec *next;
    const Symbol *source;
    const char *interfaceKey;
    const char *reservedName;
    int isOutput;
} GlslInterfaceSource;

typedef struct GlslGeometryFlat_Rec {
    struct GlslGeometryFlat_Rec *next;
    int semantic;
    GlslDecl *target;
    GlslDecl *shadow;
    GlslDecl *defined;
} GlslGeometryFlat;

typedef struct GlslGeometryInputBinding_Rec {
    struct GlslGeometryInputBinding_Rec *next;
    const Symbol *source;
    GlslDecl *declaration;
} GlslGeometryInputBinding;

typedef struct GlslGeometryOutputBinding_Rec {
    struct GlslGeometryOutputBinding_Rec *next;
    const char *interfaceKey;
    GlslDecl *declaration;
} GlslGeometryOutputBinding;

typedef struct GlslLowerContext_Rec {
    GlslModule *module;
    const GlslProfileDesc *profile;
    /* Legacy -version 1.1 tree path only; the Cg IR lowering never
     * reads frontend scopes (see GlslLowerCgIR below). */
    Scope *scope;
    /* Verified Cg IR module consumed by the Cg 2.0 lowering path. */
    const CgIRModule *source;
    const CgIRFunction *entry;
    GlslFunction *function;
    GlslMatrixHelper *matrixHelpers;
    GlslMatrixHelper *lastMatrixHelper;
    GlslMatrixSelectorHelper *selectorHelpers;
    GlslMatrixSelectorHelper *lastSelectorHelper;
    GlslInterfaceSource *interfaceSources;
    GlslGeometryFlat *geometryFlat;
    GlslGeometryFlat *lastGeometryFlat;
    GlslGeometryInputBinding *geometryInputs;
    GlslGeometryOutputBinding *geometryOutputs;
    SourceLoc statementLoc;
    int loopDepth;
} GlslLowerContext;

void GlslSetLoc(GlslLoc *target, const SourceLoc *source);

const char *GlslAllocateSymbolNameForSource(
    GlslLowerContext *context, const void *identity, const char *source,
    const SourceLoc *loc);

const char *GlslAllocateNameForSource(GlslLowerContext *context,
    const char *source, const SourceLoc *loc);

const char *GlslAllocateDistinctNameForSource(
    GlslLowerContext *context, const char *source, const SourceLoc *loc);

const char *GlslAllocateScopedSymbolNameForSource(
    GlslLowerContext *context, const void *nameSpace, const void *identity,
    const char *source, const SourceLoc *loc);

int GlslLowerError(GlslLowerContext *context);

void GlslRecordFailure(GlslLowerContext *context, const char *reason);

void GlslRecordFailureKind(GlslLowerContext *context,
                                  GlslErrorKind kind,
                                  const char *reason);

void GlslRecordFailureKindAt(GlslLowerContext *context,
                                    GlslErrorKind kind,
                                    const char *reason,
                                    const SourceLoc *loc);

const char *GlslUnsupportedExprReason(const expr *source);

char *GlslCopyText(GlslModule *module, const char *text);

int GlslTypesEqual(const GlslType *left, const GlslType *right);

int GlslIsSamplerType(const GlslType *type);

GlslDecl *GlslFindDecl(GlslLowerContext *context,
                              const void *identity);

GlslFunction *GlslFindFunction(GlslModule *module,
                                      const void *identity);

int GlslLowerType(GlslLowerContext *context, Type *source,
                         GlslType *target, const SourceLoc *loc);

void GlslInsertDecl(GlslDecl **list, GlslDecl *decl);

int GlslEnsureTypeAt(GlslLowerContext *context, Type *type,
                            const SourceLoc *loc);

int GlslEnsureSymbolTypes(GlslLowerContext *context, Symbol *symbol);

int GlslEnsureParameterTypes(GlslLowerContext *context,
                                    Symbol *formal);

int GlslSortStructs(GlslLowerContext *context);

int GlslCollectParameters(GlslLowerContext *context, Symbol *formal,
                                 int entry);

GlslBinding *GlslFindUniformBinding(GlslModule *module,
                                           const Symbol *symbol);

int GlslCollectUniforms(GlslLowerContext *context, Symbol *program);

int GlslValidateUniformLimit(GlslLowerContext *context);

int GlslValidateInterfaceLimits(GlslLowerContext *context);

int GlslAllocateTextureUnits(GlslLowerContext *context);

int GlslFiniteDefaultFloat(float value);

int GlslCollectDefaults(GlslLowerContext *context);

int GlslCollectLocals(GlslLowerContext *context, Symbol *symbol,
                             int entry);

void GlslInsertBinding(GlslBinding **list, GlslBinding *binding);

GlslDecl *GlslLowerInterface(GlslLowerContext *context,
                                    Symbol *member);

int GlslCollectCallsInStatements(GlslLowerContext *context,
                                        stmt *source);

int GlslAssignHelperNames(GlslLowerContext *context);

void GlslMarkForwardCalls(GlslLowerContext *context);

void GlslAppendExpr(GlslExpr **list, GlslExpr *expression);

GlslExpr *GlslLowerExprChain(GlslLowerContext *context, expr *source,
                                    opcode listOp);

GlslExpr *GlslNewLiteral(GlslLowerContext *context, GlslBase base,
    int intValue, float floatValue);

GlslExpr *GlslNewSwizzle(GlslLowerContext *context, GlslExpr *object,
    const GlslType *type, const char *mask);

int GlslMatrixSelectorCount(const expr *source);

GlslExpr *GlslMatrixSelectorComponent(GlslLowerContext *context,
    GlslExpr *matrix, const expr *selectorSource, int component);

GlslExpr *GlslLowerMatrixSwizzle(GlslLowerContext *context,
    expr *source, const GlslType *type);

int GlslMatrixNumericParameterType(const GlslType *type);

GlslMatrixSelectorHelper *GlslGetMatrixSelectorHelper(
    GlslLowerContext *context, GlslMatrixSelectorHelperKind kind,
    const GlslType *matrixType, const GlslType *valueType, int count,
    int mask);

GlslExpr *GlslLowerImpureMatrixConstructor(
    GlslLowerContext *context, GlslExpr *arguments, const GlslType *type);

GlslExpr *GlslLowerMatrixConstructor(GlslLowerContext *context,
    expr *source, const GlslType *type);

int GlslValidateTextureCall(GlslLowerContext *context,
    GlslBuiltin builtin, const GlslType *result, GlslExpr *arguments);

GlslBuiltin GlslIntrinsicBuiltin(CgIntrinsic intrinsic);

GlslExpr *GlslLowerExpr(GlslLowerContext *context, expr *source);

int GlslLowerStatementList(GlslLowerContext *context, stmt *source,
                                  GlslStmt **list);

int GlslLowerHelper(GlslLowerContext *context,
                           GlslFunction *function);

void GlslPrependMatrixHelpers(GlslLowerContext *context);

void GlslPrependMatrixSelectorHelpers(GlslLowerContext *context);

int GlslValidateEntryInterfaces(GlslLowerContext *context,
                                       Symbol *program);

int GlslIRInstallGeometryInfo(GlslLowerContext *context);

GlslInterpolation GlslInterpolationForType(const GlslType *type);

int GlslIRRegisterGeometryInput(GlslLowerContext *context,
    const CgIRDecl *param);

GlslStmt *GlslIRGeometryAssignments(
    GlslLowerContext *context, const CgIRGeometryValue *value);

GlslStmt *GlslIRGeometryFlatAssignments(
    GlslLowerContext *context, const CgIRGeometryValue *value);

GlslFlatReplay *GlslIRGeometryFlatReplay(
    GlslLowerContext *context);

int GlslLowerTypeAuto(GlslLowerContext *context, Type *source,
                             GlslType *target, const SourceLoc *loc);

int GlslEnsureTypeAuto(GlslLowerContext *context, Type *type,
                              const SourceLoc *loc);

int GlslIRScalarBase(GlslLowerContext *context, CgScalarKind kind,
                            GlslBase *base, const SourceLoc *loc);

int GlslIRIsSamplerValue(const Type *type);

int GlslIRType(GlslLowerContext *context, Type *source,
                      GlslType *target, const SourceLoc *loc);

int GlslIREnsureTypeAt(GlslLowerContext *context, Type *type,
                              const SourceLoc *loc);

int GlslIRSamplerPlacementCheck(GlslLowerContext *context,
                                       const CgIRDecl *decl);

const CgIRFunction *GlslIRFindIRFunction(const CgIRModule *source,
                                                const Symbol *symbol);

int GlslIRCollectCallsInStmt(GlslLowerContext *context,
                                    const CgIRStmt *stmt);

int GlslIRCollectGeometryFlatState(GlslLowerContext *context);

void GlslIRMarkForwardCallsInStmt(GlslLowerContext *context,
                                         GlslFunction *caller,
                                         const CgIRStmt *stmt);

int GlslIRValidateEntryInterfaces(GlslLowerContext *context,
                                         const CgIRFunction *entry);

int GlslIRNeedsMaterialization(const CgIRExpr *expr);

GlslExpr *GlslCloneExpr(GlslModule *module, const GlslExpr *expr);

GlslDecl *GlslIRAddLocal(GlslLowerContext *context, Symbol *symbol,
                                Type *type, const SourceLoc *loc);

int GlslIRLowerAggregateAssign(GlslLowerContext *context,
                                      const CgIRExpr *assign,
                                      GlslStmt **list);

int GlslIRSharedSelection(const CgIRExpr *expr,
                                 const CgIRExpr **objectOut,
                                 int *countOut, int *maskOut);

const CgIRDecl *GlslIRGeometryEntryParameter(
    const GlslLowerContext *context, const Symbol *symbol);

GlslExpr *GlslIRGeometryBuiltinElement(
    GlslLowerContext *context, const CgIRDecl *param,
    GlslExpr *indexExpr, const GlslType *resultType,
    const SourceLoc *loc);

GlslExpr *GlslIRGeometryBuiltinArray(
    GlslLowerContext *context, const CgIRExpr *expr,
    const GlslType *type);

GlslExpr *GlslIRLowerExpr(GlslLowerContext *context,
                                 const CgIRExpr *expr);

const char *GlslIRDeclNameText(const CgIRDecl *decl);

GlslExpr *GlslIRMatrixElement(GlslLowerContext *context,
                                     GlslExpr *matrix, int row,
                                     int column);

int GlslIRTryGroupWrite(GlslLowerContext *context,
                               const CgIRStmt *head,
                               const CgIRStmt **nextOut, GlslStmt **list);

int GlslIRLocalDeclaration(GlslLowerContext *context,
                                  const CgIRStmt *stmt,
                                  const CgIRStmt **nextOut, GlslStmt **out);

int GlslIRBlockBody(GlslLowerContext *context,
                           const CgIRStmt *stmt, GlslStmt **out);

int GlslIRLowerFunctionBody(GlslLowerContext *context,
                                   const CgIRFunction *irFunction);

int GlslIRCollectUniforms(GlslLowerContext *context,
                                 const CgIRFunction *entry);

int GlslIREnsureEntryLocals(GlslLowerContext *context,
                                   const CgIRFunction *entry);

#endif // __GLSL_LOWER_INTERNAL_H
