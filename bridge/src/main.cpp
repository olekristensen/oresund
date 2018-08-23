//TODO:

// add fbo walls
// make meshes into vbo's
// add projectors and viewports
// create shaders
// make text string file format

// separate click radius from draw radius
// abstract DraggablePoint into template
// don't move model when dragging points
// only select one point at a time.


#include "ofMain.h"
#include "ofAppGLFWWindow.h"

#include "ofxAssimpModelLoader.h"
#include "ofxImGui.h"
#include "Mapamok.h"
#include "DraggablePoints.h"
#include "MeshUtils.h"
#include "Projector.h"
#include "ofAutoshader.h"
#include "ofxPBR.h"
#include "ofxPBRHelper.h"

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

class GuiTheme : public ofxImGui::BaseTheme
{
public:
    
    GuiTheme()
    {
        col_main_text = ofColor(ofFloatColor(0.3,0.3,0.2));
        col_main_head = ofColor(64,255,0);
        col_main_area = ofColor(245,244,245);
        col_win_popup = ofColor::black;
        col_win_backg = ofColor(245,244,245,255);
        
        setup();
        
    }
    
    void setup()
    {
        
    }
    
};

class ofApp : public ofBaseApp {
public:
    
    // GUI
    string title = "Öresund bridge";
    ofxImGui::Gui gui;
    bool guiToggle = true;
    int guiColumnWidth = 250;
    bool showPBRHelper = false;
    ofTrueTypeFont guiFont;
    
    // STATE
    bool editCalibration = true;
    float backgroundBrightness = 0;
    bool shaderToggle = false;
    int renderModeSelection = 0;
    bool showScales = false;
    
    // MODELS
    ofxAssimpModelLoader calibrationModel;
    ofxAssimpModelLoader renderModel;
    ofxAssimp3dPrimitive * renderModelPrimitive;
    ofxAssimp3dPrimitive * calibrationModelPrimitive;

    // CALIBRATION
    const float cornerRatio = .1;
    const int cornerMinimum = 6;
    const float mergeTolerance = .001;
    const float selectionMergeTolerance = .5;
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

    // PBR
    
    ofxPBRCubeMap cubemap[2];
    ofxPBRMaterial material;
    ofxPBRLight pbrLight;
    ofxPBR pbr;
    
    ofEasyCam * cam;
    
    ofFbo::Settings defaultFboSettings;
    
    ofxPBRHelper pbrHelper;
    
    ofAutoShader shader, tonemap, fxaa;
    float exposure = 1.0;
    float gamma = 2.2;
    
    function<void()> scene;
    
    void setup() {
        
        ofSetWindowTitle(title);
        ofSetVerticalSync(true);
        ofDisableArbTex();
        guiFont.load("fonts/OpenSans-Light.ttf", 16);
        
        if(ofFile::doesFileExist("models/calibration.dae")) {
            loadCalibrationModel("models/calibration.dae");
        } else if(ofFile::doesFileExist("models/calibration.3ds")) {
            loadCalibrationModel("models/calibration.3ds");
        }
        
        renderModelPrimitive = nullptr;
        
        if(ofFile::doesFileExist("models/render.dae")) {
            loadRenderModel("models/render.dae");
        } else if(ofFile::doesFileExist("models/render.3ds")) {
            loadRenderModel("models/render.3ds");
        }
        
        defaultFboSettings.textureTarget = GL_TEXTURE_2D;
        defaultFboSettings.useDepth = true;
        defaultFboSettings.depthStencilAsTexture = true;
        defaultFboSettings.useStencil = true;
        defaultFboSettings.minFilter = GL_LINEAR;
        defaultFboSettings.maxFilter = GL_LINEAR;
        defaultFboSettings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
        defaultFboSettings.wrapModeVertical = GL_CLAMP_TO_EDGE;
        
        mViewPort = ofRectangle(0, 0, projectionResolution.x, projectionResolution.y);
        
        mProjectorPerspective = make_shared<Projector>(ofRectangle(0,projectionResolution.y,ofGetWidth(), ofGetHeight()-projectionResolution.y),
                                                       defaultFboSettings);
        mProjectors.insert(make_pair("perspective", mProjectorPerspective));
        
        mProjectorLeft = make_shared<Projector>(ofRectangle(0, 0, projectionResolution.x, projectionResolution.y),
                                                defaultFboSettings);
        mProjectors.insert(make_pair("left", mProjectorLeft));
        
        mProjectorRight = make_shared<Projector>(ofRectangle(projectionResolution.x, 0, projectionResolution.x, projectionResolution.y),
                                                 defaultFboSettings);
        
        mProjectors.insert(make_pair("right", mProjectorRight));
        
        pbr.setup(1024*4);
        
        scene = bind(&ofApp::renderScene, this);
        
        ofxPBRFiles::getInstance()->setup("ofxPBRAssets");
        pbrHelper.setup(&pbr, ofxPBRFiles::getInstance()->getPath() + "/settings", true);
        pbrHelper.addLight(&pbrLight, "light");
        pbrHelper.addMaterial(&material, "material");
        pbrHelper.addCubeMap(&cubemap[0], "cubeMap1");
        pbrHelper.addCubeMap(&cubemap[1], "cubeMap2");
        
        string shaderPath = "shaders/postEffect/";
        shader.loadAuto(shaderPath + "shader");
        tonemap.loadAuto(shaderPath + "tonemap");
        fxaa.loadAuto(shaderPath + "fxaa");
        
        setupGui();
        
        for(auto projector : mProjectors){
            projector.second->referencePoints.enableControlEvents();
            projector.second->referencePoints.enableDrawEvent();
            projector.second->cam.setDistance(10);
            projector.second->cam.setNearClip(.01);
            projector.second->cam.setFarClip(10000);
            //projector.second.cam.setVFlip(false); // new, and nessecary?
        }
        
        
    }
    
    enum {
        RENDER_MODE_FACES = 0,
        RENDER_MODE_OUTLINE,
        RENDER_MODE_WIREFRAME_FULL,
        RENDER_MODE_WIREFRAME_OCCLUDED
    };
    
    void update() {
        calibrationModel.update();
        renderModel.update();
        if(renderModelPrimitive != nullptr){
            renderModelPrimitive->update();
        }
        
    }
    
    void renderScene() {
        ofEnableDepthTest();
        ofDisableArbTex();

    
        pbr.begin(cam);
        
        ofSetColor(255);
        
        material.begin(&pbr);
        renderModelPrimitive->recursiveDraw();
        material.end();
        
        /* PBR TEST SCENE
         material.roughness = 0.0;
         material.metallic = 0.0;
         material.begin(&pbr);
         ofDrawBox(0, -40, 0, 2000, 10, 2000);
         material.end();
         
         ofSetColor(255,0,0);
         for(int i=0;i<10;i++){
         material.roughness = float(i) / 9.0;
         for(int j=0;j<10;j++){
         material.metallic = float(j) / 9.0;
         material.begin(&pbr);
         ofDrawSphere(i * 100 - 450, 0, j * 100 - 450, 35);
         material.end();
         }
         }
         */
        pbr.end();
        ofEnableArbTex();

        ofDisableDepthTest();
    }
    
    void renderCalibration() {
        
        if(shaderToggle){
            shader.begin();
            shader.setUniform1f("elapsedTime", ofGetElapsedTimef());
            shader.end();
        }
        
        if(renderModeSelection == RENDER_MODE_FACES) {
            ofEnableDepthTest();
            ofSetColor(255, 128);
            if(shaderToggle) shader.begin();
            calibrationModelPrimitive->recursiveDraw(OF_MESH_FILL);
            if(shaderToggle) shader.end();
            ofDisableDepthTest();
        } else if(renderModeSelection == RENDER_MODE_WIREFRAME_FULL) {
            if(shaderToggle) shader.begin();
            calibrationModelPrimitive->recursiveDraw(OF_MESH_WIREFRAME);
            if(shaderToggle) shader.end();
        } else if(renderModeSelection == RENDER_MODE_OUTLINE || renderModeSelection == RENDER_MODE_WIREFRAME_OCCLUDED) {
            if(shaderToggle) shader.begin();
            prepareRender(true, true, false);
            glEnable(GL_POLYGON_OFFSET_FILL);
            float lineWidth = ofGetStyle().lineWidth;
            if(renderModeSelection == RENDER_MODE_OUTLINE) {
                glPolygonOffset(-lineWidth, -lineWidth);
            } else if(renderModeSelection == RENDER_MODE_WIREFRAME_OCCLUDED) {
                glPolygonOffset(+lineWidth, +lineWidth);
            }
            glColorMask(false, false, false, false);
            calibrationModelPrimitive->recursiveDraw(OF_MESH_FILL);
            glColorMask(true, true, true, true);
            glDisable(GL_POLYGON_OFFSET_FILL);
            calibrationModelPrimitive->recursiveDraw(OF_MESH_WIREFRAME);
            prepareRender(false, false, false);
            if(shaderToggle) shader.end();
        }
        
    }
    
    void drawCalibrationEditor() {
        
        for (auto projector : mProjectors){
            projector.second->cam.begin(projector.second->viewPort);
            
            if(showScales) {
                ofPushStyle();
                ofSetColor(255,0,255,64);
                ofDrawGrid(1.0, 10, true);
                ofDrawAxis(1.2);
                ofPopStyle();
            }
            
            ofPushStyle();
            ofSetColor(255, 32);
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            ofDisableDepthTest();
            ofFill();
            calibrationModelPrimitive->recursiveDraw();
            ofEnableDepthTest();
            ofPopStyle();
            
            projector.second->cam.end();
            
            ofMesh cornerMeshImage = calibrationCornerMesh;
            // should only update this if necessary
            project(cornerMeshImage, projector.second->cam, projector.second->viewPort - projector.second->viewPort.getPosition());
            
            // if this is a new mesh, create the points
            // should use a "dirty" flag instead allowing us to reset manually
            if(calibrationCornerMesh.getNumVertices() != projector.second->referencePoints.size()) {
                projector.second->referencePoints.clear();
                for(int i = 0; i < cornerMeshImage.getNumVertices(); i++) {
                    projector.second->referencePoints.add(ofVec2f(cornerMeshImage.getVertex(i)));
                }
            }
            
            // if the calibration is ready, use the calibration to find the corner positions
            
            // otherwise, update the points
            for(int i = 0; i < projector.second->referencePoints.size(); i++) {
                DraggablePoint& cur = projector.second->referencePoints.get(i);
                if(!cur.hit) {
                    cur.position = cornerMeshImage.getVertex(i);
                } else {
                    ofDrawLine(cur.position, cornerMeshImage.getVertex(i));
                }
            }
            
            // calculating the 3d mesh
            vector<ofVec2f> imagePoints;
            vector<ofVec3f> objectPoints;
            for(int j = 0; j <  projector.second->referencePoints.size(); j++) {
                DraggablePoint& cur =  projector.second->referencePoints.get(j);
                if(cur.hit) {
                    imagePoints.push_back(cur.position);
                    objectPoints.push_back(calibrationCornerMesh.getVertex(j));
                }
            }
            // should only calculate this when the points are updated
            projector.second->mapamok.update(projector.second->viewPort.width, projector.second->viewPort.height, imagePoints, objectPoints);
            
        }
        
    }
    
    void draw() {
        
        ofBackground(backgroundBrightness);
        ofSetColor(255);
        
        if(editCalibration) {
            
            // EDITOR

            ofEnableAlphaBlending();
            drawCalibrationEditor();
            
            for (auto projector : mProjectors){
                
                projector.second->referencePoints.enableControlEvents();
                projector.second->referencePoints.enableDrawEvent();

                if(projector.second->mapamok.calibrationReady){
                    projector.second->mapamok.begin(projector.second->viewPort);
                    ofSetColor(255, 128);
                    renderCalibration();
                    projector.second->mapamok.end();
                }
            }
            
        } else {
            
            // RENDERING
            
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            pbr.makeDepthMap(scene);
            
            for (auto projector : mProjectors){
            
                projector.second->referencePoints.disableControlEvents();
                projector.second->referencePoints.disableDrawEvent();

                ofDisableAlphaBlending();
                ofEnableDepthTest();

                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);

                projector.second->firstPass.begin();
                projector.second->firstPass.activateAllDrawBuffers();
                ofClear(0);
                
                if(projector.second->mapamok.calibrationReady){
                    projector.second->mapamok.begin(projector.second->viewPort - projector.second->viewPort.getPosition());
                    cam = &projector.second->cam;
                    scene();
                    projector.second->mapamok.end();
                } else {
                    projector.second->cam.begin(projector.second->viewPort - projector.second->viewPort.getPosition());
                    pbr.drawEnvironment(&(projector.second->cam));
                    cam = &projector.second->cam;
                    scene();
                    projector.second->cam.end();
                }
                
                projector.second->firstPass.end();
                
                glDisable(GL_CULL_FACE);

                ofDisableDepthTest();
                ofEnableAlphaBlending();
                
                // post effect
                projector.second->secondPass.begin();
                ofClear(0);
                tonemap.begin();
                tonemap.setUniformTexture("image", projector.second->firstPass.getTexture(), 0);
                tonemap.setUniform1f("exposure", exposure);
                tonemap.setUniform1f("gamma", gamma);
                projector.second->firstPass.draw(0, 0);
                tonemap.end();
                projector.second->secondPass.end();

                projector.second->output.begin();
                fxaa.begin();
                fxaa.setUniformTexture("image", projector.second->secondPass.getTexture(), 0);
                fxaa.setUniform2f("texel", 1.25 / float(projector.second->secondPass.getWidth()), 1.25 / float(projector.second->secondPass.getHeight()));
                projector.second->secondPass.draw(0,0);
                fxaa.end();
                projector.second->output.end();
                
                projector.second->output.draw(projector.second->viewPort);

            }
            
        }
        
        // draw frames
        for (auto projector : mProjectors){
            ofPushStyle();
            ofSetColor(255,255,255,255);
            ofNoFill();
            ofDrawRectangle(projector.second->viewPort);
            ofFill();
            guiFont.drawString(projector.first, projector.second->viewPort.x+12,projector.second->viewPort.y+24);
            ofPopStyle();
        }

        
        // GUI
        
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        ofFill();
        if(guiToggle){
            gui.begin();
            ImGuiWindowFlags window_flags = 0;
            window_flags |= ImGuiWindowFlags_NoTitleBar;
            window_flags |= ImGuiWindowFlags_NoResize;
            window_flags |= ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoCollapse;
            window_flags |= ImGuiWindowFlags_ShowBorders;
            
            ImGui::SetNextWindowPos(ofVec2f(0,0));
            ImGui::SetNextWindowSize(ofVec2f(guiColumnWidth,ofGetHeight()));
            ImGui::Begin("Main###Debug", NULL, window_flags);
            
            // Title
            ImGui::PushFont(ImGuiIO().Fonts->Fonts[2]);
            ImGui::TextUnformatted(title.c_str());
            ImGui::PopFont();
            
            ImGui::TextUnformatted("den frie vilje");
            ImGui::Text("FPS %.3f", ofGetFrameRate());
            
            ImGui::Separator();
            
            ImGui::PushFont(ImGuiIO().Fonts->Fonts[1]);
            ImGui::TextUnformatted("Calibration");
            ImGui::PopFont();
            
            ImGui::Separator();
            
            ImGui::Checkbox("Edit", &editCalibration);
            //            bool guiCalibrationReady = mapamok.calibrationReady;
            //            ImGui::Checkbox("Ready", &guiCalibrationReady);
            ImGui::Checkbox("Show scales", &showScales);
            //            ImGui::Checkbox("Fix principal point", &mapamok.bCV_CALIB_FIX_PRINCIPAL_POINT);
            //            ImGui::Checkbox("Fix aspect ratio", &mapamok.bCV_CALIB_FIX_ASPECT_RATIO);
            //            ImGui::Checkbox("Fix K1", &mapamok.bCV_CALIB_FIX_K1);
            //            ImGui::Checkbox("Fix K2", &mapamok.bCV_CALIB_FIX_K2);
            //            ImGui::Checkbox("Fix K3", &mapamok.bCV_CALIB_FIX_K3);
            //            ImGui::Checkbox("Zero tangent dist", &mapamok.bCV_CALIB_ZERO_TANGENT_DIST);
            
            ImGui::Checkbox("Use Shader", &shaderToggle);
            bool guiShaderValid = shader.isValid;
            ImGui::Checkbox("Shader Valid", &guiShaderValid);
            
            const char* guiRenderModeSelectionItems[] = { "Faces", "Outline", "Wireframe Full", "Wireframe Occluded" };
            ImGui::Combo("Render Mode", &renderModeSelection, guiRenderModeSelectionItems, IM_ARRAYSIZE(guiRenderModeSelectionItems));
            
            ImGui::PushFont(ImGuiIO().Fonts->Fonts[1]);
            ImGui::TextUnformatted("Rendering");
            ImGui::PopFont();
            
            ImGui::Separator();
            
            static bool enableCameraControl;
            ImGui::Checkbox("Enable Camera Control", &enableCameraControl);
            if(enableCameraControl){
                for(auto projector : mProjectors){
                    projector.second->cam.enableMouseInput();
                }
            } else {
                for(auto projector : mProjectors){
                    projector.second->cam.disableMouseInput();
                }
            }
            
            ImGui::PushFont(ImGuiIO().Fonts->Fonts[1]);
            ImGui::TextUnformatted("Interface");
            ImGui::PopFont();
            
            ImGui::Separator();
            static bool guiShowTest;
            ImGui::Checkbox("Show Test Window", &guiShowTest);
            ImGui::Checkbox("Show PBR Helper", &showPBRHelper);
            if(guiShowTest)
                ImGui::ShowTestWindow();
            
            ImGui::End();
            
            if(showPBRHelper){
                ImGui::Begin("ofxPBRHelper");
                ImGui::DragFloat("exposure", &exposure, 0.1);
                ImGui::DragFloat("gamma", &gamma, 0.1);
                pbrHelper.drawGui();
                ImGui::End();
            }
            
            gui.end();
        }
        
    }
    
    void loadCalibrationModel(string filename) {
        
        ofSetLogLevel("ofxAssimpModelLoader", OF_LOG_VERBOSE);
        
        calibrationModel.loadModel(filename, false, false);
        
        // rotate the model to match the ofMeshes we get later...
        calibrationModel.setRotation(0, 180, 0, 0, 1.0);
        
        // make sure to load to scale
        calibrationModel.setScaleNormalization(false);
        calibrationModel.calculateDimensions();
        
        calibrationModelPrimitive = calibrationModel.getPrimitives();
        vector<ofMesh> meshes = calibrationModelPrimitive->getBakedMeshesRecursive();
        
        // join all the meshes
        calibrationMesh = ofVboMesh();
        calibrationMesh = joinMeshes(meshes);
        ofVec3f cornerMin, cornerMax;
        getBoundingBox(calibrationMesh, cornerMin, cornerMax);
        
        // merge
        // another good metric for finding corners is if there is a single vertex touching
        // the wall of the bounding box, that point is a good control point
        
        calibrationCornerMesh = ofVboMesh();
        for(int i = 0; i < meshes.size(); i++) {
            ofMesh mergedMesh = mergeNearbyVertices(meshes[i], mergeTolerance);
            if(mergedMesh.getVertices().size() > cornerMinimum){
                vector<unsigned int> cornerIndices = getRankedCorners(mergedMesh);
                int n = cornerIndices.size() * cornerRatio;
                n = MIN(MAX(n, cornerMinimum), cornerIndices.size());
                for(int j = 0; j < n; j++) {
                    int index = cornerIndices[j];
                    const ofVec3f& corner = mergedMesh.getVertices()[index];
                    calibrationCornerMesh.addVertex(corner);
                }
            }
        }
        calibrationCornerMesh = mergeNearbyVertices(calibrationCornerMesh, selectionMergeTolerance);
        calibrationCornerMesh.setMode(OF_PRIMITIVE_POINTS);
    }
    
    void loadRenderModel(string filename) {
        
        ofSetLogLevel("ofxAssimpModelLoader", OF_LOG_VERBOSE);
        // load model, optimize and pretransform all vertices in the global space.
        renderModel.loadModel(filename, false, false);
        
        // rotate the model to match the ofMeshes we get later...
        renderModel.setRotation(0, 180, 0, 0, 1.0);
        
        // make sure to load to scale
        renderModel.setScaleNormalization(false);
        renderModel.calculateDimensions();
        
        renderModelPrimitive = renderModel.getPrimitives();
        
    }
    
    void dragEvent(ofDragInfo dragInfo) {
        if(dragInfo.files.size() == 1) {
            string filename = dragInfo.files[0];
            loadCalibrationModel(filename);
        }
    }
    
    void keyPressed(int key) {
        if(key == 'f') {
            ofToggleFullscreen();
        }
        if(key == '\t') {
            guiToggle = !guiToggle;
        }
        if(key == 's') {
            for(auto projector : mProjectors){
                projector.second->mapamok.save("calibrations/" + projector.first);
            }
        }
        if(key == 'l') {
            for(auto projector : mProjectors){
                projector.second->mapamok.load("calibrations/" + projector.first);
            }
        }
    }
    
    void windowResized(int w, int h){
        mProjectors.at("perspective")->viewPort.set(0, projectionResolution.y, w, h-projectionResolution.y);
        mProjectors.at("perspective")->resizeFbos();
    }
    
    void setupGui(){
        
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = false;
        
        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Light.ttf", true).c_str(), 16);
        io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Regular.ttf", true).c_str(), 16);
        io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Light.ttf", true).c_str(), 32);
        io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Regular.ttf", true).c_str(), 11);
        io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Bold.ttf", true).c_str(), 11);
        io.Fonts->Build();
        
        gui.setup(new GuiTheme());
        
        ImGuiStyle & style = ImGui::GetStyle();
        
        style.WindowPadding            = ImVec2(15, 15);
        style.WindowRounding           = .0f;
        style.FramePadding             = ImVec2(8, 3);
        style.FrameRounding            = 3.0f;
        style.ItemSpacing              = ImVec2(12, 5);
        style.ItemInnerSpacing         = ImVec2(8, 6);
        style.IndentSpacing            = 25.0f;
        style.ScrollbarSize            = 15.0f;
        style.ScrollbarRounding        = 9.0f;
        style.GrabMinSize              = 8.0f;
        style.GrabRounding             = 2.0f;
        
        style.Colors[ImGuiCol_Text]                  = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.40f, 0.39f, 0.38f, 0.77f);
        style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.96f, 0.96f, 0.96f, 1.0f);
        style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(1.00f, 1.00f, 1.00f, 0.35f);
        style.Colors[ImGuiCol_PopupBg]               = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_Border]                = ImVec4(0.84f, 0.83f, 0.80f, 0.42f);
        style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
        style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.03f, 0.00f, 0.00f, 0.02f);
        style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.84f, 0.83f, 0.80f, 0.42f);
        style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.84f, 0.83f, 0.80f, 0.20f);
        style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.84f, 0.83f, 0.80f, 1.00f);
        style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(1.00f, 1.00f, 1.00f, 0.92f);
        style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(1.00f, 1.00f, 1.00f, 0.76f);
        style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.00f, 0.00f, 0.00f, 0.21f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.00f, 0.00f, 0.00f, 0.32f);
        style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        style.Colors[ImGuiCol_ComboBg]               = ImVec4(1.00f, 0.98f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.00f, 0.00f, 0.00f, 0.19f);
        style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.00f, 0.00f, 0.00f, 0.14f);
        style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        style.Colors[ImGuiCol_Button]                = ImVec4(0.00f, 0.00f, 0.00f, 0.09f);
        style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.00f, 0.00f, 0.00f, 0.01f);
        style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.00f, 0.00f, 0.00f, 0.12f);
        style.Colors[ImGuiCol_Header]                = ImVec4(0.02f, 0.05f, 0.00f, 0.05f);
        style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.00f, 0.00f, 0.00f, 0.05f);
        style.Colors[ImGuiCol_HeaderActive]          = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_Column]                = ImVec4(0.00f, 0.00f, 0.00f, 0.32f);
        style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);
        style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.00f, 0.00f, 0.00f, 0.04f);
        style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.25f, 1.00f, 0.00f, 0.78f);
        style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_CloseButton]           = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
        style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
        style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
        style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
        style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
        style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.25f, 1.00f, 0.00f, 0.57f);
        style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(1.00f, 0.98f, 0.95f, 0.68f);
    }
    
};

int main() {
    //TODO: windows for projections
    ofGLWindowSettings settings;
    settings.setGLVersion(4, 1);
    settings.setSize(1280,720);
    ofCreateWindow(settings);
    ofRunApp(new ofApp());
}
