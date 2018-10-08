#pragma once

#include "ofMain.h"
#include <librealsense2/rs.hpp>
#include "tracker.hpp"

class ofApp : public ofBaseApp{
public:
    void setup();
    void update();
    void draw();
    
    void loadBagFile(string path);
    
    rs2::pipeline pipe;
    rs2::device device;
    rs2::pipeline_profile selection;
    rs2::colorizer color_map;
    rs2::frame colored_depth;
    rs2::frame colored_filtered;
    rs2_intrinsics intrinsics;
    
    rs2::decimation_filter dec_filter;
    rs2::spatial_filter spat_filter;
    rs2::temporal_filter temp_filter;

    rs2::points points;
    rs2::pointcloud pc;
    
    ofVboMesh mesh;
    ofEasyCam cam;
    
    ofNode origin;
    
    MeshTracker tracker;
};
