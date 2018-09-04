#pragma once

//
//  ofApp.h
//  bridge
//
//  Created by ole kristensen on 30/08/2018.
//

#include "ofxAssimpModelLoader.h"
#include "ofxImGui.h"
#include "Mapamok.h"
#include "DraggablePoints.h"
#include "Projector.h"
#include "ofAutoshader.h"
#include "ofxPBR.h"

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

enum class CalibrationRenderMode
{
    Faces,
    Outline,
    WireframeFull,
    WireframeOccluded
};

class ofApp : public ofBaseApp {
public:
    
    void setup();
    void update();
    void draw();
    
    void drawProjectorRight(ofEventArgs & args);
    void drawProjectorLeft(ofEventArgs & args);
    
    void drawProjection(shared_ptr<Projector> & projector);
    
    void keyPressed(int key);
    void windowResized(int w, int h);
    
    void allocateViewFbo();
    void renderScene();
    void renderCalibration();
    void drawCalibrationEditor();
    
    void loadSpaceModel(string filename);
    void loadFullModel(string filename);
    
    // GUI
    bool imGui();
    ofxImGui::Gui gui;
    string title = "Öresund bridge";
    bool guiVisible = true;
    bool mouseOverGui;
    int guiColumnWidth = 250;
    
    ofTrueTypeFont guiFont;
    
    // PARAMETERS
    
    ofParameter<bool> pCalibrationEdit {"Edit", true};
    ofParameter<bool> pCalibrationShowScales {"Show Scales", true};
    ofParameter<int> pCalibrationHighlightIndex{ "Highlight", 0, 0, 0 };
    ofParameter<int> pCalibrationRenderMode{ "Render Mode", static_cast<int>(CalibrationRenderMode::Faces) };
    ofParameterGroup pgCalibration{ "Calibration", pCalibrationEdit, pCalibrationShowScales, pCalibrationHighlightIndex, pCalibrationRenderMode };

    ofParameter<float> pPbrExposure{ "Exposure", 1.0f, 0.0f, 20.0f };
    ofParameter<float> pPbrGamma{ "Gamma", 2.2f, 0.0f, 5.0f };
    ofParameterGroup pgPbr{ "PBR", pPbrExposure, pPbrGamma };

    // STATE
    bool shaderToggle = false;
    
    // MODELS
    ofxAssimpModelLoader spaceModel;
    ofxAssimpModelLoader fullModel;
    ofxAssimp3dPrimitive * fullModelPrimitive = nullptr;
    ofxAssimp3dPrimitive * spaceModelPrimitive = nullptr;
    ofxAssimp3dPrimitive * wallModelNode = nullptr;
    ofxAssimp3dPrimitive * trussModelNode = nullptr;
    ofxAssimp3dPrimitive * pylonsModelNode = nullptr;
    ofxAssimp3dPrimitive * approachModelNode = nullptr;
    
    // CALIBRATION
    const float cornerRatio = 1.0;
    const int cornerMinimum = 6;
    const float mergeTolerance = .001;
    const float selectionMergeTolerance = .001;
    ofVboMesh calibrationMesh, calibrationCornerMesh;
    
    // PROJECTORS
    glm::vec2 projectionResolution = {1280, 720};
    
    map<string, shared_ptr<Projector> > mProjectors;
    
    ofRectangle mViewPortLeft;
    shared_ptr<Projector> mProjectorLeft;
    
    ofRectangle mViewPortRight;
    shared_ptr<Projector> mProjectorRight;
    
    ofRectangle mViewPortPerspective;
    shared_ptr<Projector> mProjectorPerspective;
    
    ofRectangle mViewPort;
    
    ofEasyCam mCamLeft;
    ofEasyCam mCamRight;
    ofEasyCam mCamPerspective;
    
    // VIEW
    
    ofxAssimp3dPrimitive * viewNode;
    ofCamera viewCamera;
    ofFbo viewFbo;
    ofRectangle viewRectangle;
    float viewResolution = 200;
    
    // PBR
    
    ofxPBRCubeMap cubeMap;
    vector<ofxPBRMaterial> materials;
    ofxPBRLight pbrLight;
    ofxPBR pbr;
    
    ofCamera * cam;
    
    ofFbo::Settings defaultFboSettings;
    
    ofAutoShader shader, tonemap, fxaa;
    
    function<void()> scene;

};
