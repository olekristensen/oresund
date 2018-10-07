//
//  World.hpp
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
    ofNode offset;
    map< string, ofxAssimp3dPrimitive * > primitives;
    map< string, shared_ptr< ofVboMesh> > meshes;
    map< string, shared_ptr< ofVboMesh> > textureMeshes;
};
