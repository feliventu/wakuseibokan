#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "Structure.h"

enum TARGETS {GROUND, AIR};

class Launcher : public Structure
{
protected:
    Vec3f firingpos;
public:
    GLfloat modelview[16];

    float zoom;

    Launcher(int faction);
    void init();
    void drawModel(float yRot, float xRot, float x, float y, float z);

    void getViewPort(Vec3f &Up, Vec3f &position, Vec3f &forward);
    Vec3f getForward();
    void setForward(float x, float y, float z);
    void setForward(Vec3f);

    Vec3f getFiringPort();

    Vehicle* fire(dWorldID world, dSpaceID space);
    Vehicle* fireAir(dWorldID world, dSpaceID space);
    Vehicle* fireGround(dWorldID world, dSpaceID space);

    void ground();
    void air();

    void virtual doControl(Controller);
    void virtual doControl();
};


#endif // LAUNCHER_H
