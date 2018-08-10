#pragma once

class ofAutoShader : public ofShader {
public:
	void setup(string name) {
		this->name = name;
		ofEventArgs args;
		update(args);
		ofAddListener(ofEvents().update, this, &ofAutoShader::update);
	}
	
	void update(ofEventArgs &args) {	
		bool needsReload = false;
			
		string fragName = name + ".frag";
		ofFile fragFile(fragName);
		if(fragFile.exists()) {
            std::time_t fragTimestamp = std::filesystem::last_write_time(fragFile);
			if(fragTimestamp != lastFragTimestamp) {
				needsReload = true;
				lastFragTimestamp = fragTimestamp;
			}
		} else {
			fragName = "";
		}
		
		string vertName = name + ".vert";
		ofFile vertFile(vertName);
		if(vertFile.exists()) {
			std::time_t vertTimestamp = std::filesystem::last_write_time(vertFile);
			if(vertTimestamp != lastVertTimestamp) {
				needsReload = true;
				lastVertTimestamp = vertTimestamp;
			}
		} else {
			vertName = "";
		}
		
		if(needsReload) {
			ofLogVerbose("ofAutoShader") << "reloading shader at " << ofGetTimestampString("%H:%M:%S");
			ofShader::load(vertName, fragName);
		}
	}
private:
	string name;
	std::time_t lastFragTimestamp, lastVertTimestamp;
};
