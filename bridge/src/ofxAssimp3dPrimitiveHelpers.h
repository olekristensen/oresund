#pragma once
//
//  ofxAssimp3dPrimitiveHelpers.h
//  bridge
//
//  Created by ole kristensen on 30/08/2018.
//

void renderPrimitiveWithMaterialsRecursive(ofxAssimp3dPrimitive* primitive, vector<ofxPBRMaterial> & materials, ofxPBR & pbr, ofPolyRenderMode renderType = OF_MESH_FILL){
    if(primitive->textureIndex >= 0) materials[primitive->textureIndex].begin(&pbr);
    primitive->of3dPrimitive::draw(renderType);
    if(primitive->textureIndex >= 0) materials[primitive->textureIndex].end();
    for(auto c : primitive->getChildren()) {
        renderPrimitiveWithMaterialsRecursive(c, materials, pbr, renderType);
    }
}

ofxAssimp3dPrimitive * getFirstPrimitiveWithTextureIndex(ofxAssimp3dPrimitive* primitive, int textureIndex){
    if(primitive->textureIndex == textureIndex)
        return primitive;
    else {
        for(auto c : primitive->getChildren()) {
            ofxAssimp3dPrimitive * retP = getFirstPrimitiveWithTextureIndex(c, textureIndex);
            if(retP != nullptr){
                return retP;
            }
        }
    }
    return nullptr;
}

vector<ofxAssimp3dPrimitive*> getPrimitivesWithTextureIndex(ofxAssimp3dPrimitive* primitive, int textureIndex){
    vector<ofxAssimp3dPrimitive*> retVec;
    if(primitive->textureIndex == textureIndex){
        retVec.push_back(primitive);
        return retVec;
    } else {
        for(auto c : primitive->getChildren()) {
            auto childVec = getPrimitivesWithTextureIndex(c, textureIndex);
            if(childVec.size() > 0){
                for(auto child : childVec){
                    retVec.push_back(child);
                }
            }
        }
    }
    return retVec;
}

ofxAssimp3dPrimitive * getFirstPrimitiveWithTextureNameContaining(ofxAssimp3dPrimitive* primitive, string str){
    int textureIndex = 0;
    for(auto name : primitive->textureNames){
        if(ofStringTimesInString(name, str) > 0){
            break;
        }
        textureIndex++;
    }
    
    if(primitive->textureIndex == textureIndex)
        return primitive;
    else {
        for(auto c : primitive->getChildren()) {
            ofxAssimp3dPrimitive * retP = getFirstPrimitiveWithTextureIndex(c, textureIndex);
            if(retP != nullptr){
                return retP;
            }
        }
    }
    return nullptr;
}
