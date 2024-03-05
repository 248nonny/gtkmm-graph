#pragma once

#include <vector>
#include <gtkmm.h>



// This GUI_SCALE macro should ideally be defined in a CMakeLists.txt file; if not it defaults to 1.
#ifndef GUI_SCALE
#define GUI_SCALE 1
#endif


// max params for graphs.
const short int MAX_MAIN_LINE_COUNT = 100;
const short int MAX_SUB_LINE_COUNT = 100;
const short int MAX_DATAPOINTS = 200;

// for pads on the edges of the drawing area.
enum class GraphPad
{
    PAD_TOP,
    PAD_RIGHT,
    PAD_BOTTOM,
    PAD_LEFT
};

// to define whether the x axis is linear or logarithmic.
enum class AxisType
{
    LOG,
    LINEAR
};


#define PAD_TOP 0
#define PAD_RIGHT 1
#define PAD_BOTTOM 2
#define PAD_LEFT 3

#define NUM_COLOURS 5


// - GraphData (contains GraphDataSets)
// --- GraphDataSet (contains x,y pairs)
// ----- x,y pair (vector of doubles with size 2)
using GraphDataSet = std::vector<std::vector<double>>;
using GraphData = std::vector<GraphDataSet>;


#define DEFAULT_TEST_DATA_SIZE 200

// Future Me here: When I wrote this, I was still exploring c++, so I must say the structure of all that is below,
// and all that is in the Graph.cpp file is rather unorthodox and at times confusing. I apologize in advance :P


// a struct with some parameters for the graph.
struct Grid {
    bool runbefore = false;

    AxisType x_type = AxisType::LINEAR;

    int prev_width;
    int prev_height;

    int width;
    int height;

    // NOTE I just changed the types to float, errors may occur :P
    double xstart = 0;
    double xstop = 1;
    double ystart = -40;
    double ystop = 100;

    // variables that store the coordinates to draw lines at.
    double main_x_lines[MAX_MAIN_LINE_COUNT]; // main lines determine tick marks, sub lines are for visuals only.
    int main_x_line_count = 0;
    std::string x_line_labels[MAX_MAIN_LINE_COUNT];

    double sub_x_lines[MAX_SUB_LINE_COUNT];
    int sub_x_line_count = 0;

    double main_y_lines[MAX_MAIN_LINE_COUNT];
    int main_y_line_count = 0;

    double sub_y_lines[MAX_SUB_LINE_COUNT];
    int sub_y_line_count = 0;

    double main_y_line_increment = 20;
    double y_line_subdiv = 4;

    //for linear x axis only:
    double main_x_line_increment = 0.1;
    double x_line_subdiv = 5;

    // goes like a compass, element 0 is top, 1 is right, 2 is bottom, 3 is left.
    // this relationship is defined in a series of "#define" statements at the top of the file.
    int pads[4] = {16,16,50,50};

    // a set of functions that map the x-y variable space to literal pixels in the DrawingArea;
    // this is used extensively when drawing lines on the graph later.
    std::function<double(double)> trnfrm[2]; // first element is x trnfm, second is y.

    int fontsize = 12;

    double current_scale = 1; // used with GUI_SCALE to scale graph elements.

    float grid_line_rgba[4] = {0.7,0.7,0.7, 0.6}; // define color of the grid lines.

    float data_line_width = 2; // width of grid line in px (will be scaled according to GUI_SCALE).

    float data_line_opacity = 0.7;

    // colors to use for each dataset (multiple can be graphed at a time, so this helps distinguish them).
    double data_line_rgba[NUM_COLOURS][3] = {
        {1,0.27058,0}, // orange-y
        {0.114, 0.929, 0}, //green
        {0, 0.914, 0.929},
        {0.604, 0, 0.929},
        {0.929, 0, 0}
    };

    // text size and offset for axis numbering.
    int text_offset = 10;
    int text_angle = 60;

    // line widths for graph grid lines.
    int thick_line_width = 4;
    int thin_line_width = 2;
};

// The Graph class, inheriting from the Gtk DrawingArea.
class Graph : public Gtk::DrawingArea {
public:

    Graph();

    // create grid parameters and data variables.
    Grid grid;
    GraphData data;
    int data_index;

    // setter function to write new data to graph. Graph.queue_draw() should be called after to render the new data
    // (this is inherited from the DrawingArea).
    void write_data(GraphDataSet input_data, int data_slot = 0);

    // setter function to update the graphics scale; useful for high dpi screens.
    void update_gui_scale(double scale = 1);

    // functions to test the graph by creating certain types of data; these were used extensively for testing.
    void make_random_data(int data_slot = 0);
    void make_sine_data(int data_slot = 0);
    void make_log_data(int data_slot = 0);
    void make_linear_data(int data_slot = 0);

protected:

    // the function that gets called every time the DrawingArea is asked to redraw iteself.
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);

    // a function that determines x and y values for grid lines based on the domain and range specified in "grid" (ystart, xstart, ystop, xstop).
    void get_grid_lines();

    // a function that finds the maps from x,y in the space of the data to literal pixels (based on height and width of widget).
    void find_trnfrm();

    // this was used to test whether the trnfrm found is accurate; it is no longer necessary,
    // but I'm keeping it here for sentimental value :D
    void test_trnfrm(const Cairo::RefPtr<Cairo::Context>& cr);

    // this function reads stored x and y values for grid lines and draws them.
    void draw_grid_lines(const Cairo::RefPtr<Cairo::Context>& cr);
    
    // functions to draw vertical or horizontal lines within the bounds of the graph.
    void draw_v_line(const Cairo::RefPtr<Cairo::Context>& cr, double x);
    void draw_h_line(const Cairo::RefPtr<Cairo::Context>& cr, double y);

    // This function draws a line connecting all the datapoints in the graph's stored data (it plots the data :o)
    void plot_data(const Cairo::RefPtr<Cairo::Context>& cr);

};

