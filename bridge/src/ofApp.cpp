//
//  ofApp.cpp
//  bridge
//
//  Created by ole kristensen on 30/08/2018.
//

#include "ofApp.h"
#include "ofxAssimp3dPrimitiveHelpers.h"
#include "MeshUtils.h"

void ofApp::allocateViewFbo() {
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
        projector.second->referencePoints.setCamera(&projector.second->cam);
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
    
    gui.setup();
    
}

void ofApp::update() {
    spaceModel.update();
    fullModel.update();
    if(fullModelPrimitive != nullptr){
        fullModelPrimitive->update();
    }
    
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

void ofApp::renderCalibration() {
    
    if(shaderToggle){
        shader.begin();
        shader.setUniform1f("elapsedTime", ofGetElapsedTimef());
        shader.end();
    }
    
    auto calibrationRenderMode = static_cast<CalibrationRenderMode>(this->pCalibrationRenderMode.get());
    
    if(calibrationRenderMode == CalibrationRenderMode::Faces) {
        ofEnableDepthTest();
        ofSetColor(255, 128);
        if(shaderToggle) shader.begin();
        spaceModelPrimitive->recursiveDraw(OF_MESH_FILL);
        if(shaderToggle) shader.end();
        ofDisableDepthTest();
    } else if(calibrationRenderMode == CalibrationRenderMode::WireframeFull) {
        if(shaderToggle) shader.begin();
        spaceModelPrimitive->recursiveDraw(OF_MESH_WIREFRAME);
        if(shaderToggle) shader.end();
    } else if(calibrationRenderMode == CalibrationRenderMode::Outline || calibrationRenderMode == CalibrationRenderMode::WireframeOccluded) {
        if(shaderToggle) shader.begin();
        prepareRender(true, true, false);
        glEnable(GL_POLYGON_OFFSET_FILL);
        float lineWidth = ofGetStyle().lineWidth;
        if(calibrationRenderMode == CalibrationRenderMode::Outline) {
            glPolygonOffset(-lineWidth, -lineWidth);
        } else if(calibrationRenderMode == CalibrationRenderMode::WireframeOccluded) {
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

void ofApp::drawCalibrationEditor() {
    
    for (auto projector : mProjectors){
        projector.second->cam.begin(projector.second->viewPort);
        
        // Scales
        
        if(pCalibrationShowScales) {
            ofPushStyle();
            ofSetColor(255,0,255,64);
            ofDrawGrid(1.0, 10, true);
            ofDrawAxis(1.2);
            ofPopStyle();
        }
        
        // Transparent Model
        
        ofPushStyle();
        ofSetColor(255, 32);
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        ofDisableDepthTest();
        ofFill();
        spaceModelPrimitive->recursiveDraw();
        ofEnableDepthTest();
        
        // ViewFbo

        ofPushMatrix();
        //viewNode->transformGL();
        ofScale((viewRectangle.getHeight()/viewFbo.getHeight()));
        ofRotateYDeg(90.0);
        viewFbo.draw(0,0);
        //viewNode->restoreTransformGL();
        ofPopMatrix();
        
        // Coloured Model with Highlighting
        
        for(int mIndex = 0 ; mIndex < spaceModelPrimitive->textureNames.size(); mIndex++){
            ofColor c;
            c.setHsb(360.0*mIndex/spaceModelPrimitive->textureNames.size(), 255, 255);
            ofPushStyle();
            ofSetColor(c);
            auto primitives = getPrimitivesWithTextureIndex(spaceModelPrimitive, mIndex);
            for(auto p : primitives){
                if(mIndex == pCalibrationHighlightIndex){
                    shader.begin();
                    shader.setUniform1f("elapsedTime", ofGetElapsedTimef());
                }
                p->recursiveDraw();
                if(mIndex == pCalibrationHighlightIndex){
                    shader.end();
                }
            }
            ofPopStyle();
        }
        
        // Camera Frustrums
        
        if(projector.first == "perspective"){
            for (auto p : mProjectors) {
                if(p.first != "perspective"){
                    ofPushStyle();
                    ofSetColor(255, 255, 0);
                    p.second->cam.drawFrustum(p.second->viewPort);
                    ofPopStyle();
                }
            }
        }
        
        ofEnableDepthTest();
        ofPopStyle();
        
        projector.second->cam.end();
        
        // UPDATE CALIBRATION
        
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
    
    //cout<<ofGetCurrentViewport()<<endl;
    /*pbr.updateDepthMaps();
    
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
    
    projector->referencePoints.disableControlEvents();
    projector->referencePoints.disableDrawEvent();
    
    ofDisableAlphaBlending();
    ofEnableDepthTest();
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    
    projector->renderPass.begin();
    projector->renderPass.activateAllDrawBuffers();
    ofClear(0);
    
    if(projector->mapamok.calibrationReady){
        projector->mapamok.begin(ofGetCurrentViewport());
        cam = &projector->mapamok.cam;
        pbr.setCamera(cam);
        pbr.renderScene();
        projector->mapamok.end();
    } else {
        projector->cam.begin(ofGetCurrentViewport());
        cam = &projector->cam;
        pbr.setCamera(cam);
        pbr.renderScene();
        projector->cam.end();
    }
    
    projector->renderPass.end();
    
    glDisable(GL_CULL_FACE);
    
    ofDisableDepthTest();
    ofEnableAlphaBlending();
    
    // post effect
    projector->tonemapPass.begin();
    ofClear(0);
    tonemap.begin();
    tonemap.setUniformTexture("image", projector->renderPass.getTexture(), 0);
    tonemap.setUniform1f("exposure", pPbrExposure);
    tonemap.setUniform1f("gamma", pPbrGamma);
    projector->renderPass.draw(0, 0);
    tonemap.end();
    projector->tonemapPass.end();
    
    projector->output.begin();
    fxaa.begin();
    fxaa.setUniformTexture("image", projector->tonemapPass.getTexture(), 0);
    fxaa.setUniform2f("texel", 1.25 / float(projector->tonemapPass.getWidth()), 1.25 / float(projector->tonemapPass.getHeight()));
    projector->tonemapPass.draw(0,0);
    fxaa.end();
    projector->output.end();
    
    ofPushStyle();
    projector->output.draw(ofGetCurrentViewport());
    ofPopStyle();*/
    
}

void ofApp::draw() {
    
    ofBackground(0);
    ofSetColor(255);
    
    if(pCalibrationEdit) {
        
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
            tonemap.setUniform1f("exposure", pPbrExposure);
            tonemap.setUniform1f("gamma", pPbrGamma);
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
    vector<ofMesh> meshes = spaceModelPrimitive->getBakedMeshesRecursive();
    
    auto calibrationPrimitive = getFirstPrimitiveWithTextureNameContaining(spaceModelPrimitive, "room.calibration");
    
    wallModelNode = getFirstPrimitiveWithTextureNameContaining(spaceModelPrimitive, "room.walls");
    approachModelNode = getFirstPrimitiveWithTextureNameContaining(spaceModelPrimitive, "approach.deck");
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
            
            ImGui::Checkbox("Use Shader", &shaderToggle);
            bool guiShaderValid = shader.isValid;
            ImGui::Checkbox("Shader Valid", &guiShaderValid);
            
            if(ImGui::SliderFloat("Resolution", &viewResolution, 1.0, 300.0)){
                allocateViewFbo();
            }
            
            //ImGui::PushFont(font1);
            ImGui::TextUnformatted("Interface");
            //ImGui::PopFont();
            
            ImGui::Separator();
            
            static bool guiShowTest;
            ImGui::Checkbox("Show Test Window", &guiShowTest);
            if(guiShowTest)
                ImGui::ShowTestWindow();

            if (ofxImGui::BeginTree(this->pgCalibration, mainSettings))
            {
                ofxImGui::AddParameter(pCalibrationEdit);
                ofxImGui::AddParameter(pCalibrationShowScales);

                for(auto & projector : mProjectors){
                    ImGui::TextUnformatted(projector.first.c_str());
                    if(ImGui::Button("Load")){
                        projector.second->load("calibrations/" + projector.first);
                    } ImGui::SameLine();
                    if(ImGui::Button("Save")){
                        projector.second->save("calibrations/" + projector.first);
                    }ImGui::SameLine();
                    if(ImGui::Button("Clear")){
                        projector.second->referencePoints.clear();
                    }
                }

                static const vector<string> labels = { "Faces", "Outline", "Wireframe full", "Wireframe occluded" };
                
                ofxImGui::AddCombo(this->pCalibrationRenderMode, labels);
                
                vector<string> objectNames;
                for(auto str :spaceModelPrimitive->textureNames){
                    string objectName = ofSplitString(str, "/").back();
                    auto objectNameComponents = ofSplitString(objectName, ".");
                    objectNameComponents.pop_back();
                    objectName = ofJoinString(objectNameComponents, ".");
                    objectNames.push_back(objectName);
                }
                
                ofxImGui::AddCombo(this->pCalibrationHighlightIndex, objectNames);
                
                ofxImGui::EndTree(mainSettings);
            }

            ofxImGui::AddGroup(pgPbr, mainSettings);
            
            
        }
        
        
    }
    ofxImGui::EndWindow(mainSettings);
    
    this->gui.end();
    
    return mainSettings.mouseOverGui;
}
