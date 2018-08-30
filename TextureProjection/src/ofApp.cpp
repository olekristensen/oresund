#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
	ofEnableLighting();
	ofEnableDepthTest();
	
    texture.load("Rhythmus.jpg");
    texture.getTexture().setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
    texture.getTexture().setTextureWrap(GL_CLAMP_TO_BORDER_ARB, GL_CLAMP_TO_BORDER_ARB);
    
    textureProjectionShader.load("TextureProjection");
	
	plane.set(20000,20000,2,2);
    
    camera.enableMouseInput();
}

//--------------------------------------------------------------
void ofApp::update(){
	
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    ofBackground(0);
    
    camera.setFarClip(1000);
    //camera.setPosition(0,0,800);
    //camera.lookAt(ofVec3f());
	camera.begin();
    
    // projector coordinates
    ofVec3f projectorPos = ofVec3f( 0, 0, 400 );
    ofVec3f projectorLookAt = ofVec3f( sin(ofGetElapsedTimef()) * 200, cos(ofGetElapsedTimef())*200, 0);
    ofVec3f projectorUp = ofVec3f( 0, 1, 0);
    
    // projector's view matrix
    ofMatrix4x4 projectionView;
    projectionView.makeLookAtViewMatrix(projectorPos, projectorLookAt, projectorUp);
    
    // projector's perspective materix
    float aspect = float(texture.getWidth()) / texture.getHeight();
    ofMatrix4x4 projectionProj = ofMatrix4x4::newPerspectiveMatrix(30, aspect, 0.2, 2000.0);
    
    // translate into texture space (0.0 - 1.0)
    ofMatrix4x4 projectionTrans = ofMatrix4x4::newIdentityMatrix();
    projectionTrans.scale(ofVec3f(0.5, 0.5, 0.5));
    projectionTrans.translate(ofVec3f(0.5, 0.5, 0.5));
    projectionTrans.scale(ofVec3f(480, 360, 1)); // @note: image texture size
    
    // create our projector matrices
    //ofMatrix4x4 projectorMat = projectionTrans * projectionProj * projectionView; // wrong way!
    ofMatrix4x4 projectorMat =  projectionView * projectionProj * projectionTrans ;
    
    // set our plane's material properties
    ofFloatColor ambientMat = ofFloatColor(0.1,0.1,0.1,1.0);
    ofFloatColor diffuseMat = ofFloatColor(0.5,0.5,0.5,1.0);
    ofFloatColor specularMat = ofFloatColor(0.0,0.0,0.0,1.0);
    float shininessMat = 0;
    
    // finally, our model matrix
    ofMatrix4x4 modelMatrix = ofMatrix4x4::newIdentityMatrix();
    
    textureProjectionShader.begin();
    
    textureProjectionShader.setUniformMatrix4f("modelMatrix", modelMatrix);
    textureProjectionShader.setUniformMatrix4f("projectorMatrix", projectorMat);
    textureProjectionShader.setUniformTexture("projectorTex", texture, 0);
    textureProjectionShader.setUniform3fv("material.ka", &ambientMat.r);
    textureProjectionShader.setUniform3fv("material.kd", &diffuseMat.r);
    textureProjectionShader.setUniform3fv("material.ks", &specularMat.r);
    textureProjectionShader.setUniform1f("material.shininess", shininessMat);
    textureProjectionShader.setUniform3f("light.intensity", 1.0f, 1.0f, 1.0f);
    textureProjectionShader.setUniform3f("light.position", 0.0, 0.0, 30.0);
    
    plane.draw();
    ofDrawSphere(0, 0, 100);
    
    textureProjectionShader.end();
    
    // debugin'
    
    ofSetColor(255);
    ofDrawSphere(projectorPos, 5);
    ofDrawSphere(projectorLookAt, 2);
    ofDrawLine(projectorPos, projectorLookAt);
    
	camera.end();
    
    // preview the texture
    
    float w = 60;
    float h = 60 * 1.0/aspect;
    
    texture.draw(10, 10, w, h);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
