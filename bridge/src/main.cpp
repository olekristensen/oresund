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
#include "ofAutoshader.h"
#include "ofxPBR.h"
#include "ofxPBRHelper.h"

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

class ReferencePoints : public DraggablePoints {
public:
    void draw(ofEventArgs& args) {
        ofPushStyle();
        ofSetColor(ofColor::red);
        DraggablePoints::draw(args);
        ofPopStyle();
    }
};

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

    // MAPAMOK
    Mapamok mapamok;
    const float cornerRatio = .1;
    const int cornerMinimum = 6;
    const float mergeTolerance = .001;
    const float selectionMergeTolerance = .5;
    ofVboMesh calibrationMesh, calibrationCornerMesh;
    ReferencePoints referencePoints;
    
    ofEasyCam cam;

    ofxPBRCubeMap cubemap[2];
    ofxPBRMaterial material;
    ofxPBRLight pbrLight;
    ofxPBR pbr;
    
    ofFbo firstPass, secondPass;
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

        defaultFboSettings.textureTarget = GL_TEXTURE_2D;
        defaultFboSettings.useDepth = true;
        defaultFboSettings.depthStencilAsTexture = true;
        defaultFboSettings.useStencil = true;
        defaultFboSettings.minFilter = GL_LINEAR;
        defaultFboSettings.maxFilter = GL_LINEAR;
        defaultFboSettings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
        defaultFboSettings.wrapModeVertical = GL_CLAMP_TO_EDGE;
        resizeFbos();

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

        cam.setDistance(10);
        cam.setNearClip(.01);
        cam.setFarClip(10000);
        
        referencePoints.setClickRadius(8);
        referencePoints.enableControlEvents();
        
        pbr.setup(1024);
        
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
        pbr.begin(&cam);
        
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
            calibrationMesh.drawFaces();
            if(shaderToggle) shader.end();
            ofDisableDepthTest();
        } else if(renderModeSelection == RENDER_MODE_WIREFRAME_FULL) {
            if(shaderToggle) shader.begin();
            calibrationMesh.drawWireframe();
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
            calibrationMesh.drawFaces();
            glColorMask(true, true, true, true);
            glDisable(GL_POLYGON_OFFSET_FILL);
            calibrationMesh.drawWireframe();
            prepareRender(false, false, false);
            if(shaderToggle) shader.end();
        }
        
    }
    
    void drawCalibraitonEditor() {
        
        cam.begin();
        
        if(showScales) {
            ofPushStyle();
            ofSetColor(255,0,255,64);
            ofDrawGrid(1.0, 10, true);
            ofDrawAxis(1.2);
            ofPopStyle();
        }

        ofPushStyle();
        ofSetColor(255, 64);
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        ofFill();
        calibrationMesh.drawFaces();
        ofPopStyle();
        
        cam.end();
        
        ofMesh cornerMeshImage = calibrationCornerMesh;
        // should only update this if necessary
        project(cornerMeshImage, cam, ofGetWindowRect());
        
        // if this is a new mesh, create the points
        // should use a "dirty" flag instead allowing us to reset manually
        if(calibrationCornerMesh.getNumVertices() != referencePoints.size()) {
            referencePoints.clear();
            for(int i = 0; i < cornerMeshImage.getNumVertices(); i++) {
                referencePoints.add(ofVec2f(cornerMeshImage.getVertex(i)));
            }
        }
        
        // if the calibration is ready, use the calibration to find the corner positions
        
        // otherwise, update the points
        for(int i = 0; i < referencePoints.size(); i++) {
            DraggablePoint& cur = referencePoints.get(i);
            if(!cur.hit) {
                cur.position = cornerMeshImage.getVertex(i);
            } else {
                ofDrawLine(cur.position, cornerMeshImage.getVertex(i));
            }
        }
        
        // should be a better way to do this
        ofEventArgs args;
        referencePoints.draw(args);
        
        // calculating the 3d mesh
        vector<ofVec2f> imagePoints;
        vector<ofVec3f> objectPoints;
        for(int i = 0; i < referencePoints.size(); i++) {
            DraggablePoint& cur = referencePoints.get(i);
            if(cur.hit) {
                imagePoints.push_back(cur.position);
                objectPoints.push_back(calibrationCornerMesh.getVertex(i));
            }
        }
        
        // should only calculate this when the points are updated
        mapamok.update(ofGetWidth(), ofGetHeight(), imagePoints, objectPoints);
    }
    
    void draw() {
        if(!editCalibration){
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            pbr.makeDepthMap(scene);
            
            ofDisableAlphaBlending();
            ofEnableDepthTest();
            
            glCullFace(GL_FRONT);
            
        }

        ofBackground(backgroundBrightness);
        ofSetColor(255);
        
        // EDITOR
        
        if(editCalibration) {
            
            drawCalibraitonEditor();
            
            if(mapamok.calibrationReady) {
                mapamok.begin();
                ofSetColor(255, 128);
                renderCalibration();
                mapamok.end();
            }
            
        } else {
            
            
            firstPass.begin();
            firstPass.activateAllDrawBuffers();
            ofClear(0);

            if(mapamok.calibrationReady) {
                mapamok.begin();
                //pbr.drawEnvironment(&cam);
                scene();
                mapamok.end();
            } else {
                cam.begin();
                pbr.drawEnvironment(&cam);
                scene();
                cam.end();
            }

            firstPass.end();
            glDisable(GL_CULL_FACE);
            
            ofDisableDepthTest();
            ofEnableAlphaBlending();
            
            // post effect
            
            secondPass.begin();
            ofClear(0);
            tonemap.begin();
            tonemap.setUniformTexture("image", firstPass.getTexture(), 0);
            tonemap.setUniform1f("exposure", exposure);
            tonemap.setUniform1f("gamma", gamma);
            firstPass.draw(0, 0);
            tonemap.end();
            secondPass.end();
            
            fxaa.begin();
            fxaa.setUniformTexture("image", secondPass.getTexture(), 0);
            fxaa.setUniform2f("texel", 1.0 / float(secondPass.getWidth()), 1.0 / float(secondPass.getHeight()));
            secondPass.draw(0, 0);
            fxaa.end();
            
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
            bool guiCalibrationReady = mapamok.calibrationReady;
            ImGui::Checkbox("Ready", &guiCalibrationReady);
            ImGui::Checkbox("Show scales", &showScales);
            ImGui::Checkbox("Fix principal point", &mapamok.bCV_CALIB_FIX_PRINCIPAL_POINT);
            ImGui::Checkbox("Fix aspect ratio", &mapamok.bCV_CALIB_FIX_ASPECT_RATIO);
            ImGui::Checkbox("Fix K1", &mapamok.bCV_CALIB_FIX_K1);
            ImGui::Checkbox("Fix K2", &mapamok.bCV_CALIB_FIX_K2);
            ImGui::Checkbox("Fix K3", &mapamok.bCV_CALIB_FIX_K3);
            ImGui::Checkbox("Zero tangent dist", &mapamok.bCV_CALIB_ZERO_TANGENT_DIST);

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
            if(enableCameraControl)
                cam.enableMouseInput();
            else
                cam.disableMouseInput();
            
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
        
        auto calibrationPrimitive = calibrationModel.getPrimitives();
        vector<ofMesh> meshes = calibrationPrimitive->getBakedMeshesRecursive();
        
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
    
    void resizeFbos(){
        ofFbo::Settings firstPassSettings;
        firstPassSettings = defaultFboSettings;
        firstPassSettings.width = ofGetWidth();
        firstPassSettings.height = ofGetHeight();
        firstPassSettings.internalformat = GL_RGBA32F;
        firstPassSettings.colorFormats.push_back(GL_RGBA32F);
        firstPass.allocate(firstPassSettings);
        
        ofFbo::Settings secondPassSettings;
        secondPassSettings = defaultFboSettings;
        secondPassSettings.width = ofGetWidth();
        secondPassSettings.height = ofGetHeight();
        secondPassSettings.internalformat = GL_RGB;
        secondPassSettings.colorFormats.push_back(GL_RGB);
        secondPass.allocate(secondPassSettings);
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
            mapamok.save("calibrations/test");
        }
        if(key == 'l') {
            mapamok.load("calibrations/test");
        }
    }
    
    void windowResized(int w, int h){
        resizeFbos();
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
