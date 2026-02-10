//
//  entity.h
//  ComputerGraphics
//
//  Created by Martina Dulanto Ostornol on 29/1/26.
//

#ifndef entity_h
#define entity_h
#pragma once
#include "mesh.h"

class Entity {
public:
    Mesh* mesh;
    Matrix44 model;

    Entity();
    void Update(float seconds_elapsed);
};

#endif /* entity_h */
