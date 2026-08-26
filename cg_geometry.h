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

/*
 * cg_geometry.h
 */

#if !defined(__CG_GEOMETRY_H)
#define __CG_GEOMETRY_H 1

#include <stddef.h>

typedef struct Symbol_Rec Symbol;
typedef union Type_Rec Type;
typedef union expr_rec expr;
typedef union stmt_rec stmt;
typedef struct CgReachGraph_Rec CgReachGraph;

typedef enum CgIRStage_Rec {
    CGIR_STAGE_UNKNOWN = 0,
    CGIR_STAGE_NEUTRAL,
    CGIR_STAGE_VERTEX,
    CGIR_STAGE_GEOMETRY,
    CGIR_STAGE_FRAGMENT
} CgIRStage;

typedef enum CgGeometryInput_Rec {
    CG_GEOMETRY_INPUT_UNKNOWN = 0,
    CG_GEOMETRY_INPUT_POINT,
    CG_GEOMETRY_INPUT_LINE,
    CG_GEOMETRY_INPUT_LINE_ADJACENCY,
    CG_GEOMETRY_INPUT_TRIANGLE,
    CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY
} CgGeometryInput;

typedef enum CgGeometryOutput_Rec {
    CG_GEOMETRY_OUTPUT_UNKNOWN = 0,
    CG_GEOMETRY_OUTPUT_POINTS,
    CG_GEOMETRY_OUTPUT_LINE_STRIP,
    CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP
} CgGeometryOutput;

typedef struct CgProfileOption_Rec {
    struct CgProfileOption_Rec *next;
    const char *text;
    int ordinal;
} CgProfileOption;

typedef struct CgGeometryModifiers_Rec {
    CgGeometryInput input;
    CgGeometryOutput output;
    SourceLoc inputLoc;
    SourceLoc outputLoc;
} CgGeometryModifiers;

typedef struct CgGeometryOptions_Rec {
    CgGeometryInput input;
    CgGeometryOutput output;
    unsigned int maxOutputVertices;
    int hasMaxOutputVertices;
    int inputOrdinal;
    int outputOrdinal;
    int verticesOrdinal;
} CgGeometryOptions;

typedef struct CgGeometryConfig_Rec {
    CgIRStage stage;
    CgGeometryInput inputTopology;
    CgGeometryOutput outputTopology;
    unsigned int inputVertexCount;
    unsigned int maxOutputVertices;
    int hasMaxOutputVertices;
    SourceLoc inputLoc;
    SourceLoc outputLoc;
    SourceLoc maxVerticesLoc;
} CgGeometryConfig;

typedef enum CgGeometryDiagnosticReason_Rec {
    CG_GEOMETRY_DIAGNOSTIC_NONE = 0,
    CG_GEOMETRY_DIAGNOSTIC_UNKNOWN_OPTION,
    CG_GEOMETRY_DIAGNOSTIC_MALFORMED_VERTICES,
    CG_GEOMETRY_DIAGNOSTIC_REPEATED_INPUT,
    CG_GEOMETRY_DIAGNOSTIC_REPEATED_OUTPUT,
    CG_GEOMETRY_DIAGNOSTIC_CONFLICTING_INPUT,
    CG_GEOMETRY_DIAGNOSTIC_CONFLICTING_OUTPUT,
    CG_GEOMETRY_DIAGNOSTIC_DUPLICATE_VERTICES,
    CG_GEOMETRY_DIAGNOSTIC_SOURCE_OPTION_CONFLICT,
    CG_GEOMETRY_DIAGNOSTIC_MISSING_INPUT,
    CG_GEOMETRY_DIAGNOSTIC_STAGE_CONFLICT,
    CG_GEOMETRY_DIAGNOSTIC_OPERATION_ARITY,
    CG_GEOMETRY_DIAGNOSTIC_OPERATION_CONTEXT,
    CG_GEOMETRY_DIAGNOSTIC_OUTPUT_SEMANTIC,
    CG_GEOMETRY_DIAGNOSTIC_DUPLICATE_SEMANTIC,
    CG_GEOMETRY_DIAGNOSTIC_SEMANTIC,
    CG_GEOMETRY_DIAGNOSTIC_FLAT_POSITION,
    CG_GEOMETRY_DIAGNOSTIC_ENTRY_CALL,
    CG_GEOMETRY_DIAGNOSTIC_REACHABLE_STAGE,
    CG_GEOMETRY_DIAGNOSTIC_ALLOCATION,
    CG_GEOMETRY_DIAGNOSTIC_ATTRIB_PLACEMENT,
    CG_GEOMETRY_DIAGNOSTIC_ATTRIB_ELEMENT,
    CG_GEOMETRY_DIAGNOSTIC_ATTRIB_STAGE,
    CG_GEOMETRY_DIAGNOSTIC_VALUE_TYPE
} CgGeometryDiagnosticReason;

/*
 * Where one declaration sits when its type is an AttribArray.  The
 * two input uses name formals of a geometry entry and of a helper it
 * reaches; every other use is a prohibited placement.
 */

typedef enum CgGeometryDeclarationUse_Rec {
    CG_GEOMETRY_DECL_ENTRY_INPUT,
    CG_GEOMETRY_DECL_HELPER_INPUT,
    CG_GEOMETRY_DECL_GLOBAL,
    CG_GEOMETRY_DECL_UNIFORM,
    CG_GEOMETRY_DECL_OUTPUT,
    CG_GEOMETRY_DECL_RETURN,
    CG_GEOMETRY_DECL_MEMBER,
    CG_GEOMETRY_DECL_LOCAL
} CgGeometryDeclarationUse;

typedef struct CgGeometryDiagnostic_Rec {
    CgGeometryDiagnosticReason reason;
    SourceLoc loc;
    int optionOrdinal;
    const char *optionText;
} CgGeometryDiagnostic;

/*
 * Geometry program records.  One CgGeometryProgram accumulates the
 * selected entry and its resolution state; every later extension adds
 * fields here instead of parallel side tables.  Allocation goes
 * through the caller-supplied allocator pair so resolution state can
 * live in a caller-chosen arena.
 */

typedef void *(*CgGeometryAllocFn)(void *arg, size_t size);

typedef struct CgGeometryTypeView_Rec {
    struct CgGeometryTypeView_Rec *next;
    const Type *sourceType;
    const Type *resolvedType;
} CgGeometryTypeView;

/*
 * The three geometry operations, identified by intrinsic identity:
 * emitVertex writes output values, flatAttrib marks constant outputs,
 * and restartStrip ends one instance with no values at all.
 */

typedef enum CgGeometryOperationKind_Rec {
    CG_GEOMETRY_OPERATION_EMIT,
    CG_GEOMETRY_OPERATION_FLAT,
    CG_GEOMETRY_OPERATION_RESTART
} CgGeometryOperationKind;

/*
 * One resolved output value of an operation: "canonicalSemantic" is
 * the equality atom (root case-folded to upper case, numeric suffix
 * canonicalized), "sourceSemantic" preserves the binding's source
 * spelling, and "loc" anchors the semantic at its origin -- an inline
 * annotation site or the declaration that carries the semantic.
 */

typedef struct CgGeometryValue_Rec {
    struct CgGeometryValue_Rec *next;
    int canonicalSemantic;
    int sourceSemantic;
    Type *type;
    expr *value;
    SourceLoc loc;
} CgGeometryValue;

/*
 * One classified geometry operation statement.  Records append in
 * source order to program->operations; "statement" back-links the
 * expression statement so later passes can find a record from the
 * tree and vice versa.
 */

typedef struct CgGeometryOperation_Rec {
    struct CgGeometryOperation_Rec *next;
    CgGeometryOperationKind kind;
    stmt *statement;
    CgGeometryValue *values;
    SourceLoc loc;
} CgGeometryOperation;

typedef struct CgGeometryProgram_Rec {
    CgGeometryAllocFn alloc;
    void *allocArg;
    CgGeometryConfig config;
    const Symbol *entry;
    CgGeometryTypeView *typeViews;
    CgGeometryOperation *operations;
    int failed;
} CgGeometryProgram;

void CgGeometryInitProgram(CgGeometryProgram *program,
                           CgGeometryAllocFn alloc, void *allocArg);

/*
 * Lookup helpers over a program's accumulated resolution state: the
 * operation record classified for "statement", and the resolved view
 * recorded for "sourceType".  Both answer NULL when nothing matches.
 */

CgGeometryOperation *CgGeometryFindOperation(
                           const CgGeometryProgram *program,
                           const stmt *statement);
Type *CgGeometryFindResolvedType(const CgGeometryProgram *program,
                                 const Type *sourceType);

/*
 * Resolve one operation's argument bundle into an ordered value list.
 * Arguments flatten recursively in declaration order; each leaf picks
 * its semantic by this exact order: inline annotation, directly
 * referenced declaration, selected aggregate member, then direct
 * indexing of an attribute-array parameter.  Arithmetic and
 * constructors never inherit.  Canonical root case plus numeric
 * suffix decide equality while the source spelling atom is preserved.
 * Failures name structured reasons: unresolved leaf, empty bundle,
 * illegal leaf value type, duplicate canonical semantic, and, for
 * flatAttrib only, a canonical POSITION.  All records allocate
 * through program->alloc; allocation refusal marks program->failed.
 *
 * Returns: TRUE if O.K.
 *
 */

int CgGeometryResolveBundle(CgGeometryProgram *program,
                            CgGeometryOperationKind kind,
                            expr *arguments,
                            CgGeometryValue **values,
                            CgGeometryDiagnostic *diagnostic);

/*
 * Classify one statement of a typed function body.  A complete
 * expression statement whose call selects a geometry-special
 * intrinsic identity validates arity (emitVertex/flatAttrib need at
 * least one argument, restartStrip none) and -- given a non-NULL
 * initialized program -- resolves its bundle and appends the
 * operation record.  A special call nested anywhere else (assignment,
 * constructor, conditional, return, argument, or arithmetic node)
 * fails with the context reason; conditions and return expressions
 * are scanned directly.  With program == NULL only syntax-level
 * checks run and no records are built, so global classification can
 * diagnose malformed dead code consistently before any selected-
 * program analysis exists.  Bundle failures that carry no better
 * anchor (a plain argument whose wrapper call resolution removed)
 * are reported at the statement's own location.
 *
 * Returns: TRUE if O.K.
 *
 */

int CgGeometryClassifyOperationStatement(CgGeometryProgram *program,
                                         stmt *statement,
                                         CgGeometryDiagnostic *diagnostic);

/*
 * The shared AttribArray<T> element rule: numeric scalars, vectors,
 * matrices, ordinary arrays, and structs are legal elements; void,
 * undefined recovery types, functions, samplers, interfaces,
 * connectors, and everything else are not.  Placement validation on
 * top of this rule arrives with declaration checking.
 */

int CgGeometryAcceptsAttribArrayElement(const Type *type);

/*
 * Declaration placement of one AttribArray symbol.  "program" is the
 * selected geometry program when one has been resolved and NULL while
 * only source-level facts are known.  Entry inputs are admitted on
 * the strength of their own declaration; helper inputs additionally
 * require a resolved geometry program, since reachability is proven
 * only by selected-program analysis.  Every other use fails with a
 * structured placement reason, an illegal element outranks any
 * placement rule, and symbols whose type is not an attribute array
 * always validate.  On failure "diagnostic" names the reason at the
 * symbol's location.
 */

int CgGeometryValidateAttribArrayDeclaration(
    const CgGeometryProgram *program, const Symbol *symbol,
    CgGeometryDeclarationUse use, CgGeometryDiagnostic *diagnostic);

/*
 * Fold every attribute-array .length query in the selected program's
 * reachable function bodies into an int constant equal to
 * config.inputVertexCount, recording one resolved type view
 * (source Type *, resolved Type *) per distinct source array in
 * program->typeViews.  The canonical source types are never mutated:
 * only statement-tree nodes change.  Fails with a structured stage
 * reason when no resolved geometry program is supplied -- such uses
 * are rejected outright instead of folding to zero -- and with the
 * allocation reason when the program's allocator refuses storage.
 */

int CgGeometryResolveLengths(CgGeometryProgram *program,
                             const CgReachGraph *reach,
                             CgGeometryDiagnostic *diagnostic);

void CgGeometryInitModifiers(CgGeometryModifiers *modifiers);
int CgGeometryApplyInputModifier(CgGeometryModifiers *modifiers,
                                 CgGeometryInput input,
                                 const SourceLoc *loc,
                                 CgGeometryDiagnostic *diagnostic);
int CgGeometryApplyOutputModifier(CgGeometryModifiers *modifiers,
                                  CgGeometryOutput output,
                                  const SourceLoc *loc,
                                  CgGeometryDiagnostic *diagnostic);
void CgGeometryInitOptions(CgGeometryOptions *options);
int CgGeometryParseOptions(const CgProfileOption *first,
                           CgGeometryOptions *options,
                           CgGeometryDiagnostic *diagnostic);
int CgGeometryResolveConfig(const CgGeometryModifiers *source,
                            const CgGeometryOptions *options,
                            CgIRStage profileStage,
                            CgGeometryConfig *config,
                            CgGeometryDiagnostic *diagnostic);
unsigned int CgGeometryInputVertexCount(CgGeometryInput input);
CgGeometryOutput CgGeometryDefaultOutput(CgGeometryInput input);
const char *CgGeometryInputName(CgGeometryInput input);
const char *CgGeometryOutputName(CgGeometryOutput output);

#endif
