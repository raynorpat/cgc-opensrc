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

#ifndef __ARB_LOWER_INTERNAL_H
#define __ARB_LOWER_INTERNAL_H

#include "slglobals.h"
#include "cg_stdlib.h"
#include "arb_ir.h"

typedef struct ConsumedStmt_Rec {
    struct ConsumedStmt_Rec *next;
    stmt *stmt;
} ConsumedStmt;

typedef struct LoopInit_Rec {
    struct LoopInit_Rec *next;
    stmt *loop;
    Symbol *symbol;
    int value;
} LoopInit;

typedef struct ArbLowerContext_Rec {
    ArbProgram *ir;
    const ArbProfileDesc *profile;
    Symbol *program;
    struct ArbStaticValue_Rec *staticValues;
    int dstSelValid;        // Target selection order recorded by LowerLValue
    int dstSelWidth;
    int dstSel[4];
    ConsumedStmt *consumedHead; // Statements claimed by loop analysis
    int attrRegno[16];          // Seen vertex attribute registers
    int attrName[16];           // Binding spelling atom per register
    int attrCount;
    LoopInit *loopInitHead;     // Paired while/do initializers
} ArbLowerContext;

#define ARB_MAX_UNROLL 256

typedef struct ArbStaticValue_Rec {
    struct ArbStaticValue_Rec *next;
    Symbol *symbol;
    int value;
} ArbStaticValue;

int ArbLowerMaskFromType(Type *fType);
int ArbLowerGetSymbolTemp(ArbLowerContext *ctx, Symbol *symbol);
int ArbLowerGetSymbolTempBlock(ArbLowerContext *ctx, Symbol *symbol, int *count);
void ArbLowerClearSymbolTemps(ArbLowerContext *ctx);
void ArbLowerTrackSymbolTemp(ArbLowerContext *ctx, Symbol *symbol);
int ArbLowerStaticFind(const ArbLowerContext *ctx, Symbol *symbol, int *value);
void ArbLowerMarkConsumedStmt(ArbLowerContext *ctx, stmt *fStmt);
int ArbLowerIsConsumedStmt(ArbLowerContext *ctx, stmt *fStmt);
int ArbLowerUniformQuadBase(Symbol *symb);
int ArbLowerConnectorMember(ArbLowerContext *ctx, expr *expression,
                                ArbOperand *operand);
int ArbLowerLValue(ArbLowerContext *ctx, expr *expression,
                       ArbOperand *operand, int *mask);
void ArbLowerPairWhileInitializers(ArbLowerContext *ctx, stmt *list);
int ArbLowerCanonicalFor(ArbLowerContext *ctx, stmt *fStmt);
int ArbLowerCanonicalWhileDo(ArbLowerContext *ctx, stmt *fStmt,
                                 int testFirst);
int ArbLowerStatement(ArbLowerContext *ctx, stmt *statement);
ArbOperand ArbLowerSmearOperand(ArbOperand operand);
int ArbLowerEmitBinary(ArbLowerContext *ctx, ArbOpcode opcode, int mask,
                      const SourceLoc *loc, ArbOperand a, ArbOperand b,
                      ArbOperand *result);
int ArbLowerEmitUnary(ArbLowerContext *ctx, ArbOpcode opcode, int mask,
                     const SourceLoc *loc, ArbOperand a, ArbOperand *result);
int ArbLowerBuildSelect(ArbLowerContext *ctx, const SourceLoc *loc,
                       ArbOperand cond, ArbOperand tv, ArbOperand fv,
                       int width, int mask, ArbOperand *result);
int ArbLowerTypeWidth(Type *fType);
int ArbLowerWidthMask(int width);
void ArbLowerComposeSwizzledSource(ArbOperand *operand, const int *selection,
                                  int width);
int ArbLowerSwizzleNode(ArbLowerContext *ctx, expr *expression,
                            ArbOperand *operand);
int ArbLowerExpression(ArbLowerContext *ctx, expr *expression,
                           ArbOperand *operand);

#endif // __ARB_LOWER_INTERNAL_H
