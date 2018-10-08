#include "ofApp.h"

ofPoint cursor;

void ofApp::setup(){
    
    ofSetFrameRate(30);
    ofSetVerticalSync(true);
    ofSetLogLevel(OF_LOG_NOTICE);
    ofDisableLighting();
    ofEnableDepthTest();
    ofEnableAlphaBlending();
    ofEnableAntiAliasing();
    ofSetFullscreen(true);
    
    mesh.setMode(OF_PRIMITIVE_POINTS);

    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH, 848, 480, RS2_FORMAT_ANY, 60);
    selection = pipe.start(cfg);
    
    // Find first depth sensor (devices can have zero or more then one)
    auto depth_sensor = selection.get_device().first<rs2::depth_sensor>();
    if (depth_sensor.supports(RS2_OPTION_EMITTER_ENABLED))
    {
        depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 1.f); // Enable emitter
    }
    if (depth_sensor.supports(RS2_OPTION_LASER_POWER))
    {
        // Query min and max values:
        auto range = depth_sensor.get_option_range(RS2_OPTION_LASER_POWER);
        depth_sensor.set_option(RS2_OPTION_LASER_POWER, range.max); // Set max power
    }


    auto scale =  depth_sensor.get_depth_scale();
    
    auto stream = pipe.get_active_profile().get_stream(RS2_STREAM_DEPTH);
    if (auto video_stream = stream.as<rs2::video_stream_profile>())
    {
        try
        {
            //If the stream is indeed a video stream, we can now simply call get_intrinsics()
            intrinsics = video_stream.get_intrinsics();
            
            auto principal_point = std::make_pair(intrinsics.ppx, intrinsics.ppy);
            auto focal_length = std::make_pair(intrinsics.fx, intrinsics.fy);
            rs2_distortion model = intrinsics.model;
            
            std::cout << "Principal Point         : " << principal_point.first << ", " << principal_point.second << std::endl;
            std::cout << "Focal Length            : " << focal_length.first << ", " << focal_length.second << std::endl;
            std::cout << "Distortion Model        : " << model << std::endl;
            std::cout << "Distortion Coefficients : [" << intrinsics.coeffs[0] << "," << intrinsics.coeffs[1] << "," <<
            intrinsics.coeffs[2] << "," << intrinsics.coeffs[3] << "," << intrinsics.coeffs[4] << "]" << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to get intrinsics for the given stream. " << e.what() << std::endl;
        }
    }

    tracker.set(4, 2, 4);
    tracker.setGlobalPosition(0, 1, 0);
    
    ofNode camera;
    camera.setParent(origin);
    camera.setPosition(1.85, 2.2, 2);
    camera.lookAt(glm::vec3(0,1.0,0));
    camera.rollDeg(180-15);
    camera.panDeg(14);
    camera.tiltDeg(10);
    tracker.setup(3, glm::vec3(1.95,1.0,-.85), camera, origin );
    
    cam.setParent(origin);
    
    dec_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, 2.0);
    spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.95f);
    temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.1f);
    temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 65.0f);
    temp_filter.set_option(RS2_OPTION_HOLES_FILL, 7);

}

void ofApp::update(){

    // Get depth data from camera
    auto frames = pipe.wait_for_frames();
    auto depth = frames.get_depth_frame();
    
    rs2::frame filtered = depth;
    // Note the concatenation of output/input frame to build up a chain
    filtered = dec_filter.process(filtered);
    filtered = spat_filter.process(filtered);
    filtered = temp_filter.process(filtered);

    points = pc.calculate(filtered);

    // Create oF mesh
    mesh.clear();
    int n = points.size();
    if(n!=0){
        const rs2::vertex * vs = points.get_vertices();
        for(int i=0; i<n; i++){
            if(vs[i].z){
                const rs2::vertex v = vs[i];
                glm::vec3 v3(v.x,-v.y,-v.z);
                mesh.addVertex(v3);
                int wasAdded = tracker.addVertex(v3);
                
                ofFloatColor c;
                if(wasAdded == 0){
                    c = ofFloatColor::lightGray;
                } else if (wasAdded == 1){
                    c = ofFloatColor::cyan;
                } else if (wasAdded == 2){
                    c= ofFloatColor::green;
                } else if (wasAdded == 3){
                    c = ofFloatColor::blueSteel;
                }
                
                mesh.addColor(c);
            }
        }
        tracker.update();
    }
    cam.orbitDeg(ofGetElapsedTimef()*10.0, -45., 3.0, tracker.heads.front());
    cam.setTarget(tracker.heads.front());
    cam.setNearClip(0.1);
}

void ofApp::draw(){
    
    ofBackground(100);
    
    cam.begin();
    ofEnableDepthTest();

    ofDrawAxis(1);
    ofDrawGrid(1, 5, true, false, true, false);
    
    tracker.camera.transformGL();
    ofDrawAxis(1);
    mesh.draw();
    tracker.camera.restoreTransformGL();

    tracker.draw();

    cam.end();
}

