//
//  ofApp.cpp
//  bridge
//
//  Created by ole kristensen on 30/08/2018.
//

#include "ofApp.hpp"
#include "ofxAssimp3dPrimitiveHelpers.hpp"
#include "MeshUtils.hpp"

using namespace ofxChoreograph;

void ofApp::setup() {
    
    // WINDOW
    ofSetWindowTitle(title);
    ofSetVerticalSync(true);
    guiFont.load("fonts/OpenSans-Light.ttf", 16);
    
    // MODELS
    
    if(ofFile::doesFileExist("models/space.dae")) {
        loadSpaceModel("models/space.dae");
    }
        
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
        projector.second->cam.setDistance(10);
        projector.second->cam.setNearClip(.1);
        projector.second->cam.setFarClip(10000);
        projector.second->cam.setVFlip(false);
        projector.second->referencePoints.setCamera(&projector.second->cam);
        projector.second->pg.setName(projector.first);
        projector.second->pg.add(projector.second->pgCalibration);
        if(projector.first == "perspective"){
            projector.second->pg.add(projector.second->pTrackViewCamera);
        }
        pgProjectors.add(projector.second->pg);
    }
    pgProjectors.setName("Projectors");
    pgGlobal.add(pgProjectors);
    
    //VIEW PLANE
    
    mViewPlane = make_shared<ViewPlane>(defaultFboSettings, shader, tonemap, fxaa, world.primitives["view"], world);
    
    pgGlobal.add(mViewPlane->pg);
    
    //PBR
    
    cam = &mProjectorLeft->cam;
    
    scene = bind(&ofApp::renderScene, this);
    
    pbr.setup(scene, cam, 1024*4);
    
    materials.resize(world.primitives["model"]->textureNames.size());
    
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
    
    load("default");
    
    outputs["envExposure"]().makeReferenceTo(pPbrEnvExposure);

    auto m = timeline.apply(&outputs["envExposure"]);
    m.set(0.0);
    m.hold(5.0);
    m.then<RampTo>(1.0, 20.0);
    
}

void ofApp::update() {

    // TIMELINE
    timeline.step(ofGetLastFrameTime());
    
    // PBR UPDATES
    cubeMap.setEnvLevel(pPbrEnvLevel);
    cubeMap.setExposure(pPbrEnvExposure);
    cubeMap.setRotation(pPbrEnvRotation);
    
    // MODELS
    spaceModel.update();
    fullModel.update();
    if(world.primitives["model"] != nullptr){
        world.primitives["model"]->update();
    }
    
    mViewPlane->cam.setGlobalPosition(3+(cos(ofGetElapsedTimef())*.5), 2, -2+sin(ofGetElapsedTimef()));

    //PROJECTORS
    for (auto projector : mProjectors){
        projector.second->update(calibrationCornerMesh);
        if(projector.first == "perspective"){
            projector.second->referencePoints.disableDrawEvent();
            projector.second->referencePoints.disableControlEvents();
            if(projector.second->pTrackViewCamera){
                projector.second->cam.setGlobalPosition(mViewPlane->cam.getGlobalPosition());
                projector.second->cam.lookAt(mViewPlane->plane.getPosition(), glm::vec3(0,1,0));
            }
        }
    }
    
    //SHADERS
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
        renderPrimitiveWithMaterialsRecursive(world.primitives["model"], materials, pbr);
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

void ofApp::renderView() {
    ofPushStyle();

    mViewPlane->begin(true, true);
    
    cam = &mViewPlane->cam;
    //pbr.setCamera(cam);
    pbr.setDrawEnvironment(true);
    pbr.renderScene();
    
    mViewPlane->end();
    ofPopStyle();
}

void ofApp::draw() {
    
    ofBackground(0);
    ofSetColor(255);
    ofEnableAlphaBlending();
    
    pbr.updateDepthMaps();
    
    renderView();
    
    // RENDER PROJECTOR FBO's
    
    for (auto projector : mProjectors){
        
        projector.second->renderCalibrationEditor(world.primitives["space"]);
        
        if(!projector.second->pCalibrationEdit){
            
            projector.second->begin(true);
            
            ofPushStyle();
            
            ofEnableAlphaBlending();
            ofEnableDepthTest();
            
            ofSetColor(0, 255);
            world.primitives["floor"]->recursiveDraw();
            world.primitives["walls"]->recursiveDraw();

            ofPopStyle();

            ofEnableAlphaBlending();
            ofEnableDepthTest();

            cam = &projector.second->getCam();
            
            //TODO: Align reflections to viewplane
            //cam = &mViewPlane->cam;
            //pbr.setCamera(cam);
            pbr.setDrawEnvironment(false);
            pbr.renderScene();
            
            ofPushStyle();
            ofEnableDepthTest();
            ofEnableAlphaBlending();
            
            mViewPlane->draw(true);
            ofPopStyle();
            
            projector.second->end();
            
        }
        
        if(projector.first == "perspective" && !projector.second->pTrackViewCamera){
            projector.second->begin(false, false, false);
            ofPushStyle();
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            ofDisableDepthTest();
            ofSetColor(255, 64);
            mViewPlane->cam.drawFrustum(ofRectangle(0,0,mViewPlane->output.getWidth(), mViewPlane->output.getHeight()));
            ofPopStyle();
            projector.second->end();
        }
        
    }
    
    // DRAW PROJECTORS
    for (auto projector : mProjectors){
        ofPushStyle();
        ofSetColor(255,255,255,255);
        ofDisableAlphaBlending();
        ofDisableDepthTest();
        projector.second->draw();
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
    
    
    auto spaceModelPrimitive = spaceModel.getPrimitives();
    spaceModelPrimitive->setParent(world.origin);
    world.primitives["space"] = spaceModelPrimitive;
    
    world.primitives["calibration"] = spaceModelPrimitive->getFirstPrimitiveWithTextureNameContaining("room.calibration");
    vector<ofMesh> meshes = world.primitives["calibration"]->getBakedMeshesRecursive();
    
    world.primitives["walls"] = spaceModelPrimitive->getFirstPrimitiveWithTextureNameContaining("room.walls");
    world.primitives["floor"] = spaceModelPrimitive->getFirstPrimitiveWithTextureNameContaining("room.floor");
    world.primitives["approach"] = spaceModelPrimitive->getFirstPrimitiveWithTextureNameContaining("approach.deck");
    world.primitives["truss"] = spaceModelPrimitive->getFirstPrimitiveWithTextureNameContaining("room.truss");
    world.primitives["view"] = spaceModelPrimitive->getFirstPrimitiveWithTextureNameContaining("room.view");
    
    world.primitives["calibration"]->clearParent();
    
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
    
    world.primitives["model"] = fullModel.getPrimitives();
    world.primitives["model"]->setParent(world.origin);
    
    
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
            
            bool guiShaderValid = shader.isValid;
            ImGui::Checkbox("Shader Valid", &guiShaderValid);
            
            //ImGui::PushFont(font1);
            ImGui::TextUnformatted("Interface");
            //ImGui::PopFont();
            
            ImGui::Separator();
            
            static bool guiShowTest;
            ImGui::Checkbox("Show Test Window", &guiShowTest);
            if(guiShowTest)
                ImGui::ShowTestWindow();
            
            if(ofxImGui::BeginTree("Calibration", mainSettings)){
                
                for(auto & projector : mProjectors){
                    
                    if(ofxImGui::BeginTree(projector.first.c_str(), mainSettings)){
                        auto p = projector.second;
                        
                        ofxImGui::AddParameter(p->pCalibrationEdit);
                        ofxImGui::AddParameter(p->pCalibrationDrawScales);
                        ofxImGui::AddCombo(p->pCalibrationMeshDrawMode, p->CalibrationMeshDrawModeLabels);
                        ofxImGui::AddCombo(p->pCalibrationMeshColorMode, p->CalibrationMeshColorModeLabels);
                        ofxImGui::AddParameter(p->pCalibrationProjectorColor);
                        vector<string> objectNames;
                        for(auto str :world.primitives["space"]->textureNames){
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

                        if(projector.first == "perspective"){
                            ofxImGui::AddParameter(p->pTrackViewCamera);
                        }
                        
                        ofxImGui::EndTree(mainSettings);
                    }
                    
                }
                
                ofxImGui::EndTree(mainSettings);
                
            }
            
        }
        ofxImGui::AddGroup(pgPbr, mainSettings);

        ofxImGui::AddGroup(mViewPlane->pg, mainSettings);

        ofxImGui::AddGroup(pgScenes, mainSettings);
        
    }
    ofxImGui::EndWindow(mainSettings);
    
    this->gui.end();
    
    return mainSettings.mouseOverGui;
}
