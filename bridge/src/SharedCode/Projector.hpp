#pragma once

//
//  Projector.h
//  mapamok
//
//  Created by dantheman on 5/18/14.
//
//

#include "ofMain.h"
#include "Mapamok.hpp"
#include "DraggablePoints.hpp"

class Projector{
    
public:
    ofRectangle viewPort;
    Mapamok mapamok;
    DraggablePoints referencePoints;
    ofMesh cornerMeshImage;
    
    ofShader & highlightShader, tonemapShader, fxaaShader;
    
    ofFbo hdrPass, tonemapPass, output;
    
    bool renderingHdr = false;
    bool forcingEasyCam = false;
    
    ofFbo::Settings & defaultFboSettings;
    
    ofEasyCam cam;
    
    enum class CalibrationMeshDrawMode
    {
        Faces,
        Outline,
        WireframeFull,
        WireframeOccluded
    };
    
    const vector<string> CalibrationMeshDrawModeLabels = { "Faces", "Outline", "Wireframe full", "Wireframe occluded" };
    
    enum class CalibrationMeshColorMode
    {
        Neutral,
        ProjectorColor,
        MeshColor
    };
    
    const vector<string> CalibrationMeshColorModeLabels = { "Neutral", "ProjectorColor", "MeshColor" };
    
    ofParameter<bool> pCalibrationEdit {"Calibrate", true};
    ofParameter<bool> pCalibrationDrawScales {"Draw Scales", true};
    ofParameter<int>  pCalibrationMeshDrawMode{ "Render Mode", static_cast<int>(CalibrationMeshDrawMode::Faces) };
    ofParameter<int>  pCalibrationMeshColorMode{ "Color Mode", static_cast<int>(CalibrationMeshColorMode::MeshColor) };
    ofParameter<ofFloatColor> pCalibrationProjectorColor{ "Projector color", ofFloatColor(1.0,1.0,1.0,1.0), ofFloatColor(0.0,0.0,0.0,0.0),ofFloatColor(1.0,1.0,1.0,1.0)};
    ofParameter<int>  pCalibrationHighlightIndex{ "Highlight", 0, 0, 0 };
    ofParameterGroup  pgCalibration{ "Calibration", pCalibrationEdit, pCalibrationDrawScales, pCalibrationMeshDrawMode, pCalibrationMeshColorMode,pCalibrationProjectorColor, pCalibrationHighlightIndex };
    ofParameterGroup pg;
    
    
    Projector(ofRectangle viewPort, ofFbo::Settings & defaultFboSettings, ofShader & highlightShader, ofShader & tonemapShader, ofShader & fxaaShader)
    : viewPort(viewPort), defaultFboSettings(defaultFboSettings), highlightShader(highlightShader), tonemapShader(tonemapShader), fxaaShader(fxaaShader)
    {
        referencePoints.setClickRadius(5);
        referencePoints.setViewPort(viewPort);
        
        resizeFbos();
    }
    
    void resizeFbos(){
        
        int height = fmaxf(8,viewPort.getHeight());
        int width = fmaxf(8,viewPort.getWidth());
        
        ofFbo::Settings firstPassSettings;
        firstPassSettings = defaultFboSettings;
        firstPassSettings.width = width;
        firstPassSettings.height = height;
        firstPassSettings.internalformat = GL_RGBA32F;
        firstPassSettings.colorFormats.push_back(GL_RGBA32F);
        hdrPass.allocate(firstPassSettings);
        
        ofFbo::Settings secondPassSettings;
        secondPassSettings = defaultFboSettings;
        secondPassSettings.width = width;
        secondPassSettings.height = height;
        secondPassSettings.internalformat = GL_RGB;
        secondPassSettings.colorFormats.push_back(GL_RGB);
        tonemapPass.allocate(secondPassSettings);
        
        ofFbo::Settings outputSettings;
        outputSettings = defaultFboSettings;
        outputSettings.width = width;
        outputSettings.height = height;
        outputSettings.depthStencilAsTexture = false;
        outputSettings.numSamples = 8;
        outputSettings.internalformat = GL_RGB;
        outputSettings.colorFormats.push_back(GL_RGB);
        output.allocate(outputSettings);
        
        cam.setControlArea(viewPort);
    }
    void update(ofMesh & cornerMesh){
        
        cornerMeshImage = cornerMesh;
        
        if(pCalibrationEdit){
            referencePoints.enableControlEvents();
            //referencePoints.enableDrawEvent();
        } else {
            referencePoints.disableControlEvents();
            //referencePoints.disableDrawEvent();
        }
        
        project(cornerMeshImage, cam, viewPort - viewPort.getPosition());
        
        if(cornerMesh.getNumVertices() != referencePoints.size()) {
            referencePoints.clear();
            for(int i = 0; i < cornerMeshImage.getNumVertices(); i++) {
                referencePoints.add(ofVec2f(cornerMeshImage.getVertex(i)));
            }
        }
        
        
        // otherwise, update the points
        for(int i = 0; i < referencePoints.size(); i++) {
            DraggablePoint& cur = referencePoints.get(i);
            if(!cur.hit) {
                cur.position = cornerMeshImage.getVertex(i);
            }
        }
        
        // calculating the 3d mesh
        vector<ofVec2f> imagePoints;
        vector<ofVec3f> objectPoints;
        for(int j = 0; j < referencePoints.size(); j++) {
            DraggablePoint& cur =  referencePoints.get(j);
            if(cur.hit) {
                imagePoints.push_back(cur.position);
                objectPoints.push_back(cornerMesh.getVertex(j));
            }
        }
        // should only calculate this when the points are updated
        mapamok.update(viewPort.width, viewPort.height, imagePoints, objectPoints);
        
    }
    
    void project(ofMesh& mesh, const ofCamera& camera, ofRectangle viewport) {
        ofMatrix4x4 modelViewProjectionMatrix = camera.getModelViewProjectionMatrix(viewport);
        viewport.width /= 2;
        viewport.height /= 2;
        for(int i = 0; i < mesh.getNumVertices(); i++) {
            glm::vec3 & glmCur = mesh.getVerticesPointer()[i];
            ofVec3f cur = ofVec3f(glmCur);
            ofVec3f CameraXYZ = cur * modelViewProjectionMatrix;
            glmCur.x = (CameraXYZ.x + 1.0f) * viewport.width + viewport.x;
            glmCur.y = (1.0f - CameraXYZ.y) * viewport.height + viewport.y;
            glmCur.z = CameraXYZ.z / 2;
            
        }
    }
    
    void load(string filePath) {
        mapamok.load(filePath);
        referencePoints.load(filePath);
    }
    
    void save(string filePath) {
        mapamok.save(filePath);
        referencePoints.save(filePath);
    }
    
    void renderCalibrationEditor(ofxAssimp3dPrimitive * calibrationPrimitive){
        if(pCalibrationEdit){
            begin(false, false);
            ofPushStyle();
            ofSetLineWidth(3.0);
            ofEnableSmoothing();
            
            // SCALES
            if(pCalibrationDrawScales){
                ofDrawGrid(1.0, 10, true);
                ofDrawAxis(1.2);
            }
            
            // MESHES
            
            auto colorMode = static_cast<CalibrationMeshColorMode>(this->pCalibrationMeshColorMode.get());
            auto drawMode = static_cast<CalibrationMeshDrawMode>(this->pCalibrationMeshDrawMode.get());
            
            for(int mIndex = 0 ; mIndex < calibrationPrimitive->textureNames.size(); mIndex++){
                ofPushStyle();
                ofColor c;
                if(colorMode == CalibrationMeshColorMode::MeshColor){
                    float step = 1.0/calibrationPrimitive->textureNames.size();
                    c.setHsb(360.0*((mIndex*step)-(step*2.0/3.0)), 255, 255);
                    ofEnableAlphaBlending();
                    prepareRender(true, false, false);
                }
                else if(colorMode == CalibrationMeshColorMode::ProjectorColor){
                    c.set(pCalibrationProjectorColor.get());
                    ofEnableAlphaBlending();
                    prepareRender(false, false, false);
                }
                else if(colorMode == CalibrationMeshColorMode::Neutral){
                    c.setHsb(0, 0, 255, 32);
                    ofEnableAlphaBlending();
                    prepareRender(false, false, false);
                }
                ofSetColor(c);
                auto primitives = calibrationPrimitive->getPrimitivesWithTextureIndex(mIndex);
                for(auto p : primitives){
                    if(mIndex == pCalibrationHighlightIndex){
                        highlightShader.begin();
                        highlightShader.setUniform1f("elapsedTime", ofGetElapsedTimef());
                        prepareRender(true, false, false);
                    }
                    if(drawMode == CalibrationMeshDrawMode::Faces) {
                        p->recursiveDraw(OF_MESH_FILL);
                    } else if(drawMode == CalibrationMeshDrawMode::WireframeFull) {
                        p->recursiveDraw(OF_MESH_WIREFRAME);
                    } else if(drawMode == CalibrationMeshDrawMode::Outline || drawMode == CalibrationMeshDrawMode::WireframeOccluded) {
                        prepareRender(true, true, false);
                        glEnable(GL_POLYGON_OFFSET_FILL);
                        float lineWidth = ofGetStyle().lineWidth;
                        if(drawMode == CalibrationMeshDrawMode::Outline) {
                            glPolygonOffset(-lineWidth, -lineWidth);
                        } else if(drawMode == CalibrationMeshDrawMode::WireframeOccluded) {
                            glPolygonOffset(+lineWidth, +lineWidth);
                        }
                        glColorMask(false, false, false, false);
                        p->recursiveDraw(OF_MESH_FILL);
                        glColorMask(true, true, true, true);
                        glDisable(GL_POLYGON_OFFSET_FILL);
                        p->recursiveDraw(OF_MESH_WIREFRAME);
                        prepareRender(false, false, false);
                    }
                    if(mIndex == pCalibrationHighlightIndex){
                        highlightShader.end();
                    }
                }
                ofPopStyle();
            }
            ofPopStyle();
            
            ofEnableAlphaBlending();
            ofDisableDepthTest();
            
            if(calibrationReady()){
                // Extra model wireframe when calibration ready
                ofSetColor(255,64);
                cam.begin();
                calibrationPrimitive->recursiveDraw(OF_MESH_WIREFRAME);
                cam.end();
            }
            
            // lines
            ofPushView();
            ofPushMatrix();
            ofPushStyle();{
                ofViewport(0,0, viewPort.width, viewPort.height, true);
                ofSetupScreenPerspective();
                if(!calibrationReady()){
                    ofTranslate(0, viewPort.height/2.0);
                    ofScale(1.0, -1.0, 1.0);
                    ofTranslate(0, -viewPort.height/2.0);
                }
                
                for(int i = 0; i < referencePoints.size(); i++) {
                    DraggablePoint& cur = referencePoints.get(i);
                    ofSetColor(0, 64);
                    ofFill();
                    ofDrawCircle(cornerMeshImage.getVertex(i), 3);
                    ofSetColor(17, 133, 247, 191);
                    ofNoFill();
                    ofDrawLine(cur.position, cornerMeshImage.getVertex(i));
                    ofDrawCircle(cornerMeshImage.getVertex(i), 2);
                }
                
            } ofPopStyle();
            ofPopMatrix();
            ofPopView();
            
            // Points
            ofPushMatrix();
            ofPushStyle();
            ofSetColor(255,255);
            referencePoints.draw(!calibrationReady());
            
            ofPopStyle();
            ofPopMatrix();
            
            end();
        }
    }
    
    void begin(bool hdr = true, bool forceEasyCam = false, bool clear = true){
        renderingHdr = hdr;
        forcingEasyCam = forceEasyCam;
        
        ofDisableAlphaBlending();
        ofEnableDepthTest();
        
        if(renderingHdr){
            hdrPass.begin();
            hdrPass.activateAllDrawBuffers();
        } else {
            output.begin();
        }
        if(clear) ofClear(0);
        
        if(forceEasyCam){
            cam.begin(viewPort - viewPort.getPosition());
        } else {
            if(mapamok.calibrationReady)
                mapamok.begin(viewPort - viewPort.getPosition());
            else
                cam.begin(viewPort - viewPort.getPosition());
        }
    }
    
    void end(){
        if(forcingEasyCam){
            cam.end();
        } else{
            if(mapamok.calibrationReady)
                mapamok.end();
            else
                cam.end();
        }
        if(renderingHdr){
            hdrPass.end();
            
            ofDisableDepthTest();
            ofEnableAlphaBlending();
            
            tonemapPass.begin();
            ofClear(0);
            
            tonemapShader.begin();
            
            tonemapShader.setUniformTexture("image", hdrPass.getTexture(), 0);
            hdrPass.draw(0, 0);
            tonemapShader.end();
            tonemapPass.end();
            
            output.begin();
            fxaaShader.begin();
            fxaaShader.setUniformTexture("image", tonemapPass.getTexture(), 0);
            fxaaShader.setUniform2f("texel", 1.25 / float(tonemapPass.getWidth()), 1.25 / float(tonemapPass.getHeight()));
            tonemapPass.draw(0,0);
            fxaaShader.end();
            output.end();
            
        } else {
            output.end();
        }
    }
    
    void draw(){
        ofPushStyle();
        output.draw(viewPort);
        ofPopStyle();
    }
    
    void prepareRender(bool useDepthTesting, bool useBackFaceCulling, bool useFrontFaceCulling) {
        ofSetDepthTest(useDepthTesting);
        if(useBackFaceCulling || useFrontFaceCulling) {
            glEnable(GL_CULL_FACE);
            if(useBackFaceCulling && useFrontFaceCulling) {
                glCullFace(GL_FRONT_AND_BACK);
            } else if(useBackFaceCulling) {
                glCullFace(GL_BACK);
            } else if(useFrontFaceCulling) {
                glCullFace(GL_FRONT);
            }
        } else {
            glDisable(GL_CULL_FACE);
        }
    }
    
    
    
    bool calibrationReady(){
        return mapamok.calibrationReady;
    }
    
    ofCamera & getCam(){
        if(calibrationReady()){
            return mapamok.cam;
        } else {
            return cam;
        }
    }
    
};
