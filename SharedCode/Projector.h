//
//  Projector.h
//  mapamok
//
//  Created by dantheman on 5/18/14.
//
//


#pragma once
#include "ofMain.h"
#include "Mapamok.h"
#include "DraggablePoints.h"



class Projector{
public:
    ofRectangle viewPort;
    Mapamok mapamok;
    DraggablePoints referencePoints;
};
