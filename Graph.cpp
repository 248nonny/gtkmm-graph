
#include "Graph.hpp"
#include <gtkmm/drawingarea.h>
#include <cmath>
#include <string>
#include <glog/logging.h>

Graph::Graph() {

    // setup some Gtk parameters.
    set_vexpand(true);
    set_hexpand(true);
    set_margin(0);

    // set minimum size
    set_size_request(grid.pads[PAD_LEFT] + grid.pads[PAD_RIGHT], grid.pads[PAD_TOP] + grid.pads[PAD_BOTTOM]);

    // attach the draw function so it is called whenever the DrawingArea is redrawn.
    set_draw_func(sigc::mem_fun(*this, &Graph::on_draw));

    // update the GUI scale to the macro defined in the hpp file (or with CMake).
    update_gui_scale(GUI_SCALE);

}

// this function is called whenever the drawing area is redrawn, so after window resizes and when there's new data.
void Graph::on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height) {

    // set width and height in graph variables.
    grid.width = width;
    grid.height = height;

    // this will run the first time the graph is drawn only.
    if (!grid.runbefore) {
        find_trnfrm();
        get_grid_lines();

        cr->set_line_cap(Cairo::Context::LineCap::ROUND);
    }


    // this is run whenever the graph is resized.
    if (grid.prev_height != height || grid.prev_width !=width) {

        // find the x,y -> pixels transform for the new dimensions.
        find_trnfrm();

        // figure out where we want the grid lines to be.
        get_grid_lines();
    }

    // we draw the grid lines
    draw_grid_lines(cr);
    cr->stroke();

    // we draw the data
    plot_data(cr);
    cr->stroke();

    if (!grid.runbefore) {
        grid.runbefore = true;
    }
}


void Graph::find_trnfrm() {
    // for x:
    long double a,b;

    // we want to find a function where the min. x value is at the leftmost part of the graph, with the right pad as spacing,
    // and where the max. x value is at the rightmost part of the graph, with the left pad as spacing.

    if (grid.x_type == AxisType::LOG) {
        // the math made sense at the time of writing, and it works without issues :P
        a = (double)(grid.width - grid.pads[PAD_LEFT] - grid.pads[PAD_RIGHT]) / (log10(grid.xstop) - log10(grid.xstart));
        b = grid.pads[PAD_LEFT] - a * (grid.xstart > 0 ? log10(grid.xstart) : 0.00001);
        grid.trnfrm[0] = [a,b](double xval) {return xval > 0 ? a * log10(xval) + b : b;};
    } else if (grid.x_type == AxisType::LINEAR) {
        // a simple linear transformation with slope and intercept.
        a = (double)(grid.width - grid.pads[PAD_LEFT] - grid.pads[PAD_RIGHT]) / (grid.xstop - grid.xstart);
        b = grid.pads[PAD_LEFT] - a * grid.xstart;
        grid.trnfrm[0] = [a,b](double xval) {return a * xval + b;};
    } else {
        printf("error: Graph.grid.x_type is of unknown type.\n");
    }

    // now for y, which is strictly linear:
    a = (double)(grid.height - grid.pads[PAD_TOP] - grid.pads[PAD_BOTTOM]) / (double)(grid.ystart - grid.ystop);
    b = grid.pads[PAD_TOP] - a * grid.ystop;
    grid.trnfrm[1] = [a,b](double yval) {return a * yval + b;};
}


void Graph::get_grid_lines() {


    DLOG(INFO) << "getting grid lines...";

    if (grid.x_type == AxisType::LOG) {

        // find main log x lines; these will be drawn at each power of 10.
        float logdiff = log10((double)grid.xstop / (double)grid.xstart);

        grid.main_x_line_count = ceil(logdiff);

        for (int i = 0; i < grid.main_x_line_count; i++) {
            double &x = grid.main_x_lines[i] = pow(10, ceil(log10(grid.xstart))) * pow(10, i);
        }

        // find sub x lines; these will be at multiples of each power of 10 until the next power of ten.
        // e.g. between 10 and 100 there will be sub lines at 20, 30, 40, 50, 60, 70, 80, and 90.
        grid.sub_x_line_count = 0;

        // find the first and last powers of 10:
        short int firstPow = ceil(log10(grid.xstart));
        short int lastPow = floor(log10(grid.xstop));

        // first we loop thru and find the leftmost lines (to the left of the first big line):
        for (int i = 0; i < 10; i++) {

            // find values to the left by multiples of 10^(n-1) where n is the power of the first big line.
            double val = pow(10,firstPow) - (i + 1) * pow(10, firstPow - 1);

            // keep going until we hit the edge of the graph:
            if (val >= grid.xstart) {
                grid.sub_x_lines[i] = val;
            } else {
                grid.sub_x_line_count = i;
                break;
            }
        }

        // now we add 8 multiples (of sub lines) for each main x line (except the last):
        for (int i = 0; i < grid.main_x_line_count - 1; i++) {

            double &mVal = grid.main_x_lines[i];

            for (int i = 0; i < 8; i++) {
                grid.sub_x_lines[grid.sub_x_line_count] = mVal * (i + 2); // start with 2 * 10^n, end with 9 * 10^n.
                grid.sub_x_line_count++;
            }
        }

        // now we get the values after the last big line:
        for (int i = 0; i < 10; i++) {

            double val = pow(10,lastPow) + (i + 1) * pow(10,lastPow);

            // stop when we hit the right edge of the graph's domain.
            if (val <= grid.xstop) {
                grid.sub_x_lines[grid.sub_x_line_count] = val;
                grid.sub_x_line_count++;
            } else {
                break;
            }
        }


    } else if (grid.x_type == AxisType::LINEAR) {

        // find total length of domain
        int xdiff = grid.xstop - grid.xstart;

        // find how many main lines we will have:
        grid.main_x_line_count = floor((float)xdiff/(float)grid.main_x_line_increment);

        // make sure we don't exceed allocated space for lines array.
        if (grid.main_x_line_count >= MAX_MAIN_LINE_COUNT - 1) {
            printf("Warning! max line count exceeded, memory issues may be encountered.");
            return;
        }

        // add an extra line to account for the start and ending edges of the graph,
        // but only if the xdiff is very close to a multiple of the line increment.
        if (xdiff* 10000 % (int)(grid.main_x_line_increment * 1000) == 0) {
            grid.main_x_line_count += 1;
        }

        // find main x lines
        for (int i = 0; i < grid.main_x_line_count || i < 100; i++) {
            grid.main_x_lines[i] = grid.xstart + i * grid.main_x_line_increment;
        }

        // find sub x lines
        for (int i = 0; i < grid.main_x_line_count * grid.x_line_subdiv; i++) {

            double val = grid.xstart + i * ((float)grid.main_x_line_increment/(float)grid.x_line_subdiv);

            // store "val" in lines array only if it remains within bounds; otherwise we're done.
            if ((int)(val*1000) % (int)(grid.main_x_line_increment * 1000) == 0 && val <= grid.xstop) {
                grid.sub_x_lines[i] = val;
            } else {
                grid.sub_x_line_count = i;
                break;
            }
        }

    }

    // now we store the labels to render next to the lines.
    for (int i = 0; i < grid.main_x_line_count; i++) {

        if (grid.main_x_lines[i] == 0) {

            grid.x_line_labels[i] = "0";

        } else if (log10(grid.main_x_lines[i]) < 0) {

            int f = ceil(-1 *log10(grid.main_x_lines[i]));
            grid.x_line_labels[i] = "0.";

            for (int i = 0; i < f - 1; i++) {
                grid.x_line_labels[i].append("0");
            }

            grid.x_line_labels[i].append(std::to_string((int)((grid.main_x_lines[i] * pow(10,f) + 0.5))));

        } else {

            grid.x_line_labels[i] = std::to_string((int)grid.main_x_lines[i]);

        }
    }


    // now for the y lines :o
    // (same process as for linear x lines)
    int ydiff = grid.ystop - grid.ystart;
    grid.main_y_line_count = (((int)(ydiff*1000) % (int)(grid.main_y_line_increment*1000)) == 0 && (int)(grid.ystart*1000)%(int)(grid.main_y_line_increment*1000) == 0) ? floor((float)ydiff/(float)grid.main_y_line_increment) + 1 : floor((float)ydiff/(float)grid.main_y_line_increment);

    // find main y lines:
    for (int i = 0; i < grid.main_y_line_count; i++) {
        grid.main_y_lines[i] = grid.ystart + i * grid.main_y_line_increment;
    }


    // find sub y lines:
    for (int i = 0; i < grid.main_y_line_count * grid.y_line_subdiv; i++) {
        double val = grid.ystart + i * ((float)grid.main_y_line_increment/(float)grid.y_line_subdiv);
        if ((int)(val*1000)%(int)(grid.main_y_line_increment * 1000) == 0 && val <= grid.ystop) {
            grid.sub_y_lines[i] = val;
        } else {
            grid.sub_y_line_count = i;
            break;
        }
    }


    // record the dimensions that we calculated this for as "prev height" to later check if recalculating all this is necessary. 
    grid.prev_height = grid.height;
    grid.prev_width = grid.width;

    DLOG(INFO) << "Lines acquired successfully.";
}


// a quick function to test the trnfm variable by directly drawing linear data on the graph.
void Graph::test_trnfrm(const Cairo::RefPtr<Cairo::Context>& cr) {

    float slope = (float)(grid.ystop - grid.ystart) / (float)(grid.xstop - grid.xstart); // y = ax + b
    float b = grid.ystart - slope * grid.xstart;

    cr->move_to(grid.trnfrm[0](grid.xstart), grid.trnfrm[1](grid.ystart));

    for (int i = 0; i < 50; i++) {
        float x = grid.xstart + ((float)(grid.xstop - grid.xstart) / 50) * i;
        float y = slope * x + b;
        cr->line_to(grid.trnfrm[0](x),grid.trnfrm[1](y));
    }

    cr->stroke();

}


void Graph::draw_grid_lines(const Cairo::RefPtr<Cairo::Context>& cr) {

    // set color to grid line color, and set fontsize.
    cr->set_source_rgba(grid.grid_line_rgba[0],grid.grid_line_rgba[1],grid.grid_line_rgba[2],grid.grid_line_rgba[3]);
    cr->set_font_size(grid.fontsize);

    // draw main x lines
    for (int i = 0; i < grid.main_x_line_count; i++) {

        draw_v_line(cr, grid.main_x_lines[i]);

        // draw line label
        cr->move_to(grid.trnfrm[0](grid.main_x_lines[i]) - 6, grid.trnfrm[1](grid.ystart) + grid.text_offset);
        cr->rotate_degrees(grid.text_angle);
        cr->show_text(grid.x_line_labels[i]);
        cr->rotate_degrees(-grid.text_angle);
    }

    // draw main y lines
    for (int i = 0; i < grid.main_y_line_count; i++) {
        double &y = grid.main_y_lines[i];
        draw_h_line(cr, y);

        // draw line label
        cr->move_to(grid.trnfrm[0](grid.xstart) - grid.text_offset * 3,grid.trnfrm[1](y) + 0.3 * grid.fontsize);
        cr->show_text(std::to_string((int)y));

    }

    // set line width and stroke main lines.
    cr->set_line_width(grid.thick_line_width);
    cr->stroke();

    //draw sub x lines
    for (int i = 0; i < grid.sub_x_line_count; i++) {
        draw_v_line(cr, grid.sub_x_lines[i]);
    }

    // draw sub y lines
    for (int i = 0; i < grid.sub_y_line_count; i++) {
        draw_h_line(cr, grid.sub_y_lines[i]);
    }

    // set line width and stroke sub lines
    cr->set_line_width(grid.thin_line_width);
    cr->stroke();

    // all done!
}

void Graph::draw_v_line(const Cairo::RefPtr<Cairo::Context>& cr, double x) {
    cr->move_to(grid.trnfrm[0](x), grid.trnfrm[1](grid.ystart));
    cr->line_to(grid.trnfrm[0](x), grid.trnfrm[1](grid.ystop));
}

void Graph::draw_h_line(const Cairo::RefPtr<Cairo::Context>& cr, double y) {
    cr->move_to(grid.trnfrm[0](grid.xstart), grid.trnfrm[1](y));
    cr->line_to(grid.trnfrm[0](grid.xstop), grid.trnfrm[1](y));
}


void Graph::plot_data(const Cairo::RefPtr<Cairo::Context>& cr) {

    DLOG(INFO) << "\nplotting graph data. number of data sets: " << data.size();

    // plot each data set.
    for (int i = 0; i < data.size(); i++) {
        double range = (grid.xstop - grid.xstart) * 0.01;

        DLOG(INFO) << "plotting data set " << i+1 << "/" << data.size() << " of size " << data[i].size();

        // check if data exists in current dataset:
        if (data[i].size() > 0) {

            // set line width and line color.
            cr->set_line_width(grid.data_line_width);
            int rgba_i = i % NUM_COLOURS;
            cr->set_source_rgba(grid.data_line_rgba[rgba_i][0],grid.data_line_rgba[rgba_i][1],grid.data_line_rgba[rgba_i][2],grid.data_line_opacity);
            
            DLOG(INFO) << "data exists in this data set. finding first plottable point.";

            double x0;
            double y0;

            // find coordinates of first point; unchanged if first data point is within graph bounds,
            // but along x or y axis if values are out of range.
            if (data[i][0][0] <= grid.xstart) {
                x0 = grid.trnfrm[0](grid.xstart);
            } else if (data[i][0][0] >= grid.xstop) {
                x0 = grid.trnfrm[0](grid.xstop);
            } else {
                x0 = grid.trnfrm[0](data[i][0][0]);
            }

            if (data[i][0][1] <= grid.ystart) {
                y0 = grid.trnfrm[1](grid.ystart);
            } else if (data[i][0][1] >= grid.ystop) {
                y0 = grid.trnfrm[1](grid.ystop);
            } else {
                y0 = grid.trnfrm[1](data[i][0][1]);
            }

            // move to first point
            cr->move_to(x0, y0);

            // draw a line to each data point while staying within graph bounds.
            for (int j = 0; j < data[i].size(); j++) {
                cr->line_to(
                    grid.trnfrm[0]((data[i][j][0] >= grid.xstart && data[i][j][0] <= grid.xstop) ? data[i][j][0] : (data[i][j][0] >= grid.xstop) * grid.xstop + (data[i][j][0] < grid.xstop) * grid.xstart),
                    grid.trnfrm[1]((data[i][j][1] >= grid.ystart && data[i][j][1] <= grid.ystop) ? data[i][j][1] : (data[i][j][1] >= grid.ystop) * grid.ystop + (data[i][j][1] < grid.ystop) * grid.ystart)
                );
            }
            
            // stroke the data lines.
            cr->stroke();
            

        }
    }

    DLOG(INFO) << "graph data plotted successfully.";
}


// fill data with randomness.
void Graph::make_random_data(int data_slot) {
    DLOG(INFO) << "making random data for graph in data slot " << data_slot << ".";
    // allocate_data(DEFAULT_TEST_DATA_SIZE);
    data[data_slot].resize(DEFAULT_TEST_DATA_SIZE);

    for (int i = 0; i < data[data_slot].size(); i++) {
        
        data[data_slot][i][0] = rand() % (int)(grid.xstop - grid.xstart) + grid.xstart;
        data[data_slot][i][1] = rand() % (int)(grid.ystop - grid.ystart) + grid.ystart;
    }
    
}

// fill data with log. curve.
void Graph::make_log_data(int data_slot) {
    DLOG(INFO) << "making log data for graph in data slot " << data_slot << ".";

    // allocate_data(DEFAULT_TEST_DATA_SIZE);
    data[data_slot].resize(DEFAULT_TEST_DATA_SIZE);

    double a = (double)(grid.ystop - grid.ystart) / log10(grid.xstop/grid.xstart);
    double b = grid.ystart - a * log10(grid.xstart);

    double c = (double)(grid.xstop - grid.xstart) / (pow(10,data[data_slot].size()));
    double d = grid.xstart - c;

    for (int i = 0; i < data[data_slot].size(); i++) {
        double &x = data[data_slot][i][0] = c * pow(10,i + 1) + d;
        data[data_slot][i][1] = a * log10(x) + b;
    }
}

// fill data with straight line.
void Graph::make_linear_data(int data_slot) {
    DLOG(INFO) << "making linear data for graph in data slot " << data_slot << ".";
    // allocate_data(DEFAULT_TEST_DATA_SIZE);
    data[data_slot].resize(DEFAULT_TEST_DATA_SIZE);
    double a = (double)(grid.ystop-grid.ystart)/(grid.xstop - grid.xstart);
    double b = grid.ystart - a * grid.xstart;

    double c = (double)(grid.xstop - grid.xstart) / (pow(10,data[data_slot].size()));
    double d = grid.xstart - c;    

    for (int i = 0; i < data[data_slot].size(); i++) {
        data[data_slot][i].resize(2);
        // DLOG(INFO) << "amoggggg" << i;
        double &x = data[data_slot][i][0] = c * pow(10,i + 1) + d;
        data[data_slot][i][1] = a * x + b;
    }
}


void Graph::write_data(GraphDataSet input_data, int data_slot) {

    DLOG(INFO) << "writing graph data in slot " << data_slot << ".";

    // remove all data first, then write into variable
    data[data_slot].resize(0);
    data[data_slot].reserve(input_data.size());

    // write data
    for (int i = 0; i < input_data.size(); i++) {
        data[data_slot].push_back({input_data[i][0], input_data[i][1]});
    }

    DLOG(INFO) << "data written successfully; queuing redraw.";
    queue_draw();
}

void Graph::update_gui_scale(double scale) {

    // figure out factor of change between current scale and new scale.
    double factor = scale / grid.current_scale;

    // scale everything accordingly.
    grid.fontsize = grid.fontsize * factor;
    grid.text_offset = grid.text_offset * factor;

    for (int i = 0; i < 4; i++) {
        grid.pads[i] = grid.pads[i] * factor;
    }

    grid.data_line_width = grid.data_line_width * factor;

    // set current scale
    grid.current_scale = scale;
}