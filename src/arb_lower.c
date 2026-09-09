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
// arb_lower.c - Lower the normalized Cg AST into the private ARB vector IR.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "arb_lower_internal.h"

/*
 * ArbLowerProgram() - Lower the whole program body.  Returns 0 after any
 *         failed statement or expression lowering.
 */

int ArbLowerProgram(ArbProgram *ir, const ArbProfileDesc *profile,
                    Symbol *program)
{
    ArbLowerContext ctx;

    memset(&ctx, 0, sizeof(ctx));
    ctx.ir = ir;
    ctx.profile = profile;
    ctx.program = program;
    ctx.staticValues = NULL;

    if (!ArbLowerStatement(&ctx, program->details.fun.statements)) {
        ArbLowerClearSymbolTemps(&ctx);
        return 0;
    }
    ArbLowerClearSymbolTemps(&ctx);

    if (profile->stage == ARB_STAGE_VERTEX) {
        // Generic ATTRn and conventional spellings must not mix on one
        // attribute register: compare resolved registers and spellings.
        int regnoName[16];
        int ii;
        ArbInstruction *walk;

        for (ii = 0; ii < 16; ii++)
            regnoName[ii] = 0;
        for (walk = ir->first; walk; walk = walk->next) {
            for (ii = 0; ii < walk->srcCount; ii++) {
                if (walk->src[ii].file == ARB_REG_INPUT &&
                    walk->src[ii].index >= 0 && walk->src[ii].index < 16)
                {
                    int name = walk->src[ii].bindingName;
                    if (regnoName[walk->src[ii].index] == 0)
                        regnoName[walk->src[ii].index] = name;
                    else if (regnoName[walk->src[ii].index] != name)
                    {
                        SemanticError(&program->loc,
                                      ERROR___ARB_ATTRIBUTE_ALIAS);
                        return 0;
                    }
                }
            }
        }

        // Required output: POSITION (register 0) must be written.
        for (walk = ir->first; walk; walk = walk->next) {
            if ((walk->dst.file == ARB_REG_OUTPUT ||
                 (walk->dst.file == ARB_REG_TEMP &&
                  walk->dst.index >= 0)) &&
                walk->dst.file == ARB_REG_OUTPUT &&
                walk->dst.index == 0)
            {
                break;
            }
        }
        if (!walk) {
            SemanticError(&program->loc,
                          ERROR___ARB_REQUIRED_POSITION);
            return 0;
        }
    }
    return 1;
} // ArbLowerProgram
