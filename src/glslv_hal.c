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
// glslv_hal.c
//

#include <stdio.h>

#include "slglobals.h"
#include "glsl_hal.h"

#define NUMELS(x) (sizeof(x) / sizeof((x)[0]))
#define FLT TYPE_BASE_FLOAT
#define INT TYPE_BASE_INT

static ConnectorRegisters inputRegs_glslv[] = {
    { "ATTRIB0",       0, FLT,  0, 4, REG_ALLOC | REG_INPUT },
    { "ATTRIB1",       0, FLT,  1, 4, REG_ALLOC | REG_INPUT },
    { "ATTRIB2",       0, FLT,  2, 4, REG_ALLOC | REG_INPUT },
    { "ATTRIB3",       0, FLT,  3, 4, REG_ALLOC | REG_INPUT },
    { "ATTRIB4",       0, FLT,  4, 4, REG_ALLOC | REG_INPUT },
    { "ATTRIB5",       0, FLT,  5, 4, REG_ALLOC | REG_INPUT },
    { "ATTRIB6",       0, FLT,  6, 4, REG_ALLOC | REG_INPUT },
    { "ATTRIB7",       0, FLT,  7, 4, REG_ALLOC | REG_INPUT },
    { "ATTRIB8",       0, FLT,  8, 4, REG_ALLOC | REG_INPUT },
    { "ATTRIB9",       0, FLT,  9, 4, REG_ALLOC | REG_INPUT },
    { "ATTRIB10",      0, FLT, 10, 4, REG_ALLOC | REG_INPUT },
    { "ATTRIB11",      0, FLT, 11, 4, REG_ALLOC | REG_INPUT },
    { "ATTRIB12",      0, FLT, 12, 4, REG_ALLOC | REG_INPUT },
    { "ATTRIB13",      0, FLT, 13, 4, REG_ALLOC | REG_INPUT },
    { "ATTRIB14",      0, FLT, 14, 4, REG_ALLOC | REG_INPUT },
    { "ATTRIB15",      0, FLT, 15, 4, REG_ALLOC | REG_INPUT },
    { "POSITION0",     0, FLT, 16, 4, REG_RESERVED | REG_INPUT },
    { "NORMAL0",       0, FLT, 17, 3, REG_RESERVED | REG_INPUT },
    { "COLOR0",        0, FLT, 18, 4, REG_RESERVED | REG_INPUT },
    { "COLOR1",        0, FLT, 19, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD0",     0, FLT, 20, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD1",     0, FLT, 21, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD2",     0, FLT, 22, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD3",     0, FLT, 23, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD4",     0, FLT, 24, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD5",     0, FLT, 25, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD6",     0, FLT, 26, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD7",     0, FLT, 27, 4, REG_RESERVED | REG_INPUT },
    { "TANGENT0",      0, FLT, 28, 3, REG_RESERVED | REG_INPUT },
    { "BINORMAL0",     0, FLT, 29, 3, REG_RESERVED | REG_INPUT },
    { "BLENDWEIGHT0",  0, FLT, 30, 4, REG_RESERVED | REG_INPUT },
    { "BLENDINDICES0", 0, FLT, 31, 4, REG_RESERVED | REG_INPUT },
    { "VERTEXID0",     0, INT, 32, 1, REG_RESERVED | REG_INPUT }
};

static ConnectorRegisters outputRegs_glslv[] = {
    { "POSITION0", 0, FLT,  0, 4, REG_RESERVED | REG_OUTPUT | REG_WRITE_REQUIRED },
    { "COLOR0",    0, FLT,  1, 4, REG_RESERVED | REG_OUTPUT },
    { "COLOR1",    0, FLT,  2, 4, REG_RESERVED | REG_OUTPUT },
    { "TEXCOORD0", 0, FLT,  3, 4, REG_ALLOC | REG_OUTPUT },
    { "TEXCOORD1", 0, FLT,  4, 4, REG_ALLOC | REG_OUTPUT },
    { "TEXCOORD2", 0, FLT,  5, 4, REG_ALLOC | REG_OUTPUT },
    { "TEXCOORD3", 0, FLT,  6, 4, REG_ALLOC | REG_OUTPUT },
    { "TEXCOORD4", 0, FLT,  7, 4, REG_ALLOC | REG_OUTPUT },
    { "TEXCOORD5", 0, FLT,  8, 4, REG_ALLOC | REG_OUTPUT },
    { "TEXCOORD6", 0, FLT,  9, 4, REG_ALLOC | REG_OUTPUT },
    { "TEXCOORD7", 0, FLT, 10, 4, REG_ALLOC | REG_OUTPUT },
    { "FOG0",      0, FLT, 11, 1, REG_RESERVED | REG_OUTPUT },
    { "PSIZE0",    0, FLT, 12, 1, REG_RESERVED | REG_OUTPUT },
    { "VERTEXID0", 0, INT, 13, 1, REG_RESERVED | REG_OUTPUT }
};

static ConnectorDescriptor connectors_glslv[] = {
    { CID_GLSLV_IN_NAME, 0, CID_GLSLV_IN_ID, CONNECTOR_IS_INPUT,
      NUMELS(inputRegs_glslv), inputRegs_glslv },
    { CID_GLSLV_OUT_NAME, 0, CID_GLSLV_OUT_ID, CONNECTOR_IS_OUTPUT,
      NUMELS(outputRegs_glslv), outputRegs_glslv }
};

static GlslSemanticDesc semanticMap_glslv[] = {
    { "ATTRIB",       "ATTRIB",       0, 16, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_ATTRIBUTE },
    { "POSITION",     "POSITION",     0,  1, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_ATTRIBUTE },
    { "NORMAL",       "NORMAL",       0,  1, SEM_IN | SEM_VARYING, 3, GLSL_INTERFACE_ATTRIBUTE },
    { "COLOR",        "COLOR",        0,  2, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_ATTRIBUTE },
    { "TEXCOORD",     "TEXCOORD",     0,  8, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_ATTRIBUTE },
    { "TANGENT",      "TANGENT",      0,  1, SEM_IN | SEM_VARYING, 3, GLSL_INTERFACE_ATTRIBUTE },
    { "BINORMAL",     "BINORMAL",     0,  1, SEM_IN | SEM_VARYING, 3, GLSL_INTERFACE_ATTRIBUTE },
    { "BLENDWEIGHT",  "BLENDWEIGHT",  0,  1, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_ATTRIBUTE },
    { "BLENDINDICES", "BLENDINDICES", 0,  1, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_ATTRIBUTE },
    { "VERTEXID",     "VERTEXID",     0,  1, SEM_IN | SEM_VARYING, 1, GLSL_INTERFACE_VERTEX_ID },
    { "POSITION",     "POSITION",     0,  1, SEM_OUT | SEM_VARYING | SEM_REQUIRED, 4, GLSL_INTERFACE_POSITION },
    { "COLOR",        "COLOR",        0,  2, SEM_OUT | SEM_VARYING, 4, GLSL_INTERFACE_VARYING },
    { "TEXCOORD",     "TEXCOORD",     0,  8, SEM_OUT | SEM_VARYING, 4, GLSL_INTERFACE_VARYING },
    { "FOG",          "FOG",          0,  1, SEM_OUT | SEM_VARYING, 1, GLSL_INTERFACE_VARYING },
    { "PSIZE",        "PSIZE",        0,  1, SEM_OUT | SEM_VARYING, 1, GLSL_INTERFACE_POINT_SIZE },
    { "VERTEXID",     "VERTEXID",     0,  1, SEM_OUT | SEM_VARYING, 1, GLSL_INTERFACE_USER }
};

static GlslSemanticAlias aliases_glslv[] = {
    { "DIFFUSE",  "COLOR0" },
    { "SPECULAR", "COLOR1" },
    { "FOGCOORD", "FOG0" },
    { "HPOS",     "POSITION0" }
};

static GlslProfileDesc profile_glslv = {
    GLSL_STAGE_VERTEX, PROFILE_GLSLV_NAME, PROFILE_GLSLV_ID,
    CID_GLSLV_IN_ID, CID_GLSLV_OUT_ID,
    connectors_glslv, NUMELS(connectors_glslv),
    semanticMap_glslv, NUMELS(semanticMap_glslv),
    aliases_glslv, NUMELS(aliases_glslv),
    inputRegs_glslv, NUMELS(inputRegs_glslv),
    outputRegs_glslv, NUMELS(outputRegs_glslv),
    /* Core 1.50 portable minima: 16 vertex attributes, 1,024 numeric
     * uniform components, 64 vertex output components, 16 texture
     * units. */
    { 16, 0, 64, 0, 0, 1024, 16, 0 }
};

int InitHAL_glslv(slHAL *hal)
{
    return GlslInitHAL(hal, &profile_glslv);
}
