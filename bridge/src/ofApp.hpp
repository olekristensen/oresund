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
#include "World.hpp"
#include "ViewPlane.hpp"
#include "ofxChoreograph.h"
#include "MeshTracker.hpp"
#include <librealsense2/rs.hpp>
#include <iostream>
#include <type_traits>

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

// to log enums
template<typename T>
std::ostream& operator<<(typename std::enable_if<std::is_enum<T>::value, std::ostream>::type& stream, const T& e)
{
    return stream << static_cast<typename std::underlying_type<T>::type>(e);
}

ImVec4 from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a, bool consistent_color = false);
ImVec4 operator+(const ImVec4& c, float v);

static const ImVec4 light_blue = from_rgba(0, 174, 239, 255, true); // Light blue color for selected elements such as play button glyph when paused
static const ImVec4 regular_blue = from_rgba(0, 115, 200, 255, true); // Checkbox mark, slider grabber
static const ImVec4 light_grey = from_rgba(0xc3, 0xd5, 0xe5, 0xff, true); // Text
static const ImVec4 dark_window_background = from_rgba(9, 11, 13, 255);
static const ImVec4 almost_white_bg = from_rgba(230, 230, 230, 255, true);
static const ImVec4 black = from_rgba(0, 0, 0, 255, true);
static const ImVec4 transparent = from_rgba(0, 0, 0, 0, true);
static const ImVec4 white = from_rgba(0xff, 0xff, 0xff, 0xff, true);
static const ImVec4 scrollbar_bg = from_rgba(14, 17, 20, 255);
static const ImVec4 scrollbar_grab = from_rgba(54, 66, 67, 255);
static const ImVec4 grey{ 0.5f,0.5f,0.5f,1.f };
static const ImVec4 dark_grey = from_rgba(30, 30, 30, 255);
static const ImVec4 sensor_header_light_blue = from_rgba(80, 99, 115, 0xff);
static const ImVec4 sensor_bg = from_rgba(36, 44, 51, 0xff);
static const ImVec4 redish = from_rgba(255, 46, 54, 255, true);
static const ImVec4 dark_red = from_rgba(200, 46, 54, 255, true);
static const ImVec4 button_color = from_rgba(62, 77, 89, 0xff);
static const ImVec4 header_window_bg = from_rgba(36, 44, 54, 0xff);
static const ImVec4 header_color = from_rgba(62, 77, 89, 255);
static const ImVec4 title_color = from_rgba(27, 33, 38, 255);
static const ImVec4 device_info_color = from_rgba(33, 40, 46, 255);
static const ImVec4 yellow = from_rgba(229, 195, 101, 255, true);
static const ImVec4 green = from_rgba(0x20, 0xe0, 0x20, 0xff, true);
static const ImVec4 dark_sensor_bg = from_rgba(0x1b, 0x21, 0x25, 200);

// STATE

enum class state {
    STARTING=0,
    WAITING=1,
    TRACKING=2,
    PLAYING=3
};

class ofApp : public ofBaseApp {
public:
    
    void setup();
    void update();
    void extracted();
    
    void draw();
        
    void keyPressed(int key);
    void keycodePressed(ofKeyEventArgs& e);
    void windowResized(int w, int h);
    
    void pbrRenderScene();
    
    void renderCalibration();
    void drawCalibrationEditor();
    
    void loadRenderModel(string filename);
    void loadNodeModel(string filename);
    
    void save(string name);
    void load(string name);
    
    // GUI
    
    bool imGui();
    ofxImGui::Gui gui;
    string title = "Øresund Bridge";
    bool guiVisible = true;
    bool mouseOverGui;
    int guiColumnWidth = 250;
    ImFont* gui_font_header;
    ImFont* gui_font_text;
    ofImage logo;
    GLuint logoID;
    
    ofTrueTypeFont fontHeader;
    ofTrueTypeFont fontBody;

    // PARAMETERS
    
    ofParameter<float> pPbrGamma{ "Gamma", 2.2f, 0.0f, 5.0f };
    ofParameter<float> pPbrEnvLevel{ "Environment Level", 0.1f, 0.0f, 1.0f };
    ofParameter<float> pPbrEnvExposure{ "Environment Exposure", 1.0f, 0.0f, 2.0f };
    ofParameter<float> pPbrEnvRotation{ "Environment Rotation", 0.0f, 0.0f, TWO_PI };
    ofParameter<float> pPbrExposure{ "Exposure", 1.0f, 0.0f, 20.0f };
    ofParameter<ofFloatColor> pPbrRoomColor{ "Room Color", ofFloatColor(0.,0.,0.,1.), ofFloatColor(0.,0.,0.,0.), ofFloatColor(1.,1.,1.,1.)};
    ofParameter<ofFloatColor> pPbrDirectionalLightColor{ "Directional Light Color", ofFloatColor(0.,0.,0.,0.), ofFloatColor(0.,0.,0.,0.), ofFloatColor(1.,1.,1.,1.)};
    ofParameter<ofFloatColor> pPbrSpotLightColor{ "Spot Light Color", ofFloatColor(0.,0.,0.,0.), ofFloatColor(0.,0.,0.,0.), ofFloatColor(1.,1.,1.,1.)};
    ofParameter<bool> pPbrFullModelView { "Full model in first person", false};
    ofParameterGroup pgPbrMaterials;
    ofParameterGroup pgPbr{ "PBR", pPbrEnvLevel, pPbrEnvExposure, pPbrEnvRotation, pPbrExposure, pPbrGamma, pPbrFullModelView, pPbrRoomColor, pPbrDirectionalLightColor, pPbrSpotLightColor, pgPbrMaterials};

    ofParameterGroup pgProjectors;

    ofParameter<ofFloatColor> pVideoColor{ "Color", ofFloatColor(1.,1.,1.,1.), ofFloatColor(0.,0.,0.,0.), ofFloatColor(1.,1.,1.,1.)};
    ofParameter<bool> pVideoDrawTestChart{ "Test Chart", false};
    ofParameter<float> pVideoPylonsAlpha{ "Pylons Alpha", 1.0, 0.0, 1.0};
    ofParameter<glm::vec3> pVideoOrigin{ "Shader origin", glm::vec3(0.,0.,0.), glm::vec3(-100.,-100.,-100.), glm::vec3(100.,100.,100.)};
    ofParameter<glm::vec2> pVideoOffset{ "Shader offset", glm::vec2(0.,0), glm::vec2(-1.,-1.), glm::vec2(1.,1.)};
    ofParameterGroup pgVideo{ "Video", pVideoColor, pVideoPylonsAlpha, pVideoDrawTestChart, pVideoOrigin, pVideoOffset };

    ofParameter<glm::vec3> pHacksPylonOffset{ "Pylon offset", glm::vec3(0.,0.,0.), glm::vec3(-1.,-1.,-1.), glm::vec3(1.,1.,1.)};

    ofParameter<glm::vec3> pHacksBridgeOffset{ "Bridge offset", glm::vec3(0.,0.,0.), glm::vec3(-10.,-10.,-10.), glm::vec3(10.,10.,10.)};
    ofParameter<glm::vec3> pHacksBridgeRotation{ "Bridge Rotation", glm::vec3(0.,0.,0.), glm::vec3(-180.,-180.,-180.), glm::vec3(180.,180.,180.)};
    ofParameter<float> pHacksBridgeScale{ "Bridge Scale", 1.0, 0.0, 1.0};
    ofParameter<float> pHacksBridgeLerp{ "Bridge Lerp", 0.0, 0.0, 1.0};

    ofParameterGroup pgHacks{ "Hacks", pHacksPylonOffset, pHacksBridgeOffset, pHacksBridgeRotation, pHacksBridgeScale, pHacksBridgeLerp};

    ofParameter<bool> pTrackingEnabled{ "Enabled", false};
    ofParameter<bool> pTrackingVisible{ "Visible", false};
    ofParameter<float> pTrackingTimeout{ "Timeout", 30.0, 0.0, 5*60.0};
    ofParameter<glm::vec3> pHeadPosition{ "Head Position", glm::vec3(0.,0.,0.), glm::vec3(-10.,-10.,-10.), glm::vec3(10.,10.,10.)};
    ofParameter<glm::vec3> pHeadOffset{ "Head Offset", glm::vec3(0.,0.,0.), glm::vec3(-1.,-1.,-1.), glm::vec3(1.,1.,1.)};
    ofParameter<glm::vec3> pTrackingCameraPosition{ "Tracking Camera Position", glm::vec3(0.,0.,0.), glm::vec3(-10.,-10.,-10.), glm::vec3(10.,10.,10.)};
    ofParameter<glm::vec3> pTrackingCameraRotation{ "Tracking Camera Rotation", glm::vec3(0.,0.,0.), glm::vec3(-180.,-180.,-180.), glm::vec3(180.,180.,180.)};

    ofParameter<glm::vec3> pTrackingBoxPosition{ "Tracking Box Position", glm::vec3(0.,0.,0.), glm::vec3(-10.,-10.,-10.), glm::vec3(10.,10.,10.)};
    ofParameter<glm::vec3> pTrackingBoxRotation{ "Tracking Box Rotation", glm::vec3(0.,0.,0.), glm::vec3(-180.,-180.,-180.), glm::vec3(180.,180.,180.)};
    ofParameter<glm::vec3> pTrackingBoxSize{ "Tracking Box Size", glm::vec3(1.,1.,1.), glm::vec3(0.,0.,0.), glm::vec3(10.,10.,10.)};

    ofParameter<glm::vec3> pTriggerBoxPosition{ "Trigger Box Position", glm::vec3(0.,0.,0.), glm::vec3(-10.,-10.,-10.), glm::vec3(10.,10.,10.)};
    ofParameter<glm::vec3> pTriggerBoxRotation{ "Trigger Box Rotation", glm::vec3(0.,0.,0.), glm::vec3(-180.,-180.,-180.), glm::vec3(180.,180.,180.)};
    ofParameter<glm::vec3> pTriggerBoxSize{ "Trigger Box Size", glm::vec3(1.,1.,1.), glm::vec3(0.,0.,0.), glm::vec3(10.,10.,10.)};
    
    ofParameter<glm::vec3> pTrackingStartPosition{ "Start Position", glm::vec3(0.,0.,0.), glm::vec3(-10.,-10.,-10.), glm::vec3(10.,10.,10.)};

    ofParameterGroup pgTracking{"Tracking", pTrackingEnabled, pTrackingVisible, pTrackingTimeout, pHeadPosition, pHeadOffset, pTrackingCameraPosition, pTrackingCameraRotation, pTrackingBoxPosition, pTrackingBoxRotation, pTrackingBoxSize, pTriggerBoxPosition, pTriggerBoxRotation, pTriggerBoxSize, pTrackingStartPosition};

    ofParameter<float> pAudioWindVolume{"Wind volume", 1.0, 0.0, 1.0};
    ofParameter<float> pAudioVideoVolume{"Video volume", 1.0, 0.0, 1.0};
    ofParameter<float> pAudioEnteringVolume{"Entering volume", 1.0, 0.0, 1.0};

    ofParameterGroup pgAudio{"Audio", pAudioWindVolume, pAudioVideoVolume, pAudioEnteringVolume};
    
    ofParameterGroup pgGlobal{"Global", pgAudio, pgVideo, pgTracking, pgPbr, pgHacks};
    
    // TIMELINE
    
    state appState = state::STARTING;
    
    ofxChoreograph::Timeline timeline;
    
    map< string, ofxChoreograph::Output<ofParameter<float> > > timelineFloatOutputs;
    map< string, ofxChoreograph::Output<ofParameter<glm::vec2> > > timelineVec2Outputs;
    map< string, ofxChoreograph::Output<ofParameter<glm::vec3> > > timelineVec3Outputs;
    map< string, ofxChoreograph::Output<ofParameter<ofFloatColor> > > timelineFloatColorOutputs;
    void startAnimation();
    
    // WORLD
    
    World world;
    
    // MODELS
    ofxAssimpModelLoader renderModelLoader;
    ofxAssimpModelLoader nodeModelLoader;
    ofxAssimp3dPrimitive * renderPrimitive;
    ofxAssimp3dPrimitive * nodePrimitive;
    
    // CALIBRATION
    const float cornerRatio = 1.0;
    const int cornerMinimum = 6;
    const float mergeTolerance = .001;
    const float selectionMergeTolerance = .001;
    ofVboMesh calibrationMesh, calibrationCornerMesh;
    
    // PROJECTORS
    glm::vec2 projectionResolution = {1920, 1200};
    
    map<string, shared_ptr<Projector> > mProjectors;
    
    ofRectangle mViewPortFront;
    shared_ptr<Projector> mProjectorFront;
    
    ofRectangle mViewPortSide;
    shared_ptr<Projector> mProjectorSide;
    
    ofRectangle mViewPortFirstPerson;
    shared_ptr<Projector> mProjectorFirstPerson;
    
    ofRectangle mViewPort;
    
    ofEasyCam mCamWall;
    ofEasyCam mCamSide;
    ofEasyCam mCamFirstPerson;
    
    void drawProjectorSide(ofEventArgs & args);
    void drawProjectorWall(ofEventArgs & args);
    void drawProjection(shared_ptr<Projector> & projector);
        
    // VIEW
    
    shared_ptr<ViewPlane> mViewFront;
    shared_ptr<ViewPlane> mViewSide;
    void renderViews();
    
    // VIDEO
    
    ofVideoPlayer videoPlayer;
    ofImage videoTestChart;
    
    // AUDIO
    ofSoundPlayer windLoopPlayer;
    ofSoundPlayer enteringSoundPlayer;

    // PBR
    
    ofxPBRCubeMap cubeMap;
    vector<ofxPBRMaterial> materials;
    vector<ofxPBRLight> lights;
    ofxPBR pbr;
    ofxAssimp3dPrimitive * currentRenderPrimitive;

    ofCamera * cam;
    map < string, bool > pbrMaterialFlags;
    shared_ptr< vector<int>> pbrRenderTextureIndexes ;
    
    ofFbo::Settings defaultFboSettings;
    
    ofAutoShader shader, videoShader, tonemap, fxaa;
    
    function<void()> scene;
    
    shared_ptr< vector<int> > getTextureIndexesContainingString(string str){
        shared_ptr< vector<int> > retVec = make_shared<vector<int> >();
        int aTextureIndex = 0;
        for(string name : renderPrimitive->textureNames){
            if(ofStringTimesInString(name, str) > 0){
                retVec->push_back(aTextureIndex);
            }
            aTextureIndex++;
        }
        return retVec;
    }

    // TRACKING
    
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
    
    ofMesh trackingMesh;
        
    ofCamera trackingCamera;
    
    MeshTracker tracker;
    ofBoxPrimitive triggerBox;

};
