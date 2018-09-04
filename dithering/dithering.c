// Dithering shader implementation inspired by (and completely reworked):
// http://www.anisopteragames.com/how-to-fix-color-banding-with-dithering/
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <GL/glew.h>
#include <GL/glut.h>

GLboolean animating=GL_TRUE, dithering=GL_FALSE;

GLuint ditherProgram=0;
GLuint ditherShader=0;
GLuint frameTexture=0,bayerMatrixTexture=0;
GLuint frameFramebuffer=0, depthRenderBuffer=0;

void initDitheringShader()
{
    ditherProgram=glCreateProgram();
    ditherShader=glCreateShader(GL_FRAGMENT_SHADER);
    const char* src=
        "uniform sampler2D frame, bayerMatrix;\n"
        "void main()\n"
        "{\n"
        "    vec4 color=texture2D(frame,gl_TexCoord[0].xy);\n"
        "    float bayer=texture2D(bayerMatrix,gl_FragCoord.xy/8.).r*(255./64.); // scaled to [0..1]\n"
#ifdef MONITOR_6_BPP // use this for 6 bit per subpixel monitors
        "    const float rgbByteMax=63.;\n"
#else
        "    const float rgbByteMax=255.;\n"
#endif
        "    vec4 rgba=rgbByteMax*color;\n"
        "    vec4 head=floor(rgba);\n"
        "    vec4 tail=rgba-head;\n"
        "    color=head+step(bayer,tail);\n"
        "    gl_FragColor=color/rgbByteMax;\n"
        "}\n"

        ;
    const GLint length=strlen(src);
    glShaderSource(ditherShader,1,&src,&length);
    glCompileShader(ditherShader);
    GLint status;
    glGetShaderiv(ditherShader,GL_COMPILE_STATUS,&status);
    if(!status)
    {
        fprintf(stderr,"Failed to compile shader\n");
        exit(2);
    }
    glAttachShader(ditherProgram,ditherShader);
    glLinkProgram(ditherProgram);
    glGetProgramiv(ditherProgram,GL_LINK_STATUS,&status);
    if(!status)
    {
        fprintf(stderr,"Failed to link shading program\n");
        exit(3);
    }
    static const char bayerPattern[] = {
        0,  32,  8, 40,  2, 34, 10, 42,  /* 8x8 Bayer ordered dithering  */
        48, 16, 56, 24, 50, 18, 58, 26,  /* pattern.  Each input pixel   */
        12, 44,  4, 36, 14, 46,  6, 38,  /* is scaled to the 0..63 range */
        60, 28, 52, 20, 62, 30, 54, 22,  /* before looking in this table */
        3,  35, 11, 43,  1, 33,  9, 41,  /* to determine the action.     */
        51, 19, 59, 27, 49, 17, 57, 25,
        15, 47,  7, 39, 13, 45,  5, 37,
        63, 31, 55, 23, 61, 29, 53, 21,};
    glGenTextures(1,&bayerMatrixTexture);
    glBindTexture(GL_TEXTURE_2D,bayerMatrixTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 8,8, 0,GL_LUMINANCE, GL_UNSIGNED_BYTE, bayerPattern);
}

void initLightAndMaterial()
{
    const GLfloat ambient[4]={0.5,0.5,0.5,1};
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ambient);
    const GLfloat emission[4]={0.2,0.2,0.2,1};
    glMaterialfv(GL_FRONT, GL_EMISSION, emission);
    const GLfloat diffuseColor[4]={1,1,1,1};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseColor);
    const GLfloat position[4]={2,0,-2,1};
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHT0);
    const GLfloat globalAmbient[4]={0,0,0,1};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,globalAmbient);
}

void checkRequirements()
{
    if(!GL_ARB_texture_float)
    {
        fputs("Float textures are not supported, this demo relies on them\n",stderr);
        exit(1);
    }
    if(!GL_ARB_framebuffer_object)
    {
        fputs("FBO is not supported, no good way to do postprocessing\n",stderr);
        exit(1);
    }
    /* We need OpenGL 2.0+ for GLSL and NPOT textures.
     * Extension interface fot GL_ARB_shader_objects is too
     * different from core so not trying to use it.
     */
    if(!GLEW_VERSION_2_0)
    {
        fprintf(stderr,"Need OpenGL>=2.0 for GLSL and NPOT textures\n");
        exit(1);
    }
}

GLboolean init()
{
    checkRequirements();
    initLightAndMaterial();
    initDitheringShader();
    return 1;
}

unsigned getTime()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_usec/1000+tv.tv_sec*1000;
}

void renderScene()
{
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    static unsigned oldTime;
    if(!oldTime) oldTime=getTime();
    const unsigned curTime=getTime();
    if(animating)
    {
        static double angle=0;
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        angle += (curTime-oldTime)%13500 * 360 / 13500.;
        glRotatef(angle,1,0,0);
        glRotatef(angle,0,1,0);
    }
    oldTime=curTime;

    typedef struct CubeVertex
    {
        GLfloat x,  y,  z;
        GLfloat nx, ny, nz;
        GLfloat u, v;
    } CubeVertex;
    static const CubeVertex vertices[] =
    {
    //   x  y  z  nx ny nz   u v
       { 1,-1,-1,  0, 0,-1,  0,1},
       { 1, 1,-1,  0, 0,-1,  0,0},
       {-1, 1,-1,  0, 0,-1,  1,0},
       {-1,-1,-1,  0, 0,-1,  1,1},

       {-1,-1,-1, -1, 0, 0,  0,1},
       {-1, 1,-1, -1, 0, 0,  0,0},
       {-1, 1, 1, -1, 0, 0,  1,0},
       {-1,-1, 1, -1, 0, 0,  1,1},

       {-1,-1, 1,  0, 0, 1,  0,1},
       {-1, 1, 1,  0, 0, 1,  0,0},
       { 1, 1, 1,  0, 0, 1,  1,0},
       { 1,-1, 1,  0, 0, 1,  1,1},

       { 1,-1, 1,  1, 0, 0,  0,1},
       { 1, 1, 1,  1, 0, 0,  0,0},
       { 1, 1,-1,  1, 0, 0,  1,0},
       { 1,-1,-1,  1, 0, 0,  1,1},

       { 1,-1,-1,  0,-1, 0,  0,1},
       {-1,-1,-1,  0,-1, 0,  0,0},
       {-1,-1, 1,  0,-1, 0,  1,0},
       { 1,-1, 1,  0,-1, 0,  1,1},

       { 1, 1, 1,  0, 1, 0,  0,1},
       {-1, 1, 1,  0, 1, 0,  0,0},
       {-1, 1,-1,  0, 1, 0,  1,0},
       { 1, 1,-1,  0, 1, 0,  1,1},
    };
    const GLushort indices[]=
    {
         0, 1, 2,   2, 3, 0,
         4, 5, 6,   6, 7, 4,
         8, 9,10,  10,11, 8,
        12,13,14,  14,15,12,
        16,17,18,  18,19,16,
        20,21,22,  22,23,20,
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glFrontFace(GL_CW);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glVertexPointer(3, GL_FLOAT, sizeof(CubeVertex), vertices);
    glNormalPointer(GL_FLOAT, sizeof(CubeVertex), (char*)vertices+3*sizeof(GLfloat));
    glDrawElements(GL_TRIANGLES, sizeof indices/sizeof*indices, GL_UNSIGNED_SHORT, indices);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void blitFBToScreen()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,frameTexture);
    if(dithering)
    {
        const GLint frameLoc=glGetUniformLocation(ditherProgram,"frame");
        if(frameLoc==-1)
            fprintf(stderr,"Failed to get location of frame uniform\n");
        glUseProgram(ditherProgram);
        glUniform1i(frameLoc,0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D,bayerMatrixTexture);
        const GLint bayerMatrixLoc=glGetUniformLocation(ditherProgram,"bayerMatrix");
        if(bayerMatrixLoc==-1)
            fprintf(stderr,"Failed to get location of Bayer matrix uniform\n");
        glUniform1i(bayerMatrixLoc,1);
    }
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
     glLoadIdentity();
     glMatrixMode(GL_MODELVIEW);
     glPushMatrix();
      glLoadIdentity();
      glOrtho(0,1,0,1,-1,1);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D,frameTexture);
      glEnable(GL_TEXTURE_2D);
      glBegin(GL_QUADS);
       glTexCoord2f(0,0);
       glVertex2f(0,0);
       glTexCoord2f(0,1);
       glVertex2f(0,1);
       glTexCoord2f(1,1);
       glVertex2f(1,1);
       glTexCoord2f(1,0);
       glVertex2f(1,0);
      glEnd();
      glDisable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D,0);
     glMatrixMode(GL_MODELVIEW);
     glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glUseProgram(0);
}

void display()
{
    static GLboolean inited;
    if(!inited) inited=init();

    glBindFramebuffer(GL_FRAMEBUFFER,frameFramebuffer);
    renderScene();
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    blitFBToScreen();

    glutSwapBuffers();
    glutPostRedisplay();
}

void reshape(int width, int height)
{
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspect=(float)width/height;
    gluPerspective(180./4, aspect, 1, 100);
    if(aspect<1)
    {
        const GLfloat fixup[4*4]=
        {
            aspect, 0, 0, 0,
            0, aspect, 0, 0,
            0,    0,   1, 0,
            0,    0,   0, 1,
        };
        glMultMatrixf(fixup);
    }
    gluLookAt(0, 0,-5,
              0, 0, 0,
              0, 1, 0);

    // reinitialize FBO
    if(!frameTexture)
        glGenTextures(1,&frameTexture);
    glBindTexture(GL_TEXTURE_2D,frameTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    if(!frameFramebuffer)
        glGenFramebuffers(1,&frameFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER,frameFramebuffer);
    if(!depthRenderBuffer)
        glGenRenderbuffers(1,&depthRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER,depthRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT32,width,height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depthRenderBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,frameTexture,0);
    GLenum status=glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status!=GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr,"Error: framebuffer is incomplete: status=%#x\n",status);
        exit(10);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glBindRenderbuffer(GL_RENDERBUFFER,0);
    glBindTexture(GL_TEXTURE_2D,0);
}

void keyboard(unsigned char key,int x,int y)
{
    char winTitle[1024];
    switch(key)
    {
    case ' ':
        animating=!animating;
        snprintf(winTitle,sizeof winTitle,"Animation %sabled",animating?"en":"dis");
        break;
    case 'd':
        dithering=!dithering;
        snprintf(winTitle,sizeof winTitle,"Dithering %sabled",dithering?"en":"dis");
        break;
    default:
        return;
    }
    glutSetWindowTitle(winTitle);
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize (1200, 900);
    glutCreateWindow ("Dithering test");
    if(glewInit()!=GLEW_OK)
    {
        fputs("Failed to init GLEW\n",stderr);
        exit(1);
    }
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    fputs("Press <SPACE> to toggle animation, 'd' to toggle dithering\n",stderr);
    glutMainLoop();
    return 0;
}