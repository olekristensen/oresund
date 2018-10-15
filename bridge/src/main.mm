//TODO:

// make text string file format

// separate click radius from draw radius
// abstract DraggablePoint into template
// don't move model when dragging points
// only select one point at a time.


#include "ofMain.h"
#include "ofAppGLFWWindow.h"
#include "ofApp.hpp"
#import <Cocoa/Cocoa.h>

int main() {

    
    string logFileName = ofFilePath::getUserHomeDir() + "/logs/" + ofGetTimestampString("%Y-%m-%d") + " bridge.log";
    ofLogToFile(logFileName, true);
    const string timestampFormat = "%Y-%m-%d %H:%M:%S.%i";

    ofLogNotice(ofGetTimestampString(timestampFormat)) << "STARTUP";

    ofGLFWWindowSettings controlWindowSettings;
    ofGLFWWindowSettings projectorWindowSettings;
    controlWindowSettings.setGLVersion(4, 1);
    projectorWindowSettings.setGLVersion(4, 1);
    
    shared_ptr<ofAppBaseWindow> controlWindow;
    shared_ptr<ofAppBaseWindow> projectorLeftWindow;
    shared_ptr<ofAppBaseWindow> projectorRightWindow;

    // Get screen widths and heights from Quartz Services
    // See https://developer.apple.com/library/mac/documentation/GraphicsImaging/Reference/Quartz_Services_Ref/index.html
    
    CGDisplayCount displayCount;
    CGDirectDisplayID displays[32];
    
    // Grab the active displays
    CGGetActiveDisplayList(32, displays, &displayCount);
    int numDisplays= displayCount;
    
    // print display info.
    ofLogNotice("DISPLAYS") << numDisplays << " display(s) detected.";
    
    for (int i = 0; i < displayCount; i++){
        ofLogNotice("DISPLAYS") << displays[i] << "\t(" << CGDisplayPixelsWide(displays[i]) << "x" << CGDisplayPixelsHigh(displays[i]) << ")";
    }
    
    CGRect mainDisplayBounds= CGDisplayBounds ( displays[0] );

    controlWindowSettings.setSize(
                                  round(mainDisplayBounds.size.width),
                                  mainDisplayBounds.size.height - 100);
    controlWindowSettings.setPosition(ofVec2f(mainDisplayBounds.origin.x, mainDisplayBounds.origin.y + 80));
    
    controlWindow = ofCreateWindow(controlWindowSettings);

    projectorWindowSettings.shareContextWith = controlWindow;

    
    if(numDisplays == 1 ){

        float divisions = 4.0;
        // one display: palce gui and mainWindow in a default arrangement
        float width = mainDisplayBounds.size.width * (divisions-1) / divisions;
        projectorWindowSettings.setSize(
                             round(width),
                            round((width * (9.0 / 16.0)) / 2.0));
        projectorWindowSettings.setPosition(ofVec2f(round(mainDisplayBounds.size.width / divisions) ,mainDisplayBounds.origin.y));
        projectorWindowSettings.decorated = true;
        
        projectorLeftWindow = ofCreateWindow(projectorWindowSettings);
        projectorRightWindow = ofCreateWindow(projectorWindowSettings);

        ofLogNotice("DISPLAYS") << "configured default one display setup";
        
    } else if (numDisplays == 2){
        
        // three or more displays: palce resizeable gui on first and fill second and third with a couple of projectorWindows.
        
        projectorWindowSettings.decorated = false;
        
        projectorWindowSettings.setSize(mainDisplayBounds.size.width,mainDisplayBounds.size.height);
        projectorWindowSettings.setPosition(ofVec2f(mainDisplayBounds.origin.x,mainDisplayBounds.origin.y));
        
        CGRect secondDisplayBounds= CGDisplayBounds ( displays[1] );
        
        projectorRightWindow = ofCreateWindow(projectorWindowSettings);
        projectorRightWindow->setFullscreen(true);
        
        projectorWindowSettings.setSize(secondDisplayBounds.size.width,secondDisplayBounds.size.height);
        projectorWindowSettings.setPosition(ofVec2f(secondDisplayBounds.origin.x,secondDisplayBounds.origin.y));
        
        projectorLeftWindow = ofCreateWindow(projectorWindowSettings);
        projectorLeftWindow->setFullscreen(true);
        
        CGCaptureAllDisplays();
        NSWindow * leftWindow = (NSWindow *)projectorLeftWindow->getCocoaWindow();
        [leftWindow setLevel:CGShieldingWindowLevel()];
        NSWindow * rightWindow = (NSWindow *)projectorRightWindow->getCocoaWindow();
        [rightWindow setLevel:CGShieldingWindowLevel()];

        ofLogNotice("DISPLAYS") << "configured two display setup";
        
    } else if (numDisplays > 2){
        
        // three or more displays: palce resizeable gui on first and fill second and third with a couple of projectorWindows.
        
        projectorWindowSettings.decorated = false;
        
        CGRect secondDisplayBounds= CGDisplayBounds ( displays[1] );
        
        projectorWindowSettings.setSize(secondDisplayBounds.size.width,secondDisplayBounds.size.height);
        projectorWindowSettings.setPosition(ofVec2f(secondDisplayBounds.origin.x,secondDisplayBounds.origin.y));
        
        projectorLeftWindow = ofCreateWindow(projectorWindowSettings);
        
        CGRect thirdDisplayBounds= CGDisplayBounds ( displays[2] );
        
        projectorWindowSettings.setSize(thirdDisplayBounds.size.width,thirdDisplayBounds.size.height);
        projectorWindowSettings.setPosition(ofVec2f(thirdDisplayBounds.origin.x,thirdDisplayBounds.origin.y));
        
        projectorRightWindow = ofCreateWindow(projectorWindowSettings);
        
        ofLogNotice("DISPLAYS") << "configured three display setup";
        
    }

    shared_ptr<ofApp> controlApp(new ofApp);
    
    controlApp->projectionResolution.x = projectorWindowSettings.getWidth();
    controlApp->projectionResolution.y = projectorWindowSettings.getHeight();

    ofAddListener(projectorRightWindow->events().draw,controlApp.get(),&ofApp::drawProjectorWall);
    ofAddListener(projectorLeftWindow->events().draw,controlApp.get(),&ofApp::drawProjectorSide);
    
    ofRunApp(controlWindow, controlApp);
    projectorLeftWindow->makeCurrent();
    projectorRightWindow->makeCurrent();

    projectorLeftWindow->hideCursor();
    projectorRightWindow->hideCursor();
    ofRunMainLoop();
}
