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
license, under NVIDIA's copyrights in this original NVIDIA software, to
use, reproduce, modify and redistribute the NVIDIA Software, with or
without modifications, in source and/or binary forms; provided that if
you redistribute the NVIDIA Software, you must retain the copyright
notice of NVIDIA, this notice and the following text and disclaimers in
all such redistributions of the NVIDIA Software.  Neither the name,
trademarks, service marks nor logos of NVIDIA Corporation may be used to
endorse or promote products derived from the NVIDIA Software without
specific prior written permission from NVIDIA.  Except as expressly
stated in this notice, no other rights or licenses express or implied,
are granted by NVIDIA herein, including but not limited to any patent
rights that may be infringed by your derivative works or by other works
in which the NVIDIA Software may be incorporated. No hardware is
licensed hereunder.

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
// hlsl_bind.c
//

#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "hlsl_hal.h"
#include "hlsl_modern.h"

typedef struct HlslBankState_Rec {
    unsigned char c[HLSL_MAX_FLOAT_CONSTANTS];
    unsigned char i[HLSL_MAX_INT_CONSTANTS];
    unsigned char b[HLSL_MAX_BOOL_CONSTANTS];
    unsigned char s[HLSL_MAX_SAMPLERS];
} HlslBankState;

typedef struct HlslLeafPlan_Rec {
    struct HlslLeafPlan_Rec *next;
    HlslType type;
    const char *publicName;
    int recursiveOffset;
    int defaultOffset;
    int defaultCount;
    HlslRegisterBank bank;
    int regno;
    int span;
} HlslLeafPlan;

typedef struct HlslLeafPlanList_Rec {
    HlslLeafPlan *head;
    HlslLeafPlan *tail;
    int count;
} HlslLeafPlanList;

static void *HlslBindAlloc(HlslModule *module, size_t size)
{
    void *memory;

    if (module == NULL || module->alloc == NULL)
        return NULL;
    memory = (*module->alloc)(module->allocArg, size);
    if (memory != NULL)
        memset(memory, 0, size);
    return memory;
} // HlslBindAlloc

static int HlslBindFailure(HlslModule *module, const HlslBinding *binding,
                           HlslErrorKind kind, const char *reason)
{
    return HlslFail(module, kind,
                    binding != NULL ? &binding->loc : NULL, reason);
} // HlslBindFailure

static int HlslPublicIdentifierIsValid(const char *name)
{
    const char *current;

    if (name == NULL ||
        !((name[0] >= 'A' && name[0] <= 'Z') ||
          (name[0] >= 'a' && name[0] <= 'z') || name[0] == '_'))
    {
        return 0;
    }
    for (current = name + 1; *current != '\0'; current++) {
        if (!((*current >= 'A' && *current <= 'Z') ||
              (*current >= 'a' && *current <= 'z') ||
              (*current >= '0' && *current <= '9') || *current == '_'))
        {
            return 0;
        }
    }
    return 1;
} // HlslPublicIdentifierIsValid

static const char *HlslPublicBindingName(const HlslBinding *binding)
{
    const char *name;

    if (binding == NULL)
        return NULL;
    name = binding->publicName != NULL && binding->publicName[0] != '\0' ?
           binding->publicName : binding->name;
    return HlslPublicIdentifierIsValid(name) ? name : NULL;
} // HlslPublicBindingName

static const char *HlslBankName(HlslRegisterBank bank)
{
    switch (bank) {
    case HLSL_REGISTER_C: return "c";
    case HLSL_REGISTER_I: return "i";
    case HLSL_REGISTER_B: return "b";
    case HLSL_REGISTER_S: return "s";
    case HLSL_REGISTER_NONE: break;
    }
    return "?";
} // HlslBankName

static HlslRegisterBank HlslTypeBank(const HlslType *type)
{
    if (type == NULL)
        return HLSL_REGISTER_NONE;
    if (type->arraySize > 0)
        return HlslTypeBank(type->elementType);
    switch (type->base) {
    case HLSL_BASE_INT:
    case HLSL_BASE_UINT:
        return HLSL_REGISTER_I;
    case HLSL_BASE_BOOL:
        return HLSL_REGISTER_B;
    case HLSL_BASE_SAMPLER1D:
    case HLSL_BASE_SAMPLER2D:
    case HLSL_BASE_SAMPLER3D:
    case HLSL_BASE_SAMPLERCUBE:
        return HLSL_REGISTER_S;
    default:
        return HLSL_REGISTER_C;
    }
} // HlslTypeBank

static int HlslHomogeneousBank(const HlslType *type,
                               HlslRegisterBank *result)
{
    const HlslDecl *member;
    HlslRegisterBank bank;
    HlslRegisterBank memberBank;
    int hasBank;

    if (type == NULL || result == NULL)
        return 0;
    if (type->arraySize > 0)
        return HlslHomogeneousBank(type->elementType, result);
    if (type->base != HLSL_BASE_STRUCT) {
        bank = HlslTypeBank(type);
        if (bank == HLSL_REGISTER_NONE)
            return 0;
        *result = bank;
        return 1;
    }
    bank = HLSL_REGISTER_NONE;
    hasBank = 0;
    for (member = type->members; member != NULL; member = member->next) {
        if (!HlslHomogeneousBank(&member->type, &memberBank))
            return 0;
        if (hasBank && memberBank != bank)
            return 0;
        bank = memberBank;
        hasBank = 1;
    }
    if (!hasBank)
        return 0;
    *result = bank;
    return 1;
} // HlslHomogeneousBank

static int HlslTypeComponentCount(const HlslType *type)
{
    const HlslDecl *member;
    int count;
    int memberCount;

    if (type == NULL || type->arraySize < 0)
        return 0;
    if (type->arraySize > 0) {
        count = HlslTypeComponentCount(type->elementType);
        if (count <= 0 || type->arraySize > INT_MAX / count)
            return 0;
        return type->arraySize * count;
    }
    if (type->base == HLSL_BASE_STRUCT) {
        count = 0;
        for (member = type->members; member != NULL; member = member->next) {
            memberCount = HlslTypeComponentCount(&member->type);
            if (memberCount <= 0 || count > INT_MAX - memberCount)
                return 0;
            count += memberCount;
        }
        return count;
    }
    if (type->rows > 0 && type->cols > 0)
        return type->rows * type->cols;
    switch (type->base) {
    case HLSL_BASE_FLOAT:
    case HLSL_BASE_INT:
    case HLSL_BASE_UINT:
    case HLSL_BASE_BOOL:
        return type->len;
    case HLSL_BASE_SAMPLER1D:
    case HLSL_BASE_SAMPLER2D:
    case HLSL_BASE_SAMPLER3D:
    case HLSL_BASE_SAMPLERCUBE:
        return 0;
    case HLSL_BASE_VOID:
    case HLSL_BASE_STRUCT:
        return 0;
    }
    return 0;
} // HlslTypeComponentCount

static const char *HlslJoinPublicPath(HlslModule *module,
                                      const char *parent,
                                      const char *member)
{
    char *path;
    size_t parentLength;
    size_t memberLength;

    if (parent == NULL || member == NULL || member[0] == '\0')
        return NULL;
    parentLength = strlen(parent);
    memberLength = strlen(member);
    if (parentLength > (size_t) -1 - memberLength - 2)
        return NULL;
    path = (char *) HlslBindAlloc(module,
                                  parentLength + memberLength + 2);
    if (path == NULL)
        return NULL;
    memcpy(path, parent, parentLength);
    path[parentLength] = '.';
    memcpy(path + parentLength + 1, member, memberLength + 1);
    return path;
} // HlslJoinPublicPath

static const char *HlslIndexPublicPath(HlslModule *module,
                                       const char *parent, int index)
{
    char number[32];
    char *path;
    size_t parentLength;
    size_t numberLength;

    if (parent == NULL || index < 0)
        return NULL;
    sprintf(number, "%d", index);
    parentLength = strlen(parent);
    numberLength = strlen(number);
    if (parentLength > (size_t) -1 - numberLength - 3)
        return NULL;
    path = (char *) HlslBindAlloc(module,
        parentLength + numberLength + 3);
    if (path == NULL)
        return NULL;
    memcpy(path, parent, parentLength);
    path[parentLength] = '[';
    memcpy(path + parentLength + 1, number, numberLength);
    path[parentLength + numberLength + 1] = ']';
    path[parentLength + numberLength + 2] = '\0';
    return path;
} // HlslIndexPublicPath

static int HlslAppendLeafPlan(HlslLeafPlanList *list, HlslLeafPlan *plan)
{
    if (list == NULL || plan == NULL)
        return 0;
    if (list->count == INT_MAX)
        return 0;
    if (list->tail != NULL)
        list->tail->next = plan;
    else
        list->head = plan;
    list->tail = plan;
    list->count++;
    return 1;
} // HlslAppendLeafPlan

static int HlslPlanLeaves(HlslModule *module, const HlslType *type,
                          const char *path, int recursiveOffset,
                          int defaultOffset, HlslLeafPlanList *plans)
{
    const HlslDecl *member;
    HlslLeafPlan *plan;
    const char *memberPath;
    const char *elementPath;
    HlslRegisterBank homogeneousBank;
    int componentCount;
    int elementComponents;
    int elementSpan;
    int homogeneous;
    int i;
    int memberComponents;
    int memberSpan;
    int span;

    span = HlslTypeRegisterSpan(type);
    if (span <= 0)
        return 0;
    homogeneous = HlslHomogeneousBank(type, &homogeneousBank);
    if (!homogeneous &&
        type->arraySize == 0 && type->base == HLSL_BASE_STRUCT)
    {
        for (member = type->members; member != NULL; member = member->next) {
            memberPath = HlslJoinPublicPath(module, path,
                member->publicName != NULL ? member->publicName :
                                             member->name);
            memberSpan = HlslTypeRegisterSpan(&member->type);
            memberComponents = HlslTypeComponentCount(&member->type);
            if (memberPath == NULL || memberSpan <= 0 ||
                memberComponents < 0 ||
                !HlslPlanLeaves(module, &member->type, memberPath,
                                recursiveOffset, defaultOffset, plans))
            {
                return 0;
            }
            if (recursiveOffset > INT_MAX - memberSpan ||
                defaultOffset > INT_MAX - memberComponents)
            {
                return 0;
            }
            recursiveOffset += memberSpan;
            defaultOffset += memberComponents;
        }
        return 1;
    }
    if (!homogeneous && type->arraySize > 0)
    {
        elementSpan = HlslTypeRegisterSpan(type->elementType);
        elementComponents = HlslTypeComponentCount(type->elementType);
        if (elementSpan <= 0 || elementComponents < 0)
            return 0;
        for (i = 0; i < type->arraySize; i++) {
            elementPath = HlslIndexPublicPath(module, path, i);
            if (elementPath == NULL ||
                !HlslPlanLeaves(module, type->elementType, elementPath,
                                recursiveOffset, defaultOffset, plans))
            {
                return 0;
            }
            recursiveOffset += elementSpan;
            defaultOffset += elementComponents;
        }
        return 1;
    }
    if (!homogeneous)
        return 0;

    plan = (HlslLeafPlan *) HlslBindAlloc(module, sizeof(HlslLeafPlan));
    componentCount = HlslTypeComponentCount(type);
    if (plan == NULL || path == NULL)
        return 0;
    plan->type = *type;
    plan->publicName = path;
    plan->recursiveOffset = recursiveOffset;
    plan->defaultOffset = defaultOffset;
    plan->defaultCount = componentCount;
    plan->bank = homogeneousBank;
    plan->span = span;
    if (plan->bank == HLSL_REGISTER_NONE)
        return 0;
    return HlslAppendLeafPlan(plans, plan);
} // HlslPlanLeaves

static unsigned char *HlslStateBank(HlslBankState *state,
                                    HlslRegisterBank bank)
{
    switch (bank) {
    case HLSL_REGISTER_C: return state->c;
    case HLSL_REGISTER_I: return state->i;
    case HLSL_REGISTER_B: return state->b;
    case HLSL_REGISTER_S: return state->s;
    case HLSL_REGISTER_NONE: break;
    }
    return NULL;
} // HlslStateBank

static int HlslBankMaximum(HlslRegisterBank bank)
{
    switch (bank) {
    case HLSL_REGISTER_C: return HLSL_MAX_FLOAT_CONSTANTS;
    case HLSL_REGISTER_I: return HLSL_MAX_INT_CONSTANTS;
    case HLSL_REGISTER_B: return HLSL_MAX_BOOL_CONSTANTS;
    case HLSL_REGISTER_S: return HLSL_MAX_SAMPLERS;
    case HLSL_REGISTER_NONE: break;
    }
    return 0;
} // HlslBankMaximum

static int HlslBankLimit(const HlslProfileDesc *profile,
                         HlslRegisterBank bank)
{
    if (profile == NULL || profile->limits == NULL)
        return 0;
    switch (bank) {
    case HLSL_REGISTER_C: return profile->limits->floatConstants;
    case HLSL_REGISTER_I: return profile->limits->intConstants;
    case HLSL_REGISTER_B: return profile->limits->boolConstants;
    case HLSL_REGISTER_S: return profile->limits->samplers;
    case HLSL_REGISTER_NONE: break;
    }
    return 0;
} // HlslBankLimit

static void HlslLoadBankState(const HlslModule *module,
                              HlslBankState *state)
{
    memcpy(state->c, module->cRegisterUsed, sizeof(state->c));
    memcpy(state->i, module->iRegisterUsed, sizeof(state->i));
    memcpy(state->b, module->bRegisterUsed, sizeof(state->b));
    memcpy(state->s, module->sRegisterUsed, sizeof(state->s));
} // HlslLoadBankState

static void HlslStoreBankState(HlslModule *module,
                               const HlslBankState *state)
{
    memcpy(module->cRegisterUsed, state->c, sizeof(state->c));
    memcpy(module->iRegisterUsed, state->i, sizeof(state->i));
    memcpy(module->bRegisterUsed, state->b, sizeof(state->b));
    memcpy(module->sRegisterUsed, state->s, sizeof(state->s));
} // HlslStoreBankState

static int HlslRangeIsFree(const unsigned char *used, int first, int span)
{
    int i;

    for (i = 0; i < span; i++) {
        if (used[first + i])
            return 0;
    }
    return 1;
} // HlslRangeIsFree

static void HlslMarkRange(unsigned char *used, int first, int span)
{
    int i;

    for (i = 0; i < span; i++)
        used[first + i] = 1;
} // HlslMarkRange

static int HlslFindRange(const unsigned char *used, int limit, int span)
{
    int first;

    if (span <= 0 || span > limit)
        return -1;
    for (first = 0; first <= limit - span; first++) {
        if (HlslRangeIsFree(used, first, span))
            return first;
    }
    return -1;
} // HlslFindRange

static int HlslUsedCount(const unsigned char *used, int limit)
{
    int count;
    int i;

    count = 0;
    for (i = 0; i < limit; i++) {
        if (used[i])
            count++;
    }
    return count;
} // HlslUsedCount

static int HlslSaturatingAdd(int left, int right)
{
    if (left < 0)
        return right;
    if (right < 0 || left > INT_MAX - right)
        return INT_MAX;
    return left + right;
} // HlslSaturatingAdd

static int HlslResourceFailure(HlslModule *module,
                               const HlslBinding *binding,
                               HlslRegisterBank bank, int used, int limit)
{
    if (module != NULL && module->errors == 0) {
        module->resourceName = HlslBankName(bank);
        module->resourceUsed = used;
        module->resourceAvailable = limit;
    }
    return HlslBindFailure(module, binding, HLSL_ERROR_RESOURCE_LIMIT,
                           binding != NULL ? binding->name : NULL);
} // HlslResourceFailure

static const char *HlslCollisionReason(HlslModule *module,
                                       const char *publicName,
                                       HlslRegisterBank bank, int regno)
{
    const char *name;
    const char *bankName;
    char number[32];
    char *reason;
    size_t length;

    name = publicName != NULL ? publicName : "anonymous";
    bankName = HlslBankName(bank);
    sprintf(number, "%d", regno);
    length = strlen(name) + strlen(bankName) + strlen(number) + 5;
    reason = (char *) HlslBindAlloc(module, length);
    if (reason == NULL)
        return name;
    sprintf(reason, "%s at %s%s", name, bankName, number);
    return reason;
} // HlslCollisionReason

static int HlslReservePlan(HlslModule *module,
                           const HlslProfileDesc *profile,
                           const HlslBinding *binding,
                           HlslBankState *state, HlslLeafPlan *plan,
                           int explicitRegno)
{
    unsigned char *used;
    int first;
    int limit;
    int maximum;
    int i;

    used = HlslStateBank(state, plan->bank);
    limit = HlslBankLimit(profile, plan->bank);
    maximum = HlslBankMaximum(plan->bank);
    if (used == NULL || limit < 0 || limit > maximum)
        return HlslBindFailure(module, binding, HLSL_ERROR_INVALID_IR,
                               "invalid HLSL register limits");
    if (binding->hasExplicitRegister) {
        first = explicitRegno;
        if (first < 0 || plan->span > limit || first > limit - plan->span)
            return HlslResourceFailure(module, binding, plan->bank,
                HlslSaturatingAdd(first, plan->span), limit);
        for (i = 0; i < plan->span; i++) {
            if (used[first + i]) {
                return HlslBindFailure(module, binding,
                    HLSL_ERROR_REGISTER_COLLISION,
                    HlslCollisionReason(module, plan->publicName, plan->bank,
                                        first + i));
            }
        }
    } else {
        first = HlslFindRange(used, limit, plan->span);
        if (first < 0) {
            return HlslResourceFailure(module, binding, plan->bank,
                HlslSaturatingAdd(HlslUsedCount(used, limit), plan->span),
                limit);
        }
    }
    plan->regno = first;
    HlslMarkRange(used, first, plan->span);
    return 1;
} // HlslReservePlan

static int HlslPreflightImplicitType(HlslModule *module,
                                     const HlslProfileDesc *profile,
                                     const HlslBinding *binding,
                                     HlslBankState *state,
                                     const HlslType *type)
{
    const HlslDecl *member;
    HlslLeafPlan plan;
    HlslRegisterBank bank;
    int i;

    if (HlslHomogeneousBank(type, &bank)) {
        memset(&plan, 0, sizeof(plan));
        plan.bank = bank;
        plan.span = HlslTypeRegisterSpan(type);
        return plan.span > 0 &&
               HlslReservePlan(module, profile, binding, state, &plan, 0);
    }
    if (type->arraySize > 0) {
        for (i = 0; i < type->arraySize; i++) {
            if (!HlslPreflightImplicitType(module, profile, binding, state,
                                           type->elementType))
            {
                return 0;
            }
        }
        return 1;
    }
    if (type->base == HLSL_BASE_STRUCT) {
        for (member = type->members; member != NULL; member = member->next) {
            if (!HlslPreflightImplicitType(module, profile, binding, state,
                                           &member->type))
            {
                return 0;
            }
        }
        return 1;
    }
    return HlslBindFailure(module, binding, HLSL_ERROR_UNSUPPORTED_TYPE,
                           binding->name);
} // HlslPreflightImplicitType

static int HlslPreflightImplicitBinding(HlslModule *module,
                                        const HlslProfileDesc *profile,
                                        const HlslBinding *binding)
{
    HlslBankState state;

    /* Each leaf consumes at least one register, so this walk fails within
       the active banks' finite capacity without materializing leaf plans. */
    HlslLoadBankState(module, &state);
    return HlslPreflightImplicitType(module, profile, binding, &state,
                                     &binding->type);
} // HlslPreflightImplicitBinding

static char *HlslGlobalSourceName(HlslModule *module, const char *path)
{
    char *name;
    char *current;
    size_t length;

    if (path == NULL)
        return NULL;
    length = strlen(path);
    name = (char *) HlslBindAlloc(module, length + 4);
    if (name == NULL)
        return NULL;
    memcpy(name, "cg_", 3);
    memcpy(name + 3, path, length + 1);
    for (current = name + 3; *current != '\0'; current++) {
        if (!((*current >= 'A' && *current <= 'Z') ||
              (*current >= 'a' && *current <= 'z') ||
              (*current >= '0' && *current <= '9') || *current == '_'))
        {
            *current = '_';
        }
    }
    return name;
} // HlslGlobalSourceName

static HlslBase HlslDefaultBase(const HlslType *type)
{
    while (type != NULL && type->arraySize > 0)
        type = type->elementType;
    if (type == NULL)
        return HLSL_BASE_VOID;
    return type->base;
} // HlslDefaultBase

static int HlslDefaultScalarIsValid(HlslBase base, float value)
{
    int integerValue;

    if (value != value || value > FLT_MAX || value < -FLT_MAX)
        return 0;
    if (base == HLSL_BASE_FLOAT)
        return 1;
    if (base == HLSL_BASE_BOOL)
        return value == 0.0f || value == 1.0f;
    if (base != HLSL_BASE_INT || (double) value > (double) INT_MAX ||
        (double) value < (double) INT_MIN)
    {
        return 0;
    }
    integerValue = (int) value;
    return (float) integerValue == value;
} // HlslDefaultScalarIsValid

static int HlslValidateDefaultType(const HlslType *type,
                                   const float *values, int *offset)
{
    const HlslDecl *member;
    HlslBase base;
    int count;
    int i;

    if (type->arraySize > 0) {
        for (i = 0; i < type->arraySize; i++) {
            if (!HlslValidateDefaultType(type->elementType, values, offset))
                return 0;
        }
        return 1;
    }
    if (type->base == HLSL_BASE_STRUCT) {
        for (member = type->members; member != NULL; member = member->next) {
            if (!HlslValidateDefaultType(&member->type, values, offset))
                return 0;
        }
        return 1;
    }
    base = HlslDefaultBase(type);
    if (base == HLSL_BASE_SAMPLER1D || base == HLSL_BASE_SAMPLER2D ||
        base == HLSL_BASE_SAMPLER3D || base == HLSL_BASE_SAMPLERCUBE)
    {
        return 1;
    }
    if (type->rows > 0 && type->cols > 0)
        count = type->rows * type->cols;
    else
        count = type->len;
    for (i = 0; i < count; i++) {
        if (!HlslDefaultScalarIsValid(base, values[*offset]))
            return 0;
        (*offset)++;
    }
    return 1;
} // HlslValidateDefaultType

static int HlslValidateDefaultLiteralType(
    const HlslType *type, const HlslDefaultLiteral *values, int *offset)
{
    const HlslDecl *member;
    const HlslDefaultLiteral *value;
    HlslBase base;
    int count;
    int i;

    if (type->arraySize > 0) {
        for (i = 0; i < type->arraySize; i++) {
            if (!HlslValidateDefaultLiteralType(type->elementType, values,
                                                offset))
            {
                return 0;
            }
        }
        return 1;
    }
    if (type->base == HLSL_BASE_STRUCT) {
        for (member = type->members; member != NULL; member = member->next) {
            if (!HlslValidateDefaultLiteralType(&member->type, values,
                                                offset))
            {
                return 0;
            }
        }
        return 1;
    }
    base = HlslDefaultBase(type);
    if (base == HLSL_BASE_SAMPLER1D || base == HLSL_BASE_SAMPLER2D ||
        base == HLSL_BASE_SAMPLER3D || base == HLSL_BASE_SAMPLERCUBE)
    {
        return 1;
    }
    count = type->rows > 0 && type->cols > 0 ?
            type->rows * type->cols : type->len;
    for (i = 0; i < count; i++) {
        value = &values[*offset];
        if (value->base != base ||
            (base == HLSL_BASE_FLOAT &&
             (value->value.floating != value->value.floating ||
              value->value.floating > FLT_MAX ||
              value->value.floating < -FLT_MAX)) ||
            (base == HLSL_BASE_BOOL &&
             value->value.boolean != 0 && value->value.boolean != 1) ||
            (base != HLSL_BASE_FLOAT && base != HLSL_BASE_INT &&
             base != HLSL_BASE_BOOL))
        {
            return 0;
        }
        (*offset)++;
    }
    return 1;
} // HlslValidateDefaultLiteralType

static int HlslDefaultValuesAreValid(const HlslBinding *binding)
{
    int offset;

    if (binding->defaultCount == 0)
        return binding->defaultValues == NULL &&
               binding->defaultLiterals == NULL;
    if ((binding->defaultValues == NULL) ==
        (binding->defaultLiterals == NULL))
    {
        return 0;
    }
    offset = 0;
    return (binding->defaultLiterals != NULL ?
            HlslValidateDefaultLiteralType(&binding->type,
                                           binding->defaultLiterals,
                                           &offset) :
            HlslValidateDefaultType(&binding->type,
                                    binding->defaultValues, &offset)) &&
           offset == binding->defaultCount;
} // HlslDefaultValuesAreValid

static HlslExpr *HlslBuildDefaultValue(HlslModule *module,
                                       const HlslType *type,
                                       const float *values,
                                       const HlslDefaultLiteral *literals,
                                       int count,
                                       int *offset)
{
    HlslExpr *initializer;
    HlslExpr *value;
    HlslType scalarType;
    HlslBase base;
    int componentCount;
    int i;

    if (type == NULL || count <= 0 || offset == NULL ||
        ((values == NULL) == (literals == NULL)))
    {
        return NULL;
    }
    if (type->arraySize > 0) {
        initializer = HlslNewExpr(module, HLSL_EXPR_CONSTRUCT, *type);
        if (initializer == NULL)
            return NULL;
        for (i = 0; i < type->arraySize; i++) {
            value = HlslBuildDefaultValue(module, type->elementType,
                                           values, literals, count, offset);
            if (value == NULL)
                return NULL;
            HlslAppendExpr(&initializer->u.construct.arguments, value);
        }
        return initializer;
    }
    if (type->base == HLSL_BASE_STRUCT) {
        HlslDecl *member;

        initializer = HlslNewExpr(module, HLSL_EXPR_CONSTRUCT, *type);
        if (initializer == NULL)
            return NULL;
        for (member = type->members; member != NULL;
             member = member->next)
        {
            value = HlslBuildDefaultValue(module, &member->type, values,
                                           literals, count, offset);
            if (value == NULL)
                return NULL;
            HlslAppendExpr(&initializer->u.construct.arguments, value);
        }
        return initializer;
    }
    base = HlslDefaultBase(type);
    if (base != HLSL_BASE_FLOAT && base != HLSL_BASE_INT &&
        base != HLSL_BASE_BOOL)
    {
        return NULL;
    }
    componentCount = type->rows > 0 && type->cols > 0 ?
                     type->rows * type->cols : type->len;
    if (componentCount <= 0 || *offset < 0 ||
        *offset > count - componentCount)
    {
        return NULL;
    }
    initializer = componentCount == 1 ? NULL :
        HlslNewExpr(module, HLSL_EXPR_CONSTRUCT, *type);
    if (componentCount > 1 && initializer == NULL)
        return NULL;
    scalarType = HlslNumericType(base, 1);
    for (i = 0; i < componentCount; i++) {
        if (base == HLSL_BASE_FLOAT) {
            value = HlslNewExpr(module, HLSL_EXPR_FLOAT, scalarType);
            if (value != NULL)
                value->u.literalFloat = literals != NULL ?
                    literals[*offset].value.floating : values[*offset];
        } else if (base == HLSL_BASE_INT) {
            value = HlslNewExpr(module, HLSL_EXPR_INT, scalarType);
            if (value != NULL)
                value->u.literalInt = literals != NULL ?
                    literals[*offset].value.integer : (int) values[*offset];
        } else {
            value = HlslNewExpr(module, HLSL_EXPR_BOOL, scalarType);
            if (value != NULL)
                value->u.literalBool = literals != NULL ?
                    literals[*offset].value.boolean :
                    values[*offset] != 0.0f;
        }
        if (value == NULL)
            return NULL;
        (*offset)++;
        if (componentCount == 1)
            return value;
        HlslAppendExpr(&initializer->u.construct.arguments, value);
    }
    return initializer;
} // HlslBuildDefaultValue

static int HlslBuildDefaultInitializer(HlslModule *module,
                                       const HlslType *type,
                                       const float *values,
                                       const HlslDefaultLiteral *literals,
                                       int count,
                                       HlslExpr **result)
{
    int offset;

    if (result == NULL)
        return 0;
    *result = NULL;
    if (type == NULL || count <= 0)
        return 1;
    offset = 0;
    *result = HlslBuildDefaultValue(module, type, values, literals,
                                    count, &offset);
    return *result != NULL && offset == count;
} // HlslBuildDefaultInitializer

static void HlslAppendAllocatedBinding(HlslBinding **list,
                                       HlslBinding *binding)
{
    HlslBinding *current;

    if (*list == NULL) {
        *list = binding;
        return;
    }
    for (current = *list; current->allocationNext != NULL;
         current = current->allocationNext)
    {
    }
    current->allocationNext = binding;
} // HlslAppendAllocatedBinding

static int HlslBuildLeafRecords(HlslModule *module, HlslBinding *binding,
                                HlslLeafPlan *plans,
                                HlslBinding **records,
                                HlslDecl **globals)
{
    HlslLeafPlan *plan;
    HlslBinding *record;
    HlslDecl *declaration;
    char *sourceName;
    const char *emittedName;
    float *defaults;
    HlslDefaultLiteral *defaultLiterals;

    for (plan = plans; plan != NULL; plan = plan->next) {
        record = HlslNewBinding(module, binding->storage, plan->type,
                                plan->publicName, binding->semantic);
        sourceName = HlslGlobalSourceName(module, plan->publicName);
        emittedName = record != NULL && sourceName != NULL ?
                      HlslAllocateGeneratedName(module, record,
                                                sourceName) : NULL;
        declaration = emittedName != NULL ?
            HlslNewDecl(module, binding->storage, plan->type,
                        emittedName) : NULL;
        if (record == NULL || declaration == NULL)
            return 0;
        record->publicName = plan->publicName;
        record->loc = binding->loc;
        record->sourceOrdinal = binding->sourceOrdinal;
        record->recursiveOffset = plan->recursiveOffset;
        record->hasExplicitRegister = binding->hasExplicitRegister;
        record->isAllocated = 1;
        record->sourceBase = binding->sourceBase;
        record->physical.bank = plan->bank;
        record->physical.regno = plan->regno;
        record->physical.span = plan->span;
        record->declaration = declaration;
        if (binding->defaultCount > 0) {
            if (plan->defaultCount <= 0 ||
                plan->defaultOffset > binding->defaultCount ||
                plan->defaultCount >
                    binding->defaultCount - plan->defaultOffset)
            {
                return 0;
            }
            defaults = NULL;
            defaultLiterals = NULL;
            if (binding->defaultLiterals != NULL) {
                defaultLiterals =
                    (HlslDefaultLiteral *) HlslBindAlloc(module,
                        (size_t) plan->defaultCount *
                            sizeof(HlslDefaultLiteral));
                if (defaultLiterals == NULL)
                    return 0;
                memcpy(defaultLiterals,
                       binding->defaultLiterals + plan->defaultOffset,
                       (size_t) plan->defaultCount *
                           sizeof(HlslDefaultLiteral));
            } else {
                defaults = (float *) HlslBindAlloc(module,
                    (size_t) plan->defaultCount * sizeof(float));
                if (defaults == NULL)
                    return 0;
                memcpy(defaults,
                       binding->defaultValues + plan->defaultOffset,
                       (size_t) plan->defaultCount * sizeof(float));
            }
            record->defaultCount = plan->defaultCount;
            record->defaultValues = defaults;
            record->defaultLiterals = defaultLiterals;
            if (!HlslBuildDefaultInitializer(module, &plan->type, defaults,
                                              defaultLiterals,
                                              plan->defaultCount,
                                              &declaration->initializer))
            {
                return 0;
            }
        }
        declaration->semantic = binding->semantic;
        if (binding->declaration != NULL)
            declaration->identity = binding->declaration->identity;
        declaration->loc = binding->loc;
        declaration->sourceOrdinal = binding->sourceOrdinal;
        declaration->physical = record->physical;
        HlslAppendBinding(records, record);
        HlslAppendDecl(globals, declaration);
    }
    return 1;
} // HlslBuildLeafRecords

static void HlslAdoptSingleLeafDeclaration(HlslBinding *binding,
                                           HlslBinding *record,
                                           HlslDecl **globals)
{
    HlslDecl *generated;
    HlslDecl *stable;
    const void *identity;

    stable = binding->declaration;
    generated = record->declaration;
    if (stable == NULL || generated == NULL)
        return;
    identity = stable->identity;
    *stable = *generated;
    stable->identity = identity;
    record->declaration = stable;
    *globals = stable;
} // HlslAdoptSingleLeafDeclaration

int HlslAllocateOneBinding(HlslModule *module,
                           const HlslProfileDesc *profile,
                           HlslBinding *binding)
{
    HlslBankState state;
    HlslLeafPlanList plans;
    HlslName *savedNames;
    HlslLeafPlan *plan;
    HlslBinding *records;
    HlslBinding *record;
    HlslBinding *nextRecord;
    HlslDecl *globals;
    const char *publicName;
    HlslRegisterBank homogeneousBank;
    int defaultComponents;
    int explicitRegno;
    int homogeneous;
    int planCount;
    int typeSpan;

    if (module == NULL || profile == NULL || profile->limits == NULL ||
        binding == NULL || binding->name == NULL ||
        module->stage != profile->stage || binding->isAllocated)
    {
        return HlslBindFailure(module, binding, HLSL_ERROR_INVALID_IR,
                               "invalid HLSL binding");
    }
    publicName = HlslPublicBindingName(binding);
    if (publicName == NULL)
        return HlslBindFailure(module, binding, HLSL_ERROR_INVALID_IR,
                               "invalid HLSL public binding name");
    typeSpan = HlslTypeRegisterSpan(&binding->type);
    if (typeSpan <= 0)
        return HlslBindFailure(module, binding,
                               HLSL_ERROR_UNSUPPORTED_TYPE, binding->name);
    defaultComponents = HlslTypeComponentCount(&binding->type);
    if (binding->defaultCount < 0 ||
        (binding->defaultCount > 0 &&
         (((binding->defaultValues == NULL) ==
           (binding->defaultLiterals == NULL)) ||
          binding->defaultCount != defaultComponents)) ||
        !HlslDefaultValuesAreValid(binding))
    {
        return HlslBindFailure(module, binding, HLSL_ERROR_INVALID_IR,
                               "invalid HLSL binding default");
    }
    if (binding->hasExplicitRegister) {
        homogeneous = HlslHomogeneousBank(&binding->type,
                                           &homogeneousBank);
        if (binding->physical.bank == HLSL_REGISTER_NONE ||
            (homogeneous && binding->physical.span != 0 &&
             binding->physical.span != typeSpan))
        {
            return HlslBindFailure(module, binding, HLSL_ERROR_INVALID_IR,
                                   binding->name);
        }
        if (!homogeneous ||
            homogeneousBank != binding->physical.bank)
        {
            return HlslBindFailure(module, binding,
                                   HLSL_ERROR_UNSUPPORTED_TYPE,
                                   binding->name);
        }
    } else if (!HlslPreflightImplicitBinding(module, profile, binding)) {
        return 0;
    }

    memset(&plans, 0, sizeof(plans));
    if (!HlslPlanLeaves(module, &binding->type, publicName, 0, 0,
                        &plans))
    {
        return HlslBindFailure(module, binding,
                               HLSL_ERROR_UNSUPPORTED_TYPE, binding->name);
    }
    planCount = plans.count;

    HlslLoadBankState(module, &state);
    for (plan = plans.head; plan != NULL; plan = plan->next) {
        if (binding->hasExplicitRegister &&
            binding->physical.regno < 0)
        {
            explicitRegno = -1;
        } else if (binding->hasExplicitRegister &&
                   binding->physical.regno >
                       INT_MAX - plan->recursiveOffset)
        {
            explicitRegno = INT_MAX;
        } else {
            explicitRegno = binding->physical.regno +
                            plan->recursiveOffset;
        }
        if (!HlslReservePlan(module, profile, binding, &state, plan,
                             explicitRegno))
        {
            return 0;
        }
    }
    records = NULL;
    globals = NULL;
    savedNames = module->names;
    if (!HlslBuildLeafRecords(module, binding, plans.head, &records,
                              &globals))
    {
        module->names = savedNames;
        return HlslBindFailure(module, binding, HLSL_ERROR_INVALID_IR,
                               "HLSL binding allocation");
    }
    if (planCount == 1)
        HlslAdoptSingleLeafDeclaration(binding, records, &globals);

    HlslStoreBankState(module, &state);
    binding->leafBindings = records;
    binding->isAllocated = 1;
    if (planCount == 1) {
        binding->physical = records->physical;
        binding->declaration = records->declaration;
    } else {
        memset(&binding->physical, 0, sizeof(binding->physical));
    }
    HlslAppendDecl(&module->globals, globals);
    for (record = records; record != NULL; record = nextRecord) {
        nextRecord = record->next;
        HlslAppendAllocatedBinding(&module->allocatedBindings, record);
    }
    return 1;
} // HlslAllocateOneBinding

static int HlslBindingCount(const HlslBinding *bindings)
{
    const HlslBinding *slow;
    const HlslBinding *fast;
    int count;

    slow = bindings;
    fast = bindings;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            return -1;
    }
    count = 0;
    for (; bindings != NULL; bindings = bindings->next) {
        if (count == INT_MAX)
            return -1;
        count++;
    }
    return count;
} // HlslBindingCount

static HlslExpr *HlslBindingSymbol(HlslModule *module, HlslDecl *decl)
{
    HlslExpr *expression;

    expression = decl != NULL ?
        HlslNewExpr(module, HLSL_EXPR_SYMBOL, decl->type) : NULL;
    if (expression != NULL)
        expression->u.symbol = decl;
    return expression;
} // HlslBindingSymbol

static HlslExpr *HlslBindingMember(HlslModule *module, HlslExpr *object,
                                   HlslDecl *member)
{
    HlslExpr *expression;

    expression = object != NULL && member != NULL ?
        HlslNewExpr(module, HLSL_EXPR_MEMBER, member->type) : NULL;
    if (expression != NULL) {
        expression->u.member.object = object;
        expression->u.member.decl = member;
        expression->u.member.name = member->name;
    }
    return expression;
} // HlslBindingMember

static HlslExpr *HlslBindingIndex(HlslModule *module, HlslExpr *object,
                                  const HlslType *elementType, int index)
{
    HlslExpr *expression;
    HlslExpr *subscript;

    expression = object != NULL && elementType != NULL ?
        HlslNewExpr(module, HLSL_EXPR_INDEX, *elementType) : NULL;
    subscript = HlslNewExpr(module, HLSL_EXPR_INT,
                            HlslNumericType(HLSL_BASE_INT, 1));
    if (expression == NULL || subscript == NULL)
        return NULL;
    subscript->u.literalInt = index;
    expression->u.index.object = object;
    expression->u.index.index = subscript;
    return expression;
} // HlslBindingIndex

static int HlslAppendBindingCopy(HlslModule *module, const HlslType *type,
                                 HlslExpr *target, HlslExpr *source,
                                 HlslStmt **statements)
{
    HlslDecl *member;
    HlslExpr *assignment;
    HlslStmt *statement;
    int i;

    if (type == NULL || target == NULL || source == NULL)
        return 0;
    if (type->arraySize > 0) {
        for (i = 0; i < type->arraySize; i++) {
            if (!HlslAppendBindingCopy(module, type->elementType,
                    HlslBindingIndex(module, target, type->elementType, i),
                    HlslBindingIndex(module, source, type->elementType, i),
                    statements))
            {
                return 0;
            }
        }
        return 1;
    }
    if (type->base == HLSL_BASE_STRUCT) {
        for (member = type->members; member != NULL; member = member->next) {
            if (!HlslAppendBindingCopy(module, &member->type,
                    HlslBindingMember(module, target, member),
                    HlslBindingMember(module, source, member), statements))
            {
                return 0;
            }
        }
        return 1;
    }
    assignment = HlslNewExpr(module, HLSL_EXPR_BINARY, *type);
    statement = HlslNewStmt(module, HLSL_STMT_EXPRESSION);
    if (assignment == NULL || statement == NULL)
        return 0;
    assignment->u.binary.op = HLSL_OP_ASSIGN;
    assignment->u.binary.left = target;
    assignment->u.binary.right = source;
    statement->u.expression = assignment;
    HlslAppendStmt(statements, statement);
    return 1;
} // HlslAppendBindingCopy

static int HlslAppendMixedBindingCopy(HlslModule *module,
                                      const HlslType *type,
                                      HlslExpr *target,
                                      HlslBinding **leaf,
                                      HlslStmt **statements)
{
    HlslRegisterBank bank;
    HlslDecl *member;
    HlslExpr *element;
    int i;

    if (type == NULL || target == NULL || leaf == NULL)
        return 0;
    if (HlslHomogeneousBank(type, &bank)) {
        if (*leaf == NULL || (*leaf)->declaration == NULL ||
            !HlslAppendBindingCopy(module, type, target,
                HlslBindingSymbol(module, (*leaf)->declaration), statements))
        {
            return 0;
        }
        *leaf = (*leaf)->next;
        return 1;
    }
    if (type->arraySize > 0) {
        for (i = 0; i < type->arraySize; i++) {
            element = HlslBindingIndex(module, target, type->elementType, i);
            if (!HlslAppendMixedBindingCopy(module, type->elementType,
                                             element, leaf, statements))
            {
                return 0;
            }
        }
        return 1;
    }
    if (type->base == HLSL_BASE_STRUCT) {
        for (member = type->members; member != NULL; member = member->next) {
            if (!HlslAppendMixedBindingCopy(module, &member->type,
                    HlslBindingMember(module, target, member), leaf,
                    statements))
            {
                return 0;
            }
        }
        return 1;
    }
    return 0;
} // HlslAppendMixedBindingCopy

static int HlslExpressionUsesDecl(const HlslExpr *expression,
                                  const HlslDecl *decl)
{
    const HlslExpr *argument;

    if (expression == NULL)
        return 0;
    switch (expression->kind) {
    case HLSL_EXPR_SYMBOL:
        return expression->u.symbol == decl;
    case HLSL_EXPR_UNARY:
        return HlslExpressionUsesDecl(expression->u.unary.operand, decl);
    case HLSL_EXPR_BINARY:
        return HlslExpressionUsesDecl(expression->u.binary.left, decl) ||
               HlslExpressionUsesDecl(expression->u.binary.right, decl);
    case HLSL_EXPR_CONDITIONAL:
        return HlslExpressionUsesDecl(
                   expression->u.conditional.condition, decl) ||
               HlslExpressionUsesDecl(
                   expression->u.conditional.trueExpr, decl) ||
               HlslExpressionUsesDecl(
                   expression->u.conditional.falseExpr, decl);
    case HLSL_EXPR_CALL:
        argument = expression->u.call.arguments;
        break;
    case HLSL_EXPR_CONSTRUCT:
        argument = expression->u.construct.arguments;
        break;
    case HLSL_EXPR_CAST:
        return HlslExpressionUsesDecl(expression->u.cast.expression, decl);
    case HLSL_EXPR_MEMBER:
        return HlslExpressionUsesDecl(expression->u.member.object, decl);
    case HLSL_EXPR_INDEX:
        return HlslExpressionUsesDecl(expression->u.index.object, decl) ||
               HlslExpressionUsesDecl(expression->u.index.index, decl);
    case HLSL_EXPR_SWIZZLE:
        return HlslExpressionUsesDecl(expression->u.swizzle.object, decl);
    case HLSL_EXPR_INT:
    case HLSL_EXPR_FLOAT:
    case HLSL_EXPR_BOOL:
    default:
        return 0;
    }
    for (; argument != NULL; argument = argument->next) {
        if (HlslExpressionUsesDecl(argument, decl))
            return 1;
    }
    return 0;
} // HlslExpressionUsesDecl

static int HlslStatementsUseDecl(const HlslStmt *statement,
                                 const HlslDecl *decl);

static int HlslStatementUsesDecl(const HlslStmt *statement,
                                 const HlslDecl *decl)
{
    if (statement == NULL)
        return 0;
    switch (statement->kind) {
    case HLSL_STMT_DECLARATION:
        return statement->u.declaration != NULL &&
               HlslExpressionUsesDecl(
                   statement->u.declaration->initializer, decl);
    case HLSL_STMT_EXPRESSION:
        return HlslExpressionUsesDecl(statement->u.expression, decl);
    case HLSL_STMT_IF:
        return HlslExpressionUsesDecl(statement->u.ifStmt.condition, decl) ||
               HlslStatementsUseDecl(statement->u.ifStmt.trueBranch, decl) ||
               HlslStatementsUseDecl(statement->u.ifStmt.falseBranch, decl);
    case HLSL_STMT_WHILE:
    case HLSL_STMT_DO:
        return HlslExpressionUsesDecl(statement->u.loop.condition, decl) ||
               HlslStatementsUseDecl(statement->u.loop.body, decl);
    case HLSL_STMT_FOR:
        return HlslStatementsUseDecl(statement->u.forStmt.init, decl) ||
               HlslExpressionUsesDecl(statement->u.forStmt.condition, decl) ||
               HlslStatementsUseDecl(statement->u.forStmt.step, decl) ||
               HlslStatementsUseDecl(statement->u.forStmt.body, decl);
    case HLSL_STMT_BLOCK:
        return HlslStatementsUseDecl(statement->u.block, decl);
    case HLSL_STMT_RETURN:
        return HlslExpressionUsesDecl(statement->u.returnExpr, decl);
    default:
        return 0;
    }
} // HlslStatementUsesDecl

static int HlslStatementsUseDecl(const HlslStmt *statement,
                                 const HlslDecl *decl)
{
    for (; statement != NULL; statement = statement->next) {
        if (HlslStatementUsesDecl(statement, decl))
            return 1;
    }
    return 0;
} // HlslStatementsUseDecl

static void HlslReplaceExpressionDecl(HlslExpr *expression,
                                      const HlslDecl *source,
                                      HlslDecl *replacement)
{
    HlslExpr *argument;

    for (; expression != NULL; expression = expression->next) {
        switch (expression->kind) {
        case HLSL_EXPR_SYMBOL:
            if (expression->u.symbol == source)
                expression->u.symbol = replacement;
            break;
        case HLSL_EXPR_UNARY:
            HlslReplaceExpressionDecl(expression->u.unary.operand,
                                      source, replacement);
            break;
        case HLSL_EXPR_BINARY:
            HlslReplaceExpressionDecl(expression->u.binary.left,
                                      source, replacement);
            HlslReplaceExpressionDecl(expression->u.binary.right,
                                      source, replacement);
            break;
        case HLSL_EXPR_CONDITIONAL:
            HlslReplaceExpressionDecl(
                expression->u.conditional.condition, source, replacement);
            HlslReplaceExpressionDecl(
                expression->u.conditional.trueExpr, source, replacement);
            HlslReplaceExpressionDecl(
                expression->u.conditional.falseExpr, source, replacement);
            break;
        case HLSL_EXPR_CALL:
            argument = expression->u.call.arguments;
            HlslReplaceExpressionDecl(argument, source, replacement);
            break;
        case HLSL_EXPR_CONSTRUCT:
            argument = expression->u.construct.arguments;
            HlslReplaceExpressionDecl(argument, source, replacement);
            break;
        case HLSL_EXPR_CAST:
            HlslReplaceExpressionDecl(expression->u.cast.expression,
                                      source, replacement);
            break;
        case HLSL_EXPR_MEMBER:
            HlslReplaceExpressionDecl(expression->u.member.object,
                                      source, replacement);
            break;
        case HLSL_EXPR_INDEX:
            HlslReplaceExpressionDecl(expression->u.index.object,
                                      source, replacement);
            HlslReplaceExpressionDecl(expression->u.index.index,
                                      source, replacement);
            break;
        case HLSL_EXPR_SWIZZLE:
            HlslReplaceExpressionDecl(expression->u.swizzle.object,
                                      source, replacement);
            break;
        case HLSL_EXPR_INT:
        case HLSL_EXPR_FLOAT:
        case HLSL_EXPR_BOOL:
        default:
            break;
        }
    }
} // HlslReplaceExpressionDecl

static void HlslReplaceStatementDecl(HlslStmt *statement,
                                     const HlslDecl *source,
                                     HlslDecl *replacement)
{
    for (; statement != NULL; statement = statement->next) {
        switch (statement->kind) {
        case HLSL_STMT_DECLARATION:
            if (statement->u.declaration != NULL) {
                HlslReplaceExpressionDecl(
                    statement->u.declaration->initializer,
                    source, replacement);
            }
            break;
        case HLSL_STMT_EXPRESSION:
            HlslReplaceExpressionDecl(statement->u.expression,
                                      source, replacement);
            break;
        case HLSL_STMT_IF:
            HlslReplaceExpressionDecl(statement->u.ifStmt.condition,
                                      source, replacement);
            HlslReplaceStatementDecl(statement->u.ifStmt.trueBranch,
                                     source, replacement);
            HlslReplaceStatementDecl(statement->u.ifStmt.falseBranch,
                                     source, replacement);
            break;
        case HLSL_STMT_WHILE:
        case HLSL_STMT_DO:
            HlslReplaceExpressionDecl(statement->u.loop.condition,
                                      source, replacement);
            HlslReplaceStatementDecl(statement->u.loop.body,
                                     source, replacement);
            break;
        case HLSL_STMT_FOR:
            HlslReplaceStatementDecl(statement->u.forStmt.init,
                                     source, replacement);
            HlslReplaceExpressionDecl(statement->u.forStmt.condition,
                                      source, replacement);
            HlslReplaceStatementDecl(statement->u.forStmt.step,
                                     source, replacement);
            HlslReplaceStatementDecl(statement->u.forStmt.body,
                                     source, replacement);
            break;
        case HLSL_STMT_BLOCK:
            HlslReplaceStatementDecl(statement->u.block,
                                     source, replacement);
            break;
        case HLSL_STMT_RETURN:
            HlslReplaceExpressionDecl(statement->u.returnExpr,
                                      source, replacement);
            break;
        default:
            break;
        }
    }
} // HlslReplaceStatementDecl

static HlslExpr *HlslFindFunctionCallExpr(HlslExpr *expression,
                                          const HlslFunction *function)
{
    HlslExpr *argument;
    HlslExpr *call;

    if (expression == NULL)
        return NULL;
    switch (expression->kind) {
    case HLSL_EXPR_UNARY:
        return HlslFindFunctionCallExpr(expression->u.unary.operand,
                                        function);
    case HLSL_EXPR_BINARY:
        call = HlslFindFunctionCallExpr(expression->u.binary.left, function);
        return call != NULL ? call :
            HlslFindFunctionCallExpr(expression->u.binary.right, function);
    case HLSL_EXPR_CONDITIONAL:
        call = HlslFindFunctionCallExpr(
                   expression->u.conditional.condition, function);
        if (call == NULL)
            call = HlslFindFunctionCallExpr(
                       expression->u.conditional.trueExpr, function);
        return call != NULL ? call :
            HlslFindFunctionCallExpr(expression->u.conditional.falseExpr,
                                     function);
    case HLSL_EXPR_CALL:
        if (expression->u.call.function == function)
            return expression;
        argument = expression->u.call.arguments;
        break;
    case HLSL_EXPR_CONSTRUCT:
        argument = expression->u.construct.arguments;
        break;
    case HLSL_EXPR_CAST:
        return HlslFindFunctionCallExpr(expression->u.cast.expression,
                                        function);
    case HLSL_EXPR_MEMBER:
        return HlslFindFunctionCallExpr(expression->u.member.object,
                                        function);
    case HLSL_EXPR_INDEX:
        call = HlslFindFunctionCallExpr(expression->u.index.object,
                                        function);
        return call != NULL ? call :
            HlslFindFunctionCallExpr(expression->u.index.index, function);
    case HLSL_EXPR_SWIZZLE:
        return HlslFindFunctionCallExpr(expression->u.swizzle.object,
                                        function);
    case HLSL_EXPR_SYMBOL:
    case HLSL_EXPR_INT:
    case HLSL_EXPR_FLOAT:
    case HLSL_EXPR_BOOL:
    default:
        return NULL;
    }
    for (; argument != NULL; argument = argument->next) {
        call = HlslFindFunctionCallExpr(argument, function);
        if (call != NULL)
            return call;
    }
    return NULL;
} // HlslFindFunctionCallExpr

static HlslExpr *HlslFindFunctionCallStatements(HlslStmt *statement,
                                                const HlslFunction *function)
{
    HlslExpr *call;

    for (; statement != NULL; statement = statement->next) {
        switch (statement->kind) {
        case HLSL_STMT_DECLARATION:
            call = statement->u.declaration != NULL ?
                HlslFindFunctionCallExpr(
                    statement->u.declaration->initializer, function) : NULL;
            break;
        case HLSL_STMT_EXPRESSION:
            call = HlslFindFunctionCallExpr(statement->u.expression,
                                            function);
            break;
        case HLSL_STMT_IF:
            call = HlslFindFunctionCallExpr(statement->u.ifStmt.condition,
                                            function);
            if (call == NULL)
                call = HlslFindFunctionCallStatements(
                           statement->u.ifStmt.trueBranch, function);
            if (call == NULL)
                call = HlslFindFunctionCallStatements(
                           statement->u.ifStmt.falseBranch, function);
            break;
        case HLSL_STMT_WHILE:
        case HLSL_STMT_DO:
            call = HlslFindFunctionCallExpr(statement->u.loop.condition,
                                            function);
            if (call == NULL)
                call = HlslFindFunctionCallStatements(statement->u.loop.body,
                                                       function);
            break;
        case HLSL_STMT_FOR:
            call = HlslFindFunctionCallStatements(statement->u.forStmt.init,
                                                   function);
            if (call == NULL)
                call = HlslFindFunctionCallExpr(
                           statement->u.forStmt.condition, function);
            if (call == NULL)
                call = HlslFindFunctionCallStatements(
                           statement->u.forStmt.step, function);
            if (call == NULL)
                call = HlslFindFunctionCallStatements(
                           statement->u.forStmt.body, function);
            break;
        case HLSL_STMT_BLOCK:
            call = HlslFindFunctionCallStatements(statement->u.block,
                                                   function);
            break;
        case HLSL_STMT_RETURN:
            call = HlslFindFunctionCallExpr(statement->u.returnExpr,
                                            function);
            break;
        default:
            call = NULL;
            break;
        }
        if (call != NULL)
            return call;
    }
    return NULL;
} // HlslFindFunctionCallStatements

static HlslDecl **HlslFindEntryParameter(HlslModule *module,
                                         const HlslBinding *binding,
                                         int *ordinal)
{
    HlslDecl **place;
    const void *identity;
    int index;

    if (module == NULL || module->entry == NULL || binding == NULL ||
        binding->declaration == NULL)
    {
        return NULL;
    }
    identity = binding->declaration->identity;
    index = 0;
    for (place = &module->entry->parameters; *place != NULL;
         place = &(*place)->next)
    {
        if ((*place)->identity == identity) {
            if (ordinal != NULL)
                *ordinal = index;
            return place;
        }
        index++;
    }
    return NULL;
} // HlslFindEntryParameter

static int HlslRemoveEntryArgument(HlslModule *module, int ordinal,
                                   const HlslBinding *binding)
{
    HlslExpr *call;
    HlslExpr **place;
    HlslExpr *argument;
    int index;

    if (module == NULL || module->wrapper == NULL || ordinal < 0)
        return 0;
    call = HlslFindFunctionCallStatements(module->wrapper->body,
                                           module->entry);
    if (call == NULL)
        return 0;
    index = 0;
    for (place = &call->u.call.arguments; *place != NULL;
         place = &(*place)->next)
    {
        if (index++ == ordinal) {
            argument = *place;
            if (argument->kind != HLSL_EXPR_SYMBOL ||
                argument->u.symbol != binding->declaration)
            {
                return 0;
            }
            *place = argument->next;
            argument->next = NULL;
            return 1;
        }
    }
    return 0;
} // HlslRemoveEntryArgument

static void HlslMarkPhysicalIntegerParameter(HlslModule *module,
                                             HlslBinding *binding)
{
    HlslDecl **parameter;

    if (binding == NULL || binding->physical.bank != HLSL_REGISTER_I)
    {
        return;
    }
    parameter = HlslFindEntryParameter(module, binding, NULL);
    if (parameter != NULL)
        (*parameter)->physical = binding->physical;
} // HlslMarkPhysicalIntegerParameter

static int HlslInsertBeforeFirstUse(HlslFunction *function,
                                    const HlslDecl *decl,
                                    HlslStmt *initializers)
{
    HlslStmt **place;
    HlslStmt *tail;

    if (function == NULL || decl == NULL || initializers == NULL)
        return 0;
    for (place = &function->body; *place != NULL;
         place = &(*place)->next)
    {
        if (HlslStatementUsesDecl(*place, decl)) {
            for (tail = initializers; tail->next != NULL;
                 tail = tail->next)
            {
            }
            tail->next = *place;
            *place = initializers;
            return 1;
        }
    }
    return 0;
} // HlslInsertBeforeFirstUse

static HlslDecl *HlslNewMixedBindingLocal(HlslModule *module,
                                          HlslFunction *function,
                                          const HlslBinding *binding,
                                          const HlslDecl *source)
{
    HlslDecl *local;
    const char *name;
    const char *sourceName;
    const void *nameSpace;
    HlslRegisterBank bank;

    if (module == NULL || function == NULL || binding == NULL ||
        source == NULL)
    {
        return NULL;
    }
    sourceName = HlslPublicBindingName(binding);
    nameSpace = function->identity != NULL ? function->identity : function;
    name = sourceName != NULL ?
        HlslAllocateScopedSymbolName(module, nameSpace,
                                     source->identity, sourceName) : NULL;
    local = name != NULL ?
        HlslNewDecl(module, HLSL_STORAGE_NONE, source->type, name) : NULL;
    if (local != NULL) {
        local->publicName = sourceName;
        local->loc = source->loc;
        local->sourceOrdinal = source->sourceOrdinal;
        local->identity = source->identity;
        local->parameterQualifier = HLSL_PARAMETER_IN;
        if (HlslHomogeneousBank(&binding->type, &bank))
            local->sourceBank = bank;
    }
    return local;
} // HlslNewMixedBindingLocal

static int HlslInstallMixedBindingConsumer(HlslModule *module,
                                           HlslFunction *function,
                                           HlslBinding *binding,
                                           HlslDecl *source)
{
    HlslBinding *leaf;
    HlslDecl *local;
    HlslStmt *initializers;

    if (!HlslStatementsUseDecl(function->body, source))
        return 1;
    local = HlslNewMixedBindingLocal(module, function, binding, source);
    if (local == NULL)
        return 0;
    HlslReplaceStatementDecl(function->body, source, local);
    initializers = NULL;
    leaf = binding->leafBindings;
    if (!HlslAppendMixedBindingCopy(module, &binding->type,
            HlslBindingSymbol(module, local), &leaf, &initializers) ||
        leaf != NULL || initializers == NULL)
    {
        return 0;
    }
    HlslAppendDecl(&function->locals, local);
    return HlslInsertBeforeFirstUse(function, local, initializers);
} // HlslInstallMixedBindingConsumer

static int HlslTypeIsPhysicalIntegerStorage(const HlslType *type)
{
    return type != NULL && type->arraySize == 0 &&
           type->base == HLSL_BASE_INT && type->len == 4 &&
           type->rows == 0 && type->cols == 0;
} // HlslTypeIsPhysicalIntegerStorage

static int HlslTypeIsScalarBoolean(const HlslType *type)
{
    return type != NULL && type->arraySize == 0 &&
           type->base == HLSL_BASE_BOOL && type->len == 1 &&
           type->rows == 0 && type->cols == 0;
} // HlslTypeIsScalarBoolean

static int HlslBindingTypeIsSampler(const HlslType *type)
{
    if (type == NULL)
        return 0;
    while (type->arraySize > 0)
        type = type->elementType;
    return type != NULL && type->base >= HLSL_BASE_SAMPLER1D &&
           type->base <= HLSL_BASE_SAMPLERCUBE;
} // HlslBindingTypeIsSampler

static int HlslBindingNeedsReconstruction(const HlslBinding *binding)
{
    return binding != NULL && binding->leafBindings != NULL &&
           (binding->leafBindings->next != NULL ||
            (binding->leafBindings->physical.bank == HLSL_REGISTER_I &&
             !HlslTypeIsPhysicalIntegerStorage(&binding->type)) ||
            (binding->leafBindings->physical.bank == HLSL_REGISTER_B &&
             !HlslTypeIsScalarBoolean(&binding->type)));
} // HlslBindingNeedsReconstruction

static int HlslInstallReconstructedBindingValue(HlslModule *module,
                                                HlslBinding *binding)
{
    HlslDecl **parameterPlace;
    HlslDecl *valueDecl;
    HlslFunction *function;
    int ordinal;

    if (module == NULL || module->entry == NULL || binding == NULL ||
        binding->declaration == NULL || binding->leafBindings == NULL)
    {
        return 0;
    }
    ordinal = -1;
    parameterPlace = HlslFindEntryParameter(module, binding, &ordinal);
    if (parameterPlace != NULL) {
        valueDecl = *parameterPlace;
        if (!HlslRemoveEntryArgument(module, ordinal, binding))
            return 0;
        *parameterPlace = valueDecl->next;
        valueDecl->next = NULL;
    } else {
        valueDecl = binding->declaration;
    }
    if (valueDecl == NULL ||
        (module->wrapper != NULL &&
         HlslStatementsUseDecl(module->wrapper->body, valueDecl)))
    {
        return 0;
    }
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function != module->wrapper &&
            !HlslInstallMixedBindingConsumer(module, function, binding,
                                             valueDecl))
        {
            return 0;
        }
    }
    /* A multi-leaf aggregate's identity declaration has been replaced by
       owned locals in every consumer.  Its physical leaves remain global,
       but the root must not remain as a dangling declaration. */
    if (binding->leafBindings->next != NULL)
        binding->declaration = NULL;
    return 1;
} // HlslInstallReconstructedBindingValue

static int HlslBindingIdentityUsesBank(const HlslModule *module,
                                       const HlslDecl *declaration,
                                       HlslRegisterBank bank)
{
    const HlslBinding *binding;
    const HlslBinding *leaf;
    const void *identity;

    if (module == NULL || declaration == NULL)
        return 0;
    if (declaration->physical.bank == bank)
        return 1;
    identity = declaration->identity;
    if (identity == NULL)
        return 0;
    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        for (leaf = binding->leafBindings; leaf != NULL;
             leaf = leaf->next)
        {
            if (leaf->physical.bank == bank && leaf->declaration != NULL &&
                leaf->declaration->identity == identity)
            {
                return 1;
            }
        }
    }
    return 0;
} // HlslBindingIdentityUsesBank

static const HlslDecl *HlslReferenceRootDeclaration(
    const HlslExpr *expression)
{
    while (expression != NULL && expression->kind != HLSL_EXPR_SYMBOL) {
        if (expression->kind == HLSL_EXPR_MEMBER)
            expression = expression->u.member.object;
        else if (expression->kind == HLSL_EXPR_INDEX)
            expression = expression->u.index.object;
        else if (expression->kind == HLSL_EXPR_SWIZZLE)
            expression = expression->u.swizzle.object;
        else
            return NULL;
    }
    return expression != NULL ? expression->u.symbol : NULL;
} // HlslReferenceRootDeclaration

static int HlslReferenceUsesBank(const HlslModule *module,
                                 const HlslExpr *expression,
                                 HlslRegisterBank bank)
{
    const HlslDecl *root;
    HlslRegisterBank typeBank;

    if (expression == NULL ||
        !HlslHomogeneousBank(&expression->type, &typeBank) ||
        typeBank != bank)
    {
        return 0;
    }
    root = HlslReferenceRootDeclaration(expression);
    return root != NULL &&
           (root->sourceBank == bank ||
            HlslBindingIdentityUsesBank(module, root, bank));
} // HlslReferenceUsesBank

static int HlslReferenceIdentityUsesBank(const HlslModule *module,
                                         const HlslExpr *expression,
                                         HlslRegisterBank bank)
{
    const HlslDecl *root;

    root = HlslReferenceRootDeclaration(expression);
    return root != NULL &&
           (root->sourceBank == bank ||
            HlslBindingIdentityUsesBank(module, root, bank));
} // HlslReferenceIdentityUsesBank

static int HlslExpressionUsesBank(const HlslModule *module,
                                  const HlslExpr *expression,
                                  HlslRegisterBank bank)
{
    const HlslExpr *argument;

    if (expression == NULL)
        return 0;
    switch (expression->kind) {
    case HLSL_EXPR_SYMBOL:
    case HLSL_EXPR_MEMBER:
    case HLSL_EXPR_SWIZZLE:
        return HlslReferenceUsesBank(module, expression, bank);
    case HLSL_EXPR_INDEX:
        return HlslReferenceUsesBank(module, expression, bank) ||
               HlslExpressionUsesBank(module,
                                      expression->u.index.index, bank);
    case HLSL_EXPR_UNARY:
        return HlslExpressionUsesBank(module,
                                      expression->u.unary.operand, bank);
    case HLSL_EXPR_BINARY:
        return HlslExpressionUsesBank(module,
                                      expression->u.binary.left, bank) ||
               HlslExpressionUsesBank(module,
                                      expression->u.binary.right, bank);
    case HLSL_EXPR_CONDITIONAL:
        return HlslExpressionUsesBank(
                   module, expression->u.conditional.condition, bank) ||
               HlslExpressionUsesBank(
                   module, expression->u.conditional.trueExpr, bank) ||
               HlslExpressionUsesBank(
                   module, expression->u.conditional.falseExpr, bank);
    case HLSL_EXPR_CALL:
        argument = expression->u.call.arguments;
        break;
    case HLSL_EXPR_CONSTRUCT:
        argument = expression->u.construct.arguments;
        break;
    case HLSL_EXPR_CAST:
        return HlslExpressionUsesBank(module,
                                      expression->u.cast.expression, bank);
    case HLSL_EXPR_INT:
    case HLSL_EXPR_FLOAT:
    case HLSL_EXPR_BOOL:
    default:
        return 0;
    }
    for (; argument != NULL; argument = argument->next) {
        if (HlslExpressionUsesBank(module, argument, bank))
            return 1;
    }
    return 0;
} // HlslExpressionUsesBank

static int HlslUnsupportedRegisterUse(HlslModule *module,
                                      const HlslLoc *loc,
                                      const char *reason)
{
    /* C6402 is the public profile-boundary diagnostic.  These source
       programs are legal Cg, but SM3 cannot represent the selected
       i#/b# data path faithfully. */
    return HlslFail(module, HLSL_ERROR_STAGE_OPERATION, loc, reason);
} // HlslUnsupportedRegisterUse

static int HlslExpressionValueUsesBank(const HlslModule *module,
                                       const HlslExpr *expression,
                                       HlslRegisterBank bank)
{
    HlslRegisterBank typeBank;

    return expression != NULL &&
           HlslHomogeneousBank(&expression->type, &typeBank) &&
           typeBank == bank &&
           HlslExpressionUsesBank(module, expression, bank);
} // HlslExpressionValueUsesBank

static HlslRegisterBank HlslExpressionSourceBank(
    const HlslModule *module, const HlslExpr *expression)
{
    const HlslExpr *argument;
    HlslRegisterBank bank;
    HlslRegisterBank homogeneousBank;

    if (expression == NULL)
        return HLSL_REGISTER_NONE;
    if (HlslExpressionUsesBank(module, expression, HLSL_REGISTER_I))
        return HLSL_REGISTER_I;
    if (HlslExpressionUsesBank(module, expression, HLSL_REGISTER_B))
        return HLSL_REGISTER_B;
    /* A whole mixed aggregate does not have one homogeneous bank, but an
       alias still carries constant-bank provenance.  One conservative
       marker is enough to reject it if it later crosses a helper return. */
    if (expression != NULL &&
        !HlslHomogeneousBank(&expression->type, &homogeneousBank))
    {
        if (HlslReferenceIdentityUsesBank(module, expression,
                                          HLSL_REGISTER_I))
        {
            return HLSL_REGISTER_I;
        }
        if (HlslReferenceIdentityUsesBank(module, expression,
                                          HLSL_REGISTER_B))
        {
            return HLSL_REGISTER_B;
        }
        /* Aggregate-valued operators can hide a mixed binding behind an
           alias.  Recurse only while the result remains non-homogeneous;
           a scalar member such as mixed.scalar must not inherit the I/B
           provenance of its containing binding. */
        argument = NULL;
        switch (expression->kind) {
        case HLSL_EXPR_UNARY:
            return HlslExpressionSourceBank(
                module, expression->u.unary.operand);
        case HLSL_EXPR_BINARY:
            bank = HlslExpressionSourceBank(
                module, expression->u.binary.left);
            return bank != HLSL_REGISTER_NONE ? bank :
                HlslExpressionSourceBank(
                    module, expression->u.binary.right);
        case HLSL_EXPR_CONDITIONAL:
            bank = HlslExpressionSourceBank(
                module, expression->u.conditional.trueExpr);
            return bank != HLSL_REGISTER_NONE ? bank :
                HlslExpressionSourceBank(
                    module, expression->u.conditional.falseExpr);
        case HLSL_EXPR_CALL:
            argument = expression->u.call.arguments;
            break;
        case HLSL_EXPR_CONSTRUCT:
            argument = expression->u.construct.arguments;
            break;
        case HLSL_EXPR_CAST:
            return HlslExpressionSourceBank(
                module, expression->u.cast.expression);
        case HLSL_EXPR_MEMBER:
            return HlslExpressionSourceBank(
                module, expression->u.member.object);
        case HLSL_EXPR_INDEX:
            return HlslExpressionSourceBank(
                module, expression->u.index.object);
        case HLSL_EXPR_SWIZZLE:
            return HlslExpressionSourceBank(
                module, expression->u.swizzle.object);
        case HLSL_EXPR_SYMBOL:
        case HLSL_EXPR_INT:
        case HLSL_EXPR_FLOAT:
        case HLSL_EXPR_BOOL:
        default:
            break;
        }
        for (; argument != NULL; argument = argument->next) {
            bank = HlslExpressionSourceBank(module, argument);
            if (bank != HLSL_REGISTER_NONE)
                return bank;
        }
    }
    return HLSL_REGISTER_NONE;
} // HlslExpressionSourceBank

static int HlslPropagateCallBanksExpression(HlslModule *module,
                                            HlslExpr *expression,
                                            int *changed);

static HlslDecl *HlslRegisterAssignmentRoot(HlslExpr *expression)
{
    while (expression != NULL && expression->kind != HLSL_EXPR_SYMBOL) {
        if (expression->kind == HLSL_EXPR_MEMBER)
            expression = expression->u.member.object;
        else if (expression->kind == HLSL_EXPR_INDEX)
            expression = expression->u.index.object;
        else if (expression->kind == HLSL_EXPR_SWIZZLE)
            expression = expression->u.swizzle.object;
        else
            return NULL;
    }
    return expression != NULL ? expression->u.symbol : NULL;
} // HlslRegisterAssignmentRoot

static int HlslPropagateCallBanksExpressionList(HlslModule *module,
                                                HlslExpr *expression,
                                                int *changed)
{
    for (; expression != NULL; expression = expression->next) {
        if (!HlslPropagateCallBanksExpression(module, expression, changed))
            return 0;
    }
    return 1;
} // HlslPropagateCallBanksExpressionList

static int HlslPropagateCallBanksExpression(HlslModule *module,
                                            HlslExpr *expression,
                                            int *changed)
{
    HlslExpr *argument;
    HlslDecl *parameter;
    HlslDecl *target;
    HlslRegisterBank bank;

    if (expression == NULL)
        return 1;
    switch (expression->kind) {
    case HLSL_EXPR_UNARY:
        return HlslPropagateCallBanksExpression(
            module, expression->u.unary.operand, changed);
    case HLSL_EXPR_BINARY:
        if (expression->u.binary.op == HLSL_OP_ASSIGN)
        {
            bank = HlslExpressionSourceBank(
                module, expression->u.binary.right);
            target = HlslRegisterAssignmentRoot(
                expression->u.binary.left);
            if (bank != HLSL_REGISTER_NONE && target != NULL &&
                target->sourceBank == HLSL_REGISTER_NONE)
            {
                target->sourceBank = bank;
                *changed = 1;
            }
        }
        return HlslPropagateCallBanksExpression(
                   module, expression->u.binary.left, changed) &&
               HlslPropagateCallBanksExpression(
                   module, expression->u.binary.right, changed);
    case HLSL_EXPR_CONDITIONAL:
        return HlslPropagateCallBanksExpression(
                   module, expression->u.conditional.condition, changed) &&
               HlslPropagateCallBanksExpression(
                   module, expression->u.conditional.trueExpr, changed) &&
               HlslPropagateCallBanksExpression(
                   module, expression->u.conditional.falseExpr, changed);
    case HLSL_EXPR_CALL:
        argument = expression->u.call.arguments;
        parameter = expression->u.call.function != NULL ?
                    expression->u.call.function->parameters : NULL;
        while (argument != NULL && parameter != NULL) {
            bank = HLSL_REGISTER_NONE;
            if (HlslExpressionUsesBank(module, argument, HLSL_REGISTER_I))
                bank = HLSL_REGISTER_I;
            else if (HlslExpressionUsesBank(module, argument,
                                            HLSL_REGISTER_B))
                bank = HLSL_REGISTER_B;
            if (argument->type.arraySize > 0 &&
                bank != HLSL_REGISTER_NONE &&
                parameter->sourceBank == HLSL_REGISTER_NONE)
            {
                parameter->sourceBank = bank;
                *changed = 1;
            }
            argument = argument->next;
            parameter = parameter->next;
        }
        return HlslPropagateCallBanksExpressionList(
            module, expression->u.call.arguments, changed);
    case HLSL_EXPR_CONSTRUCT:
        return HlslPropagateCallBanksExpressionList(
            module, expression->u.construct.arguments, changed);
    case HLSL_EXPR_CAST:
        return HlslPropagateCallBanksExpression(
            module, expression->u.cast.expression, changed);
    case HLSL_EXPR_MEMBER:
        return HlslPropagateCallBanksExpression(
            module, expression->u.member.object, changed);
    case HLSL_EXPR_INDEX:
        return HlslPropagateCallBanksExpression(
                   module, expression->u.index.object, changed) &&
               HlslPropagateCallBanksExpression(
                   module, expression->u.index.index, changed);
    case HLSL_EXPR_SWIZZLE:
        return HlslPropagateCallBanksExpression(
            module, expression->u.swizzle.object, changed);
    default:
        return 1;
    }
} // HlslPropagateCallBanksExpression

static int HlslPropagateCallBanksStatements(HlslModule *module,
                                            HlslStmt *statement,
                                            int *changed)
{
    HlslRegisterBank bank;

    for (; statement != NULL; statement = statement->next) {
        switch (statement->kind) {
        case HLSL_STMT_DECLARATION:
            if (statement->u.declaration != NULL &&
                statement->u.declaration->sourceBank ==
                    HLSL_REGISTER_NONE)
            {
                bank = HlslExpressionSourceBank(
                    module, statement->u.declaration->initializer);
                if (bank != HLSL_REGISTER_NONE) {
                    statement->u.declaration->sourceBank = bank;
                    *changed = 1;
                }
            }
            if (statement->u.declaration != NULL &&
                !HlslPropagateCallBanksExpression(module,
                    statement->u.declaration->initializer, changed))
                return 0;
            break;
        case HLSL_STMT_EXPRESSION:
            if (!HlslPropagateCallBanksExpression(
                    module, statement->u.expression, changed))
                return 0;
            break;
        case HLSL_STMT_IF:
            if (!HlslPropagateCallBanksExpression(
                    module, statement->u.ifStmt.condition, changed) ||
                !HlslPropagateCallBanksStatements(
                    module, statement->u.ifStmt.trueBranch, changed) ||
                !HlslPropagateCallBanksStatements(
                    module, statement->u.ifStmt.falseBranch, changed))
                return 0;
            break;
        case HLSL_STMT_WHILE:
        case HLSL_STMT_DO:
            if (!HlslPropagateCallBanksExpression(
                    module, statement->u.loop.condition, changed) ||
                !HlslPropagateCallBanksStatements(
                    module, statement->u.loop.body, changed))
                return 0;
            break;
        case HLSL_STMT_FOR:
            if (!HlslPropagateCallBanksStatements(
                    module, statement->u.forStmt.init, changed) ||
                !HlslPropagateCallBanksExpression(
                    module, statement->u.forStmt.condition, changed) ||
                !HlslPropagateCallBanksStatements(
                    module, statement->u.forStmt.step, changed) ||
                !HlslPropagateCallBanksStatements(
                    module, statement->u.forStmt.body, changed))
                return 0;
            break;
        case HLSL_STMT_BLOCK:
            if (!HlslPropagateCallBanksStatements(
                    module, statement->u.block, changed))
                return 0;
            break;
        case HLSL_STMT_RETURN:
            if (!HlslPropagateCallBanksExpression(
                    module, statement->u.returnExpr, changed))
                return 0;
            break;
        default:
            break;
        }
    }
    return 1;
} // HlslPropagateCallBanksStatements

static int HlslPropagateCallBanks(HlslModule *module)
{
    HlslFunction *function;
    int changed;

    do {
        changed = 0;
        for (function = module->functions; function != NULL;
             function = function->next)
        {
            if (!HlslPropagateCallBanksStatements(module, function->body,
                                                  &changed))
                return 0;
        }
    } while (changed);
    return 1;
} // HlslPropagateCallBanks

static HlslExpr *HlslRegisterSymbol(HlslModule *module, HlslDecl *decl)
{
    HlslExpr *expression;

    expression = decl != NULL ?
        HlslNewExpr(module, HLSL_EXPR_SYMBOL, decl->type) : NULL;
    if (expression != NULL)
        expression->u.symbol = decl;
    return expression;
} // HlslRegisterSymbol

static HlslExpr *HlslRegisterBinary(HlslModule *module, HlslOperator op,
                                    HlslType type, HlslExpr *left,
                                    HlslExpr *right)
{
    HlslExpr *expression;

    expression = left != NULL && right != NULL ?
        HlslNewExpr(module, HLSL_EXPR_BINARY, type) : NULL;
    if (expression != NULL) {
        expression->u.binary.op = op;
        expression->u.binary.left = left;
        expression->u.binary.right = right;
    }
    return expression;
} // HlslRegisterBinary

static HlslStmt *HlslRegisterExpressionStatement(HlslModule *module,
                                                 HlslExpr *expression)
{
    HlslStmt *statement;

    statement = expression != NULL ?
        HlslNewStmt(module, HLSL_STMT_EXPRESSION) : NULL;
    if (statement != NULL)
        statement->u.expression = expression;
    return statement;
} // HlslRegisterExpressionStatement

static HlslStmt *HlslRegisterAssignment(HlslModule *module,
                                        HlslExpr *target,
                                        HlslExpr *value)
{
    HlslExpr *assignment;

    assignment = target != NULL ?
        HlslRegisterBinary(module, HLSL_OP_ASSIGN, target->type,
                           target, value) : NULL;
    return HlslRegisterExpressionStatement(module, assignment);
} // HlslRegisterAssignment

static HlslStmt *HlslBuildBooleanControl(HlslModule *module,
                                         HlslExpr *condition,
                                         HlslStmt *trueBranch,
                                         HlslStmt *falseBranch)
{
    HlslStmt *outer;
    HlslStmt *nested;

    if (condition == NULL)
        return NULL;
    if (condition->kind == HLSL_EXPR_UNARY &&
        condition->u.unary.op == HLSL_OP_LOGICAL_NOT)
    {
        return HlslBuildBooleanControl(module,
            condition->u.unary.operand, falseBranch, trueBranch);
    }
    if (condition->kind == HLSL_EXPR_BINARY &&
        condition->u.binary.op == HLSL_OP_LOGICAL_AND)
    {
        nested = HlslBuildBooleanControl(module,
            condition->u.binary.right, trueBranch, falseBranch);
        return nested != NULL ? HlslBuildBooleanControl(module,
            condition->u.binary.left, nested, falseBranch) : NULL;
    }
    if (condition->kind == HLSL_EXPR_BINARY &&
        condition->u.binary.op == HLSL_OP_LOGICAL_OR)
    {
        nested = HlslBuildBooleanControl(module,
            condition->u.binary.right, trueBranch, falseBranch);
        return nested != NULL ? HlslBuildBooleanControl(module,
            condition->u.binary.left, trueBranch, nested) : NULL;
    }
    outer = HlslNewStmt(module, HLSL_STMT_IF);
    if (outer != NULL) {
        outer->loc = condition->loc;
        outer->u.ifStmt.condition = condition;
        outer->u.ifStmt.trueBranch = trueBranch;
        outer->u.ifStmt.falseBranch = falseBranch;
    }
    return outer;
} // HlslBuildBooleanControl

static int HlslBooleanControlNeedsExpansion(const HlslExpr *condition)
{
    if (condition == NULL)
        return 0;
    if (condition->kind == HLSL_EXPR_BINARY)
        return condition->u.binary.op == HLSL_OP_LOGICAL_AND ||
               condition->u.binary.op == HLSL_OP_LOGICAL_OR;
    if (condition->kind == HLSL_EXPR_UNARY &&
        condition->u.unary.op == HLSL_OP_LOGICAL_NOT)
    {
        return HlslBooleanControlNeedsExpansion(
            condition->u.unary.operand);
    }
    return 0;
} // HlslBooleanControlNeedsExpansion

static HlslDecl *HlslNewRegisterTemporary(HlslModule *module,
                                          HlslFunction *function,
                                          const HlslExpr *source,
                                          HlslType type,
                                          const char *baseName)
{
    HlslDecl *declaration;
    const void *nameSpace;
    void *identity;
    const char *name;

    if (module == NULL || function == NULL || source == NULL)
        return NULL;
    nameSpace = function->identity != NULL ? function->identity : function;
    identity = HlslBindAlloc(module, 1);
    name = identity != NULL ? HlslAllocateScopedSymbolName(
        module, nameSpace, identity, baseName) : NULL;
    declaration = name != NULL ?
        HlslNewDecl(module, HLSL_STORAGE_NONE, type, name) : NULL;
    if (declaration != NULL) {
        declaration->identity = identity;
        declaration->loc = source->loc;
        HlslAppendDecl(&function->locals, declaration);
    }
    return declaration;
} // HlslNewRegisterTemporary

static int HlslRewriteRegisterExpression(HlslModule *module,
    HlslFunction *function, HlslExpr **place, HlslStmt **prefix, int inLoop);

static int HlslRewriteRegisterExpressionList(HlslModule *module,
    HlslFunction *function, HlslExpr **place, HlslStmt **prefix, int inLoop)
{
    while (*place != NULL) {
        if (!HlslRewriteRegisterExpression(module, function, place,
                                            prefix, inLoop))
        {
            return 0;
        }
        place = &(*place)->next;
    }
    return 1;
} // HlslRewriteRegisterExpressionList

static int HlslExpressionHasIntegerIndex(const HlslModule *module,
                                         const HlslExpr *expression)
{
    const HlslExpr *argument;

    if (expression == NULL)
        return 0;
    switch (expression->kind) {
    case HLSL_EXPR_INDEX:
        return HlslExpressionUsesBank(module,
                   expression->u.index.index, HLSL_REGISTER_I) ||
               HlslExpressionHasIntegerIndex(
                   module, expression->u.index.object) ||
               HlslExpressionHasIntegerIndex(
                   module, expression->u.index.index);
    case HLSL_EXPR_MEMBER:
        return HlslExpressionHasIntegerIndex(
            module, expression->u.member.object);
    case HLSL_EXPR_SWIZZLE:
        return HlslExpressionHasIntegerIndex(
            module, expression->u.swizzle.object);
    case HLSL_EXPR_UNARY:
        return HlslExpressionHasIntegerIndex(
            module, expression->u.unary.operand);
    case HLSL_EXPR_BINARY:
        return HlslExpressionHasIntegerIndex(
                   module, expression->u.binary.left) ||
               HlslExpressionHasIntegerIndex(
                   module, expression->u.binary.right);
    case HLSL_EXPR_CONDITIONAL:
        return HlslExpressionHasIntegerIndex(
                   module, expression->u.conditional.condition) ||
               HlslExpressionHasIntegerIndex(
                   module, expression->u.conditional.trueExpr) ||
               HlslExpressionHasIntegerIndex(
                   module, expression->u.conditional.falseExpr);
    case HLSL_EXPR_CALL:
        argument = expression->u.call.arguments;
        break;
    case HLSL_EXPR_CONSTRUCT:
        argument = expression->u.construct.arguments;
        break;
    case HLSL_EXPR_CAST:
        return HlslExpressionHasIntegerIndex(
            module, expression->u.cast.expression);
    default:
        return 0;
    }
    for (; argument != NULL; argument = argument->next) {
        if (HlslExpressionHasIntegerIndex(module, argument))
            return 1;
    }
    return 0;
} // HlslExpressionHasIntegerIndex

static int HlslRewriteIntegerIndex(HlslModule *module,
                                   HlslFunction *function,
                                   HlslExpr **place, HlslStmt **prefix,
                                   int inLoop)
{
    HlslExpr *expression;

    (void)function;
    (void)prefix;
    (void)inLoop;
    expression = *place;
    if (expression == NULL || expression->kind != HLSL_EXPR_INDEX ||
        !HlslExpressionUsesBank(module, expression->u.index.index,
                                HLSL_REGISTER_I))
    {
        return 1;
    }
    return HlslUnsupportedRegisterUse(module, &expression->loc,
                                      "i-register indexed data path");
} // HlslRewriteIntegerIndex

static int HlslRewriteBooleanSelection(HlslModule *module,
                                       HlslFunction *function,
                                       HlslExpr **place,
                                       HlslStmt **prefix, int inLoop)
{
    HlslExpr *expression;
    HlslDecl *selected;
    HlslStmt *trueAssignment;
    HlslStmt *falseAssignment;
    HlslStmt *control;

    expression = *place;
    if (expression == NULL || expression->kind != HLSL_EXPR_CONDITIONAL ||
        !HlslExpressionUsesBank(module,
            expression->u.conditional.condition, HLSL_REGISTER_B))
    {
        return 1;
    }
    if (inLoop || expression->type.arraySize != 0 ||
        expression->type.base == HLSL_BASE_BOOL ||
        expression->type.base == HLSL_BASE_STRUCT ||
        HlslBindingTypeIsSampler(&expression->type))
    {
        return HlslUnsupportedRegisterUse(module, &expression->loc,
                                          "b-register conditional data path");
    }
    selected = HlslNewRegisterTemporary(module, function, expression,
                                        expression->type, "bank_select");
    trueAssignment = HlslRegisterAssignment(module,
        HlslRegisterSymbol(module, selected),
        expression->u.conditional.trueExpr);
    falseAssignment = HlslRegisterAssignment(module,
        HlslRegisterSymbol(module, selected),
        expression->u.conditional.falseExpr);
    control = selected != NULL ? HlslBuildBooleanControl(module,
        expression->u.conditional.condition,
        trueAssignment, falseAssignment) : NULL;
    if (trueAssignment == NULL || falseAssignment == NULL || control == NULL)
        return 0;
    HlslAppendStmt(prefix, control);
    *place = HlslRegisterSymbol(module, selected);
    if (*place == NULL)
        return 0;
    (*place)->loc = expression->loc;
    (*place)->next = expression->next;
    expression->next = NULL;
    return 1;
} // HlslRewriteBooleanSelection

static int HlslRewriteRegisterExpression(HlslModule *module,
    HlslFunction *function, HlslExpr **place, HlslStmt **prefix, int inLoop)
{
    HlslExpr *expression;
    HlslExpr *argument;
    HlslDecl *parameter;

    if (place == NULL || *place == NULL)
        return 1;
    expression = *place;
    if (expression->kind == HLSL_EXPR_BINARY &&
        expression->u.binary.op == HLSL_OP_ASSIGN &&
        HlslExpressionHasIntegerIndex(module,
                                      expression->u.binary.left))
    {
        return HlslUnsupportedRegisterUse(
            module, &expression->loc, "i-register indexed lvalue");
    }
    if (expression->kind == HLSL_EXPR_UNARY &&
        expression->u.unary.op >= HLSL_OP_PRE_INCREMENT &&
        expression->u.unary.op <= HLSL_OP_POST_DECREMENT &&
        HlslExpressionHasIntegerIndex(module,
                                      expression->u.unary.operand))
    {
        return HlslUnsupportedRegisterUse(
            module, &expression->loc, "i-register indexed lvalue");
    }
    switch (expression->kind) {
    case HLSL_EXPR_UNARY:
        if (!HlslRewriteRegisterExpression(module, function,
                &expression->u.unary.operand, prefix, inLoop))
        {
            return 0;
        }
        break;
    case HLSL_EXPR_BINARY:
        if (!HlslRewriteRegisterExpression(module, function,
                &expression->u.binary.left, prefix, inLoop) ||
            !HlslRewriteRegisterExpression(module, function,
                &expression->u.binary.right, prefix, inLoop))
        {
            return 0;
        }
        break;
    case HLSL_EXPR_CONDITIONAL:
        if (!HlslRewriteRegisterExpression(module, function,
                &expression->u.conditional.condition, prefix, inLoop) ||
            !HlslRewriteRegisterExpression(module, function,
                &expression->u.conditional.trueExpr, prefix, inLoop) ||
            !HlslRewriteRegisterExpression(module, function,
                &expression->u.conditional.falseExpr, prefix, inLoop))
        {
            return 0;
        }
        break;
    case HLSL_EXPR_CALL:
        argument = expression->u.call.arguments;
        parameter = expression->u.call.function != NULL ?
                    expression->u.call.function->parameters : NULL;
        while (argument != NULL && parameter != NULL) {
            if (parameter->parameterQualifier != HLSL_PARAMETER_IN &&
                HlslExpressionHasIntegerIndex(module, argument))
            {
                return HlslUnsupportedRegisterUse(
                    module, &argument->loc, "i-register indexed lvalue");
            }
            argument = argument->next;
            parameter = parameter->next;
        }
        if (!HlslRewriteRegisterExpressionList(module, function,
                &expression->u.call.arguments, prefix, inLoop))
        {
            return 0;
        }
        break;
    case HLSL_EXPR_CONSTRUCT:
        if (!HlslRewriteRegisterExpressionList(module, function,
                &expression->u.construct.arguments, prefix, inLoop))
        {
            return 0;
        }
        break;
    case HLSL_EXPR_CAST:
        if (!HlslRewriteRegisterExpression(module, function,
                &expression->u.cast.expression, prefix, inLoop))
        {
            return 0;
        }
        break;
    case HLSL_EXPR_MEMBER:
        if (!HlslRewriteRegisterExpression(module, function,
                &expression->u.member.object, prefix, inLoop))
        {
            return 0;
        }
        break;
    case HLSL_EXPR_INDEX:
        if (!HlslRewriteRegisterExpression(module, function,
                &expression->u.index.object, prefix, inLoop) ||
            !HlslRewriteRegisterExpression(module, function,
                &expression->u.index.index, prefix, inLoop))
        {
            return 0;
        }
        break;
    case HLSL_EXPR_SWIZZLE:
        if (!HlslRewriteRegisterExpression(module, function,
                &expression->u.swizzle.object, prefix, inLoop))
        {
            return 0;
        }
        break;
    default:
        break;
    }
    if (!HlslRewriteIntegerIndex(module, function, place, prefix, inLoop))
        return 0;
    return HlslRewriteBooleanSelection(module, function, place,
                                       prefix, inLoop);
} // HlslRewriteRegisterExpression

static int HlslRewriteRegisterStatements(HlslModule *module,
                                         HlslFunction *function,
                                         HlslStmt **statements,
                                         int inLoop)
{
    HlslStmt *source;
    HlslStmt *next;
    HlslStmt *prefix;
    HlslStmt *result;
    HlslStmt *rewritten;

    if (statements == NULL)
        return 1;
    source = *statements;
    result = NULL;
    while (source != NULL) {
        next = source->next;
        source->next = NULL;
        prefix = NULL;
        rewritten = source;
        switch (source->kind) {
        case HLSL_STMT_DECLARATION:
            if (source->u.declaration != NULL &&
                !HlslRewriteRegisterExpression(module, function,
                    &source->u.declaration->initializer, &prefix, inLoop))
            {
                return 0;
            }
            break;
        case HLSL_STMT_EXPRESSION:
            if (!HlslRewriteRegisterExpression(module, function,
                    &source->u.expression, &prefix, inLoop))
            {
                return 0;
            }
            break;
        case HLSL_STMT_IF:
            if (!HlslRewriteRegisterStatements(module, function,
                    &source->u.ifStmt.trueBranch, inLoop) ||
                !HlslRewriteRegisterStatements(module, function,
                    &source->u.ifStmt.falseBranch, inLoop) ||
                !HlslRewriteRegisterExpression(module, function,
                    &source->u.ifStmt.condition, &prefix, inLoop))
            {
                return 0;
            }
            if (HlslExpressionUsesBank(module,
                    source->u.ifStmt.condition, HLSL_REGISTER_B) &&
                HlslBooleanControlNeedsExpansion(
                    source->u.ifStmt.condition))
            {
                rewritten = HlslBuildBooleanControl(module,
                    source->u.ifStmt.condition,
                    source->u.ifStmt.trueBranch,
                    source->u.ifStmt.falseBranch);
                if (rewritten == NULL)
                    return 0;
            }
            break;
        case HLSL_STMT_WHILE:
        case HLSL_STMT_DO:
            /* A prefix inserted into the body remains inside the repeated
               region.  Conditions cannot be prefixed here because that
               would evaluate them only once before the loop. */
            if (!HlslRewriteRegisterStatements(module, function,
                    &source->u.loop.body, 0) ||
                !HlslRewriteRegisterExpression(module, function,
                    &source->u.loop.condition, &prefix, 1))
            {
                return 0;
            }
            break;
        case HLSL_STMT_FOR:
            if (!HlslRewriteRegisterStatements(module, function,
                    &source->u.forStmt.init, 1) ||
                !HlslRewriteRegisterExpression(module, function,
                    &source->u.forStmt.condition, &prefix, 1) ||
                !HlslRewriteRegisterStatements(module, function,
                    &source->u.forStmt.step, 1) ||
                !HlslRewriteRegisterStatements(module, function,
                    &source->u.forStmt.body, 0))
            {
                return 0;
            }
            break;
        case HLSL_STMT_BLOCK:
            if (!HlslRewriteRegisterStatements(module, function,
                                                &source->u.block, inLoop))
                return 0;
            break;
        case HLSL_STMT_RETURN:
            if (!HlslRewriteRegisterExpression(module, function,
                    &source->u.returnExpr, &prefix, inLoop))
            {
                return 0;
            }
            break;
        default:
            break;
        }
        HlslAppendStmt(&result, prefix);
        HlslAppendStmt(&result, rewritten);
        source = next;
    }
    *statements = result;
    return 1;
} // HlslRewriteRegisterStatements

static int HlslValidateRegisterExpression(HlslModule *module,
    const HlslExpr *expression, int controlFlags)
{
    const HlslExpr *argument;
    HlslRegisterBank argumentBank;
    int usesInteger;
    int usesBoolean;

    if (expression == NULL)
        return 1;
    usesInteger = HlslExpressionUsesBank(module, expression,
                                         HLSL_REGISTER_I);
    usesBoolean = HlslExpressionUsesBank(module, expression,
                                         HLSL_REGISTER_B);
    switch (expression->kind) {
    case HLSL_EXPR_UNARY:
        if (!HlslValidateRegisterExpression(module,
                expression->u.unary.operand, controlFlags))
        {
            return 0;
        }
        if (usesBoolean &&
            expression->u.unary.op == HLSL_OP_LOGICAL_NOT &&
            (controlFlags & 2) != 0)
        {
            return 1;
        }
        if (usesInteger || usesBoolean)
            return HlslUnsupportedRegisterUse(module, &expression->loc,
                usesInteger ? "i-register arithmetic data path" :
                              "b-register data path");
        return 1;
    case HLSL_EXPR_BINARY:
        if (!HlslValidateRegisterExpression(module,
                expression->u.binary.left, controlFlags) ||
            !HlslValidateRegisterExpression(module,
                expression->u.binary.right, controlFlags))
        {
            return 0;
        }
        if (expression->u.binary.op == HLSL_OP_ASSIGN &&
            HlslExpressionValueUsesBank(module,
                expression->u.binary.right, HLSL_REGISTER_I) &&
            expression->loc.line > 0)
        {
            return HlslUnsupportedRegisterUse(module, &expression->loc,
                                              "i-register assignment data path");
        }
        if ((expression->u.binary.op == HLSL_OP_LOGICAL_AND ||
             expression->u.binary.op == HLSL_OP_LOGICAL_OR) && usesBoolean)
        {
            return HlslUnsupportedRegisterUse(module, &expression->loc,
                                              "b-register logical data path");
        }
        if ((expression->u.binary.op == HLSL_OP_EQUAL ||
             expression->u.binary.op == HLSL_OP_NOT_EQUAL) && usesBoolean)
        {
            return HlslUnsupportedRegisterUse(
                module, &expression->loc,
                "b-register comparison data path");
        }
        if (expression->u.binary.op != HLSL_OP_ASSIGN && usesInteger &&
            !((controlFlags & 1) != 0 &&
              expression->u.binary.op >= HLSL_OP_EQUAL &&
              expression->u.binary.op <= HLSL_OP_GREATER_EQUAL))
        {
            return HlslUnsupportedRegisterUse(module, &expression->loc,
                expression->u.binary.op >= HLSL_OP_BITWISE_OR &&
                expression->u.binary.op <= HLSL_OP_SHIFT_RIGHT ?
                "i-register bitwise data path" :
                "i-register arithmetic data path");
        }
        return 1;
    case HLSL_EXPR_CONDITIONAL:
        if (!HlslValidateRegisterExpression(module,
                expression->u.conditional.condition, controlFlags) ||
            !HlslValidateRegisterExpression(module,
                expression->u.conditional.trueExpr, controlFlags) ||
            !HlslValidateRegisterExpression(module,
                expression->u.conditional.falseExpr, controlFlags))
        {
            return 0;
        }
        if (usesInteger || usesBoolean)
            return HlslUnsupportedRegisterUse(module, &expression->loc,
                usesInteger ? "i-register conditional data path" :
                              "b-register conditional data path");
        return 1;
    case HLSL_EXPR_CALL:
        argument = expression->u.call.arguments;
        for (; argument != NULL; argument = argument->next) {
            if (!HlslValidateRegisterExpression(module, argument,
                                                 controlFlags))
                return 0;
            if (expression->loc.line > 0 &&
                !HlslHomogeneousBank(&argument->type, &argumentBank) &&
                (HlslReferenceIdentityUsesBank(
                     module, argument, HLSL_REGISTER_I) ||
                 HlslReferenceIdentityUsesBank(
                     module, argument, HLSL_REGISTER_B)))
            {
                return HlslUnsupportedRegisterUse(module, &expression->loc,
                    "mixed constant-bank helper argument");
            }
            if (expression->loc.line > 0 && argument->type.arraySize == 0 &&
                HlslExpressionUsesBank(module, argument, HLSL_REGISTER_I))
            {
                return HlslUnsupportedRegisterUse(
                    module, &expression->loc, "i-register helper argument");
            }
            if (expression->loc.line > 0 && argument->type.arraySize == 0 &&
                HlslExpressionUsesBank(module, argument, HLSL_REGISTER_B))
            {
                return HlslUnsupportedRegisterUse(
                    module, &expression->loc, "b-register helper argument");
            }
        }
        return 1;
    case HLSL_EXPR_CONSTRUCT:
        argument = expression->u.construct.arguments;
        for (; argument != NULL; argument = argument->next) {
            if (!HlslValidateRegisterExpression(module, argument,
                                                 controlFlags))
                return 0;
        }
        if (expression->loc.line > 0 && (usesInteger || usesBoolean))
            return HlslUnsupportedRegisterUse(module, &expression->loc,
                usesInteger ? "i-register construction data path" :
                              "b-register construction data path");
        return 1;
    case HLSL_EXPR_CAST:
        if (!HlslValidateRegisterExpression(module,
                expression->u.cast.expression, controlFlags))
        {
            return 0;
        }
        if (usesInteger || usesBoolean)
            return HlslUnsupportedRegisterUse(module, &expression->loc,
                usesInteger ? "i-register cast data path" :
                              "b-register cast data path");
        return 1;
    case HLSL_EXPR_INDEX:
        if (!HlslValidateRegisterExpression(module,
                expression->u.index.object, controlFlags) ||
            !HlslValidateRegisterExpression(module,
                expression->u.index.index, controlFlags))
        {
            return 0;
        }
        if (HlslExpressionUsesBank(module, expression->u.index.index,
                                   HLSL_REGISTER_I))
        {
            return HlslUnsupportedRegisterUse(module, &expression->loc,
                                              "i-register indexed data path");
        }
        return 1;
    case HLSL_EXPR_MEMBER:
        return HlslValidateRegisterExpression(module,
            expression->u.member.object, controlFlags);
    case HLSL_EXPR_SWIZZLE:
        return HlslValidateRegisterExpression(module,
            expression->u.swizzle.object, controlFlags);
    default:
        return 1;
    }
} // HlslValidateRegisterExpression

static int HlslValidateRegisterStatements(HlslModule *module,
                                          const HlslStmt *statement,
                                          int inLoop)
{
    HlslRegisterBank returnBank;

    for (; statement != NULL; statement = statement->next) {
        switch (statement->kind) {
        case HLSL_STMT_DECLARATION:
            if (statement->u.declaration != NULL &&
                !HlslValidateRegisterExpression(module,
                    statement->u.declaration->initializer, 0))
                return 0;
            break;
        case HLSL_STMT_EXPRESSION:
            if (!HlslValidateRegisterExpression(module,
                                                 statement->u.expression, 0))
                return 0;
            break;
        case HLSL_STMT_IF:
            if (!HlslValidateRegisterExpression(module,
                    statement->u.ifStmt.condition, 2) ||
                !HlslValidateRegisterStatements(module,
                    statement->u.ifStmt.trueBranch, inLoop) ||
                !HlslValidateRegisterStatements(module,
                    statement->u.ifStmt.falseBranch, inLoop))
            {
                return 0;
            }
            break;
        case HLSL_STMT_WHILE:
        case HLSL_STMT_DO:
            if (!HlslValidateRegisterExpression(module,
                    statement->u.loop.condition, 3) ||
                !HlslValidateRegisterStatements(module,
                    statement->u.loop.body, 1))
            {
                return 0;
            }
            break;
        case HLSL_STMT_FOR:
            if (!HlslValidateRegisterStatements(module,
                    statement->u.forStmt.init, 1) ||
                !HlslValidateRegisterExpression(module,
                    statement->u.forStmt.condition, 3) ||
                !HlslValidateRegisterStatements(module,
                    statement->u.forStmt.step, 1) ||
                !HlslValidateRegisterStatements(module,
                    statement->u.forStmt.body, 1))
            {
                return 0;
            }
            break;
        case HLSL_STMT_BLOCK:
            if (!HlslValidateRegisterStatements(module,
                                                 statement->u.block, inLoop))
                return 0;
            break;
        case HLSL_STMT_RETURN:
            if (statement->u.returnExpr != NULL &&
                !HlslHomogeneousBank(&statement->u.returnExpr->type,
                                     &returnBank) &&
                HlslExpressionSourceBank(module,
                    statement->u.returnExpr) != HLSL_REGISTER_NONE)
            {
                return HlslUnsupportedRegisterUse(module,
                    &statement->u.returnExpr->loc,
                    "mixed constant-bank helper return");
            }
            if (HlslExpressionValueUsesBank(module,
                    statement->u.returnExpr, HLSL_REGISTER_I))
            {
                return HlslUnsupportedRegisterUse(module,
                    &statement->u.returnExpr->loc,
                    "i-register helper return");
            }
            if (HlslExpressionValueUsesBank(module,
                    statement->u.returnExpr, HLSL_REGISTER_B))
            {
                return HlslUnsupportedRegisterUse(module,
                    &statement->u.returnExpr->loc,
                    "b-register helper return");
            }
            if (!HlslValidateRegisterExpression(module,
                    statement->u.returnExpr, 0))
                return 0;
            break;
        default:
            break;
        }
    }
    return 1;
} // HlslValidateRegisterStatements

static int HlslLegalizeRegisterUses(HlslModule *module)
{
    HlslFunction *function;

    if (!HlslPropagateCallBanks(module))
        return 0;
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (!HlslRewriteRegisterStatements(module, function,
                                            &function->body, 0) ||
            !HlslValidateRegisterStatements(module, function->body, 0))
        {
            return 0;
        }
    }
    return 1;
} // HlslLegalizeRegisterUses

int HlslAllocateBindings(HlslModule *module,
                         const HlslProfileDesc *profile)
{
    HlslBinding **ordered;
    HlslBinding *binding;
    HlslBinding *swap;
    int count;
    int i;
    int j;
    int explicitPass;

    if (module == NULL || profile == NULL || profile->limits == NULL ||
        module->stage != profile->stage)
    {
        return HlslBindFailure(module, NULL, HLSL_ERROR_INVALID_IR,
                               "invalid HLSL binding module");
    }
    count = HlslBindingCount(module->bindings);
    if (count < 0)
        return HlslBindFailure(module, NULL, HLSL_ERROR_INVALID_IR,
                               "cyclic HLSL binding list");
    if (count == 0)
        return 1;
    ordered = (HlslBinding **) HlslBindAlloc(module,
        (size_t) count * sizeof(HlslBinding *));
    if (ordered == NULL)
        return HlslBindFailure(module, NULL, HLSL_ERROR_INVALID_IR,
                               "HLSL binding order");
    i = 0;
    for (binding = module->bindings; binding != NULL; binding = binding->next)
        ordered[i++] = binding;
    for (i = 1; i < count; i++) {
        swap = ordered[i];
        j = i;
        while (j > 0 &&
               ordered[j - 1]->sourceOrdinal > swap->sourceOrdinal)
        {
            ordered[j] = ordered[j - 1];
            j--;
        }
        ordered[j] = swap;
    }
    for (explicitPass = 1; explicitPass >= 0; explicitPass--) {
        for (i = 0; i < count; i++) {
            if (!!ordered[i]->hasExplicitRegister == explicitPass &&
                !HlslAllocateOneBinding(module, profile, ordered[i]))
            {
                return 0;
            }
        }
    }
    for (i = 0; i < count; i++) {
        HlslMarkPhysicalIntegerParameter(module, ordered[i]);
        if (module->entry != NULL &&
            HlslBindingNeedsReconstruction(ordered[i]) &&
            !HlslInstallReconstructedBindingValue(module, ordered[i]))
        {
            return HlslBindFailure(module, ordered[i],
                                   HLSL_ERROR_INVALID_IR,
                                   "mixed HLSL binding value");
        }
    }
    return HlslLegalizeRegisterUses(module);
} // HlslAllocateBindings

static HlslExpr *HlslWrapperSymbol(HlslModule *module, HlslDecl *decl)
{
    HlslExpr *expression;

    expression = HlslNewExpr(module, HLSL_EXPR_SYMBOL, decl->type);
    if (expression != NULL)
        expression->u.symbol = decl;
    return expression;
} // HlslWrapperSymbol

static HlslExpr *HlslWrapperMember(HlslModule *module, HlslDecl *object,
                                   HlslDecl *member)
{
    HlslExpr *expression;

    expression = HlslNewExpr(module, HLSL_EXPR_MEMBER, member->type);
    if (expression != NULL) {
        expression->u.member.object = object != NULL ?
                                      HlslWrapperSymbol(module, object) :
                                      NULL;
        expression->u.member.decl = member;
        expression->u.member.name = member->name;
        if (object != NULL && expression->u.member.object == NULL)
            return NULL;
    }
    return expression;
} // HlslWrapperMember

static HlslExpr *HlslWrapperAssign(HlslModule *module, HlslExpr *left,
                                   HlslExpr *right)
{
    HlslExpr *expression;

    if (left == NULL || right == NULL)
        return NULL;
    expression = HlslNewExpr(module, HLSL_EXPR_BINARY, left->type);
    if (expression != NULL) {
        expression->u.binary.op = HLSL_OP_ASSIGN;
        expression->u.binary.left = left;
        expression->u.binary.right = right;
    }
    return expression;
} // HlslWrapperAssign

static HlslExpr *HlslWrapperConvert(HlslModule *module, HlslExpr *source,
                                    HlslType targetType)
{
    HlslExpr *cast;

    if (source == NULL)
        return NULL;
    if (source->type.base == targetType.base &&
        source->type.len == targetType.len &&
        source->type.rows == targetType.rows &&
        source->type.cols == targetType.cols)
    {
        return source;
    }
    cast = HlslNewExpr(module, HLSL_EXPR_CAST, targetType);
    if (cast != NULL)
        cast->u.cast.expression = source;
    return cast;
} // HlslWrapperConvert

static int HlslWrapperRepairInput(HlslModule *module, HlslExpr *expression,
                                  HlslDecl *input)
{
    if (expression == NULL)
        return 0;
    if (expression->kind == HLSL_EXPR_CAST)
        return HlslWrapperRepairInput(module,
                                      expression->u.cast.expression, input);
    if (expression->kind == HLSL_EXPR_MEMBER &&
        expression->u.member.object == NULL)
    {
        expression->u.member.object = HlslWrapperSymbol(module, input);
        return expression->u.member.object != NULL;
    }
    return 1;
} // HlslWrapperRepairInput

static const char *HlslWrapperCopyText(HlslModule *module,
                                       const char *source)
{
    char *copy;
    size_t length;

    if (module == NULL || module->alloc == NULL || source == NULL)
        return NULL;
    length = strlen(source) + 1;
    copy = (char *) module->alloc(module->allocArg, length);
    if (copy != NULL)
        memcpy(copy, source, length);
    return copy;
} // HlslWrapperCopyText

static int HlslWrapperModernSemantic(HlslModule *module,
    const HlslProfileDesc *profile, HlslDecl *decl, const char *source,
    HlslDirection direction)
{
    char upper[64];
    char root[64];
    char spelling[96];
    size_t i;
    HlslSemanticKind kind;
    HlslBase abiBase;
    int index;

    if (profile->semanticPolicy != HLSL_SEMANTIC_POLICY_MODERN)
        return 1;
    if (source == NULL)
        return 0;
    for (i = 0; source[i] != '\0'; i++) {
        if (i + 1 >= sizeof(upper))
            return 0;
        upper[i] = source[i] >= 'a' && source[i] <= 'z' ?
                   (char) (source[i] - 'a' + 'A') : source[i];
    }
    upper[i] = '\0';
    if (!HlslParseSemantic(upper, root, sizeof(root), &index))
        return 0;
    kind = HlslModernSemantic(profile->stage, direction, root, index);
    if (kind == HLSL_SEMANTIC_UNSUPPORTED)
        return 0;
    if (kind == HLSL_SEMANTIC_USER) {
        if (sprintf(spelling, "%s%d", root, index) < 0)
            return 0;
    } else if (!HlslModernSemanticSpelling(kind, index, spelling,
                                           sizeof(spelling)))
    {
        return 0;
    }
    decl->semantic = HlslWrapperCopyText(module, spelling);
    decl->canonicalSemantic = HlslWrapperCopyText(module, spelling);
    if (decl->semantic == NULL || decl->canonicalSemantic == NULL)
        return 0;
    decl->semanticKind = kind;
    decl->semanticIndex = index;
    decl->interpolation = decl->type.base == HLSL_BASE_INT ||
                          decl->type.base == HLSL_BASE_UINT ?
                          HLSL_INTERPOLATION_NOINTERPOLATION :
                          HLSL_INTERPOLATION_DEFAULT;
    if (kind != HLSL_SEMANTIC_USER) {
        abiBase = HlslModernAbiBase(kind);
        if (abiBase == HLSL_BASE_UINT || abiBase == HLSL_BASE_BOOL)
            decl->type.base = abiBase;
    }
    return 1;
} // HlslWrapperModernSemantic

static HlslStmt *HlslWrapperExprStmt(HlslModule *module,
                                     HlslExpr *expression)
{
    HlslStmt *statement;

    if (expression == NULL)
        return NULL;
    statement = HlslNewStmt(module, HLSL_STMT_EXPRESSION);
    if (statement != NULL)
        statement->u.expression = expression;
    return statement;
} // HlslWrapperExprStmt

static HlslDecl *HlslWrapperMemberDecl(HlslModule *module,
                                       const HlslProfileDesc *profile,
                                       HlslDecl *owner,
                                       const HlslDecl *source,
                                       const char *name,
                                       const char *semantic)
{
    HlslDecl *member;
    const char *emittedName;
    const void *identity;

    identity = source->identity != NULL ? source->identity : source;
    emittedName = HlslAllocateScopedSymbolName(module, owner, identity,
                                                name);
    member = emittedName != NULL ?
             HlslNewDecl(module, HLSL_STORAGE_NONE, source->type,
                         emittedName) : NULL;
    if (member != NULL) {
        member->identity = source->identity;
        member->publicName = source->publicName;
        member->semantic = semantic;
        member->inputSemantic = source->inputSemantic;
        member->loc = source->loc;
        member->sourceOrdinal = source->sourceOrdinal;
        if (!HlslWrapperModernSemantic(module, profile, member, semantic,
            owner->storage == HLSL_STORAGE_OUTPUT ?
                HLSL_DIRECTION_OUTPUT : HLSL_DIRECTION_INPUT))
        {
            return NULL;
        }
    }
    return member;
} // HlslWrapperMemberDecl

static HlslBinding *HlslWrapperFindBinding(HlslModule *module,
                                           const void *identity)
{
    HlslBinding *binding;

    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (binding->declaration != NULL &&
            binding->declaration->identity == identity)
        {
            return binding;
        }
    }
    return NULL;
} // HlslWrapperFindBinding

static const char *HlslWrapperInputSemantic(
    const HlslProfileDesc *profile, const HlslDecl *parameter)
{
    (void) profile;
    return parameter->inputSemantic != NULL ? parameter->inputSemantic :
                                              parameter->semantic;
} // HlslWrapperInputSemantic

static const char *HlslWrapperScalarResultSemantic(
    const HlslProfileDesc *profile, const HlslFunction *entry)
{
    (void) profile;
    return entry->semantic;
} // HlslWrapperScalarResultSemantic

static const char *HlslWrapperDeclSemantic(
    const HlslProfileDesc *profile, const HlslDecl *decl, int isOutput)
{
    const char *source;

    (void) profile;
    if (decl == NULL)
        return NULL;
    source = !isOutput && decl->inputSemantic != NULL ?
             decl->inputSemantic : decl->semantic;
    return source;
} // HlslWrapperDeclSemantic

static int HlslWrapperIsEmptyEntry(const HlslModule *module)
{
    const HlslFunction *entry;

    entry = module != NULL ? module->entry : NULL;
    return entry != NULL && !strcmp(entry->name, "main") &&
           entry->result.base == HLSL_BASE_VOID &&
           entry->parameters == NULL;
} // HlslWrapperIsEmptyEntry

static int hlslVertexInputIdentity;
static int hlslVertexOutputIdentity;
static int hlslPixelInputIdentity;
static int hlslPixelOutputIdentity;

int HlslBuildEntryWrapper(HlslModule *module,
                          const HlslProfileDesc *profile)
{
    HlslFunction *entry;
    HlslFunction *wrapper;
    HlslDecl *inputStruct;
    HlslDecl *outputStruct;
    HlslDecl *inputParameter;
    HlslDecl *outputLocal;
    HlslDecl *resultLocal;
    HlslDecl *parameter;
    HlslDecl *inputMember;
    HlslDecl *outputMember;
    HlslDecl *resultMember;
    HlslDecl *wrapperResultMember;
    HlslDecl *local;
    HlslBinding *binding;
    HlslExpr *arguments;
    HlslExpr *argument;
    HlslExpr *call;
    HlslExpr *assignment;
    HlslStmt *statement;
    HlslStmt *initializers;
    HlslStmt *parameterCopies;
    HlslType inputType;
    HlslType outputType;
    const char *inputName;
    const char *memberName;
    const char *outputName;
    const char *semantic;

    if (module == NULL || profile == NULL || module->entry == NULL ||
        module->stage != profile->stage)
    {
        return HlslBindFailure(module, NULL, HLSL_ERROR_INVALID_IR,
                               "invalid HLSL wrapper module");
    }
    if (HlslWrapperIsEmptyEntry(module))
        return 1;
    entry = module->entry;
    inputName = HlslAllocateGeneratedName(module,
        module->stage == HLSL_STAGE_VERTEX ?
            (const void *) &hlslVertexInputIdentity :
            (const void *) &hlslPixelInputIdentity,
        module->stage == HLSL_STAGE_VERTEX ?
            "cg_VertexIn" : "cg_PixelIn");
    outputName = HlslAllocateGeneratedName(module,
        module->stage == HLSL_STAGE_VERTEX ?
            (const void *) &hlslVertexOutputIdentity :
            (const void *) &hlslPixelOutputIdentity,
        module->stage == HLSL_STAGE_VERTEX ?
            "cg_VertexOut" : "cg_PixelOut");
    if (inputName == NULL || outputName == NULL)
        return HlslBindFailure(module, NULL, HLSL_ERROR_NAME_COLLISION,
                               "HLSL wrapper interface name");
    inputType = HlslNumericType(HLSL_BASE_STRUCT, 0);
    inputType.structName = inputName;
    outputType = HlslNumericType(HLSL_BASE_STRUCT, 0);
    outputType.structName = outputName;
    inputStruct = HlslNewDecl(module, HLSL_STORAGE_INPUT,
                              inputType, inputName);
    outputStruct = HlslNewDecl(module, HLSL_STORAGE_OUTPUT,
                               outputType, outputName);
    if (inputStruct == NULL || outputStruct == NULL)
        return HlslBindFailure(module, NULL, HLSL_ERROR_INVALID_IR,
                               "HLSL wrapper structures");

    if (entry->result.base == HLSL_BASE_STRUCT) {
        for (resultMember = entry->result.members; resultMember != NULL;
             resultMember = resultMember->next)
        {
            semantic = HlslWrapperDeclSemantic(profile, resultMember, 1);
            if (semantic == NULL)
                return HlslBindFailure(module, NULL, HLSL_ERROR_SEMANTIC,
                                       resultMember->name);
            wrapperResultMember = HlslWrapperMemberDecl(module, profile,
                outputStruct, resultMember, resultMember->name,
                semantic);
            if (wrapperResultMember == NULL)
                return 0;
            HlslAppendDecl(&outputStruct->members, wrapperResultMember);
        }
    } else if (entry->result.base != HLSL_BASE_VOID) {
        semantic = HlslWrapperScalarResultSemantic(profile, entry);
        if (semantic == NULL)
            return HlslBindFailure(module, NULL, HLSL_ERROR_SEMANTIC,
                                   "entry result");
        memberName = HlslAllocateScopedSymbolName(module, outputStruct,
                                                  entry, "result");
        wrapperResultMember = memberName != NULL ?
            HlslNewDecl(module, HLSL_STORAGE_NONE, entry->result,
                        memberName) : NULL;
        if (wrapperResultMember == NULL)
            return 0;
        wrapperResultMember->semantic = semantic;
        if (!HlslWrapperModernSemantic(module, profile,
                                       wrapperResultMember, semantic,
                                       HLSL_DIRECTION_OUTPUT))
        {
            return HlslBindFailure(module, NULL, HLSL_ERROR_SEMANTIC,
                                   semantic);
        }
        HlslAppendDecl(&outputStruct->members, wrapperResultMember);
    }

    arguments = NULL;
    initializers = NULL;
    parameterCopies = NULL;
    for (parameter = entry->parameters; parameter != NULL;
         parameter = parameter->next)
    {
        if (parameter->storage == HLSL_STORAGE_UNIFORM ||
            parameter->storage == HLSL_STORAGE_SAMPLER)
        {
            binding = HlslWrapperFindBinding(module, parameter->identity);
            if (binding == NULL || binding->declaration == NULL)
                return HlslBindFailure(module, NULL,
                                       HLSL_ERROR_INVALID_IR,
                                       "entry uniform binding");
            argument = HlslWrapperSymbol(module, binding->declaration);
        } else if (parameter->parameterQualifier == HLSL_PARAMETER_IN &&
                   parameter->type.base == HLSL_BASE_STRUCT)
        {
            HlslDecl *sourceMember;

            local = HlslNewDecl(module, HLSL_STORAGE_NONE,
                                parameter->type, parameter->name);
            if (local == NULL)
                return 0;
            local->identity = parameter->identity;
            argument = HlslWrapperSymbol(module, local);
            for (sourceMember = parameter->type.members;
                 sourceMember != NULL; sourceMember = sourceMember->next)
            {
                semantic = HlslWrapperDeclSemantic(profile, sourceMember,
                                                   0);
                inputMember = HlslWrapperMemberDecl(module, profile, inputStruct,
                    sourceMember, sourceMember->name, semantic);
                if (inputMember == NULL || semantic == NULL)
                    return HlslBindFailure(module, NULL,
                                           HLSL_ERROR_SEMANTIC,
                                           sourceMember->name);
                HlslAppendDecl(&inputStruct->members, inputMember);
                statement = HlslWrapperExprStmt(module,
                    HlslWrapperAssign(module,
                        HlslWrapperMember(module, local, sourceMember),
                        HlslWrapperConvert(module,
                            HlslWrapperMember(module, NULL, inputMember),
                            sourceMember->type)));
                if (statement == NULL)
                    return 0;
                HlslAppendStmt(&initializers, statement);
            }
        } else if (parameter->parameterQualifier == HLSL_PARAMETER_IN) {
            inputMember = HlslWrapperMemberDecl(module, profile, inputStruct,
                parameter, parameter->name, parameter->semantic);
            if (inputMember == NULL)
                return 0;
            HlslAppendDecl(&inputStruct->members, inputMember);
            argument = HlslWrapperConvert(module,
                HlslWrapperMember(module, NULL, inputMember),
                parameter->type);
            /* Fill the object once inputParameter exists below. */
        } else {
            local = HlslNewDecl(module, HLSL_STORAGE_NONE,
                                parameter->type, parameter->name);
            if (local == NULL)
                return 0;
            local->identity = parameter->identity;
            /* Wrapper locals are installed after wrapper creation. */
            argument = HlslWrapperSymbol(module, local);
            if (parameter->parameterQualifier == HLSL_PARAMETER_INOUT) {
                semantic = HlslWrapperInputSemantic(profile, parameter);
                inputMember = HlslWrapperMemberDecl(module, profile, inputStruct,
                    parameter, parameter->name, semantic);
                if (inputMember == NULL || semantic == NULL)
                    return HlslBindFailure(module, NULL,
                                           HLSL_ERROR_SEMANTIC,
                                           parameter->name);
                HlslAppendDecl(&inputStruct->members, inputMember);
                statement = HlslWrapperExprStmt(module,
                    HlslWrapperAssign(module,
                        HlslWrapperSymbol(module, local),
                        HlslWrapperConvert(module,
                            HlslWrapperMember(module, NULL, inputMember),
                            local->type)));
                if (statement == NULL)
                    return 0;
                HlslAppendStmt(&initializers, statement);
            }
            outputMember = HlslWrapperMemberDecl(module, profile, outputStruct,
                parameter, parameter->name, parameter->semantic);
            if (outputMember == NULL)
                return 0;
            HlslAppendDecl(&outputStruct->members, outputMember);
            statement = HlslWrapperExprStmt(module,
                HlslWrapperAssign(module,
                    HlslWrapperMember(module, NULL, outputMember),
                    HlslWrapperConvert(module,
                        HlslWrapperSymbol(module, local),
                        outputMember->type)));
            if (statement == NULL)
                return 0;
            /* Stash the local temporarily on the argument expression. */
            argument->u.symbol = local;
            HlslAppendStmt(&parameterCopies, statement);
        }
        if (argument == NULL)
            return 0;
        HlslAppendExpr(&arguments, argument);
    }

    inputStruct->type.members = inputStruct->members;
    outputStruct->type.members = outputStruct->members;
    inputType.members = inputStruct->members;
    outputType.members = outputStruct->members;
    if (inputStruct->members == NULL || outputStruct->members == NULL)
        return HlslBindFailure(module, NULL, HLSL_ERROR_ENTRY_ABI,
                               "empty HLSL wrapper interface");
    HlslAppendDecl(&module->structs, inputStruct);
    HlslAppendDecl(&module->structs, outputStruct);

    wrapper = HlslNewFunction(module, outputType, "main");
    inputParameter = HlslNewDecl(module, HLSL_STORAGE_INPUT,
                                 inputType, "input");
    outputLocal = HlslNewDecl(module, HLSL_STORAGE_NONE,
                              outputType, "output");
    if (wrapper == NULL || inputParameter == NULL || outputLocal == NULL)
        return 0;
    wrapper->identity = wrapper;
    inputParameter->identity = inputParameter;
    outputLocal->identity = outputLocal;
    HlslAppendDecl(&wrapper->parameters, inputParameter);

    /* Repair the deferred input/output member object expressions and move
     * out/inout temporaries into the wrapper's local list. */
    for (argument = arguments; argument != NULL; argument = argument->next) {
        if (!HlslWrapperRepairInput(module, argument, inputParameter))
            return 0;
        if (argument->kind == HLSL_EXPR_SYMBOL &&
                   argument->u.symbol != NULL &&
                   argument->u.symbol->storage == HLSL_STORAGE_NONE &&
                   argument->u.symbol != outputLocal)
        {
            HlslAppendDecl(&wrapper->locals, argument->u.symbol);
        }
    }
    for (statement = initializers; statement != NULL;
         statement = statement->next)
    {
        HlslExpr *right;

        right = statement->u.expression->u.binary.right;
        if (!HlslWrapperRepairInput(module, right, inputParameter))
            return 0;
    }
    HlslAppendDecl(&wrapper->locals, outputLocal);
    HlslAppendStmt(&wrapper->body, initializers);

    call = HlslNewExpr(module, HLSL_EXPR_CALL, entry->result);
    if (call == NULL)
        return 0;
    call->u.call.function = entry;
    call->u.call.name = entry->name;
    call->u.call.arguments = arguments;
    if (entry->result.base == HLSL_BASE_STRUCT) {
        resultLocal = HlslNewDecl(module, HLSL_STORAGE_NONE,
                                  entry->result, "cg_result");
        if (resultLocal == NULL)
            return 0;
        HlslAppendDecl(&wrapper->locals, resultLocal);
        statement = HlslWrapperExprStmt(module,
            HlslWrapperAssign(module, HlslWrapperSymbol(module, resultLocal),
                              call));
        if (statement == NULL)
            return 0;
        HlslAppendStmt(&wrapper->body, statement);
        resultMember = entry->result.members;
        wrapperResultMember = outputStruct->members;
        while (resultMember != NULL && wrapperResultMember != NULL) {
            assignment = HlslWrapperAssign(module,
                HlslWrapperMember(module, outputLocal,
                                  wrapperResultMember),
                HlslWrapperConvert(module,
                    HlslWrapperMember(module, resultLocal, resultMember),
                    wrapperResultMember->type));
            statement = HlslWrapperExprStmt(module, assignment);
            if (statement == NULL)
                return 0;
            HlslAppendStmt(&wrapper->body, statement);
            resultMember = resultMember->next;
            wrapperResultMember = wrapperResultMember->next;
        }
    } else if (entry->result.base != HLSL_BASE_VOID) {
        statement = HlslWrapperExprStmt(module,
            HlslWrapperAssign(module,
                HlslWrapperMember(module, outputLocal,
                                  outputStruct->members),
                HlslWrapperConvert(module, call,
                    outputStruct->members->type)));
        if (statement == NULL)
            return 0;
        HlslAppendStmt(&wrapper->body, statement);
    } else {
        statement = HlslWrapperExprStmt(module, call);
        if (statement == NULL)
            return 0;
        HlslAppendStmt(&wrapper->body, statement);
    }
    /* Repair deferred output objects before appending post-call copies. */
    for (statement = parameterCopies; statement != NULL;
         statement = statement->next)
    {
        HlslExpr *left;

        left = statement->u.expression->u.binary.left;
        if (left != NULL && left->kind == HLSL_EXPR_MEMBER &&
            left->u.member.object == NULL)
        {
            left->u.member.object = HlslWrapperSymbol(module, outputLocal);
        }
    }
    HlslAppendStmt(&wrapper->body, parameterCopies);
    statement = HlslNewStmt(module, HLSL_STMT_RETURN);
    if (statement == NULL)
        return 0;
    statement->u.returnExpr = HlslWrapperSymbol(module, outputLocal);
    if (statement->u.returnExpr == NULL)
        return 0;
    HlslAppendStmt(&wrapper->body, statement);
    module->wrapper = wrapper;
    HlslAppendFunction(&module->functions, wrapper);
    return 1;
} // HlslBuildEntryWrapper
