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
#include "cg_stdlib.h"

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
 * TestGetAlignment() - Minimal alignment query so struct completion
 *         never calls through an unset HAL slot.
 */

static int TestGetAlignment(Type *fType)
{
    (void) fType;
    return 4;
}

/*
 * ArenaAlloc() - Bounded bump allocator so resolution state allocated
 *         through a program record never leaks and never depends on
 *         malloc failure timing.
 */

static unsigned char lViewArena[16384];
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

/*
 * The operation-record, bundle-resolution, and statement-classifier
 * tests below hand-build exactly the node shapes the parser and call
 * resolution produce: GEOMETRY_ARGUMENT_OP wrappers around typed
 * leaves, MEMBER_SELECTOR_OP chains over MEMBER_OP symbols, and
 * FUN_CALL_OP nodes whose callee carries an immutable catalog
 * signature.
 */

static const CgIntrinsicSignature lEmitVertexSignature = {
    CG_INTRINSIC_EMIT_VERTEX, "emitVertex", NULL, NULL,
    CG_INTRINSIC_FLAG_SIDE_EFFECT | CG_INTRINSIC_FLAG_GEOMETRY
};

static const CgIntrinsicSignature lFlatAttribSignature = {
    CG_INTRINSIC_FLAT_ATTRIB, "flatAttrib", NULL, NULL,
    CG_INTRINSIC_FLAG_SIDE_EFFECT | CG_INTRINSIC_FLAG_GEOMETRY
};

static const CgIntrinsicSignature lRestartStripSignature = {
    CG_INTRINSIC_RESTART_STRIP, "restartStrip", NULL, NULL,
    CG_INTRINSIC_FLAG_SIDE_EFFECT | CG_INTRINSIC_FLAG_GEOMETRY
};

/*
 * TestAtom() - Intern one spelling so semantic atoms read like source.
 */

static int TestAtom(const char *text)
{
    return AddAtom(atable, text);
}

/*
 * WrapArgument() - A hand-built geometry argument wrapper with the
 *         exact shape NewGeometryArgument writes (no support.c link).
 */

static expr *WrapArgument(struct geometry_arg_rec *storage,
                          SourceLoc *loc, expr *value, int semantic)
{
    memset(storage, 0, sizeof(*storage));
    storage->kind = BINARY_N;
    storage->type = value ? value->common.type : UndefinedType;
    storage->op = GEOMETRY_ARGUMENT_OP;
    storage->left = value;
    storage->semantic = semantic;
    if (loc) {
        storage->loc = *loc;
    }
    return (expr *) storage;
}

/*
 * InitOpSymb() - A symbolic leaf node with an explicit opcode, so
 *         VARIABLE_OP references and MEMBER_OP member names share one
 *         initializer.
 */

static void InitOpSymb(symb *node, opcode op, Symbol *symbol, Type *type)
{
    memset(node, 0, sizeof(*node));
    node->kind = SYMB_N;
    node->op = op;
    node->symbol = symbol;
    node->type = type;
    node->IsLValue = 1;
}

/*
 * MakeBinaryNode() / MakeTrinaryNode() - Generic interior nodes with
 *         an explicit result type.
 */

static expr *MakeBinaryNode(binary *storage, opcode op, expr *left,
                            expr *right, Type *type)
{
    memset(storage, 0, sizeof(*storage));
    storage->kind = BINARY_N;
    storage->op = op;
    storage->left = left;
    storage->right = right;
    storage->type = type;
    return (expr *) storage;
}

static expr *MakeTrinaryNode(trinary *storage, opcode op, expr *arg1,
                             expr *arg2, expr *arg3, Type *type)
{
    memset(storage, 0, sizeof(*storage));
    storage->kind = TRINARY_N;
    storage->op = op;
    storage->arg1 = arg1;
    storage->arg2 = arg2;
    storage->arg3 = arg3;
    storage->type = type;
    return (expr *) storage;
}

/*
 * AppendArgument() - Join one actual argument onto a FUN_ARG_OP chain,
 *         mirroring ArgumentList().
 */

static expr *AppendArgument(binary *linkStorage, expr *chain, expr *arg)
{
    expr *link;

    link = MakeBinaryNode(linkStorage, FUN_ARG_OP, arg, NULL, NULL);
    if (!chain) {
        return link;
    }
    while (chain->bin.right != NULL) {
        chain = chain->bin.right;
    }
    chain->bin.right = link;
    return chain;
}

/*
 * MakeSpecialCall() - A FUN_CALL_OP node whose callee symbol carries
 *         "signature", exactly what the special-call short circuit
 *         leaves after typecheck.
 */

static expr *MakeSpecialCall(binary *callStorage, symb *calleeStorage,
                             Symbol *callee, const CgIntrinsicSignature *signature,
                             expr *arguments)
{
    memset(callee, 0, sizeof(*callee));
    callee->kind = FUNCTION_S;
    callee->name = TestAtom(signature->name);
    callee->details.fun.intrinsic = signature;
    InitOpSymb(calleeStorage, VARIABLE_OP, callee, VoidType);
    return MakeBinaryNode(callStorage, FUN_CALL_OP,
                          (expr *) calleeStorage, arguments, VoidType);
}

/*
 * MakeMemberSpec / MakeStructType() - A struct type whose members sit
 *         in declaration order on the member scope's params chain,
 *         each carrying its declared semantic atom and location.
 */

typedef struct TestMemberSpec_Rec {
    const char *name;
    Type *type;
    const char *semantic;
    int line;
} TestMemberSpec;

static Type *MakeStructType(const TestMemberSpec *specs, int count)
{
    Scope *members;
    Type *strct;
    Symbol *member;
    SourceLoc loc;
    int ii;

    strct = NewType(TYPE_CATEGORY_STRUCT, 0);
    /* The unit never runs StartGlobalScope, so this scope owns a
     * fresh process-lifetime pool instead of borrowing one. */
    members = NewScopeInPool(mem_CreatePool(0, 0));
    members->IsStructScope = 1;
    loc.file = 3;
    loc.line = 1;
    for (ii = 0; ii < count; ii++) {
        loc.line = specs[ii].line;
        member = NewSymbol(&loc, members, TestAtom(specs[ii].name),
                           specs[ii].type, VARIABLE_S);
        if (specs[ii].semantic) {
            member->details.var.semantics = TestAtom(specs[ii].semantic);
        }
        AddParameter(members, member);
    }
    return SetStructMembers(&loc, strct, members);
}

/*
 * CountValues() - Walk one resolved bundle's value list.
 */

static int CountValues(const CgGeometryValue *values)
{
    int count = 0;

    while (values) {
        count++;
        values = values->next;
    }
    return count;
}

/*
 * ValueAt() - The bundle value at zero-based position "index".
 */

static const CgGeometryValue *ValueAt(const CgGeometryValue *values, int index)
{
    while (index > 0 && values) {
        values = values->next;
        index--;
    }
    return values;
}

/*
 * TestFindLookups() - CgGeometryFindResolvedType answers only the
 *         exact recorded source pointer and CgGeometryFindOperation
 *         only the recorded statement, both without touching the four
 *         backend temporary slots.
 */

static void TestFindLookups(void)
{
    CgGeometryProgram program;
    CgGeometryTypeView view;
    CgGeometryOperation operation;
    Type *unresolved;
    Type *resolved;
    expr_stmt statement;
    expr_stmt other;

    lViewArenaUsed = 0;
    unresolved = CgGetAttribArrayType(Float4Type, 0);
    resolved = CgGetAttribArrayType(Float4Type, 3);
    MakeGeometryProgram(&program, CGIR_STAGE_GEOMETRY);

    assert(CgGeometryFindResolvedType(&program, unresolved) == NULL);
    assert(CgGeometryFindOperation(&program, (stmt *) &statement) == NULL);

    memset(&view, 0, sizeof(view));
    view.sourceType = unresolved;
    view.resolvedType = resolved;
    view.next = NULL;
    program.typeViews = &view;

    InitExprStmt(&statement, NULL);
    InitExprStmt(&other, NULL);
    memset(&operation, 0, sizeof(operation));
    operation.kind = CG_GEOMETRY_OPERATION_EMIT;
    operation.statement = (stmt *) &statement;
    operation.next = NULL;
    program.operations = &operation;

    assert(CgGeometryFindResolvedType(&program, unresolved) == resolved);
    assert(CgGeometryFindResolvedType(&program, resolved) == NULL);
    assert(CgGeometryFindOperation(&program,
           (stmt *) &statement) == &operation);
    assert(CgGeometryFindOperation(&program, (stmt *) &other) == NULL);
}

/*
 * TestResolveBundleNestedDeclarationOrder() - The brief's synthetic
 *         nested bundle: leaves flatten recursively in source
 *         declaration order, each keeping its canonical semantic atom,
 *         its source spelling atom, its canonical leaf type, and the
 *         location of the declaration that bound the semantic.
 */

static void TestResolveBundleNestedDeclarationOrder(void)
{
    TestMemberSpec innerSpecs[2] = {
        { "pos",   NULL, "POSITION",  4 },
        { "color", NULL, "COLOR0",    5 }
    };
    TestMemberSpec outerSpecs[2] = {
        { "inner", NULL, NULL,        8 },
        { "uv",    NULL, "TEXCOORD0", 9 }
    };
    CgGeometryProgram program;
    CgGeometryDiagnostic diagnostic;
    CgGeometryValue *values;
    Type *inner;
    Type *outer;
    Type *float2;
    Symbol variable;
    symb varNode;
    struct geometry_arg_rec wrapper;
    expr *argument;
    binary argLink;
    const CgGeometryValue *leaf;

    innerSpecs[0].type = Float4Type;
    innerSpecs[1].type = Float4Type;
    outerSpecs[1].type = GetStandardTypeKind(CG_SCALAR_FLOAT, 2, 0);
    outerSpecs[0].type = MakeStructType(innerSpecs, 2);
    inner = outerSpecs[0].type;
    float2 = outerSpecs[1].type;
    outer = MakeStructType(outerSpecs, 2);

    lViewArenaUsed = 0;
    MakeGeometryProgram(&program, CGIR_STAGE_GEOMETRY);
    StackSymbol(&variable, outer, 11);
    InitOpSymb(&varNode, VARIABLE_OP, &variable, outer);
    wrapper.loc.file = 7;
    argument = WrapArgument(&wrapper, NULL, (expr *) &varNode, 0);
    argument = AppendArgument(&argLink, NULL, argument);
    wrapper.loc.line = 12;
    values = NULL;

    assert(CgGeometryResolveBundle(&program, CG_GEOMETRY_OPERATION_EMIT,
           argument, &values, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_NONE);
    assert(CountValues(values) == 3);

    leaf = ValueAt(values, 0);
    assert(!strcmp(GetAtomString(atable, leaf->canonicalSemantic),
                   "POSITION"));
    assert(leaf->sourceSemantic == TestAtom("POSITION"));
    assert(leaf->type == Float4Type);
    assert(leaf->loc.file == 3 && leaf->loc.line == 4);
    assert(leaf->value->common.kind == BINARY_N);
    assert(leaf->value->bin.op == MEMBER_SELECTOR_OP);

    leaf = ValueAt(values, 1);
    assert(!strcmp(GetAtomString(atable, leaf->canonicalSemantic),
                   "COLOR0"));
    assert(leaf->type == Float4Type);
    assert(leaf->loc.line == 5);

    /* The trailing TEXCOORD0 leaf comes from the outer struct's own
     * member order, after every nested inner-struct leaf. */
    leaf = ValueAt(values, 2);
    assert(!strcmp(GetAtomString(atable, leaf->canonicalSemantic),
                   "TEXCOORD0"));
    assert(leaf->type == float2);
    assert(leaf->loc.line == 9);

    /* Flattened member references were allocated through the program
     * arena: they are distinct nodes, never the caller's input tree. */
    assert(values->value != (expr *) &varNode);
    (void) inner;
}

/*
 * TestResolveBundleUnresolvedLeaf() - A leaf no rule can bind fails
 *         with the structured output-semantic reason at the argument's
 *         own anchor; arithmetic never inherits anything.
 */

static void TestResolveBundleUnresolvedLeaf(void)
{
    CgGeometryProgram program;
    CgGeometryDiagnostic diagnostic;
    CgGeometryValue *values;
    Symbol left;
    Symbol right;
    symb leftNode;
    symb rightNode;
    binary sum;
    struct geometry_arg_rec wrapper;
    binary argLink;
    expr *argument;

    lViewArenaUsed = 0;
    MakeGeometryProgram(&program, CGIR_STAGE_GEOMETRY);
    StackSymbol(&left, Float4Type, 4);
    StackSymbol(&right, Float4Type, 5);
    InitOpSymb(&leftNode, VARIABLE_OP, &left, Float4Type);
    InitOpSymb(&rightNode, VARIABLE_OP, &right, Float4Type);
    MakeBinaryNode(&sum, ADD_V_OP, (expr *) &leftNode,
                   (expr *) &rightNode, Float4Type);
    wrapper.loc.file = 7;
    argument = WrapArgument(&wrapper, NULL, (expr *) &sum, 0);
    argument = AppendArgument(&argLink, NULL, argument);
    wrapper.loc.line = 7;
    values = NULL;

    assert(!CgGeometryResolveBundle(&program,
           CG_GEOMETRY_OPERATION_EMIT, argument, &values, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_OUTPUT_SEMANTIC);
    assert(diagnostic.loc.line == 7);
    assert(values == NULL);
}

/*
 * TestResolveBundleInlineAnnotation() - An inline annotation wins over
 *     inheritance, keeps its source spelling atom untouched, and
 *     canonicalizes root case plus numeric suffix for equality only.
 */

static void TestResolveBundleInlineAnnotation(void)
{
    CgGeometryProgram program;
    CgGeometryDiagnostic diagnostic;
    CgGeometryValue *values;
    Symbol value;
    symb valueNode;
    binary argLink;
    struct geometry_arg_rec annotated;
    expr *argument;

    lViewArenaUsed = 0;
    MakeGeometryProgram(&program, CGIR_STAGE_GEOMETRY);
    StackSymbol(&value, Float4Type, 4);
    InitOpSymb(&valueNode, VARIABLE_OP, &value, Float4Type);
    argument = WrapArgument(&annotated, NULL, (expr *) &valueNode,
                            TestAtom("Color0"));
    argument = AppendArgument(&argLink, NULL, argument);
    annotated.loc.file = 7;
    annotated.loc.line = 6;
    values = NULL;

    assert(CgGeometryResolveBundle(&program, CG_GEOMETRY_OPERATION_EMIT,
           argument, &values, &diagnostic));
    assert(CountValues(values) == 1);
    assert(!strcmp(GetAtomString(atable, values->sourceSemantic),
                   "Color0"));
    assert(!strcmp(GetAtomString(atable, values->canonicalSemantic),
                   "COLOR0"));
    assert(values->loc.line == 6);
    assert(values->type == Float4Type);
}

/*
 * TestResolveBundleDuplicateSemantics() - Canonical equality rejects a
 *         second binding of the same output slot case-insensitively
 *         and across numeric-spelling aliases, naming the second
 *         occurrence's spelling and origin; a genuinely different
 *         suffix still passes.
 */

static void TestResolveBundleDuplicateSemantics(void)
{
    const char *secondSpellings[2];
    int ii;
    CgGeometryProgram program;
    CgGeometryDiagnostic diagnostic;
    CgGeometryValue *values;
    Symbol firstFormal;
    Symbol secondFormal;
    symb firstNode;
    symb secondNode;
    binary links[2];
    struct geometry_arg_rec wrappers[2];
    expr *argument;

    secondSpellings[0] = "color0";
    secondSpellings[1] = "COLOR00";
    for (ii = 0; ii < 2; ii++) {
        lViewArenaUsed = 0;
        MakeGeometryProgram(&program, CGIR_STAGE_GEOMETRY);
        StackSymbol(&firstFormal, Float4Type, 4);
        firstFormal.details.var.semantics = TestAtom("COLOR0");
        StackSymbol(&secondFormal, Float4Type, 5);
        InitOpSymb(&firstNode, VARIABLE_OP, &firstFormal, Float4Type);
        InitOpSymb(&secondNode, VARIABLE_OP, &secondFormal, Float4Type);
        wrappers[0].loc.file = 7;
        wrappers[0].loc.line = 6;
        wrappers[1].loc.file = 7;
        wrappers[1].loc.line = 7;
        argument = AppendArgument(&links[0], NULL,
                                  WrapArgument(&wrappers[0], NULL,
                                               (expr *) &firstNode, 0));
        argument = AppendArgument(&links[1], argument,
                                  WrapArgument(&wrappers[1], NULL,
                                               (expr *) &secondNode,
                                               TestAtom(secondSpellings[ii])));
        wrappers[0].loc.file = 7;
        wrappers[0].loc.line = 6;
        wrappers[1].loc.file = 7;
        wrappers[1].loc.line = 7;
        values = NULL;
        assert(!CgGeometryResolveBundle(&program,
               CG_GEOMETRY_OPERATION_EMIT, argument, &values,
               &diagnostic));
        if (diagnostic.reason !=
                CG_GEOMETRY_DIAGNOSTIC_DUPLICATE_SEMANTIC ||
            diagnostic.loc.line != 7 ||
            diagnostic.optionText == NULL ||
            strcmp(diagnostic.optionText, secondSpellings[ii]) != 0)
        {
            fprintf(stdout, "FAIL ii=%d reason=%d line=%d text=%s\n",
                    ii, (int) diagnostic.reason, diagnostic.loc.line,
                    diagnostic.optionText ?
                    diagnostic.optionText : "(null)");
            fflush(stdout);
            assert(0);
        }
        /* A failed resolution leaves the output list untouched. */
        assert(values == NULL);
    }

    /* Control: COLOR1 against COLOR0 is not a duplicate. */
    lViewArenaUsed = 0;
    MakeGeometryProgram(&program, CGIR_STAGE_GEOMETRY);
    StackSymbol(&firstFormal, Float4Type, 4);
    firstFormal.details.var.semantics = TestAtom("COLOR0");
    StackSymbol(&secondFormal, Float4Type, 5);
    secondFormal.details.var.semantics = TestAtom("COLOR1");
    InitOpSymb(&firstNode, VARIABLE_OP, &firstFormal, Float4Type);
    InitOpSymb(&secondNode, VARIABLE_OP, &secondFormal, Float4Type);
    argument = AppendArgument(&links[0], NULL,
                              WrapArgument(&wrappers[0], NULL,
                                           (expr *) &firstNode, 0));
    argument = AppendArgument(&links[1], argument,
                              WrapArgument(&wrappers[1], NULL,
                                           (expr *) &secondNode, 0));
    wrappers[0].loc.file = 7;
    wrappers[0].loc.line = 6;
    wrappers[1].loc.file = 7;
    wrappers[1].loc.line = 7;
    values = NULL;
    assert(CgGeometryResolveBundle(&program,
           CG_GEOMETRY_OPERATION_EMIT, argument, &values, &diagnostic));
    assert(CountValues(values) == 2);
}

/*
 * TestResolveBundleAttribIndexInheritance() - Direct indexing of an
 *         attribute-array parameter inherits the parameter's binding:
 *         the leaf keeps the element type and the declaration site,
 *         and flatAttrib rejects the inherited POSITION canonically.
 */

static void TestResolveBundleAttribIndexInheritance(void)
{
    CgGeometryProgram program;
    CgGeometryDiagnostic diagnostic;
    CgGeometryValue *values;
    Type *attribArray;
    Symbol formalP;
    symb pNode;
    symb indexNode;
    binary argLink;
    binary index;
    struct geometry_arg_rec wrapper;
    expr *argument;
    constant subscript;

    lViewArenaUsed = 0;
    attribArray = CgGetAttribArrayType(Float4Type, 0);
    MakeGeometryProgram(&program, CGIR_STAGE_GEOMETRY);
    StackSymbol(&formalP, attribArray, 3);
    formalP.details.var.semantics = TestAtom("POSITION");
    InitOpSymb(&pNode, VARIABLE_OP, &formalP, attribArray);
    memset(&subscript, 0, sizeof(subscript));
    subscript.kind = CONST_N;
    subscript.op = ICONST_OP;
    subscript.type = IntType;
    MakeBinaryNode(&index, ARRAY_INDEX_OP, (expr *) &pNode,
                   (expr *) &subscript, Float4Type);
    argument = WrapArgument(&wrapper, NULL, (expr *) &index, 0);
    argument = AppendArgument(&argLink, NULL, argument);
    wrapper.loc.file = 7;
    wrapper.loc.line = 6;
    values = NULL;

    /* emitVertex(p[0]) inherits POSITION with the element type. */
    assert(CgGeometryResolveBundle(&program, CG_GEOMETRY_OPERATION_EMIT,
           argument, &values, &diagnostic));
    assert(CountValues(values) == 1);
    assert(values->canonicalSemantic == TestAtom("POSITION"));
    assert(values->type == Float4Type);
    assert(values->loc.line == 3);

    /* flatAttrib(p[0]) rejects the inherited POSITION. */
    values = NULL;
    assert(!CgGeometryResolveBundle(&program,
           CG_GEOMETRY_OPERATION_FLAT, argument, &values, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_FLAT_POSITION);
    assert(diagnostic.loc.line == 3);
    assert(values == NULL);
}

/*
 * TestResolveBundleIllegalValueType() - A semantically bound leaf of a
 *     non-output type (a sampler here) fails with its own structured
 *     reason instead of flowing into records.
 */

static void TestResolveBundleIllegalValueType(void)
{
    CgGeometryProgram program;
    CgGeometryDiagnostic diagnostic;
    CgGeometryValue *values;
    Type *sampler2D;
    binary argLink;
    Symbol bad;
    symb badNode;
    struct geometry_arg_rec wrapper;
    expr *argument;

    lViewArenaUsed = 0;
    sampler2D = GetSamplerType(CG_SAMPLER_2D);
    MakeGeometryProgram(&program, CGIR_STAGE_GEOMETRY);
    StackSymbol(&bad, sampler2D, 4);
    bad.details.var.semantics = TestAtom("COLOR0");
    InitOpSymb(&badNode, VARIABLE_OP, &bad, sampler2D);
    values = NULL;
    argument = WrapArgument(&wrapper, NULL, (expr *) &badNode, 0);
    argument = AppendArgument(&argLink, NULL, argument);
    wrapper.loc.file = 7;
    wrapper.loc.line = 6;

    assert(!CgGeometryResolveBundle(&program,
           CG_GEOMETRY_OPERATION_EMIT, argument, &values, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_VALUE_TYPE);
    /* An inherited binding anchors the type problem at its
     * declaration, not at the use site. */
    assert(diagnostic.loc.line == 4);
    assert(values == NULL);
}

/*
 * TestResolveBundleEmptyArguments() - An empty bundle is an arity
 *         failure even when reached directly through the resolver.
 */

static void TestResolveBundleEmptyArguments(void)
{
    CgGeometryProgram program;
    CgGeometryDiagnostic diagnostic;
    CgGeometryValue *values;

    MakeGeometryProgram(&program, CGIR_STAGE_GEOMETRY);
    values = NULL;
    assert(!CgGeometryResolveBundle(&program,
           CG_GEOMETRY_OPERATION_EMIT, NULL, &values, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_OPERATION_ARITY);
    assert(values == NULL);
}

/*
 * TestClassifyBuildsRecords() - Complete-statement special calls
 *         classify by intrinsic identity: emit/flat build ordered
 *         operation records with resolved bundles, restartStrip builds
 *         an empty record, and lookups answer by statement pointer.
 */

static void TestClassifyBuildsRecords(void)
{
    CgGeometryProgram program;
    CgGeometryDiagnostic diagnostic;
    Type *attribArray;
    Symbol formalP;
    Symbol calleeEmit;
    Symbol calleeFlat;
    Symbol calleeRestart;
    symb pNode;
    symb calleeNodes[3];
    constant subscript;
    binary index;
    binary calls[3];
    binary links[2];
    struct geometry_arg_rec wrappers[2];
    expr_stmt firstStatement;
    expr_stmt secondStatement;
    expr_stmt thirdStatement;
    expr *flatArgs;
    expr *emitArgs;
    const CgGeometryOperation *operation;

    lViewArenaUsed = 0;
    attribArray = CgGetAttribArrayType(Float4Type, 0);
    MakeGeometryProgram(&program, CGIR_STAGE_GEOMETRY);
    StackSymbol(&formalP, attribArray, 3);
    formalP.details.var.semantics = TestAtom("POSITION");

    /* flatAttrib(p[0] : TEXCOORD0); emitVertex(p[0] : POSITION);
     * restartStrip(); */
    InitOpSymb(&pNode, VARIABLE_OP, &formalP, attribArray);
    memset(&subscript, 0, sizeof(subscript));
    subscript.kind = CONST_N;
    subscript.op = ICONST_OP;
    subscript.type = IntType;
    MakeBinaryNode(&index, ARRAY_INDEX_OP, (expr *) &pNode,
                   (expr *) &subscript, Float4Type);
    flatArgs = AppendArgument(&links[0], NULL,
                              WrapArgument(&wrappers[0], NULL,
                                           (expr *) &index,
                                           TestAtom("TEXCOORD0")));
    wrappers[0].loc.file = 7;
    wrappers[0].loc.line = 4;
    InitExprStmt(&firstStatement,
                 MakeSpecialCall(&calls[0], &calleeNodes[0], &calleeFlat,
                                 &lFlatAttribSignature, flatArgs));
    firstStatement.loc.file = 7;
    firstStatement.loc.line = 4;

    /* emitVertex(p[0] : POSITION); restartStrip(); */
    MakeBinaryNode(&index, ARRAY_INDEX_OP, (expr *) &pNode,
                   (expr *) &subscript, Float4Type);
    emitArgs = AppendArgument(&links[1], NULL,
                              WrapArgument(&wrappers[1], NULL,
                                           (expr *) &index,
                                           TestAtom("POSITION")));
    wrappers[1].loc.file = 7;
    wrappers[1].loc.line = 5;
    InitExprStmt(&secondStatement,
                 MakeSpecialCall(&calls[1], &calleeNodes[1], &calleeEmit,
                                 &lEmitVertexSignature, emitArgs));
    secondStatement.loc.file = 7;
    secondStatement.loc.line = 5;
    firstStatement.next = (stmt *) &secondStatement;

    InitExprStmt(&thirdStatement,
                 MakeSpecialCall(&calls[2], &calleeNodes[2], &calleeRestart,
                                 &lRestartStripSignature, NULL));
    thirdStatement.loc.file = 7;
    thirdStatement.loc.line = 6;
    secondStatement.next = (stmt *) &thirdStatement;
    thirdStatement.next = NULL;

    if (!CgGeometryClassifyOperationStatement(&program,
            (stmt *) &firstStatement, &diagnostic)) {
            assert(0);
    }
    assert(CgGeometryClassifyOperationStatement(&program,
           (stmt *) &secondStatement, &diagnostic));
    assert(CgGeometryClassifyOperationStatement(&program,
           (stmt *) &thirdStatement, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_NONE);

    operation = CgGeometryFindOperation(&program, (stmt *) &firstStatement);
    assert(operation && operation->kind == CG_GEOMETRY_OPERATION_FLAT);
    assert(CountValues(operation->values) == 1);
    assert(operation->loc.line == 4);
    assert(operation->next &&
           operation->next->kind == CG_GEOMETRY_OPERATION_EMIT);
    assert(CountValues(operation->next->values) == 1);
    assert(operation->next->next &&
           operation->next->next->kind == CG_GEOMETRY_OPERATION_RESTART);
    assert(operation->next->next->values == NULL);
    assert(operation->next->next->next == NULL);
    assert(CgGeometryClassifyOperationStatement(NULL,
           (stmt *) &firstStatement, &diagnostic));
}

/*
 * TestClassifyRejectsBadArity() - emitVertex/flatAttrib require at
 *         least one argument and restartStrip none, whatever dead-code
 *         context holds them.
 */

static void TestClassifyRejectsBadArity(void)
{
    CgGeometryProgram program;
    CgGeometryDiagnostic diagnostic;
    Symbol calleeEmit;
    Symbol calleeRestart;
    Symbol formalP;
    symb pNode;
    symb calleeNodes[2];
    binary call;
    binary link;
    struct geometry_arg_rec wrapper;
    expr_stmt statement;
    expr *args;

    lViewArenaUsed = 0;
    MakeGeometryProgram(&program, CGIR_STAGE_GEOMETRY);
    StackSymbol(&formalP, Float4Type, 3);

    /* restartStrip(p); */
    InitOpSymb(&pNode, VARIABLE_OP, &formalP, Float4Type);
    wrapper.loc.file = 7;
    wrapper.loc.line = 4;
    args = AppendArgument(&link, NULL,
                          WrapArgument(&wrapper, NULL,
                                       (expr *) &pNode, 0));
    InitExprStmt(&statement,
                 MakeSpecialCall(&call, &calleeNodes[0], &calleeRestart,
                                 &lRestartStripSignature, args));
    statement.loc.file = 7;
    statement.loc.line = 4;
    assert(!CgGeometryClassifyOperationStatement(&program,
           (stmt *) &statement, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_OPERATION_ARITY);
    assert(diagnostic.loc.line == 4);
    assert(!strcmp(diagnostic.optionText, "restartStrip"));
    assert(program.operations == NULL);

    /* emitVertex(); */
    InitExprStmt(&statement,
                 MakeSpecialCall(&call, &calleeNodes[1], &calleeEmit,
                                 &lEmitVertexSignature, NULL));
    statement.loc.file = 7;
    statement.loc.line = 5;
    assert(!CgGeometryClassifyOperationStatement(&program,
           (stmt *) &statement, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_OPERATION_ARITY);
    assert(!strcmp(diagnostic.optionText, "emitVertex"));
    assert(program.operations == NULL);
}

/*
 * TestClassifyRejectsNestedContexts() - A special call anywhere except
 *         the complete expression of an expression statement fails
 *         with the context reason naming the operation, including
 *         inside conditions and returns; ordinary statements still
 *         classify clean.
 */

static void TestClassifyRejectsNestedContexts(void)
{
    CgGeometryProgram program;
    CgGeometryDiagnostic diagnostic;
    Symbol calleeEmit;
    Symbol formalP;
    Symbol boolVar;
    symb pNode;
    symb boolNode;
    symb calleeNode;
    binary call;
    binary assign;
    binary add;
    binary link;
    trinary conditional;
    struct geometry_arg_rec wrapper;
    expr_stmt statement;
    if_stmt ifStatement;
    return_stmt returnStatement;
    expr *args;
    int ii;

    lViewArenaUsed = 0;
    MakeGeometryProgram(&program, CGIR_STAGE_GEOMETRY);
    StackSymbol(&formalP, Float4Type, 3);
    formalP.details.var.semantics = TestAtom("POSITION");
    StackSymbol(&boolVar, BooleanType, 4);
    InitOpSymb(&pNode, VARIABLE_OP, &formalP, Float4Type);
    InitOpSymb(&boolNode, VARIABLE_OP, &boolVar, BooleanType);
    wrapper.loc.file = 7;
    wrapper.loc.line = 5;
    args = AppendArgument(&link, NULL,
                          WrapArgument(&wrapper, NULL,
                                       (expr *) &pNode, 0));
    MakeSpecialCall(&call, &calleeNode, &calleeEmit,
                    &lEmitVertexSignature, args);

    for (ii = 0; ii < 4; ii++) {
        switch (ii) {
        case 0:
            /* assignment nesting: p = emitVertex(p); shaped generically */
            InitExprStmt(&statement,
                         MakeBinaryNode(&assign, ASSIGN_GEN_OP,
                                        (expr *) &boolNode,
                                        (expr *) &call, VoidType));
            break;
        case 1:
            /* arithmetic nesting */
            InitExprStmt(&statement,
                         MakeBinaryNode(&add, ADD_V_OP, (expr *) &pNode,
                                        (expr *) &call, Float4Type));
            break;
        case 2:
            /* conditional nesting */
            InitExprStmt(&statement,
                         MakeTrinaryNode(&conditional, COND_GEN_OP,
                                         (expr *) &boolNode,
                                         (expr *) &call,
                                         (expr *) &pNode, Float4Type));
            break;
        default:
            InitExprStmt(&statement, (expr *) &call);
            break;
        }
        statement.loc.file = 7;
        statement.loc.line = 5 + ii;
        if (ii < 3) {
            assert(!CgGeometryClassifyOperationStatement(&program,
                   (stmt *) &statement, &diagnostic));
            assert(diagnostic.reason ==
                   CG_GEOMETRY_DIAGNOSTIC_OPERATION_CONTEXT);
            assert(diagnostic.loc.line == 5 + ii);
            assert(!strcmp(diagnostic.optionText, "emitVertex"));
            assert(program.operations == NULL);
        } else {
            /* The bare complete statement classifies clean. */
            assert(CgGeometryClassifyOperationStatement(&program,
                   (stmt *) &statement, &diagnostic));
            assert(program.operations != NULL);
            program.operations = NULL;
        }
    }

    /* Condition nesting: if (emitVertex(p)) ; */
    memset(&ifStatement, 0, sizeof(ifStatement));
    ifStatement.kind = IF_STMT;
    ifStatement.cond = (expr *) &call;
    ifStatement.thenstmt = NULL;
    ifStatement.elsestmt = NULL;
    ifStatement.loc.file = 7;
    ifStatement.loc.line = 12;
    assert(!CgGeometryClassifyOperationStatement(&program,
           (stmt *) &ifStatement, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_OPERATION_CONTEXT);
    assert(diagnostic.loc.line == 12);

    /* Return nesting: return emitVertex(p); */
    memset(&returnStatement, 0, sizeof(returnStatement));
    returnStatement.kind = RETURN_STMT;
    returnStatement.exp = (expr *) &call;
    returnStatement.loc.file = 7;
    returnStatement.loc.line = 13;
    assert(!CgGeometryClassifyOperationStatement(&program,
           (stmt *) &returnStatement, &diagnostic));
    assert(diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_OPERATION_CONTEXT);
    assert(diagnostic.loc.line == 13);
}

int main(void)
{
    CgStruct cg;
    slHAL hal;

    memset(&cg, 0, sizeof(cg));
    memset(&hal, 0, sizeof(hal));
    hal.GetSizeof = TestGetSizeof;
    hal.GetAlignment = TestGetAlignment;
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
    TestFindLookups();
    TestResolveBundleNestedDeclarationOrder();
    TestResolveBundleUnresolvedLeaf();
    TestResolveBundleInlineAnnotation();
    TestResolveBundleDuplicateSemantics();
    TestResolveBundleAttribIndexInheritance();
    TestResolveBundleIllegalValueType();
    TestResolveBundleEmptyArguments();
    TestClassifyBuildsRecords();
    TestClassifyRejectsBadArity();
    TestClassifyRejectsNestedContexts();
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
