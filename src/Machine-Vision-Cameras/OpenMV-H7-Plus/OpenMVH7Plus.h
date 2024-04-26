#ifndef __OPENMV_H7_PLUS_H
#define __OPENMV_H7_PLUS_H

#include "Machine-Vision-Cameras/MachineVisionCamera.h"

class OpenMVH7Plus : public CameraInterface {
public:
    static OpenMVH7Plus& instance();

    bool setup() override;

    bool readCount() override;

protected:
    OpenMVH7Plus();
    
    virtual ~OpenMVH7Plus();
    
    OpenMVH7Plus(const OpenMVH7Plus&) = delete;

    OpenMVH7Plus& operator=(const OpenMVH7Plus&) = delete;

    static OpenMVH7Plus *_instance;
};

#endif  /* __OPENMV_H7_PLUS_H */