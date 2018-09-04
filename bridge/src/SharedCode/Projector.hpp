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
    
    ofFbo renderPass, tonemapPass, output;
    ofFbo::Settings & defaultFboSettings;
    
    Projector(ofRectangle viewPort, ofFbo::Settings & defaultFboSettings)
    : viewPort(viewPort), defaultFboSettings(defaultFboSettings)
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
        renderPass.allocate(firstPassSettings);
        
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
        outputSettings.internalformat = GL_RGB;
        outputSettings.colorFormats.push_back(GL_RGB);
        output.allocate(outputSettings);
        
        cam.setControlArea(viewPort);
    }
    void update(ofMesh & cornerMesh){
        
        ofMesh cornerMeshImage = cornerMesh;
        
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
            } else {
                ofDrawLine(cur.position, cornerMeshImage.getVertex(i));
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
    
    ofEasyCam cam;
    
};
