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
 * cg_geometry.c
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slglobals.h"
#include "cg_geometry.h"
#include "cg_reach.h"

#define CG_GEOMETRY_VERTICES_PREFIX "Vertices="

/*
 * Fixed spellings for the "-po" topology options.  Matching is exact:
 * no case folding, no partial matches, no whitespace around values.
 */

typedef struct CgGeometryInputSpelling_Rec {
    const char *text;
    CgGeometryInput input;
} CgGeometryInputSpelling;

typedef struct CgGeometryOutputSpelling_Rec {
    const char *text;
    CgGeometryOutput output;
} CgGeometryOutputSpelling;

static const CgGeometryInputSpelling inputSpellings[] = {
    { "POINT",        CG_GEOMETRY_INPUT_POINT },
    { "LINE",         CG_GEOMETRY_INPUT_LINE },
    { "LINE_ADJ",     CG_GEOMETRY_INPUT_LINE_ADJACENCY },
    { "TRIANGLE",     CG_GEOMETRY_INPUT_TRIANGLE },
    { "TRIANGLE_ADJ", CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY }
};

static const CgGeometryOutputSpelling outputSpellings[] = {
    { "POINT_OUT",    CG_GEOMETRY_OUTPUT_POINTS },
    { "LINE_OUT",     CG_GEOMETRY_OUTPUT_LINE_STRIP },
    { "TRIANGLE_OUT", CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP }
};

static const char *inputNames[] = {
    "unknown",
    "POINT",
    "LINE",
    "LINE_ADJACENCY",
    "TRIANGLE",
    "TRIANGLE_ADJACENCY"
};

static const char *outputNames[] = {
    "unknown",
    "POINTS",
    "LINE_STRIP",
    "TRIANGLE_STRIP"
};

/*
 * ClearDiagnostic() - Reset a diagnostic to its stable none state so
 *         callers never read stale fields after a success.
 */

static void ClearDiagnostic(CgGeometryDiagnostic *diagnostic)
{
    diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_NONE;
    diagnostic->loc.file = 0;
    diagnostic->loc.line = 0;
    diagnostic->optionOrdinal = 0;
    diagnostic->optionText = NULL;
} // ClearDiagnostic

/*
 * LookupInputSpelling() - Answer 1 with "answer" set when "text" is
 *         exactly one of the input topology spellings.
 */

static int LookupInputSpelling(const char *text, CgGeometryInput *answer)
{
    int ii;

    for (ii = 0;
         ii < (int)(sizeof(inputSpellings) / sizeof(inputSpellings[0]));
         ii++) {
        if (!strcmp(text, inputSpellings[ii].text)) {
            *answer = inputSpellings[ii].input;
            return 1;
        }
    }
    return 0;
} // LookupInputSpelling

/*
 * LookupOutputSpelling() - Answer 1 with "answer" set when "text" is
 *         exactly one of the output topology spellings.
 */

static int LookupOutputSpelling(const char *text, CgGeometryOutput *answer)
{
    int ii;

    for (ii = 0;
         ii < (int)(sizeof(outputSpellings) / sizeof(outputSpellings[0]));
         ii++) {
        if (!strcmp(text, outputSpellings[ii].text)) {
            *answer = outputSpellings[ii].output;
            return 1;
        }
    }
    return 0;
} // LookupOutputSpelling

/*
 * ParseVertexCount() - Decode a strict decimal "Vertices" value: at
 *         least one digit, digits only, nonzero, and no overflow past
 *         unsigned int.  The check is digit-by-digit; strtol is never
 *         used, so signs, whitespace, suffixes, and trailing text all
 *         fail here.
 */

static unsigned int ParseVertexCount(const char *text, int *valid)
{
    unsigned int limit = UINT_MAX;
    unsigned int value = 0;

    *valid = 1;
    if (*text == '\0') {
        *valid = 0;
        return 0;
    }
    while (*text != '\0') {
        unsigned int digit;

        if (*text < '0' || *text > '9') {
            *valid = 0;
            return 0;
        }
        digit = (unsigned int)(*text - '0');
        if (value > (limit - digit) / 10) {
            /* Decimal overflow: another digit would wrap. */
            *valid = 0;
            return 0;
        }
        value = value * 10 + digit;
        text++;
    }
    if (value == 0) {
        /* A zero maximum vertex count is meaningless. */
        *valid = 0;
        return 0;
    }
    return value;
} // ParseVertexCount

void CgGeometryInitModifiers(CgGeometryModifiers *modifiers)
{
    modifiers->input = CG_GEOMETRY_INPUT_UNKNOWN;
    modifiers->output = CG_GEOMETRY_OUTPUT_UNKNOWN;
    modifiers->inputLoc.file = 0;
    modifiers->inputLoc.line = 0;
    modifiers->outputLoc.file = 0;
    modifiers->outputLoc.line = 0;
} // CgGeometryInitModifiers

/*
 * CgGeometryApplyInputModifier() - Record one source-level input
 *         topology modifier.  A fresh value is stored with its location;
 *         an occupied slot always fails so callers can report at the
 *         first location: an equal value as a repeated modifier and a
 *         different value as a conflicting one.  Failed applications
 *         never disturb the recorded value or location.
 */

int CgGeometryApplyInputModifier(CgGeometryModifiers *modifiers,
                                 CgGeometryInput input,
                                 const SourceLoc *loc,
                                 CgGeometryDiagnostic *diagnostic)
{
    ClearDiagnostic(diagnostic);
    if (modifiers->input != CG_GEOMETRY_INPUT_UNKNOWN) {
        diagnostic->loc = modifiers->inputLoc;
        if (modifiers->input == input) {
            diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_REPEATED_INPUT;
        } else {
            diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_CONFLICTING_INPUT;
        }
        return 0;
    }
    modifiers->input = input;
    modifiers->inputLoc = *loc;
    return 1;
} // CgGeometryApplyInputModifier

/*
 * CgGeometryApplyOutputModifier() - Record one source-level output
 *         topology modifier with the same repeat/conflict discipline as
 *         the input slot; the two slots stay independent.
 */

int CgGeometryApplyOutputModifier(CgGeometryModifiers *modifiers,
                                  CgGeometryOutput output,
                                  const SourceLoc *loc,
                                  CgGeometryDiagnostic *diagnostic)
{
    ClearDiagnostic(diagnostic);
    if (modifiers->output != CG_GEOMETRY_OUTPUT_UNKNOWN) {
        diagnostic->loc = modifiers->outputLoc;
        if (modifiers->output == output) {
            diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_REPEATED_OUTPUT;
        } else {
            diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_CONFLICTING_OUTPUT;
        }
        return 0;
    }
    modifiers->output = output;
    modifiers->outputLoc = *loc;
    return 1;
} // CgGeometryApplyOutputModifier

void CgGeometryInitOptions(CgGeometryOptions *options)
{
    options->input = CG_GEOMETRY_INPUT_UNKNOWN;
    options->output = CG_GEOMETRY_OUTPUT_UNKNOWN;
    options->maxOutputVertices = 0;
    options->hasMaxOutputVertices = 0;
    options->inputOrdinal = 0;
    options->outputOrdinal = 0;
    options->verticesOrdinal = 0;
} // CgGeometryInitOptions

/*
 * CgGeometryParseOptions() - Fold one raw "-po" option chain into
 *         "options".  Equal repeats of any option are accepted;
 *         conflicting topologies and unequal Vertices values are
 *         rejected.  Every rejection names the offending node's ordinal
 *         so callers can attribute the diagnostic without parsing
 *         option text themselves.
 */

int CgGeometryParseOptions(const CgProfileOption *first,
                           CgGeometryOptions *options,
                           CgGeometryDiagnostic *diagnostic)
{
    const size_t verticesPrefixLength =
        sizeof(CG_GEOMETRY_VERTICES_PREFIX) - 1;
    const CgProfileOption *option;
    CgGeometryInput input;
    CgGeometryOutput output;
    const char *digits;
    unsigned int value;
    int valid;

    ClearDiagnostic(diagnostic);
    for (option = first; option != NULL; option = option->next) {
        if (LookupInputSpelling(option->text, &input)) {
            if (options->input != CG_GEOMETRY_INPUT_UNKNOWN &&
                options->input != input) {
                diagnostic->reason =
                    CG_GEOMETRY_DIAGNOSTIC_CONFLICTING_INPUT;
                diagnostic->optionOrdinal = option->ordinal;
                diagnostic->optionText = option->text;
                return 0;
            }
            if (options->input == CG_GEOMETRY_INPUT_UNKNOWN) {
                options->input = input;
                options->inputOrdinal = option->ordinal;
            }
        } else if (LookupOutputSpelling(option->text, &output)) {
            if (options->output != CG_GEOMETRY_OUTPUT_UNKNOWN &&
                options->output != output) {
                diagnostic->reason =
                    CG_GEOMETRY_DIAGNOSTIC_CONFLICTING_OUTPUT;
                diagnostic->optionOrdinal = option->ordinal;
                diagnostic->optionText = option->text;
                return 0;
            }
            if (options->output == CG_GEOMETRY_OUTPUT_UNKNOWN) {
                options->output = output;
                options->outputOrdinal = option->ordinal;
            }
        } else if (!strncmp(option->text, CG_GEOMETRY_VERTICES_PREFIX,
                            verticesPrefixLength)) {
            digits = option->text + verticesPrefixLength;
            value = ParseVertexCount(digits, &valid);
            if (!valid) {
                diagnostic->reason =
                    CG_GEOMETRY_DIAGNOSTIC_MALFORMED_VERTICES;
                diagnostic->optionOrdinal = option->ordinal;
                diagnostic->optionText = option->text;
                return 0;
            }
            if (options->hasMaxOutputVertices &&
                options->maxOutputVertices != value) {
                diagnostic->reason =
                    CG_GEOMETRY_DIAGNOSTIC_DUPLICATE_VERTICES;
                diagnostic->optionOrdinal = option->ordinal;
                diagnostic->optionText = option->text;
                return 0;
            }
            if (!options->hasMaxOutputVertices) {
                options->maxOutputVertices = value;
                options->hasMaxOutputVertices = 1;
                options->verticesOrdinal = option->ordinal;
            }
        } else {
            /* Includes GLSLVersion=150: there is deliberately no hidden
             * profile-version selector behind "-po". */
            diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_UNKNOWN_OPTION;
            diagnostic->optionOrdinal = option->ordinal;
            diagnostic->optionText = option->text;
            return 0;
        }
    }
    return 1;
} // CgGeometryParseOptions

/*
 * CgGeometryResolveConfig() - Merge source-level geometry modifiers and
 *         parsed "-po" options into one resolved program configuration.
 *         Source values win over option values; contradictory pairs are
 *         rejected.  When nothing selects geometry, the profile's own
 *         stage is copied through unchanged.  A geometry program always
 *         has an explicit input topology and never an unknown one; its
 *         missing output topology and vertex count derive from the
 *         input.  An unknown maximum value is not judged here.
 */

int CgGeometryResolveConfig(const CgGeometryModifiers *source,
                            const CgGeometryOptions *options,
                            CgIRStage profileStage,
                            CgGeometryConfig *config,
                            CgGeometryDiagnostic *diagnostic)
{
    CgGeometryInput resolvedInput;
    CgGeometryOutput resolvedOutput;
    int isGeometry;

    ClearDiagnostic(diagnostic);
    resolvedInput = source->input != CG_GEOMETRY_INPUT_UNKNOWN ?
                    source->input : options->input;
    resolvedOutput = source->output != CG_GEOMETRY_OUTPUT_UNKNOWN ?
                     source->output : options->output;
    isGeometry = resolvedInput != CG_GEOMETRY_INPUT_UNKNOWN ||
                 resolvedOutput != CG_GEOMETRY_OUTPUT_UNKNOWN ||
                 options->hasMaxOutputVertices ||
                 profileStage == CGIR_STAGE_GEOMETRY;

    if (source->input != CG_GEOMETRY_INPUT_UNKNOWN &&
        options->input != CG_GEOMETRY_INPUT_UNKNOWN &&
        options->input != source->input) {
        diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_SOURCE_OPTION_CONFLICT;
        diagnostic->optionOrdinal = options->inputOrdinal;
        return 0;
    }
    if (source->output != CG_GEOMETRY_OUTPUT_UNKNOWN &&
        options->output != CG_GEOMETRY_OUTPUT_UNKNOWN &&
        options->output != source->output) {
        diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_SOURCE_OPTION_CONFLICT;
        diagnostic->optionOrdinal = options->outputOrdinal;
        return 0;
    }

    config->inputLoc = source->inputLoc;
    config->outputLoc = source->outputLoc;
    config->maxVerticesLoc.file = 0;
    config->maxVerticesLoc.line = 0;
    if (!isGeometry) {
        config->stage = profileStage;
        config->inputTopology = CG_GEOMETRY_INPUT_UNKNOWN;
        config->outputTopology = CG_GEOMETRY_OUTPUT_UNKNOWN;
        config->inputVertexCount = 0;
        config->maxOutputVertices = 0;
        config->hasMaxOutputVertices = 0;
        return 1;
    }

    if (resolvedInput == CG_GEOMETRY_INPUT_UNKNOWN) {
        diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_MISSING_INPUT;
        return 0;
    }
    config->stage = CGIR_STAGE_GEOMETRY;
    config->inputTopology = resolvedInput;
    config->outputTopology = resolvedOutput != CG_GEOMETRY_OUTPUT_UNKNOWN ?
                             resolvedOutput :
                             CgGeometryDefaultOutput(resolvedInput);
    config->inputVertexCount = CgGeometryInputVertexCount(resolvedInput);
    config->maxOutputVertices = options->maxOutputVertices;
    config->hasMaxOutputVertices = options->hasMaxOutputVertices;
    return 1;
} // CgGeometryResolveConfig

unsigned int CgGeometryInputVertexCount(CgGeometryInput input)
{
    switch (input) {
    case CG_GEOMETRY_INPUT_POINT:
        return 1;
    case CG_GEOMETRY_INPUT_LINE:
        return 2;
    case CG_GEOMETRY_INPUT_LINE_ADJACENCY:
        return 4;
    case CG_GEOMETRY_INPUT_TRIANGLE:
        return 3;
    case CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY:
        return 6;
    case CG_GEOMETRY_INPUT_UNKNOWN:
    default:
        return 0;
    }
} // CgGeometryInputVertexCount

CgGeometryOutput CgGeometryDefaultOutput(CgGeometryInput input)
{
    switch (input) {
    case CG_GEOMETRY_INPUT_POINT:
        return CG_GEOMETRY_OUTPUT_POINTS;
    case CG_GEOMETRY_INPUT_LINE:
    case CG_GEOMETRY_INPUT_LINE_ADJACENCY:
        return CG_GEOMETRY_OUTPUT_LINE_STRIP;
    case CG_GEOMETRY_INPUT_TRIANGLE:
    case CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY:
        return CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP;
    case CG_GEOMETRY_INPUT_UNKNOWN:
    default:
        return CG_GEOMETRY_OUTPUT_UNKNOWN;
    }
} // CgGeometryDefaultOutput

const char *CgGeometryInputName(CgGeometryInput input)
{
    if (input > CG_GEOMETRY_INPUT_UNKNOWN &&
        input <= CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY) {
        return inputNames[input];
    }
    return inputNames[CG_GEOMETRY_INPUT_UNKNOWN];
} // CgGeometryInputName

const char *CgGeometryOutputName(CgGeometryOutput output)
{
    if (output > CG_GEOMETRY_OUTPUT_UNKNOWN &&
        output <= CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP) {
        return outputNames[output];
    }
    return outputNames[CG_GEOMETRY_OUTPUT_UNKNOWN];
} // CgGeometryOutputName

///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Geometry programs and attrib arrays: ////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * CgGeometryInitProgram() - Prepare one program record: zero every
 *         field, then store the caller-supplied allocator pair so
 *         later resolution stages allocate their state through it.
 */

void CgGeometryInitProgram(CgGeometryProgram *program,
                           CgGeometryAllocFn alloc, void *allocArg)
{
    if (!program) {
        return;
    }
    memset(program, 0, sizeof(*program));
    program->alloc = alloc;
    program->allocArg = allocArg;
} // CgGeometryInitProgram

/*
 * CgGeometryAcceptsAttribArrayElement() - The shared AttribArray<T>
 *         element rule.  Property bits are read directly so the rule
 *         holds wherever geometry parsing runs, including unit tests
 *         with no symbol table; the category and base macros it uses
 *         are the single source of those encodings.  Undefined
 *         recovery types carry a scalar category, so the base check is
 *         what keeps poison out of attribute arrays.
 */

int CgGeometryAcceptsAttribArrayElement(const Type *type)
{
    int category;
    int base;

    if (!type) {
        return 0;
    }
    if (type->properties & TYPE_MISC_VOID) {
        return 0;
    }
    category = type->properties & TYPE_CATEGORY_MASK;
    base = type->properties & TYPE_BASE_MASK;
    switch (category) {
    case TYPE_CATEGORY_SCALAR:
        return base == TYPE_BASE_CFLOAT || base == TYPE_BASE_CINT ||
               base == TYPE_BASE_FLOAT || base == TYPE_BASE_INT ||
               base == TYPE_BASE_BOOLEAN;
    case TYPE_CATEGORY_ARRAY:
    case TYPE_CATEGORY_STRUCT:
        return 1;
    default:
        return 0;
    }
} // CgGeometryAcceptsAttribArrayElement

/*
 * CgGeometryValidateAttribArrayDeclaration() - Judge one declaration
 *         whose type is an AttribArray.  The element rule fires first
 *         because it is intrinsic to the type; then the use decides:
 *         entry inputs stand on their own source modifiers (selected-
 *         program analysis re-checks them against the resolved config
 *         later), helper inputs need a resolved geometry program, and
 *         every remaining placement is rejected outright.  Symbols of
 *         any other type always validate so callers can apply this to
 *         whole formal and member chains without pre-filtering.
 *
 * Returns: TRUE if O.K.
 *
 */

int CgGeometryValidateAttribArrayDeclaration(
    const CgGeometryProgram *program, const Symbol *symbol,
    CgGeometryDeclarationUse use, CgGeometryDiagnostic *diagnostic)
{
    const Type *type;

    ClearDiagnostic(diagnostic);
    if (!symbol) {
        return 1;
    }
    type = symbol->type;
    if (!CgIsAttribArray(type)) {
        return 1;
    }
    if (!CgGeometryAcceptsAttribArrayElement(
            CgAttribArrayElement(type))) {
        diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_ATTRIB_ELEMENT;
        diagnostic->loc = symbol->loc;
        return 0;
    }
    switch (use) {
    case CG_GEOMETRY_DECL_ENTRY_INPUT:
        if (program &&
            program->config.stage != CGIR_STAGE_GEOMETRY) {
            diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_ATTRIB_STAGE;
            diagnostic->loc = symbol->loc;
            return 0;
        }
        return 1;
    case CG_GEOMETRY_DECL_HELPER_INPUT:
        if (!program ||
            program->config.stage != CGIR_STAGE_GEOMETRY) {
            diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_ATTRIB_STAGE;
            diagnostic->loc = symbol->loc;
            return 0;
        }
        return 1;
    case CG_GEOMETRY_DECL_GLOBAL:
    case CG_GEOMETRY_DECL_UNIFORM:
    case CG_GEOMETRY_DECL_OUTPUT:
    case CG_GEOMETRY_DECL_RETURN:
    case CG_GEOMETRY_DECL_MEMBER:
    case CG_GEOMETRY_DECL_LOCAL:
        diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_ATTRIB_PLACEMENT;
        diagnostic->loc = symbol->loc;
        return 0;
    default:
        diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_ATTRIB_PLACEMENT;
        diagnostic->loc = symbol->loc;
        return 0;
    }
} // CgGeometryValidateAttribArrayDeclaration

/*
 * lNewResolvedLengthConstant() - The folded replacement for one
 *         attribute-array .length query: an int constant node of the
 *         selected topology's vertex count, allocated from the
 *         program's own arena.  Node shape mirrors the parser's
 *         numeric constants exactly.
 */

static expr *lNewResolvedLengthConstant(CgGeometryProgram *program,
                                        int *failed)
{
    constant *node;
    CgNumericValue value;

    node = (constant *) program->alloc(program->allocArg,
                                       sizeof(*node));
    if (!node) {
        *failed = 1;
        return NULL;
    }
    memset(node, 0, sizeof(*node));
    node->kind = CONST_N;
    node->op = ICONST_OP;
    node->type = GetStandardTypeKind(CG_SCALAR_CINT, 0, 0);
    node->subop = SUBOP__(CgScalarLegacyBase(CG_SCALAR_CINT));
    CgNumericSetSigned(&value, CG_SCALAR_CINT,
                       (CgInt64) program->config.inputVertexCount);
    node->val[0] = value;
    return (expr *) node;
} // lNewResolvedLengthConstant

/*
 * lFoldLengthExpr() - Replace every ARRAY_LENGTH_OP whose operand is
 *         an attribute array with the resolved int constant,
 *         recursing through the expression shapes statements carry.
 *         Ordinary arrays keep their parse-time handling untouched.
 */

static expr *lFoldLengthExpr(CgGeometryProgram *program, expr *fExpr,
                             int *failed)
{
    if (!fExpr || *failed) {
        return fExpr;
    }
    switch (fExpr->common.kind) {
    case UNARY_N:
        fExpr->un.arg = lFoldLengthExpr(program, fExpr->un.arg, failed);
        if (!*failed && fExpr->un.op == ARRAY_LENGTH_OP &&
            CgIsAttribArray(fExpr->un.arg ?
                            fExpr->un.arg->common.type : NULL))
        {
            return lNewResolvedLengthConstant(program, failed);
        }
        break;
    case BINARY_N:
        fExpr->bin.left = lFoldLengthExpr(program, fExpr->bin.left,
                                          failed);
        fExpr->bin.right = lFoldLengthExpr(program, fExpr->bin.right,
                                           failed);
        break;
    case TRINARY_N:
        fExpr->tri.arg1 = lFoldLengthExpr(program, fExpr->tri.arg1,
                                          failed);
        fExpr->tri.arg2 = lFoldLengthExpr(program, fExpr->tri.arg2,
                                          failed);
        fExpr->tri.arg3 = lFoldLengthExpr(program, fExpr->tri.arg3,
                                          failed);
        break;
    case SYMB_N:
    case CONST_N:
    default:
        break;
    }
    return fExpr;
} // lFoldLengthExpr

/*
 * lFoldLengthStmt() - Walk one statement chain, folding length
 *         queries inside every statement kind that carries
 *         expressions or nested bodies.
 */

static void lFoldLengthStmt(CgGeometryProgram *program, stmt *fStmt,
                            int *failed)
{
    for (; fStmt && !*failed; fStmt = fStmt->commonst.next) {
        switch (fStmt->commonst.kind) {
        case EXPR_STMT:
            fStmt->exprst.exp =
                lFoldLengthExpr(program, fStmt->exprst.exp, failed);
            break;
        case IF_STMT:
            fStmt->ifst.cond =
                lFoldLengthExpr(program, fStmt->ifst.cond, failed);
            lFoldLengthStmt(program, fStmt->ifst.thenstmt, failed);
            lFoldLengthStmt(program, fStmt->ifst.elsestmt, failed);
            break;
        case WHILE_STMT:
        case DO_STMT:
            fStmt->whilest.cond =
                lFoldLengthExpr(program, fStmt->whilest.cond, failed);
            lFoldLengthStmt(program, fStmt->whilest.body, failed);
            break;
        case FOR_STMT:
            lFoldLengthStmt(program, fStmt->forst.init, failed);
            fStmt->forst.cond =
                lFoldLengthExpr(program, fStmt->forst.cond, failed);
            lFoldLengthStmt(program, fStmt->forst.step, failed);
            lFoldLengthStmt(program, fStmt->forst.body, failed);
            break;
        case BLOCK_STMT:
            lFoldLengthStmt(program, fStmt->blockst.body, failed);
            break;
        case RETURN_STMT:
            fStmt->returnst.exp =
                lFoldLengthExpr(program, fStmt->returnst.exp, failed);
            break;
        case DISCARD_STMT:
            fStmt->discardst.cond =
                lFoldLengthExpr(program, fStmt->discardst.cond, failed);
            break;
        case COMMENT_STMT:
        case BREAK_STMT:
        case CONTINUE_STMT:
        default:
            break;
        }
    }
} // lFoldLengthStmt

/*
 * CgGeometryResolveLengths() - Fold attribute-array .length queries
 *         across the selected reachability graph and record the
 *         resolved type views.  Each reachable function contributes
 *         its attrib-array formals as deduplicated views and gets its
 *         body folded; allocation goes through the program's arena
 *         and any refusal marks the whole program failed.
 *
 * Returns: TRUE if O.K.
 *
 */

int CgGeometryResolveLengths(CgGeometryProgram *program,
                             const CgReachGraph *reach,
                             CgGeometryDiagnostic *diagnostic)
{
    const CgReachNode *node;
    CgGeometryTypeView *view;
    CgGeometryTypeView **tail;
    Symbol *formal;
    Symbol *function;
    int ii;
    int failed;

    ClearDiagnostic(diagnostic);
    if (!program || !reach ||
        program->config.stage != CGIR_STAGE_GEOMETRY) {
        diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_ATTRIB_STAGE;
        return 0;
    }
    failed = 0;
    for (ii = 0; ii < reach->nodeCount && !failed; ii++) {
        node = &reach->nodes[ii];
        function = node->symbol;
        if (!function || function->kind != FUNCTION_S) {
            continue;
        }
        for (formal = function->details.fun.params;
             formal && !failed; formal = formal->next)
        {
            if (!CgIsAttribArray(formal->type)) {
                continue;
            }
            view = program->typeViews;
            while (view && view->sourceType != formal->type) {
                view = view->next;
            }
            if (view) {
                continue;
            }
            view = (CgGeometryTypeView *)
                program->alloc(program->allocArg, sizeof(*view));
            if (!view) {
                failed = 1;
                break;
            }
            view->sourceType = formal->type;
            view->resolvedType =
                CgGetAttribArrayType(
                    CgAttribArrayElement(formal->type),
                    program->config.inputVertexCount);
            view->next = NULL;
            tail = &program->typeViews;
            while (*tail != NULL) {
                tail = &(*tail)->next;
            }
            *tail = view;
        }
        if (!failed && function->details.fun.statements) {
            lFoldLengthStmt(program,
                            function->details.fun.statements, &failed);
        }
    }
    if (failed) {
        program->failed = 1;
        diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_ALLOCATION;
        return 0;
    }
    return 1;
} // CgGeometryResolveLengths
