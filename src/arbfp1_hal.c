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
// arbfp1_hal.c
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "arb_hal.h"

#define NUMELS(x) (sizeof(x) / sizeof((x)[0]))

#define FLT TYPE_BASE_FLOAT

// Fragment input registers.  Register numbers are private indices resolved
// to ARB fragment input names during code generation.

static ConnectorRegisters inputCRegs_arbfp1[] = {
    { "WPOS",  0, FLT,  0, 4, REG_ALLOC | REG_INPUT, },
    { "COL0",  0, FLT,  1, 4, REG_ALLOC | REG_INPUT, },
    { "COL1",  0, FLT,  2, 4, REG_ALLOC | REG_INPUT, },
    { "FOGC",  0, FLT,  3, 4, REG_ALLOC | REG_INPUT, },
    { "TEX0",  0, FLT,  4, 4, REG_ALLOC | REG_INPUT, },
    { "TEX1",  0, FLT,  5, 4, REG_ALLOC | REG_INPUT, },
    { "TEX2",  0, FLT,  6, 4, REG_ALLOC | REG_INPUT, },
    { "TEX3",  0, FLT,  7, 4, REG_ALLOC | REG_INPUT, },
    { "TEX4",  0, FLT,  8, 4, REG_ALLOC | REG_INPUT, },
    { "TEX5",  0, FLT,  9, 4, REG_ALLOC | REG_INPUT, },
    { "TEX6",  0, FLT, 10, 4, REG_ALLOC | REG_INPUT, },
    { "TEX7",  0, FLT, 11, 4, REG_ALLOC | REG_INPUT, },
};

// Fragment result registers:

static ConnectorRegisters outputCRegs_arbfp1[] = {
    { "COL",  0, FLT, 0, 4, REG_RESERVED | REG_OUTPUT | REG_WRITE_REQUIRED, },
    { "DEPR", 0, FLT, 1, 1, REG_RESERVED | REG_OUTPUT, },
};

enum {
    ARBFP_SEMANTIC_IN_GROUP = 0,
    ARBFP_SEMANTIC_OUT_GROUP = 1,
};

// Varying input semantics:

static SemanticsDescriptor Semantics_arbfp1[] = {
    { "POSITION", FLT, 4, 0,  0, ARBFP_SEMANTIC_IN_GROUP, SEM_IN | SEM_VARYING, },
    { "WPOS",     FLT, 4, 0,  0, ARBFP_SEMANTIC_IN_GROUP, SEM_IN | SEM_VARYING, },
    { "COLOR",    FLT, 4, 1,  2, ARBFP_SEMANTIC_IN_GROUP, SEM_IN | SEM_VARYING, },
    { "FOG",      FLT, 4, 3,  0, ARBFP_SEMANTIC_IN_GROUP, SEM_IN | SEM_VARYING, },
    { "TEXCOORD", FLT, 4, 4,  8, ARBFP_SEMANTIC_IN_GROUP, SEM_IN | SEM_VARYING, },

    // Varying output semantics:

    { "COLOR",    FLT, 4, 0,  0, ARBFP_SEMANTIC_OUT_GROUP, SEM_OUT | SEM_VARYING | SEM_REQUIRED, },
    { "COLOR0",   FLT, 4, 0,  0, ARBFP_SEMANTIC_OUT_GROUP, SEM_OUT | SEM_VARYING | SEM_REQUIRED, },
    { "DEPTH",    FLT, 1, 1,  0, ARBFP_SEMANTIC_OUT_GROUP, SEM_OUT | SEM_VARYING, },
};

static ConnectorDescriptor connectors_arbfp1[] = {
    { "ARBFP1_IN",  0, CID_ARBFP1_IN_ID,  CONNECTOR_IS_INPUT,  NUMELS(inputCRegs_arbfp1),  inputCRegs_arbfp1  },
    { "ARBFP1_OUT", 0, CID_ARBFP1_OUT_ID, CONNECTOR_IS_OUTPUT, NUMELS(outputCRegs_arbfp1), outputCRegs_arbfp1 },
};

static const ArbLimits limits_arbfp1 = {
    72, 48, 24, 4, 16, 24, 10, 0, 2
};

const ArbProfileDesc ArbProfile_arbfp1 = {
    PROFILE_ARBFP1_NAME,
    PROFILE_ARBFP1_ID,
    ARB_STAGE_FRAGMENT,
    "!!ARBfp1.0",
    &limits_arbfp1,
    connectors_arbfp1,
    NUMELS(connectors_arbfp1),
    Semantics_arbfp1,
    NUMELS(Semantics_arbfp1)
};

/*
 * InitHAL_arbfp1()
 */

int InitHAL_arbfp1(slHAL *fHAL)
{
    fHAL->InitHAL = InitHAL_arbfp1;
    return InitHAL_arb(fHAL, &ArbProfile_arbfp1);
} // InitHAL_arbfp1
