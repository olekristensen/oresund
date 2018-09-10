//
//  Scene.hpp
//  bridge
//
//  Created by Johan Bichel Lindegaard on 8/30/18.
//

#pragma once
#include "ofMain.h"

class World {
public:
    ofNode origin;
};

class Scene {
    
public:
    
    ofParameter<bool> enabled {"enabled", true};
    ofParameter<bool> enabledDraw {"draw", true};
    ofParameter<bool> enabledDrawModel {"draw model", true};
    ofParameter<float> alpha {"alpha", 1.0, 0.0, 1.0};
    
    //ofParameter<bool> qlab {"add to qlab", false};
    ofParameterGroup params {"untitled", enabled, alpha, enabledDraw, enabledDrawModel};
    
    // add dynamic draw order
    float time;
    
    //    ofxTimeline * mainTimeline;
    //    ofxTLCurves * tlenabled;
    
    Scene() {
    };
    
    virtual ~Scene() {};
    
    virtual void enable() {};
    virtual void disable() {};
    virtual void reconstruct() {};
    
    void drawSceneModel() {
        if(enabledDrawModel.get() && enabled.get()) {
            drawModel();
        }
    };
    
    virtual void drawModel() {};
    
    virtual void exit(){};
    
    vector<shared_ptr<Scene>> * allScenes;
    
    shared_ptr<Scene> getScene(string n) {
        for(auto s : *allScenes) {
            //cout<<s->getParameters().getName()<<endl;
            if(s->getParameters().getName() == n) {
                return s;
            }
        }
    }
    
    void setupScene(ofParameterGroup * mainP, World  * w, vector<shared_ptr<Scene>> * _allScenes ) {
        
        allScenes = _allScenes;
        
        globalParams = mainP;
        world = w;
        
        //enabled.addListener(this, &ofxStereoscopy::Scene::enableToggled);
        
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
        
        if(enabled.get() && isSetup && enabledDraw.get()) {
            ofPushMatrix();ofPushView();ofPushStyle();
            draw();
            ofPopStyle();ofPopView();ofPopMatrix();
        }
        
    };
    
    ofParameterGroup & getParameters() {
        return params;
    }
    
    /*ofVec3f getDancerPositionNormalised(int dancer){
        return globalParams->getGroup("scenes").getGroup("roomScene").getGroup("dancers").getVec3f(dancer==1?"one":"two");
    }*/
    
    /*ofVec3f dp(int dancer = 0){
        if(dancer > 0){
            return getDancerPositionNormalised(dancer) * globalParams->getVec3f("stage_size_cm");
        } else {
            return (dp(1)+dp(2))/2.0;
        }
    }*/
    
    /*ofVec3f getWorldSize() {
        return globalParams->getVec3f("stage_size_cm").get();
    }*/
    
    virtual void drawGui() {};
    virtual void setupGui() {};
    
    ofParameterGroup * globalParams;
    World * world;
    
private:
    
    virtual void setup(){};
    virtual void update(){};
    virtual void draw(){};
    bool isSetup = false;
    bool bReconstruct = true;
    
};

