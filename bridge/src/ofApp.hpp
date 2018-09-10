#pragma once

//
//  ofApp.h
//  bridge
//
//  Created by ole kristensen on 30/08/2018.
//

#include "ofxAssimpModelLoader.h"
#include "ofxImGui.h"
#include "Mapamok.hpp"
#include "DraggablePoints.hpp"
#include "Projector.hpp"
#include "ofAutoShader.hpp"
#include "ofxPBR.h"
#include "Scene.hpp"
#include "ViewPlane.hpp"

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

class ofApp : public ofBaseApp {
public:
    
    ofApp() {
        
        pgScenes.setName("Scenes");
        
        //scenes.push_back(make_shared<BoxSplit>());
        
        for( auto s : scenes) {
            pgScenes.add(s->getParameters());
        }
        
        pgGlobal.add(pgScenes);
    }
    
    void setup();
    void update();
    void extracted();
    
    void draw();
    
    void keyPressed(int key);
    void windowResized(int w, int h);
    
    void renderScene();
    void renderCalibration();
    void drawCalibrationEditor();
    
    void loadSpaceModel(string filename);
    void loadFullModel(string filename);
    
    void save(string name);
    void load(string name);
    
    // GUI
    bool imGui();
    ofxImGui::Gui gui;
    string title = "Öresund bridge";
    bool guiVisible = true;
    bool mouseOverGui;
    int guiColumnWidth = 250;
    
    ofTrueTypeFont guiFont;
    
    // SCENES
    
    vector<shared_ptr<Scene> > scenes;
    ofParameterGroup pgScenes;

    // PARAMETERS
    
    ofParameter<float> pPbrGamma{ "Gamma", 2.2f, 0.0f, 5.0f };
    ofParameter<float> pPbrEnvLevel{ "Environment Level", 0.1f, 0.0f, 1.0f };
    ofParameter<float> pPbrEnvExposure{ "Environment Exposure", 1.0f, 0.0f, 20.0f };
    ofParameter<float> pPbrEnvRotation{ "Environment Rotation", 0.0f, 0.0f, TWO_PI };
    ofParameter<float> pPbrExposure{ "Exposure", 1.0f, 0.0f, 20.0f };
    ofParameterGroup pgPbr{ "PBR", pPbrEnvLevel, pPbrEnvExposure, pPbrEnvRotation, pPbrExposure, pPbrGamma };
    
    ofParameterGroup pgProjectors;

    ofParameterGroup pgGlobal{"Global", pgPbr};

    // STATE
    bool shaderToggle = false;
    
    // WORLD
    
    World world;
    
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
    
    //glm::vec2 projectionResolution = {1920, 1200};
    
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
    
    void drawProjectorRight(ofEventArgs & args);
    void drawProjectorLeft(ofEventArgs & args);
    void drawProjection(shared_ptr<Projector> & projector);
        
    // VIEW
    
    ofxAssimp3dPrimitive * viewNode;
    ofCamera viewCamera;
    ofFbo viewFbo;
    ofPlanePrimitive viewPlane;
    
    void setupViewPlane(float & resolution);
    void renderView();
    void drawView();
    
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
