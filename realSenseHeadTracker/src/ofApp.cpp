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
    
    mesh.setMode(OF_PRIMITIVE_POINTS);

    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH);
    selection = pipe.start(cfg);
    
    // Find first depth sensor (devices can have zero or more then one)
    auto sensor = selection.get_device().first<rs2::depth_sensor>();
    auto scale =  sensor.get_depth_scale();
    
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
    camera.setPosition(2, 0.2, 2);
    camera.lookAt(glm::vec3(0,1.0,0));
    tracker.setup(3, glm::vec3(1.9,0.5,1.9), camera, origin );
    
    cam.setParent(origin);
    
    dec_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, 3);
    spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.75f);

}

void ofApp::update(){

    // Get depth data from camera
    auto frames = pipe.wait_for_frames();
    auto depth = frames.get_depth_frame();
    
    rs2::frame filtered = depth;
    // Note the concatenation of output/input frame to build up a chain
    filtered = dec_filter.process(filtered);
    filtered = spat_filter.process(filtered);

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
                bool wasAdded = tracker.addVertex(v3);
                mesh.addColor(ofFloatColor(wasAdded?1.0:0,0,ofMap(v.z, 0, 6, 1, 0), 0.8));
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

