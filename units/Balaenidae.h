#ifndef BALAENIDAE_H
#define BALAENIDAE_H

#include "Vehicle.h"
#include "Manta.h"

class Balaenidae : public Vehicle
{
protected:
    dGeomID geom;
    float rudder;
public:
    ~Balaenidae();
    Balaenidae();
    void drawModel(float yRot, float xRot, float x, float y, float z);
    void init();
    int getType();
    void embody(dWorldID world, dSpaceID space);
    void embody(dBodyID myBodySelf);

    void doControl(Controller controller);
    void doControl();

    void getViewPort(Vec3f &Up, Vec3f &position, Vec3f &forward);
    void doDynamics();


    void doDynamics(dBodyID body);

    void drawModel();
    Vehicle* spawn(dWorldID  world,dSpaceID space,int type);

    void launch(Manta* m);
    void taxi(Manta *m);
};

#endif // BALAENIDAE_H
