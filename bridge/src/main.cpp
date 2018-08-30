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
//#include "ofxPBRHelper.h"

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
    int textureIndexForSpaceModel = 0;
    
    // MODELS
    ofxAssimpModelLoader spaceModel;
    ofxAssimpModelLoader fullModel;
    ofxAssimp3dPrimitive * fullModelPrimitive = nullptr;
    ofxAssimp3dPrimitive * spaceModelPrimitive = nullptr;
    ofxAssimp3dPrimitive * wallModelNode = nullptr;
    ofxAssimp3dPrimitive * trussModelNode = nullptr;
    ofxAssimp3dPrimitive * pylonsModelNode = nullptr;
    ofxAssimp3dPrimitive * deckModelNode = nullptr;
    
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
    
    //ofxPBRHelper pbrHelper;
    
    ofAutoShader shader, tonemap, fxaa;
    float exposure = 1.0;
    float gamma = 2.2;
    
    function<void()> scene;
    
    void allocateViewFbo() {
        auto viewFboSettings = defaultFboSettings;
        viewRectangle.x = 0;
        viewRectangle.y = 0;
        ofVec3f viewCornerMin;
        ofVec3f viewCornerMax;
        getBoundingBox(viewNode->getBakedMesh(), viewCornerMin, viewCornerMax);
        viewRectangle.width =fabs(viewCornerMax.z - viewCornerMin.z);
        viewRectangle.height =fabs(viewCornerMax.y - viewCornerMin.y);
        viewFboSettings.width = round(fabs(viewRectangle.width) * viewResolution);
        viewFboSettings.height = round(fabs(viewRectangle.height) * viewResolution);
        viewFbo.allocate(viewFboSettings);
    }
    
    void setup() {
        
        // WINDOW
        ofSetWindowTitle(title);
        ofSetVerticalSync(true);
        //ofDisableArbTex();
        guiFont.load("fonts/OpenSans-Light.ttf", 16);

        // MODELS
        
        spaceModelPrimitive = nullptr;

        if(ofFile::doesFileExist("models/space.dae")) {
            loadSpaceModel("models/space.dae");
        } else if(ofFile::doesFileExist("models/space.3ds")) {
            loadSpaceModel("models/space.3ds");
        }
        
        fullModelPrimitive = nullptr;
        
        if(ofFile::doesFileExist("models/full.dae")) {
            loadFullModel("models/full.dae");
        } else if(ofFile::doesFileExist("models/full.3ds")) {
            loadFullModel("models/full.3ds");
        }
        
        // FBOs
        
        defaultFboSettings.textureTarget = GL_TEXTURE_2D;
        defaultFboSettings.useDepth = true;
        defaultFboSettings.depthStencilAsTexture = true;
        defaultFboSettings.useStencil = true;
        defaultFboSettings.minFilter = GL_LINEAR;
        defaultFboSettings.maxFilter = GL_LINEAR;
        defaultFboSettings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
        defaultFboSettings.wrapModeVertical = GL_CLAMP_TO_EDGE;
        
        allocateViewFbo();

        // PROJECTORS
        
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

        for(auto projector : mProjectors){
            projector.second->referencePoints.enableControlEvents();
            projector.second->referencePoints.enableDrawEvent();
            projector.second->cam.setDistance(10);
            projector.second->cam.setNearClip(.1);
            projector.second->cam.setFarClip(10000);
            projector.second->cam.setVFlip(false);
        }

        string shaderPath = "shaders/postEffect/";
        tonemap.loadAuto(shaderPath + "tonemap");
        fxaa.loadAuto(shaderPath + "fxaa");
        shader.loadAuto("shaders/shader");
        shader.bindDefaults();
        
        //PBR
        
        cam = &mProjectorLeft->cam;
        
        scene = bind(&ofApp::renderScene, this);
        
        pbr.setup(scene, cam, 1024*4);
        
        materials.resize(fullModelPrimitive->textureNames.size());
        
        int materialIndex = 0;
        for (auto & material : materials){
            material.baseColor.set(0.95, 1.0, 1.0);
            material.metallic = ofMap(materialIndex++, 0, materials.size(), 0.0, 1.0);
            material.roughness = 0.2;
        }

        cubeMap.load("ofxPBRAssets/panoramas/Barce_Rooftop_C_3k.hdr", 1024, true, "ofxPBRAssets/cubemapCache");
        cubeMap.setEnvLevel(0.1);
        
        pbr.setCubeMap(&cubeMap);
        pbr.setDrawEnvironment(true);

        setupGui();
        
    }
    
    enum {
        RENDER_MODE_FACES = 0,
        RENDER_MODE_OUTLINE,
        RENDER_MODE_WIREFRAME_FULL,
        RENDER_MODE_WIREFRAME_OCCLUDED
    };
    
    void update() {
        spaceModel.update();
        fullModel.update();
        if(fullModelPrimitive != nullptr){
            fullModelPrimitive->update();
        }
        
    }
    
    void renderPrimitiveWithMaterialsRecursive(ofxAssimp3dPrimitive* primitive, vector<ofxPBRMaterial> & materials, ofPolyRenderMode renderType = OF_MESH_FILL){
        if(primitive->textureIndex >= 0) materials[primitive->textureIndex].begin(&pbr);
        primitive->of3dPrimitive::draw(renderType);
        if(primitive->textureIndex >= 0) materials[primitive->textureIndex].end();
        for(auto c : primitive->getChildren()) {
            renderPrimitiveWithMaterialsRecursive(c, materials, renderType);
        }
    }
    
    ofxAssimp3dPrimitive * getFirstPrimitiveWithTextureIndex(ofxAssimp3dPrimitive* primitive, int textureIndex){
        if(primitive->textureIndex == textureIndex)
            return primitive;
        else {
            for(auto c : primitive->getChildren()) {
                ofxAssimp3dPrimitive * retP = getFirstPrimitiveWithTextureIndex(c, textureIndex);
                if(retP != nullptr){
                    return retP;
                }
            }
        }
        return nullptr;
    }

    vector<ofxAssimp3dPrimitive*> getPrimitivesWithTextureIndex(ofxAssimp3dPrimitive* primitive, int textureIndex){
        vector<ofxAssimp3dPrimitive*> retVec;
        if(primitive->textureIndex == textureIndex){
            retVec.push_back(primitive);
            return retVec;
        } else {
            for(auto c : primitive->getChildren()) {
                auto childVec = getPrimitivesWithTextureIndex(c, textureIndex);
                if(childVec.size() > 0){
                    for(auto child : childVec){
                        retVec.push_back(child);
                    }
                }
            }
        }
        return retVec;
    }

    ofxAssimp3dPrimitive * getFirstPrimitiveWithTextureNameContaining(ofxAssimp3dPrimitive* primitive, string str){
        int textureIndex = 0;
        for(auto name : primitive->textureNames){
            if(ofStringTimesInString(name, str) > 0){
                break;
            }
            textureIndex++;
        }
        
        if(primitive->textureIndex == textureIndex)
            return primitive;
        else {
            for(auto c : primitive->getChildren()) {
                ofxAssimp3dPrimitive * retP = getFirstPrimitiveWithTextureIndex(c, textureIndex);
                if(retP != nullptr){
                    return retP;
                }
            }
        }
        return nullptr;
    }

    void renderScene() {
        
        
        ofEnableDepthTest();
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        pbr.beginDefaultRenderer();
        {
            ofSetColor(255,255);
            renderPrimitiveWithMaterialsRecursive(fullModelPrimitive, materials);
        }
        pbr.endDefaultRenderer();
        glDisable(GL_CULL_FACE);
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
            spaceModelPrimitive->recursiveDraw(OF_MESH_FILL);
            if(shaderToggle) shader.end();
            ofDisableDepthTest();
        } else if(renderModeSelection == RENDER_MODE_WIREFRAME_FULL) {
            if(shaderToggle) shader.begin();
            spaceModelPrimitive->recursiveDraw(OF_MESH_WIREFRAME);
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
            spaceModelPrimitive->recursiveDraw(OF_MESH_FILL);
            glColorMask(true, true, true, true);
            glDisable(GL_POLYGON_OFFSET_FILL);
            spaceModelPrimitive->recursiveDraw(OF_MESH_WIREFRAME);
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
            spaceModelPrimitive->recursiveDraw();
            ofEnableDepthTest();

            ofPushMatrix();
            //viewNode->transformGL();
            ofScale((viewRectangle.getHeight()/viewFbo.getHeight()));
            ofRotateYDeg(90.0);
            viewFbo.draw(0,0);
            //viewNode->restoreTransformGL();
            ofPopMatrix();

            for(int mIndex = 0 ; mIndex < spaceModelPrimitive->textureNames.size(); mIndex++){
                ofColor c;
                c.setHsb(360.0*mIndex/spaceModelPrimitive->textureNames.size(), 255, 255);
                ofPushStyle();
                ofSetColor(c);
                auto primitives = getPrimitivesWithTextureIndex(spaceModelPrimitive, mIndex);
                for(auto p : primitives){
                    if(mIndex == textureIndexForSpaceModel){
                        shader.begin();
                        shader.setUniform1f("elapsedTime", ofGetElapsedTimef());
                    }
                    p->recursiveDraw();
                    if(mIndex == textureIndexForSpaceModel){
                        shader.end();
                    }
                }
                ofPopStyle();
            }
            
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
        
        
        viewFbo.begin();
        ofScale(1.0/viewFbo.getWidth());
        ofClear(0,128,255, 255);
        ofPushStyle();
        ofPushMatrix();
        ofScale(1.0, guiFont.getSize());
        guiFont.drawString("View", 0.10, 0.10);
        ofPopMatrix();
        ofPopStyle();
        viewFbo.end();
        
        ofBackground(0);
        
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
            
            
            pbr.updateDepthMaps();
            
            for (auto projector : mProjectors){
            
                projector.second->referencePoints.disableControlEvents();
                projector.second->referencePoints.disableDrawEvent();

                ofDisableAlphaBlending();
                ofEnableDepthTest();

                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);

                projector.second->renderPass.begin();
                projector.second->renderPass.activateAllDrawBuffers();
                ofClear(0);
                
                if(projector.second->mapamok.calibrationReady){
                    projector.second->mapamok.begin(projector.second->viewPort - projector.second->viewPort.getPosition());
                    cam = &projector.second->mapamok.cam;
                    pbr.setCamera(cam);
                    pbr.renderScene();
                    projector.second->mapamok.end();
                } else {
                    projector.second->cam.begin(projector.second->viewPort - projector.second->viewPort.getPosition());
                    cam = &projector.second->cam;
                    pbr.setCamera(cam);
                    pbr.renderScene();
                    projector.second->cam.end();
                }
                
                projector.second->renderPass.end();
                
                glDisable(GL_CULL_FACE);

                ofDisableDepthTest();
                ofEnableAlphaBlending();
                
                // post effect
                projector.second->tonemapPass.begin();
                ofClear(0);
                tonemap.begin();
                tonemap.setUniformTexture("image", projector.second->renderPass.getTexture(), 0);
                tonemap.setUniform1f("exposure", exposure);
                tonemap.setUniform1f("gamma", gamma);
                projector.second->renderPass.draw(0, 0);
                tonemap.end();
                projector.second->tonemapPass.end();
                
                projector.second->output.begin();
                fxaa.begin();
                fxaa.setUniformTexture("image", projector.second->tonemapPass.getTexture(), 0);
                fxaa.setUniform2f("texel", 1.25 / float(projector.second->tonemapPass.getWidth()), 1.25 / float(projector.second->tonemapPass.getHeight()));
                projector.second->tonemapPass.draw(0,0);
                fxaa.end();

                projector.second->output.end();

                ofPushStyle();
                projector.second->output.draw(projector.second->viewPort);
                ofPopStyle();
            }
            
        }
        
        // draw frames and labels
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
            
            for(auto & projector : mProjectors){
                ImGui::TextUnformatted(projector.first.c_str());
                if(ImGui::Button("Load")){
                    projector.second->mapamok.load("calibrations/" + projector.first);
                } ImGui::SameLine();
                if(ImGui::Button("Save")){
                    projector.second->mapamok.save("calibrations/" + projector.first);
                }
            }
            
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
            
            ImGui::SliderInt("Texture", &textureIndexForSpaceModel, 0, spaceModelPrimitive->textureNames.size()-1);
            ImGui::TextUnformatted(ofSplitString(spaceModelPrimitive->textureNames[textureIndexForSpaceModel], "/").back().c_str());
            
            const char* guiRenderModeSelectionItems[] = { "Faces", "Outline", "Wireframe Full", "Wireframe Occluded" };
            ImGui::Combo("Render Mode", &renderModeSelection, guiRenderModeSelectionItems, IM_ARRAYSIZE(guiRenderModeSelectionItems));
            
            if(ImGui::SliderFloat("Resolution", &viewResolution, 1.0, 300.0)){
                allocateViewFbo();
            }
            
            ImGui::PushFont(ImGuiIO().Fonts->Fonts[1]);
            ImGui::TextUnformatted("Interface");
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
            
            static bool guiShowTest;
            ImGui::Checkbox("Show Test Window", &guiShowTest);
            if(guiShowTest)
                ImGui::ShowTestWindow();
            ImGui::Checkbox("Show PBR Helper", &showPBRHelper);
            
            ImGui::End();
            
            if(showPBRHelper){
                ImGui::Begin("ofxPBRHelper");
                ImGui::DragFloat("exposure", &exposure, 0.1);
                ImGui::DragFloat("gamma", &gamma, 0.1);
                //pbrHelper.drawGui();
                ImGui::End();
            }
  
            gui.end();
        }
        
    }
    
    void loadSpaceModel(string filename) {
        
        ofSetLogLevel("ofxAssimpModelLoader", OF_LOG_NOTICE);
        
        spaceModel.loadModel(filename, false, false);
        
        // rotate the model to match the ofMeshes we get later...
        spaceModel.setRotation(0, 180, 0, 0, 1.0);
        
        // make sure to load to scale
        spaceModel.setScaleNormalization(false);
        spaceModel.calculateDimensions();
        
        spaceModelPrimitive = spaceModel.getPrimitives();
        vector<ofMesh> meshes = spaceModelPrimitive->getBakedMeshesRecursive();
        
        auto calibrationPrimitive = getFirstPrimitiveWithTextureNameContaining(spaceModelPrimitive, "room.calibration");
        
        wallModelNode = getFirstPrimitiveWithTextureNameContaining(spaceModelPrimitive, "room.walls");
        deckModelNode = getFirstPrimitiveWithTextureNameContaining(spaceModelPrimitive, "approach.deck");
        trussModelNode = getFirstPrimitiveWithTextureNameContaining(spaceModelPrimitive, "room.truss");
        viewNode = getFirstPrimitiveWithTextureNameContaining(spaceModelPrimitive, "room.view");
        
        meshes = calibrationPrimitive->getBakedMeshesRecursive();
        
        calibrationPrimitive->clearParent();
        
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
    
    void loadFullModel(string filename) {
        
        ofSetLogLevel("ofxAssimpModelLoader", OF_LOG_NOTICE);
        // load model, optimize and pretransform all vertices in the global space.
        fullModel.loadModel(filename, false, false);
        
        // rotate the model to match the ofMeshes we get later...
        fullModel.setRotation(0, 180, 0, 0, 1.0);
        
        // make sure to load to scale
        fullModel.setScaleNormalization(false);
        fullModel.calculateDimensions();
        
        fullModelPrimitive = fullModel.getPrimitives();
        
    }
    
    void dragEvent(ofDragInfo dragInfo) {
        if(dragInfo.files.size() == 1) {
            string filename = dragInfo.files[0];
            loadSpaceModel(filename);
        }
    }
    
    void keyPressed(int key) {
        if(key == 'f') {
            ofToggleFullscreen();
        }
        if(key == '\t') {
            guiToggle = !guiToggle;
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
