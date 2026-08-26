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
 * cg_geometry_test.c
 */

#if defined(NDEBUG)
#undef NDEBUG
#endif
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slglobals.h"
#include "cg_geometry.h"
#include "cg_types.h"
#include "cg_reach.h"

CgStruct *Cg;
Scope *CurrentScope;

/*
 * OptionChain() - Terminate one manually built option node.
 */

static CgProfileOption *OptionChain(CgProfileOption *node,
                                    const char *text,
                                    int ordinal,
                                    CgProfileOption *next)
{
    node->next = next;
    node->text = text;
    node->ordinal = ordinal;
    return node;
}

static void TestTopologyDefaults(void)
{
    CgGeometryModifiers source;
    CgGeometryOptions options;
    CgGeometryConfig config;
    CgGeometryDiagnostic diagnostic;

    CgGeometryInitModifiers(&source);
    CgGeometryInitOptions(&options);
    source.input = CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY;
    assert(CgGeometryResolveConfig(&source, &options,
           CGIR_STAGE_NEUTRAL, &config, &diagnostic));
    assert(config.stage == CGIR_STAGE_GEOMETRY);
    assert(config.inputTopology == CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY);
    assert(config.outputTopology == CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP);
    assert(config.inputVertexCount == 6);
    assert(!config.hasMaxOutputVertices);
}

static void TestGeometryOptions(void)
{
    CgProfileOption third;
    CgProfileOption second;
    CgProfileOption first;
    CgGeometryOptions options;
    CgGeometryDiagnostic diagnostic;

    third.next = NULL;
    third.text = "Vertices=12";
    third.ordinal = 2;
    second.next = &third;
    second.text = "TRIANGLE_OUT";
    second.ordinal = 1;
    first.next = &second;
    first.text = "TRIANGLE";
    first.ordinal = 0;
    CgGeometryInitOptions(&options);
    assert(CgGeometryParseOptions(&first, &options, &diagnostic));
    assert(options.input == CG_GEOMETRY_INPUT_TRIANGLE);
    assert(options.output == CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP);
    assert(options.hasMaxOutputVertices);
    assert(options.maxOutputVertices == 12);
}

static void TestInitializedState(void)
{
    CgGeometryModifiers source;
    CgGeometryOptions options;

    CgGeometryInitModifiers(&source);
    assert(source.input == CG_GEOMETRY_INPUT_UNKNOWN);
    assert(source.output == CG_GEOMETRY_OUTPUT_UNKNOWN);
    assert(source.inputLoc.file == 0 && source.inputLoc.line == 0);
    assert(source.outputLoc.file == 0 && source.outputLoc.line == 0);

    CgGeometryInitOptions(&options);
    assert(options.input == CG_GEOMETRY_INPUT_UNKNOWN);
    assert(options.output == CG_GEOMETRY_OUTPUT_UNKNOWN);
    assert(options.maxOutputVertices == 0);
    assert(!options.hasMaxOutputVertices);
}

static void TestInputCountsAndDefaults(void)
{
    const CgGeometryInput inputs[5] = {
        CG_GEOMETRY_INPUT_POINT,
        CG_GEOMETRY_INPUT_LINE,
        CG_GEOMETRY_INPUT_LINE_ADJACENCY,
        CG_GEOMETRY_INPUT_TRIANGLE,
        CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY
    };
    const unsigned int counts[5] = { 1, 2, 4, 3, 6 };
    const CgGeometryOutput outputs[5] = {
        CG_GEOMETRY_OUTPUT_POINTS,
        CG_GEOMETRY_OUTPUT_LINE_STRIP,
        CG_GEOMETRY_OUTPUT_LINE_STRIP,
        CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP,
        CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP
    };
    int ii;
    CgGeometryModifiers source;
    CgGeometryOptions options;
    CgGeometryConfig config;
    CgGeometryDiagnostic diagnostic;

    assert(CgGeometryInputVertexCount(CG_GEOMETRY_INPUT_UNKNOWN) == 0);
    assert(CgGeometryDefaultOutput(CG_GEOMETRY_INPUT_UNKNOWN) ==
           CG_GEOMETRY_OUTPUT_UNKNOWN);
    for (ii = 0; ii < 5; ii++) {
        assert(CgGeometryInputVertexCount(inputs[ii]) == counts[ii]);
        assert(CgGeometryDefaultOutput(inputs[ii]) == outputs[ii]);

        CgGeometryInitModifiers(&source);
        CgGeometryInitOptions(&options);
        source.input = inputs[ii];
        assert(CgGeometryResolveConfig(&source, &options,
               CGIR_STAGE_NEUTRAL, &config, &diagnostic));
        assert(config.stage == CGIR_STAGE_GEOMETRY);
        assert(config.inputTopology == inputs[ii]);
        assert(config.outputTopology == outputs[ii]);
        assert(config.inputVertexCount == counts[ii]);
        assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_NONE);
    }
    assert(!strcmp(CgGeometryInputName(CG_GEOMETRY_INPUT_TRIANGLE),
                   "TRIANGLE"));
    assert(!strcmp(CgGeometryInputName(
               CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY),
               "TRIANGLE_ADJACENCY"));
    assert(!strcmp(CgGeometryOutputName(CG_GEOMETRY_OUTPUT_POINTS),
                   "POINTS"));
    assert(!strcmp(CgGeometryOutputName(CG_GEOMETRY_OUTPUT_LINE_STRIP),
                   "LINE_STRIP"));
    assert(!strcmp(CgGeometryOutputName(
               CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP), "TRIANGLE_STRIP"));
}

static void TestNonGeometryStageCopy(void)
{
    const CgIRStage stages[4] = {
        CGIR_STAGE_UNKNOWN,
        CGIR_STAGE_NEUTRAL,
        CGIR_STAGE_VERTEX,
        CGIR_STAGE_FRAGMENT
    };
    int ii;
    CgGeometryModifiers source;
    CgGeometryOptions options;
    CgGeometryConfig config;
    CgGeometryDiagnostic diagnostic;

    for (ii = 0; ii < 4; ii++) {
        CgGeometryInitModifiers(&source);
        CgGeometryInitOptions(&options);
        assert(CgGeometryResolveConfig(&source, &options, stages[ii],
               &config, &diagnostic));
        assert(config.stage == stages[ii]);
        assert(config.inputTopology == CG_GEOMETRY_INPUT_UNKNOWN);
        assert(config.outputTopology == CG_GEOMETRY_OUTPUT_UNKNOWN);
        assert(config.inputVertexCount == 0);
        assert(config.maxOutputVertices == 0);
        assert(!config.hasMaxOutputVertices);
    }
}

static void TestGeometryFromOptionsAlone(void)
{
    CgProfileOption first;
    CgProfileOption second;
    CgGeometryModifiers source;
    CgGeometryOptions options;
    CgGeometryConfig config;
    CgGeometryDiagnostic diagnostic;

    OptionChain(&second, "Vertices=7", 1, NULL);
    OptionChain(&first, "LINE", 0, &second);
    CgGeometryInitModifiers(&source);
    CgGeometryInitOptions(&options);
    assert(CgGeometryParseOptions(&first, &options, &diagnostic));
    assert(options.inputOrdinal == 0);
    assert(options.verticesOrdinal == 1);
    assert(CgGeometryResolveConfig(&source, &options,
           CGIR_STAGE_FRAGMENT, &config, &diagnostic));
    assert(config.stage == CGIR_STAGE_GEOMETRY);
    assert(config.inputTopology == CG_GEOMETRY_INPUT_LINE);
    assert(config.outputTopology == CG_GEOMETRY_OUTPUT_LINE_STRIP);
    assert(config.inputVertexCount == 2);
    assert(config.hasMaxOutputVertices);
    assert(config.maxOutputVertices == 7);
}

static void TestDuplicateEqualAccepted(void)
{
    CgProfileOption second;
    CgProfileOption first;
    CgGeometryOptions options;
    CgGeometryDiagnostic diagnostic;

    OptionChain(&second, "LINE", 1, NULL);
    OptionChain(&first, "LINE", 0, &second);
    CgGeometryInitOptions(&options);
    assert(CgGeometryParseOptions(&first, &options, &diagnostic));
    assert(options.input == CG_GEOMETRY_INPUT_LINE);

    OptionChain(&second, "POINT_OUT", 1, NULL);
    OptionChain(&first, "POINT_OUT", 0, &second);
    CgGeometryInitOptions(&options);
    assert(CgGeometryParseOptions(&first, &options, &diagnostic));
    assert(options.output == CG_GEOMETRY_OUTPUT_POINTS);
}

static void TestConflictingInputRejected(void)
{
    CgProfileOption second;
    CgProfileOption first;
    CgGeometryOptions options;
    CgGeometryDiagnostic diagnostic;

    OptionChain(&second, "LINE", 1, NULL);
    OptionChain(&first, "TRIANGLE", 0, &second);
    CgGeometryInitOptions(&options);
    assert(!CgGeometryParseOptions(&first, &options, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_CONFLICTING_INPUT);
    assert(diagnostic.optionOrdinal == 1);
}

static void TestConflictingOutputRejected(void)
{
    CgProfileOption second;
    CgProfileOption first;
    CgGeometryOptions options;
    CgGeometryDiagnostic diagnostic;

    OptionChain(&second, "TRIANGLE_OUT", 1, NULL);
    OptionChain(&first, "POINT_OUT", 0, &second);
    CgGeometryInitOptions(&options);
    assert(!CgGeometryParseOptions(&first, &options, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_CONFLICTING_OUTPUT);
    assert(diagnostic.optionOrdinal == 1);
}

static void TestSourceOptionConflict(void)
{
    CgProfileOption inputOption;
    CgProfileOption outputOption;
    CgGeometryModifiers source;
    CgGeometryOptions options;
    CgGeometryConfig config;
    CgGeometryDiagnostic diagnostic;

    OptionChain(&inputOption, "LINE", 0, NULL);
    CgGeometryInitModifiers(&source);
    CgGeometryInitOptions(&options);
    assert(CgGeometryParseOptions(&inputOption, &options, &diagnostic));
    source.input = CG_GEOMETRY_INPUT_TRIANGLE;
    assert(!CgGeometryResolveConfig(&source, &options,
           CGIR_STAGE_NEUTRAL, &config, &diagnostic));
    assert(diagnostic.reason ==
           CG_GEOMETRY_DIAGNOSTIC_SOURCE_OPTION_CONFLICT);
    assert(diagnostic.optionOrdinal == options.inputOrdinal);

    OptionChain(&outputOption, "TRIANGLE_OUT", 3, NULL);
    CgGeometryInitModifiers(&source);
    CgGeometryInitOptions(&options);
    assert(CgGeometryParseOptions(&outputOption, &options, &diagnostic));
    source.output = CG_GEOMETRY_OUTPUT_POINTS;
    assert(!CgGeometryResolveConfig(&source, &options,
           CGIR_STAGE_NEUTRAL, &config, &diagnostic));
    assert(diagnostic.reason ==
           CG_GEOMETRY_DIAGNOSTIC_SOURCE_OPTION_CONFLICT);
    assert(diagnostic.optionOrdinal == options.outputOrdinal);
}

static void TestUnknownOptionRejected(void)
{
    CgProfileOption second;
    CgProfileOption first;
    CgGeometryOptions options;
    CgGeometryDiagnostic diagnostic;

    /* No hidden profile-version selector exists behind "-po". */
    OptionChain(&first, "GLSLVersion=150", 0, NULL);
    CgGeometryInitOptions(&options);
    assert(!CgGeometryParseOptions(&first, &options, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_UNKNOWN_OPTION);
    assert(diagnostic.optionOrdinal == 0);
    assert(!strcmp(diagnostic.optionText, "GLSLVersion=150"));

    /* Spellings are matched exactly, never partially or caseless. */
    OptionChain(&second, "TRIANGLE_OUT", 1, NULL);
    OptionChain(&first, "triangle", 0, &second);
    CgGeometryInitOptions(&options);
    assert(!CgGeometryParseOptions(&first, &options, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_UNKNOWN_OPTION);
    assert(diagnostic.optionOrdinal == 0);

    OptionChain(&first, "TRIANGL", 2, NULL);
    CgGeometryInitOptions(&options);
    assert(!CgGeometryParseOptions(&first, &options, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_UNKNOWN_OPTION);
    assert(diagnostic.optionOrdinal == 2);
}

static void TestMalformedVerticesRejected(void)
{
    const char *texts[9];
    int ii;
    CgProfileOption first;
    CgProfileOption second;
    CgGeometryOptions options;
    CgGeometryDiagnostic diagnostic;

    texts[0] = "Vertices=0";
    texts[1] = "Vertices=+8";
    texts[2] = "Vertices=-8";
    texts[3] = "Vertices=x";
    texts[4] = "Vertices=";
    texts[5] = "Vertices=8x";
    texts[6] = "Vertices=8 ";
    texts[7] = "Vertices= 8";
    texts[8] = "Vertices=4294967296";
    for (ii = 0; ii < 9; ii++) {
        OptionChain(&second, "POINT", 0, NULL);
        OptionChain(&first, texts[ii], 1, &second);
        CgGeometryInitOptions(&options);
        assert(!CgGeometryParseOptions(&first, &options, &diagnostic));
        assert(diagnostic.reason ==
               CG_GEOMETRY_DIAGNOSTIC_MALFORMED_VERTICES);
        assert(diagnostic.optionOrdinal == 1);
        assert(!options.hasMaxOutputVertices);
    }
}

static void TestVerticesBoundaryAccepted(void)
{
    CgProfileOption first;
    CgGeometryOptions options;
    CgGeometryDiagnostic diagnostic;

    OptionChain(&first, "Vertices=4294967295", 0, NULL);
    CgGeometryInitOptions(&options);
    assert(CgGeometryParseOptions(&first, &options, &diagnostic));
    assert(options.hasMaxOutputVertices);
    assert(options.maxOutputVertices == UINT_MAX);
    assert(options.verticesOrdinal == 0);
}

static void TestDuplicateVerticesRejected(void)
{
    CgProfileOption second;
    CgProfileOption first;
    CgGeometryOptions options;
    CgGeometryDiagnostic diagnostic;

    OptionChain(&second, "Vertices=13", 1, NULL);
    OptionChain(&first, "Vertices=12", 0, &second);
    CgGeometryInitOptions(&options);
    assert(!CgGeometryParseOptions(&first, &options, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_DUPLICATE_VERTICES);
    assert(diagnostic.optionOrdinal == 1);
}

static void TestDuplicateEqualVerticesAccepted(void)
{
    CgProfileOption second;
    CgProfileOption first;
    CgGeometryOptions options;
    CgGeometryDiagnostic diagnostic;

    /* Equal repeats keep the first occurrence's provenance. */
    OptionChain(&second, "Vertices=12", 1, NULL);
    OptionChain(&first, "Vertices=12", 0, &second);
    CgGeometryInitOptions(&options);
    assert(CgGeometryParseOptions(&first, &options, &diagnostic));
    assert(options.hasMaxOutputVertices);
    assert(options.maxOutputVertices == 12);
    assert(options.verticesOrdinal == 0);
}

static void TestGeometryStageRequiresInput(void)
{
    CgGeometryModifiers source;
    CgGeometryOptions options;
    CgGeometryConfig config;
    CgGeometryDiagnostic diagnostic;

    /* A geometry-selected profile still needs an input topology. */
    CgGeometryInitModifiers(&source);
    CgGeometryInitOptions(&options);
    assert(!CgGeometryResolveConfig(&source, &options,
           CGIR_STAGE_GEOMETRY, &config, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_MISSING_INPUT);
}

/*
 * SourceLocOfLine() - A recognizable location for modifier provenance.
 */

static SourceLoc SourceLocOfLine(int line)
{
    SourceLoc loc;

    loc.file = 7;
    loc.line = line;
    return loc;
}

static void TestApplyInputModifier(void)
{
    CgGeometryModifiers modifiers;
    CgGeometryDiagnostic diagnostic;
    SourceLoc firstLoc;
    SourceLoc secondLoc;

    firstLoc = SourceLocOfLine(3);
    secondLoc = SourceLocOfLine(9);

    CgGeometryInitModifiers(&modifiers);
    assert(CgGeometryApplyInputModifier(&modifiers,
           CG_GEOMETRY_INPUT_LINE, &firstLoc, &diagnostic));
    assert(modifiers.input == CG_GEOMETRY_INPUT_LINE);
    assert(modifiers.inputLoc.line == 3);
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_NONE);

    /* Identical repeat: rejected as repeated, naming the first spot. */
    assert(!CgGeometryApplyInputModifier(&modifiers,
           CG_GEOMETRY_INPUT_LINE, &secondLoc, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_REPEATED_INPUT);
    assert(diagnostic.loc.line == 3);

    /* Contradictory value: a different structured reason, same anchor. */
    assert(!CgGeometryApplyInputModifier(&modifiers,
           CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY, &secondLoc, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_CONFLICTING_INPUT);
    assert(diagnostic.loc.line == 3);

    /* Failures leave the recorded value and location untouched. */
    assert(modifiers.input == CG_GEOMETRY_INPUT_LINE);
    assert(modifiers.inputLoc.line == 3);
}

static void TestApplyOutputModifier(void)
{
    CgGeometryModifiers modifiers;
    CgGeometryDiagnostic diagnostic;
    SourceLoc firstLoc;
    SourceLoc secondLoc;

    firstLoc = SourceLocOfLine(4);
    secondLoc = SourceLocOfLine(8);

    CgGeometryInitModifiers(&modifiers);
    assert(CgGeometryApplyOutputModifier(&modifiers,
           CG_GEOMETRY_OUTPUT_LINE_STRIP, &firstLoc, &diagnostic));
    assert(modifiers.output == CG_GEOMETRY_OUTPUT_LINE_STRIP);
    assert(modifiers.outputLoc.line == 4);
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_NONE);

    /* Identical repeat: rejected as repeated, naming the first spot. */
    assert(!CgGeometryApplyOutputModifier(&modifiers,
           CG_GEOMETRY_OUTPUT_LINE_STRIP, &secondLoc, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_REPEATED_OUTPUT);
    assert(diagnostic.loc.line == 4);

    /* Contradictory value: a different structured reason, same anchor. */
    assert(!CgGeometryApplyOutputModifier(&modifiers,
           CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP, &secondLoc, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_CONFLICTING_OUTPUT);
    assert(diagnostic.loc.line == 4);

    assert(modifiers.output == CG_GEOMETRY_OUTPUT_LINE_STRIP);
    assert(modifiers.outputLoc.line == 4);

    /* Input and output slots are independent records. */
    assert(CgGeometryApplyInputModifier(&modifiers,
           CG_GEOMETRY_INPUT_POINT, &secondLoc, &diagnostic));
    assert(modifiers.input == CG_GEOMETRY_INPUT_POINT);
    assert(modifiers.output == CG_GEOMETRY_OUTPUT_LINE_STRIP);
}

/*
 * StackType() - A zero-initialized type header for predicate tests:
 *         every field starts at zero, exactly like InitType leaves a
 *         Type, so only the assigned properties and payload differ
 *         from an untouched header.
 */

static void StackType(Type *type)
{
    memset(type, 0, sizeof(*type));
}

/*
 * TestAttribArrayElementTypePredicate() - The shared AttribArray<T>
 *         element rule accepts scalar, vector, matrix, ordinary
 *         array, and nested struct headers; it rejects void,
 *         undefined recovery types, functions, samplers, and
 *         interfaces.
 */

static void TestAttribArrayElementTypePredicate(void)
{
    Type scalar;
    Type vector;
    Type matrix;
    Type ordinaryArray;
    Type inner;
    Type outer;
    Type voidType;
    Type undefinedType;
    Type function;
    Type sampler2D;
    Type interfaceType;

    StackType(&scalar);
    scalar.properties = TYPE_BASE_FLOAT | TYPE_CATEGORY_SCALAR;
    scalar.co.scalarKind = CG_SCALAR_FLOAT;

    StackType(&vector);
    vector.properties = TYPE_BASE_FLOAT | TYPE_CATEGORY_ARRAY |
                        TYPE_MISC_PACKED;
    vector.arr.eltype = &scalar;
    vector.arr.numels = 4;

    StackType(&matrix);
    matrix.properties = TYPE_BASE_FLOAT | TYPE_CATEGORY_ARRAY |
                        TYPE_MISC_PACKED;
    matrix.arr.eltype = &vector;
    matrix.arr.numels = 4;

    StackType(&ordinaryArray);
    ordinaryArray.properties = TYPE_BASE_FLOAT | TYPE_CATEGORY_ARRAY;
    ordinaryArray.arr.eltype = &scalar;
    ordinaryArray.arr.numels = 2;

    /* Two struct headers where Outer models a body holding one Inner
     * member: the element rule judges the declared header category,
     * not the member bodies. */
    StackType(&inner);
    inner.properties = TYPE_CATEGORY_STRUCT;
    inner.str.tag = 1;
    inner.str.unqualifiedtype = &inner;

    StackType(&outer);
    outer.properties = TYPE_CATEGORY_STRUCT;
    outer.str.tag = 2;
    outer.str.unqualifiedtype = &outer;

    StackType(&voidType);
    voidType.properties = TYPE_BASE_VOID | TYPE_CATEGORY_SCALAR |
                          TYPE_MISC_VOID;

    StackType(&undefinedType);
    undefinedType.properties = TYPE_BASE_UNDEFINED_TYPE |
                               TYPE_CATEGORY_SCALAR;

    StackType(&function);
    function.properties = TYPE_CATEGORY_FUNCTION;
    function.fun.rettype = &scalar;

    StackType(&sampler2D);
    sampler2D.properties = TYPE_CATEGORY_SAMPLER | TYPE_BASE_FIRST_USER;
    sampler2D.samp.samplerKind = CG_SAMPLER_2D;

    StackType(&interfaceType);
    interfaceType.properties = TYPE_CATEGORY_INTERFACE;
    interfaceType.iface.tag = 3;

    assert(CgGeometryAcceptsAttribArrayElement(&scalar));
    assert(CgGeometryAcceptsAttribArrayElement(&vector));
    assert(CgGeometryAcceptsAttribArrayElement(&matrix));
    assert(CgGeometryAcceptsAttribArrayElement(&ordinaryArray));
    assert(CgGeometryAcceptsAttribArrayElement(&inner));
    assert(CgGeometryAcceptsAttribArrayElement(&outer));

    assert(!CgGeometryAcceptsAttribArrayElement(NULL));
    assert(!CgGeometryAcceptsAttribArrayElement(&voidType));
    assert(!CgGeometryAcceptsAttribArrayElement(&undefinedType));
    assert(!CgGeometryAcceptsAttribArrayElement(&function));
    assert(!CgGeometryAcceptsAttribArrayElement(&sampler2D));
    assert(!CgGeometryAcceptsAttribArrayElement(&interfaceType));
}

/*
 * FakeAlloc() - A recognizable allocator identity for the program
 *         record's stored allocator.
 */

static void *FakeAlloc(void *arg, size_t size)
{
    (void) arg;
    (void) size;
    return NULL;
}

/*
 * TestInitProgramStoresAllocator() - CgGeometryInitProgram zeroes the
 *         whole record (no stale stage, views, entry, or failure
 *         state) and stores the allocator pair verbatim.
 */

static void TestInitProgramStoresAllocator(void)
{
    CgGeometryProgram program;
    int cookie;

    cookie = 0;
    memset(&program, 0xa7, sizeof(program));
    CgGeometryInitProgram(&program, FakeAlloc, &cookie);
    assert(program.alloc == FakeAlloc);
    assert(program.allocArg == &cookie);
    assert(program.entry == NULL);
    assert(program.typeViews == NULL);
    assert(!program.failed);
    assert(program.config.stage == CGIR_STAGE_UNKNOWN);
    assert(program.config.inputTopology == CG_GEOMETRY_INPUT_UNKNOWN);
}

/*
 * TestRegisterNames() - Stub HAL name registration for InitSymbolTable.
 */

static int TestRegisterNames(slHAL *fHAL)
{
    (void) fHAL;
    return 1;
}

/*
 * TestGetSizeof() - Minimal size query mirroring GetSizeof_HAL.
 */

static int TestGetSizeof(Type *fType)
{
    if (!fType) {
        return 0;
    }
    return fType->co.size;
}

/*
 * ArenaAlloc() - Bounded bump allocator so resolution state allocated
 *         through a program record never leaks and never depends on
 *         malloc failure timing.
 */

static unsigned char lViewArena[8192];
static size_t lViewArenaUsed;

static void *ArenaAlloc(void *arg, size_t size)
{
    void *block;

    (void) arg;
    if (lViewArenaUsed + size > sizeof(lViewArena)) {
        return NULL;
    }
    block = &lViewArena[lViewArenaUsed];
    lViewArenaUsed += size;
    return block;
}

/*
 * StackSymbol() - A zero-initialized variable symbol whose only
 *         assigned fields are the kind, name atom, location, and type
 *         under test.
 */

static Symbol *StackSymbol(Symbol *symbol, Type *type, int line)
{
    memset(symbol, 0, sizeof(*symbol));
    symbol->kind = VARIABLE_S;
    symbol->name = 1;
    symbol->loc.file = 3;
    symbol->loc.line = line;
    symbol->type = type;
    return symbol;
}

/*
 * MakeGeometryProgram() - One initialized program record whose stage
 *         and input vertex count are set for validation tests; every
 *         other field keeps its cleared state.
 */

static void MakeGeometryProgram(CgGeometryProgram *program, CgIRStage stage)
{
    CgGeometryInitProgram(program, ArenaAlloc, NULL);
    program->config.stage = stage;
    program->config.inputTopology = CG_GEOMETRY_INPUT_TRIANGLE;
    program->config.outputTopology = CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP;
    program->config.inputVertexCount = 3;
}

/*
 * TestAttribArrayDeclarationValidation() - Placement rules: entry
 *         inputs are admitted on the strength of their own source
 *         modifiers (selected-program analysis confirms them later),
 *         helper inputs need a resolved geometry program, every other
 *         use is rejected with one structured reason, and illegal
 *         elements are intrinsic failures that outrank placement.
 */

static void TestAttribArrayDeclarationValidation(void)
{
    const CgGeometryDeclarationUse placements[6] = {
        CG_GEOMETRY_DECL_GLOBAL,
        CG_GEOMETRY_DECL_UNIFORM,
        CG_GEOMETRY_DECL_OUTPUT,
        CG_GEOMETRY_DECL_RETURN,
        CG_GEOMETRY_DECL_MEMBER,
        CG_GEOMETRY_DECL_LOCAL
    };
    CgGeometryProgram geometry;
    CgGeometryProgram neutral;
    CgGeometryDiagnostic diagnostic;
    Type *array4;
    Type *badVoid;
    Type *badSampler;
    Type *ordinary;
    Symbol formal;
    int ii;

    array4 = CgGetAttribArrayType(Float4Type, 0);
    badVoid = CgGetAttribArrayType(VoidType, 0);
    badSampler = CgGetAttribArrayType(GetSamplerType(CG_SAMPLER_2D), 0);
    ordinary = Float4Type;
    assert(CgIsAttribArray(array4));

    /* Entry inputs self-certify; resolved geometry admits helpers. */
    StackSymbol(&formal, array4, 9);
    MakeGeometryProgram(&geometry, CGIR_STAGE_GEOMETRY);
    MakeGeometryProgram(&neutral, CGIR_STAGE_NEUTRAL);

    assert(CgGeometryValidateAttribArrayDeclaration(NULL, &formal,
           CG_GEOMETRY_DECL_ENTRY_INPUT, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_NONE);
    assert(CgGeometryValidateAttribArrayDeclaration(&geometry, &formal,
           CG_GEOMETRY_DECL_ENTRY_INPUT, &diagnostic));
    assert(CgGeometryValidateAttribArrayDeclaration(&geometry, &formal,
           CG_GEOMETRY_DECL_HELPER_INPUT, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_NONE);

    /* Helper inputs without a resolved geometry program are rejected:
       reachability is proven only by selected-program analysis. */
    assert(!CgGeometryValidateAttribArrayDeclaration(NULL, &formal,
           CG_GEOMETRY_DECL_HELPER_INPUT, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_ATTRIB_STAGE);
    assert(diagnostic.loc.line == 9);
    assert(!CgGeometryValidateAttribArrayDeclaration(&neutral, &formal,
           CG_GEOMETRY_DECL_HELPER_INPUT, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_ATTRIB_STAGE);
    assert(!CgGeometryValidateAttribArrayDeclaration(&neutral, &formal,
           CG_GEOMETRY_DECL_ENTRY_INPUT, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_ATTRIB_STAGE);

    /* Every non-input placement fails with its own structured reason,
       whether or not a program has been resolved. */
    for (ii = 0; ii < 6; ii++) {
        assert(!CgGeometryValidateAttribArrayDeclaration(&geometry,
               &formal, placements[ii], &diagnostic));
        assert(diagnostic.reason ==
               CG_GEOMETRY_DIAGNOSTIC_ATTRIB_PLACEMENT);
        assert(diagnostic.loc.line == 9);
        assert(!CgGeometryValidateAttribArrayDeclaration(NULL,
               &formal, placements[ii], &diagnostic));
        assert(diagnostic.reason ==
               CG_GEOMETRY_DIAGNOSTIC_ATTRIB_PLACEMENT);
    }

    /* Illegal elements are intrinsic failures that fire before any
     * placement rule, so recovery names the type problem first. */
    StackSymbol(&formal, badVoid, 4);
    assert(!CgGeometryValidateAttribArrayDeclaration(NULL, &formal,
           CG_GEOMETRY_DECL_ENTRY_INPUT, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_ATTRIB_ELEMENT);
    assert(!CgGeometryValidateAttribArrayDeclaration(NULL, &formal,
           CG_GEOMETRY_DECL_LOCAL, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_ATTRIB_ELEMENT);

    StackSymbol(&formal, badSampler, 5);
    assert(!CgGeometryValidateAttribArrayDeclaration(&geometry, &formal,
           CG_GEOMETRY_DECL_ENTRY_INPUT, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_ATTRIB_ELEMENT);

    /* Non-attribute-array declarations are never validated. */
    StackSymbol(&formal, ordinary, 7);
    assert(CgGeometryValidateAttribArrayDeclaration(NULL, &formal,
           CG_GEOMETRY_DECL_LOCAL, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_NONE);
    assert(CgGeometryValidateAttribArrayDeclaration(NULL, NULL,
           CG_GEOMETRY_DECL_GLOBAL, &diagnostic));
}

/*
 * InitSymbNode() - A hand-built variable reference of "type": the
 *         minimal SYMB_N shape every walker must leave alone.
 */

static void InitSymbNode(symb *node, Type *type)
{
    memset(node, 0, sizeof(*node));
    node->kind = SYMB_N;
    node->op = VARIABLE_OP;
    node->type = type;
    node->IsLValue = 1;
}

/*
 * InitLengthNode() - A hand-built ARRAY_LENGTH_OP query over "arg"
 *         with the canonical int type the parser writes.
 */

static void InitLengthNode(unary *node, expr *arg)
{
    memset(node, 0, sizeof(*node));
    node->kind = UNARY_N;
    node->op = ARRAY_LENGTH_OP;
    node->type = GetStandardTypeKind(CG_SCALAR_INT, 0, 0);
    node->arg = arg;
}

/*
 * InitExprStmt() - A hand-built expression statement holding "exp".
 */

static void InitExprStmt(expr_stmt *statement, expr *exp)
{
    memset(statement, 0, sizeof(*statement));
    statement->kind = EXPR_STMT;
    statement->exp = exp;
}

/*
 * StackFunction() - A zero-initialized function symbol whose body is
 *         "body" and whose single formal chain starts at "params".
 */

static Symbol *StackFunction(Symbol *function, stmt *body, Symbol *params)
{
    memset(function, 0, sizeof(*function));
    function->kind = FUNCTION_S;
    function->name = 2;
    function->loc.file = 3;
    function->loc.line = 12;
    function->details.fun.statements = body;
    function->details.fun.params = params;
    return function;
}

/*
 * TestResolveLengths() - Folding: a resolved geometry program replaces
 *         every attribute-array .length node in its reachable set
 *         with an int constant equal to config.inputVertexCount,
 *         records one resolved type view per source array, never
 *         mutates the unresolved canonical type, leaves unreachable
 *         bodies untouched, and rejects use outside a resolved
 *         geometry program.
 */

static void TestResolveLengths(void)
{
    CgGeometryProgram geometry;
    CgGeometryDiagnostic diagnostic;
    CgReachGraph graph;
    CgReachNode nodes[2];
    Type *unresolved;
    Type *resolved;
    Symbol entryFunction;
    Symbol helperFunction;
    Symbol formalP;
    Symbol formalQ;
    expr_stmt firstStatement;
    expr_stmt secondStatement;
    expr_stmt helperStatement;
    unary firstLength;
    unary secondLength;
    symb firstVar;
    symb secondVar;
    const constant *folded;

    lViewArenaUsed = 0;
    unresolved = CgGetAttribArrayType(Float4Type, 0);
    resolved = CgGetAttribArrayType(Float4Type, 3);

    /* Entry body: two chained length queries over two formals. */
    StackSymbol(&formalP, unresolved, 13);
    formalP.next = &formalQ;
    StackSymbol(&formalQ, unresolved, 14);
    formalQ.next = NULL;
    InitSymbNode(&firstVar, unresolved);
    InitSymbNode(&secondVar, unresolved);
    InitLengthNode(&firstLength, (expr *) &firstVar);
    InitLengthNode(&secondLength, (expr *) &secondVar);
    InitExprStmt(&firstStatement, (expr *) &firstLength);
    InitExprStmt(&secondStatement, (expr *) &secondLength);
    firstStatement.next = (stmt *) &secondStatement;

    StackFunction(&entryFunction, (stmt *) &firstStatement, &formalP);
    StackFunction(&helperFunction, NULL, NULL);

    nodes[0].symbol = &entryFunction;
    nodes[0].witness.from = NULL;
    nodes[0].witness.to = &entryFunction;
    nodes[0].expanded = 1;
    graph.nodes = nodes;
    graph.nodeCount = 1;
    graph.capacity = 1;
    graph.failed = 0;

    MakeGeometryProgram(&geometry, CGIR_STAGE_GEOMETRY);
    geometry.entry = &entryFunction;
    assert(CgGeometryResolveLengths(&geometry, &graph, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_NONE);

    /* Both length nodes became int constants of the input count. */
    folded = (const constant *) firstStatement.exp;
    assert(folded->kind == CONST_N);
    assert(folded->op == ICONST_OP);
    assert(folded->val[0].value.i == 3);
    assert(GetScalarKind(folded->type) == CG_SCALAR_CINT);
    folded = (const constant *) secondStatement.exp;
    assert(folded->kind == CONST_N);
    assert(folded->val[0].value.i == 3);

    /* One deduplicated resolved view per source array; the canonical
     * source type itself stays unresolved. */
    assert(geometry.typeViews != NULL);
    assert(geometry.typeViews->sourceType == unresolved);
    assert(geometry.typeViews->resolvedType == resolved);
    assert(geometry.typeViews->next == NULL);
    assert(CgAttribArrayExtent(unresolved) == 0);
    assert(CgIsAttribArray(unresolved));
    assert(CgAttribArrayElement(resolved) == Float4Type);

    /* An unreachable helper keeps its length node untouched. */
    InitSymbNode(&firstVar, unresolved);
    InitLengthNode(&firstLength, (expr *) &firstVar);
    InitExprStmt(&helperStatement, (expr *) &firstLength);
    StackFunction(&helperFunction, (stmt *) &helperStatement, &formalQ);
    assert(CgGeometryResolveLengths(&geometry, &graph, &diagnostic));
    assert(helperStatement.exp == (expr *) &firstLength);

    /* Use outside a resolved geometry program is rejected, not
     * silently folded to zero. */
    MakeGeometryProgram(&geometry, CGIR_STAGE_VERTEX);
    assert(!CgGeometryResolveLengths(&geometry, &graph, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_ATTRIB_STAGE);
    assert(!CgGeometryResolveLengths(NULL, &graph, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_ATTRIB_STAGE);
}

int main(void)
{
    CgStruct cg;
    slHAL hal;

    memset(&cg, 0, sizeof(cg));
    memset(&hal, 0, sizeof(hal));
    hal.GetSizeof = TestGetSizeof;
    hal.RegisterNames = TestRegisterNames;
    cg.theHAL = &hal;
    Cg = &cg;

    assert(InitAtomTable(atable, 0));
    assert(InitSymbolTable(Cg));

    TestInitializedState();
    TestTopologyDefaults();
    TestGeometryOptions();
    TestInputCountsAndDefaults();
    TestNonGeometryStageCopy();
    TestGeometryFromOptionsAlone();
    TestDuplicateEqualAccepted();
    TestConflictingInputRejected();
    TestConflictingOutputRejected();
    TestSourceOptionConflict();
    TestUnknownOptionRejected();
    TestMalformedVerticesRejected();
    TestVerticesBoundaryAccepted();
    TestDuplicateVerticesRejected();
    TestDuplicateEqualVerticesAccepted();
    TestGeometryStageRequiresInput();
    TestApplyInputModifier();
    TestApplyOutputModifier();
    TestAttribArrayElementTypePredicate();
    TestInitProgramStoresAllocator();
    TestAttribArrayDeclarationValidation();
    TestResolveLengths();
    return 0;
}

void SemanticError(SourceLoc *loc, int num, const char *mess, ...)
{
    (void) loc;
    (void) num;
    (void) mess;
}

void InternalError(SourceLoc *loc, int num, const char *mess, ...)
{
    (void) loc;
    (void) num;
    (void) mess;
}

dtype CurrentDeclTypeSpecs = { 0, };

Symbol *DefineTypedef(SourceLoc *loc, Scope *fScope, int atom, Type *fType)
{
    (void) loc;
    (void) fScope;
    (void) atom;
    (void) fType;
    return NULL;
}
