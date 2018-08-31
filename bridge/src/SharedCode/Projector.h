#pragma once

//
//  Projector.h
//  mapamok
//
//  Created by dantheman on 5/18/14.
//
//

#include "ofMain.h"
#include "Mapamok.h"
#include "DraggablePoints.h"



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
        firstPassSettings.numSamples = 8;
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
    
    void load(string filePath) {
        mapamok.load(filePath);
        //TODO: reference points persistence
    }
    
    void save(string filePath) {
        mapamok.save(filePath);
        //TODO: reference points persistence
    }
    
    ofEasyCam cam;

};
