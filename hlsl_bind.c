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
    if (module != NULL && module->errors == 0) {
        module->errorKind = kind;
        module->errorReason = reason;
        if (binding != NULL)
            module->errorLoc = binding->loc;
    }
    if (module != NULL)
        module->errors++;
    return 0;
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

static int HlslNonEntryUsesDecl(const HlslModule *module,
                                const HlslDecl *decl)
{
    const HlslFunction *function;

    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function != module->entry && function != module->wrapper &&
            HlslStatementsUseDecl(function->body, decl))
        {
            return 1;
        }
    }
    return 0;
} // HlslNonEntryUsesDecl

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

static int HlslInstallMixedBindingValue(HlslModule *module,
                                        HlslBinding *binding)
{
    HlslBinding *leaf;
    HlslDecl **parameterPlace;
    HlslDecl *valueDecl;
    HlslStmt *initializers;
    int ordinal;
    int used;

    if (module == NULL || module->entry == NULL || binding == NULL ||
        binding->declaration == NULL || binding->leafBindings == NULL ||
        binding->leafBindings->next == NULL)
    {
        return 0;
    }
    ordinal = -1;
    parameterPlace = HlslFindEntryParameter(module, binding, &ordinal);
    if (parameterPlace != NULL) {
        valueDecl = *parameterPlace;
        used = HlslStatementsUseDecl(module->entry->body, valueDecl);
        if (!HlslRemoveEntryArgument(module, ordinal, binding))
            return 0;
        *parameterPlace = valueDecl->next;
        valueDecl->next = NULL;
        if (!used)
            return 1;
    } else {
        valueDecl = binding->declaration;
        used = HlslStatementsUseDecl(module->entry->body, valueDecl);
        if (!used)
            return !HlslNonEntryUsesDecl(module, valueDecl);
    }
    if (HlslNonEntryUsesDecl(module, valueDecl))
        return 0;
    valueDecl->storage = HLSL_STORAGE_NONE;
    valueDecl->parameterQualifier = HLSL_PARAMETER_IN;
    valueDecl->initializer = NULL;
    initializers = NULL;
    leaf = binding->leafBindings;
    if (!HlslAppendMixedBindingCopy(module, &binding->type,
            HlslBindingSymbol(module, valueDecl), &leaf,
            &initializers) || leaf != NULL || initializers == NULL)
    {
        return 0;
    }
    HlslAppendDecl(&module->entry->locals, valueDecl);
    return HlslInsertBeforeFirstUse(module->entry, valueDecl,
                                    initializers);
} // HlslInstallMixedBindingValue

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
        if (module->entry != NULL &&
            ordered[i]->leafBindings != NULL &&
            ordered[i]->leafBindings->next != NULL &&
            !HlslInstallMixedBindingValue(module, ordered[i]))
        {
            return HlslBindFailure(module, ordered[i],
                                   HLSL_ERROR_INVALID_IR,
                                   "mixed HLSL binding value");
        }
    }
    return 1;
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
            wrapperResultMember = HlslWrapperMemberDecl(module,
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
                inputMember = HlslWrapperMemberDecl(module, inputStruct,
                    sourceMember, sourceMember->name, semantic);
                if (inputMember == NULL || semantic == NULL)
                    return HlslBindFailure(module, NULL,
                                           HLSL_ERROR_SEMANTIC,
                                           sourceMember->name);
                HlslAppendDecl(&inputStruct->members, inputMember);
                statement = HlslWrapperExprStmt(module,
                    HlslWrapperAssign(module,
                        HlslWrapperMember(module, local, sourceMember),
                        HlslWrapperMember(module, NULL, inputMember)));
                if (statement == NULL)
                    return 0;
                HlslAppendStmt(&initializers, statement);
            }
        } else if (parameter->parameterQualifier == HLSL_PARAMETER_IN) {
            inputMember = HlslWrapperMemberDecl(module, inputStruct,
                parameter, parameter->name, parameter->semantic);
            if (inputMember == NULL)
                return 0;
            HlslAppendDecl(&inputStruct->members, inputMember);
            argument = HlslWrapperMember(module, NULL, inputMember);
            /* Fill the object once inputParameter exists below. */
            if (argument != NULL)
                argument->u.member.object = NULL;
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
                inputMember = HlslWrapperMemberDecl(module, inputStruct,
                    parameter, parameter->name, semantic);
                if (inputMember == NULL || semantic == NULL)
                    return HlslBindFailure(module, NULL,
                                           HLSL_ERROR_SEMANTIC,
                                           parameter->name);
                HlslAppendDecl(&inputStruct->members, inputMember);
                statement = HlslWrapperExprStmt(module,
                    HlslWrapperAssign(module,
                        HlslWrapperSymbol(module, local),
                        HlslWrapperMember(module, NULL, inputMember)));
                if (statement == NULL)
                    return 0;
                HlslAppendStmt(&initializers, statement);
            }
            outputMember = HlslWrapperMemberDecl(module, outputStruct,
                parameter, parameter->name, parameter->semantic);
            if (outputMember == NULL)
                return 0;
            HlslAppendDecl(&outputStruct->members, outputMember);
            statement = HlslWrapperExprStmt(module,
                HlslWrapperAssign(module,
                    HlslWrapperMember(module, NULL, outputMember),
                    HlslWrapperSymbol(module, local)));
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
        if (argument->kind == HLSL_EXPR_MEMBER &&
            argument->u.member.object == NULL)
        {
            argument->u.member.object = HlslWrapperSymbol(module,
                                                           inputParameter);
        } else if (argument->kind == HLSL_EXPR_SYMBOL &&
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
        if (right != NULL && right->kind == HLSL_EXPR_MEMBER &&
            right->u.member.object == NULL)
        {
            right->u.member.object = HlslWrapperSymbol(module,
                                                        inputParameter);
        }
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
                HlslWrapperMember(module, resultLocal, resultMember));
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
                                  outputStruct->members), call));
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
