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
ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER PRODUCTS.

IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL,
EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, LOST
PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/
/* glslg_hal.c */

#include <stdio.h>

#include "slglobals.h"
#include "glsl_hal.h"

#define NUMELS(x) (sizeof(x) / sizeof((x)[0]))
#define FLT TYPE_BASE_FLOAT
#define INT TYPE_BASE_INT

static ConnectorRegisters inputRegs_glslg[] = {
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
    { "PSIZE0",        0, FLT, 17, 1, REG_RESERVED | REG_INPUT },
    { "FOG0",          0, FLT, 18, 1, REG_RESERVED | REG_INPUT },
    { "COLOR0",        0, FLT, 19, 4, REG_RESERVED | REG_INPUT },
    { "COLOR1",        0, FLT, 20, 4, REG_RESERVED | REG_INPUT },
    { "BCOL0",         0, FLT, 21, 4, REG_RESERVED | REG_INPUT },
    { "BCOL1",         0, FLT, 22, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD0",     0, FLT, 23, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD1",     0, FLT, 24, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD2",     0, FLT, 25, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD3",     0, FLT, 26, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD4",     0, FLT, 27, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD5",     0, FLT, 28, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD6",     0, FLT, 29, 4, REG_RESERVED | REG_INPUT },
    { "TEXCOORD7",     0, FLT, 30, 4, REG_RESERVED | REG_INPUT },
    { "CLP0",          0, FLT, 31, 1, REG_RESERVED | REG_INPUT },
    { "CLP1",          0, FLT, 32, 1, REG_RESERVED | REG_INPUT },
    { "CLP2",          0, FLT, 33, 1, REG_RESERVED | REG_INPUT },
    { "CLP3",          0, FLT, 34, 1, REG_RESERVED | REG_INPUT },
    { "CLP4",          0, FLT, 35, 1, REG_RESERVED | REG_INPUT },
    { "CLP5",          0, FLT, 36, 1, REG_RESERVED | REG_INPUT },
    { "INSTANCEID0",   0, INT, 37, 1, REG_RESERVED | REG_INPUT },
    { "VERTEXID0",     0, INT, 38, 1, REG_RESERVED | REG_INPUT },
    { "PRIMITIVEID0",  0, INT, 39, 1, REG_RESERVED | REG_INPUT }
};

static ConnectorRegisters outputRegs_glslg[] = {
    { "POSITION0",    0, FLT,  0, 4, REG_RESERVED | REG_OUTPUT | REG_WRITE_REQUIRED },
    { "PSIZE0",       0, FLT,  1, 1, REG_RESERVED | REG_OUTPUT },
    { "FOG0",         0, FLT,  2, 1, REG_RESERVED | REG_OUTPUT },
    { "COLOR0",       0, FLT,  3, 4, REG_RESERVED | REG_OUTPUT },
    { "COLOR1",       0, FLT,  4, 4, REG_RESERVED | REG_OUTPUT },
    { "BCOL0",        0, FLT,  5, 4, REG_RESERVED | REG_OUTPUT },
    { "BCOL1",        0, FLT,  6, 4, REG_RESERVED | REG_OUTPUT },
    { "TEXCOORD0",    0, FLT,  7, 4, REG_ALLOC | REG_OUTPUT },
    { "TEXCOORD1",    0, FLT,  8, 4, REG_ALLOC | REG_OUTPUT },
    { "TEXCOORD2",    0, FLT,  9, 4, REG_ALLOC | REG_OUTPUT },
    { "TEXCOORD3",    0, FLT, 10, 4, REG_ALLOC | REG_OUTPUT },
    { "TEXCOORD4",    0, FLT, 11, 4, REG_ALLOC | REG_OUTPUT },
    { "TEXCOORD5",    0, FLT, 12, 4, REG_ALLOC | REG_OUTPUT },
    { "TEXCOORD6",    0, FLT, 13, 4, REG_ALLOC | REG_OUTPUT },
    { "TEXCOORD7",    0, FLT, 14, 4, REG_ALLOC | REG_OUTPUT },
    { "CLP0",         0, FLT, 15, 1, REG_RESERVED | REG_OUTPUT },
    { "CLP1",         0, FLT, 16, 1, REG_RESERVED | REG_OUTPUT },
    { "CLP2",         0, FLT, 17, 1, REG_RESERVED | REG_OUTPUT },
    { "CLP3",         0, FLT, 18, 1, REG_RESERVED | REG_OUTPUT },
    { "CLP4",         0, FLT, 19, 1, REG_RESERVED | REG_OUTPUT },
    { "CLP5",         0, FLT, 20, 1, REG_RESERVED | REG_OUTPUT },
    { "PRIMITIVEID0", 0, INT, 21, 1, REG_RESERVED | REG_OUTPUT },
    { "LAYER0",       0, INT, 22, 1, REG_RESERVED | REG_OUTPUT }
};

static ConnectorDescriptor connectors_glslg[] = {
    { CID_GLSLG_IN_NAME, 0, CID_GLSLG_IN_ID, CONNECTOR_IS_INPUT,
      NUMELS(inputRegs_glslg), inputRegs_glslg },
    { CID_GLSLG_OUT_NAME, 0, CID_GLSLG_OUT_ID, CONNECTOR_IS_OUTPUT,
      NUMELS(outputRegs_glslg), outputRegs_glslg }
};

static GlslSemanticDesc semanticMap_glslg[] = {
    { "ATTRIB",      "ATTRIB",      0, 16, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_USER },
    { "POSITION",    "POSITION",    0,  1, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_GEOMETRY_POSITION_IN },
    { "PSIZE",       "PSIZE",       0,  1, SEM_IN | SEM_VARYING, 1, GLSL_INTERFACE_USER },
    { "FOG",         "FOG",         0,  1, SEM_IN | SEM_VARYING, 1, GLSL_INTERFACE_USER },
    { "COLOR",       "COLOR",       0,  2, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_USER },
    { "BCOL",        "BCOL",        0,  2, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_USER },
    { "TEXCOORD",    "TEXCOORD",    0,  8, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_USER },
    { "CLP",         "CLP",         0,  6, SEM_IN | SEM_VARYING, 1, GLSL_INTERFACE_USER },
    { "INSTANCEID",  "INSTANCEID",  0,  1, SEM_IN | SEM_VARYING, 1, GLSL_INTERFACE_PRIMITIVE_ID_IN },
    { "VERTEXID",    "VERTEXID",    0,  1, SEM_IN | SEM_VARYING, 1, GLSL_INTERFACE_USER },
    { "PRIMITIVEID", "PRIMITIVEID", 0,  1, SEM_IN | SEM_VARYING, 1, GLSL_INTERFACE_PRIMITIVE_ID_IN },
    { "POSITION",    "POSITION",    0,  1, SEM_OUT | SEM_VARYING | SEM_REQUIRED, 4, GLSL_INTERFACE_POSITION },
    { "PSIZE",       "PSIZE",       0,  1, SEM_OUT | SEM_VARYING, 1, GLSL_INTERFACE_POINT_SIZE },
    { "FOG",         "FOG",         0,  1, SEM_OUT | SEM_VARYING, 1, GLSL_INTERFACE_USER },
    { "COLOR",       "COLOR",       0,  2, SEM_OUT | SEM_VARYING, 4, GLSL_INTERFACE_USER },
    { "BCOL",        "BCOL",        0,  2, SEM_OUT | SEM_VARYING, 4, GLSL_INTERFACE_USER },
    { "TEXCOORD",    "TEXCOORD",    0,  8, SEM_OUT | SEM_VARYING, 4, GLSL_INTERFACE_USER },
    { "CLP",         "CLP",         0,  6, SEM_OUT | SEM_VARYING, 1, GLSL_INTERFACE_USER },
    { "PRIMITIVEID", "PRIMITIVEID", 0,  1, SEM_OUT | SEM_VARYING, 1, GLSL_INTERFACE_PRIMITIVE_ID_OUT },
    { "LAYER",       "LAYER",       0,  1, SEM_OUT | SEM_VARYING, 1, GLSL_INTERFACE_LAYER }
};

static GlslSemanticAlias aliases_glslg[] = {
    { "DIFFUSE",  "COLOR0" },
    { "SPECULAR", "COLOR1" },
    { "FOGCOORD", "FOG0" },
    { "HPOS",     "POSITION0" }
};

static GlslProfileDesc profile_glslg = {
    GLSL_STAGE_GEOMETRY, PROFILE_GLSLG_NAME, PROFILE_GLSLG_ID,
    CID_GLSLG_IN_ID, CID_GLSLG_OUT_ID,
    connectors_glslg, NUMELS(connectors_glslg),
    semanticMap_glslg, NUMELS(semanticMap_glslg),
    aliases_glslg, NUMELS(aliases_glslg),
    inputRegs_glslg, NUMELS(inputRegs_glslg),
    outputRegs_glslg, NUMELS(outputRegs_glslg),
    /* Core 1.50 portable geometry minima: 64 input components per
     * vertex, 128 output components per emitted vertex, 256 output
     * vertices, 1,024 total output components, 1,024 uniform
     * components, and 16 texture units. */
    { 0, 64, 128, 256, 1024, 1024, 16, 0 }
};

const GlslProfileDesc *GlslGeometryProfileDesc(void)
{
    return &profile_glslg;
}

int InitHAL_glslg(slHAL *hal)
{
    return GlslInitHAL(hal, &profile_glslg);
}
