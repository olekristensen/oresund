#pragma once

//
//  ViewPlane.h
//  bridge
//
//  Created by ole on 10/09/2018.
//

#import "World.hpp"

class ViewPlane {
public:
    
    ofCamera cam;
    
    ofShader & highlightShader, tonemapShader, fxaaShader;
    
    ofFbo hdrPass, tonemapPass, output;
    
    bool renderingHdr = false;
    
    ofFbo::Settings & defaultFboSettings;
    
    ofPlanePrimitive plane;
    ofxAssimp3dPrimitive * viewNode;
    World & world;
    
    ofParameter<float> pViewResolution {"Resolution", 200, 10, 1000};
    ofParameterGroup pg{ "View", pViewResolution };
    
    ofVec3f windowTopLeft, windowBottomLeft, windowBottomRight;
    
    ViewPlane(ofFbo::Settings & defaultFboSettings, ofShader & highlightShader, ofShader & tonemapShader, ofShader & fxaaShader, ofxAssimp3dPrimitive * viewNode, World & world)
    : defaultFboSettings(defaultFboSettings), highlightShader(highlightShader), tonemapShader(tonemapShader), fxaaShader(fxaaShader), viewNode(viewNode), world(world)
    {
        float initialResolution = pViewResolution.get();
        resizeFbos(initialResolution);
        pViewResolution.addListener(this, &ViewPlane::resizeFbos);
        cam.setParent(world.origin);
        cam.setScale(1,1,1);
        cam.setNearClip(0.1);
        cam.setFarClip(10000);
        cam.setPosition(ofVec3f(4.,2.0,-3.25));

    }
    
    void resizeFbos(float & resolution){
        
        ofVec3f viewCornerMin;
        ofVec3f viewCornerMax;
        
        // PLANE

        glm::vec2 planeSize;
        float rotationZ = 0.0;
        float rotationX = 0.0;
        float translationX = 1.0;
        auto boundingBoxSize = viewNode->boundingBox.getSize();
        
        boundingBoxSize *= viewNode->getGlobalScale();
        if(boundingBoxSize.x == 0.0){
            planeSize.x = boundingBoxSize.y;
            planeSize.y = boundingBoxSize.z;
            rotationZ = 90.;
            rotationX = -90.;
        } else if (boundingBoxSize.y == 0.0){
            planeSize.x = boundingBoxSize.x;
            planeSize.y = boundingBoxSize.z;
            rotationZ = -180.;
            rotationX = -90.;
            translationX = -1.0;
        }
        
        plane.setParent(world.origin);
        plane.setWidth(planeSize.x);
        plane.setHeight(planeSize.y);
        plane.setGlobalPosition(viewNode->getGlobalPosition());
        plane.setGlobalOrientation(viewNode->getGlobalOrientation());
        plane.rotateDeg(rotationZ, plane.getZAxis());
        plane.truck(translationX*plane.getWidth()/2.0);
        plane.dolly(plane.getHeight()/2.0);
        plane.rotateDeg(rotationX, plane.getXAxis());
        plane.rotateDeg(180, plane.getXAxis());
        
        // FBO's
        
        auto viewFboSettings = defaultFboSettings;
        
        viewFboSettings.width = floor(plane.getWidth() * resolution);
        viewFboSettings.height = floor(plane.getHeight() * resolution);
        
        ofFbo::Settings firstPassSettings;
        firstPassSettings = viewFboSettings;
        firstPassSettings.minFilter = GL_LINEAR;
        firstPassSettings.maxFilter = GL_LINEAR;
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
        outputSettings.numSamples = 4;
        outputSettings.minFilter = GL_LINEAR;
        outputSettings.maxFilter = GL_LINEAR;
        outputSettings.internalformat = GL_RGBA;
        outputSettings.colorFormats.push_back(GL_RGBA);
        output.allocate(outputSettings);
        
        // CAMERA
        
        //cam.lookAt(toOf(plane.getGlobalPosition()) * toOf(plane.getGlobalTransformMatrix()));

        //cam.setupPerspective();
    }
    
    void begin(bool hdr = true, bool clear = true){
        ofPushStyle();
        ofPushView();
        ofPushMatrix();
        
        renderingHdr = hdr;
        
        // TODO: Orient ViewPortal
        
        float width = plane.getWidth();
        float height = plane.getHeight();
        
        
        windowTopLeft.set(-width/2.,
                              height/2.,
                              0.0);
        windowTopLeft = windowTopLeft * plane.getGlobalTransformMatrix();
        windowBottomLeft.set(-width/2.,
                                 -height/2.,
                                 0.0f);
        windowBottomLeft = windowBottomLeft * plane.getGlobalTransformMatrix();
        windowBottomRight.set(width/2.,
                                   -height/2.,
                                   0);
        windowBottomRight = windowBottomRight * plane.getGlobalTransformMatrix();
        
        // To setup off axis view portal we need to be in the plane's matrix. All window coordinates of the camera are relative to the plane.
        cam.setupOffAxisViewPortal(windowTopLeft , windowBottomLeft, windowBottomRight );
        //cam.setFarClip(10000);
        //cam.setNearClip(.1);
        //cam.setVFlip(false);
        
        if(renderingHdr){
            hdrPass.begin();
            hdrPass.activateAllDrawBuffers();
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
            
            ofPushStyle();
            
            //ofDisableDepthTest();
            //ofEnableAlphaBlending();
            
            /* fxaa
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
             */
            
            ofPopStyle();
            
        } else {
            output.end();
        }
        ofPopMatrix();
        ofPopView();
        ofPopStyle();
    }
        
    void draw(bool hdr = false, bool ldr = false){
        ofPushStyle();
        if(hdr && !ldr){
            hdrPass.getTexture().bind();
            plane.drawFaces();
            hdrPass.getTexture().unbind();
        } else if(ldr) {
            if(hdr){
                output.begin();
                ofClear(0);
                tonemapShader.begin();
                tonemapShader.setUniformTexture("image", hdrPass.getTexture(), 0);
                hdrPass.draw(0, 0);
                tonemapShader.end();
                output.end();
            }
            output.getTexture().bind();
            plane.drawFaces();
            output.getTexture().unbind();
        }
        ofPopStyle();
    }
    
    void drawCameraModel(){
        ofPushStyle();
        ofEnableDepthTest();
        
        ofPushMatrix();
        
        cam.transformGL();
        
        ofFill();
        ofPushMatrix();
        ofRotateXDeg(-90);
        ofDrawCone(0,-.1,0, .1, .2);
        ofPopMatrix();
        
        ofPushMatrix();
        ofMultMatrix(toOf(cam.getProjectionMatrix()).getInverse());
        ofNoFill();
        ofDrawBox(2.0, 2.0, 2.0);
        ofPopMatrix();
        
        cam.restoreTransformGL();
        
        ofPopMatrix();
        ofPopStyle();
    }

    
};
