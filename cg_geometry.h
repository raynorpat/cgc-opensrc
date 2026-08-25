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
    CG_GEOMETRY_DIAGNOSTIC_ALLOCATION
} CgGeometryDiagnosticReason;

typedef struct CgGeometryDiagnostic_Rec {
    CgGeometryDiagnosticReason reason;
    SourceLoc loc;
    int optionOrdinal;
    const char *optionText;
} CgGeometryDiagnostic;

void CgGeometryInitModifiers(CgGeometryModifiers *modifiers);
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
