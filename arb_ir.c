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
express or implied, are granted by NVIDIA herein including but not
limited to any patent rights that may be infringed by your derivative
works. No hardware is licensed hereunder.

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
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

//
// arb_ir.c - Allocation, interning, construction, validation, and
//        destruction for the private ARB vector IR.
//

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NO_PARSER 1
#include "slglobals.h"
#include "arb_ir.h"

// IR instructions and constants are owned by the program itself rather
// than by any parser scope; restore the plain allocator the pool macros
// in memory.h replaced.

#undef malloc
#undef calloc
#undef realloc
#undef free

/*
 * Source counts indexed by ArbOpcode.  KIL is handled specially: it is a
 * texture-class instruction with no destination and one source.
 */

static int FindConstant(const ArbProgram *program, int index, float *out);

static const signed char sourceCounts[ARB_OP_LAST] = {
    1, 2, 1, 3, 1, 2, 2, 2, 2, 1,
    1, 1, 1, 1, 1, 1, 1, 3, 3, 2,
    2, 1, 2, 2, 1, 1, 1, 2, 1, 2,
    2, 1, 1, 1, 1, 2
};

// Opcode classification shared by validation and later resource checks:

int ArbIsTextureOpcode(ArbOpcode opcode)
{
    switch (opcode) {
    case ARB_OP_TEX:
    case ARB_OP_TXB:
    case ARB_OP_TXP:
    case ARB_OP_KIL:
        return 1;
    default:
        return 0;
    }
} // ArbIsTextureOpcode

static int IsVertexOnlyRejected(ArbOpcode opcode)
{
    switch (opcode) {
    case ARB_OP_CMP:
    case ARB_OP_COS:
    case ARB_OP_KIL:
    case ARB_OP_LRP:
    case ARB_OP_SCS:
    case ARB_OP_SIN:
    case ARB_OP_TEX:
    case ARB_OP_TXB:
    case ARB_OP_TXP:
        return 1;
    default:
        return 0;
    }
} // IsVertexOnlyRejected

static int IsFragmentOnlyRejected(ArbOpcode opcode)
{
    switch (opcode) {
    case ARB_OP_ARL:
    case ARB_OP_EXP:
    case ARB_OP_LOG:
        return 1;
    default:
        return 0;
    }
} // IsFragmentOnlyRejected

static void InitOperand(ArbOperand *operand, ArbRegisterFile file, int index)
{
    operand->file = file;
    operand->index = index;
    operand->bindingName = 0;
    operand->swizzle[0] = 0;
    operand->swizzle[1] = 1;
    operand->swizzle[2] = 2;
    operand->swizzle[3] = 3;
    operand->negate = 0;
    operand->absolute = 0;
    operand->relative = 0;
    operand->relativeOffset = 0;
} // InitOperand

/*
 * ArbInitProgram() - Prepare an empty program for one stage.
 */

void ArbInitProgram(ArbProgram *program, ArbStage stage)
{
    memset(program, 0, sizeof(ArbProgram));
    program->stage = stage;
    program->maxTextureUnitUsed = -1;
} // ArbInitProgram

/*
 * ArbFreeProgram() - Release every instruction and interned constant.
 */

void ArbFreeProgram(ArbProgram *program)
{
    ArbInstruction *inst = program->first;
    ArbConstant *cnst = program->constants;

    while (inst) {
        ArbInstruction *next = inst->next;
        free(inst);
        inst = next;
    }
    while (cnst) {
        ArbConstant *next = cnst->next;
        free(cnst);
        cnst = next;
    }
    memset(program, 0, sizeof(ArbProgram));
} // ArbFreeProgram

/*
 * ArbNewTemp() - Allocate a virtual temporary index.
 */

int ArbNewTemp(ArbProgram *program)
{
    return program->numVirtualTemps++;
} // ArbNewTemp

ArbOperand ArbTempOperand(int index)
{
    ArbOperand operand;
    InitOperand(&operand, ARB_REG_TEMP, index);
    return operand;
} // ArbTempOperand

ArbOperand ArbInputOperand(int index)
{
    ArbOperand operand;
    InitOperand(&operand, ARB_REG_INPUT, index);
    return operand;
} // ArbInputOperand

ArbOperand ArbOutputOperand(int index)
{
    ArbOperand operand;
    InitOperand(&operand, ARB_REG_OUTPUT, index);
    return operand;
} // ArbOutputOperand

ArbOperand ArbParamOperand(int index)
{
    ArbOperand operand;
    InitOperand(&operand, ARB_REG_PARAM, index);
    return operand;
} // ArbParamOperand

ArbOperand ArbConstOperand(int index)
{
    ArbOperand operand;
    InitOperand(&operand, ARB_REG_CONST, index);
    return operand;
} // ArbConstOperand

/*
 * ArbAppendInstruction() - Append an instruction with an initialized
 *         destination and no sources yet.
 */

ArbInstruction *ArbAppendInstruction(ArbProgram *program, ArbOpcode opcode,
                                     const SourceLoc *loc, ArbOperand dst)
{
    ArbInstruction *inst = (ArbInstruction *) malloc(sizeof(ArbInstruction));

    if (!inst)
        return NULL;
    memset(inst, 0, sizeof(ArbInstruction));
    inst->opcode = opcode;
    if (loc)
        inst->loc = *loc;
    else {
        inst->loc.file = 0;
        inst->loc.line = 0;
    }
    inst->dst = dst;
    inst->mask = ARB_MASK_XYZW;
    inst->saturate = 0;
    inst->textureUnit = -1;
    inst->textureTarget = ARB_TEX_NONE;
    inst->physicalTemp = -1;
    inst->srcCount = 0;
    inst->next = NULL;
    if (program->last) {
        program->last->next = inst;
    } else {
        program->first = inst;
    }
    program->last = inst;
    program->numInstructions++;
    return inst;
} // ArbAppendInstruction

/*
 * ArbAddSource() - Append one initialized source operand.  Returns zero
 *         once an instruction already carries its maximum of three.
 */

int ArbAddSource(ArbInstruction *instruction, ArbOperand source)
{
    if (instruction->srcCount >= 3)
        return 0;
    instruction->src[instruction->srcCount++] = source;
    return 1;
} // ArbAddSource

/*
 * ArbInternConstant() - Return the index of a PARAM constant holding
 *         "value".  A scalar is expanded to all four components.  Values
 *         compare on all four floats plus the recorded size so repeated
 *         constants share one declaration.
 */

int ArbInternConstant(ArbProgram *program, const float *value, int size)
{
    ArbConstant *cnst = program->constants;
    float expanded[4];
    int index = 0;
    int ii;

    if (size == 1) {
        // Scalars expand to all four components.
        expanded[0] = value[0];
        expanded[1] = value[0];
        expanded[2] = value[0];
        expanded[3] = value[0];
        value = expanded;
        size = 4;
    } else if (size < 4) {
        for (ii = 0; ii < size && ii < 4; ii++)
            expanded[ii] = value[ii];
        for (; ii < 4; ii++)
            expanded[ii] = 0.0f;
        value = expanded;
        size = 4;
    } else if (size > 4) {
        size = 4;
    }
    while (cnst) {
        if (cnst->size == size &&
            cnst->value[0] == value[0] &&
            cnst->value[1] == value[1] &&
            cnst->value[2] == value[2] &&
            cnst->value[3] == value[3])
        {
            return cnst->index;
        }
        index = cnst->index + 1;
        cnst = cnst->next;
    }
    cnst = (ArbConstant *) malloc(sizeof(ArbConstant));
    if (!cnst)
        return -1;
    cnst->next = NULL;
    for (ii = 0; ii < 4; ii++)
        cnst->value[ii] = value[ii];
    cnst->size = size;
    cnst->index = index;
    if (program->constants) {
        ArbConstant *tail = program->constants;
        while (tail->next)
            tail = tail->next;
        tail->next = cnst;
    } else {
        program->constants = cnst;
    }
    program->numConstants++;
    return index;
} // ArbInternConstant

/*
 * ValidateOperand() - Check one operand's shape.
 */

static ArbIRStatus ValidateOperand(const ArbProgram *program,
                                   const ArbOperand *operand)
{
    int ii;

    switch (operand->file) {
    case ARB_REG_NONE:
        return ARB_IR_BAD_SOURCE;
    case ARB_REG_TEMP:
        if (operand->index < 0 || operand->index >= program->numVirtualTemps)
            return ARB_IR_BAD_SOURCE;
        break;
    case ARB_REG_INPUT:
    case ARB_REG_OUTPUT:
    case ARB_REG_PARAM:
    case ARB_REG_CONST:
        if (operand->index < 0)
            return ARB_IR_BAD_SOURCE;
        break;
    case ARB_REG_ADDRESS:
    default:
        return ARB_IR_BAD_SOURCE;
    }
    for (ii = 0; ii < 4; ii++) {
        if (operand->swizzle[ii] < 0 || operand->swizzle[ii] > 3)
            return ARB_IR_BAD_SOURCE;
    }
    return ARB_IR_VALID;
} // ValidateOperand

/*
 * ArbGetConstant() - Copy an interned constant's four values.
 */

int ArbGetConstant(const ArbProgram *program, int index, float out[4])
{
    return FindConstant(program, index, out);
} // ArbGetConstant

/*
 * ArbTruncateConstants() - Roll the constant pool back to "count" entries,
 *     releasing anything interned speculatively afterward.
 */

void ArbTruncateConstants(ArbProgram *program, int count)
{
    ArbConstant *cnst = program->constants;
    ArbConstant *keepTail = NULL;
    int kept = 0;

    while (cnst) {
        if (cnst->index >= count) {
            ArbConstant *next = cnst->next;
            free(cnst);
            program->numConstants--;
            cnst = next;
        } else {
            if (kept == 0)
                program->constants = cnst;
            else
                keepTail->next = cnst;
            keepTail = cnst;
            kept++;
            cnst = cnst->next;
        }
    }
    if (keepTail)
        keepTail->next = NULL;
    else
        program->constants = NULL;
} // ArbTruncateConstants

/*
 * ArbCheckResourceLimits() - Pure limit comparison in enum order; returns
 *     the first overflowing field with its actual and allowed values.
 */

ArbResourceStatus ArbCheckResourceLimits(const ArbResources *resources,
                                         const ArbLimits *limits,
                                         int *actual, int *allowed)
{
    struct {
        int actual;
        int limit;
        ArbResourceStatus status;
    } fields[] = {
        { resources->instructions,      limits->instructions,
          ARB_RESOURCE_INSTRUCTIONS },
        { resources->aluInstructions,   limits->aluInstructions,
          ARB_RESOURCE_ALU_INSTRUCTIONS },
        { resources->texInstructions,   limits->texInstructions,
          ARB_RESOURCE_TEX_INSTRUCTIONS },
        { resources->texIndirections,   limits->texIndirections,
          ARB_RESOURCE_TEX_INDIRECTIONS },
        { resources->temporaries,       limits->temporaries,
          ARB_RESOURCE_TEMPORARIES },
        { resources->parameters,        limits->parameters,
          ARB_RESOURCE_PARAMETERS },
        { resources->attributes,        limits->attributes,
          ARB_RESOURCE_ATTRIBUTES },
        { resources->addressRegisters,  limits->addressRegisters,
          ARB_RESOURCE_ADDRESS_REGISTERS },
        { resources->textureUnits,      limits->textureUnits,
          ARB_RESOURCE_TEXTURE_UNITS },
    };
    int ii;

    for (ii = 0; ii < (int) (sizeof(fields) / sizeof(fields[0])); ii++) {
        if (fields[ii].limit > 0 && fields[ii].actual > fields[ii].limit) {
            if (actual)
                *actual = fields[ii].actual;
            if (allowed)
                *allowed = fields[ii].limit;
            return fields[ii].status;
        }
    }
    return ARB_RESOURCE_OK;
} // ArbCheckResourceLimits

/*
 * ArbValidateIR() - Structural and stage validation over the whole
 *         program.  This runs after lowering and again after legal-
 *         ization; user-source problems are expected to be diagnosed by
 *         their specific lowering paths before this internal safety net.
 */

ArbIRStatus ArbValidateIR(const ArbProgram *program)
{
    const ArbInstruction *inst = program->first;

    while (inst) {
        int expected;
        int ii;

        if (inst->opcode < 0 || inst->opcode >= ARB_OP_LAST)
            return ARB_IR_BAD_OPCODE;
        if (program->stage == ARB_STAGE_VERTEX &&
            IsVertexOnlyRejected(inst->opcode))
        {
            return ARB_IR_STAGE_OPCODE;
        }
        if (program->stage == ARB_STAGE_FRAGMENT &&
            IsFragmentOnlyRejected(inst->opcode))
        {
            return ARB_IR_STAGE_OPCODE;
        }

        expected = sourceCounts[inst->opcode];
        if (inst->srcCount != expected)
            return ARB_IR_BAD_SOURCE_COUNT;

        if (!inst->mask || (inst->mask & ~ARB_MASK_XYZW))
            return ARB_IR_BAD_MASK;

        if (inst->opcode == ARB_OP_KIL) {
            // No destination.
        } else if (inst->opcode == ARB_OP_ARL) {
            if (inst->dst.file != ARB_REG_ADDRESS)
                return ARB_IR_BAD_DESTINATION;
        } else {
            if (inst->dst.file != ARB_REG_TEMP &&
                inst->dst.file != ARB_REG_OUTPUT)
            {
                return ARB_IR_BAD_DESTINATION;
            }
            if (inst->dst.file == ARB_REG_TEMP &&
                (inst->dst.index < 0 ||
                 inst->dst.index >= program->numVirtualTemps))
            {
                return ARB_IR_BAD_DESTINATION;
            }
        }

        if (inst->opcode == ARB_OP_TEX || inst->opcode == ARB_OP_TXB ||
            inst->opcode == ARB_OP_TXP)
        {
            if (inst->textureUnit < 0 || inst->textureTarget == ARB_TEX_NONE)
                return ARB_IR_BAD_TEXTURE;
        }

        // Relative PARAM addressing: the base grammar's offset range is
        // -64 through 63.  Rebasing in the lowerer keeps every emitted
        // operand inside this window.

        for (ii = 0; ii < inst->srcCount; ii++) {
            if (inst->src[ii].file == ARB_REG_PARAM &&
                inst->src[ii].relative)
            {
                if (inst->src[ii].index < -64 || inst->src[ii].index > 63)
                    return ARB_IR_BAD_SOURCE;
            }
        }

        for (ii = 0; ii < inst->srcCount; ii++) {
            ArbIRStatus status = ValidateOperand(program, &inst->src[ii]);
            if (status != ARB_IR_VALID)
                return status;
        }
        inst = inst->next;
    }
    return ARB_IR_VALID;
} // ArbValidateIR

///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Safe Local Optimizations /////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

static int IsIdentitySwizzleOp(const signed char *swizzle)
{
    return swizzle[0] == 0 && swizzle[1] == 1 &&
           swizzle[2] == 2 && swizzle[3] == 3;
} // IsIdentitySwizzleOp

static int OperandReferencesTemp(const ArbOperand *operand, int temp)
{
    return operand->file == ARB_REG_TEMP && operand->index == temp;
} // OperandReferencesTemp

static int InstructionDefinesTemp(const ArbInstruction *inst, int temp)
{
    return inst->dst.file == ARB_REG_TEMP && inst->dst.index == temp &&
           inst->opcode != ARB_OP_KIL;
} // InstructionDefinesTemp

/*
 * TempUseCounts() - Count how many source operands reference each virtual
 *         temporary.
 */

static void TempUseCounts(const ArbProgram *program, int *counts, int maxTemps)
{
    const ArbInstruction *inst;
    int ii;

    for (ii = 0; ii < maxTemps; ii++)
        counts[ii] = 0;
    for (inst = program->first; inst; inst = inst->next) {
        for (ii = 0; ii < inst->srcCount; ii++) {
            if (inst->src[ii].file == ARB_REG_TEMP &&
                inst->src[ii].index >= 0 && inst->src[ii].index < maxTemps)
            {
                counts[inst->src[ii].index]++;
            }
        }
    }
} // TempUseCounts

/*
 * RemoveIdentityMoves() - Drop MOV t, t with the same effective mask and
 *         swizzle and no modifiers.
 */

static int RemoveIdentityMoves(ArbProgram *program)
{
    ArbInstruction **link = &program->first;
    ArbInstruction *inst;
    int changed = 0;

    while ((inst = *link) != NULL) {
        int identity;
        int lane;

        identity = inst->opcode == ARB_OP_MOV &&
                   inst->dst.file == ARB_REG_TEMP &&
                   inst->srcCount == 1 &&
                   inst->src[0].file == ARB_REG_TEMP &&
                   inst->src[0].index == inst->dst.index &&
                   !inst->src[0].negate && !inst->src[0].absolute &&
                   inst->mask == ARB_MASK_XYZW &&
                   IsIdentitySwizzleOp(inst->src[0].swizzle);

        // Scalar temporaries carry an xxxx read swizzle; a masked self
        // move is still an identity when every written lane reads itself.

        if (!identity &&
            inst->opcode == ARB_OP_MOV &&
            inst->dst.file == ARB_REG_TEMP &&
            inst->srcCount == 1 &&
            inst->src[0].file == ARB_REG_TEMP &&
            inst->src[0].index == inst->dst.index &&
            !inst->src[0].negate && !inst->src[0].absolute)
        {
            identity = 1;
            for (lane = 0; lane < 4; lane++) {
                if (!(inst->mask & (1 << lane)))
                    continue;
                if (inst->src[0].swizzle[lane] != lane) {
                    identity = 0;
                    break;
                }
            }
        }
        if (identity) {
            *link = inst->next;
            if (program->last == inst)
                program->last = NULL;
            free(inst);
            program->numInstructions--;
            changed = 1;
        } else {
            link = &inst->next;
        }
    }
    // Fix the tail pointer after any removals.
    inst = program->first;
    while (inst && inst->next)
        inst = inst->next;
    program->last = inst;
    return changed;
} // RemoveIdentityMoves

/*
 * PropagateSingleFullMoves() - Replace uses of a temporary whose only
 *         definition is a full-mask MOV of an unmodified identity source
 *         with that source.  Relative addressing never propagates.
 */

static int PropagateSingleFullMoves(ArbProgram *program)
{
    int *defCounts;
    ArbInstruction *inst;
    int changed = 0;
    int ii, jj;

    if (program->numVirtualTemps <= 0)
        return 0;
    defCounts = (int *) calloc((size_t) program->numVirtualTemps,
                               sizeof(int));
    if (!defCounts)
        return 0;
    for (inst = program->first; inst; inst = inst->next) {
        if (InstructionDefinesTemp(inst, inst->dst.index) &&
            inst->dst.index < program->numVirtualTemps)
        {
            defCounts[inst->dst.index]++;
        }
    }
    for (inst = program->first; inst; inst = inst->next) {
        ArbOperand replacement;
        int canReplace;

        if (inst->opcode != ARB_OP_MOV ||
            inst->dst.file != ARB_REG_TEMP ||
            inst->mask != ARB_MASK_XYZW ||
            inst->srcCount != 1)
        {
            continue;
        }
        if (defCounts[inst->dst.index] != 1)
            continue;
        replacement = inst->src[0];
        canReplace = replacement.file != ARB_REG_ADDRESS &&
                     !replacement.negate && !replacement.absolute &&
                     replacement.relative == 0 &&
                     IsIdentitySwizzleOp(replacement.swizzle);
        if (!canReplace)
            continue;
        for (jj = 0; jj < 4; jj++)
            replacement.swizzle[jj] = (signed char) jj;
        {
            ArbInstruction *user;
            for (user = inst->next; user; user = user->next) {
                for (ii = 0; ii < user->srcCount; ii++) {
                    if (OperandReferencesTemp(&user->src[ii],
                                              inst->dst.index))
                    {
                        user->src[ii] = replacement;
                        changed = 1;
                    }
                }
            }
        }
    }
    free(defCounts);
    return changed;
} // PropagateSingleFullMoves

/*
 * RemoveDeadDefinitions() - Delete instructions defining a temporary that
 *     no source anywhere references.  Output writes are always preserved.
 */

static int RemoveDeadDefinitions(ArbProgram *program)
{
    int *counts;
    ArbInstruction **link;
    ArbInstruction *inst;
    int changed = 0;
    int removed;

    if (program->numVirtualTemps <= 0)
        return 0;
    counts = (int *) malloc(sizeof(int) * program->numVirtualTemps);
    if (!counts)
        return 0;
    do {
        removed = 0;
        TempUseCounts(program, counts, program->numVirtualTemps);
        link = &program->first;
        while ((inst = *link) != NULL) {
            int dead = InstructionDefinesTemp(inst, inst->dst.index) &&
                       counts[inst->dst.index] == 0;
            if (dead) {
                *link = inst->next;
                free(inst);
                program->numInstructions--;
                removed = 1;
                changed = 1;
            } else {
                link = &inst->next;
            }
        }
        inst = program->first;
        while (inst && inst->next)
            inst = inst->next;
        program->last = inst;
    } while (removed);
    free(counts);
    return changed;
} // RemoveDeadDefinitions

/*
 * ConstLaneValues() - Fetch an interned constant's four values.
 */

static int FindConstant(const ArbProgram *program, int index, float *out)
{
    const ArbConstant *cnst = program->constants;
    while (cnst) {
        if (cnst->index == index) {
            out[0] = cnst->value[0];
            out[1] = cnst->value[1];
            out[2] = cnst->value[2];
            out[3] = cnst->value[3];
            return 1;
        }
        cnst = cnst->next;
    }
    return 0;
} // FindConstant

static int IsConstAllValue(const ArbProgram *program, const ArbOperand *op,
                           float want)
{
    float vals[4];
    int ii;

    if (op->file != ARB_REG_CONST || op->negate || op->absolute ||
        !IsIdentitySwizzleOp(op->swizzle))
    {
        return 0;
    }
    if (!FindConstant(program, op->index, vals))
        return 0;
    for (ii = 0; ii < 4; ii++) {
        if (vals[ii] != want)
            return 0;
    }
    return 1;
} // IsConstAllValue

/*
 * FoldNeutralConstants() - Fold MUL(x, 1) and SUB(x, +0) into moves.
 *         ADD-with-zero and MUL-with-zero change signed-zero/NaN/Inf
 *         behavior and are deliberately not folded.
 */

static int FoldNeutralConstants(ArbProgram *program)
{
    ArbInstruction **link = &program->first;
    ArbInstruction *inst;
    int changed = 0;

    while ((inst = *link) != NULL) {
        int fold = 0;
        int keepIdx = -1;

        if ((inst->opcode == ARB_OP_MUL || inst->opcode == ARB_OP_SUB) &&
            inst->dst.file == ARB_REG_TEMP && inst->srcCount == 2)
        {
            if (inst->opcode == ARB_OP_MUL &&
                IsConstAllValue(program, &inst->src[1], 1.0f))
            {
                fold = 1;
                keepIdx = 0;
            } else if (inst->opcode == ARB_OP_SUB &&
                       IsConstAllValue(program, &inst->src[1], 0.0f))
            {
                fold = 1;
                keepIdx = 0;
            } else if (inst->opcode == ARB_OP_MUL &&
                       IsConstAllValue(program, &inst->src[0], 1.0f))
            {
                fold = 1;
                keepIdx = 1;
            }
        }
        if (fold && inst->src[keepIdx].file != ARB_REG_ADDRESS) {
            ArbInstruction *mov;
            mov = (ArbInstruction *) malloc(sizeof(ArbInstruction));
            if (mov) {
                *mov = *inst;
                mov->opcode = ARB_OP_MOV;
                mov->srcCount = 1;
                mov->src[0] = inst->src[keepIdx];
                mov->next = inst->next;
                *link = mov;
                if (program->last == inst)
                    program->last = mov;
                free(inst);
                changed = 1;
                inst = mov;
            }
        }
        link = &inst->next;
    }
    return changed;
} // FoldNeutralConstants

/*
 * ArbOptimizeProgram() - Run the safe passes until one makes no change.
 */

void ArbOptimizeProgram(ArbProgram *program)
{
    int changed;

    do {
        changed = 0;
        changed |= RemoveIdentityMoves(program);
        changed |= PropagateSingleFullMoves(program);
        changed |= RemoveDeadDefinitions(program);
        changed |= FoldNeutralConstants(program);
    } while (changed);
} // ArbOptimizeProgram

///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Linear Scan Allocation ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

typedef struct ArbInterval_Rec {
    int virtualTemp;
    int first;
    int last;
    int physical;
} ArbInterval;

typedef struct ArbActive_Rec {
    int interval;
    struct ArbActive_Rec *next;
} ArbActive;

static void ReleaseActive(ArbActive *list)
{
    while (list) {
        ArbActive *next = list->next;
        free(list);
        list = next;
    }
} // ReleaseActive

/*
 * ArbAllocateTemporaries() - Deterministic linear-scan allocation.  Every
 *     virtual temporary collapses its per-component uses into one
 *     full-register interval [first definition, last source use].  The
 *     TEMP operand indices are rewritten to physical registers on success.
 */

ArbAllocStatus ArbAllocateTemporaries(ArbProgram *program, int maxTemporaries)
{
    ArbInterval *intervals;
    ArbActive *active = NULL;
    ArbInstruction *inst;
    int position;
    int numIntervals = 0;
    int highestPhysical = -1;
    int ii, jj;

    if (ArbValidateIR(program) != ARB_IR_VALID)
        return ARB_ALLOC_INVALID_IR;
    if (program->numVirtualTemps <= 0) {
        program->numPhysicalTemps = 0;
        return ARB_ALLOC_OK;
    }

    intervals = (ArbInterval *) malloc(sizeof(ArbInterval) *
                                       program->numVirtualTemps);
    if (!intervals)
        return ARB_ALLOC_INVALID_IR;
    for (ii = 0; ii < program->numVirtualTemps; ii++) {
        intervals[ii].virtualTemp = ii;
        intervals[ii].first = -1;
        intervals[ii].last = -1;
        intervals[ii].physical = -1;
    }

    // Number instructions from zero, recording first definition and last
    // source use for every temporary.

    position = 0;
    for (inst = program->first; inst; inst = inst->next, position++) {
        if (InstructionDefinesTemp(inst, inst->dst.index) &&
            inst->dst.index < program->numVirtualTemps)
        {
            if (intervals[inst->dst.index].first < 0 ||
                position < intervals[inst->dst.index].first)
            {
                intervals[inst->dst.index].first = position;
            }
            if (position > intervals[inst->dst.index].last)
                intervals[inst->dst.index].last = position;
        }
        for (ii = 0; ii < inst->srcCount; ii++) {
            int temp;
            if (inst->src[ii].file != ARB_REG_TEMP)
                continue;
            temp = inst->src[ii].index;
            if (temp < 0 || temp >= program->numVirtualTemps)
                continue;
            if (intervals[temp].first < 0) {
                // Read before any recorded definition: start here.
                intervals[temp].first = position;
            }
            if (position > intervals[temp].last)
                intervals[temp].last = position;
        }
    }

    // Collect live intervals in order of first definition.

    for (ii = 0; ii < program->numVirtualTemps; ii++) {
        if (intervals[ii].first >= 0)
            intervals[numIntervals++] = intervals[ii];
    }
    for (ii = 1; ii < numIntervals; ii++) {
        ArbInterval key = intervals[ii];
        jj = ii - 1;
        while (jj >= 0 && intervals[jj].first > key.first) {
            intervals[jj + 1] = intervals[jj];
            jj--;
        }
        intervals[jj + 1] = key;
    }

    // Linear scan with an active list kept sorted by last use.

    for (ii = 0; ii < numIntervals; ii++) {
        ArbActive **link = &active;
        ArbActive *node;
        int regFound = 0;
        int candidate;

        // Expire intervals whose last use is before this first use.
        while (*link != NULL) {
            if (intervals[(*link)->interval].last <
                intervals[ii].first)
            {
                node = *link;
                *link = node->next;
                free(node);
            } else {
                link = &(*link)->next;
            }
        }

        // Assign the lowest free physical register.
        for (candidate = 0; candidate < maxTemporaries; candidate++) {
            ArbActive *scan = active;
            int taken = 0;
            while (scan) {
                if (intervals[scan->interval].physical == candidate) {
                    taken = 1;
                    break;
                }
                scan = scan->next;
            }
            if (!taken) {
                intervals[ii].physical = candidate;
                regFound = 1;
                break;
            }
        }
        if (!regFound) {
            free(intervals);
            ReleaseActive(active);
            return ARB_ALLOC_TEMP_LIMIT;
        }
        if (intervals[ii].physical > highestPhysical)
            highestPhysical = intervals[ii].physical;

        // Insert into the active list sorted by interval last use.
        node = (ArbActive *) malloc(sizeof(ArbActive));
        if (!node) {
            free(intervals);
            ReleaseActive(active);
            return ARB_ALLOC_INVALID_IR;
        }
        node->interval = ii;
        link = &active;
        while (*link != NULL &&
               intervals[(*link)->interval].last <= intervals[ii].last)
        {
            link = &(*link)->next;
        }
        node->next = *link;
        *link = node;
    }
    ReleaseActive(active);

    // Rewrite every TEMP operand index to its physical register via a
    // virtual-to-physical map built before any mutation.

    {
        int *map = (int *) malloc(sizeof(int) * program->numVirtualTemps);
        if (!map) {
            free(intervals);
            ReleaseActive(active);
            return ARB_ALLOC_INVALID_IR;
        }
        for (jj = 0; jj < program->numVirtualTemps; jj++)
            map[jj] = -1;
        for (ii = 0; ii < numIntervals; ii++)
            map[intervals[ii].virtualTemp] = intervals[ii].physical;
        for (inst = program->first; inst; inst = inst->next) {
            if (InstructionDefinesTemp(inst, -1) || inst->dst.file == ARB_REG_TEMP)
            {
                int vt = inst->dst.index;
                if (vt >= 0 && vt < program->numVirtualTemps &&
                    map[vt] >= 0)
                {
                    inst->physicalTemp = map[vt];
                    inst->dst.index = map[vt];
                }
            }
            for (ii = 0; ii < inst->srcCount; ii++) {
                int vt = inst->src[ii].index;
                if (inst->src[ii].file == ARB_REG_TEMP &&
                    vt >= 0 && vt < program->numVirtualTemps &&
                    map[vt] >= 0)
                {
                    inst->src[ii].index = map[vt];
                }
            }
        }
        free(map);
    }

    program->numPhysicalTemps = highestPhysical + 1;
    free(intervals);
    return ARB_ALLOC_OK;
} // ArbAllocateTemporaries
