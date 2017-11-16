#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Point2D {
    GLfloat x;
    GLfloat y;
};

typedef float Color[4];

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
            
                Series();
                Series(SeriesData data_in);
                ~Series();
                void push_back(Point2D point);
                void refresh();
                void draw();
                
                SeriesData data;
                Color color;
            
            protected:
            
                GLuint vbo;
                
        };

        Series& series(unsigned int num);
        
    protected:
        
        class Frame {
        
            public:
            
                Frame();
                ~Frame();
                void xticks(std::vector<float> ticks, float size);
                void yticks(std::vector<float> ticks, float size);
                void draw();
                
                Color color;
                
            protected:
            
                class Ticks {
                
                    public:
                    
                        Ticks();
                        Ticks(PointArray ticks);
                        ~Ticks();
                        void draw();
                        
                    protected:
                    
                        unsigned int num;
                        GLuint vbo;
                
                };
            
                PointArray frame;

                Ticks xaxis;
                Ticks yaxis;
                GLuint vbo;
        
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