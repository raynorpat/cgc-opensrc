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
// glsl_hal.h
//

#if !defined(__GLSL_HAL_H)
#define __GLSL_HAL_H 1

#include "hal.h"
#include "glsl_ir.h"

#define VENDOR_STRING_GLSL "OpenGL"
#define VERSION_STRING_GLSL "1.50"
#define PROFILE_GLSLV_NAME "glslv"
#define PROFILE_GLSLV_ID 12
#define PROFILE_GLSLF_NAME "glslf"
#define PROFILE_GLSLF_ID 13
#define CID_GLSLV_IN_NAME "glslv_in"
#define CID_GLSLV_IN_ID 14
#define CID_GLSLV_OUT_NAME "glslv_out"
#define CID_GLSLV_OUT_ID 15
#define CID_GLSLF_IN_NAME "glslf_in"
#define CID_GLSLF_IN_ID 16
#define CID_GLSLF_OUT_NAME "glslf_out"
#define CID_GLSLF_OUT_ID 17
#define GLSL_BUILTIN_GROUP 3

#define TYPE_BASE_GLSL_SAMPLER1D   (TYPE_BASE_FIRST_USER + 0)
#define TYPE_BASE_GLSL_SAMPLER2D   (TYPE_BASE_FIRST_USER + 1)
#define TYPE_BASE_GLSL_SAMPLER3D   (TYPE_BASE_FIRST_USER + 2)
#define TYPE_BASE_GLSL_SAMPLERCUBE (TYPE_BASE_FIRST_USER + 3)

typedef struct GlslLimits_Rec {
    int attributes;
    int uniformComponents;
    int varyingComponents;
    int textureUnits;
    int colorOutputs;
} GlslLimits;

typedef enum GlslInterface_Enum {
    GLSL_INTERFACE_ATTRIBUTE,
    GLSL_INTERFACE_VARYING,
    GLSL_INTERFACE_POSITION,
    GLSL_INTERFACE_POINT_SIZE,
    GLSL_INTERFACE_FRAG_COORD,
    GLSL_INTERFACE_FRONT_FACING,
    GLSL_INTERFACE_FRAG_COLOR,
    GLSL_INTERFACE_FRAG_DEPTH,
    /* Fragment COLOR0 in core 1.50: a deterministic user output
     * (cg_COLOR0), not a compatibility built-in.  Kept separate from
     * GLSL_INTERFACE_VARYING so the exact vec4 size rule and the
     * one-color-output limit stay attached to the color semantic. */
    GLSL_INTERFACE_COLOR_OUTPUT
} GlslInterface;

typedef struct GlslSemanticDesc_Rec {
    const char *root;
    const char *canonicalRoot;
    int firstIndex;
    int count;
    int properties;
    int size;
    GlslInterface interfaceKind;
} GlslSemanticDesc;

typedef struct GlslSemanticAlias_Rec {
    const char *alias;
    const char *canonical;
} GlslSemanticAlias;

typedef struct GlslProfileDesc_Rec {
    GlslStage stage;
    const char *name;
    int pid;
    int inputCid;
    int outputCid;
    ConnectorDescriptor *connectors;
    int numConnectors;
    GlslSemanticDesc *semanticMap;
    int numSemanticMap;
    GlslSemanticAlias *aliases;
    int numAliases;
    ConnectorRegisters *inputRegs;
    int numInputRegs;
    ConnectorRegisters *outputRegs;
    int numOutputRegs;
    GlslLimits limits;
} GlslProfileDesc;

int RegisterProfiles_glsl(void);
int GlslInitHAL(slHAL *hal, const GlslProfileDesc *profile);
int InitHAL_glslv(slHAL *hal);
int InitHAL_glslf(slHAL *hal);
/* Cg 2.0 boundary: lower a verified Cg IR module into the GLSL module. */
int GlslLowerCgIR(GlslModule *module, const GlslProfileDesc *profile,
    const CgIRModule *source);
/* Legacy tree consumer, retained only for explicit -version 1.1
 * compiles; see glsl_lower.c. */
int GlslLowerLegacyProgram(GlslModule *module, const GlslProfileDesc *profile,
    SourceLoc *loc, Scope *scope, Symbol *program);
int GlslWriteModule(FILE *out, const GlslModule *module);
const char *GlslCanonicalInterfaceName(const GlslProfileDesc *profile,
    int semantic, int isOutput);

#endif // !defined(__GLSL_HAL_H)
