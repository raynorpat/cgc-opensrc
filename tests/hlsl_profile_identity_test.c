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
license, under NVIDIA's copyrights in this original NVIDIA software
(the "NVIDIA Software"), to use, reproduce, modify and redistribute the
NVIDIA Software, with or without modifications, in source and/or binary
forms; provided that if you redistribute the NVIDIA Software, you must
retain the copyright notice of NVIDIA, this notice and the following
text and disclaimers in all such redistributions of the NVIDIA Software.
Neither the name, trademarks, service marks nor logos of NVIDIA
Corporation may be used to endorse or promote products derived from
this NVIDIA Software without specific prior written permission from NVIDIA.
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
// hlsl_profile_identity_test.c
//

#if defined(NDEBUG)
#undef NDEBUG
#endif
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "hlsl_hal.h"

CgStruct *Cg;
Scope *CurrentScope;

int InitHAL_hlslv(slHAL *hal)
{
    (void) hal;
    return 1;
}

int InitHAL_hlslf(slHAL *hal)
{
    (void) hal;
    return 1;
}

static slProfile *FindProfile(const char *name)
{
    slProfile *profile;
    int index;

    for (index = 0; (profile = EnumerateProfiles(index)) != NULL; index++) {
        if (!strcmp(profile->name, name))
            return profile;
    }
    return NULL;
}

static int AssertProfileIdentity(const slProfile *profile,
                                 CgProfileStage stage,
                                 const char *wildcard)
{
    int valid;

    if (profile == NULL) {
        fprintf(stderr, "missing registered profile for %s\n", wildcard);
        return 0;
    }
    valid = 1;
    if (profile->profileIdentity.stage != stage) {
        fprintf(stderr, "%s stage: expected %d, got %d\n", profile->name,
                stage, profile->profileIdentity.stage);
        valid = 0;
    }
    if (profile->profileIdentity.wildcardCount != 1) {
        fprintf(stderr, "%s wildcard count: expected 1, got %d\n",
                profile->name, profile->profileIdentity.wildcardCount);
        valid = 0;
    }
    if (profile->profileIdentity.wildcards == NULL) {
        fprintf(stderr, "%s wildcard atom is missing\n", profile->name);
        valid = 0;
    } else if (strcmp(GetAtomString(atable,
                       profile->profileIdentity.wildcards[0]), wildcard)) {
        fprintf(stderr, "%s wildcard: expected %s, got %s\n", profile->name,
                wildcard, GetAtomString(atable,
                    profile->profileIdentity.wildcards[0]));
        valid = 0;
    }
    if (profile->profileIdentity.specificity == NULL) {
        fprintf(stderr, "%s specificity is missing\n", profile->name);
        valid = 0;
    } else if (profile->profileIdentity.specificity[0] != 10) {
        fprintf(stderr, "%s specificity: expected 10, got %d\n",
                profile->name, profile->profileIdentity.specificity[0]);
        valid = 0;
    }
    return valid;
}

int main(void)
{
    CgStruct cg;
    slProfile *fragment;
    slProfile *vertex;
    int valid;

    memset(&cg, 0, sizeof(cg));
    Cg = &cg;
    if (!InitAtomTable(atable, 0) || !RegisterProfiles_hlsl())
        return 2;

    vertex = FindProfile(PROFILE_HLSLV_NAME);
    fragment = FindProfile(PROFILE_HLSLF_NAME);
    valid = AssertProfileIdentity(vertex, CG_PROFILE_STAGE_VERTEX, "vs");
    valid &= AssertProfileIdentity(fragment, CG_PROFILE_STAGE_FRAGMENT, "ps");

    FreeAtomTable(atable);
    return valid ? 0 : 1;
}
