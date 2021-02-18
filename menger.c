/*
--------------------------------------------------
    James William Fletcher (james@voxdsp.com)
        December 2020 - menger.c
--------------------------------------------------
    Emscripten / C & SDL / OpenGL ES2 / GLSL ES

    Example Project for exAux.h

    https://github.com/esaux

    Colour Converter: https://www.easyrgb.com
*/

#include <emscripten.h>
#include <emscripten/html5.h>
#include <time.h>

#include <SDL.h>
#include <SDL_opengles2.h>

#define REGULAR_PHONG
#include "esAux.h"
#include "ncube.h"


//*************************************
// globals
//*************************************
char appTitle[] = "MengerCube";
SDL_Window* wnd;
SDL_GLContext glc;
SDL_Renderer* rdr;
Uint32 winw = 0, winh = 0;
GLfloat aspect;
GLfloat sens = 0.3;
GLfloat sens_mul = 1.0;

GLint projection_id;
GLint modelview_id;
GLint normalmat_id = -1;
GLint position_id;
GLint lightpos_id;
GLint normal_id;
GLint solidcolor_id;
GLint color_id;
GLint opacity_id;
GLint normal_id;
GLint type_id;

GLuint shaderProgram;

ESMatrix projection;
ESMatrix view;
ESMatrix modelview;
ESMatrix normalmat;

GLfloat xrot = 0;
GLfloat yrot = 0;
GLfloat zoom = -6.0;

ESModel mdlCube;


//*************************************
// render functions
//*************************************
void rCube()
{
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (GLfloat*) &projection.m[0][0]);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (GLfloat*) &view.m[0][0]);

    if(normalmat_id != -1)
    {
        ESMatrix inverted;
        esMatrixLoadIdentity(&inverted);
        esMatrixLoadIdentity(&normalmat);
        esCramerInvert(&modelview.m[0][0], &inverted.m[0][0]);
        esTranspose(&normalmat, &inverted);
        glUniformMatrix4fv(normalmat_id, 1, GL_FALSE, (GLfloat*) &normalmat.m[0][0]);
    }
    
    glDrawElements(GL_TRIANGLES, ncube_numind, GL_UNSIGNED_INT, 0);
}

//*************************************
// update & render
//*************************************
void main_loop()
{
//*************************************
// time delta for interpolation
//*************************************
    static Uint32 lt = 0;
    const Uint32 t = SDL_GetTicks();
    GLfloat deltaTime = (t-lt) * 0.02; // 0.02 = 1 / (1000/20)
    if(deltaTime > 1.0)
        deltaTime = 1.0;
    lt = t;

    //animate the light
    if(zoom == -26.0)
        glUniform3f(lightpos_id, sin(t) * esRandFloat(0, 1), cos(t) * esRandFloat(0, 1), sin(t) * esRandFloat(0, 1)); // candle light flicker
    else if(zoom != 0.0)
        glUniform3f(lightpos_id, sin(t * 0.001) * 10.0, cos(t * 0.001) * 10.0, sin(t * 0.001) * 10.0); // oscillatory motion

//*************************************
// input handling
//*************************************
    static int x=0, y=0, lx=0, ly=0, md=0;
    static int bs = 0;
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_KEYDOWN:
            {
                if(event.key.keysym.sym == SDLK_z)
                {
                    glUniform3f(color_id, esRandFloat(0, 1), esRandFloat(0, 1), esRandFloat(0, 1));
                    glUniform1f(opacity_id, 1.0);
                    glDisable(GL_BLEND);
                }
                if(event.key.keysym.sym == SDLK_x)
                {
                    glUniform3f(color_id, esRandFloat(0, 1), esRandFloat(0, 1), esRandFloat(0, 1));
                    glUniform1f(opacity_id, 0.5);
                    glEnable(GL_BLEND);
                }

                if(event.key.keysym.sym == SDLK_a)
                {
                    shadeLambert1(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
                    glUniform3f(color_id, esRandFloat(0, 1), esRandFloat(0, 1), esRandFloat(0, 1));

                    glBindBuffer(GL_ARRAY_BUFFER, mdlCube.vid);
                    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
                    glEnableVertexAttribArray(position_id);

                    glBindBuffer(GL_ARRAY_BUFFER, mdlCube.nid);
                    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
                    glEnableVertexAttribArray(normal_id);

                    normalmat_id = -1;
                }
                if(event.key.keysym.sym == SDLK_s)
                {
                    shadePhong1(&position_id, &projection_id, &modelview_id, &normalmat_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
                    glUniform3f(color_id, esRandFloat(0, 1), esRandFloat(0, 1), esRandFloat(0, 1));

                    glBindBuffer(GL_ARRAY_BUFFER, mdlCube.vid);
                    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
                    glEnableVertexAttribArray(position_id);

                    glBindBuffer(GL_ARRAY_BUFFER, mdlCube.nid);
                    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
                    glEnableVertexAttribArray(normal_id);
                }
            }
            break;

            case SDL_MULTIGESTURE:
            {
                static int lmgt = 0;
                if(SDL_GetTicks() > lmgt)
                {
                    if(zoom == 0.0)
                    {
                        zoom = -6.0;
                        sens = 0.1*sens_mul;
                        glUniform3f(color_id, 1.0, 1.0, 1.0);
                        glUniform3f(lightpos_id, 0, 0, 0);
                    }
                    else if(zoom == -6.0)
                    {
                        zoom = -10.0;
                        sens = 0.1*sens_mul;
                        glUniform3f(color_id, 1.0, 1.0, 0.0);
                    }
                    else if(zoom == -10.0)
                    {
                        zoom = -16.0;
                        sens = 0.3*sens_mul;
                        glUniform3f(color_id, 1.0, 0.0, 1.0);
                    }
                    else if(zoom == -16.0)
                    {
                        zoom = -26.0;
                        sens = 0.2*sens_mul;
                        glUniform3f(color_id, 0.0, 1.0, 1.0);
                    }
                    else if(zoom == -26.0)
                    {
                        zoom = 0.0;
                        sens = 0.2*sens_mul;
                        glUniform3f(color_id, 1.0, 1.0, 1.0);
                        glUniform3f(lightpos_id, 0, 0, -4.0);

                        if(bs == 0)
                        {
                            glUniform1f(opacity_id, 0.5);
                            glEnable(GL_BLEND);
                            bs = 1;
                        }
                        else
                        {
                            glDisable(GL_BLEND);
                            bs = 0;
                        }
                    }
                    lmgt = SDL_GetTicks() + 333;
                }
            }
            break;

            case SDL_FINGERDOWN:
            {
                lx = event.tfinger.x * winw;
                ly = event.tfinger.y * winh;
                x = event.tfinger.x * winw;
                y = event.tfinger.y * winh;
                md = 1;
            }
            break;

            case SDL_FINGERUP:
            {
                md = 0;
            }
            break;

            case SDL_MOUSEBUTTONDOWN:
            {
                lx = event.button.x;
                ly = event.button.y;
                x = event.button.x;
                y = event.button.y;

                if(event.button.button == SDL_BUTTON_LEFT)
                    md = 1;
            }
            break;

            case SDL_MOUSEMOTION:
            {
                if(md == 1)
                {
                    x = event.motion.x;
                    y = event.motion.y;
                }
            }
            break;

            case SDL_MOUSEBUTTONUP:
            {
                if(event.button.button == SDL_BUTTON_LEFT)
                    md = 0;
                if(event.button.button == SDL_BUTTON_RIGHT)
                {
                    if(zoom == 0.0)
                    {
                        zoom = -6.0;
                        sens = 0.1*sens_mul;
                        glUniform3f(color_id, 1.0, 1.0, 1.0);
                        glUniform3f(lightpos_id, 0, 0, 0);
                    }
                    else if(zoom == -6.0)
                    {
                        zoom = -10.0;
                        sens = 0.1*sens_mul;
                        glUniform3f(color_id, 1.0, 1.0, 0.0);
                    }
                    else if(zoom == -10.0)
                    {
                        zoom = -16.0;
                        sens = 0.3*sens_mul;
                        glUniform3f(color_id, 1.0, 0.0, 1.0);
                    }
                    else if(zoom == -16.0)
                    {
                        zoom = -26.0;
                        sens = 0.2*sens_mul;
                        glUniform3f(color_id, 0.0, 1.0, 1.0);
                    }
                    else if(zoom == -26.0)
                    {
                        zoom = 0.0;
                        sens = 0.2*sens_mul;
                        glUniform3f(color_id, 1.0, 1.0, 1.0);
                        glUniform3f(lightpos_id, 0, 0, -4.0);

                        if(bs == 0)
                        {
                            glUniform1f(opacity_id, 0.5);
                            glEnable(GL_BLEND);
                            bs = 1;
                        }
                        else
                        {
                            glDisable(GL_BLEND);
                            bs = 0;
                        }
                    }
                }
            }
            break;
        }
    }

//*************************************
// delta orbit / mouse control
//*************************************
    GLfloat xd = (lx-x);
    GLfloat yd = (ly-y);
    lx = x;
    ly = y;

    xrot += xd*sens;
    yrot += yd*sens;

    // prevent pole inversion
    if(yrot > 180.0)
        yrot = 180.0;
    if(yrot < -0.0)
        yrot = -0.0;

//*************************************
// camera control
//*************************************
    esMatrixLoadIdentity(&view);

    esTranslate(&view, 0, 0, zoom);

    esRotate(&view, -yrot, 1, 0, 0);
    esRotate(&view, xrot, 0, 0, 1);

//*************************************
// begin render
//*************************************
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//*************************************
// render main
//*************************************
    rCube();

//*************************************
// swap buffers / display render
//*************************************
    SDL_GL_SwapWindow(wnd);
}

//*************************************
// Process Entry Point
//*************************************
int main(int argc, char** argv)
{
//*************************************
// setup render context / window
//*************************************
    double width, height;
    emscripten_get_element_css_size("body", &width, &height);
    winw = (Uint32)width, winh = (Uint32)height;
    wnd = SDL_CreateWindow(appTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winw, winh, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetSwapInterval(0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    glc = SDL_GL_CreateContext(wnd);
    rdr = SDL_CreateRenderer(wnd, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

//*************************************
// projection
//*************************************

    aspect = (GLfloat) winw / (GLfloat) winh;

    if(aspect < 1.0)
        zoom = -26.0;

    esMatrixLoadIdentity(&projection);
    esPerspective(&projection, 60.0f, aspect, 1.0f, 60.0f);

//*************************************
// compile & link shader program
//*************************************

    makeAllShaders();
    //makePhong1();
    //makeLambert1();

//*************************************
// configure render options
//*************************************

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //shadeLambert1(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
    shadePhong1(&position_id, &projection_id, &modelview_id, &normalmat_id, &lightpos_id, &normal_id, &color_id, &opacity_id);

    glUniform3f(color_id, 1.0, 1.0, 1.0);
    glUniform3f(lightpos_id, 0, 0, 0);
    glUniform1f(opacity_id, 1.0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

//*************************************
// bind vertex and index buffers
//*************************************

    esBind(GL_ARRAY_BUFFER, &mdlCube.vid, ncube_vertices, sizeof(ncube_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlCube.nid, ncube_normals, sizeof(ncube_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlCube.iid, ncube_indices, sizeof(ncube_indices), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, mdlCube.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlCube.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlCube.iid);

//*************************************
// execute update / render loop
//*************************************

    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;

}

