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
    enum TRACKING_STATES {
        TRACKING_STATE_READY,
        TRACKING_STATE_TRACKING,
        TRACKING_STATE_LOST
    };
    
    int state = TRACKING_STATE_READY;
    float lastTimeTracking;
    float ttl = 2.0;
    glm::vec3 globalDirectionBias = {0,0.025,0.0};
    
    ofxCv::KalmanPosition kalman;

    glm::vec3 trackPointSum;
    float radiusSquaredMax = 0.0;
    float radiusSet = 0.0;
    glm::vec3 localFloorPoint;
    float minFloorDistance = 0.5;
    int trackPointCount = 1;
    int lastTrackPointCount = 1;

    bool isReady(){
        return state == TRACKING_STATE_READY;
    }
    
    bool isTracking(){
        return state == TRACKING_STATE_TRACKING;
    }
    
    bool isLost(){
        return state == TRACKING_STATE_LOST;
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
    
    bool addTrackPoint(glm::vec3 & v){
        float dist = distanceToHead2(v);
        if(dist < radiusSquared){
            trackPointSum += v;
            trackPointCount++;
            radiusSquaredMax = fmaxf(radiusSquaredMax, dist);
            return true;
        } else if (dist < radiusSquared * 1.5){
            return true;
        } else /*if (dist < 3.0*3.0)*/ {
            // distance to line towards floor
            
            /*
             float dist_to_segment_squared(float px, float py, float pz, float lx1, float ly1, float lz1, float lx2, float ly2, float lz2) {
             float line_dist = dist_sq(lx1, ly1, lz1, lx2, ly2, lz2);
             if (line_dist == 0) return dist_sq(px, py, pz, lx1, ly1, lz1);
             float t = ((px - lx1) * (lx2 - lx1) + (py - ly1) * (ly2 - ly1) + (pz - lz1) * (lz2 - lz1)) / line_dist;
             t = constrain(t, 0, 1);
             return dist_sq(px, py, pz, lx1 + t * (lx2 - lx1), ly1 + t * (ly2 - ly1), lz1 + t * (lz2 - lz1));
             }
*/
            float distV2Line = -1.0;
            
            auto pos = getPosition();
            float line_dist = glm::distance2(localFloorPoint, pos);
            if (line_dist == 0) distV2Line = glm::distance2(v, localFloorPoint);
            else {
            float t = ((v.x - localFloorPoint.x) * (pos.x - localFloorPoint.x) + (v.y - localFloorPoint.y) * (pos.y - localFloorPoint.y) + (v.z - localFloorPoint.z) * (pos.z - localFloorPoint.y)) / line_dist;
            t = ofClamp(t, 0.0, 1.0);
            distV2Line = glm::distance2(v, glm::vec3(localFloorPoint.x + t * (pos.x - localFloorPoint.x),
                                                     localFloorPoint.y + t * (pos.y - localFloorPoint.y),
                                                     localFloorPoint.z + t * (pos.z - localFloorPoint.z)));
            }
            if(distV2Line < minFloorDistance*minFloorDistance)
                return true;
        }
        return false;
    }
    
    void update(ofNode & startingPointNode){
        auto now = ofGetElapsedTimef();

        if(trackPointCount > 20){
            if(isReady() || isLost()){
                state = TRACKING_STATE_TRACKING;
            }
            trackPointSum /= trackPointCount;
            lastTrackPointCount = trackPointCount;
            setPosition(trackPointSum);
            auto gp = getGlobalPosition();
            kalman.update(gp+globalDirectionBias); // feed measurement
            setGlobalPosition(kalman.getEstimation());
            auto newFloorP = glm::inverse(parent->getGlobalTransformMatrix()) * glm::vec4(gp.x, 0.0, gp.z, 1.0);
            localFloorPoint = glm::vec3(newFloorP) / newFloorP.w;
            radiusSquaredMax = 0.0;
            lastTimeTracking = now;
        } else {
            auto gp = getGlobalPosition();
            kalman.update(gp); // feed measurement
        }
        if(now - lastTimeTracking > ttl){
            if(isTracking()){
                state = TRACKING_STATE_LOST;
                lastTimeTracking = now;
            } else if (isLost()) {
                setGlobalPosition(startingPointNode.getGlobalPosition());
                state = TRACKING_STATE_READY;
                setRadius(radiusSet);
                auto gp = getGlobalPosition();
                auto newFloorP = glm::inverse(parent->getGlobalTransformMatrix()) * glm::vec4(gp.x, 0.0, gp.z, 1.0);
                localFloorPoint = glm::vec3(newFloorP) / newFloorP.w;
                lastTimeTracking = now;
            }
        }
        
        trackPointSum = getPosition();
        trackPointCount = 1;
        
    }
    
    void set( float radius, int resolution){
        kalman.init(1/1000000000., 1/1000000.); // inverse of (smoothness, rapidness);
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
    
    bool addVertex(glm::vec3 & v){
        bool pointFound = false;
        
        // tracking heads consume first
        for(auto & head : heads){
            if(head.isTracking()){
                pointFound = head.addTrackPoint(v);
            }
            if(pointFound) break;
        }
        if(pointFound) return pointFound;
        
        // then comes the rest
        for(auto & head : heads){
            if(!head.isTracking()){
                pointFound = head.addTrackPoint(v);
            }
            if(pointFound) break;
        }
        return pointFound;
    }

    void update(){
        for(auto & head : heads){
            head.update(this->startingPoint);
        }
        // make sure the first ones are the highest.
        std::sort(heads.begin(), heads.end(), [](head a, head b) {
            return a.getGlobalPosition().y > b.getGlobalPosition().y;
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
            ofDrawBitmapString(ofToString(head.lastTrackPointCount), head.getPosition());
            ofDrawCone(head.localFloorPoint, 0.025, 0.05);
            head.getParent()->restoreTransformGL();
        }
    }
};
