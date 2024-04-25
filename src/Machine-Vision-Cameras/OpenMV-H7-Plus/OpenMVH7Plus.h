#ifndef __OPENMVH7PLUS_H
#define __OPENMVH7PLUS_H

#include "Machine-Vision-Cameras/MachineVisionCamera.h"

class OpenMVH7Plus : public CameraInstance {
public:
    static OpenMVH7Plus& instance();

    bool setup() override;

    bool capture() override;

protected:
    OpenMVH7Plus();
    
    virtual ~OpenMVH7Plus();
    
    OpenMVH7Plus(const OpenMVH7Plus&) = delete;

    OpenMVH7Plus& operator=(const OpenMVH7Plus&) = delete;

    static OpenMVH7Plus *_instance;
};

#endif  /* __OPENMVH7PLUS_H */