/*    
    This file is a part of Stonefish.

    Stonefish is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Stonefish is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

//
//  Comm.cpp
//  Stonefish
//
//  Created by Patryk Cieslak on 25/02/20.
//  Copyright (c) 2020 Patryk Cieslak. All rights reserved.
//

#include "comms/Comm.h"

#include "core/SimulationApp.h"
#include "core/SimulationManager.h"
#include "graphics/OpenGLPipeline.h"
#include "entities/SolidEntity.h"
#include "entities/StaticEntity.h"

namespace sf
{

Comm::Comm(std::string uniqueName, uint64_t deviceId, Scalar frequency)
{
    name = SimulationApp::getApp()->getSimulationManager()->getNameManager()->AddName(uniqueName);
    id = deviceId;
    cId = 0;
    freq = frequency == Scalar(0) ? Scalar(1) : frequency;
    eleapsedTime = Scalar(0);
    renderable = false;
    newDataAvailable = false;
    updateMutex = SDL_CreateMutex();
    attach = nullptr;
    o2c = Transform();
}

Comm::~Comm()
{
    if(SimulationApp::getApp() != nullptr)
        SimulationApp::getApp()->getSimulationManager()->getNameManager()->RemoveName(name);
    SDL_DestroyMutex(updateMutex);
}

Transform Comm::getDeviceFrame()
{
    if(attach != nullptr)
    {
        if(attach->getType() == EntityType::ENTITY_STATIC)
        {
            return ((StaticEntity*)attach)->getTransform() * o2c;
        }
        else if(attach->getType() == EntityType::ENTITY_SOLID)
        {
            return ((SolidEntity*)attach)->getOTransform() * o2c;
        }
    }
    
    return o2c;
}

std::string Comm::getName()
{
    return name;
}

uint64_t Comm::getDeviceId()
{
    return id;
}

uint64_t Comm::getConnectedId()
{
    return cId;
}

void Comm::MarkDataOld()
{
    newDataAvailable = false;
}

void Comm::setUpdateFrequency(Scalar f)
{
    freq = f == Scalar(0) ? Scalar(1) : f;
}

bool Comm::isNewDataAvailable()
{
    return newDataAvailable;
}

void Comm::setRenderable(bool render)
{
    renderable = render;
}

bool Comm::isRenderable()
{
    return renderable;
}

void Comm::Connect(uint64_t deviceId)
{
    cId = deviceId;
}

void Comm::AttachToWorld(const Transform& origin)
{
    o2c = origin;
}

void Comm::AttachToStatic(StaticEntity* body, const Transform& origin)
{
    if(body != nullptr)
    {
        o2c = origin;
        attach = body;
    }
}

void Comm::AttachToSolid(SolidEntity* body, const Transform& origin)
{
    if(body != nullptr)
    {
        o2c = origin;
        attach = body;
    }
}

void Comm::Update(Scalar dt)
{
    SDL_LockMutex(updateMutex);
    
    if(freq <= Scalar(0.)) // Every simulation tick
    {
        InternalUpdate(dt);
        newDataAvailable = true;
    }
    else //Fixed rate
    {
        eleapsedTime += dt;
        Scalar invFreq = Scalar(1.)/freq;
        
        if(eleapsedTime >= invFreq)
        {
            InternalUpdate(invFreq);
            eleapsedTime -= invFreq;
            newDataAvailable = true;
        }
    }
    
    SDL_UnlockMutex(updateMutex);
}

std::vector<Renderable> Comm::Render()
{
    std::vector<Renderable> items(0);
    
    Renderable item;
    item.type = RenderableType::SENSOR_CS;
    item.model = glMatrixFromTransform(getDeviceFrame());
    items.push_back(item);
    
    return items;
}
    
}
