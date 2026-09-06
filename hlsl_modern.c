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
// hlsl_modern.c
//

#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "hlsl_modern.h"

#define HLSL_MODERN_VERTEX   0x01
#define HLSL_MODERN_PIXEL    0x02
#define HLSL_MODERN_GEOMETRY 0x04
#define HLSL_MODERN_INPUT    0x10
#define HLSL_MODERN_OUTPUT   0x20

typedef struct HlslModernSemanticDesc_Rec {
    const char *root;
    HlslSemanticKind semantic;
    int stages;
    int directions;
    int firstIndex;
    int count;
} HlslModernSemanticDesc;

static const HlslModernSemanticDesc modernSemantics[] = {
    { "POSITION", HLSL_SEMANTIC_SV_POSITION,
      HLSL_MODERN_VERTEX | HLSL_MODERN_GEOMETRY,
      HLSL_MODERN_OUTPUT, 0, 1 },
    { "POSITION", HLSL_SEMANTIC_SV_POSITION,
      HLSL_MODERN_GEOMETRY, HLSL_MODERN_INPUT, 0, 1 },
    { "POSITION", HLSL_SEMANTIC_SV_POSITION,
      HLSL_MODERN_PIXEL, HLSL_MODERN_INPUT, 0, 1 },
    { "HPOS", HLSL_SEMANTIC_SV_POSITION,
      HLSL_MODERN_VERTEX | HLSL_MODERN_GEOMETRY,
      HLSL_MODERN_OUTPUT, 0, 1 },
    { "WPOS", HLSL_SEMANTIC_SV_POSITION,
      HLSL_MODERN_PIXEL, HLSL_MODERN_INPUT, 0, 1 },
    { "COLOR", HLSL_SEMANTIC_SV_TARGET,
      HLSL_MODERN_PIXEL, HLSL_MODERN_OUTPUT, 0, 8 },
    { "DEPTH", HLSL_SEMANTIC_SV_DEPTH,
      HLSL_MODERN_PIXEL, HLSL_MODERN_OUTPUT, 0, 1 },
    { "VERTEXID", HLSL_SEMANTIC_SV_VERTEX_ID,
      HLSL_MODERN_VERTEX, HLSL_MODERN_INPUT, 0, 1 },
    { "INSTANCEID", HLSL_SEMANTIC_SV_INSTANCE_ID,
      HLSL_MODERN_VERTEX, HLSL_MODERN_INPUT, 0, 1 },
    { "INSTANCEID", HLSL_SEMANTIC_SV_PRIMITIVE_ID,
      HLSL_MODERN_GEOMETRY, HLSL_MODERN_INPUT, 0, 1 },
    { "PRIMITIVEID", HLSL_SEMANTIC_SV_PRIMITIVE_ID,
      HLSL_MODERN_GEOMETRY, HLSL_MODERN_INPUT | HLSL_MODERN_OUTPUT, 0, 1 },
    { "LAYER", HLSL_SEMANTIC_SV_RT_ARRAY_INDEX,
      HLSL_MODERN_GEOMETRY, HLSL_MODERN_OUTPUT, 0, 1 },
    { "FACE", HLSL_SEMANTIC_SV_IS_FRONT_FACE,
      HLSL_MODERN_PIXEL, HLSL_MODERN_INPUT, 0, 1 },
    { "CLP", HLSL_SEMANTIC_SV_CLIP_DISTANCE,
      HLSL_MODERN_VERTEX | HLSL_MODERN_GEOMETRY,
      HLSL_MODERN_OUTPUT, 0, 2 },
    { "PSIZE", HLSL_SEMANTIC_UNSUPPORTED,
      HLSL_MODERN_VERTEX | HLSL_MODERN_GEOMETRY,
      HLSL_MODERN_OUTPUT, 0, 1 }
};

static int HlslModernStageFlag(HlslStage stage)
{
    if (stage == HLSL_STAGE_VERTEX)
        return HLSL_MODERN_VERTEX;
    if (stage == HLSL_STAGE_PIXEL)
        return HLSL_MODERN_PIXEL;
    if (stage == HLSL_STAGE_GEOMETRY)
        return HLSL_MODERN_GEOMETRY;
    return 0;
} // HlslModernStageFlag

static int HlslModernStringsEqual(const char *left, const char *right)
{
    char leftChar;
    char rightChar;

    if (left == NULL || right == NULL)
        return 0;
    while (*left != '\0' && *right != '\0') {
        leftChar = *left++;
        rightChar = *right++;
        if (leftChar >= 'a' && leftChar <= 'z')
            leftChar = (char) (leftChar - 'a' + 'A');
        if (rightChar >= 'a' && rightChar <= 'z')
            rightChar = (char) (rightChar - 'a' + 'A');
        if (leftChar != rightChar)
            return 0;
    }
    return *left == *right;
} // HlslModernStringsEqual

HlslSemanticKind HlslModernSemantic(HlslStage stage,
    HlslDirection direction, const char *root, int index)
{
    int stageFlag;
    int directionFlag;
    int i;

    if (root == NULL || index < 0)
        return HLSL_SEMANTIC_UNSUPPORTED;
    stageFlag = HlslModernStageFlag(stage);
    directionFlag = direction == HLSL_DIRECTION_OUTPUT ?
                    HLSL_MODERN_OUTPUT : HLSL_MODERN_INPUT;
    for (i = 0; i < (int) (sizeof(modernSemantics) /
                            sizeof(modernSemantics[0])); i++)
    {
        if (HlslModernStringsEqual(root, modernSemantics[i].root) &&
            (modernSemantics[i].stages & stageFlag) != 0 &&
            (modernSemantics[i].directions & directionFlag) != 0 &&
            index >= modernSemantics[i].firstIndex &&
            index < modernSemantics[i].firstIndex + modernSemantics[i].count)
        {
            return modernSemantics[i].semantic;
        }
    }
    return HLSL_SEMANTIC_USER;
} // HlslModernSemantic

int HlslModernSemanticsConflict(HlslStage stage, HlslDirection direction,
    const char *leftRoot, int leftIndex,
    const char *rightRoot, int rightIndex)
{
    HlslSemanticKind left;
    HlslSemanticKind right;

    left = HlslModernSemantic(stage, direction, leftRoot, leftIndex);
    right = HlslModernSemantic(stage, direction, rightRoot, rightIndex);
    if (left == HLSL_SEMANTIC_USER ||
        left == HLSL_SEMANTIC_UNSUPPORTED || left != right)
    {
        return 0;
    }
    return leftIndex == rightIndex;
} // HlslModernSemanticsConflict

HlslBase HlslModernAbiBase(HlslSemanticKind semantic)
{
    switch (semantic) {
    case HLSL_SEMANTIC_SV_VERTEX_ID:
    case HLSL_SEMANTIC_SV_INSTANCE_ID:
    case HLSL_SEMANTIC_SV_PRIMITIVE_ID:
    case HLSL_SEMANTIC_SV_RT_ARRAY_INDEX:
        return HLSL_BASE_UINT;
    case HLSL_SEMANTIC_SV_IS_FRONT_FACE:
        return HLSL_BASE_BOOL;
    default:
        return HLSL_BASE_FLOAT;
    }
} // HlslModernAbiBase

int HlslModernSemanticIsLegal(HlslStage stage, HlslDirection direction,
                              HlslSemanticKind semantic)
{
    int stageFlag;
    int directionFlag;
    int i;

    if (semantic == HLSL_SEMANTIC_USER)
        return 1;
    if (semantic == HLSL_SEMANTIC_UNSUPPORTED)
        return 0;
    stageFlag = HlslModernStageFlag(stage);
    directionFlag = direction == HLSL_DIRECTION_OUTPUT ?
                    HLSL_MODERN_OUTPUT : HLSL_MODERN_INPUT;
    for (i = 0; i < (int) (sizeof(modernSemantics) /
                            sizeof(modernSemantics[0])); i++)
    {
        if (modernSemantics[i].semantic == semantic &&
            (modernSemantics[i].stages & stageFlag) != 0 &&
            (modernSemantics[i].directions & directionFlag) != 0)
        {
            return 1;
        }
    }
    return 0;
} // HlslModernSemanticIsLegal

int HlslModernSemanticSpelling(HlslSemanticKind semantic, int index,
                               char *text, size_t size)
{
    char spelling[96];
    const char *root;
    int usesIndex;
    int written;

    root = NULL;
    usesIndex = 0;
    switch (semantic) {
    case HLSL_SEMANTIC_SV_POSITION: root = "SV_Position"; break;
    case HLSL_SEMANTIC_SV_TARGET:
        root = "SV_Target";
        usesIndex = 1;
        break;
    case HLSL_SEMANTIC_SV_DEPTH: root = "SV_Depth"; break;
    case HLSL_SEMANTIC_SV_VERTEX_ID: root = "SV_VertexID"; break;
    case HLSL_SEMANTIC_SV_INSTANCE_ID: root = "SV_InstanceID"; break;
    case HLSL_SEMANTIC_SV_PRIMITIVE_ID: root = "SV_PrimitiveID"; break;
    case HLSL_SEMANTIC_SV_RT_ARRAY_INDEX:
        root = "SV_RenderTargetArrayIndex";
        break;
    case HLSL_SEMANTIC_SV_IS_FRONT_FACE: root = "SV_IsFrontFace"; break;
    case HLSL_SEMANTIC_SV_CLIP_DISTANCE:
        root = "SV_ClipDistance";
        usesIndex = 1;
        break;
    default:
        return 0;
    }
    if (text == NULL || size == 0 || index < 0)
        return 0;
    written = usesIndex ? sprintf(spelling, "%s%d", root, index) :
                          sprintf(spelling, "%s", root);
    if (written < 0 || (size_t) written >= size)
        return 0;
    memcpy(text, spelling, (size_t) written + 1);
    return 1;
} // HlslModernSemanticSpelling

HlslInterpolation HlslModernRequiredInterpolation(int sourceBase)
{
    return sourceBase == TYPE_BASE_INT || sourceBase == TYPE_BASE_CINT ?
           HLSL_INTERPOLATION_NOINTERPOLATION :
           HLSL_INTERPOLATION_DEFAULT;
} // HlslModernRequiredInterpolation

const char *HlslInterpolationName(HlslInterpolation interpolation)
{
    switch (interpolation) {
    case HLSL_INTERPOLATION_DEFAULT: return "default";
    case HLSL_INTERPOLATION_LINEAR: return "linear";
    case HLSL_INTERPOLATION_CENTROID: return "centroid";
    case HLSL_INTERPOLATION_NOPERSPECTIVE: return "noperspective";
    case HLSL_INTERPOLATION_NOINTERPOLATION: return "nointerpolation";
    }
    return NULL;
} // HlslInterpolationName
