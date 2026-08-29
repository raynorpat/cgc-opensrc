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
// hlslv_hal.c
//
// Vertex stage descriptor, semantics, and SM3 limits for the hlslv profile.

#include <stdio.h>
#include <stdlib.h>

#include "slglobals.h"
#include "hlsl_hal.h"

#define NUMELS(x) ((int) (sizeof(x) / sizeof((x)[0])))
#define FLT TYPE_BASE_FLOAT

// Vertex Shader Model 3 limits:
// 16 input registers, 12 output registers, 256 float constants,
// 16 integer constants, 16 bool constants, 4 samplers, 0 color outputs.
static const HlslLimits limits_hlslv = {
    16,   // inputs
    12,   // outputs
    256,  // floatConstants
    16,   // intConstants
    16,   // boolConstants
    4,    // samplers
    0     // colorOutputs
};

static ConnectorRegisters inputCRegs_hlslv[] = {
    { "POSITION0",     0, FLT,  0, 4, REG_RESERVED | REG_INPUT },
    { "BLENDWEIGHT0",  0, FLT,  1, 4, REG_RESERVED | REG_INPUT },
    { "BLENDINDICES0", 0, FLT,  2, 4, REG_RESERVED | REG_INPUT },
    { "NORMAL0",       0, FLT,  3, 3, REG_RESERVED | REG_INPUT },
    { "PSIZE0",        0, FLT,  4, 1, REG_RESERVED | REG_INPUT },
    { "TEXCOORD0",     0, FLT,  5, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD1",     0, FLT,  6, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD2",     0, FLT,  7, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD3",     0, FLT,  8, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD4",     0, FLT,  9, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD5",     0, FLT, 10, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD6",     0, FLT, 11, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD7",     0, FLT, 12, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD8",     0, FLT, 13, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD9",     0, FLT, 14, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD10",    0, FLT, 15, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD11",    0, FLT, 16, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD12",    0, FLT, 17, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD13",    0, FLT, 18, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD14",    0, FLT, 19, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD15",    0, FLT, 20, 4, REG_RESERVED | REG_INPUT },
    { "TANGENT0",      0, FLT, 21, 3, REG_RESERVED | REG_INPUT },
    { "BINORMAL0",     0, FLT, 22, 3, REG_RESERVED | REG_INPUT },
    { "COLOR0",        0, FLT, 23, 4, REG_RESERVED | REG_INPUT },
    { "COLOR1",        0, FLT, 24, 4, REG_RESERVED | REG_INPUT }
};

static ConnectorRegisters outputCRegs_hlslv[] = {
    { "POSITION0", 0, FLT,  0, 4, REG_RESERVED | REG_OUTPUT |
                                      REG_WRITE_REQUIRED },
    { "PSIZE0",    0, FLT,  1, 1, REG_RESERVED | REG_OUTPUT },
    { "FOG0",      0, FLT,  2, 1, REG_RESERVED | REG_OUTPUT },
    { "COLOR0",    0, FLT,  3, 4, REG_RESERVED | REG_OUTPUT },
    { "COLOR1",    0, FLT,  4, 4, REG_RESERVED | REG_OUTPUT },
    { "TEXCOORD0", 0, FLT,  5, 4, REG_RESERVED | REG_OUTPUT },
    { "TEXCOORD1", 0, FLT,  6, 4, REG_RESERVED | REG_OUTPUT },
    { "TEXCOORD2", 0, FLT,  7, 4, REG_RESERVED | REG_OUTPUT },
    { "TEXCOORD3", 0, FLT,  8, 4, REG_RESERVED | REG_OUTPUT },
    { "TEXCOORD4", 0, FLT,  9, 4, REG_RESERVED | REG_OUTPUT },
    { "TEXCOORD5", 0, FLT, 10, 4, REG_RESERVED | REG_OUTPUT },
    { "TEXCOORD6", 0, FLT, 11, 4, REG_RESERVED | REG_OUTPUT },
    { "TEXCOORD7", 0, FLT, 12, 4, REG_RESERVED | REG_OUTPUT }
};

static const HlslSemanticDesc inputSemantics_hlslv[] = {
    { "POSITION",     0,  1, SEM_IN | SEM_VARYING, 4,
      HLSL_INTERFACE_VARYING },
    { "BLENDWEIGHT",  0,  1, SEM_IN | SEM_VARYING, 4,
      HLSL_INTERFACE_VARYING },
    { "BLENDINDICES", 0,  1, SEM_IN | SEM_VARYING, 4,
      HLSL_INTERFACE_VARYING },
    { "NORMAL",       0,  1, SEM_IN | SEM_VARYING, 3,
      HLSL_INTERFACE_VARYING },
    { "PSIZE",        0,  1, SEM_IN | SEM_VARYING, 1,
      HLSL_INTERFACE_POINT_SIZE },
    { "TEXCOORD",     0, 16, SEM_IN | SEM_VARYING, 4,
      HLSL_INTERFACE_VARYING },
    { "TANGENT",      0,  1, SEM_IN | SEM_VARYING, 3,
      HLSL_INTERFACE_VARYING },
    { "BINORMAL",     0,  1, SEM_IN | SEM_VARYING, 3,
      HLSL_INTERFACE_VARYING },
    { "COLOR",        0,  2, SEM_IN | SEM_VARYING, 4,
      HLSL_INTERFACE_COLOR }
};

static const HlslSemanticDesc outputSemantics_hlslv[] = {
    { "POSITION", 0, 1, SEM_OUT | SEM_VARYING | SEM_REQUIRED, 4,
      HLSL_INTERFACE_POSITION },
    { "PSIZE",    0, 1, SEM_OUT | SEM_VARYING, 1,
      HLSL_INTERFACE_POINT_SIZE },
    { "FOG",      0, 1, SEM_OUT | SEM_VARYING, 1,
      HLSL_INTERFACE_VARYING },
    { "COLOR",    0, 2, SEM_OUT | SEM_VARYING, 4,
      HLSL_INTERFACE_COLOR },
    { "TEXCOORD", 0, 8, SEM_OUT | SEM_VARYING, 4,
      HLSL_INTERFACE_VARYING }
};

static const HlslSemanticAlias inputAliases_hlslv[] = {
    { "COL0",   "COLOR0" },
    { "COL1",   "COLOR1" },
    { "TEX0",   "TEXCOORD0" },
    { "TEX1",   "TEXCOORD1" },
    { "TEX2",   "TEXCOORD2" },
    { "TEX3",   "TEXCOORD3" },
    { "TEX4",   "TEXCOORD4" },
    { "TEX5",   "TEXCOORD5" },
    { "TEX6",   "TEXCOORD6" },
    { "TEX7",   "TEXCOORD7" },
    { "TEX8",   "TEXCOORD8" },
    { "TEX9",   "TEXCOORD9" },
    { "TEX10",  "TEXCOORD10" },
    { "TEX11",  "TEXCOORD11" },
    { "TEX12",  "TEXCOORD12" },
    { "TEX13",  "TEXCOORD13" },
    { "TEX14",  "TEXCOORD14" },
    { "TEX15",  "TEXCOORD15" },
    { "ATTR0",  "TEXCOORD0" },
    { "ATTR1",  "TEXCOORD1" },
    { "ATTR2",  "TEXCOORD2" },
    { "ATTR3",  "TEXCOORD3" },
    { "ATTR4",  "TEXCOORD4" },
    { "ATTR5",  "TEXCOORD5" },
    { "ATTR6",  "TEXCOORD6" },
    { "ATTR7",  "TEXCOORD7" },
    { "ATTR8",  "TEXCOORD8" },
    { "ATTR9",  "TEXCOORD9" },
    { "ATTR10", "TEXCOORD10" },
    { "ATTR11", "TEXCOORD11" },
    { "ATTR12", "TEXCOORD12" },
    { "ATTR13", "TEXCOORD13" },
    { "ATTR14", "TEXCOORD14" },
    { "ATTR15", "TEXCOORD15" },
    { "ATTRIB0",  "TEXCOORD0" },
    { "ATTRIB1",  "TEXCOORD1" },
    { "ATTRIB2",  "TEXCOORD2" },
    { "ATTRIB3",  "TEXCOORD3" },
    { "ATTRIB4",  "TEXCOORD4" },
    { "ATTRIB5",  "TEXCOORD5" },
    { "ATTRIB6",  "TEXCOORD6" },
    { "ATTRIB7",  "TEXCOORD7" },
    { "ATTRIB8",  "TEXCOORD8" },
    { "ATTRIB9",  "TEXCOORD9" },
    { "ATTRIB10", "TEXCOORD10" },
    { "ATTRIB11", "TEXCOORD11" },
    { "ATTRIB12", "TEXCOORD12" },
    { "ATTRIB13", "TEXCOORD13" },
    { "ATTRIB14", "TEXCOORD14" },
    { "ATTRIB15", "TEXCOORD15" }
};

static const HlslSemanticAlias outputAliases_hlslv[] = {
    { "HPOS", "POSITION0" },
    { "COL0", "COLOR0" },
    { "COL1", "COLOR1" },
    { "TEX0", "TEXCOORD0" },
    { "TEX1", "TEXCOORD1" },
    { "TEX2", "TEXCOORD2" },
    { "TEX3", "TEXCOORD3" },
    { "TEX4", "TEXCOORD4" },
    { "TEX5", "TEXCOORD5" },
    { "TEX6", "TEXCOORD6" },
    { "TEX7", "TEXCOORD7" }
};

// Connector descriptors with input/output CIDs:
static ConnectorDescriptor connectors_hlslv[] = {
    { "hlslv_in",  0, CID_HLSLV_IN_ID,  CONNECTOR_IS_INPUT,
      NUMELS(inputCRegs_hlslv),  inputCRegs_hlslv },
    { "hlslv_out", 0, CID_HLSLV_OUT_ID, CONNECTOR_IS_OUTPUT,
      NUMELS(outputCRegs_hlslv), outputCRegs_hlslv },
};

const HlslProfileDesc HlslProfile_hlslv = {
    HLSL_STAGE_VERTEX,
    PROFILE_HLSLV_NAME,
    "vs_3_0",
    PROFILE_HLSLV_ID,
    CID_HLSLV_IN_ID,
    CID_HLSLV_OUT_ID,
    connectors_hlslv,
    NUMELS(connectors_hlslv),
    inputSemantics_hlslv,
    NUMELS(inputSemantics_hlslv),
    inputAliases_hlslv,
    NUMELS(inputAliases_hlslv),
    outputSemantics_hlslv,
    NUMELS(outputSemantics_hlslv),
    outputAliases_hlslv,
    NUMELS(outputAliases_hlslv),
    inputCRegs_hlslv,
    NUMELS(inputCRegs_hlslv),
    outputCRegs_hlslv,
    NUMELS(outputCRegs_hlslv),
    &limits_hlslv
};

///////////////////////////////////////////////////////////////////////////////
/////////////////////////// End of hlslv_hal.c ////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
