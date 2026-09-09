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
// arbvp1_hal.c
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "arb_hal.h"

#define NUMELS(x) (sizeof(x) / sizeof((x)[0]))

#define FLT TYPE_BASE_FLOAT

// Vertex input attribute registers.  Register numbers follow the
// ARB_vertex_program vertex attrib numbering; the conventional spellings
// come first so they win canonical-name resolution, then the generic
// ATTR0..ATTR15 aliases refer to the same registers.

static ConnectorRegisters inputCRegs_arbvp1[] = {
    { "POSITION",     0, FLT,  0, 4, REG_ALLOC | REG_INPUT, },
    { "BLENDWEIGHT",  0, FLT,  1, 4, REG_ALLOC | REG_INPUT, },
    { "NORMAL",       0, FLT,  2, 4, REG_ALLOC | REG_INPUT, },
    { "COLOR0",       0, FLT,  3, 4, REG_ALLOC | REG_INPUT, },
    { "COLOR1",       0, FLT,  4, 4, REG_ALLOC | REG_INPUT, },
    { "FOG",          0, FLT,  5, 4, REG_ALLOC | REG_INPUT, },
    { "TEXCOORD0",    0, FLT,  8, 4, REG_ALLOC | REG_INPUT, },
    { "TEXCOORD1",    0, FLT,  9, 4, REG_ALLOC | REG_INPUT, },
    { "TEXCOORD2",    0, FLT, 10, 4, REG_ALLOC | REG_INPUT, },
    { "TEXCOORD3",    0, FLT, 11, 4, REG_ALLOC | REG_INPUT, },
    { "TEXCOORD4",    0, FLT, 12, 4, REG_ALLOC | REG_INPUT, },
    { "TEXCOORD5",    0, FLT, 13, 4, REG_ALLOC | REG_INPUT, },
    { "TEXCOORD6",    0, FLT, 14, 4, REG_ALLOC | REG_INPUT, },
    { "TEXCOORD7",    0, FLT, 15, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR0",        0, FLT,  0, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR1",        0, FLT,  1, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR2",        0, FLT,  2, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR3",        0, FLT,  3, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR4",        0, FLT,  4, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR5",        0, FLT,  5, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR6",        0, FLT,  6, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR7",        0, FLT,  7, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR8",        0, FLT,  8, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR9",        0, FLT,  9, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR10",       0, FLT, 10, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR11",       0, FLT, 11, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR12",       0, FLT, 12, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR13",       0, FLT, 13, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR14",       0, FLT, 14, 4, REG_ALLOC | REG_INPUT, },
    { "ATTR15",       0, FLT, 15, 4, REG_ALLOC | REG_INPUT, },
};

// Vertex result registers.  Register numbers are private indices resolved
// to ARB result names during code generation.  Clip-distance results are
// not part of the base ARB_vertex_program grammar and are excluded.

static ConnectorRegisters outputCRegs_arbvp1[] = {
    { "HPOS",  0, FLT,  0, 4, REG_RESERVED | REG_OUTPUT | REG_WRITE_REQUIRED, },
    { "COL0",  0, FLT,  1, 4, REG_RESERVED | REG_OUTPUT, },
    { "COL1",  0, FLT,  2, 4, REG_RESERVED | REG_OUTPUT, },
    { "BFC0",  0, FLT,  3, 4, REG_RESERVED | REG_OUTPUT, },
    { "BFC1",  0, FLT,  4, 4, REG_RESERVED | REG_OUTPUT, },
    { "FOGC",  0, FLT,  5, 1, REG_RESERVED | REG_OUTPUT, },
    { "PSIZ",  0, FLT,  6, 1, REG_RESERVED | REG_OUTPUT, },
    { "TEX0",  0, FLT,  7, 4, REG_RESERVED | REG_OUTPUT, },
    { "TEX1",  0, FLT,  8, 4, REG_RESERVED | REG_OUTPUT, },
    { "TEX2",  0, FLT,  9, 4, REG_RESERVED | REG_OUTPUT, },
    { "TEX3",  0, FLT, 10, 4, REG_RESERVED | REG_OUTPUT, },
    { "TEX4",  0, FLT, 11, 4, REG_RESERVED | REG_OUTPUT, },
    { "TEX5",  0, FLT, 12, 4, REG_RESERVED | REG_OUTPUT, },
    { "TEX6",  0, FLT, 13, 4, REG_RESERVED | REG_OUTPUT, },
    { "TEX7",  0, FLT, 14, 4, REG_RESERVED | REG_OUTPUT, },
};

enum {
    ARBVP_SEMANTIC_IN_GROUP = 0,
    ARBVP_SEMANTIC_OUT_GROUP = 1,
};

// Varying input semantics:

static SemanticsDescriptor Semantics_arbvp1[] = {
    { "POSITION",    FLT, 4,  0,  0, ARBVP_SEMANTIC_IN_GROUP,  SEM_IN | SEM_VARYING, },
    { "BLENDWEIGHT", FLT, 4,  1,  0, ARBVP_SEMANTIC_IN_GROUP,  SEM_IN | SEM_VARYING, },
    { "NORMAL",      FLT, 4,  2,  0, ARBVP_SEMANTIC_IN_GROUP,  SEM_IN | SEM_VARYING, },
    { "COLOR",       FLT, 4,  3,  2, ARBVP_SEMANTIC_IN_GROUP,  SEM_IN | SEM_VARYING, },
    { "FOG",         FLT, 4,  5,  0, ARBVP_SEMANTIC_IN_GROUP,  SEM_IN | SEM_VARYING, },
    { "TEXCOORD",    FLT, 4,  8,  8, ARBVP_SEMANTIC_IN_GROUP,  SEM_IN | SEM_VARYING, },
    { "ATTR",        FLT, 4,  0, 16, ARBVP_SEMANTIC_IN_GROUP,  SEM_IN | SEM_VARYING, },

    // Varying output semantics:

    { "POSITION",    FLT, 4,  0,  0, ARBVP_SEMANTIC_OUT_GROUP, SEM_OUT | SEM_VARYING | SEM_REQUIRED, },
    { "COLOR",       FLT, 4,  1,  2, ARBVP_SEMANTIC_OUT_GROUP, SEM_OUT | SEM_VARYING, },
    { "BCOL",        FLT, 4,  3,  2, ARBVP_SEMANTIC_OUT_GROUP, SEM_OUT | SEM_VARYING, },
    { "FOG",         FLT, 1,  5,  0, ARBVP_SEMANTIC_OUT_GROUP, SEM_OUT | SEM_VARYING, },
    { "PSIZE",       FLT, 1,  6,  0, ARBVP_SEMANTIC_OUT_GROUP, SEM_OUT | SEM_VARYING, },
    { "TEXCOORD",    FLT, 4,  7,  8, ARBVP_SEMANTIC_OUT_GROUP, SEM_OUT | SEM_VARYING, },
};

static ConnectorDescriptor connectors_arbvp1[] = {
    { "ARBVP1_IN",  0, CID_ARBVP1_IN_ID,  CONNECTOR_IS_INPUT,  NUMELS(inputCRegs_arbvp1),  inputCRegs_arbvp1  },
    { "ARBVP1_OUT", 0, CID_ARBVP1_OUT_ID, CONNECTOR_IS_OUTPUT, NUMELS(outputCRegs_arbvp1), outputCRegs_arbvp1 },
};

// The zero ALU field means ARBVP1 tracks its single instruction limit
// only; separate ALU/texture counters are fragment-profile concepts.

static const ArbLimits limits_arbvp1 = {
    128, 0, 0, 0, 12, 96, 16, 1, 0
};

const ArbProfileDesc ArbProfile_arbvp1 = {
    PROFILE_ARBVP1_NAME,
    PROFILE_ARBVP1_ID,
    ARB_STAGE_VERTEX,
    "!!ARBvp1.0",
    &limits_arbvp1,
    connectors_arbvp1,
    NUMELS(connectors_arbvp1),
    Semantics_arbvp1,
    NUMELS(Semantics_arbvp1)
};

/*
 * InitHAL_arbvp1()
 */

int InitHAL_arbvp1(slHAL *fHAL)
{
    fHAL->InitHAL = InitHAL_arbvp1;
    return InitHAL_arb(fHAL, &ArbProfile_arbvp1);
} // InitHAL_arbvp1
