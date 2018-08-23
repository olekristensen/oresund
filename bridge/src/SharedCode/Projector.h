//
//  Projector.h
//  mapamok
//
//  Created by dantheman on 5/18/14.
//
//


#pragma once
#include "ofMain.h"
#include "Mapamok.h"
#include "DraggablePoints.h"



class Projector{
public:
    ofRectangle viewPort;
    Mapamok mapamok;
    DraggablePoints referencePoints;
    
    ofFbo firstPass, secondPass;
    ofFbo::Settings & defaultFboSettings;
    
    Projector(ofRectangle viewPort, ofFbo::Settings & defaultFboSettings)
    : viewPort(viewPort), defaultFboSettings(defaultFboSettings)
    {
        referencePoints.setClickRadius(5);
        referencePoints.setViewPort(viewPort);
        resizeFbos();
    }

    void resizeFbos(){
        ofFbo::Settings firstPassSettings;
        firstPassSettings = defaultFboSettings;
        firstPassSettings.width = viewPort.getWidth();
        firstPassSettings.height = viewPort.getHeight();
        firstPassSettings.internalformat = GL_RGBA32F;
        firstPassSettings.colorFormats.push_back(GL_RGBA32F);
        firstPass.allocate(firstPassSettings);
        
        ofFbo::Settings secondPassSettings;
        secondPassSettings = defaultFboSettings;
        secondPassSettings.width = viewPort.getWidth();
        secondPassSettings.height = viewPort.getHeight();
        secondPassSettings.internalformat = GL_RGB;
        secondPassSettings.colorFormats.push_back(GL_RGB);
        secondPass.allocate(secondPassSettings);
        
        cam.setControlArea(viewPort);
    }
    
    ofEasyCam cam;
    

};
