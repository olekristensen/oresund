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
