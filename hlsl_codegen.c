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
// hlsl_codegen.c
//

#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "hlsl_hal.h"

static int HlslIsIdentifier(const char *name)
{
    const char *current;

    if (name == NULL ||
        !((name[0] >= 'A' && name[0] <= 'Z') ||
          (name[0] >= 'a' && name[0] <= 'z') || name[0] == '_'))
    {
        return 0;
    }
    for (current = name + 1; *current != '\0'; current++) {
        if (!((*current >= 'A' && *current <= 'Z') ||
              (*current >= 'a' && *current <= 'z') ||
              (*current >= '0' && *current <= '9') || *current == '_'))
        {
            return 0;
        }
    }
    return !HlslIsReservedName(name) || !strcmp(name, "main") ||
           !strncmp(name, "cg_", 3);
}

static int HlslCanWriteModule(const HlslModule *module,
    const HlslProfileDesc *profile, const char **resultName)
{
    const HlslFunction *entry;
    const char *name;

    if (module == NULL || profile == NULL || resultName == NULL ||
        (module->stage != HLSL_STAGE_VERTEX &&
         module->stage != HLSL_STAGE_PIXEL) ||
        module->stage != profile->stage || profile->name == NULL ||
        profile->name[0] == '\0' || profile->target == NULL ||
        profile->target[0] == '\0' || module->entry == NULL)
    {
        return 0;
    }
    entry = module->entry;
    if (module->structs != NULL || module->globals != NULL ||
        module->bindings != NULL || module->wrapper != NULL ||
        module->functions != entry || entry->next != NULL ||
        !HlslIsIdentifier(entry->name) || !entry->isEntry ||
        entry->parameters != NULL || entry->locals != NULL ||
        entry->body != NULL)
    {
        return 0;
    }
    if (entry->result.base != HLSL_BASE_VOID || entry->result.len != 0 ||
        entry->result.rows != 0 || entry->result.cols != 0 ||
        entry->result.arraySize != 0 || entry->result.structName != NULL ||
        entry->result.elementType != NULL || entry->result.members != NULL)
    {
        return 0;
    }
    name = HlslTypeName(&entry->result);
    if (name == NULL || strcmp(name, "void"))
        return 0;
    *resultName = name;
    return 1;
}

int HlslWriteModule(FILE *out, const HlslModule *module,
    const HlslProfileDesc *profile)
{
    const char *resultName;

    if (out == NULL || !HlslCanWriteModule(module, profile, &resultName))
        return 0;
    if (fprintf(out, "// profile %s\n", profile->name) < 0 ||
        fprintf(out, "// target %s\n", profile->target) < 0 ||
        fprintf(out, "%s %s()\n{\n}\n", resultName,
                module->entry->name) < 0)
    {
        return 0;
    }
    return 1;
}
