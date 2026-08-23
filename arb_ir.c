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

    if (size < 4) {
        expanded[0] = value[0];
        for (ii = 1; ii < 4; ii++)
            expanded[ii] = value[0];
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

        for (ii = 0; ii < inst->srcCount; ii++) {
            ArbIRStatus status = ValidateOperand(program, &inst->src[ii]);
            if (status != ARB_IR_VALID)
                return status;
        }
        inst = inst->next;
    }
    return ARB_IR_VALID;
} // ArbValidateIR
