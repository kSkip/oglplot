#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "oglplot.h"

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

Plot::Frame::Axis::Axis(array<GLfloat,4> coords, int flip){

    memcpy(axisCoords,&(coords[0]),4*sizeof(GLfloat));
    tickDirection = (flip) ? -1.0f : 1.0f;

    glGenBuffers(1, &vboAxis);
    glBindBuffer(GL_ARRAY_BUFFER, vboAxis);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*4, axisCoords, GL_STATIC_DRAW);
    
}

void Plot::Frame::Axis::setTicks(vector<float> ticks, float size){

    /* identify side */
    vec2 a1 = vec2(axisCoords[0],axisCoords[1]);
    vec2 a2 = vec2(axisCoords[2],axisCoords[3]);
    vec2 dir = (a2 - a1) * 0.5f;
    vec2 tick = vec2(dir.y,-dir.x) * size * tickDirection;
    
    PointArray markers;
    for_each(ticks.begin(),ticks.end(),[&markers,&a1,&dir,&tick](float loc){
    
        vec2 p1 = vec2((dir.x==0) ? a1.x : loc,(dir.y==0) ? a1.y : loc);
        vec2 p2 = tick + p1;
        markers.push_back({p1.x,p1.y});
        markers.push_back({p2.x,p2.y});
    
    });
            
    numTicks = markers.size();

    glGenBuffers(1, &vboTicks);
    glBindBuffer(GL_ARRAY_BUFFER, vboTicks);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Point2D)*markers.size(), &(markers[0]), GL_STATIC_DRAW);

}

Plot::Plot(){

    glEnable(GL_DEPTH_TEST);
    GLint len1 = sizeof(vertexShader)/sizeof(vertexShader[0])-1;
    GLint len2 = sizeof(fragmentShader)/sizeof(fragmentShader[0])-1;

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    GLchar *source = vertexShader;
    glShaderSource(vshader, 1, (const GLchar**)&source, &len1);
    glCompileShader(vshader);

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    source = fragmentShader;
    glShaderSource(fshader, 1, (const GLchar**)&source, &len2);
    glCompileShader(fshader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vshader);
    glAttachShader(program, fshader);
    glLinkProgram(program);

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

    top.setTicks(ticks,size);
    bottom.setTicks(ticks,size);

}

void Plot::Frame::yticks(vector<float> ticks, float size){

    left.setTicks(ticks,size);
    right.setTicks(ticks,size);

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

    left.draw();
    right.draw();
    top.draw();
    bottom.draw();
    
}

void Plot::Frame::Axis::draw(){

    /* axis line */
    glBindBuffer(GL_ARRAY_BUFFER, vboAxis);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);
    
    glDrawArrays(GL_LINES, 0, 2);
    
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* tick marks */
    glBindBuffer(GL_ARRAY_BUFFER, vboTicks);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);
    
    glDrawArrays(GL_LINES, 0, numTicks);
    
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
