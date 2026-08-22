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
// glsl_hal.c
//

#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "glsl_hal.h"

static int RegisterNames_glsl(slHAL *hal)
{
    return 1;
}

static int GetConnectorID_glsl(int name)
{
    return CID_NONE_ID;
}

static int CheckInternalFunction_glsl(Symbol *symbol, int *group)
{
    const char *name;

    name = GetAtomString(atable, symbol->name);
    if (!strcmp(name, "rsqrt")) {
        *group = 3;
        return 1;
    }
    return 0;
}

static int FreeHAL_glsl(slHAL *hal)
{
    hal->localData = NULL;
    return 1;
}

static int GetCapsBit_glsl(int bitNumber)
{
    switch (bitNumber) {
    case CAPS_LATE_BINDINGS:
    case CAPS_INDEXED_ARRAYS:
    case CAPS_DONT_FLATTEN_IF_STATEMENTS:
        return 1;
    default:
        return 0;
    }
}

static int PrintCodeHeader_glsl(FILE *out)
{
    fprintf(out, "#version 110\n");
    return 1;
}

static int GenerateCode_glsl(SourceLoc *loc, Scope *scope, Symbol *program)
{
    return 1;
}

int GlslInitHAL(slHAL *hal, const GlslProfileDesc *profile)
{
    hal->FreeHAL = FreeHAL_glsl;
    hal->RegisterNames = RegisterNames_glsl;
    hal->GetCapsBit = GetCapsBit_glsl;
    hal->GetConnectorID = GetConnectorID_glsl;
    hal->CheckInternalFunction = CheckInternalFunction_glsl;
    hal->PrintCodeHeader = PrintCodeHeader_glsl;
    hal->GenerateCode = GenerateCode_glsl;

    hal->vendor = VENDOR_STRING_GLSL;
    hal->version = VERSION_STRING_GLSL;

    hal->semantics = profile->semantics;
    hal->numSemantics = profile->numSemantics;

    hal->incid = profile->inputCid;
    hal->inputCRegs = profile->inputRegs;
    hal->numInputCRegs = profile->numInputRegs;

    hal->outcid = profile->outputCid;
    hal->outputCRegs = profile->outputRegs;
    hal->numOutputCRegs = profile->numOutputRegs;

    hal->comment = "//";
    hal->localData = (void *) profile;

    return 1;
}

int RegisterProfiles_glsl(void)
{
    RegisterProfile(InitHAL_glslv, PROFILE_GLSLV_NAME, PROFILE_GLSLV_ID);
    RegisterProfile(InitHAL_glslf, PROFILE_GLSLF_NAME, PROFILE_GLSLF_ID);
    return 1;
}
