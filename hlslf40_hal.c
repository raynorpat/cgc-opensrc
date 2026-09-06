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
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN
ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
OF THE NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF
CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE,
EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/
// hlslf40_hal.c
//

#include <stdio.h>
#include <stdlib.h>

#include "slglobals.h"
#include "hlsl_hal.h"

static const HlslLimits limits_hlslf40 = {
    32, 8, 0, 0, 0, 16, 8,
    1, 8, 14, 4096, 128, 0, 0
};

static ConnectorRegisters inputRegs_hlslf40[] = {
    { NULL, 0, 0, 0, 0, 0 }
};
static ConnectorRegisters outputRegs_hlslf40[] = {
    { NULL, 0, 0, 0, 0, 0 }
};
static ConnectorDescriptor connectors_hlslf40[] = {
    { "hlslf40_in", 0, CID_HLSLF40_IN_ID, CONNECTOR_IS_INPUT,
      0, inputRegs_hlslf40 },
    { "hlslf40_out", 0, CID_HLSLF40_OUT_ID, CONNECTOR_IS_OUTPUT,
      0, outputRegs_hlslf40 }
};

const HlslProfileDesc HlslProfile_hlslf40 = {
    HLSL_STAGE_PIXEL,
    HLSL_SHADER_MODEL_4,
    HLSL_SYNTAX_MODERN,
    HLSL_SEMANTIC_POLICY_MODERN,
    HLSL_RESOURCE_POLICY_MODERN,
    PROFILE_HLSLF40_NAME,
    "ps_4_0",
    VERSION_STRING_HLSL_SM4,
    PROFILE_HLSLF40_ID,
    CID_HLSLF40_IN_ID,
    CID_HLSLF40_OUT_ID,
    connectors_hlslf40, 2,
    NULL, 0,
    NULL, 0,
    NULL, 0,
    NULL, 0,
    inputRegs_hlslf40, 0,
    outputRegs_hlslf40, 0,
    &limits_hlslf40,
    HLSL_CAP_TEXTURE_METHODS | HLSL_CAP_CBUFFERS |
        HLSL_CAP_DERIVATIVES | HLSL_CAP_DISCARD
};

#if !defined(HLSL_CANONICALIZATION_ONLY)

int InitHAL_hlslf40(slHAL *hal)
{
    return InitHAL_hlsl_profile(hal, &HlslProfile_hlslf40);
}

#endif // !defined(HLSL_CANONICALIZATION_ONLY)
