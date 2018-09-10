#pragma once

//
//  ViewPlane.h
//  bridge
//
//  Created by ole on 10/09/2018.
//

#import "Scene.hpp"

class ViewPlane {
public:
    
    ofCamera cam;
    
    ofShader & highlightShader, tonemapShader, fxaaShader;
    
    ofFbo hdrPass, tonemapPass, output, mask;
    
    bool renderingHdr = false;
    bool renderingMask = false;
    
    ofFbo::Settings & defaultFboSettings;
    
    ofPlanePrimitive plane;
    ofxAssimp3dPrimitive * viewNode;
    World & world;
    
    ofParameter<float> pViewResolution {"Resolution", 200, 10, 500};
    ofParameterGroup pg{ "View", pViewResolution };

    ViewPlane(ofFbo::Settings & defaultFboSettings, ofShader & highlightShader, ofShader & tonemapShader, ofShader & fxaaShader, ofxAssimp3dPrimitive * viewNode, World & world)
    : defaultFboSettings(defaultFboSettings), highlightShader(highlightShader), tonemapShader(tonemapShader), fxaaShader(fxaaShader), viewNode(viewNode), world(world)
    {
        pViewResolution.addListener(this, &ViewPlane::resizeFbos);
    }
    
    void getBoundingBox(const ofMesh& mesh, ofVec3f& cornerMin, ofVec3f& cornerMax) {
        const vector<glm::vec3>& vertices = mesh.getVertices();
        if(vertices.size() > 0) {
            cornerMin = vertices[0];
            cornerMax = vertices[0];
        }
        for(int i = 0; i < vertices.size(); i++) {
            cornerMin.x = MIN(cornerMin.x, vertices[i].x);
            cornerMin.y = MIN(cornerMin.y, vertices[i].y);
            cornerMin.z = MIN(cornerMin.z, vertices[i].z);
            cornerMax.x = MAX(cornerMax.x, vertices[i].x);
            cornerMax.y = MAX(cornerMax.y, vertices[i].y);
            cornerMax.z = MAX(cornerMax.z, vertices[i].z);
        }
    }
    
    void resizeFbos(float & resolution){
        
        ofVec3f viewCornerMin;
        ofVec3f viewCornerMax;

        // PLANE
        
        getBoundingBox(viewNode->getBakedMesh(), viewCornerMin, viewCornerMax);
        plane.setWidth(fabs(viewCornerMax.z - viewCornerMin.z));
        plane.setHeight(fabs(viewCornerMax.y - viewCornerMin.y));
        plane.setGlobalOrientation(ofQuaternion(90, ofVec3f(0,1,0)));
        plane.setPosition(0, plane.getHeight()/2.0, -plane.getWidth()/2.0);
        
        plane.setParent(world.origin);

        // FBO's
        
        auto viewFboSettings = defaultFboSettings;

        viewFboSettings.width = floor(plane.getWidth() * resolution);
        viewFboSettings.height = floor(plane.getHeight() * resolution);
        
        ofFbo::Settings firstPassSettings;
        firstPassSettings = viewFboSettings;
        firstPassSettings.internalformat = GL_RGBA32F;
        firstPassSettings.colorFormats.push_back(GL_RGBA32F);
        hdrPass.allocate(firstPassSettings);
        
        ofFbo::Settings secondPassSettings;
        secondPassSettings = viewFboSettings;
        secondPassSettings.internalformat = GL_RGBA;
        secondPassSettings.colorFormats.push_back(GL_RGBA);
        tonemapPass.allocate(secondPassSettings);
        
        ofFbo::Settings outputSettings;
        outputSettings = viewFboSettings;
        outputSettings.depthStencilAsTexture = false;
        outputSettings.numSamples = 8;
        outputSettings.minFilter = GL_LINEAR;
        outputSettings.maxFilter = GL_LINEAR;
        outputSettings.internalformat = GL_RGBA;
        outputSettings.colorFormats.push_back(GL_RGBA);
        output.allocate(outputSettings);

        ofFbo::Settings maskSettings;
        maskSettings = viewFboSettings;
        maskSettings.depthStencilAsTexture = false;
        maskSettings.numSamples = 8;
        maskSettings.minFilter = GL_LINEAR;
        maskSettings.maxFilter = GL_LINEAR;
        maskSettings.internalformat = GL_RGB;
        maskSettings.colorFormats.push_back(GL_RGB);
        mask.allocate(maskSettings);

        // CAMERA
        
        cam.setParent(world.origin);
        
        cam.setScale(1,1,1);
        cam.setNearClip(0.1);
        cam.setFarClip(10000);
        cam.lookAt(toOf(plane.getGlobalPosition()) * toOf(plane.getGlobalTransformMatrix()));
        cam.setupPerspective();
    }
    
    void begin(bool hdr = true, bool clear = true, bool masking = false){
        ofPushStyle();
        ofPushView();
        ofPushMatrix();

        renderingHdr = hdr;
        renderingMask = masking;
        
        float width = plane.getWidth();
        float height = plane.getHeight();
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
        plane.ofPlanePrimitive::transformGL();
        cam.setupOffAxisViewPortal(windowTopLeft , windowBottomLeft, windowBottomRight );
        plane.ofPlanePrimitive::restoreTransformGL();
        cam.setFarClip(10000);
        cam.setNearClip(.1);
        cam.setVFlip(false);
        
        ofDisableAlphaBlending();
        ofEnableDepthTest();
        
        if(renderingHdr){
            hdrPass.begin();
            hdrPass.activateAllDrawBuffers();
        } else if(renderingMask) {
            mask.begin();
        } else {
            output.begin();
        }
        if(clear) ofClear(0);
        
        cam.begin();
    };
    
    void end(){
        cam.end();
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
            
        } else if (renderingMask){
            mask.end();
        } else {
            output.end();
        }
        ofPopMatrix();
        ofPopView();
        ofPopStyle();
    }
    
    void draw(bool hdr = false){
        ofPushStyle();
        if(hdr && renderingHdr){
            hdrPass.getTexture().setAlphaMask(mask.getTexture());
            hdrPass.getTexture().bind();
            plane.drawFaces();
            hdrPass.getTexture().unbind();
        } else {
            output.getTexture().bind();
            plane.drawFaces();
            output.getTexture().unbind();
        }
        ofPopStyle();
    }

};
