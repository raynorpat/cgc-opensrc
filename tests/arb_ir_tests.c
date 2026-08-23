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
// arb_ir_tests.c - Unit tests for the private ARB vector IR.
//

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NO_PARSER 1
#include "slglobals.h"
#include "arb_ir.h"

int main(void)
{
    ArbProgram program;
    ArbInstruction *inst;
    ArbOperand dst, src;
    SourceLoc loc = { 0, 7 };
    int t0;
    int c0, c1;
    float value[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

    ArbInitProgram(&program, ARB_STAGE_VERTEX);
    t0 = ArbNewTemp(&program);
    assert(t0 == 0);

    dst = ArbTempOperand(t0);
    src = ArbInputOperand(0);
    inst = ArbAppendInstruction(&program, ARB_OP_MOV, &loc, dst);
    assert(inst != NULL);
    assert(ArbAddSource(inst, src));
    assert(ArbValidateIR(&program) == ARB_IR_VALID);

    c0 = ArbInternConstant(&program, value, 4);
    c1 = ArbInternConstant(&program, value, 4);
    assert(c0 == c1);
    assert(program.numConstants == 1);

    inst->srcCount = 4;
    assert(ArbValidateIR(&program) == ARB_IR_BAD_SOURCE_COUNT);
    ArbFreeProgram(&program);

    // Linear-scan boundaries: twelve overlapping intervals fit the ARBVP1
    // temporary budget exactly; a thirteenth overlapping interval fails.

    {
        ArbProgram p2;
        int k;

        ArbInitProgram(&p2, ARB_STAGE_VERTEX);
        for (k = 0; k < 12; k++)
            assert(ArbNewTemp(&p2) == k);
        for (k = 0; k < 12; k++) {
            ArbOperand d = ArbTempOperand(k);
            ArbInstruction *def = ArbAppendInstruction(&p2, ARB_OP_MOV,
                                                       NULL, d);
            assert(def != NULL);
            def->mask = ARB_MASK_XYZW;
            assert(ArbAddSource(def, ArbConstOperand(0)));
        }
        for (k = 0; k < 12; k++) {
            ArbOperand d = ArbOutputOperand(0);
            ArbInstruction *use = ArbAppendInstruction(&p2, ARB_OP_MOV,
                                                       NULL, d);
            assert(use != NULL);
            use->mask = ARB_MASK_XYZW;
            assert(ArbAddSource(use, ArbTempOperand(k)));
        }
        assert(ArbAllocateTemporaries(&p2, 12) == ARB_ALLOC_OK);
        assert(p2.numPhysicalTemps == 12);
        ArbFreeProgram(&p2);

        // Thirteenth overlapping interval: define one more temp before
        // the use block so its interval spans the entire peak window.
        ArbInitProgram(&p2, ARB_STAGE_VERTEX);
        for (k = 0; k < 13; k++)
            assert(ArbNewTemp(&p2) == k);
        {
            ArbOperand d = ArbTempOperand(12);
            ArbInstruction *def = ArbAppendInstruction(&p2, ARB_OP_MOV,
                                                       NULL, d);
            def->mask = ARB_MASK_XYZW;
            assert(ArbAddSource(def, ArbConstOperand(0)));
        }
        for (k = 0; k < 12; k++) {
            ArbOperand d = ArbTempOperand(k);
            ArbInstruction *def = ArbAppendInstruction(&p2, ARB_OP_MOV,
                                                       NULL, d);
            def->mask = ARB_MASK_XYZW;
            assert(ArbAddSource(def, ArbConstOperand(0)));
        }
        for (k = 0; k < 13; k++) {
            ArbOperand d = ArbOutputOperand(0);
            int srcIdx = k < 12 ? k : 12;
            ArbInstruction *use = ArbAppendInstruction(&p2, ARB_OP_MOV,
                                                       NULL, d);
            use->mask = ARB_MASK_XYZW;
            assert(ArbAddSource(use, ArbTempOperand(srcIdx)));
        }
        assert(ArbAllocateTemporaries(&p2, 12) == ARB_ALLOC_TEMP_LIMIT);
        assert(ArbAllocateTemporaries(&p2, 13) == ARB_ALLOC_OK);
        ArbFreeProgram(&p2);
    }
    // Pure resource-limit table: every nonzero counter in both profiles,
    // exact boundary passes and one-over fails, in enum order.

    {
        ArbLimits vp = { ARBVP_MAX_INSTRUCTIONS, 0, 0, 0,
                         ARBVP_MAX_TEMPORARIES, ARBVP_MAX_PARAMETERS,
                         ARBVP_MAX_ATTRIBUTES, ARBVP_MAX_ADDRESS_REGISTERS,
                         0 };
        ArbLimits fp = { ARBFP_MAX_INSTRUCTIONS,
                         ARBFP_MAX_ALU_INSTRUCTIONS,
                         ARBFP_MAX_TEX_INSTRUCTIONS,
                         ARBFP_MAX_TEX_INDIRECTIONS,
                         ARBFP_MAX_TEMPORARIES, ARBFP_MAX_PARAMETERS,
                         ARBFP_MAX_ATTRIBUTES, 0,
                         ARBFP_MAX_TEXTURE_UNITS };
        int actual = 0, allowed = 0;

        /* Vertex counters. */
        {
            ArbResources r;
            memset(&r, 0, sizeof(r));
            r.instructions = ARBVP_MAX_INSTRUCTIONS;
            assert(ArbCheckResourceLimits(&r, &vp, NULL, NULL) ==
                   ARB_RESOURCE_OK);
            r.instructions++;
            assert(ArbCheckResourceLimits(&r, &vp, &actual, &allowed) ==
                   ARB_RESOURCE_INSTRUCTIONS);
            assert(actual == ARBVP_MAX_INSTRUCTIONS + 1);
            assert(allowed == ARBVP_MAX_INSTRUCTIONS);

            memset(&r, 0, sizeof(r));
            r.temporaries = ARBVP_MAX_TEMPORARIES + 1;
            assert(ArbCheckResourceLimits(&r, &vp, &actual, &allowed) ==
                   ARB_RESOURCE_TEMPORARIES);
            r.temporaries = ARBVP_MAX_TEMPORARIES;
            r.parameters = ARBVP_MAX_PARAMETERS + 1;
            assert(ArbCheckResourceLimits(&r, &vp, &actual, &allowed) ==
                   ARB_RESOURCE_PARAMETERS);
            r.parameters = ARBVP_MAX_PARAMETERS;
            r.attributes = ARBVP_MAX_ATTRIBUTES + 1;
            assert(ArbCheckResourceLimits(&r, &vp, &actual, &allowed) ==
                   ARB_RESOURCE_ATTRIBUTES);
            r.attributes = ARBVP_MAX_ATTRIBUTES;
            r.addressRegisters = ARBVP_MAX_ADDRESS_REGISTERS + 1;
            assert(ArbCheckResourceLimits(&r, &vp, &actual, &allowed) ==
                   ARB_RESOURCE_ADDRESS_REGISTERS);
        }

        /* Fragment counters. */
        {
            ArbResources r;
            memset(&r, 0, sizeof(r));
            r.instructions = ARBFP_MAX_INSTRUCTIONS;
            assert(ArbCheckResourceLimits(&r, &fp, &actual, &allowed) ==
                   ARB_RESOURCE_OK);
            r.instructions++;
            assert(ArbCheckResourceLimits(&r, &fp, &actual, &allowed) ==
                   ARB_RESOURCE_INSTRUCTIONS);
            memset(&r, 0, sizeof(r));
            r.aluInstructions = ARBFP_MAX_ALU_INSTRUCTIONS;
            assert(ArbCheckResourceLimits(&r, &fp, &actual, &allowed) ==
                   ARB_RESOURCE_OK);
            r.aluInstructions++;
            assert(ArbCheckResourceLimits(&r, &fp, &actual, &allowed) ==
                   ARB_RESOURCE_ALU_INSTRUCTIONS);
            memset(&r, 0, sizeof(r));
            r.texInstructions = ARBFP_MAX_TEX_INSTRUCTIONS + 1;
            assert(ArbCheckResourceLimits(&r, &fp, &actual, &allowed) ==
                   ARB_RESOURCE_TEX_INSTRUCTIONS);
            memset(&r, 0, sizeof(r));
            r.texIndirections = ARBFP_MAX_TEX_INDIRECTIONS + 1;
            assert(ArbCheckResourceLimits(&r, &fp, &actual, &allowed) ==
                   ARB_RESOURCE_TEX_INDIRECTIONS);
            memset(&r, 0, sizeof(r));
            r.textureUnits = ARBFP_MAX_TEXTURE_UNITS + 1;
            assert(ArbCheckResourceLimits(&r, &fp, &actual, &allowed) ==
                   ARB_RESOURCE_TEXTURE_UNITS);
            memset(&r, 0, sizeof(r));
            r.parameters = ARBFP_MAX_PARAMETERS + 1;
            assert(ArbCheckResourceLimits(&r, &fp, &actual, &allowed) ==
                   ARB_RESOURCE_PARAMETERS);
            memset(&r, 0, sizeof(r));
            r.attributes = ARBFP_MAX_ATTRIBUTES + 1;
            assert(ArbCheckResourceLimits(&r, &fp, &actual, &allowed) ==
                   ARB_RESOURCE_ATTRIBUTES);
        }
    }
    return 0;
}
