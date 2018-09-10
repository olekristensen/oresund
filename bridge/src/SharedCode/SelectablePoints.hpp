#pragma once

#include "EventWatcher.hpp"
#include "DraggablePoint.hpp"

class SelectablePoints : public EventWatcher {
protected:
	vector<DraggablePoint> points;
	set<unsigned int> selected;
	
	float clickRadiusSquared;
    ofRectangle mViewPort;
    ofEasyCam * cam = nullptr;

public:
    bool dirty = false;

	SelectablePoints()
	:clickRadiusSquared(0) {
	}
	unsigned int size() {
		return points.size();
	}
    void setViewPort(ofRectangle viewport){
        mViewPort = viewport;
    }
	void add(const ofVec2f& v) {
		points.push_back(DraggablePoint());
		points.back().position = v;
	}
    DraggablePoint& get(int i) {
        return points[i];
    }
    void clear() {
        points.clear();
        selected.clear();
        if(cam != nullptr) cam->enableMouseInput();
    }
	void setClickRadius(float clickRadius) {
		this->clickRadiusSquared = clickRadius * clickRadius;
	}
    
    void setCamera(ofEasyCam * c){
        cam = c;
    }
    
	void mousePressed(ofMouseEventArgs& mouse) {
		bool shift = ofGetKeyPressed(OF_KEY_SHIFT);
		bool hitAny = false;
        for(int i = 0; i < size(); i++) {
            bool hit = points[i].isHit(mouse-mViewPort.getTopLeft(), clickRadiusSquared);
			if(hit && !hitAny) {
				if(!points[i].selected) {
					points[i].selected = true;
					selected.insert(i);
					hitAny = true;
				}
			} else if(!shift) {
				points[i].selected = false;
				selected.erase(i);
			}
		}
        if(cam != nullptr){
            if(selected.size() == 0){
                cam->enableMouseInput();
            } else {
                cam->disableMouseInput();
            }
        }
	}
    virtual void keyPressed(ofKeyEventArgs& key) {
        if(key.key == OF_KEY_DEL || key.key == OF_KEY_BACKSPACE) {
            set<unsigned int>::iterator itr;
            for(itr = selected.begin(); itr != selected.end(); itr++) {
                points[*itr].reset();
            }
            dirty = true;
            selected.clear();
        }
    }
	void draw(ofEventArgs& args) {
        ofPushView();
        ofViewport(mViewPort.x, mViewPort.y, mViewPort.width, mViewPort.height, true);
        ofSetupScreenPerspective();
//        ofTranslate(-mViewPort.getTopLeft());
        ofPushStyle();
        ofSetColor(255, 0, 255);
        for(int i = 0; i < size(); i++) {
            points[i].draw(clickRadiusSquared);
        }
        ofPopStyle();
        ofPopView();
	}
    
    void draw(bool vFlip) {
        ofPushView();
        ofPushMatrix();
        ofPushStyle();
        ofViewport(0,0, mViewPort.width, mViewPort.height, true);
        ofSetupScreenPerspective();
        if(vFlip){
            ofTranslate(0, mViewPort.height/2.0);
            ofScale(1.0, -1.0, 1.0);
            ofTranslate(0, -mViewPort.height/2.0);
        }
        ofSetColor(255, 255);
        for(int i = 0; i < size(); i++) {
            points[i].draw(clickRadiusSquared);
        }
        ofPopStyle();
        ofPopMatrix();
        ofPopView();
    }

    void save(string filePath){

        string savePath = ofToDataPath(filePath + "/reference-points.json", true);
        
        ofJson j;
        int index = 0;
        for(auto p : points){
            j["points"][index]["x"] = p.position.x;
            j["points"][index]["y"] = p.position.y;
            j["points"][index]["hit"] = p.hit;
            index++;
        }
        ofSaveJson(std::filesystem::path(savePath), j);
    }
    void load(string filePath){

        string loadPath = ofToDataPath(filePath + "/reference-points.json", true);
        ofJson j = ofLoadJson(loadPath);
        clear();
        int index = 0;
        for(auto p : j["points"]){
            add(ofVec2f(p["x"], p["y"]));
            points.back().hit = p["hit"];
        }
    }
};
