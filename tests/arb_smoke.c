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
// arb_smoke.c - Dependency-free Windows/WGL ARB program load smoke test.
//

#include <windows.h>
#include <GL/gl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GL_VERTEX_PROGRAM_ARB          0x8620
#define GL_FRAGMENT_PROGRAM_ARB        0x8804
#define GL_PROGRAM_FORMAT_ASCII_ARB    0x8875
#define GL_PROGRAM_ERROR_POSITION_ARB  0x864B
#define GL_PROGRAM_ERROR_STRING_ARB    0x8874

typedef void (APIENTRY *PFNGLGENPROGRAMSARBPROC)(GLsizei, GLuint *);
typedef void (APIENTRY *PFNGLBINDPROGRAMARBPROC)(GLenum, GLuint);
typedef void (APIENTRY *PFNGLPROGRAMSTRINGARBPROC)(GLenum, GLenum,
                                                   GLsizei, const void *);
typedef void (APIENTRY *PFNGLDELETEPROGRAMSARBPROC)(GLsizei, const GLuint *);

static PFNGLGENPROGRAMSARBPROC genPrograms;
static PFNGLBINDPROGRAMARBPROC bindProgram;
static PFNGLPROGRAMSTRINGARBPROC programString;
static PFNGLDELETEPROGRAMSARBPROC deletePrograms;

int main(int argc, char **argv)
{
    int stageVertex;
    FILE *file;
    char *source;
    long length;
    GLenum target;
    GLuint program = 0;
    HINSTANCE gl32 = GetModuleHandle("opengl32.dll");
    WNDCLASSA wc;
    HWND window = NULL;
    HDC dc = NULL;
    HGLRC context = NULL;
    PIXELFORMATDESCRIPTOR pfd;
    int pixelFormat;
    const char *extensions;
    GLenum error;
    GLint errorPosition;
    const char *errorString;
    int ii;

    if (argc != 3 ||
        (strcmp(argv[1], "vp") != 0 && strcmp(argv[1], "fp") != 0))
    {
        fprintf(stderr, "usage: arb_smoke vp|fp <program.arb>\n");
        return 2;
    }
    stageVertex = strcmp(argv[1], "vp") == 0;

    file = fopen(argv[2], "rb");
    if (!file) {
        fprintf(stderr, "cannot open %s\n", argv[2]);
        return 2;
    }
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (length <= 0 || length > INT_MAX) {
        fclose(file);
        fprintf(stderr, "bad program size\n");
        return 2;
    }
    source = (char *) malloc((size_t) length);
    if (!source || fread(source, 1, (size_t) length, file) !=
                   (size_t) length)
    {
        free(source);
        fclose(file);
        fprintf(stderr, "cannot read %s\n", argv[2]);
        return 2;
    }
    fclose(file);

    memset(&wc, 0, sizeof(wc));
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "arb_smoke";
    if (!RegisterClassA(&wc))
        goto skip;

    window = CreateWindowExA(0, "arb_smoke", "", WS_OVERLAPPEDWINDOW, 0, 0,
                             64, 64, NULL, NULL, wc.hInstance, NULL);
    if (!window)
        goto skip;
    dc = GetDC(window);
    if (!dc)
        goto skip;

    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
                  PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.iLayerType = PFD_MAIN_PLANE;
    pixelFormat = ChoosePixelFormat(dc, &pfd);
    if (!pixelFormat || !SetPixelFormat(dc, pixelFormat, &pfd))
        goto skip;

    context = wglCreateContext(dc);
    if (!context || !wglMakeCurrent(dc, context))
        goto skip;

    extensions = (const char *) glGetString(GL_EXTENSIONS);
    if (!extensions)
        goto skip;

    {
        // Whole-token extension search.
        int nameLen = stageVertex ? (int) strlen("GL_ARB_vertex_program")
                                  : (int) strlen("GL_ARB_fragment_program");
        const char *needle = stageVertex ? "GL_ARB_vertex_program"
                                         : "GL_ARB_fragment_program";
        int found = 0;
        const char *p = extensions;
        while (*p && !found) {
            while (*p == ' ')
                p++;
            if (strncmp(p, needle, (size_t) nameLen) == 0 &&
                (p[nameLen] == '\0' || p[nameLen] == ' '))
            {
                found = 1;
            }
            while (*p && *p != ' ')
                p++;
        }
        if (!found)
            goto skip;
    }

    genPrograms = (PFNGLGENPROGRAMSARBPROC)
        wglGetProcAddress("glGenProgramsARB");
    bindProgram = (PFNGLBINDPROGRAMARBPROC)
        wglGetProcAddress("glBindProgramARB");
    programString = (PFNGLPROGRAMSTRINGARBPROC)
        wglGetProcAddress("glProgramStringARB");
    deletePrograms = (PFNGLDELETEPROGRAMSARBPROC)
        wglGetProcAddress("glDeleteProgramsARB");
    if (!genPrograms || !bindProgram || !programString || !deletePrograms ||
        genPrograms == (PFNGLGENPROGRAMSARBPROC) 1 ||
        genPrograms == (PFNGLGENPROGRAMSARBPROC) 2 ||
        genPrograms == (PFNGLGENPROGRAMSARBPROC) 3 ||
        genPrograms == (PFNGLGENPROGRAMSARBPROC) (void *) -1)
    {
        goto skip;
    }

    while (glGetError() != GL_NO_ERROR)
        ;
    if (!stageVertex) {
        // Fragment TEX instructions validate against the texture bound
        // on the sampled image unit; supply a placeholder 2D texture.
        GLuint dummyTex = 0;
        glGenTextures(1, &dummyTex);
        glBindTexture(GL_TEXTURE_2D, dummyTex);
        glEnable(GL_TEXTURE_2D);
    }
    target = stageVertex ? GL_VERTEX_PROGRAM_ARB : GL_FRAGMENT_PROGRAM_ARB;
    genPrograms(1, &program);
    if (program == 0) {
        fprintf(stderr, "glGenProgramsARB returned zero name\n");
        goto cleanup_fail;
    }
    bindProgram(target, program);
    programString(target, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei) length,
                  source);
    error = glGetError();
    if (error != GL_NO_ERROR) {
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPosition);
        errorString = (const char *) glGetString(
            GL_PROGRAM_ERROR_STRING_ARB);
        printf("%s: GL error 0x%04x at position %d: %s\n",
               stageVertex ? "vp" : "fp", error, errorPosition,
               errorString ? errorString : "");
        deletePrograms(1, &program);
        free(source);
        if (context) wglMakeCurrent(NULL, NULL);
        if (context) wglDeleteContext(context);
        if (dc) ReleaseDC(window, dc);
        if (window) DestroyWindow(window);
        UnregisterClassA("arb_smoke", GetModuleHandle(NULL));
        return 1;
    }
    printf("loaded %s program\n", stageVertex ? "vertex" : "fragment");
    deletePrograms(1, &program);
    free(source);
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(context);
    ReleaseDC(window, dc);
    DestroyWindow(window);
    UnregisterClassA("arb_smoke", GetModuleHandle(NULL));
    return 0;

skip:
    // Environment cannot provide a driver smoke test; skip.
    if (program)
        deletePrograms(1, &program);
    free(source);
    if (context) wglMakeCurrent(NULL, NULL);
    if (context) wglDeleteContext(context);
    if (dc && window) ReleaseDC(window, dc);
    if (window) DestroyWindow(window);
    UnregisterClassA("arb_smoke", GetModuleHandle(NULL));
    printf("SKIP: no suitable WGL/ARB environment\n");
    return 77;

cleanup_fail:
    if (program)
        deletePrograms(1, &program);
    free(source);
    if (context) wglMakeCurrent(NULL, NULL);
    if (context) wglDeleteContext(context);
    if (dc && window) ReleaseDC(window, dc);
    if (window) DestroyWindow(window);
    UnregisterClassA("arb_smoke", GetModuleHandle(NULL));
    return 1;
}
