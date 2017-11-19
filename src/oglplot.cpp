#include "oglplot.h"
#include "shader_utilities.h"
#include <algorithm>
#include <cmath>
#include <stdio.h>

GLchar vertexShader[] = "#version 410\n"
                        "#extension GL_EXT_gpu_shader4 : enable\n"
                        "layout(location = 0) in vec2 position;\n"
                        "uniform mat4 view;"
                        "uniform vec4 clipPlane[4];"
                        "void main(){"
                        "gl_Position = view*vec4(position,0.0,1.0);"
                        "for(int i=0;i<4;i++)"
                        "gl_ClipDistance[i] = dot(clipPlane[i],gl_Position);"
                        "}";

GLchar fragmentShader[] = "#version 410\n"
                          "uniform vec4 color;\n" 
                          "out vec4 outputF;\n"
                          "void main(){outputF = color;}";

using namespace std;
using namespace glm;


/*
 * Globals
 */

Color defaultColors[3] = {vec4(1.0f,0.0f,0.0f,1.0f),
                          vec4(0.0f,1.0f,0.0f,1.0f),
                          vec4(0.0f,0.0f,1.0f,1.0f)};                            

/*
 * Constructors
 */
 
Plot::Series::Series(SeriesData data_in){

    data = data_in;
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Point2D)*data.size(), &(data[0]), GL_DYNAMIC_DRAW);

}

Plot::Series::~Series(){

    if(!vbo) glDeleteBuffers(1, &vbo);

}

Plot::Frame::Frame(){
    
    frame.resize(8);
    frame[0] = {-1.0f,-1.0f};
    frame[1] = { 1.0f,-1.0f};
    frame[2] = { 1.0f,-1.0f};
    frame[3] = { 1.0f, 1.0f};
    frame[4] = { 1.0f, 1.0f};
    frame[5] = {-1.0f, 1.0f};
    frame[6] = {-1.0f, 1.0f};
    frame[7] = {-1.0f,-1.0f};
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Point2D)*8, &(frame[0]), GL_STATIC_DRAW);
    
    color = vec4(1.0f,1.0f,1.0f,1.0f);
    
}

Plot::Frame::~Frame(){

    if(!vbo) glDeleteBuffers(1, &vbo);

}

Plot::Frame::Ticks::Ticks(){}

Plot::Frame::Ticks::Ticks(PointArray ticks){

    num = ticks.size();

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Point2D)*ticks.size(), &(ticks[0]), GL_DYNAMIC_DRAW);

}

Plot::Frame::Ticks::~Ticks(){

    if(!vbo) glDeleteBuffers(1, &vbo);

}

Plot::Plot(){

    glEnable(GL_DEPTH_TEST);
    GLint len1 = sizeof(vertexShader)/sizeof(vertexShader[0])-1;
    GLint len2 = sizeof(fragmentShader)/sizeof(fragmentShader[0])-1;
    
    GLuint vshader = make_shader(GL_VERTEX_SHADER,vertexShader,&len1);
    GLuint fshader = make_shader(GL_FRAGMENT_SHADER,fragmentShader,&len2);
    GLuint program = make_program(vshader,fshader);
    glUseProgram(program);
    
    glEnable(GL_CLIP_DISTANCE0);
    glEnable(GL_CLIP_DISTANCE1);
    glEnable(GL_CLIP_DISTANCE2);
    glEnable(GL_CLIP_DISTANCE3);
        
    uniform_view = glGetUniformLocation(program,"view");
    uniform_color = glGetUniformLocation(program,"color");
    uniform_clipping = glGetUniformLocation(program,"clipPlane");
    
    setClippingPlanes();
    
    calcScale();
    
    nextColor = 0;
    
}

/*
 * Public interfaces
 */
void Plot::addSeries(SeriesData data_in){

    Series newSeries(data_in);
    newSeries.color = defaultColors[nextColor++ % 3];
    allSeries.push_back(newSeries);
    
}

Plot::Series& Plot::series(unsigned int num){

    return allSeries[num];

}

void Plot::Series::append(Point2D point){

    data.push_back(point);
    refresh();

}

void Plot::Series::refresh(){

    if(!vbo) glDeleteBuffers(1, &vbo);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Point2D)*data.size(), &(data[0]), GL_DYNAMIC_DRAW);
    
}

void Plot::limits(){

    vector<float> xAll, yAll;
    for_each(allSeries.begin(),allSeries.end(),[&xAll,&yAll](Series& s){
    
        vector<float> xvec, yvec;
        for_each(s.data.begin(),s.data.end(),[&xvec](Point2D p){xvec.push_back(p.x);});
        for_each(s.data.begin(),s.data.end(),[&yvec](Point2D p){yvec.push_back(p.y);});
        xAll.insert(xAll.end(),xvec.begin(),xvec.end());
        yAll.insert(yAll.end(),yvec.begin(),yvec.end());
    
    });
    
    auto xlim = minmax_element(xAll.begin(),xAll.end());
    auto ylim = minmax_element(yAll.begin(),yAll.end());

    xlower = *xlim.first; xupper = *xlim.second;
    ylower = *ylim.first; yupper = *ylim.second;
    
    calcScale();

}

void Plot::limits(float xlow, float xup, float ylow, float yup){

    xlower = xlow; xupper = xup; ylower = ylow; yupper = yup;
    calcScale();
    
}

void Plot::xlim(float lower, float upper){

    xlower = lower;
    xupper = upper;
    calcScale();
    
}

void Plot::ylim(float lower, float upper){

    ylower = lower;
    yupper = upper;
    calcScale();
        
}

void Plot::xticks(float stride, float size){

    xtickstride = stride;
    float offset = fmod(xlower,stride);
    float tick = xlower + offset;
    vector<float> tickloc;
    while(tick <= xupper){
        tickloc.push_back(tick);
        tick += stride;
    }
    float range = xupper - xlower;
    float scale = 2.0f / range;
    float shift = xlower + range / 2.0f;
    for_each(tickloc.begin(),tickloc.end(),[scale,shift](float& t){t=scale*(t-shift);});
    
    frame.xticks(tickloc,size);
}

void Plot::yticks(float stride, float size){

    ytickstride = stride;
    float offset = fmod(xlower,stride);
    float tick = ylower + offset;
    vector<float> tickloc;
    while(tick <= yupper){
        tickloc.push_back(tick);
        tick += stride;
    }
    float range = yupper - ylower;
    float scale = 2.0f / range;
    float shift = ylower + range / 2.0f;
    for_each(tickloc.begin(),tickloc.end(),[scale,shift](float& t){t=scale*(t-shift);});
    
    frame.yticks(tickloc,size);

}

void Plot::Frame::xticks(vector<float> ticks, float size){

    PointArray tickmarkers;
    for_each(ticks.begin(),ticks.end(),[&tickmarkers,&size](float loc){
    
        Point2D markerStart = {loc,-1.0f};
        Point2D markerEnd = {loc,-1.0f-size};
        tickmarkers.push_back(markerStart);
        tickmarkers.push_back(markerEnd);
    
    });
    
    xaxis = Ticks(tickmarkers);

}

void Plot::Frame::yticks(vector<float> ticks, float size){

    PointArray tickmarkers;
    for_each(ticks.begin(),ticks.end(),[&tickmarkers,&size](float loc){
    
        Point2D markerStart = {-1.0f,loc};
        Point2D markerEnd = {-1.0f-size,loc};
        tickmarkers.push_back(markerStart);
        tickmarkers.push_back(markerEnd);
    
    });
    
    yaxis = Ticks(tickmarkers);

}

/*
 * Protected/private methods
 */
void Plot::calcScale(){

    float xrange = xupper - xlower;
    float yrange = yupper - ylower;
    vec3 scalevec = vec3(frameSize*(1-0.01)/xrange,frameSize*(1-0.01)/yrange,1.0f);
    vec3 translatevec = vec3(-1.0f*(xlower+xrange/2.0f),-1.0f*(ylower+yrange/2.0f), 0.0f);
    view = scale(mat4(1.0f),scalevec)*translate(mat4(1.0f),translatevec);
    
}

void Plot::setClippingPlanes(){

    clippingPlanes.resize(4);
    
    clippingPlanes[0] = vec4(1.0f,0.0,0.0,frameSize/2.0f);
    clippingPlanes[1] = vec4(0.0,1.0f,0.0,frameSize/2.0f);
    clippingPlanes[2] = vec4(-1.0f,0.0,0.0,frameSize/2.0f);
    clippingPlanes[3] = vec4(0.0,-1.0f,0.0,frameSize/2.0f);
    
    defaultClipping.resize(4);

    defaultClipping[0] = vec4(1.0f,0.0,0.0,1.0f);
    defaultClipping[1] = vec4(0.0,1.0f,0.0,1.0f);
    defaultClipping[2] = vec4(-1.0f,0.0,0.0,1.0f);
    defaultClipping[3] = vec4(0.0,-1.0f,0.0,1.0f);           
}

/*
 * Drawing methods
 */
void Plot::Series::draw(){

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);
    
    glDrawArrays(GL_LINE_STRIP, 0, data.size());
    
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
}

void Plot::Frame::draw(){

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);
    
    glDrawArrays(GL_LINES, 0, frame.size());
    
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    xaxis.draw();
    yaxis.draw();

}

void Plot::Frame::Ticks::draw(){

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);
    
    glDrawArrays(GL_LINES, 0, num);
    
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
}

void Plot::draw(){

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniform4fv(uniform_clipping,4,value_ptr(defaultClipping[0]));
        
    mat4 frameScale = scale(mat4(1.0f),vec3(frameSize/2.0f,frameSize/2.0f,1.0f));
    glUniformMatrix4fv(uniform_view, 1, GL_FALSE, value_ptr(frameScale));
    
    glUniform4fv(uniform_color,1,value_ptr(frame.color));
    frame.draw();
    
    glUniform4fv(uniform_clipping,4,value_ptr(clippingPlanes[0]));
    
    glUniformMatrix4fv(uniform_view, 1, GL_FALSE, value_ptr(view));

    for_each(allSeries.begin(),allSeries.end(),[this](Series& s){
        glUniform4fv(uniform_color,1,value_ptr(s.color));
        s.draw();
    });

}
