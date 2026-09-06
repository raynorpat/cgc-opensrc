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
#include <limits.h>
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

typedef struct HlslModernTypeFrame_Rec {
    const struct HlslModernTypeFrame_Rec *parent;
    const HlslType *type;
} HlslModernTypeFrame;

static int HlslModernTypeFrameContains(const HlslModernTypeFrame *frame,
                                       const HlslType *type)
{
    for (; frame != NULL; frame = frame->parent) {
        if (frame->type == type)
            return 1;
    }
    return 0;
} // HlslModernTypeFrameContains

static int HlslModernDeclListHasCycle(const HlslDecl *list)
{
    const HlslDecl *slow;
    const HlslDecl *fast;

    slow = list;
    fast = list;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            return 1;
    }
    return 0;
} // HlslModernDeclListHasCycle

static int HlslModernRoundPackCursor(HlslModernPackCursor *cursor)
{
    if (cursor->component == 0)
        return 1;
    if (cursor->vector == INT_MAX)
        return 0;
    cursor->vector++;
    cursor->component = 0;
    return 1;
} // HlslModernRoundPackCursor

static int HlslModernAdvanceVector(HlslModernPackCursor *cursor)
{
    if (cursor->vector == INT_MAX)
        return 0;
    cursor->vector++;
    cursor->component = 0;
    return 1;
} // HlslModernAdvanceVector

static int HlslModernLogicalComponentCountInner(const HlslType *type,
    const HlslModernTypeFrame *parent, int depth, int *result)
{
    HlslModernTypeFrame frame;
    const HlslDecl *member;
    int count;
    int memberCount;

    if (type == NULL || result == NULL || depth > 128 ||
        type->arraySize < 0 || HlslModernTypeFrameContains(parent, type))
    {
        return 0;
    }
    frame.parent = parent;
    frame.type = type;
    if (type->arraySize > 0) {
        if (!HlslModernLogicalComponentCountInner(type->elementType, &frame,
                                                  depth + 1, &count) ||
            count <= 0 || type->arraySize > INT_MAX / count)
        {
            return 0;
        }
        *result = type->arraySize * count;
        return 1;
    }
    if (type->elementType != NULL)
        return 0;
    if (type->base == HLSL_BASE_STRUCT) {
        if (type->members == NULL ||
            HlslModernDeclListHasCycle(type->members))
        {
            return 0;
        }
        count = 0;
        for (member = type->members; member != NULL; member = member->next) {
            if (!HlslModernLogicalComponentCountInner(&member->type, &frame,
                    depth + 1, &memberCount) || memberCount <= 0 ||
                count > INT_MAX - memberCount)
            {
                return 0;
            }
            count += memberCount;
        }
        if (count <= 0)
            return 0;
        *result = count;
        return 1;
    }
    if (type->members != NULL || type->structName != NULL)
        return 0;
    if (type->rows > 0 || type->cols > 0) {
        if (type->base != HLSL_BASE_FLOAT || type->rows < 1 ||
            type->rows > 4 || type->cols < 1 || type->cols > 4 ||
            type->len != 0 || type->rows > INT_MAX / type->cols)
        {
            return 0;
        }
        *result = type->rows * type->cols;
        return 1;
    }
    if ((type->base != HLSL_BASE_FLOAT && type->base != HLSL_BASE_INT &&
         type->base != HLSL_BASE_UINT && type->base != HLSL_BASE_BOOL) ||
        type->len < 1 || type->len > 4)
    {
        return 0;
    }
    *result = type->len;
    return 1;
} // HlslModernLogicalComponentCountInner

int HlslModernLogicalComponentCount(const HlslType *type, int *count)
{
    int result;

    if (count == NULL ||
        !HlslModernLogicalComponentCountInner(type, NULL, 0, &result))
    {
        return 0;
    }
    *count = result;
    return 1;
} // HlslModernLogicalComponentCount

static int HlslModernPackTypeInner(const HlslType *type,
    HlslModernPackCursor *cursor, const HlslModernTypeFrame *parent,
    int depth)
{
    HlslModernPackCursor elementCursor;
    HlslModernTypeFrame frame;
    const HlslDecl *member;
    int elementVectors;
    int vectors;

    if (type == NULL || cursor == NULL || depth > 128 ||
        cursor->vector < 0 || cursor->component < 0 ||
        cursor->component > 3 || type->arraySize < 0 ||
        HlslModernTypeFrameContains(parent, type))
    {
        return 0;
    }
    frame.parent = parent;
    frame.type = type;
    if (type->arraySize > 0) {
        if (type->elementType == NULL || !HlslModernRoundPackCursor(cursor))
            return 0;
        elementCursor.vector = 0;
        elementCursor.component = 0;
        if (!HlslModernPackTypeInner(type->elementType, &elementCursor,
                                     &frame, depth + 1) ||
            !HlslModernRoundPackCursor(&elementCursor))
        {
            return 0;
        }
        elementVectors = elementCursor.vector;
        if (elementVectors <= 0 ||
            type->arraySize > INT_MAX / elementVectors)
        {
            return 0;
        }
        vectors = type->arraySize * elementVectors;
        if (cursor->vector > INT_MAX - vectors)
            return 0;
        cursor->vector += vectors;
        return 1;
    }
    if (type->elementType != NULL)
        return 0;
    if (type->base == HLSL_BASE_STRUCT) {
        if (type->members == NULL ||
            HlslModernDeclListHasCycle(type->members) ||
            !HlslModernRoundPackCursor(cursor))
        {
            return 0;
        }
        for (member = type->members; member != NULL; member = member->next) {
            if (!HlslModernPackTypeInner(&member->type, cursor, &frame,
                                         depth + 1))
            {
                return 0;
            }
        }
        return HlslModernRoundPackCursor(cursor);
    }
    if (type->rows > 0 || type->cols > 0) {
        if (type->base != HLSL_BASE_FLOAT || type->rows < 1 ||
            type->rows > 4 || type->cols < 1 || type->cols > 4 ||
            type->len != 0 || type->members != NULL ||
            type->structName != NULL ||
            !HlslModernRoundPackCursor(cursor) ||
            cursor->vector > INT_MAX - type->rows)
        {
            return 0;
        }
        cursor->vector += type->rows;
        return 1;
    }
    if ((type->base != HLSL_BASE_FLOAT && type->base != HLSL_BASE_INT &&
         type->base != HLSL_BASE_UINT && type->base != HLSL_BASE_BOOL) ||
        type->len < 1 || type->len > 4 || type->structName != NULL ||
        type->elementType != NULL || type->members != NULL)
    {
        return 0;
    }
    if (type->len > 4 - cursor->component &&
        !HlslModernRoundPackCursor(cursor))
    {
        return 0;
    }
    cursor->component += type->len;
    if (cursor->component == 4)
        return HlslModernAdvanceVector(cursor);
    return 1;
} // HlslModernPackTypeInner

int HlslModernPackType(const HlslType *type, HlslModernPackCursor *cursor,
                       HlslPackOffset *offset, int *vectorSpan)
{
    HlslModernPackCursor work;
    HlslModernPackCursor start;
    int aggregate;
    int endComponent;
    int span;

    if (type == NULL || cursor == NULL || offset == NULL ||
        vectorSpan == NULL || cursor->vector < 0 ||
        cursor->component < 0 || cursor->component > 3)
    {
        return 0;
    }
    work = *cursor;
    aggregate = type->arraySize > 0 || type->base == HLSL_BASE_STRUCT ||
                type->rows > 0 || type->cols > 0;
    if ((aggregate ||
         (type->len >= 1 && type->len <= 4 &&
          type->len > 4 - work.component)) &&
        !HlslModernRoundPackCursor(&work))
    {
        return 0;
    }
    start = work;
    if (!HlslModernPackTypeInner(type, &work, NULL, 0))
        return 0;
    if (work.vector < start.vector)
        return 0;
    endComponent = work.component;
    span = work.vector - start.vector + (endComponent != 0);
    if (span <= 0 || start.vector > INT_MAX / 4 ||
        work.vector > INT_MAX / 4 ||
        (aggregate && work.component != 0))
    {
        return 0;
    }
    offset->vector = start.vector;
    offset->component = start.component;
    offset->componentCount = aggregate ? span * 4 : type->len;
    *vectorSpan = span;
    *cursor = work;
    return 1;
} // HlslModernPackType

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

static int HlslModernHasReservedPrefix(const char *root)
{
    if (root == NULL)
        return 0;
    return (root[0] == 'S' || root[0] == 's') &&
           (root[1] == 'V' || root[1] == 'v') && root[2] == '_';
} // HlslModernHasReservedPrefix

HlslSemanticKind HlslModernSemantic(HlslStage stage,
    HlslDirection direction, const char *root, int index)
{
    int stageFlag;
    int directionFlag;
    int matched;
    int i;

    if (root == NULL || index < 0 ||
        (direction != HLSL_DIRECTION_INPUT &&
         direction != HLSL_DIRECTION_OUTPUT) ||
        HlslModernHasReservedPrefix(root))
    {
        return HLSL_SEMANTIC_UNSUPPORTED;
    }
    stageFlag = HlslModernStageFlag(stage);
    directionFlag = direction == HLSL_DIRECTION_OUTPUT ?
                    HLSL_MODERN_OUTPUT : HLSL_MODERN_INPUT;
    if (stageFlag == 0)
        return HLSL_SEMANTIC_UNSUPPORTED;
    matched = 0;
    for (i = 0; i < (int) (sizeof(modernSemantics) /
                            sizeof(modernSemantics[0])); i++)
    {
        if (HlslModernStringsEqual(root, modernSemantics[i].root) &&
            (modernSemantics[i].stages & stageFlag) != 0 &&
            (modernSemantics[i].directions & directionFlag) != 0)
        {
            matched = 1;
            if (index >= modernSemantics[i].firstIndex &&
                index < modernSemantics[i].firstIndex +
                        modernSemantics[i].count)
            {
                return modernSemantics[i].semantic;
            }
        }
    }
    return matched ? HLSL_SEMANTIC_UNSUPPORTED : HLSL_SEMANTIC_USER;
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

    if ((direction != HLSL_DIRECTION_INPUT &&
         direction != HLSL_DIRECTION_OUTPUT) ||
        semantic == HLSL_SEMANTIC_UNSUPPORTED)
    {
        return 0;
    }
    stageFlag = HlslModernStageFlag(stage);
    directionFlag = direction == HLSL_DIRECTION_OUTPUT ?
                    HLSL_MODERN_OUTPUT : HLSL_MODERN_INPUT;
    if (stageFlag == 0)
        return 0;
    if (semantic == HLSL_SEMANTIC_USER) {
        return !(stage == HLSL_STAGE_PIXEL &&
                 direction == HLSL_DIRECTION_OUTPUT);
    }
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
    int firstIndex;
    int count;
    int usesIndex;
    int written;

    root = NULL;
    firstIndex = 0;
    count = 1;
    usesIndex = 0;
    switch (semantic) {
    case HLSL_SEMANTIC_SV_POSITION: root = "SV_Position"; break;
    case HLSL_SEMANTIC_SV_TARGET:
        root = "SV_Target";
        count = 8;
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
        count = 2;
        usesIndex = 1;
        break;
    default:
        return 0;
    }
    if (text == NULL || size == 0 || index < firstIndex ||
        index >= firstIndex + count)
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

HlslInterpolation HlslModernRequiredTargetInterpolation(HlslBase base)
{
    return base == HLSL_BASE_INT || base == HLSL_BASE_UINT ?
           HLSL_INTERPOLATION_NOINTERPOLATION :
           HLSL_INTERPOLATION_DEFAULT;
} // HlslModernRequiredTargetInterpolation

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
