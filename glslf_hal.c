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
// glslf_hal.c
//

#include <stdio.h>

#include "slglobals.h"
#include "glsl_hal.h"

#define NUMELS(x) (sizeof(x) / sizeof((x)[0]))
#define FLT TYPE_BASE_FLOAT
#define BOOL TYPE_BASE_BOOLEAN

static ConnectorRegisters inputRegs_glslf[] = {
    { "COLOR0",    0, FLT,   0, 4, REG_RESERVED | REG_INPUT },
    { "COLOR1",    0, FLT,   1, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD0", 0, FLT,   2, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD1", 0, FLT,   3, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD2", 0, FLT,   4, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD3", 0, FLT,   5, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD4", 0, FLT,   6, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD5", 0, FLT,   7, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD6", 0, FLT,   8, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD7", 0, FLT,   9, 4, REG_RESERVED | REG_INPUT },
    { "FOG0",      0, FLT,  10, 1, REG_RESERVED | REG_INPUT },
    { "POSITION0", 0, FLT,  11, 4, REG_RESERVED | REG_INPUT },
    { "WPOS0",     0, FLT,  12, 4, REG_RESERVED | REG_INPUT },
    { "FACE0",     0, BOOL, 13, 1, REG_RESERVED | REG_INPUT }
};

static ConnectorRegisters outputRegs_glslf[] = {
    { "COLOR0", 0, FLT, 0, 4, REG_RESERVED | REG_OUTPUT },
    { "DEPTH0", 0, FLT, 1, 1, REG_RESERVED | REG_OUTPUT }
};

static ConnectorDescriptor connectors_glslf[] = {
    { CID_GLSLF_IN_NAME, 0, CID_GLSLF_IN_ID, CONNECTOR_IS_INPUT,
      NUMELS(inputRegs_glslf), inputRegs_glslf },
    { CID_GLSLF_OUT_NAME, 0, CID_GLSLF_OUT_ID, CONNECTOR_IS_OUTPUT,
      NUMELS(outputRegs_glslf), outputRegs_glslf }
};

static GlslSemanticDesc semanticMap_glslf[] = {
    { "COLOR",    "COLOR",    0, 2, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_VARYING },
    { "TEXCOORD", "TEXCOORD", 0, 8, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_VARYING },
    { "FOG",      "FOG",      0, 1, SEM_IN | SEM_VARYING, 1, GLSL_INTERFACE_VARYING },
    { "POSITION", "POSITION", 0, 1, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_FRAG_COORD },
    { "WPOS",     "WPOS",     0, 1, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_FRAG_COORD },
    { "FACE",     "FACE",     0, 1, SEM_IN | SEM_VARYING, 1, GLSL_INTERFACE_FRONT_FACING },
    { "COLOR",    "COLOR",    0, 1, SEM_OUT | SEM_VARYING, 4, GLSL_INTERFACE_COLOR_OUTPUT },
    { "DEPTH",    "DEPTH",    0, 1, SEM_OUT | SEM_VARYING, 1, GLSL_INTERFACE_FRAG_DEPTH }
};

static GlslSemanticAlias aliases_glslf[] = {
    { "DIFFUSE",  "COLOR0" },
    { "SPECULAR", "COLOR1" },
    { "FOGCOORD", "FOG0" }
};

static GlslProfileDesc profile_glslf = {
    GLSL_STAGE_FRAGMENT, PROFILE_GLSLF_NAME, PROFILE_GLSLF_ID,
    CID_GLSLF_IN_ID, CID_GLSLF_OUT_ID,
    connectors_glslf, NUMELS(connectors_glslf),
    semanticMap_glslf, NUMELS(semanticMap_glslf),
    aliases_glslf, NUMELS(aliases_glslf),
    inputRegs_glslf, NUMELS(inputRegs_glslf),
    outputRegs_glslf, NUMELS(outputRegs_glslf),
    /* Core 1.50 portable minima: 1,024 numeric uniform components,
     * 128 fragment input components, 16 texture units, and the
     * existing focused limit of one fragment color output. */
    { 0, 1024, 128, 16, 1 }
};

int InitHAL_glslf(slHAL *hal)
{
    return GlslInitHAL(hal, &profile_glslf);
}
