#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Point2D {
    GLfloat x;
    GLfloat y;
};

typedef glm::vec4 Color;

typedef std::vector<Point2D> SeriesData;
typedef std::vector<Point2D> PointArray;

class Plot {

    public:
    
        Plot();
        
        void addSeries(SeriesData data_in);
        void addSeries();
        void limits();
        void limits(float xlow, float xup, float ylow, float yup);
        void xlim(float lower, float upper);
        void ylim(float lower, float upper);
        void xticks(float stride, float size);
        void yticks(float stride, float size);
        void draw();
    
        class Series {
        
            public:
            
                Series(SeriesData data_in);
                ~Series();
                void append(Point2D point);
                void refresh();
                void draw();
                
                SeriesData data;
                Color color;
            
            protected:
            
                GLuint vbo = 0;
                
        };

        Series& series(unsigned int num);
        
    protected:
        
        class Frame {
        
            public:
            
                Frame() : color(glm::vec4(1.0f,1.0f,1.0f,1.0f)){}
                void xticks(std::vector<float> ticks, float size);
                void yticks(std::vector<float> ticks, float size);
                void draw();
                
                Color color;
                
            protected:
            
                class Axis {
                
                    public:
                    
                        Axis(std::array<GLfloat,4> coords, int flip);
                        void draw();
                        void setTicks(std::vector<float> ticks, float size);
                        
                    protected:
                    
                        GLfloat axisCoords[4];
                        float tickDirection;
                        GLuint vboAxis  = 0;
                        GLuint vboTicks = 0;
                        unsigned int numTicks;
                        
                        void createBuffer();
                        
                };
                
                Axis   left = Axis({-1.0f,-1.0f,-1.0f,1.0f},1);
                Axis  right = Axis({1.0f,-1.0f,1.0f, 1.0f},0);
                Axis    top = Axis({-1.0f,1.0f,1.0f,1.0f},1);
                Axis bottom = Axis({-1.0f,-1.0f,1.0f,-1.0f},0);
        
        };
        
        std::vector<Series> allSeries;
        Frame frame;
        glm::mat4 view;
        std::vector<glm::vec4> clippingPlanes, defaultClipping;
        GLint uniform_view, uniform_color, uniform_clipping;
        unsigned int nextColor;
        float xlower, xupper, ylower, yupper;
        float xtickstride, xticksize;
        float ytickstride, yticksize;
        float frameSize = 1.5f;
        
        void setClippingPlanes();
        void calcScale();
        
};
