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

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
/* Real offscreen rendering. Expected values are independent CPU constants. */
int main(int argc, const char **argv)
{
    @autoreleasepool {
        if (argc != 7 && argc != 8) { fprintf(stderr,"vertex.metallib fragment.metallib vertex-entry fragment-entry case strict [r,g,b,a]\n"); return 1; }
        BOOL lodCase = !strncmp(argv[5],"lod",3) || !strcmp(argv[5],"cube_lod");
        BOOL cubeCase = !strncmp(argv[5],"cube",4);
        int cubeFace = cubeCase ? atoi(argv[5]+4) : 0;
        if(cubeCase && (cubeFace<0 || cubeFace>5)) return 1;
        const unsigned char faceColors[6][4]={{255,0,0,255},{0,255,0,255},{0,0,255,255},{255,255,0,255},{255,0,255,255},{0,255,255,255}};
        const float directions[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) { fprintf(stderr,"No Metal device\n"); return atoi(argv[6]) ? 1 : 77; }
        NSError *error = nil;
        id<MTLLibrary> vl = [device newLibraryWithURL:[NSURL fileURLWithPath:@(argv[1])] error:&error];
        if (!vl) { fprintf(stderr,"Vertex library: %s\n",error.description.UTF8String); return 1; }
        id<MTLLibrary> fl = [device newLibraryWithURL:[NSURL fileURLWithPath:@(argv[2])] error:&error];
        if (!fl) { fprintf(stderr,"Fragment library: %s\n",error.description.UTF8String); return 1; }
        id<MTLFunction> vf = [vl newFunctionWithName:@(argv[3])];
        id<MTLFunction> ff = [fl newFunctionWithName:@(argv[4])];
        if (!vf || !ff) { fprintf(stderr,"Missing exported entry\n"); return 1; }
        MTLRenderPipelineDescriptor *pd = [MTLRenderPipelineDescriptor new];
        pd.vertexFunction = vf; pd.fragmentFunction = ff;
        pd.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA32Float;
        BOOL depthCase = !strcmp(argv[5],"depth");
        if(depthCase) pd.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        MTLVertexDescriptor *vd = [MTLVertexDescriptor vertexDescriptor];
        vd.attributes[0].format = MTLVertexFormatFloat4;
        vd.attributes[0].offset = 0; vd.attributes[0].bufferIndex = 1;
        vd.layouts[1].stride = 4 * sizeof(float);
        BOOL pipelineOnly = !strncmp(argv[5],"pipeline_",9);
        if(pipelineOnly) {
            vd.attributes[1].format=!strcmp(argv[5],"pipeline_position")?MTLVertexFormatFloat4:MTLVertexFormatFloat3;
            vd.attributes[1].offset=16; vd.attributes[1].bufferIndex=1;
            vd.attributes[2].format=MTLVertexFormatFloat4; vd.attributes[2].offset=32; vd.attributes[2].bufferIndex=1;
            vd.layouts[1].stride=48;
        }
        pd.vertexDescriptor = vd;
        id<MTLRenderPipelineState> pipeline = [device newRenderPipelineStateWithDescriptor:pd error:&error];
        if (!strcmp(argv[5],"mismatch")) {
            if(pipeline || !error) { fprintf(stderr,"Mismatched varyings unexpectedly linked\n"); return 1; }
            printf("PASS mismatched varying pipeline rejected: %s\n",error.description.UTF8String); return 0;
        }
        if (!pipeline) { fprintf(stderr,"Pipeline: %s\n",error.description.UTF8String); return 1; }
        if(pipelineOnly) { printf("PASS %s pipeline on %s\n",argv[5],device.name.UTF8String); return 0; }
        const float vertices[] = {-1,-1,0,1, 3,-1,0,1, -1,3,0,1};
        id<MTLBuffer> vb = [device newBufferWithBytes:vertices length:sizeof(vertices) options:MTLResourceStorageModeShared];
        /* Name-sorted ABI: exposure at byte 0, tint at byte 16. Poison padding. */
        float uniforms[20] = {2,91,92,93, .25f,.5f,.75f,1};
        if(!strcmp(argv[5],"layout")) {
            const float layout[] = {.25f,.5f,.75f,91, 2,92,93,94,
                                    1,4,7,95, 2,5,8,96, 3,6,10,97};
            memcpy(uniforms,layout,sizeof(layout));
        }
        if(!strncmp(argv[5],"coverage_type_int",17) || !strncmp(argv[5],"coverage_type_uint",18)) {
            const unsigned int ints[4]={2,3,4,5}; memcpy(uniforms,ints,sizeof(ints));
        }
        if(cubeCase) memcpy(uniforms,directions[cubeFace],3*sizeof(float));
        id<MTLBuffer> ub = [device newBufferWithBytes:uniforms length:sizeof(uniforms) options:MTLResourceStorageModeShared];
        id<MTLTexture> sampled=nil; id<MTLSamplerState> sampler=nil;
        if(cubeCase || lodCase || !strcmp(argv[5],"texture")) {
            MTLTextureDescriptor *sd=[MTLTextureDescriptor new];
            sd.textureType=cubeCase?MTLTextureTypeCube:MTLTextureType2D;
            sd.pixelFormat=MTLPixelFormatRGBA8Unorm; sd.width=cubeCase && !lodCase?1:2; sd.height=sd.width;
            sd.mipmapLevelCount=lodCase?2:1;
            sd.storageMode=device.hasUnifiedMemory?MTLStorageModeShared:MTLStorageModeManaged;
            sd.usage=MTLTextureUsageShaderRead;
            sampled=[device newTextureWithDescriptor:sd]; if(!sampled) return 1;
            if(cubeCase) {
                for(int face=0;face<6;face++) {
                    unsigned char base[16]; for(int pixel=0;pixel<4;pixel++) memcpy(base+pixel*4,faceColors[face],4);
                    [sampled replaceRegion:MTLRegionMake2D(0,0,sd.width,sd.height) mipmapLevel:0 slice:face withBytes:base bytesPerRow:sd.width*4 bytesPerImage:sd.width*sd.height*4];
                    if(lodCase) { const unsigned char white[]={255,255,255,255};
                        [sampled replaceRegion:MTLRegionMake2D(0,0,1,1) mipmapLevel:1 slice:face withBytes:white bytesPerRow:4 bytesPerImage:4]; }
                }
            } else {
                const unsigned char pixels[]={255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255};
                [sampled replaceRegion:MTLRegionMake2D(0,0,2,2) mipmapLevel:0 withBytes:pixels bytesPerRow:8];
                if(lodCase) { const unsigned char blue[]={0,0,255,255};
                    [sampled replaceRegion:MTLRegionMake2D(0,0,1,1) mipmapLevel:1 withBytes:blue bytesPerRow:4]; }
            }
            MTLSamplerDescriptor *samplerDesc=[MTLSamplerDescriptor new];
            samplerDesc.minFilter=MTLSamplerMinMagFilterNearest; samplerDesc.magFilter=MTLSamplerMinMagFilterNearest;
            samplerDesc.mipFilter=MTLSamplerMipFilterNearest;
            samplerDesc.sAddressMode=MTLSamplerAddressModeClampToEdge; samplerDesc.tAddressMode=MTLSamplerAddressModeClampToEdge;
            sampler=[device newSamplerStateWithDescriptor:samplerDesc]; if(!sampler) return 1;
        }
        MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float width:8 height:8 mipmapped:NO];
        td.storageMode = MTLStorageModePrivate; td.usage = MTLTextureUsageRenderTarget;
        id<MTLTexture> target = [device newTextureWithDescriptor:td];
        id<MTLBuffer> readback = [device newBufferWithLength:256*16 options:MTLResourceStorageModeShared];
        id<MTLTexture> depthTexture=nil;
        id<MTLDepthStencilState> depthState=nil;
        if(depthCase) {
            MTLTextureDescriptor *dd=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:8 height:8 mipmapped:NO];
            dd.storageMode=MTLStorageModePrivate; dd.usage=MTLTextureUsageRenderTarget;
            depthTexture=[device newTextureWithDescriptor:dd];
            MTLDepthStencilDescriptor *ds=[MTLDepthStencilDescriptor new];
            ds.depthCompareFunction=MTLCompareFunctionAlways; ds.depthWriteEnabled=YES;
            depthState=[device newDepthStencilStateWithDescriptor:ds];
            if(!depthTexture || !depthState) return 1;
        }
        id<MTLCommandQueue> queue = [device newCommandQueue];
        id<MTLCommandBuffer> command = [queue commandBuffer];
        if (!vb || !ub || !target || !readback || !queue || !command) { fprintf(stderr,"Metal resource allocation failed\n"); return 1; }
        MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = target;
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass.colorAttachments[0].clearColor = MTLClearColorMake(.125,.25,.5,1);
        if(depthCase) {
            pass.depthAttachment.texture=depthTexture;
            pass.depthAttachment.loadAction=MTLLoadActionClear;
            pass.depthAttachment.storeAction=MTLStoreActionStore;
            pass.depthAttachment.clearDepth=1;
        }
        id<MTLRenderCommandEncoder> render = [command renderCommandEncoderWithDescriptor:pass];
        if (!render) return 1;
        [render setRenderPipelineState:pipeline];
        if(depthCase) [render setDepthStencilState:depthState];
        [render setVertexBuffer:vb offset:0 atIndex:1];
        [render setVertexBuffer:ub offset:0 atIndex:0];
        [render setFragmentBuffer:ub offset:0 atIndex:0];
        if(sampled) { [render setFragmentTexture:sampled atIndex:3]; [render setFragmentSamplerState:sampler atIndex:3];
            [render setVertexTexture:sampled atIndex:3]; [render setVertexSamplerState:sampler atIndex:3]; }
        [render drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [render endEncoding];
        id<MTLBlitCommandEncoder> blit = [command blitCommandEncoder];
        if (!blit) return 1;
        [blit copyFromTexture:target sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(8,8,1)
                    toBuffer:readback destinationOffset:0 destinationBytesPerRow:256 destinationBytesPerImage:256*8];
        if(depthCase) [blit copyFromTexture:depthTexture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(8,8,1)
                    toBuffer:readback destinationOffset:256*8 destinationBytesPerRow:256 destinationBytesPerImage:256*8];
        [blit endEncoding]; [command commit]; [command waitUntilCompleted];
        if (command.status != MTLCommandBufferStatusCompleted || command.error) {
            fprintf(stderr,"GPU execution: %s\n",command.error.description.UTF8String); return 1;
        }
        float expected[4] = {1,0,0,1};
        if (argc == 8) {
            char end;
            if (sscanf(argv[7],"%f,%f,%f,%f%c",&expected[0],&expected[1],&expected[2],&expected[3],&end) != 4) {
                fprintf(stderr,"Invalid independent expected pixels\n"); return 1;
            }
            for(int c=0;c<4;c++) if(!isfinite(expected[c])) return 1;
        }
        else if (!strcmp(argv[5],"arithmetic")) { expected[0]=5; expected[1]=3; expected[2]=0; expected[3]=1; }
        else if (!strcmp(argv[5],"uniform")) { expected[0]=.5f; expected[1]=1; expected[2]=1.5f; expected[3]=2; }
        else if (!strcmp(argv[5],"discard")) { expected[0]=.125f; expected[1]=.25f; expected[2]=.5f; expected[3]=1; }
        else if (!strcmp(argv[5],"matrix")) { expected[0]=17; expected[1]=39; expected[2]=23; expected[3]=34; }
        else if (!strcmp(argv[5],"layout")) { expected[0]=7; expected[1]=16; expected[2]=26.5f; expected[3]=1; }
        else if (!strcmp(argv[5],"varying")) { expected[0]=.25f; expected[1]=.5f; expected[2]=.75f; expected[3]=7; }
        else if (!strcmp(argv[5],"array")) { expected[0]=2; expected[1]=91; expected[2]=.25f; expected[3]=.5f; }
        else if (!strcmp(argv[5],"half")) { expected[0]=2; expected[1]=91; expected[2]=0; expected[3]=1; }
        else if (!strcmp(argv[5],"default")) { expected[0]=2; expected[1]=0; expected[2]=0; expected[3]=1; }
        else if (!strcmp(argv[5],"row_write")) { expected[0]=9; expected[1]=8; expected[2]=3; expected[3]=4; }
        else if (!strcmp(argv[5],"global")) { expected[0]=6; expected[1]=0; expected[2]=0; expected[3]=1; }
        else if (!strcmp(argv[5],"swap")) { expected[0]=2; expected[1]=1; expected[2]=3; expected[3]=1; }
        else if (!strcmp(argv[5],"texture")) { expected[0]=0; expected[1]=1; expected[2]=0; expected[3]=1; }
        else if (lodCase) { expected[0]=cubeCase?1:0; expected[1]=cubeCase?1:0; expected[2]=1; expected[3]=1; }
        else if (cubeCase) { for(int c=0;c<4;c++) expected[c]=faceColors[cubeFace][c]/255.0f; }
        else if (strcmp(argv[5],"solid") && !depthCase) { fprintf(stderr,"Unknown witness\n"); return 1; }
        /* Interior pixels avoid raster edge conventions. */
        for (int y=2;y<6;y++) for (int x=2;x<6;x++) {
            if(depthCase) {
                float depth=*(const float *)((const char *)readback.contents+256*8+y*256+x*4);
                if(!isfinite(depth) || fabsf(depth-.25f)>1e-5f) { fprintf(stderr,"Depth got %g expected .25\n",depth); return 1; }
            }
            const float *pixel = (const float *)((const char *)readback.contents + y*256 + x*16);
            for(int c=0;c<4;c++) if (!isfinite(pixel[c]) || fabsf(pixel[c]-expected[c]) > 1e-5f*(1+fabsf(expected[c]))) {
                fprintf(stderr,"%s pixel %d,%d lane %d: got %.9g expected %.9g\n",argv[5],x,y,c,pixel[c],expected[c]); return 1;
            }
        }
        printf("PASS %s on %s: 16 RGBA32Float pixels match (%.6g, %.6g, %.6g, %.6g)\n",argv[5],device.name.UTF8String,expected[0],expected[1],expected[2],expected[3]);
        return 0;
    }
}
