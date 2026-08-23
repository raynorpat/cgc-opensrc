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
// arb_hal.h
//

#if !defined(__ARB_HAL_H)
#define __ARB_HAL_H 1

#include "hal.h"

#define PROFILE_ARBVP1_NAME "arbvp1"
#define PROFILE_ARBVP1_ID 10
#define PROFILE_ARBFP1_NAME "arbfp1"
#define PROFILE_ARBFP1_ID 11

#define CID_ARBVP1_IN_ID 10
#define CID_ARBVP1_OUT_ID 11
#define CID_ARBFP1_IN_ID 12
#define CID_ARBFP1_OUT_ID 13

/* Minimum limits guaranteed by ARB_vertex_program. */

#define ARBVP_MAX_INSTRUCTIONS 128
#define ARBVP_MAX_TEMPORARIES 12
#define ARBVP_MAX_PARAMETERS 96
#define ARBVP_MAX_ATTRIBUTES 16
#define ARBVP_MAX_ADDRESS_REGISTERS 1

/* Minimum limits guaranteed by ARB_fragment_program. */

#define ARBFP_MAX_INSTRUCTIONS 72
#define ARBFP_MAX_ALU_INSTRUCTIONS 48
#define ARBFP_MAX_TEX_INSTRUCTIONS 24
#define ARBFP_MAX_TEX_INDIRECTIONS 4
#define ARBFP_MAX_TEMPORARIES 16
#define ARBFP_MAX_PARAMETERS 24
#define ARBFP_MAX_ATTRIBUTES 10
#define ARBFP_MAX_TEXTURE_UNITS 2

#define TYPE_BASE_SAMPLER1D   (TYPE_BASE_FIRST_USER + 0)
#define TYPE_BASE_SAMPLER2D   (TYPE_BASE_FIRST_USER + 1)
#define TYPE_BASE_SAMPLER3D   (TYPE_BASE_FIRST_USER + 2)
#define TYPE_BASE_SAMPLERCUBE (TYPE_BASE_FIRST_USER + 3)
#define TYPE_BASE_SAMPLERRECT (TYPE_BASE_FIRST_USER + 4)

typedef enum ArbStage_Enum {
    ARB_STAGE_VERTEX,
    ARB_STAGE_FRAGMENT
} ArbStage;

#define ARB_BUILTIN_GROUP 1

typedef enum ArbBuiltin_Enum {
    ARB_BUILTIN_RSQ = 1,
    ARB_BUILTIN_TEX1D,
    ARB_BUILTIN_TEX1DPROJ,
    ARB_BUILTIN_TEX1DBIAS,
    ARB_BUILTIN_TEX2D,
    ARB_BUILTIN_TEX2DPROJ,
    ARB_BUILTIN_TEX2DBIAS,
    ARB_BUILTIN_TEX3D,
    ARB_BUILTIN_TEX3DPROJ,
    ARB_BUILTIN_TEX3DBIAS,
    ARB_BUILTIN_TEXCUBE,
    ARB_BUILTIN_TEXCUBEPROJ,
    ARB_BUILTIN_TEXCUBEBIAS,
    ARB_BUILTIN_TEXRECT,
    ARB_BUILTIN_TEXRECTPROJ
} ArbBuiltin;

typedef struct ArbLimits_Rec {
    int instructions;
    int aluInstructions;
    int texInstructions;
    int texIndirections;
    int temporaries;
    int parameters;
    int attributes;
    int addressRegisters;
    int textureUnits;
} ArbLimits;

typedef struct ArbProfileDesc_Rec {
    const char *name;
    int pid;
    ArbStage stage;
    const char *header;
    const ArbLimits *limits;
    ConnectorDescriptor *connectors;
    int numConnectors;
    SemanticsDescriptor *semantics;
    int numSemantics;
} ArbProfileDesc;

typedef struct ArbHALData_Rec {
    const ArbProfileDesc *profile;
    int nextUniform;
    unsigned char uniformUsed[ARBVP_MAX_PARAMETERS];
    int nextTextureUnit;
    signed char textureTarget[ARBFP_MAX_TEXTURE_UNITS];
} ArbHALData;

extern const ArbProfileDesc ArbProfile_arbvp1;
extern const ArbProfileDesc ArbProfile_arbfp1;

int RegisterProfiles_arb(void);
int InitHAL_arbvp1(slHAL *fHAL);
int InitHAL_arbfp1(slHAL *fHAL);
int InitHAL_arb(slHAL *fHAL, const ArbProfileDesc *profile);
int GenerateCode_arb(SourceLoc *loc, Scope *fScope, Symbol *program);

#endif /* !defined(__ARB_HAL_H) */
