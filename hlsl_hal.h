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
// hlsl_hal.h
//

#if !defined(__HLSL_HAL_H)
#define __HLSL_HAL_H 1

#include "hal.h"
#include "hlsl_ir.h"

#define VENDOR_STRING_HLSL         "Microsoft"
#define VERSION_STRING_HLSL        "DirectX 9.0c Shader Model 3"
#define PROFILE_HLSLV_NAME         "hlslv"
#define PROFILE_HLSLV_ID           14
#define PROFILE_HLSLF_NAME         "hlslf"
#define PROFILE_HLSLF_ID           15
#define CID_HLSLV_IN_ID            18
#define CID_HLSLV_OUT_ID           19
#define CID_HLSLF_IN_ID            20
#define CID_HLSLF_OUT_ID           21
#define HLSL_BUILTIN_GROUP         4

typedef struct HlslLimits_Rec {
    int inputs;
    int outputs;
    int floatConstants;
    int intConstants;
    int boolConstants;
    int samplers;
    int colorOutputs;
} HlslLimits;

typedef enum HlslInterface_Enum {
    HLSL_INTERFACE_VARYING,
    HLSL_INTERFACE_POSITION,
    HLSL_INTERFACE_POINT_SIZE,
    HLSL_INTERFACE_PIXEL_POSITION,
    HLSL_INTERFACE_FACE,
    HLSL_INTERFACE_COLOR,
    HLSL_INTERFACE_DEPTH
} HlslInterface;

typedef struct HlslSemanticDesc_Rec {
    const char *root;
    int firstIndex;
    int count;
    int properties;
    int width;
    HlslInterface interfaceKind;
} HlslSemanticDesc;

typedef struct HlslSemanticAlias_Rec {
    const char *source;
    const char *target;
} HlslSemanticAlias;

struct HlslProfileDesc_Rec {
    HlslStage stage;
    const char *name;
    const char *target;
    int pid;
    int inputCid;
    int outputCid;
    ConnectorDescriptor *connectors;
    int numConnectors;
    const HlslSemanticDesc *inputSemantics;
    int numInputSemantics;
    const HlslSemanticAlias *inputAliases;
    int numInputAliases;
    const HlslSemanticDesc *outputSemantics;
    int numOutputSemantics;
    const HlslSemanticAlias *outputAliases;
    int numOutputAliases;
    ConnectorRegisters *inputRegs;
    int numInputRegs;
    ConnectorRegisters *outputRegs;
    int numOutputRegs;
    const HlslLimits *limits;
};

// Stage descriptors:
extern const HlslProfileDesc HlslProfile_hlslv;
extern const HlslProfileDesc HlslProfile_hlslf;

// Profile registration:
int RegisterProfiles_hlsl(void);
int InitHAL_hlslv(slHAL *hal);
int InitHAL_hlslf(slHAL *hal);

// Interface semantic helpers:
int HlslParseSemantic(const char *semantic, char *root, size_t rootSize,
    int *index);
int HlslDescribeSourceType(const Type *source, HlslSourceType *target);
const char *HlslCanonicalSemantic(const HlslProfileDesc *profile,
    const char *semantic, int IsOutVal);

// HLSL backend phases:
int HlslLowerProgram(HlslModule *module, const HlslProfileDesc *profile,
    SourceLoc *loc, Scope *scope, Symbol *program);
int HlslBuildEntryWrapper(HlslModule *module,
    const HlslProfileDesc *profile);
int HlslLegalizeModule(HlslModule *module,
    const HlslProfileDesc *profile);
int HlslAllocateBindings(HlslModule *module,
    const HlslProfileDesc *profile);
int HlslValidateModule(HlslModule *module,
    const HlslProfileDesc *profile);

#endif // !defined(__HLSL_HAL_H)
