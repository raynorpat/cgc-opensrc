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
NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL,
INDIRECT, INCIDENTAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

#include "msl_lower_internal.h"

const char *MslLOp(CgIROp op)
{
    switch (op) {
    case CGIR_OP_NEGATE: return "-"; case CGIR_OP_POSITIVE: return "+";
    case CGIR_OP_LOGICAL_NOT: return "!"; case CGIR_OP_BITWISE_NOT: return "~";
    case CGIR_OP_PRE_INCREMENT: case CGIR_OP_POST_INCREMENT: return "++";
    case CGIR_OP_PRE_DECREMENT: case CGIR_OP_POST_DECREMENT: return "--";
    case CGIR_OP_MULTIPLY: return "*"; case CGIR_OP_DIVIDE: return "/";
    case CGIR_OP_MODULO: return "%"; case CGIR_OP_ADD: return "+";
    case CGIR_OP_SUBTRACT: return "-"; case CGIR_OP_SHIFT_LEFT: return "<<";
    case CGIR_OP_SHIFT_RIGHT: return ">>"; case CGIR_OP_LESS: return "<";
    case CGIR_OP_GREATER: return ">"; case CGIR_OP_LESS_EQUAL: return "<=";
    case CGIR_OP_GREATER_EQUAL: return ">="; case CGIR_OP_EQUAL: return "==";
    case CGIR_OP_NOT_EQUAL: return "!="; case CGIR_OP_BITWISE_AND: return "&";
    case CGIR_OP_BITWISE_XOR: return "^"; case CGIR_OP_BITWISE_OR: return "|";
    case CGIR_OP_LOGICAL_AND: return "&&"; case CGIR_OP_LOGICAL_OR: return "||";
    case CGIR_OP_ASSIGN: return "="; case CGIR_OP_ADD_ASSIGN: return "+=";
    case CGIR_OP_SUBTRACT_ASSIGN: return "-="; case CGIR_OP_MULTIPLY_ASSIGN: return "*=";
    case CGIR_OP_DIVIDE_ASSIGN: return "/="; case CGIR_OP_MODULO_ASSIGN: return "%=";
    default: return NULL;
    }
}

const char *MslLIntrinsic(Lower *l, const CgIRExpr *e)
{
    switch (e->u.intrinsicCall.intrinsic) {
#define CASE(id, name) case CG_INTRINSIC_##id: return name
    CASE(ABS,"abs"); CASE(MIN,"min"); CASE(MAX,"max"); CASE(CLAMP,"clamp");
    CASE(SATURATE,"saturate"); CASE(FLOOR,"floor"); CASE(CEIL,"ceil"); CASE(FRAC,"fract");
    CASE(FMOD,"fmod"); CASE(SQRT,"sqrt"); CASE(RSQRT,"rsqrt"); CASE(POW,"pow");
    CASE(EXP,"exp"); CASE(EXP2,"exp2"); CASE(LOG,"log"); CASE(LOG2,"log2");
    CASE(SIN,"sin"); CASE(COS,"cos"); CASE(TAN,"tan"); CASE(ASIN,"asin");
    CASE(ACOS,"acos"); CASE(ATAN,"atan"); CASE(ATAN2,"atan2"); CASE(DOT,"dot");
    CASE(CROSS,"cross"); CASE(LENGTH,"length"); CASE(DISTANCE,"distance");
    CASE(NORMALIZE,"normalize"); CASE(REFLECT,"reflect"); CASE(REFRACT,"refract");
    CASE(LERP,"mix"); CASE(STEP,"step"); CASE(SMOOTHSTEP,"smoothstep");
    CASE(ANY,"any"); CASE(ALL,"all");
#undef CASE
    case CG_INTRINSIC_DDX: case CG_INTRINSIC_DDY:
        if (l->m->stage == MSL_FRAGMENT)
            return e->u.intrinsicCall.intrinsic == CG_INTRINSIC_DDX ? "dfdx" : "dfdy";
        break;
    default: break;
    }
    MslFail(l->m, 6602, &e->loc, "unsupported Metal intrinsic or stage"); return NULL;
}

MslExpr *MslLArgs(Lower *l, const CgIRExpr *s)
{
    MslExpr *first = NULL, **tail = &first;
    for (; s && !l->m->failed; s = s->next) {
        *tail = MslLLowerExpr(l, s); if (!*tail) break; tail = &(*tail)->next;
    }
    return first;
}

int MslLStabilize(Lower *l,MslExpr *e,MslExpr ***tail)
{
    MslExpr *index;
    if(e->kind==MSL_SYMBOL) return 1;
    if(e->kind!=MSL_MEMBER && e->kind!=MSL_SWIZZLE && e->kind!=MSL_INDEX && e->kind!=MSL_ROW)
        return MslFail(l->m,6602,&e->loc,"unsupported Metal out argument");
    if(!MslLStabilize(l,e->a,tail)) return 0;
    if(e->kind==MSL_INDEX || e->kind==MSL_ROW) {
        index=MslLTemporary(l,e->b->type,&e->loc); if(!index) return 0;
        MslLSequenceAppend(tail,MslLStore(l,MslLCopyExpr(l,index),e->b)); e->b=index;
    }
    return !l->m->failed;
}

MslExpr *MslLCopyCall(Lower *l,MslExpr *call)
{
    MslExpr *seq,*arg,*next,*temp,*copybacks=NULL,**copyTail=&copybacks,*result=NULL;
    MslExpr **tail,**args; MslDecl *formal;
    for(formal=call->function->parameters;formal && !formal->direction;formal=formal->next) {}
    if(!formal) return call;
    seq=(MslExpr *)MslAlloc(l->m,sizeof(*seq)); if(!seq) return NULL;
    seq->kind=MSL_SEQUENCE; seq->type=call->type; seq->loc=call->loc; tail=&seq->a;
    arg=call->a; args=&call->a;
    for(formal=call->function->parameters;formal && arg;formal=formal->next,arg=next) {
        next=arg->next; arg->next=NULL;
        if(arg->type.base>=MSL_TEXTURE2D) { *args=arg; args=&arg->next; continue; }
        if(formal->direction && !MslLStabilize(l,arg,&tail)) return NULL;
        temp=MslLTemporary(l,formal->type,&arg->loc); if(!temp) return NULL;
        if(formal->direction!=1) MslLSequenceAppend(&tail,MslLStore(l,MslLCopyExpr(l,temp),MslLCopyExpr(l,arg)));
        if(formal->direction) MslLSequenceAppend(&copyTail,MslLStore(l,arg,MslLCopyExpr(l,temp)));
        *args=temp; args=&temp->next;
    }
    if(call->type.base!=MSL_VOID) {
        result=MslLTemporary(l,call->type,&call->loc); if(!result) return NULL;
        MslLSequenceAppend(&tail,MslLStore(l,MslLCopyExpr(l,result),call));
    } else MslLSequenceAppend(&tail,call);
    *tail=copybacks;
    while(*tail) tail=&(*tail)->next;
    if(result) *tail=result;
    return seq;
}

static int MslReadOnlyTarget(Lower *l,const CgIRExpr *root)
{
    MslDecl *decl;
    while(root && (root->kind==CGIR_EXPR_MEMBER || root->kind==CGIR_EXPR_INDEX || root->kind==CGIR_EXPR_SWIZZLE))
        root=root->kind==CGIR_EXPR_MEMBER?root->u.member.object:root->kind==CGIR_EXPR_INDEX?root->u.index.object:root->u.swizzle.object;
    decl=root && root->kind==CGIR_EXPR_SYMBOL?MslLFindDecl(l,root->u.symbol):NULL;
    return decl && decl->readOnly;
}
MslExpr *MslLConvert(Lower *l, MslExpr *value, MslType type)
{
    MslExpr *cast;
    if (!value || (value->type.base == type.base && value->type.lanes == type.lanes && value->type.record == type.record)) return value;
    if (type.base < MSL_BOOL || type.base > MSL_FLOAT || value->type.base < MSL_BOOL || value->type.base > MSL_FLOAT) return value;
    cast=(MslExpr *)MslAlloc(l->m,sizeof(*cast));
    if (!cast) return NULL;
    cast->kind=MSL_CONSTRUCT; cast->type=type; cast->loc=value->loc; cast->a=value;
    return cast;
}

/* Expressions expanded into multiple column accesses must evaluate their
 * operands once, in source order, at the original expression position. */
static MslExpr *Materialize(Lower *l, MslExpr *value, MslExpr ***tail)
{
    MslExpr *temp=MslLTemporary(l,value->type,&value->loc);
    if (!temp) return NULL;
    MslLSequenceAppend(tail,MslLStore(l,MslLCopyExpr(l,temp),value));
    return temp;
}
static MslExpr *ExpandOnce(Lower *l, MslExpr *e, int arguments)
{
    MslExpr *seq,*arg,*next,**tail,**args;
    seq=(MslExpr *)MslAlloc(l->m,sizeof(*seq));
    if (!seq) return NULL;
    seq->kind=MSL_SEQUENCE; seq->type=e->type; seq->loc=e->loc; tail=&seq->a;
    if (arguments) {
        args=&e->a;
        for (arg=e->a;arg;arg=next) {
            next=arg->next; arg->next=NULL;
            *args=Materialize(l,arg,&tail);
            if (!*args) return NULL;
            args=&(*args)->next;
        }
    } else {
        e->a=Materialize(l,e->a,&tail);
        if (e->b) e->b=Materialize(l,e->b,&tail);
    }
    *tail=e;
    return seq;
}
static MslExpr *LowerExprMode(Lower *, const CgIRExpr *, int);
MslExpr *MslLLowerExpr(Lower *l, const CgIRExpr *s)
{
    return LowerExprMode(l,s,0);
}
static MslExpr *LowerExprMode(Lower *l, const CgIRExpr *s, int lvalue)
{
    MslExpr *e; char buf[96]; int i; const CgNumericValue *v;
    if (!s || l->m->failed) return NULL;
    e = (MslExpr *) MslAlloc(l->m, sizeof(*e)); if (!e) return NULL;
    e->type = MslLLowerType(l, s->type, &s->loc); e->loc = s->loc;
    switch (s->kind) {
    case CGIR_EXPR_CONSTANT:
        e->kind = MSL_LITERAL; v = &s->u.constant;
        if (CgScalarIsFloating(v->kind)) {
            sprintf(buf, "%.9e", (double)(float)v->value.f); strcat(buf, "f");
        } else if (v->kind == CG_SCALAR_BOOL) strcpy(buf, v->value.i ? "true" : "false");
        else if (CgScalarIsUnsigned(v->kind)) sprintf(buf, "%luu", (unsigned long)v->value.u);
        else sprintf(buf, "%ld", (long)v->value.i);
        e->text = MslString(l->m, buf); break;
    case CGIR_EXPR_SYMBOL:
        e->kind = MSL_SYMBOL; e->decl = MslLFindDecl(l, s->u.symbol);
        if (!e->decl) MslFail(l->m, 6608, &s->loc, "Metal global storage is not implemented");
        break;
    case CGIR_EXPR_CONSTRUCT:
        if(e->type.base>=MSL_TEXTURE2D) { MslFail(l->m,6602,&s->loc,"resource values cannot be constructed"); break; }
        e->kind = MSL_CONSTRUCT; e->a = MslLArgs(l, s->u.construct.arguments);
        if (!l->m->failed && e->type.base==MSL_MATRIX && s->sideEffects) e=ExpandOnce(l,e,1);
        break;
    case CGIR_EXPR_CAST:
        e->kind = MSL_CONSTRUCT; e->a = MslLLowerExpr(l, s->u.cast.operand); break;
    case CGIR_EXPR_SWIZZLE:
        e->kind = MSL_SWIZZLE; e->a = LowerExprMode(l, s->u.swizzle.object,lvalue);
        for (i = 0; i < s->u.swizzle.componentCount; ++i)
            buf[i] = "xyzw"[(s->u.swizzle.mask >> (i * 2)) & 3];
        buf[i] = 0; e->text = MslString(l->m, buf); break;
    case CGIR_EXPR_LENGTH:
        if(!IsArray(s->u.length.object->type) || s->u.length.object->type->arr.numels<1) {
            MslFail(l->m,6602,&s->loc,"unsupported Metal array length"); break;
        }
        e->kind=MSL_LITERAL; sprintf(buf,"%d",s->u.length.object->type->arr.numels); e->text=MslString(l->m,buf); break;
    case CGIR_EXPR_MEMBER:
        e->kind=MSL_MEMBER; e->a=LowerExprMode(l,s->u.member.object,lvalue);
        e->decl=MslLFindDecl(l,s->u.member.member);
        if(!e->decl) MslFail(l->m,6601,&s->loc,"unresolved Metal struct member");
        break;
    case CGIR_EXPR_INDEX:
        if(s->u.index.object->kind==CGIR_EXPR_INDEX &&
           IsMatrix(s->u.index.object->u.index.object->type,NULL,NULL)) {
            MslExpr *column=(MslExpr *)MslAlloc(l->m,sizeof(*column));
            if(!column) break;
            e->kind=MSL_INDEX; e->a=column;
            column->kind=MSL_INDEX; column->loc=s->loc;
            column->type=e->type; column->type.lanes=s->u.index.object->type->arr.numels;
            column->a=MslLLowerExpr(l,s->u.index.object->u.index.object);
            column->b=MslLLowerExpr(l,s->u.index.index);
            e->b=MslLLowerExpr(l,s->u.index.object->u.index.index); break;
        }
        e->kind = MSL_INDEX; e->a = LowerExprMode(l, s->u.index.object,lvalue);
        e->b = MslLLowerExpr(l, s->u.index.index);
        if(e->a && e->a->type.base==MSL_MATRIX) {
            e->kind=MSL_ROW;
            if (!lvalue && s->sideEffects && !l->m->failed) e=ExpandOnce(l,e,0);
        }
        break;
    case CGIR_EXPR_UNARY:
        if(s->u.unary.op>=CGIR_OP_PRE_INCREMENT && s->u.unary.op<=CGIR_OP_POST_DECREMENT && MslReadOnlyTarget(l,s->u.unary.operand)) {
            MslFail(l->m,6608,&s->loc,"writes to uniform/global storage are unsupported"); break;
        }
        e->kind = MSL_UNARY; e->a = LowerExprMode(l, s->u.unary.operand,1);
        e->text = MslLOp(s->u.unary.op);
        e->postfix = s->u.unary.op == CGIR_OP_POST_INCREMENT || s->u.unary.op == CGIR_OP_POST_DECREMENT;
        break;
    case CGIR_EXPR_BINARY:
        e->kind = MSL_BINARY; e->a = MslLLowerExpr(l, s->u.binary.left);
        e->b = MslLLowerExpr(l, s->u.binary.right); e->text = MslLOp(s->u.binary.op); break;
    case CGIR_EXPR_ASSIGN:
        if(e->type.base>=MSL_TEXTURE2D) { MslFail(l->m,6602,&s->loc,"resource values cannot be copied"); break; }
        if(MslReadOnlyTarget(l,s->u.assign.target)) {
            MslFail(l->m,6608,&s->loc,"writes to uniform/global storage are unsupported"); break;
        }
        e->kind = MSL_ASSIGN; e->a = LowerExprMode(l, s->u.assign.target,1);
        e->b = MslLLowerExpr(l, s->u.assign.value); e->text = MslLOp(s->u.assign.op);
        /* Compound operations retain the RHS scalar kind in normalized Cg IR. */
        if (s->u.assign.op != CGIR_OP_ASSIGN) e->b=MslLConvert(l,e->b,e->type);
        if(e->a && e->a->kind==MSL_ROW) {
            if(strcmp(e->text,"=")) MslFail(l->m,6602,&s->loc,"compound whole-row stores require an explicit row expression");
            else l->m->rowSetters|=1u<<e->a->type.lanes;
        }
        break;
    case CGIR_EXPR_CONDITIONAL:
        e->kind = MSL_SELECT; e->a = MslLLowerExpr(l, s->u.conditional.condition);
        e->b = MslLLowerExpr(l, s->u.conditional.trueExpr); e->c = MslLLowerExpr(l, s->u.conditional.falseExpr); break;
    case CGIR_EXPR_CALL:
        if(s->u.call.callee->details.fun.intrinsic) {
            CgIRExpr intrinsic; CgIntrinsic id=s->u.call.callee->details.fun.intrinsic->intrinsic;
            memset(&intrinsic,0,sizeof(intrinsic)); intrinsic.loc=s->loc;
            intrinsic.u.intrinsicCall.intrinsic=id;
            if(id!=CG_INTRINSIC_MUL && id!=CG_INTRINSIC_TRANSPOSE && !MslLIntrinsic(l,&intrinsic)) break;
        }
        if((s->u.call.callee->properties & SYMB_IS_BUILTIN) && s->u.call.callee->details.fun.group==MSL_BUILTIN_GROUP) {
            MslExpr *coord,*sample,*seq,*store,*uv,*lod; MslType type;
            coord=MslLLowerExpr(l,s->u.call.arguments->next);
            if(!coord) break;
            sample=e; sample->kind=MSL_SAMPLE; sample->a=MslLLowerExpr(l,s->u.call.arguments);
            seq=(MslExpr *)MslAlloc(l->m,sizeof(*seq));
            uv=(MslExpr *)MslAlloc(l->m,sizeof(*uv)); lod=(MslExpr *)MslAlloc(l->m,sizeof(*lod));
            if(!seq || !uv || !lod) break;
            type=coord->type; e=MslLTemporary(l,type,&s->loc); if(!e) break;
            store=MslLStore(l,MslLCopyExpr(l,e),coord);
            uv->kind=MSL_SWIZZLE; uv->type=type;
            uv->type.lanes=s->u.call.callee->details.fun.index==MSL_BUILTIN_TEX2DLOD?2:3;
            uv->text=uv->type.lanes==2?"xy":"xyz"; uv->a=MslLCopyExpr(l,e);
            lod->kind=MSL_SWIZZLE; lod->type=type; lod->type.lanes=1; lod->text="w"; lod->a=e;
            sample->b=uv; sample->c=lod;
            seq->kind=MSL_SEQUENCE; seq->type=sample->type; seq->loc=s->loc;
            seq->a=store; if(store) store->next=sample; e=seq; break;
        }
        e->kind = MSL_CALL; e->function = MslLFindFunction(l, s->u.call.callee);
        {
            const CgIRExpr *arg=s->u.call.arguments;
            MslDecl *formal=e->function?e->function->parameters:NULL;
            MslExpr **end=&e->a;
            for (;arg && !l->m->failed;arg=arg->next) {
                if(formal && formal->direction && MslReadOnlyTarget(l,arg)) {
                    MslFail(l->m,6608,&arg->loc,"out arguments cannot write uniform/global storage");
                    break;
                }
                *end=LowerExprMode(l,arg,formal && formal->direction);
                if (!*end) break;
                end=&(*end)->next;
                if (formal) formal=formal->next;
            }
        }
        if (!e->function) MslFail(l->m, 6602, &s->loc, "unresolved Metal helper");
        else if(!l->m->failed) e=MslLCopyCall(l,e);
        break;
    case CGIR_EXPR_INTRINSIC:
        if(s->u.intrinsicCall.intrinsic==CG_INTRINSIC_TEX2D || s->u.intrinsicCall.intrinsic==CG_INTRINSIC_TEXCUBE) {
            if(!s->u.intrinsicCall.arguments || !s->u.intrinsicCall.arguments->next || s->u.intrinsicCall.arguments->next->next) {
                MslFail(l->m,6602,&s->loc,"gradient texture signatures are unsupported"); break;
            }
            if(l->m->stage!=MSL_FRAGMENT) { MslFail(l->m,6602,&s->loc,"implicit texture sampling requires fragment stage"); break; }
            e->kind=MSL_SAMPLE; e->a=MslLLowerExpr(l,s->u.intrinsicCall.arguments);
            e->b=MslLLowerExpr(l,s->u.intrinsicCall.arguments->next);
            if(!e->a || e->a->kind!=MSL_SYMBOL || e->a->type.base<MSL_TEXTURE2D)
                MslFail(l->m,6602,&s->loc,"sampling requires a resolved texture/sampler pair");
            break;
        }
        if(s->u.intrinsicCall.intrinsic==CG_INTRINSIC_MUL) {
            e->a=MslLLowerExpr(l,s->u.intrinsicCall.arguments);
            e->b=MslLLowerExpr(l,s->u.intrinsicCall.arguments->next);
            if(e->a && e->b && e->a->type.base!=MSL_MATRIX && e->b->type.base!=MSL_MATRIX && e->a->type.lanes>1 && e->b->type.lanes>1) {
                e->kind=MSL_CALL; e->text="dot"; e->a->next=e->b; e->b=NULL;
            } else { e->kind=MSL_BINARY; e->text="*"; e->postfix=1; }
            break;
        }
        if(s->u.intrinsicCall.intrinsic==CG_INTRINSIC_TRANSPOSE) {
            e->kind=MSL_CALL; e->text="transpose"; e->a=MslLArgs(l,s->u.intrinsicCall.arguments); break;
        }
        e->kind = MSL_CALL; e->text = MslLIntrinsic(l, s);
        e->a = MslLArgs(l, s->u.intrinsicCall.arguments); break;
    default: MslFail(l->m, 6602, &s->loc, "unsupported Metal expression"); break;
    }
    if(e && e->type.base>=MSL_TEXTURE2D && e->kind!=MSL_SYMBOL)
        MslFail(l->m,6602,&s->loc,"resource values require a resolved input symbol");
    return e;
}
