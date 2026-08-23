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
// arb_ir.h - Private four-component vector IR for the ARB profiles.
//

#if !defined(__ARB_IR_H)
#define __ARB_IR_H 1

#include "arb_hal.h"

#define ARB_MASK_X 0x1
#define ARB_MASK_Y 0x2
#define ARB_MASK_Z 0x4
#define ARB_MASK_W 0x8
#define ARB_MASK_XYZW 0xf

typedef enum ArbOpcode_Enum {
    ARB_OP_ABS, ARB_OP_ADD, ARB_OP_ARL, ARB_OP_CMP, ARB_OP_COS,
    ARB_OP_DP3, ARB_OP_DP4, ARB_OP_DPH, ARB_OP_DST, ARB_OP_EX2,
    ARB_OP_EXP, ARB_OP_FLR, ARB_OP_FRC, ARB_OP_KIL, ARB_OP_LG2,
    ARB_OP_LIT, ARB_OP_LOG, ARB_OP_LRP, ARB_OP_MAD, ARB_OP_MAX,
    ARB_OP_MIN, ARB_OP_MOV, ARB_OP_MUL, ARB_OP_POW, ARB_OP_RCP,
    ARB_OP_RSQ, ARB_OP_SCS, ARB_OP_SGE, ARB_OP_SIN, ARB_OP_SLT,
    ARB_OP_SUB, ARB_OP_SWZ, ARB_OP_TEX, ARB_OP_TXB, ARB_OP_TXP,
    ARB_OP_XPD, ARB_OP_LAST
} ArbOpcode;

typedef enum ArbRegisterFile_Enum {
    ARB_REG_NONE, ARB_REG_TEMP, ARB_REG_INPUT, ARB_REG_OUTPUT,
    ARB_REG_PARAM, ARB_REG_CONST, ARB_REG_ADDRESS
} ArbRegisterFile;

typedef enum ArbTextureTarget_Enum {
    ARB_TEX_NONE, ARB_TEX_1D, ARB_TEX_2D, ARB_TEX_3D,
    ARB_TEX_CUBE, ARB_TEX_RECT
} ArbTextureTarget;

typedef enum ArbIRStatus_Enum {
    ARB_IR_VALID, ARB_IR_BAD_OPCODE, ARB_IR_BAD_DESTINATION,
    ARB_IR_BAD_SOURCE_COUNT, ARB_IR_BAD_SOURCE,
    ARB_IR_BAD_MASK, ARB_IR_BAD_TEXTURE, ARB_IR_STAGE_OPCODE
} ArbIRStatus;

typedef struct ArbOperand_Rec {
    ArbRegisterFile file;
    int index;
    int bindingName;
    signed char swizzle[4];
    unsigned char negate;
    unsigned char absolute;
    unsigned char relative;
    signed char relativeOffset;
} ArbOperand;

typedef struct ArbInstruction_Rec {
    struct ArbInstruction_Rec *next;
    SourceLoc loc;
    ArbOpcode opcode;
    ArbOperand dst;
    ArbOperand src[3];
    unsigned char srcCount;
    unsigned char mask;
    unsigned char saturate;
    signed char textureUnit;
    ArbTextureTarget textureTarget;
    int physicalTemp;
} ArbInstruction;

typedef struct ArbConstant_Rec {
    struct ArbConstant_Rec *next;
    float value[4];
    int size;
    int index;
} ArbConstant;

typedef struct ArbProgram_Rec {
    ArbStage stage;
    ArbInstruction *first;
    ArbInstruction *last;
    ArbConstant *constants;
    int numInstructions;
    int numVirtualTemps;
    int numConstants;
    int numPhysicalTemps;
} ArbProgram;

void ArbInitProgram(ArbProgram *program, ArbStage stage);
void ArbFreeProgram(ArbProgram *program);
int ArbNewTemp(ArbProgram *program);
ArbOperand ArbTempOperand(int index);
ArbOperand ArbInputOperand(int index);
ArbOperand ArbOutputOperand(int index);
ArbOperand ArbParamOperand(int index);
ArbOperand ArbConstOperand(int index);
ArbInstruction *ArbAppendInstruction(ArbProgram *program, ArbOpcode opcode,
                                     const SourceLoc *loc, ArbOperand dst);
int ArbAddSource(ArbInstruction *instruction, ArbOperand source);
int ArbInternConstant(ArbProgram *program, const float *value, int size);
ArbIRStatus ArbValidateIR(const ArbProgram *program);
int ArbIsTextureOpcode(ArbOpcode opcode);

#endif /* !defined(__ARB_IR_H) */
