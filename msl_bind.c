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

const char *MslLSemantic(Lower *l, int atom, const SourceLoc *loc)
{
    const char *src = GetAtomString(atable, atom); char buf[128]; size_t i, n;
    if (!atom || !src) { MslFail(l->m, 6603, loc, "missing Metal semantic"); return ""; }
    n = strlen(src);
    if (n > sizeof(buf) - 3) { MslFail(l->m, 6603, loc, "Metal semantic too long"); return ""; }
    for (i = 0; i < n; ++i) buf[i] = (char)toupper((unsigned char)src[i]);
    buf[n] = 0;
    if (!strcmp(buf, "HPOS")) strcpy(buf, "POSITION0");
    else if (!strcmp(buf, "DIFFUSE")) strcpy(buf, "COLOR0");
    else if (!strcmp(buf, "SPECULAR")) strcpy(buf, "COLOR1");
    else if (!strcmp(buf, "FOGCOORD")) strcpy(buf, "FOG0");
    else if (n && !isdigit((unsigned char)buf[n-1])) strcat(buf, "0");
    { char normalized[128]; size_t split=strlen(buf); unsigned index=0;
      while(split && isdigit((unsigned char)buf[split-1])) --split;
      for(i=split;buf[i];i++) {
        if(index>100000) { MslFail(l->m,6603,loc,"semantic index overflow"); return ""; }
        index=index*10+(buf[i]-'0');
      }
      snprintf(normalized,sizeof(normalized),"%.*s%u",(int)split,buf,index);
      return MslString(l->m,normalized);
    }
}

int MslLInterface(Lower *l, MslInterface **list, MslType type,
                     const char *path, const char *semantic, const SourceLoc *loc, int output)
{
    MslDecl *d; MslInterface *v,*prior; char child[2048]; int index=-1,i;
    if(type.base==MSL_RECORD) {
        for(d=type.record->members;d;d=d->next) {
            if(strlen(path)+strlen(d->name)+2>=sizeof(child))
                return MslFail(l->m,6603,loc,"Metal interface nesting limit");
            sprintf(child,"%s.%s",path,d->name);
            if(!MslLInterface(l,list,d->type,child,d->semantic,&d->loc,output)) return 0;
        }
        return 1;
    }
    if(type.base==MSL_MATRIX || type.base==MSL_ARRAY || type.base==MSL_VOID || !semantic)
        return MslFail(l->m,6603,loc,"Metal interface requires scalar/vector leaves with semantics");
    for(prior=*list;prior;prior=prior->next)
        if(!strcmp(prior->semantic,semantic)) return MslFail(l->m,6603,loc,"duplicate Metal interface semantic");
    v=(MslInterface *)MslAlloc(l->m,sizeof(*v)); if(!v) return 0;
    v->type=type; v->path=MslString(l->m,path); v->semantic=semantic; v->attribute=-1;
    if(l->m->stage==MSL_VERTEX && !output) {
        for(i=0;i<16;i++) { char a[16]; sprintf(a,"ATTRIB%d",i); if(!strcmp(a,semantic)) index=i; }
        if(type.base==MSL_BOOL) return MslFail(l->m,6603,loc,"boolean vertex attributes are unsupported");
        if(index<0) {
            const char *named[]={"POSITION0","NORMAL0","COLOR0","COLOR1","TANGENT0","BINORMAL0","BLENDWEIGHT0","BLENDINDICES0","TEXCOORD0","TEXCOORD1","TEXCOORD2","TEXCOORD3","TEXCOORD4","TEXCOORD5","TEXCOORD6","TEXCOORD7"};
            for(i=0;i<16;i++) if(!strcmp(named[i],semantic)) index=-2;
            if(index!=-2) return MslFail(l->m,6603,loc,"unsupported Metal vertex semantic");
        }
        v->attribute=index;
    } else if((output && l->m->stage==MSL_VERTEX && !strcmp(semantic,"POSITION0")) ||
              (output && l->m->stage==MSL_FRAGMENT && !strcmp(semantic,"COLOR0"))) {
        if(type.base!=MSL_FLOAT || type.lanes!=4) return MslFail(l->m,6603,loc,"position/color requires float4");
        v->builtin=1;
    } else if(output && l->m->stage==MSL_FRAGMENT && !strcmp(semantic,"DEPTH0")) {
        if(type.base!=MSL_FLOAT || type.lanes!=1) return MslFail(l->m,6603,loc,"depth requires float");
        v->builtin=2;
    } else {
        int valid=!strcmp(semantic,"COLOR0") || !strcmp(semantic,"COLOR1") || !strcmp(semantic,"FOG0");
        for(i=0;i<8;i++) { char a[16]; sprintf(a,"TEXCOORD%d",i); if(!strcmp(a,semantic)) valid=1; }
        if(!valid || (output && l->m->stage==MSL_FRAGMENT) || type.base==MSL_BOOL)
            return MslFail(l->m,6603,loc,"unsupported Metal varying semantic or type");
    }
    while(*list) list=&(*list)->next; *list=v; return !l->m->failed;
}
