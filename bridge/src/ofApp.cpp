//
//  ofApp.cpp
//  bridge
//
//  Created by ole kristensen on 30/08/2018.
//

#include "ofApp.hpp"
#include "ofxAssimp3dPrimitiveHelpers.hpp"
#include "MeshUtils.hpp"

void ofApp::setupViewPlane() {
    
    ofVec3f viewCornerMin;
    ofVec3f viewCornerMax;
    
    auto viewFboSettings = defaultFboSettings;
    
    getBoundingBox(viewNode->getBakedMesh(), viewCornerMin, viewCornerMax);
    viewPlane.setWidth(fabs(viewCornerMax.z - viewCornerMin.z));
    viewPlane.setHeight(fabs(viewCornerMax.y - viewCornerMin.y));
    viewPlane.setGlobalOrientation(ofQuaternion(90, ofVec3f(0,1,0)));
    viewPlane.setPosition(0, viewPlane.getHeight()/2.0, -viewPlane.getWidth()/2.0);
    
    viewPlane.setParent(world.origin);
    
    viewFboSettings.width = floor(viewPlane.getWidth() * viewResolution);
    viewFboSettings.height = floor(viewPlane.getHeight() * viewResolution);
    viewFbo.allocate(viewFboSettings);
    
    viewCamera.setParent(world.origin);
    
    viewCamera.setScale(1,1,1);
    viewCamera.setNearClip(0.1);
    viewCamera.setFarClip(10000);
    viewCamera.lookAt(toOf(viewPlane.getGlobalPosition()) * toOf(viewPlane.getGlobalTransformMatrix()));
    viewCamera.setupPerspective();
}

void ofApp::setup() {
    
    // WINDOW
    ofSetWindowTitle(title);
    ofSetVerticalSync(true);
    guiFont.load("fonts/OpenSans-Light.ttf", 16);
    
    // MODELS
    
    spaceModelPrimitive = nullptr;
    
    if(ofFile::doesFileExist("models/space.dae")) {
        loadSpaceModel("models/space.dae");
    }
    
    fullModelPrimitive = nullptr;
    
    if(ofFile::doesFileExist("models/full.dae")) {
        loadFullModel("models/full.dae");
    }
    
    // FBOs
    
    defaultFboSettings.textureTarget = GL_TEXTURE_2D;
    defaultFboSettings.useDepth = true;
    defaultFboSettings.depthStencilAsTexture = true;
    defaultFboSettings.useStencil = true;
    defaultFboSettings.minFilter = GL_NEAREST;
    defaultFboSettings.maxFilter = GL_NEAREST;
    defaultFboSettings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
    defaultFboSettings.wrapModeVertical = GL_CLAMP_TO_EDGE;
    
    setupViewPlane();
    
    // SHADERS
    
    string shaderPath = "shaders/postEffect/";
    tonemap.loadAuto(shaderPath + "tonemap");
    fxaa.loadAuto(shaderPath + "fxaa");
    shader.loadAuto("shaders/shader");
    shader.bindDefaults();
    
    // PROJECTORS
    
    mViewPort = ofRectangle(0, 0, projectionResolution.x, projectionResolution.y);
    
    mProjectorPerspective = make_shared<Projector>(ofRectangle(0,projectionResolution.y,ofGetWidth(), ofGetHeight()-projectionResolution.y),
                                                   defaultFboSettings, shader, tonemap, fxaa);
    mProjectors.insert(make_pair("perspective", mProjectorPerspective));
    
    mProjectorLeft = make_shared<Projector>(ofRectangle(0, 0, projectionResolution.x, projectionResolution.y),
                                            defaultFboSettings, shader, tonemap, fxaa);
    mProjectors.insert(make_pair("left", mProjectorLeft));
    
    mProjectorRight = make_shared<Projector>(ofRectangle(projectionResolution.x, 0, projectionResolution.x, projectionResolution.y),
                                             defaultFboSettings, shader, tonemap, fxaa);
    
    mProjectors.insert(make_pair("right", mProjectorRight));
    
    for(auto projector : mProjectors){
        projector.second->referencePoints.enableControlEvents();
        //projector.second->referencePoints.enableDrawEvent();
        projector.second->cam.setDistance(10);
        projector.second->cam.setNearClip(.1);
        projector.second->cam.setFarClip(10000);
        projector.second->cam.setVFlip(false);
        projector.second->referencePoints.setCamera(&projector.second->cam);
        projector.second->pg.setName(projector.first);
        projector.second->pg.add(projector.second->pgCalibration);
        pgProjectors.add(projector.second->pg);
    }
    pgProjectors.setName("Projectors");
    pgGlobal.add(pgProjectors);
    
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
    
    cubeMap.load("ofxPBRAssets/cubemaps/DH-AO-06.hdr", 1024, true, "ofxPBRAssets/cubemapCache");
    cubeMap.setEnvLevel(0.2);
    
    pbr.setCubeMap(&cubeMap);
    pbr.setDrawEnvironment(true);
    
    // SCENES
    
    for( auto s : scenes) {
        s->setupScene(&pgGlobal, &world, &scenes);
    }
    
    gui.setup();
    
    
}

void ofApp::update() {
    cubeMap.setEnvLevel(pPbrEnvLevel);
    cubeMap.setExposure(pPbrEnvExposure);
    cubeMap.setRotation(pPbrEnvRotation);
    
    spaceModel.update();
    fullModel.update();
    if(fullModelPrimitive != nullptr){
        fullModelPrimitive->update();
    }
    for (auto projector : mProjectors){
        projector.second->update(calibrationCornerMesh);
        if(projector.first == "perspective"){
            projector.second->referencePoints.disableDrawEvent();
            projector.second->referencePoints.disableControlEvents();
        }
    }
    tonemap.begin();
    tonemap.setUniform1f("time", ofGetElapsedTimef());
    tonemap.setUniform1f("exposure", pPbrExposure);
    tonemap.setUniform1f("gamma", pPbrGamma);
    tonemap.end();
}

void ofApp::renderScene() {
    
    ofEnableDepthTest();
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    pbr.beginDefaultRenderer();
    {
        ofSetColor(255,255);
        renderPrimitiveWithMaterialsRecursive(fullModelPrimitive, materials, pbr);
    }
    pbr.endDefaultRenderer();
    glDisable(GL_CULL_FACE);
    ofDisableDepthTest();
}

void ofApp::drawProjectorLeft(ofEventArgs & args) {
    drawProjection(mProjectorLeft);
}

void ofApp::drawProjectorRight(ofEventArgs & args) {
    drawProjection(mProjectorRight);
}

void ofApp::drawProjection(shared_ptr<Projector> & projector) {
    
    ofPushStyle();
    ofSetColor(255);
    
    glDisable(GL_CULL_FACE);
    ofEnableArbTex();
    ofEnableAlphaBlending();
    projector->output.draw(ofGetCurrentViewport());
    ofPopStyle();
    
}

void ofApp::drawView() {
    ofPushStyle();
    ofPushView();
    ofPushMatrix();{
        glDisable(GL_CULL_FACE);
        
        viewCamera.setGlobalPosition(3, 2, -2+sin(ofGetElapsedTimef()));
        float width = viewPlane.getWidth();
        float height = viewPlane.getHeight();
        ofVec3f windowTopLeft(0.0,
                              height,
                              0.0);
        //windowTopLeft = windowTopLeft * viewPlane.getLocalTransformMatrix();
        ofVec3f windowBottomLeft(0.0,
                                 0.0,
                                 0.0f);
        //windowBottomLeft = windowBottomLeft * viewPlane.getLocalTransformMatrix();
        ofVec3f  windowBottomRight(0.0,
                                   0.0,
                                   -width);
        //windowBottomRight = windowBottomRight * viewPlane.getLocalTransformMatrix();
        // To setup off axis view portal we need to be in the plane's matrix. All window coordinates of the camera are relative to the plane.
        viewPlane.ofPlanePrimitive::transformGL();
        viewCamera.setupOffAxisViewPortal(windowTopLeft , windowBottomLeft, windowBottomRight );
        viewPlane.ofPlanePrimitive::restoreTransformGL();
        viewCamera.setFarClip(10000);
        viewCamera.setNearClip(.1);
        viewCamera.setVFlip(false);
        
        viewFbo.begin(); {
            viewFbo.activateAllDrawBuffers();
            ofClear(0, 255);
            viewCamera.begin();
            ofSetColor(255,0,255, 64);
            fullModelPrimitive->recursiveDraw();
            viewCamera.end();
        } viewFbo.end();
        
    } ofPopMatrix();
    ofPopView();
    ofPopStyle();
}

void ofApp::draw() {
    
    ofBackground(0);
    ofSetColor(255);
    glCullFace(GL_FRONT);
    glDisable(GL_CULL_FACE);
    
    drawView();
    
    for (auto projector : mProjectors){
        
        projector.second->renderCalibrationEditor(spaceModelPrimitive);
        
        if(!projector.second->pCalibrationEdit){
            
            pbr.updateDepthMaps();
            
            projector.second->begin(true);
            
            ofDisableAlphaBlending();
            ofEnableDepthTest();
            
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            
            cam = &projector.second->getCam();
            pbr.setCamera(cam);
            pbr.renderScene();
            
            glDisable(GL_CULL_FACE);
            
            projector.second->end();
            
        }
        
        projector.second->draw();
        
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
    
    
    // Gui
    this->mouseOverGui = false;
    if (this->guiVisible)
    {
        this->mouseOverGui = this->imGui();
    }
    if (this->mouseOverGui)
    {
        for(auto projector : mProjectors){
            projector.second->cam.disableMouseInput();
        }
    }
    else
    {
        for(auto projector : mProjectors){
            projector.second->cam.enableMouseInput();
        }
    }
}

void ofApp::loadSpaceModel(string filename) {
    
    ofSetLogLevel("ofxAssimpModelLoader", OF_LOG_NOTICE);
    
    spaceModel.loadModel(filename, false, false);
    
    // rotate the model to match the ofMeshes we get later...
    spaceModel.setRotation(0, 180, 0, 0, 1.0);
    
    // make sure to load to scale
    spaceModel.setScaleNormalization(false);
    spaceModel.calculateDimensions();
    
    spaceModelPrimitive = spaceModel.getPrimitives();
    spaceModelPrimitive->setParent(world.origin);
    vector<ofMesh> meshes = spaceModelPrimitive->getBakedMeshesRecursive();
    
    auto calibrationPrimitive = spaceModelPrimitive->getFirstPrimitiveWithTextureNameContaining("room.calibration");
    
    wallModelNode = spaceModelPrimitive->getFirstPrimitiveWithTextureNameContaining("room.walls");
    approachModelNode = spaceModelPrimitive->getFirstPrimitiveWithTextureNameContaining("approach.deck");
    trussModelNode = spaceModelPrimitive->getFirstPrimitiveWithTextureNameContaining("room.truss");
    viewNode = spaceModelPrimitive->getFirstPrimitiveWithTextureNameContaining("room.view");
    
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

void ofApp::loadFullModel(string filename) {
    
    ofSetLogLevel("ofxAssimpModelLoader", OF_LOG_NOTICE);
    // load model, optimize and pretransform all vertices in the global space.
    fullModel.loadModel(filename, false, false);
    
    // rotate the model to match the ofMeshes we get later...
    fullModel.setRotation(0, 180, 0, 0, 1.0);
    
    // make sure to load to scale
    fullModel.setScaleNormalization(false);
    fullModel.calculateDimensions();
    
    fullModelPrimitive = fullModel.getPrimitives();
    fullModelPrimitive->setParent(world.origin);
    
    
}

void ofApp::save(string name){
    ofJson j;
    ofSerialize(j, pgGlobal);
    ofSaveJson("settings/" + name + ".json", j);
}

void ofApp::load(string name){
    ofJson j = ofLoadJson("settings/" + name + ".json");
    ofDeserialize(j, pgGlobal);
}

void ofApp::keyPressed(int key) {
    if(key == 'f') {
        ofToggleFullscreen();
    }
    if(key == '\t') {
        guiVisible = !guiVisible;
    }
}

void ofApp::windowResized(int w, int h){
    mProjectors.at("perspective")->viewPort.set(0, projectionResolution.y, w, h-projectionResolution.y);
    mProjectors.at("perspective")->resizeFbos();
}

bool ofApp::imGui()
{
    auto mainSettings = ofxImGui::Settings();
    
    this->gui.begin();
    {
        
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoCollapse;
        
        if (ofxImGui::BeginWindow(title.c_str(), mainSettings, window_flags)){
            
            ImGui::TextUnformatted("den frie vilje");
            ImGui::Text("FPS %.3f", ofGetFrameRate());
            
            ImGui::Separator();
            
            //            ImGui::Checkbox("Fix principal point", &mapamok.bCV_CALIB_FIX_PRINCIPAL_POINT);
            //            ImGui::Checkbox("Fix aspect ratio", &mapamok.bCV_CALIB_FIX_ASPECT_RATIO);
            //            ImGui::Checkbox("Fix K1", &mapamok.bCV_CALIB_FIX_K1);
            //            ImGui::Checkbox("Fix K2", &mapamok.bCV_CALIB_FIX_K2);
            //            ImGui::Checkbox("Fix K3", &mapamok.bCV_CALIB_FIX_K3);
            //            ImGui::Checkbox("Zero tangent dist", &mapamok.bCV_CALIB_ZERO_TANGENT_DIST);
            
            if(ImGui::Button("Load")){
                load("default");
            } ImGui::SameLine();
            if(ImGui::Button("Save")){
                save("default");
            }
            
            ImGui::Checkbox("Use Shader", &shaderToggle);
            bool guiShaderValid = shader.isValid;
            ImGui::Checkbox("Shader Valid", &guiShaderValid);
            
            if(ImGui::SliderFloat("Resolution", &viewResolution, 1.0, 300.0)){
                setupViewPlane();
            }
            
            //ImGui::PushFont(font1);
            ImGui::TextUnformatted("Interface");
            //ImGui::PopFont();
            
            ImGui::Separator();
            
            static bool guiShowTest;
            ImGui::Checkbox("Show Test Window", &guiShowTest);
            if(guiShowTest)
                ImGui::ShowTestWindow();
            
            for(auto & projector : mProjectors){
                
                if(ofxImGui::BeginTree(projector.first.c_str(), mainSettings)){
                    auto p = projector.second;
                    
                    ofxImGui::AddParameter(p->pCalibrationEdit);
                    ofxImGui::AddParameter(p->pCalibrationDrawScales);
                    ofxImGui::AddCombo(p->pCalibrationMeshDrawMode, p->CalibrationMeshDrawModeLabels);
                    ofxImGui::AddCombo(p->pCalibrationMeshColorMode, p->CalibrationMeshColorModeLabels);
                    
                    vector<string> objectNames;
                    for(auto str :spaceModelPrimitive->textureNames){
                        string objectName = ofSplitString(str, "/").back();
                        auto objectNameComponents = ofSplitString(objectName, ".");
                        objectNameComponents.pop_back();
                        objectName = ofJoinString(objectNameComponents, ".");
                        objectNames.push_back(objectName);
                    }
                    
                    ofxImGui::AddCombo(p->pCalibrationHighlightIndex, objectNames);
                    
                    if(ImGui::Button("Load")){
                        projector.second->load("calibrations/" + projector.first);
                    } ImGui::SameLine();
                    if(ImGui::Button("Save")){
                        projector.second->save("calibrations/" + projector.first);
                    }ImGui::SameLine();
                    if(ImGui::Button("Clear")){
                        projector.second->referencePoints.clear();
                    }
                    
                    ofxImGui::EndTree(mainSettings);
                }
                
            }
            
            ofxImGui::AddGroup(pgPbr, mainSettings);
            
        }
        
        
    }
    ofxImGui::EndWindow(mainSettings);
    
    this->gui.end();
    
    return mainSettings.mouseOverGui;
}
