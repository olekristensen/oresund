#pragma once

class EventWatcher {
public:
    virtual void mousePressed(ofMouseEventArgs& mouse) {}
    virtual void mouseMoved(ofMouseEventArgs& mouse) {}
    virtual void mouseDragged(ofMouseEventArgs& mouse) {}
    virtual void mouseReleased(ofMouseEventArgs& mouse) {}
    virtual void keyPressed(ofKeyEventArgs& key) {}
    virtual void keyReleased(ofKeyEventArgs& key) {}
    virtual void draw(ofEventArgs& args) {}
    
    bool controlEventsEnabled = false;
    bool drawEventsEnabled = false;
    
    void enableControlEvents() {
        if(!controlEventsEnabled){
            controlEventsEnabled = true;
            ofAddListener(ofEvents().keyPressed, this, &EventWatcher::keyPressed);
            ofAddListener(ofEvents().keyReleased, this, &EventWatcher::keyReleased);
            ofAddListener(ofEvents().mousePressed, this, &EventWatcher::mousePressed);
            ofAddListener(ofEvents().mouseReleased, this, &EventWatcher::mouseReleased);
            ofAddListener(ofEvents().mouseMoved, this, &EventWatcher::mouseMoved);
            ofAddListener(ofEvents().mouseDragged, this, &EventWatcher::mouseDragged);
        }
    }
    void disableControlEvents() {
        if(controlEventsEnabled){
            controlEventsEnabled = false;
            ofRemoveListener(ofEvents().keyPressed, this, &EventWatcher::keyPressed);
            ofRemoveListener(ofEvents().keyReleased, this, &EventWatcher::keyReleased);
            ofRemoveListener(ofEvents().mousePressed, this, &EventWatcher::mousePressed);
            ofRemoveListener(ofEvents().mouseReleased, this, &EventWatcher::mouseReleased);
            ofRemoveListener(ofEvents().mouseMoved, this, &EventWatcher::mouseMoved);
            ofRemoveListener(ofEvents().mouseDragged, this, &EventWatcher::mouseDragged);
        }
    }
    void enableDrawEvent() {
        if(!drawEventsEnabled){
            drawEventsEnabled = true;
            ofAddListener(ofEvents().draw, this, &EventWatcher::draw);
        }
    }
    void disableDrawEvent() {
        if(drawEventsEnabled){
            drawEventsEnabled = false;
            ofRemoveListener(ofEvents().draw, this, &EventWatcher::draw);
        }
    }
};
