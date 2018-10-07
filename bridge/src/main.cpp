//TODO:

// make text string file format

// separate click radius from draw radius
// abstract DraggablePoint into template
// don't move model when dragging points
// only select one point at a time.


#include "ofMain.h"
#include "ofAppGLFWWindow.h"
#include "ofApp.hpp"

int main() {
    
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
    cout<<numDisplays<<" display(s) detected."<<endl;
    
    for (int i = 0; i < displayCount; i++){
        cout << displays[i] << "\t(" << CGDisplayPixelsWide(displays[i]) << "x" << CGDisplayPixelsHigh(displays[i]) << ")"<< endl;
    }
    
    CGRect mainDisplayBounds= CGDisplayBounds ( displays[0] );

    controlWindowSettings.setSize(
                                  round(mainDisplayBounds.size.width),
                                  mainDisplayBounds.size.height - 100);
    controlWindowSettings.setPosition(ofVec2f(mainDisplayBounds.origin.x, mainDisplayBounds.origin.y + 80));
    
    controlWindow = ofCreateWindow(controlWindowSettings);

    projectorWindowSettings.shareContextWith = controlWindow;

    
    if(numDisplays < 3 ){

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

        cout << "configured default one display setup" << endl;
        
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

        cout << "configured three display setup" << endl;

    }
 
    shared_ptr<ofApp> controlApp(new ofApp);
    
    controlApp->projectionResolution.x = projectorWindowSettings.getWidth();
    controlApp->projectionResolution.y = projectorWindowSettings.getHeight();

    ofAddListener(projectorRightWindow->events().draw,controlApp.get(),&ofApp::drawProjectorWall);
    ofAddListener(projectorLeftWindow->events().draw,controlApp.get(),&ofApp::drawProjectorSide);
    
    ofRunApp(controlWindow, controlApp);
    ofRunMainLoop();
}
