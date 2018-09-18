//
//  ofApp.cpp
//  bridge
//
//  Created by ole kristensen on 30/08/2018.
//

#include "ofApp.hpp"
#include "ofxAssimp3dPrimitiveHelpers.hpp"
#include "MeshUtils.hpp"
#include <dispatch/dispatch.h>

using namespace ofxChoreograph;

ImVec4 from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a, bool consistent_color)
{
    auto res = ImVec4(r / (float)255, g / (float)255, b / (float)255, a / (float)255);
#ifdef FLIP_COLOR_SCHEME
    if (!consistent_color) return flip(res);
#endif
    return res;
}

ImVec4 operator+(const ImVec4& c, float v)
{
    return ImVec4(
                  std::max(0.f, std::min(1.f, c.x + v)),
                  std::max(0.f, std::min(1.f, c.y + v)),
                  std::max(0.f, std::min(1.f, c.z + v)),
                  std::max(0.f, std::min(1.f, c.w))
                  );
}

void ofApp::setup() {
    
    // WINDOW
    ofSetWindowTitle(title);
    ofSetVerticalSync(true);
    
    // MODELS
    
    loadNodeModel("models/nodes.dae"); //this needs to load first
    loadRenderModel("models/nodes.dae");

    
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
    
    mProjectorFirstPerson = make_shared<Projector>(ofRectangle(0,projectionResolution.y,ofGetWidth(), ofGetHeight()-projectionResolution.y),
                                                   defaultFboSettings, shader, tonemap, fxaa);
    mProjectors.insert(make_pair("first person", mProjectorFirstPerson));
    
    mProjectorFront = make_shared<Projector>(ofRectangle(0, 0, projectionResolution.x, projectionResolution.y),
                                            defaultFboSettings, shader, tonemap, fxaa);
    mProjectors.insert(make_pair("front", mProjectorFront));
    
    mProjectorSide = make_shared<Projector>(ofRectangle(projectionResolution.x, 0, projectionResolution.x, projectionResolution.y),
                                             defaultFboSettings, shader, tonemap, fxaa);
    
    mProjectors.insert(make_pair("side", mProjectorSide));
    
    for(auto projector : mProjectors){
        projector.second->referencePoints.enableControlEvents();
        projector.second->cam.setDistance(10);
        projector.second->cam.setNearClip(.1);
        projector.second->cam.setFarClip(10000);
        projector.second->cam.setVFlip(false);
        projector.second->referencePoints.setCamera(&projector.second->cam);
        projector.second->pg.setName(projector.first);
        if(projector.first == "first person"){
            projector.second->pg.add(projector.second->pEnabled);
            projector.second->pg.add(projector.second->pTrackUserCamera);
            projector.second->pg.add(projector.second->pAnimateCamera);
            projector.second->cam.setGlobalPosition(6.0,1.9, -2.0 );
            projector.second->cam.lookAt(*world.primitives["room.views.front"]);
        }
        projector.second->pg.add(projector.second->pgCalibration);
        pgProjectors.add(projector.second->pg);
        projector.second->load("calibrations/" + projector.first);
    }
    pgProjectors.setName("Projectors");
    pgGlobal.add(pgProjectors);
    
    //VIEW PLANE
    
    mViewFront = make_shared<ViewPlane>(defaultFboSettings, shader, tonemap, fxaa, world.primitives["room.views.front"], world);
    mViewSide = make_shared<ViewPlane>(defaultFboSettings, shader, tonemap, fxaa, world.primitives["room.views.side"], world);

    mViewFront->pg.setName("Front View");
    pgGlobal.add(mViewFront->pg);
    mViewSide->pg.setName("Side View");
    pgGlobal.add(mViewSide->pg);

    //PBR
    
    cam = &mProjectorFront->cam;
    scene = bind(&ofApp::pbrRenderScene, this);
    pbr.setup(scene, cam, 1024*4);
    
    materials.resize(renderPrimitive->textureNames.size());
    
    int materialIndex = 0;
    for (auto & material : materials){
        string textureName = renderPrimitive->textureNames[materialIndex] ;
        material.baseColor.set(0.95, 1.0, 1.0);
        material.metallic = ofMap(materialIndex++, 0, materials.size(), 0.0, 1.0);
        material.roughness = 0.2;
        if(ofIsStringInString(textureName, "truss")){
            material.baseColor.set(0.01, 0.01, 0.01, 1.0);
            material.metallic = 0.001;
            material.roughness = 0.5;
        }
        if(ofIsStringInString(textureName, "pylon") || ofIsStringInString(textureName, "pier")){
            material.baseColor.set(0.5, 0.5, 0.5, 1.0);
            material.metallic = 0.0;
            material.roughness = 0.9;
        }
        if(ofIsStringInString(textureName, "deck")){
            material.baseColor.set(0.1, 0.1, 0.1, 1.0);
            material.metallic = 0.0;
            material.roughness = 1.0;
        }
    }
    
    cubeMap.load("ofxPBRAssets/cubemaps/DH-AO-12.hdr", 1024, true, "ofxPBRAssets/cubemapCache");
    cubeMap.setEnvLevel(0.1);
    
    pbr.setCubeMap(&cubeMap);
    pbr.setDrawEnvironment(true);
    
    lights.resize(2);
    lights[0].setParent(world.origin);
    lights[0].setLightType(LightType_Directional);
    lights[0].setShadowType(ShadowType_None);
    lights[0].setIntensity(1.0);

    lights[1].setParent(world.origin);
    lights[1].setLightType(LightType_Spot);
    lights[1].setShadowType(ShadowType_None);
    lights[1].setSpotLightDistance(100.0);
    lights[1].setSpotLightGradient(0.75);
    lights[1].setSpotLightCutoff(20.0);
    lights[1].setIntensity(1.0);

    pbr.addLight(&lights[0]);
    pbr.addLight(&lights[1]);

    // FONTS
    
    fontHeader.load("fonts/OpenSans-Regular.ttf", 264, true, true, true);
    fontBody.load("fonts/OpenSans-Light.ttf", 264, true, true, true);
    
    
    // SCENES
    
    for( auto s : scenes) {
        s->setupScene(&pgGlobal, &world, &scenes);
    }
    
    //GUI
    
    ofAddListener(ofGetWindowPtr()->events().keyPressed, this,
                  &ofApp::keycodePressed);
    
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    gui_font_text = io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Regular.ttf").c_str(), 16.0f);
    gui_font_header = io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Regular.ttf").c_str(), 22.0f);
    
    gui.setup();
    
    logo.load("images/logo.png");
    logoID = gui.loadImage(logo);
    
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(14.,14.);
    style.WindowRounding = 4.0f;
    style.ChildWindowRounding = 4.;
    style.FramePadding = ImVec2(8, 4);
    style.FrameRounding = 4;
    style.ItemSpacing = ImVec2(6, 5);
    style.ItemInnerSpacing = ImVec2(7, 4);
    style.IndentSpacing = 18;
    style.ScrollbarRounding = 2;
    
    style.Colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.025f, 0.025f, 0.025f, 0.85f);
    style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.22f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.05f, 0.05f, 0.05f, 0.90f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.70f, 0.70f, 0.70f, 0.29f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.07f, 0.09f, 0.10f, 0.40f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.00f, 0.45f, 0.78f, 0.39f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.65f, 0.74f, 0.81f, 0.33f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.11f, 0.13f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.11f, 0.13f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.24f, 0.29f, 0.32f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.11f, 0.13f, 0.15f, 0.80f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.05f, 0.07f, 0.08f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.21f, 0.24f, 0.26f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.31f, 0.34f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.11f, 0.13f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_ComboBg]               = ImVec4(0.04f, 0.04f, 0.05f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.90f, 0.90f, 0.90f, 0.57f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(1.00f, 1.00f, 1.00f, 0.13f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.00f, 0.45f, 0.78f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.24f, 0.30f, 0.35f, 0.18f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.00f, 0.45f, 0.78f, 0.39f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.00f, 0.45f, 0.78f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.24f, 0.30f, 0.35f, 0.18f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.00f, 0.45f, 0.78f, 0.39f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.00f, 0.45f, 0.78f, 1.00f);
    style.Colors[ImGuiCol_Column]                = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.70f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.90f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
    style.Colors[ImGuiCol_CloseButton]           = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
    style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.00f, 0.45f, 0.78f, 0.39f);
    style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.00f, 0.56f, 1.00f, 0.35f);
    style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
    
    videoPlayer.setPixelFormat(OF_PIXELS_RGBA);
    videoPlayer.load("videos/En bro mellem Danmark og Sverige_2.mov");
    videoPlayer.setLoopState(OF_LOOP_NONE);
    
    load("default");
    
    // TIMELINE
    
    timelineFloatOutputs["envExposure"]().makeReferenceTo(pPbrEnvExposure);
    timelineFloatOutputs["envRotation"]().makeReferenceTo(pPbrEnvRotation);
    timelineFloatOutputs["pbrGamma"]().makeReferenceTo(pPbrGamma);
    
    timelineFloatColorOutputs["textHeaderColor"]().makeReferenceTo(pTextHeaderColor);
    timelineFloatColorOutputs["textBodyColor"]().makeReferenceTo(pTextBodyColor);
    
    
}

void ofApp::startAnimation(){
    auto pbrGamma = timeline.apply(&timelineFloatOutputs["pbrGamma"]);
    pbrGamma.set(0.0);
    pbrGamma.hold(5.0);
    pbrGamma.then<RampTo>(2.2, 10., EaseOutQuad());
    
    auto envRotation = timeline.apply(&timelineFloatOutputs["envRotation"]);
    envRotation.set(-.3);
    envRotation.hold(5.0);
    envRotation.then<RampTo>(.4, 10.f);
    
    auto textHeaderColor = timeline.apply(&timelineFloatColorOutputs["textHeaderColor"]);
    textHeaderColor.set(ofFloatColor(1.,1.,1.,0.));
    textHeaderColor.hold(10.0);
    textHeaderColor.then<RampTo>(ofFloatColor(1.,1.,1.,1.),20.f);
    
    auto textBodyColor = timeline.apply(&timelineFloatColorOutputs["textBodyColor"]);
    textBodyColor.set(ofFloatColor(1.,1.,1.,0.));
    textHeaderColor.hold(10.0);
    textBodyColor.then<RampTo>(ofFloatColor(1.,1.,1.,1.), 20.f);
}


void ofApp::update() {
    
    //VIDEO
    videoPlayer.update();
    
    // TIMELINE
    timeline.step(ofGetLastFrameTime());
    
    // PBR UPDATES
    cubeMap.setEnvLevel(pPbrEnvLevel);
    cubeMap.setExposure(pPbrEnvExposure);
    cubeMap.setRotation(pPbrEnvRotation);
    
    // MODELS
    renderModelLoader.update();
    nodeModelLoader.update();
    if(world.primitives["view"] != nullptr){
        world.primitives["view"]->update();
    }
    
    //SHADERS
    tonemap.begin();
    tonemap.setUniform1f("time", ofGetElapsedTimef());
    tonemap.setUniform1f("exposure", pPbrExposure);
    tonemap.setUniform1f("gamma", pPbrGamma);
    tonemap.end();
    
}

void ofApp::pbrRenderScene() {
    
    ofEnableDepthTest();
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    pbr.beginDefaultRenderer();
    {

        ofSetColor(255,255);
        renderPrimitiveWithMaterialsRecursive(currentRenderPrimitive, materials, pbr);
    }

    pbr.endDefaultRenderer();
    glDisable(GL_CULL_FACE);
    ofDisableDepthTest();
}

void ofApp::drawProjectorWall(ofEventArgs & args) {
    drawProjection(mProjectorFront);
}

void ofApp::drawProjectorSide(ofEventArgs & args) {
    drawProjection(mProjectorSide);
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

void ofApp::renderViews() {
    ofPushStyle();

    // HDR
    mViewSide->begin(true, true);{
        currentRenderPrimitive = renderPrimitive;
        cam = &mViewSide->cam;
        pbr.setMainCamera(cam);
        pbr.setDrawEnvironment(true);
        pbr.renderScene();
/*
        ofPushStyle();
        ofSetColor(255,255,0,127);
        world.meshes["view"]->draw();
        ofPopStyle();
 */
    }mViewSide->end();

    // HDR
    mViewFront->begin(true, true);{
        currentRenderPrimitive = renderPrimitive;
        cam = &mViewFront->cam;
        pbr.setMainCamera(cam);
        pbr.setDrawEnvironment(true);
        pbr.renderScene();
        /*
        ofPushStyle();
        ofSetColor(0,127,255,127);
        world.meshes["view"]->draw();
        ofPopStyle();
         */
    }mViewFront->end();
    
    // LDR overlays
    mViewFront->begin(false, true);{
        ofPushMatrix();
        mViewFront->plane.transformGL();
        ofDisableDepthTest();
        ofEnableAlphaBlending();
        ofFill();
        ofTranslate(-mViewFront->plane.getWidth()/2.0, -mViewFront->plane.getHeight()/2.0);
        
        /*
        ofPushMatrix();{
            ofTranslate(0.1, -0.28);
            ofScale(0.10/fontHeader.getSize());
            
            ofSetColor(ofColor(pTextBodyColor.get()));
            fontBody.drawString("The Oresund Bridge", 0.0, 0.0);
            
            ofTranslate(0, -fontHeader.getSize()*1.5);
            
            ofSetColor(ofColor(pTextHeaderColor.get()));
            fontHeader.drawString("Øresundsbroen", 0.0, 0.0);
        }ofPopMatrix();
        */
        
        ofSetColor(ofColor(pTextBodyColor.get()));

        videoPlayer.draw(0, 0, mViewFront->plane.getHeight()*videoPlayer.getWidth()*1.0/videoPlayer.getHeight(), mViewFront->plane.getHeight());
        
        mViewFront->plane.restoreTransformGL();
        ofPopMatrix();
    }mViewFront->end();
    
    ofPopStyle();
}

void ofApp::draw() {
    
    ofBackground(0);
    ofSetColor(255);
    ofEnableAlphaBlending();

    //PROJECTORS NEED TO BE UPDATED IN DRAW
    for (auto projector : mProjectors){
        if(projector.second->pEnabled){
            projector.second->update(calibrationCornerMesh);
            if(projector.first == "first person"){
                projector.second->referencePoints.disableDrawEvent();
                projector.second->referencePoints.disableControlEvents();
                if(projector.second->pAnimateCamera){
                    projector.second->cam.setGlobalPosition(3+(cos(ofGetElapsedTimef()*.5)*.5), 2, -2+sin(ofGetElapsedTimef()*.2));
                    projector.second->cam.lookAt(mViewFront->plane.getGlobalPosition(), glm::vec3(0,1,0));
                }
                if(projector.second->pTrackUserCamera){
                    mViewFront->cam.setGlobalPosition(projector.second->cam.getGlobalPosition());
                    mViewSide->cam.setGlobalPosition(projector.second->cam.getGlobalPosition());
                }
            }
        }
    }

    // LGIHTS TOO
    
    lights[0].setColor(pPbrDirectionalLightColor);
    lights[0].lookAt(mViewFront->plane.getGlobalPosition());
    lights[0].setGlobalPosition(mViewFront->plane.getGlobalPosition() * 10);
    
    lights[1].setColor(pPbrSpotLightColor);
    lights[1].lookAt(mViewFront->plane.getGlobalPosition()+glm::vec3(1.0,0.25,0.0));
    lights[1].setGlobalPosition(mViewFront->cam.getGlobalPosition());

    currentRenderPrimitive = renderPrimitive;
    pbr.updateDepthMaps();
    
    renderViews();
    
    // RENDER PROJECTOR FBO's
    
    for (auto projector : mProjectors){
        if(projector.second->pEnabled){
            
            projector.second->renderCalibrationEditor(world.primitives["room"]);
            
            if(!projector.second->pCalibrationEdit){
                
                projector.second->begin(true);{
                    
                    // HDR
                    
                    ofSetColor(255,255);
                    
                    // BLACK ROOM MASK
                    ofPushStyle();
                    ofEnableAlphaBlending();
                    ofEnableDepthTest();
                    ofSetColor(ofColor(pPbrRoomColor.get()));
                    world.primitives["room.floor"]->recursiveDraw();
                    world.primitives["room.walls"]->recursiveDraw();
                    if(projector.first == "front"){
                        world.primitives["room.truss"]->recursiveDraw();
                    }
                    if(projector.first == "first person"){
                        world.primitives["room.ceiling"]->recursiveDraw();
                    }
                    ofPopStyle();
                    
                    if(projector.first == "front" || projector.first == "first person"){
                        // HDR view
                        ofPushStyle();
                        ofEnableAlphaBlending();
                        ofEnableDepthTest();
                        mViewFront->draw(true);
                        ofPopStyle();

                        // PBR in space
                        currentRenderPrimitive = world.primitives["room.pylon"];
                        cam = &mViewFront->cam;
                        pbr.setMainCamera(cam);
                        pbr.setDrawEnvironment(false);
                        pbr.renderScene();
                    }
                    if(projector.first == "side" || projector.first == "first person"){
                        // HDR view
                        ofPushStyle();
                        ofEnableAlphaBlending();
                        ofEnableDepthTest();
                        mViewSide->draw(true);
                        ofPopStyle();
                        
                        // PBR in space
                        ofPushStyle();
                        ofEnableAlphaBlending();
                        ofEnableDepthTest();
                        currentRenderPrimitive = world.primitives[(projector.first == "side" || !pPbrFullModelView)?"room.truss":"render"];
                        cam = &mViewFront->cam;
                        pbr.setMainCamera(cam);
                        pbr.setDrawEnvironment(false);
                        pbr.renderScene();
                        ofPopStyle();
                    }
                    
                }projector.second->end();
                
                projector.second->begin(false, false, false);{
                    
                    // LDR (overlays)
                    
                    // BLACK ROOM MASK
                    ofPushStyle();
                    ofEnableAlphaBlending();
                    ofEnableDepthTest();
                    ofSetColor(0, 255);
                    glColorMask(false, false, false, false);
                    world.primitives["room.floor"]->recursiveDraw();
                    world.primitives["room.walls"]->recursiveDraw();
                    world.primitives["room.truss"]->recursiveDraw();
                    if(projector.first == "first person"){
                        world.primitives["room.ceiling"]->recursiveDraw();
                    }
                    glColorMask(true, true, true, true);
                    
                    ofPopStyle();
                    
                    if(projector.first == "front" || projector.first == "first person"){
                        
                        // TEXT
                        ofPushStyle();
                        ofEnableAlphaBlending();
                        ofEnableDepthTest();
                        ofPushMatrix();
                        ofTranslate(0.01, 0., 0.);
                        ofSetColor(255,255);
                        mViewFront->draw(false, true);
                        ofPopMatrix();
                        ofPopStyle();
                        
                    }
                    
                }projector.second->end();
                
            }
            
            if((projector.first == "first person" && !projector.second->pTrackUserCamera) || (projector.first != "first person" && projector.second->pCalibrationEdit)){
                projector.second->begin(false, false, false);
                ofPushStyle();
                ofEnableBlendMode(OF_BLENDMODE_ADD);
                ofDisableDepthTest();
                ofSetColor(255, 64);
                mViewFront->drawCameraModel();
                mViewSide->drawCameraModel();
                
                ofPushMatrix();
                ofPushStyle();
                ofDisableDepthTest();
                ofSetColor(ofColor::red);
                ofDrawSphere(mViewFront->windowTopLeft, 0.01);
                ofSetColor(ofColor::green);
                ofDrawSphere(mViewFront->windowBottomLeft, 0.01);
                ofSetColor(ofColor::blue);
                ofDrawSphere(mViewFront->windowBottomRight, 0.01);
                ofPopStyle();
                ofPopMatrix();

                ofPushMatrix();
                ofPushStyle();
                ofDisableDepthTest();
                ofSetColor(ofColor::red);
                ofDrawSphere(mViewSide->windowTopLeft, 0.01);
                ofSetColor(ofColor::green);
                ofDrawSphere(mViewSide->windowBottomLeft, 0.01);
                ofSetColor(ofColor::blue);
                ofDrawSphere(mViewSide->windowBottomRight, 0.01);
                ofPopStyle();
                ofPopMatrix();

                ofPopStyle();
                projector.second->end();
            }
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
        ofPopStyle();
    }
    
    // GUI
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofFill();
    ofSetColor(255,255);
    
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

void ofApp::loadRenderModel(string filename) {
    
    ofSetLogLevel("ofxAssimpModelLoader", OF_LOG_NOTICE);
    
    renderModelLoader.loadModel(filename, true, true);
    
    // rotate the model to match the ofMeshes we get later...
    renderModelLoader.setRotation(0, 180, 0, 0, 1.0);
    
    // make sure to load to scale
    renderModelLoader.setScaleNormalization(false);
    renderModelLoader.calculateDimensions();
    
    renderPrimitive = renderModelLoader.getPrimitives();
    renderPrimitive->setParent(world.origin);
    
    renderPrimitive->setGlobalPosition(world.offset.getPosition());

    world.primitives["render"] = renderPrimitive;

    // get rid of room meshes by orphaning
    for (auto textureName : renderPrimitive->textureNames){
        if(ofIsStringInString(textureName, "-room")){
            renderPrimitive->getFirstPrimitiveWithTextureNameContaining(textureName)->clearParent();
        }
    }
}


void ofApp::loadNodeModel(string filename) {
    
    ofSetLogLevel("ofxAssimpModelLoader", OF_LOG_NOTICE);

    nodeModelLoader.loadModel(filename, true, false);
    
    // rotate the model to match the ofMeshes we get later...
    nodeModelLoader.setRotation(0, 180, 0, 0, 1.0);
    
    // make sure to load to scale
    nodeModelLoader.setScaleNormalization(false);
    nodeModelLoader.calculateDimensions();
    
    nodePrimitive = nodeModelLoader.getPrimitives();
    nodePrimitive->setParent(world.origin);
    
    world.primitives["root"] = nodePrimitive;
    
    world.primitives["room.calibration"] = nodePrimitive->getFirstPrimitiveWithTextureNameContaining("room.calibration");
    world.primitives["room"] = (ofxAssimp3dPrimitive*) world.primitives["room.calibration"]->getParent()->getParent()->getParent();
    
    world.offset.setPosition(-world.primitives["room.calibration"]->getGlobalPosition());
    
    nodePrimitive->setGlobalPosition(world.offset.getPosition());
    
    world.primitives["room.walls"] = world.primitives["room"]->getFirstPrimitiveWithTextureNameContaining("room.walls");
    world.primitives["room.floor"] = world.primitives["room"]->getFirstPrimitiveWithTextureNameContaining("room.floor");
    world.primitives["room.ceiling"] = world.primitives["room"]->getFirstPrimitiveWithTextureNameContaining("room.ceiling");
    world.primitives["room.pylon"] = (ofxAssimp3dPrimitive*) world.primitives["room"]->getFirstPrimitiveWithTextureNameContaining("room.pylon.png")->getParent()->getParent()->getParent();
    world.primitives["room.pylon.crossbrace"] = world.primitives["room"]->getFirstPrimitiveWithTextureNameContaining("room.pylon.crossbrace");
    world.primitives["room.truss"] = (ofxAssimp3dPrimitive*) world.primitives["room"]->getFirstPrimitiveWithTextureNameContaining("room.truss")->getParent()->getParent()->getParent();
    world.primitives["room.views.front"] = world.primitives["room"]->getFirstPrimitiveWithTextureNameContaining("room.views.front");
    world.primitives["room.views.side"] = world.primitives["room"]->getFirstPrimitiveWithTextureNameContaining("room.views.side");
    world.primitives["room.calibration"] = world.primitives["room"]->getFirstPrimitiveWithTextureNameContaining("room.calibration");
    vector<ofMesh> calibrationMeshes = world.primitives["room.calibration"]->getBakedMeshesRecursive();
    
    // get room primitive out of the view drawing tree
    auto roomPosition = world.primitives["room"]->getGlobalPosition();
    auto roomOrientation = world.primitives["room"]->getGlobalOrientation();
    auto roomScale = world.primitives["room"]->getGlobalScale();
    world.primitives["room"]->clearParent();
    world.primitives["room"]->setParent(world.origin);
    world.primitives["room"]->setScale(roomScale);
    world.primitives["room"]->setGlobalPosition(roomPosition);
    world.primitives["room"]->setGlobalOrientation(roomOrientation);
    
    // get calibration pritive out of the drawing tree
    world.primitives["room.calibration"]->clearParent();
    
    // bake calibration meshe
    cout << "baking calibration mesh" << endl;
    calibrationMesh = ofVboMesh();
    calibrationMesh = joinMeshes(calibrationMeshes);
    ofVec3f cornerMin, cornerMax;
    getBoundingBox(calibrationMesh, cornerMin, cornerMax);
    
    // merge
    // another good metric for finding corners is if there is a single vertex touching
    // the wall of the bounding box, that point is a good control point
    
    calibrationCornerMesh = ofVboMesh();
    for(int i = 0; i < calibrationMeshes.size(); i++) {
        ofMesh mergedMesh = mergeNearbyVertices(calibrationMeshes[i], mergeTolerance);
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

void ofApp::save(string name){
    ofJson j;
    ofSerialize(j, pgGlobal);
    ofSaveJson("settings/" + name + ".json", j);
}

void ofApp::load(string name){
    ofJson j = ofLoadJson("settings/" + name + ".json");
    ofDeserialize(j, pgGlobal);
}

void ofApp::keyPressed(int key){
    
}

void ofApp::keycodePressed(ofKeyEventArgs& e){
    if(e.hasModifier(OF_KEY_CONTROL)){
        if(e.keycode == 'F'){
            ofToggleFullscreen();
        }
        if(e.keycode == 'G'){
            guiVisible = !guiVisible;
        }
    }
}

void ofApp::windowResized(int w, int h){
    mProjectors.at("first person")->viewPort.set(0, projectionResolution.y, w, h-projectionResolution.y);
    mProjectors.at("first person")->resizeFbos();
}

bool ofApp::imGui()
{
    auto mainSettings = ofxImGui::Settings();
    
    ofDisableDepthTest();
    
    this->gui.begin();
    {
        ImGui::PushFont(gui_font_text);
        
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoCollapse;
        
        if (ofxImGui::BeginWindow(title.c_str(), mainSettings, window_flags)){
            
            ImGui::Columns(2, "TitleColumns", false);
            ImGui::PushFont(gui_font_header);
            ImGui::TextUnformatted("Øresundsbroen");
            ImGui::PopFont();
            ImGui::TextUnformatted("(c) den frie vilje 2018 for Arup");
            ImGui::Text("FPS %.3f", ofGetFrameRate());
            int logoSize = 60;
            ImGui::SetColumnOffset(1, ImGui::GetWindowContentRegionMax().x - (logoSize + 7));
            ImGui::NextColumn();
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.8);
            ImGui::Image((ImTextureID)(uintptr_t)logoID, {static_cast<float>(logoSize),static_cast<float>(logoSize)});
            ImGui::PopStyleVar();
            ImGui::Columns(1);
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
            
            ImGui::Separator();
            
            if(ImGui::Button("Play Timeline")){
                startAnimation();
            }

            ImGui::SameLine();
            
            if(ImGui::Button("Play Video")){
                videoPlayer.play();
            }

            ImGui::Separator();
            /*
            bool guiShaderValid = shader.isValid;
            ImGui::Checkbox("Shader Valid", &guiShaderValid);
            */
            
            
            
            ImGui::Separator();
            
            static bool guiShowTest;
            ImGui::Checkbox("Show Test Window", &guiShowTest);
            if(guiShowTest)
                ImGui::ShowTestWindow();
            
            ofxImGui::AddGroup(pgPbr, mainSettings);
            
            ofxImGui::AddGroup(mViewFront->pg, mainSettings);

            ofxImGui::AddGroup(mViewSide->pg, mainSettings);

            ofxImGui::AddGroup(pgScenes, mainSettings);
            
            ofxImGui::AddGroup(pgText, mainSettings);
            
            ofxImGui::EndWindow(mainSettings);
        }
        
        for(auto & projector : mProjectors){
            
            auto p = projector.second;
            
            window_flags = 0;
            window_flags |= ImGuiWindowFlags_NoCollapse;
            window_flags |= ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoTitleBar;
            window_flags |= ImGuiWindowFlags_NoResize;
            window_flags |= ImGuiWindowFlags_NoScrollbar;
            window_flags |= ImGuiWindowFlags_NoScrollWithMouse;
            
            float windowMargin = 24;
            float windowHeight = 48;
            
            ofRectangle windowRect;
            windowRect.setPosition(glm::vec3(p->viewPort.getBottomLeft()+glm::vec2(windowMargin,-(windowMargin+windowHeight))));
            windowRect.setSize(p->viewPort.getWidth()-(2*windowMargin),windowHeight);
            
            bool mouseOverWindow = (windowRect.inside(ofGetMouseX(), ofGetMouseY()) || p->pCalibrationEdit) && !p->cam.isMouseDragging();
            
            float distanceToWindow = ofClamp(windowRect.distanceFrom(glm::vec2(ofGetMouseX(), ofGetMouseY())), 0, 100);
            
            
            if(!mouseOverWindow) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ofMap(distanceToWindow, 0, 100, 1.0, 0.1));{
                
                mainSettings.windowSize = glm::vec2(windowRect.getWidth(), windowRect.getHeight());
                mainSettings.windowPos = windowRect.getPosition();
                
                ofxImGui::BeginWindow(projector.first, mainSettings, window_flags, nullptr, ImGuiSetCond_Always);
                
                string uppercaseTitle = "";
                std::transform(projector.first.begin(), projector.first.end(),uppercaseTitle.begin(), ::toupper);
                
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6*6, 5));
                if(!mouseOverWindow) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 10.0);{
                    ImGui::PushFont(gui_font_header);
                    ImGui::TextUnformatted(uppercaseTitle.c_str());
                    ImGui::PopFont();
                    ImGui::SameLine();
                } if(!mouseOverWindow) ImGui::PopStyleVar();
                
                if(!mouseOverWindow) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5);{
                    if(projector.first == "first person"){
                        ofxImGui::AddParameter(p->pEnabled);
                        ImGui::SameLine();
                        ofxImGui::AddParameter(p->pTrackUserCamera);
                        ImGui::SameLine();
                        ofxImGui::AddParameter(p->pAnimateCamera);
                        ImGui::SameLine();
                    }
                    ofxImGui::AddParameter(p->pCalibrationEdit);
                    ImGui::SameLine();
                    
                    if(!mouseOverWindow) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.01);{
                        if(!p->pCalibrationEdit && mouseOverWindow) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, .25);{
                            ofxImGui::AddParameter(p->pCalibrationDrawScales);
                            ImGui::SameLine();
                            ImGui::PushItemWidth(200);
                            ofxImGui::AddCombo(p->pCalibrationMeshDrawMode, p->CalibrationMeshDrawModeLabels);
                            ImGui::SameLine();
                            ofxImGui::AddCombo(p->pCalibrationMeshColorMode, p->CalibrationMeshColorModeLabels);
                            ImGui::SameLine();
                            ofxImGui::AddParameter(p->pCalibrationProjectorColor);
                            ImGui::SameLine();
                            vector<string> objectNames;
                            for(auto str :world.primitives["room"]->textureNames){
                                string objectName = ofSplitString(str, "/").back();
                                auto objectNameComponents = ofSplitString(objectName, ".");
                                objectNameComponents.pop_back();
                                objectName = ofJoinString(objectNameComponents, ".");
                                objectNames.push_back(objectName);
                            }
                            
                            ofxImGui::AddCombo(p->pCalibrationHighlightIndex, objectNames);
                        } if(!p->pCalibrationEdit && mouseOverWindow) ImGui::PopStyleVar();
                        ImGui::PopItemWidth();
                        ImGui::PopStyleVar();
                    }if(!mouseOverWindow) ImGui::PopStyleVar();
                }if(!mouseOverWindow) ImGui::PopStyleVar();
                
                // Right aligned buttons are annoying in ImGui
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6*2, 5));
                
                const float ItemSpacing = ImGui::GetStyle().ItemSpacing.x;
                static float ClearButtonWidth = 100.0f; //The 100.0f is just a guess size for the first frame.
                float pos = ClearButtonWidth;
                ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - pos);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.,0,0,0.5));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.,0,0,1.0));
                if(ImGui::Button("Clear")){
                    projector.second->referencePoints.clear();
                }
                ClearButtonWidth = ImGui::GetItemRectSize().x; //Get the actual width for next frame.
                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
                
                static float LoadButtonWidth = 100.0f;
                pos += LoadButtonWidth + ItemSpacing;
                ImGui::SameLine(ImGui::GetWindowContentRegionMax().x- pos);
                if(ImGui::Button("Load")){
                    projector.second->load("calibrations/" + projector.first);
                }
                LoadButtonWidth = ImGui::GetItemRectSize().x; //Get the actual width for next frame.
                
                static float SaveButtonWidth = 100.0f;
                pos += SaveButtonWidth + ItemSpacing;
                ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - pos);
                if(ImGui::Button("Save")){
                    projector.second->save("calibrations/" + projector.first);
                }
                SaveButtonWidth = ImGui::GetItemRectSize().x; //Get the actual width for next frame.
                ImGui::PopStyleVar();
                
                ofxImGui::EndWindow(mainSettings);
                if(p->cam.isMouseDragging()) mainSettings.mouseOverGui = false;
            }if(!mouseOverWindow) ImGui::PopStyleVar();
            
        }
        
        
    }
    
    ImGui::PopFont();
    
    this->gui.end();
    
    return mainSettings.mouseOverGui;
}
