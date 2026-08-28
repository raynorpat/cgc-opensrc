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
// hlslf_hal.c
//
// Pixel stage descriptor, semantics, and SM3 limits for the hlslf profile.

#include <stdio.h>
#include <stdlib.h>

#include "slglobals.h"
#include "hlsl_hal.h"

// Pixel Shader Model 3 limits:
// 10 input registers, 5 output registers (4 colors + depth), 224 float constants,
// 16 integer constants, 16 bool constants, 16 samplers, 4 color outputs.
static const HlslLimits limits_hlslf = {
    10,   // inputs
    5,    // outputs
    224,  // floatConstants
    16,   // intConstants
    16,   // boolConstants
    16,   // samplers
    4     // colorOutputs
};

// Skeleton: zero registers; Task 3 fills the complete connectors.
static ConnectorRegisters inputCRegs_hlslf[] = { { NULL, 0, 0, 0, 0, 0 } };
static ConnectorRegisters outputCRegs_hlslf[] = { { NULL, 0, 0, 0, 0, 0 } };
#define INPUT_REGS_hlslf_NUM 0
#define OUTPUT_REGS_hlslf_NUM 0

// Connector descriptors with input/output CIDs:
static ConnectorDescriptor connectors_hlslf[] = {
    { "hlslf_in",  0, CID_HLSLF_IN_ID,  CONNECTOR_IS_INPUT,
      INPUT_REGS_hlslf_NUM,  inputCRegs_hlslf },
    { "hlslf_out", 0, CID_HLSLF_OUT_ID, CONNECTOR_IS_OUTPUT,
      OUTPUT_REGS_hlslf_NUM, outputCRegs_hlslf },
};

const HlslProfileDesc HlslProfile_hlslf = {
    HLSL_STAGE_PIXEL,
    PROFILE_HLSLF_NAME,
    "ps_3_0",
    PROFILE_HLSLF_ID,
    CID_HLSLF_IN_ID,
    CID_HLSLF_OUT_ID,
    connectors_hlslf,
    (int)(sizeof(connectors_hlslf) / sizeof(connectors_hlslf[0])),
    inputCRegs_hlslf,
    INPUT_REGS_hlslf_NUM,
    outputCRegs_hlslf,
    OUTPUT_REGS_hlslf_NUM,
    &limits_hlslf
};

///////////////////////////////////////////////////////////////////////////////
/////////////////////////// End of hlslf_hal.c ////////////////////////////////
///////////////////////////////////////////////////////////////////////////////