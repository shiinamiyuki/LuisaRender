//
// Created by Mike Smith on 2020/2/10.
//

#pragma once

#include <core/shape.h>

namespace luisa {

class ShapeReplica : public Shape {

private:
    std::shared_ptr<Shape> _reference;

public:
    ShapeReplica(Device *device, const ParameterSet &parameter_set);
    void load(GeometryEncoder &encoder) override;
    
};

}