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

int main(void)
{
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
    return 0;
}
