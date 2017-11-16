#include<stdio.h>
#include<time.h>
#include<stdlib.h>
#include<X11/X.h>
#include<X11/Xlib.h>
#define GL_GLEXT_PROTOTYPES
#include<GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <algorithm>
#include <math.h>
#include "shader_utilities.h"
#include "oglplot.h"

using namespace std;

Display                 *dpy;
Window                  root;
GLint                   att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
XVisualInfo             *vi;
Colormap                cmap;
XSetWindowAttributes    swa;
Window                  win;
GLXContext              glc;
XWindowAttributes       gwa;
XEvent                  xev;
 
int main(int argc, char *argv[]) {

    srand(time(NULL));

    dpy = XOpenDisplay(NULL);

    if(dpy == NULL) {
        printf("\n\tcannot connect to X server\n\n");
        return 0;
    }
        
    root = DefaultRootWindow(dpy);

    vi = glXChooseVisual(dpy, 0, att);

    if(vi == NULL) {
        printf("\n\tno appropriate visual found\n\n");
        return 0;
    }


    cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask;

    win = XCreateWindow(dpy, root, 0, 0, 600, 600, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

    XMapWindow(dpy, win);
    XStoreName(dpy, win, "OpenGL Plot");

    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);

    Plot plot;
    
    SeriesData data;
    for(int i=0; i<7; i++) data.push_back({(float)i,sin((float)i)});
    plot.addSeries(data);
    SeriesData data2;
    for(int i=0; i<7; i++) data2.push_back({(float)i,exp(-(float)i)});
    plot.addSeries(data2);
    plot.limits(0.0f,6.0f,-1.0f,2.0f);
    float scale = 10.0;
    plot.xticks(0.5f,0.1f);
    plot.yticks(0.5f,0.1f);

    while(1) {
        XNextEvent(dpy, &xev);

        if(xev.type == Expose) {
                XGetWindowAttributes(dpy, win, &gwa);
                glViewport(0, 0, gwa.width, gwa.height);
                plot.draw();
                glXSwapBuffers(dpy, win);
        }
                
        else if(xev.type == KeyPress) {
            if(xev.xkey.keycode == 0x09){
                glXMakeCurrent(dpy, None, NULL);
                glXDestroyContext(dpy, glc);
                XDestroyWindow(dpy, win);
                XCloseDisplay(dpy);
                exit(0);
            }
            else if(xev.xkey.keycode == 111){
                scale++;
                plot.limits(-scale,scale,-scale,scale);
                plot.draw();
                glXSwapBuffers(dpy, win);
            }
            else if(xev.xkey.keycode == 116){
                scale--;
                plot.limits(-scale,scale,-scale,scale);
                plot.draw();
                glXSwapBuffers(dpy, win);
            }
            else if(xev.xkey.keycode == 65){
                plot.limits();
                plot.draw();
                glXSwapBuffers(dpy, win);
            }
        }
    }
}
