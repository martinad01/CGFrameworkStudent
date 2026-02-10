//
//  entity.cpp
//  ComputerGraphics
//
//  Created by Martina Dulanto Ostornol on 29/1/26.
//


#include "entity.h"

Entity::Entity()
{
    mesh = nullptr;
    model.SetIdentity();
}

void Entity::Update(float seconds_elapsed)
{
    (void)seconds_elapsed; // unused for now
}



