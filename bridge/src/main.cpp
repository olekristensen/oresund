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
#include "ofAppGLFWWindow.h"
#include "ofApp.h"

int main() {
    //TODO: windows for projections
    ofGLWindowSettings settings;
    settings.setGLVersion(4, 1);
    settings.setSize(1280*2,720*2);
    ofCreateWindow(settings);
    ofRunApp(new ofApp());
}
