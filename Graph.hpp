#pragma once

#include "cairomm/context.h"
#include "cairomm/refptr.h"
#include "gtkmm/enums.h"
#include <gtkmm/drawingarea.h>

// max params for graphs.
const short int MAX_MAIN_LINE_COUNT = 100;
const short int MAX_SUB_LINE_COUNT = 100;
// #define MAX_MAIN_LINE_COUNT 100
// #define MAX_SUB_LINE_COUNT 100

const short int MAX_DATAPOINTS = 50;
// #define MAX_DATAPOINTS 50

// for pads
enum class GraphPad
{
    PAD_TOP,
    PAD_RIGHT,
    PAD_BOTTOM,
    PAD_LEFT
};
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

#include <vector>

// - data (contains datasets)
// --- dataset (contains x,y pairs)
// ----- x,y pair (double)
using GraphDataSet = std::vector<std::vector<double>>;
using GraphData = std::vector<GraphDataSet>;


// struct GraphData {
//     std::vector<std::vector<std::vector<double>>> data;
// };

#define DEFAULT_TEST_DATA_SIZE 50

struct Grid {
    bool runbefore = false;

    AxisType x_type = AxisType::LINEAR;

    int prev_width;
    int prev_height;

    int width;
    int height;

    // NOTE I just changed the types to float, errors may occur :P
    float xstart = 0;
    float xstop = 1;
    float ystart = -40;
    float ystop = 100;

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

    //for linear only:
    double main_x_line_increment = 0.1;
    double x_line_subdiv = 5;

    int pads[4] = {16,16,50,50}; // goes like compass, element 0 is up, 1 is right, 2 is bottom, 3 is left.
    // int pads[4] = {0,0,0,0};

    std::function<double(double)> trnfrm[2]; // first element is x trnfm, second is y.

    int fontsize = 12;

    float grid_line_rgba[4] = {0.7,0.7,0.7, 0.6};

    float data_line_width = 2;

    float data_line_opacity = 0.7;

    // std::vector<std::vector<float>> data_line_rgba = {
    const double data_line_rgba[NUM_COLOURS][3] = {
        {1,0.27058,0}, // orange-y
        {0.114, 0.929, 0}, //green
        {0, 0.914, 0.929},
        {0.604, 0, 0.929},
        {0.929, 0, 0}
    };
    // float data_line_rgba[4] = {1,0.27058,0,0.7};

    int text_offset = 10;
    int text_angle = 60;
    int thick_line_width = 4;
    int thin_line_width = 2;
};


void allocate_GraphData(int size, bool set_to_zero,GraphData &data, bool force_allocate = false);

class Graph : public Gtk::DrawingArea {
public:
    Graph();
    virtual ~Graph();

    Grid grid;
    GraphData data;
    int data_index;
    void write_data(GraphDataSet input_data, int data_slot = 0);

protected:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);
    void get_grid_lines();
    void find_trnfrm();
    void test_trnfrm(const Cairo::RefPtr<Cairo::Context>& cr);
    void draw_grid_lines(const Cairo::RefPtr<Cairo::Context>& cr);
    
    void draw_v_line(const Cairo::RefPtr<Cairo::Context>& cr, double x);
    void draw_h_line(const Cairo::RefPtr<Cairo::Context>& cr, double x);

    void plot_data(const Cairo::RefPtr<Cairo::Context>& cr);


    void make_random_data(int data_slot = 0);
    void make_sine_data(int data_slot = 0);
    void make_log_data(int data_slot = 0);
    void make_linear_data(int data_slot = 0);

    void sort_data_x(int data_slot);
};

