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
    
    ofParameter<int> pCalibrationRenderMode{ "RenderMode", static_cast<int>(CalibrationRenderMode::Faces) };
    ofParameterGroup pgCalibration{ "Calibration", pCalibrationRenderMode };

    // STATE
    bool editCalibration = true;
    float backgroundBrightness = 0;
    bool shaderToggle = false;
    int renderModeSelection = 0;
    bool showScales = false;
    int textureIndexForSpaceModel = 0;
    
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
    float exposure = 1.0;
    float gamma = 2.2;
    
    function<void()> scene;

};
