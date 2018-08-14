// separate click radius from draw radius
// abstract DraggablePoint into template
// don't move model when dragging points
// only select one point at a time.

#include "ofMain.h"
#include "ofAppGLFWWindow.h"
#include "ofxAssimpModelLoader.h"
#include "ofxUI.h"
#include "Projector.h"
#include "Mapamok.h"
#include "DraggablePoints.h"
#include "MeshUtils.h"
#include "GLFW/glfw3.h"
#include "ofxImGui.h"


class ofApp : public ofBaseApp {
public:
    
    // GUI
    
    ofxImGui::Gui imGui;
    
    ofxUICanvas* gui;
    ofxUIRadio* renderMode;
    
    bool editToggle = true;
    bool loadButton = false;
    bool saveButton = false;
    float backgroundBrightness = 0;
    bool useShader = false;
    
    ofEasyCam cam;
    ofRectangle mViewPortLeft;
    ofRectangle mViewPortRight;
    ofRectangle mViewPort;
    
    
    // MAPAMOK
    
    Mapamok mapamok;
    
    const float cornerRatio = 1.0;
    // const float cornerRatio = .1;
    const int cornerMinimum = 3;
    const float mergeTolerance = .001;
    const float selectionMergeTolerance = .001;
    
    vector<Projector> mProjectors;
    
    ofxAssimpModelLoader model;
    ofVboMesh mesh, cornerMesh;
    
    // RENDER MODES
    enum {
        RENDER_MODE_FACES = 0,
        RENDER_MODE_OUTLINE,
        RENDER_MODE_WIREFRAME_FULL,
        RENDER_MODE_WIREFRAME_OCCLUDED
    };

    void setup() {
        
        // WINDOW SETUP
        
        ofSetWindowTitle("Öresund");
        
        const int projectorWidth = 640;
        const int projectorHeight = 480;
        
        int screenWidth = ofGetScreenWidth();
        int screenHeight = ofGetScreenHeight();
        int monitorCount;
        
        GLFWmonitor**  monitors = glfwGetMonitors(&monitorCount);
        
        if(monitorCount == 2){
            for (int iC=0; iC < monitorCount; iC++){
                if(iC > 0){
                    // set to position on second screen
                    int xM; int yM;
                    glfwGetMonitorPos(monitors[iC], &xM, &yM);
                    const GLFWvidmode * desktopMode = glfwGetVideoMode(monitors[iC]);
                    ofRectangle monitorRect(xM, yM, desktopMode->width, desktopMode->height);
                    ofSetWindowPosition(xM+100, yM+100);
                }
            }
        } else {
            ofSetWindowPosition(100, 100);
        }
        ofSetWindowShape(projectorWidth*2, projectorHeight);

        ofSetVerticalSync(true);
        
        // VIEWPORTS
        
        mViewPortLeft = ofRectangle(0, 0, projectorWidth, projectorHeight);
        mViewPortRight = ofRectangle(projectorWidth, 0, projectorWidth, projectorHeight);
        mViewPort = ofRectangle(0, 0, projectorWidth, projectorHeight);
        
        // PROJECTORS
        
        mProjectors.assign(2, Projector());
        
        mProjectors[0].viewPort = mViewPortLeft;
        mProjectors[0].referencePoints.setClickRadius(5);
        mProjectors[0].referencePoints.setViewPort(mViewPortLeft);
        mProjectors[0].referencePoints.enableControlEvents();
        mProjectors[0].referencePoints.enableDrawEvent();
        
        mProjectors[1].viewPort = mViewPortRight;
        mProjectors[1].referencePoints.setClickRadius(5);
        mProjectors[1].referencePoints.setViewPort(mViewPortRight);
        mProjectors[1].referencePoints.enableControlEvents();
        mProjectors[1].referencePoints.enableDrawEvent();


        // GUI SETUP
        
        setupGui();
        
        // LOAD MODEL
        
        if(ofFile::doesFileExist("model.dae")) {
            loadModel("model.dae");
        } else if(ofFile::doesFileExist("model.3ds")) {
            loadModel("model.3ds");
        }
        
        // CAMERA
        
        cam.setDistance(1);
        cam.setNearClip(.1);
        cam.setFarClip(10);
        cam.setVFlip(false);
        
    }
    
    void setupGui() {
        gui = new ofxUICanvas();
        
        ofColor
        cb(64, 192),
        co(192, 192),
        coh(128, 192),
        cf(240, 255),
        cfh(128, 255),
        cp(96, 192),
        cpo(255, 192);
        gui->setUIColors(cb, co, coh, cf, cfh, cp, cpo);
        
        gui->addSpacer();
        gui->addLabel("Calibration");
        gui->addToggle("Edit", &editToggle);
        gui->addButton("Load", &loadButton);
        gui->addButton("Save", &saveButton);
        
        gui->addSpacer();
        gui->addLabel("Render");
        vector<string> renderModes;
        renderModes.push_back("Faces");
        renderModes.push_back("Depth Edges");
        renderModes.push_back("Full wireframe");
        renderModes.push_back("Occluded wireframe");
        renderMode = gui->addRadio("Render", renderModes, OFX_UI_ORIENTATION_VERTICAL, OFX_UI_FONT_MEDIUM);
        renderMode->activateToggle(renderModes[0]);
        
        gui->addSpacer();
        gui->addMinimalSlider("Background", 0, 255, &backgroundBrightness);
        gui->addToggle("Use shader", &useShader);
        
        gui->autoSizeToFitWidgets();
        
    }
    int getSelection(ofxUIRadio* radio) {
        vector<ofxUIToggle*> toggles = radio->getToggles();
        for(int i = 0; i < toggles.size(); i++) {
            if(toggles[i]->getValue()) {
                return i;
            }
        }
        return -1;
    }
    
    void render() {
        int renderModeSelection = getSelection(renderMode);
        if(renderModeSelection == RENDER_MODE_FACES) {
            //            ofEnableDepthTest();
            ofSetColor(255, 128);
            mesh.drawFaces();
            //            ofDisableDepthTest();
        } else if(renderModeSelection == RENDER_MODE_WIREFRAME_FULL) {
            mesh.drawWireframe();
        } else if(renderModeSelection == RENDER_MODE_OUTLINE || renderModeSelection == RENDER_MODE_WIREFRAME_OCCLUDED) {
            prepareRender(true, true, false);
            glEnable(GL_POLYGON_OFFSET_FILL);
            float lineWidth = ofGetStyle().lineWidth;
            if(renderModeSelection == RENDER_MODE_OUTLINE) {
                glPolygonOffset(-lineWidth, -lineWidth);
            } else if(renderModeSelection == RENDER_MODE_WIREFRAME_OCCLUDED) {
                glPolygonOffset(+lineWidth, +lineWidth);
            }
            glColorMask(false, false, false, false);
            mesh.drawFaces();
            glColorMask(true, true, true, true);
            glDisable(GL_POLYGON_OFFSET_FILL);
            mesh.drawWireframe();
            prepareRender(false, false, false);
        }
    }
    
    void drawEdit() {
        for(int i = mProjectors.size()-1; i >= 0; i--){
            cam.begin(mProjectors[i].viewPort);
            ofPushStyle();
            ofSetColor(255, 128);
            mesh.drawFaces();
            ofSetColor(0,0,255, 128);
            ofPopStyle();
            cam.end();
            //mViewPort = mProjectors[i].viewPort;
            
            ofMesh cornerMeshImage = cornerMesh;
            // should only update this if necessary
            project(cornerMeshImage, cam, mViewPort);
            
            // if this is a new mesh, create the points
            // should use a "dirty" flag instead allowing us to reset manually
            if(cornerMesh.getNumVertices() != mProjectors[i].referencePoints.size()) {
                mProjectors[i].referencePoints.clear();
                for(int j = 0; j < cornerMeshImage.getNumVertices(); j++) {
                    mProjectors[i].referencePoints.add(ofVec3f(cornerMeshImage.getVertex(j)));
                }
            }
            
            // if the calibration is ready, use the calibration to find the corner positions
            
            // otherwise, update the points
            for(int j = 0; j <  mProjectors[i].referencePoints.size(); j++) {
                DraggablePoint& cur =  mProjectors[i].referencePoints.get(j);
                if(!cur.hit) {
                    cur.position = cornerMeshImage.getVertex(j);
                } else {
                    ofPushMatrix();
                    ofTranslate(mProjectors[i].viewPort.getTopLeft());
                    ofDrawLine(cur.position, cornerMeshImage.getVertex(j));
                    ofPopMatrix();
                }
            }
            
            // calculating the 3d mesh
            vector<ofVec2f> imagePoints;
            vector<ofVec3f> objectPoints;
            for(int j = 0; j <  mProjectors[i].referencePoints.size(); j++) {
                DraggablePoint& cur =  mProjectors[i].referencePoints.get(j);
                if(cur.hit) {
                    imagePoints.push_back(cur.position);
                    objectPoints.push_back(cornerMesh.getVertex(j));
                }
            }
            // should only calculate this when the points are updated
            mProjectors[i].mapamok.update(mProjectors[i].viewPort.width, mProjectors[i].viewPort.height, imagePoints, objectPoints);
        }
    }
    
    void draw() {
        ofBackground(backgroundBrightness);
        ofSetColor(255);
        
        if(editToggle) {
            drawEdit();
        }
        for(int i = 0; i < mProjectors.size(); i++){
            if(mProjectors[i].mapamok.calibrationReady) {
                mProjectors[i].mapamok.begin(mProjectors[i].viewPort);
                
                if(editToggle) {
                    ofSetColor(255, 128);
                } else {
                    ofSetColor(255);
                }
                
                render();
                
                mProjectors[i].mapamok.end();
            }
        }
        ofPushStyle();
        ofNoFill();
        ofSetColor(255, 0, 255);
        ofDrawRectangle(mViewPortLeft);
        ofDrawRectangle(mViewPortRight);
        ofPopStyle();
        ofDrawBitmapString(ofToString((int) ofGetFrameRate()), 10, ofGetHeight() - 40);
    }
    
    void loadModel(string filename) {
        model.loadModel(filename);
        vector<ofMesh> meshes = getMeshes(model);
        
        // join all the meshes
        mesh = ofVboMesh();
        mesh = joinMeshes(meshes);
        ofLogNotice("loadModel") << "Joined meshes:\t\t " << mesh.getNumVertices() << "\t vertices" << endl;
        ofVec3f cornerMin, cornerMax;
        getBoundingBox(mesh, cornerMin, cornerMax);
        centerAndNormalize(mesh, cornerMin, cornerMax);
        
        // normalize submeshes before any further processing
        for(int i = 0; i < meshes.size(); i++) {
            centerAndNormalize(meshes[i], cornerMin, cornerMax);
        }
        
        
        //mesh.clearColors();
        int numVerts = mesh.getNumVertices();
        ofLogNotice("loadModel") << "Centered and normalised:\t " << mesh.getNumVertices() << "\t vertices" << endl;
        for(int i= 0; i < numVerts; i++){
            if(i <= numVerts/2){
                //   mesh.addColor(ofFloatColor(0, 1., 1.));
            }else{
                //   mesh.addColor(ofFloatColor(1., 0., 1.));
            }
        }
        
        // merge
        // another good metric for finding corners is if there is a single vertex touching
        // the wall of the bounding box, that point is a good control point
        cornerMesh = ofVboMesh();
        for(int i = 0; i < meshes.size(); i++) {
            ofMesh mergedMesh = mergeNearbyVertices(meshes[i], mergeTolerance);
            vector<unsigned int> cornerIndices = getRankedCorners(mergedMesh);
            ofLogNotice("loadModel") << "Mesh " <<  i << ":\t " << cornerIndices.size() << "\t corner indices" << endl;
            int n = cornerIndices.size() * cornerRatio;
            ofLogNotice("loadModel") << "Mesh " <<  i << ":\t " << n << "\t corner indices after ratio" << endl;
            n = MIN(MAX(n, cornerMinimum), cornerIndices.size());
            ofLogNotice("loadModel") << "Mesh " <<  i << ":\t " << n << "\t corner indices after maxing" << endl;
            for(int j = 0; j < n; j++) {
                int index = cornerIndices[j];
                const ofVec3f& corner = mergedMesh.getVertices()[index];
                cornerMesh.addVertex(corner);
            }
        }
        cornerMesh = mergeNearbyVertices(cornerMesh, selectionMergeTolerance);
        cornerMesh.setMode(OF_PRIMITIVE_POINTS);
    }
    
    void dragEvent(ofDragInfo dragInfo) {
        if(dragInfo.files.size() == 1) {
            string filename = dragInfo.files[0];
            loadModel(filename);
        }
    }
    
    void keyPressed(int key) {
        if(key == 'f') {
            ofToggleFullscreen();
        }
        if(key == '\t') {
            gui->toggleVisible();
        }
    }
};

int main() {
    ofGLWindowSettings windowSettings;
    windowSettings.setGLVersion(4,1);
    windowSettings.windowMode = OF_WINDOW;
    windowSettings.setSize(1280*2, 1000);
    
    ofCreateWindow(windowSettings);
    
    ofRunApp(new ofApp());
}
