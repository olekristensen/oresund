//
//  tracker.hpp
//  exampleLiveCamera
//
//  Created by ole on 24/09/2018.
//

#pragma once

#include "ofMain.h"
#include "ofxCv.h"

class head : public ofIcoSpherePrimitive {
public:
    enum class TRACKING_STATE {
        READY,
        TRACKING,
        LOST
    };
    
    TRACKING_STATE state = TRACKING_STATE::READY;
    float lastTimeTracking;
    float ttl = 2.0;
    glm::vec3 globalDirectionBias = {0,0.0375,0.0};
    
    ofxCv::KalmanPosition kalman;

    glm::vec3 trackPointSum;
    glm::vec3 rawGlobalPosition;
    float radiusSquaredMax = 0.0;
    float radiusSet = 0.0;
    float radiusSquaredScale = 1.0;
    float radiusSquaredScaleTracking = 2.0;
    float radiusSquaredScaleReady = 0.5;
    glm::vec3 localFloorPoint;
    float minFloorDistance = 0.5;
    int trackPointCount = 1;
    int lastTrackPointCount = 1;
    float trackPointWeighedCount = 1.0;
    float lastTrackPointWeighedCount = 1.0;

    bool isReady(){
        return state == TRACKING_STATE::READY;
    }
    
    bool isTracking(){
        return state == TRACKING_STATE::TRACKING;
    }
    
    bool isLost(){
        return state == TRACKING_STATE::LOST;
    }
    
    bool isWitinHead(glm::vec3 & v){
        return (distanceToHead2(v) < radiusSquared);
    }

    bool isAroundHead(glm::vec3 & v){
        return (distanceToHead2(v) < radiusSquared*1.1);
    }

    float distanceToHead2(glm::vec3 & v){
        return glm::distance2(getPosition(), v);
    }
    
    float addTrackPoint(glm::vec3 & v){
        float dist = distanceToHead2(v);
        float radiusSquaredScaled= radiusSquared * radiusSquaredScale;
        if(dist < radiusSquaredScaled){
            trackPointSum += v;
            trackPointCount++;
            trackPointWeighedCount += fabs(v.z*v.z);
            radiusSquaredMax = fmaxf(radiusSquaredMax, dist);
            return 1;
        } else if (dist < radiusSquared * 1.5){
            return 2;
        } else /*if (dist < 3.0*3.0)*/ {
            // distance to line towards floor
            
            float distV2Line = -1.0;
            
            auto pos = getPosition();
            float line_dist = glm::distance2(localFloorPoint, pos);
            if (line_dist == 0) distV2Line = glm::distance2(v, localFloorPoint);
            else {
            float t = ((v.x - localFloorPoint.x) * (pos.x - localFloorPoint.x) + (v.y - localFloorPoint.y) * (pos.y - localFloorPoint.y) + (v.z - localFloorPoint.z) * (pos.z - localFloorPoint.z)) / line_dist;
            t = ofClamp(t, 0.0, 1.0);
            distV2Line = glm::distance2(v, glm::vec3(localFloorPoint.x + t * (pos.x - localFloorPoint.x),
                                                     localFloorPoint.y + t * (pos.y - localFloorPoint.y),
                                                     localFloorPoint.z + t * (pos.z - localFloorPoint.z)));
            }
            if(distV2Line < minFloorDistance*minFloorDistance)
                return 3;
        }
        return 0;
    }
    
    void update(ofNode & startingPointNode){
        auto now = ofGetElapsedTimef();

        if(trackPointWeighedCount > 800.0){
            if(isReady() || isLost()){
                state = TRACKING_STATE::TRACKING;
            }
            radiusSquaredScale = radiusSquaredScaleTracking;
            trackPointSum /= trackPointCount;
            lastTrackPointCount = trackPointCount;
            lastTrackPointWeighedCount = trackPointWeighedCount;
            setPosition(trackPointSum);
            rawGlobalPosition = getGlobalPosition();
            kalman.update(rawGlobalPosition+globalDirectionBias); // feed measurement
            auto gp = kalman.getEstimation();
            setGlobalPosition(gp);
            auto newFloorP = glm::inverse(parent->getGlobalTransformMatrix()) * glm::vec4(gp.x, 0.0, gp.z, 1.0);
            localFloorPoint = glm::vec3(newFloorP) / newFloorP.w;
            radiusSquaredMax = 0.0;
            lastTimeTracking = now;
        } else {
            auto gp = getGlobalPosition();
            kalman.update(gp); // feed measurement
            if(isTracking()) radiusSquaredScale = radiusSquaredScaleTracking * 2.0;
            if(isReady()) setGlobalPosition(startingPointNode.getGlobalPosition());
        }
        if(now - lastTimeTracking > ttl){
            if(isTracking()){
                state = TRACKING_STATE::LOST;
                lastTimeTracking = now;
            } else if (isLost()) {
                setGlobalPosition(startingPointNode.getGlobalPosition());
                state = TRACKING_STATE::READY;
                radiusSquaredScale = radiusSquaredScaleReady;
                setRadius(radiusSet);
                auto gp = getGlobalPosition();
                auto newFloorP = glm::inverse(parent->getGlobalTransformMatrix()) * glm::vec4(gp.x, 0.0, gp.z, 1.0);
                localFloorPoint = glm::vec3(newFloorP) / newFloorP.w;
                lastTimeTracking = now;
            }
        }
        
        trackPointSum = getPosition();
        trackPointCount = 1;
        trackPointWeighedCount = 1.0;
        
    }
    
    void set( float radius, int resolution){
        kalman.init(1/10000000000., 1/10000000.); // inverse of (smoothness, rapidness);
        radiusSet = radius;
        ofIcoSpherePrimitive::set(radius, resolution);
        radiusSquared = radius*radius;
    }
    
    void setRadius(float radius){
        ofIcoSpherePrimitive::setRadius(radius);
        radiusSquared = radius*radius;
    }

private:
    float radiusSquared;
    
};

class MeshTracker : public ofBoxPrimitive{
public:
    ofNode startingPoint;
    
    ofNode camera;
    
    float headRadius = 0.3/2.;
    vector<head> heads;
    
    int maxHeads = 3;
    
    void setup(int maxHeads, glm::vec3 startingPoint, ofNode & camera, ofNode & origin ){
        
        this->setParent(origin);
        
        this->camera.setParent(origin);
        this->camera.setGlobalPosition(camera.getGlobalPosition());
        this->camera.setGlobalOrientation(camera.getGlobalOrientation());
        this->camera.setScale(camera.getScale());
        
        this->startingPoint.setParent(origin);
        this->startingPoint.setGlobalPosition(startingPoint);
        
        this->maxHeads = maxHeads;
        heads.resize(maxHeads);
        
        for( auto & head : heads){
            head.set(headRadius,1);
            head.setParent(this->camera);
            auto p = this->startingPoint.getGlobalPosition();
            head.setGlobalPosition(p);
        }
    }
    
    int addVertex(glm::vec3 & v){
        int pointFound = 0;
        
        // tracking heads consume first
        for(auto & head : heads){
            if(head.isTracking()){
                pointFound = head.addTrackPoint(v);
            }
            if(pointFound > 0) break;
        }
        if(pointFound > 0) return pointFound;
        
        // then comes the rest
        for(auto & head : heads){
            if(!head.isTracking()){
                pointFound = head.addTrackPoint(v);
            }
            if(pointFound > 0) break;
        }
        return pointFound;
    }

    void update(){
        for(auto & head : heads){
            head.update(this->startingPoint);
        }
        // make sure the first ones are the highest.
        std::sort(heads.begin(), heads.end(), [](head a, head b) {
            return a.getGlobalPosition().y > b.getGlobalPosition().y && a.state == head::TRACKING_STATE::TRACKING;
        });
        
    }

    void draw(){
        ofSetColor(255,255,255,255);
        this->drawWireframe();
        ofSetColor(255,0,255,255);
        ofDrawSphere(this->startingPoint.getGlobalPosition(), 0.05);
        for(auto & head : heads){
            if(head.isTracking()){
                ofSetColor(0,255,0,255);
            } else if (head.isReady()){
                ofSetColor(0,255,255,255);
            } else if (head.isLost()){
                ofSetColor(255,255,0,255);
            }
            head.drawWireframe();
            head.getParent()->transformGL();
            ofSetColor(255,0,0,255);
            ofDrawLine(head.getPosition(), head.localFloorPoint);
            ofSetColor(255,255);
            ofDrawBitmapString(ofToString(head.lastTrackPointWeighedCount), head.getPosition());
            ofDrawCone(head.localFloorPoint, 0.025, 0.05);
            head.getParent()->restoreTransformGL();
        }
    }
};
