#include "ofMain.h"
#include "ofAppGLFWWindow.h"
#include "ofApp.hpp"

int main() {
    ofGLFWWindowSettings settings;
    settings.setGLVersion(4, 1);
    // settings.setPosition() TODO - set fullscreen at correct positions - query connected displays?
    //ofCreateWindow(pr1settings);
    
    settings.setSize(1280*2,720*2);
    //settings.setSize(1920*2,1080*2);
    //ofCreateWindow(settings);
    shared_ptr<ofAppBaseWindow> controlWindow = ofCreateWindow(settings);
    shared_ptr<ofApp> controlApp(new ofApp);
    
    settings.shareContextWith = controlWindow;
    settings.setSize(1920,1080);
    
    shared_ptr<ofAppBaseWindow> projectorLeftWindow = ofCreateWindow(settings);
    shared_ptr<ofAppBaseWindow> projectorRightWindow = ofCreateWindow(settings);
    
    ofAddListener(projectorRightWindow->events().draw,controlApp.get(),&ofApp::drawProjectorSide);
    ofAddListener(projectorLeftWindow->events().draw,controlApp.get(),&ofApp::drawProjectorWall);
    
    ofRunApp(controlWindow, controlApp);
    ofRunMainLoop();
}
