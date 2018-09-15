//
//  Scene.hpp
//  bridge
//
//  Created by Johan Bichel Lindegaard on 8/30/18.
//

#pragma once
#include "ofMain.h"
#include "ofxAssimp3dPrimitive.hpp"

class World {
public:
    ofNode origin;
    map< string, ofxAssimp3dPrimitive * > primitives;
    map< string, shared_ptr< ofVboMesh> > meshes;
    map< string, shared_ptr< ofVboMesh> > textureMeshes;
};

class Scene {
    
public:
    
    ofParameter<bool> enabled {"Enabled", true};
    ofParameter<bool> enableModel {"Enabled in model", true};
    ofParameter<ofFloatColor> leftColor {"Left Color", ofFloatColor(1.0,1.0,1.0,1.0), ofFloatColor(.0,.0,.0,.0), ofFloatColor(1.0,1.0,1.0,1.0)};
    ofParameter<ofFloatColor> rightColor {"Right Color", ofFloatColor(1.0,1.0,1.0,1.0), ofFloatColor(.0,.0,.0,.0), ofFloatColor(1.0,1.0,1.0,1.0)};
    ofParameter<ofFloatColor> viewColor {"View Color", ofFloatColor(1.0,1.0,1.0,1.0), ofFloatColor(.0,.0,.0,.0), ofFloatColor(1.0,1.0,1.0,1.0)};
    
    ofParameterGroup pgViewAlphas {"View Alphas", leftColor, rightColor, viewColor};

    ofParameterGroup params {"untitled", enabled, pgViewAlphas};
    
    Scene() {
    };
    
    virtual ~Scene() {};
    
    virtual void enable() {};
    virtual void disable() {};
    virtual void reconstruct() {};
    
    void drawSceneModel() {
        if(enabled.get()) {
            drawModel();
        }
    };
    
    virtual void drawModel() {};
    
    virtual void exit(){};
    
    vector<shared_ptr<Scene>> * allScenes;
    
    shared_ptr<Scene> getScene(string n) {
        for(auto s : *allScenes) {
            if(s->getParameters().getName() == n) {
                return s;
            }
        }
    }
    
    void setupScene(ofParameterGroup * mainP, World  * w, vector<shared_ptr<Scene>> * _allScenes ) {
        
        allScenes = _allScenes;
        
        globalParams = mainP;
        world = w;
        
        enabled.addListener(this, &Scene::enableToggled);
        
        setup();
        isSetup = true;
    };
    
    
    template<typename type>
    void setReconstructFlag(type & t) {
        bReconstruct = true;
    }
    
    
    void enableToggled(bool & e) {
        if(e) {
            enable();
        } else {
            disable();
        }
    }
    
    void updateScene() {
        
        if(enabled.get() && isSetup) {
            
            if(bReconstruct) {
                reconstruct();
                bReconstruct = false;
            }
            
            update();
        }
        
    };
    
    void drawScene() {
        
        if(enabled.get() && isSetup) {
            ofPushMatrix();ofPushView();ofPushStyle();
            draw();
            ofPopStyle();ofPopView();ofPopMatrix();
        }
        
    };
    
    ofParameterGroup & getParameters() {
        return params;
    }
    
    ofParameterGroup * globalParams;
    World * world;
    
private:
    
    virtual void setup(){};
    virtual void update(){};
    virtual void draw(){};
    bool isSetup = false;
    bool bReconstruct = true;
    
};

