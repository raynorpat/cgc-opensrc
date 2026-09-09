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

#define NUMELS(x) ((int) (sizeof(x) / sizeof((x)[0])))
#define FLT TYPE_BASE_FLOAT

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
    4,    // colorOutputs
    1,    // depthOutputs
    0,    // clipDistanceComponents
    0,    // constantBufferSlots
    0,    // constantBufferVectors
    0,    // resources
    0,    // geometryMaxVertices
    0     // geometryTotalOutputComponents
};

static ConnectorRegisters inputCRegs_hlslf[] = {
    { "COLOR0",    0, FLT,  0, 4, REG_RESERVED | REG_INPUT },
    { "COLOR1",    0, FLT,  1, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD0", 0, FLT,  2, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD1", 0, FLT,  3, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD2", 0, FLT,  4, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD3", 0, FLT,  5, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD4", 0, FLT,  6, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD5", 0, FLT,  7, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD6", 0, FLT,  8, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD7", 0, FLT,  9, 4, REG_RESERVED | REG_INPUT },
    { "FOG0",      0, FLT, 10, 1, REG_RESERVED | REG_INPUT },
    { "VPOS",      0, FLT, 11, 2, REG_RESERVED | REG_INPUT },
    { "VFACE",     0, FLT, 12, 1, REG_RESERVED | REG_INPUT }
};

static ConnectorRegisters outputCRegs_hlslf[] = {
    { "COLOR0", 0, FLT, 0, 4, REG_RESERVED | REG_OUTPUT },
    { "COLOR1", 0, FLT, 1, 4, REG_RESERVED | REG_OUTPUT },
    { "COLOR2", 0, FLT, 2, 4, REG_RESERVED | REG_OUTPUT },
    { "COLOR3", 0, FLT, 3, 4, REG_RESERVED | REG_OUTPUT },
    { "DEPTH0", 0, FLT, 4, 1, REG_RESERVED | REG_OUTPUT },
    /* Provisional one-over slot: Task 10 target validation rejects COLOR4
     * against the active four-color limit with C6408. */
    { "COLOR4", 0, FLT, 5, 4, REG_RESERVED | REG_OUTPUT }
};

static const HlslSemanticDesc inputSemantics_hlslf[] = {
    { "COLOR",    0, 2, SEM_IN | SEM_VARYING, 4,
      HLSL_INTERFACE_COLOR },
    { "TEXCOORD", 0, 8, SEM_IN | SEM_VARYING, 4,
      HLSL_INTERFACE_VARYING },
    { "FOG",      0, 1, SEM_IN | SEM_VARYING, 1,
      HLSL_INTERFACE_VARYING },
    { "VPOS",     0, 1, SEM_IN | SEM_VARYING, 2,
      HLSL_INTERFACE_PIXEL_POSITION },
    { "VFACE",    0, 1, SEM_IN | SEM_VARYING, 1,
      HLSL_INTERFACE_FACE }
};

static const HlslSemanticDesc outputSemantics_hlslf[] = {
    { "COLOR", 0, 5, SEM_OUT | SEM_VARYING, 4,
      HLSL_INTERFACE_COLOR },
    { "DEPTH", 0, 1, SEM_OUT | SEM_VARYING, 1,
      HLSL_INTERFACE_DEPTH }
};

static const HlslSemanticAlias inputAliases_hlslf[] = {
    { "COL0",  "COLOR0" },
    { "COL1",  "COLOR1" },
    { "TEX0",  "TEXCOORD0" },
    { "TEX1",  "TEXCOORD1" },
    { "TEX2",  "TEXCOORD2" },
    { "TEX3",  "TEXCOORD3" },
    { "TEX4",  "TEXCOORD4" },
    { "TEX5",  "TEXCOORD5" },
    { "TEX6",  "TEXCOORD6" },
    { "TEX7",  "TEXCOORD7" },
    { "WPOS",  "VPOS" },
    { "FACE",  "VFACE" }
};

static const HlslSemanticAlias outputAliases_hlslf[] = {
    { "COL0", "COLOR0" },
    { "COL1", "COLOR1" },
    { "COL2", "COLOR2" },
    { "COL3", "COLOR3" }
};

// Connector descriptors with input/output CIDs:
static ConnectorDescriptor connectors_hlslf[] = {
    { "hlslf_in",  0, CID_HLSLF_IN_ID,  CONNECTOR_IS_INPUT,
      NUMELS(inputCRegs_hlslf),  inputCRegs_hlslf },
    { "hlslf_out", 0, CID_HLSLF_OUT_ID, CONNECTOR_IS_OUTPUT,
      NUMELS(outputCRegs_hlslf), outputCRegs_hlslf },
};

const HlslProfileDesc HlslProfile_hlslf = {
    HLSL_STAGE_PIXEL,
    HLSL_SHADER_MODEL_3,
    HLSL_SYNTAX_LEGACY,
    HLSL_SEMANTIC_POLICY_DX9,
    HLSL_RESOURCE_POLICY_DX9,
    PROFILE_HLSLF_NAME,
    "ps_3_0",
    VERSION_STRING_HLSL_SM3,
    PROFILE_HLSLF_ID,
    CID_HLSLF_IN_ID,
    CID_HLSLF_OUT_ID,
    connectors_hlslf,
    NUMELS(connectors_hlslf),
    inputSemantics_hlslf,
    NUMELS(inputSemantics_hlslf),
    inputAliases_hlslf,
    NUMELS(inputAliases_hlslf),
    outputSemantics_hlslf,
    NUMELS(outputSemantics_hlslf),
    outputAliases_hlslf,
    NUMELS(outputAliases_hlslf),
    inputCRegs_hlslf,
    NUMELS(inputCRegs_hlslf),
    outputCRegs_hlslf,
    NUMELS(outputCRegs_hlslf),
    &limits_hlslf,
    HLSL_CAP_DISCARD | HLSL_CAP_DERIVATIVES
};

///////////////////////////////////////////////////////////////////////////////
/////////////////////////// End of hlslf_hal.c ////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
