//TODO:

// add fbo walls
// make meshes into vbo's
// add projectors and viewports
// create shaders
// make text string file format

// separate click radius from draw radius
// abstract DraggablePoint into template
// don't move model when dragging points
// only select one point at a time.


#include "ofMain.h"
#include "ofApp.h"
#include "ofAppGLFWWindow.h"


int main() {
    //TODO: windows for projections

    ofGLWindowSettings settings;
    settings.setGLVersion(4, 1);
    settings.setSize(1920,1080);
    // settings.setPosition() TODO - set fullscreen at correct positions - query connected displays?
    //ofCreateWindow(pr1settings);
    
    shared_ptr<ofAppBaseWindow> projectorLeftWindow = ofCreateWindow(settings);
    shared_ptr<ofAppBaseWindow> projectorRightWindow = ofCreateWindow(settings);

    settings.setSize(1280*2,720*2);
    //ofCreateWindow(settings);
    shared_ptr<ofAppBaseWindow> controlWindow = ofCreateWindow(settings);
    shared_ptr<ofApp> controlApp(new ofApp);
    
    ofAddListener(projectorRightWindow->events().draw,controlApp.get(),&ofApp::drawProjectorRight);
    ofAddListener(projectorLeftWindow->events().draw,controlApp.get(),&ofApp::drawProjectorLeft);
    
    ofRunApp(controlWindow, controlApp);
    ofRunMainLoop();
}
