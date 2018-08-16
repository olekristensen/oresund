//TODO:

// load model segmented
// add fbo walls
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
#include "ofxUI.h"
#include "ofxImGui.h"

#include "Mapamok.h"
#include "DraggablePoints.h"
#include "MeshUtils.h"
#include "ofAutoshader.h"

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
    ofxImGui::Gui gui;
    bool guiToggle = true;
    int guiColumnWidth = 250;
    
    of3dPrimitive rootNode;

    string title = "Öresund bridge";
    
    Mapamok mapamok;
    ofxAssimpModelLoader model;

    const float cornerRatio = .1;
    const int cornerMinimum = 6;
    const float mergeTolerance = .01;
    const float selectionMergeTolerance = .01;
    
    bool editToggle = true;
    bool recalculateToggle = false;
    bool loadButton = false;
    bool saveButton = false;
    float backgroundBrightness = 0;
    bool shaderToggle = false;
    int renderModeSelection = 0;
    
    bool modelToggle = false;
    bool worldToggle = false;
    
    ofVboMesh mesh, cornerMesh;
    ofEasyCam cam;
    ReferencePoints referencePoints;
    
    ofAutoShader shader;
    
    void setup() {
        ofSetWindowTitle(title);
        ofSetVerticalSync(true);
        setupGui();
        if(ofFile::doesFileExist("models/model.dae")) {
            loadModel("models/model.dae");
        } else if(ofFile::doesFileExist("models/model.3ds")) {
            loadModel("models/model.3ds");
        }
        cam.setDistance(100);
        cam.setNearClip(.01);
        cam.setFarClip(10000);

        referencePoints.setClickRadius(8);
        referencePoints.enableControlEvents();
        shader.loadAuto("shaders/shader");
        
    }
    
    enum {
        RENDER_MODE_FACES = 0,
        RENDER_MODE_OUTLINE,
        RENDER_MODE_WIREFRAME_FULL,
        RENDER_MODE_WIREFRAME_OCCLUDED
    };
    
    void update() {
        model.update();
    }
    
    void render() {
        
        if(shaderToggle){
            shader.begin();
            shader.setUniform1f("elapsedTime", ofGetElapsedTimef());
            shader.end();
        }
        
        if(renderModeSelection == RENDER_MODE_FACES) {
            ofEnableDepthTest();
            ofSetColor(255, 128);
            if(shaderToggle) shader.begin();
            mesh.drawFaces();
            if(shaderToggle) shader.end();
            ofDisableDepthTest();
        } else if(renderModeSelection == RENDER_MODE_WIREFRAME_FULL) {
            if(shaderToggle) shader.begin();
            mesh.drawWireframe();
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
            mesh.drawFaces();
            glColorMask(true, true, true, true);
            glDisable(GL_POLYGON_OFFSET_FILL);
            mesh.drawWireframe();
            prepareRender(false, false, false);
            if(shaderToggle) shader.end();
        }
    }
    
    void drawEdit() {
        cam.begin();
        ofPushStyle();
        ofSetColor(255, 128);
        mesh.drawFaces();
        ofPopStyle();
        cam.end();
        
        ofMesh cornerMeshImage = cornerMesh;
        // should only update this if necessary
        project(cornerMeshImage, cam, ofGetWindowRect());
        
        // if this is a new mesh, create the points
        // should use a "dirty" flag instead allowing us to reset manually
        if(cornerMesh.getNumVertices() != referencePoints.size()) {
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
                objectPoints.push_back(cornerMesh.getVertex(i));
            }
        }
        
        // should only calculate this when the points are updated
        mapamok.update(ofGetWidth(), ofGetHeight(), imagePoints, objectPoints);
    }
    
    void draw() {
        ofBackground(backgroundBrightness);
        ofSetColor(255);
        
        if(worldToggle) {
            cam.begin();
            ofPushStyle();
            ofSetColor(255,0,255,64);
            ofDrawGrid(1.0, 10, true);
            ofDrawAxis(1.0);
            ofPopStyle();
            cam.end();
        }
        
        // EDITOR
        
        if(editToggle) {
            drawEdit();
        }
        
        if(modelToggle) {
            
            cam.begin();
            ofPushStyle();
            ofSetColor(255,255,0,128);
            model.drawFaces();
            ofPopStyle();
            cam.end();

        }
        
        // MAPAMOK
        
        if(mapamok.calibrationReady) {
            mapamok.begin();
            if(editToggle) {
                ofSetColor(255, 128);
            } else {
                ofSetColor(255);
            }
            render();
            mapamok.end();
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
            
            ImGui::Checkbox("Edit", &editToggle);
            if(editToggle){
                ImGui::SameLine();
                static bool guiCameraMoves;
                ImGui::Checkbox("Camera", &guiCameraMoves);
                if(guiCameraMoves)
                    cam.enableMouseInput();
                else
                    cam.disableMouseInput();
            } else {
                cam.disableMouseInput();
            }
            bool guiCalibrationReady = mapamok.calibrationReady;
            ImGui::Checkbox("Ready", &guiCalibrationReady);
            
            ImGui::PushFont(ImGuiIO().Fonts->Fonts[1]);
            ImGui::TextUnformatted("Rendering");
            ImGui::PopFont();
            
            ImGui::Separator();
            
            ImGui::Checkbox("Use Shader", &shaderToggle);

            bool guiShaderValid = shader.isValid;
            ImGui::Checkbox("Shader Valid", &guiShaderValid);

            const char* guiRenderModeSelectionItems[] = { "Faces", "Outline", "Wireframe Full", "Wireframe Occluded" };
            ImGui::Combo("Render Mode", &renderModeSelection, guiRenderModeSelectionItems, IM_ARRAYSIZE(guiRenderModeSelectionItems));
            
            ImGui::PushFont(ImGuiIO().Fonts->Fonts[1]);
            ImGui::TextUnformatted("Model");
            ImGui::PopFont();
            
            ImGui::Separator();

            ImGui::Checkbox("Show world", &worldToggle);
            ImGui::Checkbox("Show model", &modelToggle);

            ImGui::PushFont(ImGuiIO().Fonts->Fonts[1]);
            ImGui::TextUnformatted("Interface");
            ImGui::PopFont();
            
            ImGui::Separator();
            static bool guiShowTest;
            ImGui::Checkbox("Show Test Window", &guiShowTest);
            if(guiShowTest)
                ImGui::ShowTestWindow();
            
            ImGui::End();
            
            gui.end();
        }
        
    }
    
    void loadModel(string filename) {
        
        // load model, optimize and pretransform all vertices in the global space.
        model.loadModel(filename, true, true);

        // rotate the model to match the ofMeshes we get later...
        model.setRotation(0, 180, 0, 0, 1.0);

        // make sure to load to scale
        model.setScaleNormalization(false);
        model.calculateDimensions();
        
        vector<ofMesh> meshes = getMeshes(model);
        
        auto names = model.getMeshNames();
        for (auto name : names){
            ofLogNotice("mesh name") << name << endl;
        }
        
        // join all the meshes
        mesh = ofVboMesh();
        mesh = joinMeshes(meshes);
        ofVec3f cornerMin, cornerMax;
        getBoundingBox(mesh, cornerMin, cornerMax);
        
        // merge
        // another good metric for finding corners is if there is a single vertex touching
        // the wall of the bounding box, that point is a good control point
        
        cornerMesh = ofVboMesh();
        for(int i = 0; i < meshes.size(); i++) {
            ofMesh mergedMesh = mergeNearbyVertices(meshes[i], mergeTolerance);
            vector<unsigned int> cornerIndices = getRankedCorners(mergedMesh);
            int n = cornerIndices.size() * cornerRatio;
            n = MIN(MAX(n, cornerMinimum), cornerIndices.size());
            for(int j = 0; j < n; j++) {
                int index = cornerIndices[j];
                const ofVec3f& corner = mergedMesh.getVertices()[index];
                cornerMesh.addVertex(corner);
            }
        }
        cornerMesh = mergeNearbyVertices(cornerMesh, selectionMergeTolerance);
        cornerMesh.setMode(OF_PRIMITIVE_POINTS);
    }
    
    void dragEvent(ofDragInfo dragInfo) {
        if(dragInfo.files.size() == 1) {
            string filename = dragInfo.files[0];
            loadModel(filename);
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
    ofAppGLFWWindow window;
    ofSetupOpenGL(&window, 1280, 720, OF_WINDOW);
    ofRunApp(new ofApp());
}
