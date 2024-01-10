
#include "Graph.hpp"
#include <gtkmm/drawingarea.h>
#include <cmath>
#include <string>
#include <glog/logging.h>

Graph::Graph() {

    // allocate_data(DEFAULT_TEST_DATA_SIZE,true, true);
    // printf("lakaka\n");
    set_vexpand(true);
    set_hexpand(true);
    set_margin(0);
    set_size_request(grid.pads[PAD_LEFT] + grid.pads[PAD_RIGHT], grid.pads[PAD_TOP] + grid.pads[PAD_BOTTOM]);
    set_draw_func(sigc::mem_fun(*this, &Graph::on_draw));

}

Graph::~Graph() {
}

void Graph::on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height) {

    grid.width = width;
    grid.height = height;

    if (!grid.runbefore) {
        find_trnfrm();
        get_grid_lines();

        cr->set_line_cap(Cairo::Context::LineCap::ROUND);
        // make_log_data();
        // make_linear_data();

        // sort_data_x();
    }



    if (grid.prev_height != height || grid.prev_width !=width) {

        find_trnfrm();

        // printf("%f\n",grid.trnfrm[1](0));

        // cr->move_to(grid.trnfrm[0](grid.xstart), grid.trnfrm[1](grid.ystart));
        // cr->line_to(grid.trnfrm[0](grid.xstop), grid.trnfrm[0](grid.ystop));
        // test_trnfrm(cr);
        get_grid_lines();
    }
    draw_grid_lines(cr);
    cr->stroke();

    // allocate_data(DEFAULT_TEST_DATA_SIZE,true);
    // sort_data_x();
    // make_random_data();
    // sort_data_x();
    plot_data(cr);
    cr->stroke();

    if (!grid.runbefore) {
        grid.runbefore = true;
    }
}

void Graph::find_trnfrm() {
    // for x:
    long double a,b;

    if (grid.x_type == AxisType::LOG) {
        a = (double)(grid.width - grid.pads[PAD_LEFT] - grid.pads[PAD_RIGHT]) / (log10(grid.xstop) - log10(grid.xstart));
        b = grid.pads[PAD_LEFT] - a * log10(grid.xstart);
        grid.trnfrm[0] = [a,b](double xval) {return a * log10(xval) + b;};
    } else if (grid.x_type == AxisType::LINEAR) {
        a = (double)(grid.width - grid.pads[PAD_LEFT] - grid.pads[PAD_RIGHT]) / (grid.xstop - grid.xstart);
        b = grid.pads[PAD_LEFT] - a * grid.xstart;
        grid.trnfrm[0] = [a,b](double xval) {return a * xval + b;};
    } else {
        printf("error: Graph.grid.x_type is of unknown type.\n");
    }

    // now for y:
    a = (double)(grid.height - grid.pads[PAD_TOP] - grid.pads[PAD_BOTTOM]) / (double)(grid.ystart - grid.ystop);
    b = grid.pads[PAD_TOP] - a * grid.ystop;
    grid.trnfrm[1] = [a,b](double yval) {return a * yval + b;};
}

void Graph::get_grid_lines() {

    grid.prev_height = grid.height;
    grid.prev_width = grid.width;


    if (grid.x_type == AxisType::LOG) {

        // find main log x lines.
        float logdiff = log10((double)grid.xstop / (double)grid.xstart);

        grid.main_x_line_count = ceil(logdiff);
        if (grid.main_x_line_count >= MAX_MAIN_LINE_COUNT) {
            printf("Warning! max line count exceeded, memory issues may be encountered.");
        }

        for (int i = 0; i < grid.main_x_line_count; i++) {
            double &x = grid.main_x_lines[i] = pow(10, ceil(log10(grid.xstart))) * pow(10, i);

            // printf("BL: %d\n",(int)grid.main_x_lines[i]);
        }

        // find sub x lines.
        grid.sub_x_line_count = 0;

        short int firstPow = ceil(log10(grid.xstart));
        short int lastPow = floor(log10(grid.xstop));

        // first we loop thru and find the leftmost lines (to the left of the first big line):
        for (int i = 0; i < 10; i++) {
            double val = pow(10,firstPow) - (i + 1) * pow(10, firstPow - 1);

            if (val >= grid.xstart) {
                grid.sub_x_lines[i] = val;
                // printf("val: %f\n",val);
            } else {
                grid.sub_x_line_count = i;
                break;
            }
        }

        // now we add 8 multiples (of sub lines) for each main x line (except the last):
        for (int i = 0; i < grid.main_x_line_count - 1; i++) {
            double &mVal = grid.main_x_lines[i];


            // why did I write this? great question. am re-reading it now, and no clue tbh :P
            // I guess I'll just leave it here lol
            if (mVal == 0) {
                break;
            }

            for (int i = 0; i < 8; i++) {
                grid.sub_x_lines[grid.sub_x_line_count] = mVal * (i + 2);
                grid.sub_x_line_count++;
            }
        }

        // now we get the values after the last big line:
        for (int i = 0; i < 10; i++) {
            double val = pow(10,lastPow) + (i + 1) * pow(10,lastPow);

            if (val <= grid.xstop) {
                grid.sub_x_lines[grid.sub_x_line_count] = val;
                grid.sub_x_line_count++;
            } else {
                break;
            }
        }

    } else if (grid.x_type == AxisType::LINEAR) {
        // printf("gigo\n");
        int xdiff = grid.xstop - grid.xstart;
        grid.main_x_line_count = floor((float)xdiff/(float)grid.main_x_line_increment);

        if (grid.main_x_line_count >= MAX_MAIN_LINE_COUNT) {
            printf("Warning! max line count exceeded, memory issues may be encountered.");
        }

        if (xdiff* 10000 % (int)(grid.main_x_line_increment * 1000) == 0) {
            grid.main_x_line_count += 1;
        }

        // main x lines
        for (int i = 0; i < grid.main_x_line_count; i++) {
            grid.main_x_lines[i] = grid.xstart + i * grid.main_x_line_increment;
        }

        // sub x lines
        for (int i = 0; i < grid.main_x_line_count * grid.x_line_subdiv; i++) {
            double val = grid.xstart + i * ((float)grid.main_x_line_increment/(float)grid.x_line_subdiv);
            if ((int)(val*1000)%(int)(grid.main_x_line_increment * 1000) == 0 && val <= grid.xstop) {
                grid.sub_x_lines[i] = val;
            } else {
                grid.sub_x_line_count = i;
                break;
            }
        }
    }

    // now for the x labels
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
    int ydiff = grid.ystop - grid.ystart;
    grid.main_y_line_count = (((int)(ydiff*1000) % (int)(grid.main_y_line_increment*1000)) == 0 && (int)(grid.ystart*1000)%(int)(grid.main_y_line_increment*1000) == 0) ? floor((float)ydiff/(float)grid.main_y_line_increment) + 1 : floor((float)ydiff/(float)grid.main_y_line_increment);

    //main y lines:
    for (int i = 0; i < grid.main_y_line_count; i++) {
        grid.main_y_lines[i] = grid.ystart + i * grid.main_y_line_increment;
    }


    // sub y lines:
    for (int i = 0; i < grid.main_y_line_count * grid.y_line_subdiv; i++) {
        double val = grid.ystart + i * ((float)grid.main_y_line_increment/(float)grid.y_line_subdiv);
        if ((int)(val*1000)%(int)(grid.main_y_line_increment * 1000) == 0 && val <= grid.ystop) {
            grid.sub_y_lines[i] = val;
        } else {
            grid.sub_y_line_count = i;
            break;
        }
    }
}



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

    cr->set_source_rgba(grid.grid_line_rgba[0],grid.grid_line_rgba[1],grid.grid_line_rgba[2],grid.grid_line_rgba[3]);
    cr->set_font_size(grid.fontsize);

    // draw main x lines
    for (int i = 0; i < grid.main_x_line_count; i++) {
        static std::string x_line_labels[MAX_MAIN_LINE_COUNT];
        

        double &x = grid.main_x_lines[i];
        draw_v_line(cr, x);
        
        cr->move_to(grid.trnfrm[0](x) - 6, grid.trnfrm[1](grid.ystart) + grid.text_offset);
        cr->rotate_degrees(grid.text_angle);

        cr->show_text(grid.x_line_labels[i]);
        

        cr->rotate_degrees(-grid.text_angle);
    }

    // draw main y lines
    for (int i = 0; i < grid.main_y_line_count; i++) {
        double y = grid.main_y_lines[i];
        draw_h_line(cr, y);
        cr->move_to(grid.trnfrm[0](grid.xstart) - grid.text_offset * 3,grid.trnfrm[1](y) + 0.3 * grid.fontsize);
        cr->show_text(std::to_string((int)y));

    }

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

    cr->set_line_width(grid.thin_line_width);
    cr->stroke();
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
    for (int i = 0; i < data.size(); i++) {
        double range = (grid.xstop - grid.xstart) * 0.01;

        DLOG(INFO) << "plotting data set " << i+1 << "/" << data.size() << " of size " << data[i].size();

        if (data[i].size() > 0) {
            cr->set_line_width(grid.data_line_width);
            int rgba_i = i % NUM_COLOURS;
            printf("rgba_i: %d, r:%f, g:%f, b:%f, a:%f\n\n",rgba_i,grid.data_line_rgba[rgba_i][0],grid.data_line_rgba[rgba_i][1],grid.data_line_rgba[rgba_i][2],grid.data_line_opacity);
            
            cr->set_source_rgba(grid.data_line_rgba[rgba_i][0],grid.data_line_rgba[rgba_i][1],grid.data_line_rgba[rgba_i][2],grid.data_line_opacity);

            // cr->move_to(0, 0);
            DLOG(INFO) << "data exists in this data set. finding first plottable point.";
            for (int j = 0; j < data[i].size(); j++) {
                DLOG(INFO) << "moving to point " << j << ", which has values x:" << data[i][j][0] << ", y:" << data[i][j][1] << ".";
                cr->move_to(grid.trnfrm[0](data[i][j][0]),grid.trnfrm[1](data[i][j][1]));
                if (data[i][j][0] >= grid.xstart - range*0.01 && data[i][j][0] <= grid.xstop && data[i][j][1] >= grid.ystart && data[i][j][1] <= grid.ystop) {
                    break;
                } // what you should be doing is drawing a line to the edge of the graph and then break;-ing. find slope and stuff?
            }     // also, this should probably happen in pre-processing; one array is the raw data, one is the data to plot, that way
                // we don't have to recalculate the slope stuff every time.
            for (int j = 0; j < data[i].size(); j++) {
                double x = grid.trnfrm[0](data[i][j][0]);
                double y = grid.trnfrm[1](data[i][j][1]);
                // DLOG(INFO) << "plotting point #" << j << " with values {" << data[i][j][0] << ", " << data[i][j][1] << "}.";
                
                if (data[i][j][0] >= grid.xstart && data[i][j][0] <= grid.xstop && data[i][j][1] >= grid.ystart && data[i][j][1] <= grid.ystop) {
                    cr->line_to(x, y);
                } else {
                    break;
                }
            }
            cr->stroke();
            

        }
    }

    cr->stroke();
    DLOG(INFO) << "graph data plotted successfully.";
}

void Graph::make_random_data(int data_slot) {
    DLOG(INFO) << "making random data for graph in data slot " << data_slot << ".";
    // allocate_data(DEFAULT_TEST_DATA_SIZE);
    data[data_slot].resize(DEFAULT_TEST_DATA_SIZE);

    for (int i = 0; i < data[data_slot].size(); i++) {
        
        data[data_slot][i][0] = rand() % (int)(grid.xstop - grid.xstart) + grid.xstart;
        data[data_slot][i][1] = rand() % (int)(grid.ystop - grid.ystart) + grid.ystart;
    }
    
}

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

void Graph::make_linear_data(int data_slot) {
    DLOG(INFO) << "making linear data for graph in data slot " << data_slot << ".";
    // allocate_data(DEFAULT_TEST_DATA_SIZE);
    data[data_slot].resize(DEFAULT_TEST_DATA_SIZE);
    double a = (double)(grid.ystop-grid.ystart)/(grid.xstop - grid.xstart);
    double b = grid.ystart - a * grid.xstart;

    double c = (double)(grid.xstop - grid.xstart) / (pow(10,data[data_slot].size()));
    double d = grid.xstart - c;    

    for (int i = 0; i < data[data_slot].size(); i++) {
        double &x = data[data_slot][i][0] = c * pow(10,i + 1) + d;
        data[data_slot][i][1] = a * x + b;
    }
}



void Graph::sort_data_x(int data_slot) {
    DLOG(INFO) << "sorting data in slot " << data_slot << " by x axis value.";
    GraphDataSet tempData;
    tempData.resize(DEFAULT_TEST_DATA_SIZE);
    for (int i = 0; i < data[data_slot].size(); i++) {
        // tempData.data[i].resize(data[data_slot][i].size());
        for (int j = 0; j < 1; j++) {
            tempData[i][j] = data[data_slot][i][j];
        }
    }

    // allocate_data(DEFAULT_TEST_DATA_SIZE,true);
    data[data_slot].resize(DEFAULT_TEST_DATA_SIZE);
    
    for (int i = 0; i < tempData.size(); i ++) {
    	
        if (i == 0) {
        	data[data_slot][i][0] = tempData[i][0];
        	data[data_slot][i][1] = tempData[i][1];
            
        } else {
        	for (int ii = i; ii >= 0; ii --) {
            	
                
              	if (ii == 0) {
                	data[data_slot][ii][0] = tempData[i][0];
                    data[data_slot][ii][1] = tempData[i][1];
                } else if (data[data_slot][ii - 1][0] > tempData[i][0]) {
                	data[data_slot][ii][0] = data[data_slot][ii - 1][0];
                    data[data_slot][ii][1] = data[data_slot][ii - 1][1];
                } else {
                	data[data_slot][ii][0] = tempData[i][0];
                    data[data_slot][ii][1] = tempData[i][1];
                    break;
                }

            }
        }
        
    }
    DLOG(INFO) << "Success.";
}

void Graph::write_data(GraphDataSet input_data, int data_slot) {
    DLOG(INFO) << "writing graph data in slot " << data_slot << ".";

    if (data.size() < data_slot + 1) {
        data.resize(data_slot + 1);
    }

    data[data_slot].resize(0);
    data[data_slot].reserve(input_data.size());
    for (int i = 0; i < input_data.size(); i++) {
        data[data_slot].push_back({input_data[i][0], input_data[i][1]});
        // printf("x: %f, y: %f\n",input_data[i][0],input_data[i][1]); // for testing 
    }
    DLOG(INFO) << "data written successfully; queuing redraw.";
    queue_draw();
}

// void Graph::allocate_data(int size, bool set_to_zero, bool force_allocate) {
//     allocate_GraphData(size, set_to_zero, data, force_allocate);
// }