/*
 * Vehicle.cpp
 *
 *  Created on: Jan 16, 2011
 *      Author: faturita
 */

#define dSINGLE

#include "Vehicle.h"
#include <assert.h>

Vehicle::Vehicle()
{
	for(int i=0;i<12;i++)
	{
		R[i]=0;
	}
	R[0]=R[5]=R[10]=1;
}

void Vehicle::setXRotAngle(float xRotAngle)
{
	Vehicle::xRotAngle = xRotAngle;
}
void Vehicle::setYRotAngle(float yRotAngle)
{
	Vehicle::yRotAngle = yRotAngle;
}
float Vehicle::getSpeed()
{
	return speed;
}
void Vehicle::setSpeed(float speed)
{
	Vehicle::speed=speed;
}
void Vehicle::setPos(const Vec3f &newpos)
{
	pos[0] = newpos[0];
	pos[1] = newpos[1];
	pos[2] = newpos[2];
}
void Vehicle::setPos(float x, float y, float z)
{
	pos[0] = x;
	pos[1] = y;
	pos[2] = z;
}
Vec3f Vehicle::getPos()
{
	return pos;
}

void Vehicle::setForward(float x, float y, float z)
{
	forward[0]=x;
	forward[1]=y;
	forward[2]=z;
}

Vec3f Vehicle::getForward()
{
	return forward;
}

void Vehicle::setLocation(float fPos[3], float R[12])
{
	pos[0] = fPos[0];
	pos[1] = fPos[1];
	pos[2] = fPos[2];

	for(int i=0; i<12; i++)
	{
        Vehicle::R[i] =  R[i];
	}
}

Vec3f Vehicle::dBodyGetLinearVelVec(dBodyID body)
{
    dReal *v = (dReal *)dBodyGetLinearVel(body);

    Vec3f vec3fV;
    vec3fV[0]= v[0];vec3fV[1] = v[1]; vec3fV[2] = v[2];

    return vec3fV;
}

Vec3f Vehicle::dBodyGetLinearVelInBody(dBodyID body)
{
    dReal *v = (dReal *)dBodyGetLinearVel(body);

    Vec3f vec3fV;
    vec3fV[0]= v[0];vec3fV[1] = v[1]; vec3fV[2] = v[2];

    dVector3 velResult2;
    dBodyVectorFromWorld (body, vec3fV[0], vec3fV[1], vec3fV[2], velResult2);

    vec3fV[0]= velResult2[0];vec3fV[1] = velResult2[1]; vec3fV[2] = velResult2[2];

    return vec3fV;
}

Vec3f Vehicle::dBodyGetAngularVelInBody(dBodyID body)
{
    const dReal* velResultWorld = dBodyGetAngularVel(body);

    dVector3 velResultBody;
    dBodyVectorFromWorld (body, velResultWorld[0], velResultWorld[1], velResultWorld[2], velResultBody);

    Vec3f vec3fV;
    vec3fV[0]= velResultBody[0];vec3fV[1] = velResultBody[1]; vec3fV[2] = velResultBody[2];

    return vec3fV;
}

void Vehicle::init()
{
	//Load the model
	_model = MD2Model::load("walrusgood.md2");
    if (_model != NULL)
        _model->setAnimation("run");

}

int Vehicle::getType()
{
    return 0;
}

void Vehicle::setVector(float* V, dVector3 v)
{
    V[0] = v[0];
    V[1] = v[1];
    V[2] = v[2];
}

void Vehicle::setVector(float* V, Vec3f v)
{
    V[0] = v[0];
    V[1] = v[1];
    V[2] = v[2];
}

void  Vehicle::getViewPort(Vec3f &Up, Vec3f &position, Vec3f &forward)
{
	position = getPos();
	forward = getForward();
	Up = toVectorInFixedSystem(0.0f, 1.0f, 0.0f,0,0);

	Vec3f orig;


	forward = forward.normalize();
	orig = position;
	Up[0]=Up[2]=0;Up[1]=4;
	position = position - 20*forward + Up;
	forward = orig-position;
}

void Vehicle::drawModel(float yRot, float xRot, float x, float y, float z)
{
    //Draw the saved model
    if (_model != NULL)
    {
        glPushMatrix();
        glTranslatef(x, y, z);
		glRotatef(-180.0f, 1.0f, 0.0f, 0.0f);
		glRotatef(-180.0f, 0.0f, 0.0f, 1.0f);

		glRotatef(yRot, 0.0f, 0.0f, 1.0f);

        glRotatef(xRot, 1.0f, 0.0f, 0.0f);

        _model->draw();
        drawArrow();
        glPopMatrix();
    }
    else
    {
    	printf ("model is null\n");
    }
}

/**
void Vehicle::drawModel()
{
    assert(0 || !"Draw model should not be executed.\n");
    
    float modX=0.0f, modY=0.0f, modZ=0.0f;

    modX = pos[0];
    modY = pos[1];
    modZ = pos[2];

    Vec3f forward = toVectorInFixedSystem(0, 0, -0.1,-xRotAngle,yRotAngle);

    modX+=speed*forward[0]; modY+=speed*forward[1];modZ+=speed*forward[2];

    pos += speed * forward;

    drawModel(xRotAngle, yRotAngle, modX, modY, modZ);
}**/

void Vehicle::setThrottle(float throttle)
{
    Vehicle::throttle = throttle;
}

void Vehicle::upThrottle(float throttle)
{
    Vehicle::throttle += throttle;
}

float Vehicle::getThrottle()
{
	return Vehicle::throttle;
}

void Vehicle::doControl(Controller controller)
{
    //engine[0] = controller.pitch;
    //engine[1] = controller.yaw;
    //engine[2] = controller.roll;
}

void  Vehicle::doDynamics(dBodyID) {
    assert( 0 || !"This should not be executed.");
}
void  Vehicle::doDynamics() {
    assert( 0 || !"This should not be executed.");
}


void  Vehicle::embody(dBodyID myBodySelf)
{
    dMass m;
    
    float myMass = 1.0f;
    float radius = 2.64f;
    float length = 7.0f;
    
    dBodySetPosition(myBodySelf, pos[0], pos[1], pos[2]);
    //dMassSetBox(&m,1,length,length,length);
    dMassSetSphere(&m,1,radius);
    dMassAdjust(&m, myMass*1.0f);
    dBodySetMass(myBodySelf,&m);
    
    me = myBodySelf;
}

dBodyID Vehicle::getBodyID()
{
    return me;
}

void Vehicle::wrapDynamics(dBodyID body)
{
    dVector3 result;

    dBodyVectorToWorld(body, 0,0,1,result);
    setForward(result[0],result[1],result[2]);

    const dReal *dBodyPosition = dBodyGetPosition(body);
    const dReal *dBodyRotation = dBodyGetRotation(body);


    setPos(dBodyPosition[0],dBodyPosition[1],dBodyPosition[2]);
    setLocation((float *)dBodyPosition, (float *)dBodyRotation);
}

