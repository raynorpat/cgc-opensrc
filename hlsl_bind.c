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

static int HlslAppendLeafPlan(HlslLeafPlan **list, HlslLeafPlan *plan)
{
    HlslLeafPlan *current;

    if (list == NULL || plan == NULL)
        return 0;
    if (*list == NULL) {
        *list = plan;
        return 1;
    }
    for (current = *list; current->next != NULL; current = current->next)
        ;
    current->next = plan;
    return 1;
} // HlslAppendLeafPlan

static int HlslPlanLeaves(HlslModule *module, const HlslType *type,
                          const char *path, int recursiveOffset,
                          int defaultOffset, HlslLeafPlan **plans)
{
    const HlslDecl *member;
    HlslLeafPlan *plan;
    const char *memberPath;
    int componentCount;
    int memberComponents;
    int memberSpan;
    int span;

    span = HlslTypeRegisterSpan(type);
    if (span <= 0)
        return 0;
    if (type->arraySize == 0 && type->base == HLSL_BASE_STRUCT) {
        for (member = type->members; member != NULL; member = member->next) {
            memberPath = HlslJoinPublicPath(module, path, member->name);
            memberSpan = HlslTypeRegisterSpan(&member->type);
            memberComponents = HlslTypeComponentCount(&member->type);
            if (memberPath == NULL || memberSpan <= 0 ||
                memberComponents <= 0 ||
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

    plan = (HlslLeafPlan *) HlslBindAlloc(module, sizeof(HlslLeafPlan));
    componentCount = HlslTypeComponentCount(type);
    if (plan == NULL || path == NULL)
        return 0;
    plan->type = *type;
    plan->publicName = path;
    plan->recursiveOffset = recursiveOffset;
    plan->defaultOffset = defaultOffset;
    plan->defaultCount = componentCount;
    plan->bank = HlslTypeBank(type);
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
                                       const HlslBinding *binding,
                                       HlslRegisterBank bank, int regno)
{
    const char *name;
    const char *bankName;
    char number[32];
    char *reason;
    size_t length;

    name = binding != NULL && binding->name != NULL ?
           binding->name : "anonymous";
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
                                       first < 0 ? plan->span :
                                       first + plan->span, limit);
        for (i = 0; i < plan->span; i++) {
            if (used[first + i]) {
                return HlslBindFailure(module, binding,
                    HLSL_ERROR_REGISTER_COLLISION,
                    HlslCollisionReason(module, binding, plan->bank,
                                        first + i));
            }
        }
    } else {
        first = HlslFindRange(used, limit, plan->span);
        if (first < 0) {
            return HlslResourceFailure(module, binding, plan->bank,
                HlslUsedCount(used, limit) + plan->span, limit);
        }
    }
    plan->regno = first;
    HlslMarkRange(used, first, plan->span);
    return 1;
} // HlslReservePlan

static char *HlslGlobalSourceName(HlslModule *module, const char *path)
{
    char *name;
    char *current;
    size_t length;

    if (path == NULL)
        return NULL;
    length = strlen(path);
    name = (char *) HlslBindAlloc(module, length + 1);
    if (name == NULL)
        return NULL;
    memcpy(name, path, length + 1);
    for (current = name; *current != '\0'; current++) {
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

static HlslExpr *HlslBuildDefaultInitializer(HlslModule *module,
                                             const HlslType *type,
                                             const float *values,
                                             int count)
{
    HlslExpr *initializer;
    HlslExpr *value;
    HlslType scalarType;
    HlslBase base;
    int i;

    if (type == NULL || values == NULL || count <= 0 ||
        type->arraySize > 0 || type->base == HLSL_BASE_STRUCT)
    {
        return NULL;
    }
    base = HlslDefaultBase(type);
    if (base != HLSL_BASE_FLOAT && base != HLSL_BASE_INT &&
        base != HLSL_BASE_BOOL)
    {
        return NULL;
    }
    initializer = HlslNewExpr(module, HLSL_EXPR_CONSTRUCT, *type);
    if (initializer == NULL)
        return NULL;
    scalarType = HlslNumericType(base, 1);
    for (i = 0; i < count; i++) {
        if (base == HLSL_BASE_FLOAT) {
            value = HlslNewExpr(module, HLSL_EXPR_FLOAT, scalarType);
            if (value != NULL)
                value->u.literalFloat = values[i];
        } else if (base == HLSL_BASE_INT) {
            value = HlslNewExpr(module, HLSL_EXPR_INT, scalarType);
            if (value != NULL)
                value->u.literalInt = (int) values[i];
        } else {
            value = HlslNewExpr(module, HLSL_EXPR_BOOL, scalarType);
            if (value != NULL)
                value->u.literalBool = values[i] != 0.0f;
        }
        if (value == NULL)
            return NULL;
        HlslAppendExpr(&initializer->u.construct.arguments, value);
    }
    return initializer;
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

    for (plan = plans; plan != NULL; plan = plan->next) {
        record = HlslNewBinding(module, binding->storage, plan->type,
                                plan->publicName, binding->semantic);
        sourceName = HlslGlobalSourceName(module, plan->publicName);
        emittedName = sourceName != NULL ?
                      HlslAllocateDistinctName(module, sourceName) : NULL;
        declaration = emittedName != NULL ?
            HlslNewDecl(module, binding->storage, plan->type, emittedName) :
            NULL;
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
            if (binding->defaultValues == NULL || plan->defaultCount <= 0 ||
                plan->defaultOffset > binding->defaultCount ||
                plan->defaultCount >
                    binding->defaultCount - plan->defaultOffset)
            {
                return 0;
            }
            defaults = (float *) HlslBindAlloc(module,
                (size_t) plan->defaultCount * sizeof(float));
            if (defaults == NULL)
                return 0;
            memcpy(defaults, binding->defaultValues + plan->defaultOffset,
                   (size_t) plan->defaultCount * sizeof(float));
            record->defaultCount = plan->defaultCount;
            record->defaultValues = defaults;
            declaration->initializer = HlslBuildDefaultInitializer(
                module, &plan->type, defaults, plan->defaultCount);
        }
        declaration->semantic = binding->semantic;
        declaration->loc = binding->loc;
        declaration->sourceOrdinal = binding->sourceOrdinal;
        declaration->physical = record->physical;
        HlslAppendBinding(records, record);
        HlslAppendDecl(globals, declaration);
    }
    return 1;
} // HlslBuildLeafRecords

int HlslAllocateOneBinding(HlslModule *module,
                           const HlslProfileDesc *profile,
                           HlslBinding *binding)
{
    HlslBankState state;
    HlslLeafPlan *plans;
    HlslLeafPlan *plan;
    HlslBinding *records;
    HlslBinding *record;
    HlslBinding *nextRecord;
    HlslDecl *globals;
    int defaultComponents;
    int explicitRegno;
    int planCount;

    if (module == NULL || profile == NULL || profile->limits == NULL ||
        binding == NULL || binding->name == NULL ||
        module->stage != profile->stage || binding->isAllocated)
    {
        return HlslBindFailure(module, binding, HLSL_ERROR_INVALID_IR,
                               "invalid HLSL binding");
    }
    defaultComponents = HlslTypeComponentCount(&binding->type);
    if (binding->defaultCount < 0 ||
        (binding->defaultCount > 0 &&
         (binding->defaultValues == NULL ||
          binding->defaultCount != defaultComponents)))
    {
        return HlslBindFailure(module, binding, HLSL_ERROR_INVALID_IR,
                               "invalid HLSL binding default");
    }
    plans = NULL;
    if (!HlslPlanLeaves(module, &binding->type, binding->name, 0, 0,
                        &plans))
    {
        return HlslBindFailure(module, binding,
                               HLSL_ERROR_UNSUPPORTED_TYPE, binding->name);
    }
    planCount = 0;
    for (plan = plans; plan != NULL; plan = plan->next)
        planCount++;
    if (binding->hasExplicitRegister) {
        if (binding->physical.bank == HLSL_REGISTER_NONE ||
            (binding->physical.span != 0 && planCount == 1 &&
             binding->physical.span != plans->span))
        {
            return HlslBindFailure(module, binding, HLSL_ERROR_INVALID_IR,
                                   binding->name);
        }
        for (plan = plans; plan != NULL; plan = plan->next) {
            if (plan->bank != binding->physical.bank)
                return HlslBindFailure(module, binding,
                    HLSL_ERROR_UNSUPPORTED_TYPE, binding->name);
        }
    }

    HlslLoadBankState(module, &state);
    for (plan = plans; plan != NULL; plan = plan->next) {
        if (binding->hasExplicitRegister &&
            (binding->physical.regno < 0 ||
             binding->physical.regno > INT_MAX - plan->recursiveOffset))
        {
            explicitRegno = -1;
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
    if (!HlslBuildLeafRecords(module, binding, plans, &records, &globals))
        return HlslBindFailure(module, binding, HLSL_ERROR_INVALID_IR,
                               "HLSL binding allocation");

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
    return 1;
} // HlslAllocateBindings
