//
//  testbox.cpp
//  mycarrier
//
//  Created by Rodrigo Ramele on 22/05/14.
//  Copyright (c) 2014 Baufest. All rights reserved.
//

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdarg.h>
#include <math.h>

#include <GLUT/glut.h>

#include <ode/ode.h>

#include <vector>
#include <mutex>

#include "ThreeMaxLoader.h"

#include "font/DrawFonts.h"

#include "math/yamathutil.h"

#include "container.h"

#include "usercontrols.h"
#include "camera.h"

#include "openglutils.h"
#include "odeutils.h"

#include "imageloader.h"
#include "terrain/Terrain.h"

#include "sounds/sounds.h"

#include "engine.h"
#include "keplerivworld.h"

#include "units/Vehicle.h"
#include "units/BoxVehicle.h"
#include "units/Manta.h"
#include "units/Walrus.h"
#include "units/Balaenidae.h"
#include "units/SimplifiedDynamicManta.h"

#include "actions/Gunshot.h"

#include "structures/Structure.h"
#include "structures/Warehouse.h"
#include "structures/Hangar.h"
#include "structures/Runway.h"
#include "structures/Laserturret.h"
#include "structures/CommandCenter.h"
#include "structures/Turret.h"
#include "structures/Artillery.h"

#include "map.h"

extern  Camera Camera;

extern  Controller controller;

/* dynamics and collision objects */

static dGeomID ground;

unsigned long timer=0;

dWorldID world;
dSpaceID space;
dJointGroupID contactgroup;

container<Vehicle*> entities;

std::vector<BoxIsland*> islands;

std::vector<std::string> messages;

void nearCallback (void *data, dGeomID o1, dGeomID o2)
{
    int i,n;

    dBodyID b1,b2;

    // only collide things with the ground
    int g1 = (o1 == ground );
    int g2 = (o2 == ground );
    if (!(g1 ^ g2))
    {
        //printf ("Ground colliding..\n");

        //return;
    }


    b1 = dGeomGetBody(o1);
    b2 = dGeomGetBody(o2);
    if (b1 && b2 && dAreConnected (b1,b2)) return;

    if (b1 && isAction(b1) && b2 && (isType(b2,WALRUS) || (isType(b2,MANTA))) && isMineFire(gVehicle(b2),(Gunshot*)gVehicle(b1)) ) return;
    if (b2 && isAction(b2) && b1 && (isType(b1,WALRUS) || (isType(b1,MANTA))) && isMineFire(gVehicle(b1),(Gunshot*)gVehicle(b2)) ) return;

    int val[]={CARRIER,WALRUS,MANTA};

    if (o1 && isRay(o1) && b2 && isType(b2,val,3) && rayHit(gVehicle(b2),(LaserRay*)gVehicle(o1))) {return;}
    if (o2 && isRay(o2) && b1 && isType(b1,val,3) && rayHit(gVehicle(b1),(LaserRay*)gVehicle(o2))) {return;}

    const int N = 10;
    dContact contact[N];
    n = dCollide (o1,o2,N,&contact[0].geom,sizeof(dContact));
    if (n > 0) {
        for (i=0; i<n; i++) {

            Vehicle *v1=NULL,*v2=NULL;
            Structure *s1=NULL, *s2=NULL;
            gVehicle(v1,v2,dGeomGetBody(contact[i].geom.g1), dGeomGetBody(contact[i].geom.g2),s1,s2,contact[i].geom.g1,contact[i].geom.g2);


            // Bullets
            if ((isAction(v1) && isWalrus(v2) && isMineFire(v2,(Gunshot*)v1))
                ||
               (isAction(v2) && isWalrus(v1) && isMineFire(v1,(Gunshot*)v2)))
            {
                // Water buyoncy reaction
                contact[i].surface.mode = dContactSlip1 | dContactSlip2 |
                dContactSoftERP | dContactSoftCFM | dContactApprox1;

                contact[i].surface.mu = 0.0f;
                contact[i].surface.slip1 = 1.0f;
                contact[i].surface.slip2 = 1.0f;
                contact[i].surface.soft_erp = 1.0f;   // 0 in both will force the surface to be tight.
                contact[i].surface.soft_cfm = 1.0f;
            } else
            if ( isAction(v1) || isAction(v2))
            {
                contact[i].surface.mode = dContactSlip1 | dContactSlip2 |
                dContactSoftERP | dContactSoftCFM | dContactApprox1;
                contact[i].surface.mu = 0;
                contact[i].surface.slip1 = 0.1f;
                contact[i].surface.slip2 = 0.1f;

                if (isAction(v1) && isCarrier(v2) && hit(v2,(Gunshot*)v1)) {}
                if (isAction(v2) && isCarrier(v1) && hit(v1,(Gunshot*)v2)) {}
                if (isAction(v1) && isManta(v2) && hit(v2,(Gunshot*)v1)) {}
                if (isAction(v2) && isManta(v1) && hit(v1,(Gunshot*)v2)) {}
                if (isAction(v1) && isWalrus(v2) && hit(v2,(Gunshot*)v1)) {}
                if (isAction(v2) && isWalrus(v1) && hit(v1,(Gunshot*)v2)) {}
                if (isAction(v1) && s2 && hit(s2)) {}
                if (isAction(v2) && s1 && hit(s1)) {}
            } else
            if ( ( isManta(v1) && isCarrier(v2) && releasecontrol(v1) ) ||
                 ( isManta(v2) && isCarrier(v1) && releasecontrol(v2) ) )
                {
                // Manta landing on Carrier
                contact[i].surface.mode = dContactBounce |
                dContactApprox1;

                contact[i].surface.mu = dInfinity;
                contact[i].surface.slip1 = 0.0f;
                contact[i].surface.slip2 = 0.0f;
                contact[i].surface.bounce = 0.2f;
            } else
            if  (isRunway(s1) || isRunway(s2))
            {
                // Manta landing on Runways.
                contact[i].surface.mode = dContactBounce |
                dContactApprox1;


                contact[i].surface.mu = 0.99f;
                contact[i].surface.slip1 = 0.9f;
                contact[i].surface.slip2 = 0.9f;
                contact[i].surface.bounce = 0.2f;

                if ( isRunway(s1) && isManta(v2) && landed(v2, s1->island)) {}
                if ( isRunway(s2) && isManta(v1) && landed(v1, s2->island)) {}

            } else
            if (isIsland(contact[i].geom.g1) || isIsland(contact[i].geom.g2))
            {
                 // Island reaction
                 contact[i].surface.mode = dContactBounce |
                 dContactApprox1;

                 contact[i].surface.mu = 0;
                 contact[i].surface.bounce = 0.2f;
                 contact[i].surface.slip1 = 0.1f;
                 contact[i].surface.slip2 = 0.1f;

                 contact[i].surface.soft_erp = 0;   // 0 in both will force the surface to be tight.
                 contact[i].surface.soft_cfm = 0;


                 // Carrier stranded and Walrus arrived on island.
                 if (isIsland(contact[i].geom.g1) && isCarrier(v2) && stranded(v2,getIsland(contact[i].geom.g1))) {}
                 if (isIsland(contact[i].geom.g2) && isCarrier(v1) && stranded(v1,getIsland(contact[i].geom.g2))) {}
                 if (isIsland(contact[i].geom.g1) && isWalrus(v2)  && arrived(v2,getIsland(contact[i].geom.g1))) {}
                 if (isIsland(contact[i].geom.g2) && isWalrus(v1)  && arrived(v1,getIsland(contact[i].geom.g2))) {}


            } else
            if (ground == contact[i].geom.g1 || ground == contact[i].geom.g2 ) {

                 // Water buyoncy reaction
                 contact[i].surface.mode = dContactSlip1 | dContactSlip2 |
                 dContactSoftERP | dContactSoftCFM | dContactApprox1;

                 contact[i].surface.mu = 0.0f;
                 contact[i].surface.slip1 = 0.1f;
                 contact[i].surface.slip2 = 0.1f;
                 contact[i].surface.soft_erp = .5f;   // 0 in both will force the surface to be tight.
                 contact[i].surface.soft_cfm = .3f;

                 // Walrus reaching shore.
                 if (ground == contact[i].geom.g1 && isWalrus(v2) && departed(v2)) {}
                 if (ground == contact[i].geom.g2 && isWalrus(v1) && departed(v1)) {}

                 if (ground == contact[i].geom.g1 && v2 && isManta(v2) && groundcollisions(v2)) {}
                 if (ground == contact[i].geom.g2 && v1 && isManta(v1) && groundcollisions(v1)) {}

                 if (v1 && isWalrus(v1)) { v1->inert = false;}
                 if (v2 && isWalrus(v2)) { v2->inert = false;}

            } else {
                /**
                // Water buyoncy reaction
                contact[i].surface.mode = dContactSlip1 | dContactSlip2 |
                dContactSoftERP | dContactSoftCFM | dContactApprox1;

                contact[i].surface.mu = 0.0f;
                contact[i].surface.slip1 = 0.1f;
                contact[i].surface.slip2 = 0.1f;
                contact[i].surface.soft_erp = .5f;   // 0 in both will force the surface to be tight.
                contact[i].surface.soft_cfm = .3f;
                **/
                if (v1 && isManta(v1) && groundcollisions(v1)) {}
                if (v2 && isManta(v2) && groundcollisions(v2)) {}
            }

            dJointID c = dJointCreateContact (world,contactgroup,&contact[i]);
            dJointAttach (c,
                          dGeomGetBody(contact[i].geom.g1),
                          dGeomGetBody(contact[i].geom.g2));
        }
    }
}

void inline initIslands()
{
    BoxIsland *thermopilae = new BoxIsland();
    thermopilae->setName("Thermopilae");
    thermopilae->setLocation(0.0f,-1.0,0.0f);
    thermopilae->buildTerrainModel(space,"terrain/thermopilae.bmp");

    BoxIsland *nonsquareisland = new BoxIsland();
    nonsquareisland->setName("Atolon");
    nonsquareisland->setLocation(0.0f,-1.0f,-100 kmf);
    nonsquareisland->buildTerrainModel(space,"terrain/nonsquareisland.bmp");

    BoxIsland *vulcano = new BoxIsland();
    vulcano->setName("Vulcano");
    vulcano->setLocation(145 kmf, -1.0f, 89 kmf);
    vulcano->buildTerrainModel(space,"terrain/vulcano.bmp");

    BoxIsland *nemesis = new BoxIsland();
    nemesis->setName("Nemesis");
    nemesis->setLocation(-450 kmf, -1.0, 300 kmf);
    nemesis->buildTerrainModel(space,"terrain/nemesis.bmp");

    BoxIsland *atom = new BoxIsland();
    atom->setName("Atom");
    atom->setLocation( 500 kmf, -1.0, -100 kmf);
    atom->buildTerrainModel(space,"terrain/atom.bmp");

    BoxIsland *island = new BoxIsland();
    island->setName("Island");
    island->setLocation(-500 kmf, -1.0, 200 kmf);
    island->buildTerrainModel(space,"terrain/island.bmp");

    islands.push_back(thermopilae);
    islands.push_back(nonsquareisland);
    islands.push_back(vulcano);
    islands.push_back(nemesis);
    islands.push_back(atom);
    islands.push_back(island);
}


void test1()
{
    // Entities will be added later in time.
    Balaenidae *_b = new Balaenidae(GREEN_FACTION);
    _b->init();
    _b->embody(world,space);
    _b->setPos(0.0f,20.5f,-4000.0f);
    _b->stop();

    entities.push_back(_b);
}

void test2()
{
    SimplifiedDynamicManta *_manta1 = new SimplifiedDynamicManta(GREEN_FACTION);

    _manta1->init();
    _manta1->embody(world, space);
    _manta1->setPos(0.0f,20.5f,-6000.0f);
    _manta1->setStatus(0);
    _manta1->inert = true;
    _manta1->enableAuto();
    _manta1->setStatus(Manta::FLYING);
    _manta1->elevator = +12;
    struct controlregister c;
    c.thrust = 1000.0f/(10.0);
    c.pitch = 12;
    _manta1->setControlRegisters(c);
    _manta1->setThrottle(1000.0f);

    entities.push_back(_manta1);
}


void test4()
{
    entities.push_back(islands[0]->addStructure(new Runway(GREEN_FACTION)     ,           0.0f,    0.0f,space,world));
    entities.push_back(islands[0]->addStructure(new Hangar(GREEN_FACTION)     ,        -550.0f,    0.0f,space,world));
}


void test6()
{
    Vec3f pos(0.0f,10.0f,-4400.0f);
    Camera.setPos(pos);

    Balaenidae *b = (Balaenidae*)entities[0];

    b->setPos(0.0f,20.5f,-3000.0f);

}

void test7()
{
    Vec3f pos(0.0f,10.0f,-3700.0f);
    Camera.setPos(pos);
}

void test8()
{
    Walrus *_walrus = new Walrus(GREEN_FACTION);

    _walrus->init();
    _walrus->embody(world, space);
    _walrus->setPos(200.0f,1.32f,-6000.0f);
    _walrus->setStatus(Walrus::SAILING);
    _walrus->enableAuto();
    struct controlregister c;
    c.thrust = 200.0f;
    c.roll = 0;
    _walrus->setControlRegisters(c);
    _walrus->setThrottle(200.0f);

    entities.push_back(_walrus);
}

void checktest8(unsigned long  timer)      // Check Walrus entering and leaving an island.
{
    static bool isWalrusInIsland = false;
    static bool didWalrusLeftIsland = false;

    if (timer>100)
    {
        Walrus *_walrus = (Walrus*)entities[1];

        if (_walrus->getIsland() != NULL)
        {
            isWalrusInIsland = true;
        }

        if (_walrus->getStatus() == Walrus::OFFSHORING)
        {
            didWalrusLeftIsland = true;
        }
    }

    if (timer>=4000)
    {
        Walrus *_walrus = (Walrus*)entities[1];

        if (!isWalrusInIsland)
        {
            printf("Test failed: Walrus has not associated island.\n");
            endWorldModelling();
            exit(-1);
        } else if (!didWalrusLeftIsland) {
            printf("Test failed: Walrus never left the island.\n");
            endWorldModelling();
            exit(-1);
        } else {
            printf("Test succedded\n");
            endWorldModelling();
            exit(1);
        }
    }

}







void test14()
{
    // Entities will be added later in time.
    Balaenidae *_b = new Balaenidae(GREEN_FACTION);
    _b->init();
    _b->embody(world,space);
    _b->setPos(0.0f,20.5f,-9000.0f);
    _b->stop();

    entities.push_back(_b);

    entities.push_back(islands[5]->addStructure(new Runway(GREEN_FACTION)     ,           0.0f,    0.0f,space,world));
    entities.push_back(islands[5]->addStructure(new Hangar(GREEN_FACTION)     ,           0.0f, +550.0f,space,world));
}

void test15()
{
    // Entities will be added later in time.
    Balaenidae *_b = new Balaenidae(GREEN_FACTION);
    _b->init();
    _b->embody(world,space);
    _b->setPos(0.0f,20.5f,-9000.0f);
    _b->stop();

    entities.push_back(_b);
}


void checktest1(unsigned long timer)
{
    if (timer>500)
    {
        Vehicle *_b = entities[0];
        Vec3f val = _b->getPos()-Vec3f(0.0f,20.5f,-4000.0f);

        dReal *v = (dReal *)dBodyGetLinearVel(_b->getBodyID());
        Vec3f vec3fV;
        vec3fV[0]= v[0];vec3fV[1] = v[1]; vec3fV[2] = v[2];


        dReal *av = (dReal *)dBodyGetAngularVel(_b->getBodyID());
        Vec3f vav;
        vav[0]= av[0];vav[1] = av[1]; vav[2] = av[2];

        if (val.magnitude()>100)
        {
            printf("Test failed.\n");
            endWorldModelling();
            exit(-1);
        } else if (vav.magnitude()>10)
        {
            printf("Test failed.\n");
            endWorldModelling();
            exit(-1);
        } else if (vec3fV.magnitude()>5)
        {
            printf("Test failed.\n");
            endWorldModelling();
            exit(-1);

        } else {
            printf("Test passed OK!\n");
            endWorldModelling();
            exit(1);
        }
    }
}
void checktest2(unsigned long timer)
{
    if (timer==320)
    {
        SimplifiedDynamicManta *_manta1 = (SimplifiedDynamicManta*)entities[1];
        _manta1->elevator = -4;
        struct controlregister c;
        c.thrust = 0.0f/(10.0);
        c.pitch = -4;
        _manta1->setControlRegisters(c);
        _manta1->setThrottle(0.0f);
        _manta1->inert = true;
    }
    if (timer==1600)
    {
        Vehicle *_b = entities[1];
        Vec3f val = _b->getPos();

        dReal *v = (dReal *)dBodyGetLinearVel(_b->getBodyID());
        Vec3f vec3fV;
        vec3fV[0]= v[0];vec3fV[1] = v[1]; vec3fV[2] = v[2];


        if (val[1]<4.0f)
        {
            printf("Test failed: Height bellow expected.\n");
            endWorldModelling();
            exit(-1);
        } else if (vec3fV.magnitude()>5)
        {
            printf("Test failed: Object is moving.\n");
            endWorldModelling();
            exit(-1);

        } else {
            printf("Test passed OK!\n");
            endWorldModelling();
            exit(1);
        }
    }
}


void test3()
{
    entities.push_back(islands[0]->addStructure(new Structure()  ,           0.0f,-1000.0f,space,world));
}


void checktest3(unsigned long timer)
{

    if (timer==90)  // Slow down Manta so that it can hit the structure.
    {
        SimplifiedDynamicManta *_manta1 = (SimplifiedDynamicManta*)entities[1];
        _manta1->elevator = -4;
        struct controlregister c;
        c.thrust = 400.0f/(10.0);
        c.pitch = -4;
        _manta1->setControlRegisters(c);
        _manta1->setThrottle(400.0f);
    }
    if (timer==1000)   // In case it hit the structure, their health will be lower.
    {
        SimplifiedDynamicManta *_manta1 = (SimplifiedDynamicManta*)entities[1];

        if (_manta1->getHealth()==1000)
        {
            printf("Test failed.\n");
            endWorldModelling();
            exit(-1);
        } else {
            printf("Test succedded\n");
            endWorldModelling();
            exit(1);
        }
    }
}

void checktest4(unsigned long  timer)
{
    if (timer==100)   // Slow down Manta so that it can land on the runway
    {
        SimplifiedDynamicManta *_manta1 = (SimplifiedDynamicManta*)entities[1];
        _manta1->elevator = -4;
        struct controlregister c;
        c.thrust = 400.0f/(10.0);
        c.pitch = -4;
        _manta1->setControlRegisters(c);
        _manta1->setThrottle(400.0f);
        _manta1->setStatus(Manta::FLYING);
    }
    if (timer==1000)
    {
        SimplifiedDynamicManta *_manta1 = (SimplifiedDynamicManta*)entities[1];

        if (_manta1->getStatus()!=Manta::LANDED)
        {
            printf("Test failed.\n");
            endWorldModelling();
            exit(-1);
        } else {
            printf("Test succedded\n");
            endWorldModelling();
            exit(1);
        }
    }
}


void checktest5(unsigned long  timer)   // Manta lands on carrier.
{
    if (timer==70)
    {
        SimplifiedDynamicManta *_manta1 = (SimplifiedDynamicManta*)entities[1];
        _manta1->elevator = -4;
        struct controlregister c;
        c.thrust = 400.0f/(10.0);
        c.pitch = -4;
        _manta1->setControlRegisters(c);
        _manta1->setThrottle(400.0f);
        _manta1->setStatus(Manta::FLYING);
    }
    if (timer==240)
    {
        SimplifiedDynamicManta *_manta1 = (SimplifiedDynamicManta*)entities[1];
        _manta1->elevator = -4;
        struct controlregister c;
        c.thrust = 0.0f/(10.0);
        c.pitch = 0;
        _manta1->setControlRegisters(c);
        _manta1->setThrottle(0.0f);
        _manta1->stop();
        _manta1->setStatus(Manta::FLYING);
    }
    if (timer==1000)
    {
        SimplifiedDynamicManta *_manta1 = (SimplifiedDynamicManta*)entities[1];

        if (_manta1->getStatus()!=Manta::ON_DECK)
        {
            printf("Test failed.\n");
            endWorldModelling();
            exit(-1);
        } else {
            printf("Test succedded\n");
            endWorldModelling();
            exit(1);
        }
    }

}

void checktest6(unsigned long timer)   // Carrier offshoring.
{
    static bool isOffshoring = false;

    if (timer==100)  // This is not autopilot.  If you control de carrier, you will override the controlregister parameters.
    {
        Balaenidae *b = (Balaenidae*)entities[0];
        struct controlregister c;
        memset(&c,0,sizeof(struct controlregister));
        c.thrust = 10000.0f;
        c.pitch = 0;
        c.roll = 10;
        b->stop();
        b->setControlRegisters(c);
        b->setThrottle(1000.0f);
    }
    if (timer >100 )
    {
        Balaenidae *b = (Balaenidae*)entities[0];

        if (b->getStatus()==Balaenidae::OFFSHORING)
        {
            isOffshoring = true;
        }
    }
    if (timer==3800)
    {
        Balaenidae *b = (Balaenidae*)entities[0];

        Vehicle *_b = entities[0];
        Vec3f val = _b->getPos();

        dReal *v = (dReal *)dBodyGetLinearVel(_b->getBodyID());
        Vec3f vec3fV;
        vec3fV[0]= v[0];vec3fV[1] = v[1]; vec3fV[2] = v[2];


        dReal *av = (dReal *)dBodyGetLinearVel(_b->getBodyID());
        Vec3f vav;
        vav[0]= av[0];vav[1] = av[1]; vav[2] = av[2];

        if (!isOffshoring){
            printf("Test failed: Carrier never offshored.\n");
            endWorldModelling();
            exit(-1);
        }
        if (b->getStatus() != Balaenidae::SAILING || vav.magnitude()>100)
        {
            printf("Test failed: Carrier is still moving.\n");
            endWorldModelling();
            exit(-1);
        } else {
            printf("Test passed OK!\n");
            endWorldModelling();
            exit(1);
        }
    }
}


void checktest7(unsigned long  timer)    // Manta crashing on water (reducing its health).
{

    if (timer==380)
    {
        SimplifiedDynamicManta *_manta1 = (SimplifiedDynamicManta*)entities[1];
        _manta1->elevator -= 29;
        struct controlregister c;
        c.thrust = 3500.0f/(10.0);
        c.pitch = -29;
        _manta1->setControlRegisters(c);
        _manta1->setThrottle(350.0f);
        _manta1->setStatus(Manta::FLYING);
    }
    if (timer==900)
    {
        SimplifiedDynamicManta *_manta1 = (SimplifiedDynamicManta*)entities[1];

        // @FIXME Health reduction should be a parameter and this test should minimize it to make it work.
        // Manta should be immediately destroyed at this speed.
        if (_manta1->getHealth()==1000)
        {
            printf("Test failed.\n");
            endWorldModelling();
            exit(-1);
        } else {
            printf("Test succedded\n");
            endWorldModelling();
            exit(1);
        }
    }

}

void test9()
{
    Walrus *_walrus = new Walrus(GREEN_FACTION);
    _walrus->init();
    _walrus->embody(world, space);
    _walrus->setPos(200.0f,1.32f,-6000.0f);
    _walrus->setStatus(Walrus::SAILING);
    _walrus->stop();

    entities.push_back(_walrus);
}

void checktest9(unsigned long timer)     // Check walrus stability.
{
    if (timer>500)
    {
        Vehicle *_b = entities[1];
        Vec3f posVector = _b->getPos()-Vec3f(200.0f,1.32f,-6000.0f);

        dReal *v = (dReal *)dBodyGetLinearVel(_b->getBodyID());
        Vec3f vec3fV;
        vec3fV[0]= v[0];vec3fV[1] = v[1]; vec3fV[2] = v[2];


        dReal *av = (dReal *)dBodyGetAngularVel(_b->getBodyID());
        Vec3f vav;
        vav[0]= av[0];vav[1] = av[1]; vav[2] = av[2];

        if (posVector.magnitude()>100)
        {
            printf("Test failed: Walrus is not in their expected position.\n");
            endWorldModelling();
            exit(-1);
        } else if (vav.magnitude()>10)
        {
            printf("Test failed: Walrus is still moving.\n");
            endWorldModelling();
            exit(-1);
        } else if (vec3fV.magnitude()>5)
        {
            printf("Test failed: Walrus is still circling.\n");
            endWorldModelling();
            exit(-1);

        } else {
            printf("Test passed OK!\n");
            endWorldModelling();
            exit(1);
        }
    }
}

void test10()
{
    Vehicle *walrus = (entities[0])->spawn(world,space,WALRUS,1);
    if (walrus != NULL)
    {
        size_t id = entities.push_back(walrus);
        messages.insert(messages.begin(), std::string("Walrus has been deployed."));
    }
}

void checktest10(unsigned long timer)     // Check Walrus arriving to an island and creating the command center.
{
    static unsigned long timerstep = 0;
    Walrus *_walrus = (Walrus*)entities[1];

    static int stateMachine = 0;

    if (timer == 100)
    {
        _walrus->enableAuto();
        struct controlregister c;
        c.thrust = 0.0f;
        c.roll = 20.0f;
        _walrus->setControlRegisters(c);
    }


    if (timer == 275)
    {
        _walrus->enableAuto();
        struct controlregister c;
        c.thrust = 0.0f;
        c.roll = 0.0f;
        _walrus->setControlRegisters(c);
        _walrus->stop();
    }

    if (timer == 300)
    {

        _walrus->enableAuto();
        struct controlregister c;
        c.thrust = 200.0f;
        c.roll = -1;
        _walrus->setControlRegisters(c);
        _walrus->setThrottle(200.0f);
    }

    if (stateMachine == 0 && _walrus->getStatus()==Walrus::ROLLING)
    {
        timerstep = timer;
        stateMachine = 1;
    }

    if (stateMachine == 1 && timerstep>0 && timer == (timerstep + 50))
    {
        struct controlregister c;
        c.thrust = 0.0f;
        c.roll = 0;
        _walrus->setControlRegisters(c);
        _walrus->setThrottle(0.0f);
        _walrus->stop();

        timerstep = timer;
        stateMachine = 2;
    }

    if (stateMachine == 2 && timerstep>0 && timer == (timerstep + 150))
    {
        BoxIsland *island = _walrus->getIsland();
        int x = (rand() % 2000 + 1); x -= 1000;
        int z = (rand() % 2000 + 1); z -= 1000;

        Structure *s = island->addStructure(new CommandCenter(GREEN_FACTION),x,z,space,world);
        entities.push_back(s);

        timerstep = timer;
        stateMachine = 3;
    }

    if (stateMachine == 3 && timerstep>0 && timer == (timerstep + 100))
    {
        CommandCenter *c = findCommandCenter(islands[0]);

        if (!c)
        {
            printf("Test failed: CommandCenter was not built on the island.\n");
            endWorldModelling();
            exit(-1);

        } else {
            printf("Test passed OK!\n");
            endWorldModelling();
            exit(1);
        }

    }


}

void test11()
{
    // Entities will be added later in time.
    Balaenidae *_b = new Balaenidae(GREEN_FACTION);
    _b->init();
    _b->embody(world,space);
    _b->setPos(-400 kmf,20.5f,-400 kmf);
    _b->stop();

    entities.push_back(_b);
}

void checktest11(unsigned long timer)     // Check Carrier stability.
{
    if (timer>1500)
    {
        Vehicle *_b = entities[0];
        Vec3f val = _b->getPos()-Vec3f(-400 kmf,20.5f,-400 kmf);

        dReal *v = (dReal *)dBodyGetLinearVel(_b->getBodyID());
        Vec3f vec3fV;
        vec3fV[0]= v[0];vec3fV[1] = v[1]; vec3fV[2] = v[2];


        dReal *av = (dReal *)dBodyGetAngularVel(_b->getBodyID());
        Vec3f vav;
        vav[0]= av[0];vav[1] = av[1]; vav[2] = av[2];

        if (val.magnitude()>100)
        {
            printf("Test failed.\n");
            endWorldModelling();
            exit(-1);
        } else if (vav.magnitude()>10)
        {
            printf("Test failed.\n");
            endWorldModelling();
            exit(-1);
        } else if (vec3fV.magnitude()>5)
        {
            printf("Test failed.\n");
            endWorldModelling();
            exit(-1);

        } else {
            printf("Test passed OK!\n");
            endWorldModelling();
            exit(1);
        }
    }
}

void test13()
{
    // Entities will be added later in time.
    Balaenidae *_b = new Balaenidae(GREEN_FACTION);
    _b->init();
    _b->embody(world,space);
    _b->setPos(0.0f,20.5f,+4000.0f);
    _b->stop();

    entities.push_back(_b);

    entities.push_back(islands[0]->addStructure(new LaserTurret(GREEN_FACTION)     ,             0.0f,      0.0f,space,world));
}


void checktest13(unsigned long timer)    // Laser firing and hitting Carrier.
{
    if (timer==100)
    {
        LaserTurret *l=(LaserTurret*)entities[1];
        l->enableAuto();
        l->elevation = 0.3f; // Pushing down the turret.
        struct controlregister c;
        c.pitch = 0.0;
        l->setControlRegisters(c);
    }
    if (timer==500)
    {
        //dMatrix3 R;
        //ray = dCreateRay(space,4000.0f);
        //dGeomSetPosition(ray,1000.0f,20.5f,-4000.0f);   // 0 20 -4000
        //dRFromAxisAndAngle (R,0.0f,1.0f,0.0f,-90.0f);
        //dGeomSetRotation (ray,R);

        LaserTurret *l=(LaserTurret*)entities[1];
        Vehicle *action = (l)->fire(world,space);
        //int *idx = new int();
        //*idx = vehicles.push_back(action);
        //dBodySetData( action->getBodyID(), (void*)idx);
        if (action != NULL)
        {
            entities.push_back(action);
            gunshot();
        }

    }

    if (timer == 700)
    {
        Vehicle *_b = entities[0];

        if (_b->getHealth() == 1000.0f)
        {
            printf("Test failed: The laser did nothing to the Carrier.\n");
            endWorldModelling();
            exit(-1);

        } else {
            printf("Test passed OK!\n");
            endWorldModelling();
            exit(1);
        }

    }

}

void test12()
{
    entities.push_back(islands[0]->addStructure(new Turret(GREEN_FACTION)     ,         1550.0f,    0.0f,space,world));
    entities.push_back(islands[0]->addStructure(new Turret(GREEN_FACTION)     ,        -1550.0f,    0.0f,space,world));

    Walrus *_walrus = new Walrus(GREEN_FACTION);
    _walrus->init();
    _walrus->embody(world, space);
    _walrus->setPos(200.0f,1.32f,-6000.0f);
    _walrus->setStatus(Walrus::SAILING);
    _walrus->stop();

    entities.push_back(_walrus);
}

void checktest12(unsigned long timer)
{
    if (timer==100)
    {
        Turret *l=(Turret*)entities[1];
        l->enableAuto();
        l->elevation = 0.12f; // Pushing down the turret.
        l->azimuth = 159.0f;   // Moving around the turret.
        struct controlregister c;
        c.pitch = 0.0;
        l->setControlRegisters(c);

        // Extra
        controller.controlling = 0;
    }
    if (timer==500)
    {
        //dMatrix3 R;
        //ray = dCreateRay(space,4000.0f);
        //dGeomSetPosition(ray,1000.0f,20.5f,-4000.0f);   // 0 20 -4000
        //dRFromAxisAndAngle (R,0.0f,1.0f,0.0f,-90.0f);
        //dGeomSetRotation (ray,R);

        LaserTurret *l=(LaserTurret*)entities[1];
        Vehicle *action = (l)->fire(world,space);
        //int *idx = new int();
        //*idx = vehicles.push_back(action);
        //dBodySetData( action->getBodyID(), (void*)idx);
        if (action != NULL)
        {
            entities.push_back(action);
            gunshot();
        }

    }

    if (timer == 700)
    {
        Vehicle *_b = entities[0];

        if (_b->getHealth() == 1000.0f)
        {
            printf("Test failed: The laser did nothing to the Carrier.\n");
            endWorldModelling();
            exit(-1);

        } else {
            printf("Test passed OK!\n");
            endWorldModelling();
            exit(1);
        }

    }
}

void checktest14(unsigned long timer)
{
    static bool reached = false;

    if (timer == 100)
    {
        spawnManta(space,world,entities[0]);
    }

    if (timer == 290)
    {
        // launch
        launchManta(entities[0]);
    }


    if (timer == 300)
    {
        Vehicle *_b = entities[3];
        SimplifiedDynamicManta *_manta1 = (SimplifiedDynamicManta*)_b;
        _manta1->inert = false;
        _manta1->enableAuto();
        _manta1->setStatus(Manta::FLYING);
        _manta1->elevator = +5;
        struct controlregister c;
        c.thrust = 400.0f/(10.0);
        c.pitch = 5;
        _manta1->setControlRegisters(c);
        _manta1->setThrottle(400.0f);
    }

    if (timer > 501)
    {
        // Auto control
        Vehicle *_b = entities[3];
        SimplifiedDynamicManta *_manta1 = (SimplifiedDynamicManta*)_b;

        Vec3f Po = _manta1->getPos();

        float height = Po[1];

        Po[1] = 0.0f;

        Vec3f Pf(-500 kmf, 0.0f, 200 kmf);

        Vec3f T = Pf - Po;

        float eh, midpointpitch;


        if (!reached && T.magnitude()>200)
        {
            float distance = T.magnitude();

            Vec3f F = _manta1->getForward();

            F = F.normalize();
            T = T.normalize();


            float e = acos(  T.dot(F) );

            float signn = T.cross(F) [1];


            printf("T: %10.3f %10.3f %10.3f\n", distance, e, signn);


            struct controlregister c = _manta1->getControlRegisters();

            if (abs(e)>=0.5f)
            {
                c.roll = 3.0 * (signn>0?+1:-1) ;
            } else
            if (abs(e)>=0.4f)
            {
                c.roll = 2.0 * (signn>0?+1:-1) ;
            } else
            if (abs(e)>=0.2f)
                c.roll = 1.0 * (signn>0?+1:-1) ;
            else {
                c.roll = 0.0f;
            }

            eh = height-500.0f;
            midpointpitch = -15;
            c.thrust = 150.0f;

            if (distance<10000.0f)
            {
                c.thrust = 30.0f;
                midpointpitch = 17;
            }


            if ((abs(eh))>10.0f)
            {
                c.pitch = midpointpitch+1.0 * (eh>0 ? -1 : +1);
            } else {
                c.pitch = midpointpitch;
            }

            _manta1->setControlRegisters(c);
        } else
        {
            printf("Manta arrived to destination...\n");
            SimplifiedDynamicManta *_manta1 = (SimplifiedDynamicManta*)entities[3];
            _manta1->elevator = 36;
            struct controlregister c;
            c.thrust = 300.0f/(10.0);
            midpointpitch = 36;
            eh=height-500.0f;

            c.roll = -13;

            if ((abs(eh))>10.0f)
            {
                c.pitch = midpointpitch+1.0 * (eh>0 ? -1 : +1);
            } else {
                c.pitch = midpointpitch;
            }

            _manta1->setControlRegisters(c);
            _manta1->setThrottle(30.0f);
            //_manta1->stop();

            if (!reached)
            {
                char str[256];
                sprintf(str, "Manta %d has arrived to destination.", _manta1->getNumber()+1);
                messages.insert(messages.begin(), str);
                reached = true;
            }
        }



    }


}

void checktest15(unsigned long timer)
{
    if (timer==200)
    {
        Balaenidae *b = (Balaenidae*)entities[0];
        spawnWalrus(space,world,b);
    }
    if (timer==400)
    {
        Balaenidae *b = (Balaenidae*)entities[0];
        spawnWalrus(space,world,b);
    }

    if (timer>700)
    {
        printf("Test passed OK!\n");
        endWorldModelling();
        exit(1);
    }
}


void checktest16(unsigned long timer)
{
    if (timer==100)
    {
        Turret *l=(Turret*)entities[1];
        l->enableAuto();
        l->elevation = 0.23f; // Pushing down the turret.
        l->azimuth = 90.0f;   // Moving around the turret.
        struct controlregister c;
        c.pitch = 0.0;
        l->setControlRegisters(c);

        // Extra
        controller.controlling = 2;
    }
    if (timer==500)
    {
        //dMatrix3 R;
        //ray = dCreateRay(space,4000.0f);
        //dGeomSetPosition(ray,1000.0f,20.5f,-4000.0f);   // 0 20 -4000
        //dRFromAxisAndAngle (R,0.0f,1.0f,0.0f,-90.0f);
        //dGeomSetRotation (ray,R);

        LaserTurret *l=(LaserTurret*)entities[1];
        Vehicle *action = (l)->fire(world,space);
        //int *idx = new int();
        //*idx = vehicles.push_back(action);
        //dBodySetData( action->getBodyID(), (void*)idx);
        if (action != NULL)
        {
            entities.push_back(action);
            gunshot();
        }

    }

    if (timer == 700)
    {
        LaserTurret *_b = (LaserTurret*)entities[2];

        if (_b->getHealth() == 1000.0f)
        {
            printf("Test failed: The laser turret on the island has not been hit.\n");
            endWorldModelling();
            exit(-1);

        } else {
            printf("Test passed OK!\n");
            endWorldModelling();
            exit(1);
        }

    }
}

void checktest17(unsigned long timer)
{
    Turret *l=(Turret*)entities[1];

    Vehicle *_b = entities[0];


    // Find the vector between them, and the parameters for the turret to hit the carrier, regardless of its random position.

    Vec3f pos=_b->getPos();
    Vec3f tur = l->getPos();

    Vec3f aim = pos-tur;

    aim = aim.normalize();


    float azimuth = atan2(aim[2], aim[0])*180.0/PI;

    if (azimuth>=90) azimuth -= 90;
    else azimuth += 270;


    float incl = atan2(aim[1], aim[0]) * 180.0/PI;

    if (azimuth < 180.0f)
        incl += 180.0;

    printf ("Incl:%10.5f    Bg: %10.5f\n", incl, azimuth);

    if (timer==500)   // You should wait until the carrier stops moving.  This is a very interesting unexpected (but realistic) result.
    {
        // @NOTE The real test should be performed controlling the parameters to control the turret instead of the internal parameters of the turret.
        // I mean, this is not a real turret that should have mass and inertia.  This is straightforward aiming.
        l->enableAuto();
        l->elevation = incl;
        l->azimuth = azimuth;
        struct controlregister c;
        c.pitch = 0.0;
        l->setControlRegisters(c);

        // Extra
        controller.controlling = 0;   // Shift screen to carrier.
    }
    if (timer==700)
    {
        //dMatrix3 R;
        //ray = dCreateRay(space,4000.0f);
        //dGeomSetPosition(ray,1000.0f,20.5f,-4000.0f);   // 0 20 -4000
        //dRFromAxisAndAngle (R,0.0f,1.0f,0.0f,-90.0f);
        //dGeomSetRotation (ray,R);

        Vehicle *action = (l)->fire(world,space);
        //int *idx = new int();
        //*idx = vehicles.push_back(action);
        //dBodySetData( action->getBodyID(), (void*)idx);
        if (action != NULL)
        {
            entities.push_back(action);
            gunshot();
        }

    }

    if (timer == 900)
    {
        Vehicle *_b = entities[0];

        if (_b->getHealth() == 1000.0f)
        {
            printf("Test failed: Carrier has not been hit. Either aiming or fire was wrong..\n");
            endWorldModelling();
            exit(-1);

        } else {
            printf("Test passed OK!\n");
            endWorldModelling();
            exit(1);
        }

    }


}

void checktest18(unsigned long timer)
{
    Turret *l=(Turret*)entities[2];

    Vehicle *_b = entities[0];


    // Find the vector between them, and the parameters for the turret to hit the carrier, regardless of its random position.

    Vec3f pos=_b->getPos();
    Vec3f tur = l->getPos();

    Vec3f aim = pos-tur;

    aim = aim.normalize();


    float azimuth = atan2(aim[2], aim[0])*180.0/PI;

    if (azimuth>=90) azimuth -= 90;
    else azimuth += 270;


    float incl = atan2(aim[1], aim[0]) * 180.0/PI;

    if (azimuth < 180.0f)
        incl += 180.0;

    printf ("Incl:%10.5f    Bg: %10.5f\n", incl, azimuth);

    if (timer==500)   // You should wait until the carrier stops moving.  This is a very interesting unexpected (but realistic) result.
    {
        // @NOTE The real test should be performed controlling the parameters to control the turret instead of the internal parameters of the turret.
        // I mean, this is not a real turret that should have mass and inertia.  This is straightforward aiming.
        l->enableAuto();
        l->elevation = incl;
        l->azimuth = azimuth;
        struct controlregister c;
        c.pitch = 0.0;
        l->setControlRegisters(c);

        // Extra
        controller.controlling = 0;   // Shift screen to carrier.
    }
    if (timer==700)
    {
        //dMatrix3 R;
        //ray = dCreateRay(space,4000.0f);
        //dGeomSetPosition(ray,1000.0f,20.5f,-4000.0f);   // 0 20 -4000
        //dRFromAxisAndAngle (R,0.0f,1.0f,0.0f,-90.0f);
        //dGeomSetRotation (ray,R);

        Vehicle *action = (l)->fire(world,space);
        //int *idx = new int();
        //*idx = vehicles.push_back(action);
        //dBodySetData( action->getBodyID(), (void*)idx);
        if (action != NULL)
        {
            entities.push_back(action);
            gunshot();
        }

    }

    if (timer == 900)
    {
        Vehicle *_b = entities[0];

        if (_b->getHealth() == 1000.0f)
        {
            printf("Test failed: Carrier has not been hit. Either aiming or fire was wrong..\n");
            endWorldModelling();
            exit(-1);

        } else {
            printf("Test passed OK!\n");
            endWorldModelling();
            exit(1);
        }

    }

}

void checktest19(unsigned long timer)
{
    Turret *l1=(Turret*)entities[1];
    Turret *l2=(Turret*)entities[2];

    Vehicle *b=NULL;
    if (entities.exists(0))
        b = entities[0];
    else
    {
        printf("Test passed OK!\n");
        endWorldModelling();
        exit(1);
    }

    // Find the vector between them, and the parameters for the turret to hit the carrier, regardless of its random position.
    float azimuth1, inclination1;
    float azimuth2, inclination2;

    azimuth1 = getAzimuth((b->getPos())-(l1->getPos()));
    inclination1 = getDeclination((b->getPos())-(l1->getPos()));

    azimuth2 = getAzimuth((b->getPos())-(l2->getPos()));
    inclination2 = getDeclination((b->getPos())-(l2->getPos()));

    printf ("1:Incl:%10.5f    Bg: %10.5f\tIncl:%10.5f    Bg: %10.5f\n", inclination1, azimuth1, inclination2, azimuth2);

    if (timer>500 && (timer % 100 == 0))
    {

        l1->enableAuto();
        l1->elevation = inclination1;
        l1->azimuth = azimuth1;
        struct controlregister c;
        c.pitch = 0.0;
        l1->setControlRegisters(c);

        Vehicle *action = (l1)->fire(world,space);

        if (action != NULL)
        {
            entities.push_back(action);
            gunshot();
        }

    }


    if (timer>600 && (timer % 54 == 0))
    {
        l2->enableAuto();
        l2->elevation = inclination2;
        l2->azimuth = azimuth2;
        struct controlregister c;
        c.pitch = 0.0;
        l2->setControlRegisters(c);


        Vehicle *action = (l2)->fire(world,space);

        if (action != NULL)
        {
            entities.push_back(action);
            gunshot();
        }
    }


    if (timer == 3000)
    {
        Vehicle *_b = entities[0];

        if (_b->getHealth() == 1000.0f)
        {
            printf("Test failed: Carrier has not been hit. Either aiming or fire was wrong..\n");
            endWorldModelling();
            exit(-1);

        } else {
            printf("Test passed OK!\n");
            endWorldModelling();
            exit(1);
        }

    }
}

void test20()
{
    BoxIsland *nemesis = new BoxIsland();
    nemesis->setName("Nemesis");
    nemesis->setLocation(-450 kmf, -1.0, 300 kmf);
    nemesis->buildTerrainModel(space,"terrain/nm.bmp");

    islands.push_back(nemesis);

    // Entities will be added later in time.
    Balaenidae *_b = new Balaenidae(GREEN_FACTION);
    _b->init();
    _b->embody(world,space);
    _b->setPos(-450 kmf, -1.0, 300 kmf - 3000.0f);
    _b->stop();

    entities.push_back(_b);

    entities.push_back(islands[0]->addStructure(new Turret(GREEN_FACTION)     ,          550.0f,    0.0f,space,world));
    entities.push_back(islands[0]->addStructure(new Turret(GREEN_FACTION)     ,         -550.0f,    0.0f,space,world));

}

void checktest20(unsigned long timer)
{
    Turret *l1=(Turret*)entities[1];
    Turret *l2=(Turret*)entities[2];

    Vehicle *b=NULL;
    if (entities.exists(0))
        b = entities[0];
    else
    {
        printf("Test passed OK!\n");
        endWorldModelling();
        exit(1);
    }

    // Find the vector between them, and the parameters for the turret to hit the carrier, regardless of its random position.
    float azimuth1, inclination1;
    float azimuth2, inclination2;

    azimuth1 = getAzimuth((b->getPos())-(l1->getPos()));
    inclination1 = getDeclination((b->getPos())-(l1->getPos()));

    azimuth2 = getAzimuth((b->getPos())-(l2->getPos()));
    inclination2 = getDeclination((b->getPos())-(l2->getPos()));

    printf ("1:Incl:%10.5f    Bg: %10.5f\tIncl:%10.5f    Bg: %10.5f\n", inclination1, azimuth1, inclination2, azimuth2);

    if (timer>500 && (timer % 100 == 0))
    {

        l1->enableAuto();
        l1->elevation = inclination1;
        l1->azimuth = azimuth1;
        struct controlregister c;
        c.pitch = 0.0;
        l1->setControlRegisters(c);

        Vehicle *action = (l1)->fire(world,space);

        if (action != NULL)
        {
            entities.push_back(action);
            gunshot();
        }

    }


    if (timer>600 && (timer % 54 == 0))
    {
        l2->enableAuto();
        l2->elevation = inclination2;
        l2->azimuth = azimuth2;
        struct controlregister c;
        c.pitch = 0.0;
        l2->setControlRegisters(c);


        Vehicle *action = (l2)->fire(world,space);

        if (action != NULL)
        {
            entities.push_back(action);
            gunshot();
        }
    }


    if (timer == 3000)
    {
        Vehicle *_b = entities[0];

        if (_b->getHealth() == 1000.0f)
        {
            printf("Test failed: Carrier has not been hit. Either aiming or fire was wrong..\n");
            endWorldModelling();
            exit(-1);

        } else {
            printf("Test passed OK!\n");
            endWorldModelling();
            exit(1);
        }

    }
}

void test21()
{
    BoxIsland *nemesis = new BoxIsland();
    nemesis->setName("Nemesis");
    nemesis->setLocation(-450 kmf, -1.0, 300 kmf);
    nemesis->buildTerrainModel(space,"terrain/nemesis.bmp");

    islands.push_back(nemesis);

    Balaenidae *_b = new Balaenidae(GREEN_FACTION);
    _b->init();
    _b->embody(world,space);
    _b->setPos(-450 kmf, -1.0, 300 kmf - 3000.0f);
    _b->stop();

    entities.push_back(_b);

    Structure *t = islands[0]->addStructure(new Turret(BLUE_FACTION)     ,          550.0f,    0.0f,space,world);

    entities.push_back(t);


    Walrus *_walrus = new Walrus(GREEN_FACTION);

    _walrus->init();
    _walrus->embody(world, space);
    _walrus->setPos(-450 kmf+500.0f, -1.0, 300 kmf - 3000.0f);
    _walrus->setStatus(Walrus::SAILING);

    entities.push_back(_walrus);

    _walrus->setDestination(t->getPos()-Vec3f(0.0f,0.0f,-100.0f));
    _walrus->enableAuto();

}

void checktest21(unsigned long timer)   // Check arriving at Complex island like Nemesis and verify if Walrus can land on island.
{
    if (timer == 2000)
    {
        Walrus *b = findWalrus(GREEN_FACTION);
        Structure *t = islands[0]->getStructures()[0]; // Risky

        Vec3f d (b->getPos()-t->getPos());
        if (d.magnitude()>600.0f)
        {
            printf("Test failed: Walrus couldn't reach destination.\n");
            endWorldModelling();
            exit(-1);

        } else {
            printf("Test passed OK!\n");
            endWorldModelling();
            exit(1);
        }

    }
}

void test22()
{
    BoxIsland *nemesis = new BoxIsland();
    nemesis->setName("Nemesis");
    nemesis->setLocation(-450 kmf, -1.0, 300 kmf);
    nemesis->buildTerrainModel(space,"terrain/nemesis.bmp");

    islands.push_back(nemesis);

    Balaenidae *_b = new Balaenidae(GREEN_FACTION);
    _b->init();
    _b->embody(world,space);
    _b->setPos(-450 kmf, -1.0, 300 kmf - 3000.0f);
    _b->stop();

    entities.push_back(_b);

    Structure *t = islands[0]->addStructure(new Turret(BLUE_FACTION)     ,          550.0f,    0.0f,space,world);

    entities.push_back(t);


    Walrus *_walrus = new Walrus(GREEN_FACTION);

    _walrus->init();
    _walrus->embody(world, space);
    _walrus->setPos(-450 kmf+500.0f, -1.0, 300 kmf - 3000.0f);
    _walrus->setStatus(Walrus::SAILING);

    entities.push_back(_walrus);

    _walrus->setDestination(t->getPos()-Vec3f(0.0f,0.0f,-100.0f));
    _walrus->enableAuto();
}

void checktest22(unsigned long timer)
{
    Turret *l2=(Turret*)islands[0]->getStructures()[0]; // Risky
    Walrus *b = findWalrus(GREEN_FACTION);

    if (!b)
    {
        printf("Test passed OK!\n");
        endWorldModelling();
        exit(1);
    }

    // Find the vector between them, and the parameters for the turret to hit the carrier, regardless of its random position.
    float azimuth2, inclination2;

    azimuth2 = getAzimuth((b->getPos())-(l2->getPos()));
    inclination2 = getDeclination((b->getPos())-(l2->getPos()));

    printf ("Incl:%10.5f    Bg: %10.5f\n", inclination2, azimuth2);

    if (timer>600 && (timer % 54 == 0))
    {
        l2->enableAuto();
        l2->elevation = inclination2;
        l2->azimuth = azimuth2;
        struct controlregister c;
        c.pitch = 0.0;
        l2->setControlRegisters(c);


        Vehicle *action = (l2)->fire(world,space);

        if (action != NULL)
        {
            entities.push_back(action);
            gunshot();
        }
    }
}

void test23()
{
    BoxIsland *nemesis = new BoxIsland();
    nemesis->setName("Nemesis");
    //nemesis->setLocation(-450 kmf, -1.0, 300 kmf);
    nemesis->setLocation(0.0f,-1.0,0.0f);
    nemesis->buildTerrainModel(space,"terrain/nemesis.bmp");

    islands.push_back(nemesis);

    Balaenidae *_b = new Balaenidae(GREEN_FACTION);
    _b->init();
    _b->embody(world,space);
    _b->setPos(nemesis->getPos()-Vec3f(0.0f,0.0f,3000.0f));
    _b->stop();

    entities.push_back(_b);

    Structure *t = islands[0]->addStructure(new Turret(BLUE_FACTION)     ,          550.0f,    0.0f,space,world);

    entities.push_back(t);


    Walrus *_walrus = new Walrus(GREEN_FACTION);

    _walrus->init();
    _walrus->embody(world, space);
    _walrus->setPos(_b->getPos()+Vec3f(-500.0f,0.0f,0.0f));
    _walrus->setStatus(Walrus::SAILING);

    entities.push_back(_walrus);

    _walrus->setDestination(t->getPos()-Vec3f(0.0f,0.0f,100.0f));
    _walrus->enableAuto();
}


void checktest23(unsigned long timer)
{
    Turret *l2=(Turret*)islands[0]->getStructures()[0]; // Risky
    Walrus *b = findWalrus(GREEN_FACTION);
    //BoxVehicle *b = (BoxVehicle*)entities[2];

    if (!b)
    {
        printf("Test passed OK!\n");
        endWorldModelling();
        exit(1);
    }

    // Find the vector between them, and the parameters for the turret to hit the carrier, regardless of its random position.
    float azimuth2, inclination2;

    Vec3f firingloc = l2->getFiringPort();

    inclination2 = getDeclination((b->getPos())-(firingloc));
    azimuth2 = getAzimuth((b->getPos())-(firingloc));
    l2->setForward((b->getPos())-(firingloc));

    //l2->inclination = inclination2;
    //l2->azimuth = azimuth2;
    //struct controlregister c;
    //c.pitch = 0.0;
    //l2->setControlRegisters(c);

    if (timer>600 && (timer % 17 == 0))
    {
        //l2->enableAuto();
        /**l2->inclination = inclination2;
        l2->azimuth = azimuth2;
        struct controlregister c;
        c.pitch = 0.0;
        l2->setControlRegisters(c);
        **/

        std::cout << "Azimuth: " << azimuth2 << " Inclination: " << inclination2 << std::endl;


        Vehicle *action = (l2)->fire(world,space);

        if (action != NULL)
        {
            entities.push_back(action);
            gunshot();
        }
    }
}


void test24()
{
    BoxIsland *nemesis = new BoxIsland();
    nemesis->setName("Nemesis");
    nemesis->setLocation(0.0f,-1.0,0.0f);
    nemesis->buildTerrainModel(space,"terrain/nemesis.bmp");

    islands.push_back(nemesis);

    Balaenidae *_b = new Balaenidae(GREEN_FACTION);
    _b->init();
    _b->embody(world,space);
    _b->setPos(nemesis->getPos()-Vec3f(0.0f,0.0f,3000.0f));
    _b->stop();

    entities.push_back(_b);

    Structure *t = islands[0]->addStructure(new Turret(BLUE_FACTION)     ,         0.0f,    0.0f,space,world);

    entities.push_back(t);



    BoxVehicle * _bo= new BoxVehicle();
    _bo->init();
    _bo->embody(world, space);
    _bo->setPos(_b->getPos()+Vec3f(-500.0f,0.0f,0.0f));
    _bo->setPos(_bo->getPos()[0],20.1765f, _bo->getPos()[2]);
    _bo->stop();

    entities.push_back(_bo);

}

void checktest24(unsigned long timer)
{
    Turret *l2=(Turret*)islands[0]->getStructures()[0]; // Risky
    BoxVehicle *b = (BoxVehicle*)entities[2];

    static Vec3f *p = NULL;

    if (!b)
    {
        printf("Test Failed.\n");
        endWorldModelling();
        exit(0);
    }

    // Find the vector between them, and the parameters for the turret to hit the carrier, regardless of its random position.
    float azimuth, elevation;

    Vec3f firingloc = l2->getFiringPort();

    elevation = getDeclination((b->getPos())-(firingloc));
    azimuth = getAzimuth((b->getPos())-(firingloc));
    //l2->setForward((b->getPos())-(firingloc));

    l2->elevation = elevation;
    l2->azimuth = azimuth;

    struct controlregister c;
    c.pitch = 0.0;
    c.roll = 0.0;
    l2->setControlRegisters(c);

    std::cout << "Azimuth: " << azimuth << " Inclination: " << elevation << std::endl;

    if (timer>100 && !p)
    {
        p = new Vec3f(b->getPos());
    }


    if (timer>600 && (timer % 17 == 0))
    {
        //l2->enableAuto();
        /**l2->inclination = inclination2;
        l2->azimuth = azimuth2;
        struct controlregister c;
        c.pitch = 0.0;
        l2->setControlRegisters(c);
        **/


        Vehicle *action = (l2)->fire(world,space);

        if (action != NULL)
        {
            entities.push_back(action);
            gunshot();
        }
    }


    if (timer>1100)
    {
        Vec3f l=*p;

        if (!(l.isEquals(b->getPos())))
        {
            printf("Test passed OK!\n");
            endWorldModelling();
            exit(1);
        }

    }
}

static int testing=-1;

void initWorldModelling()
{
    initWorldModelling(-1);
}
void initWorldModelling(int testcase)
{
    /* create world */
    dRandSetSeed(1);
    dInitODE();
    //dInitODE2(dInitFlagManualThreadCleanup);
    //dAllocateODEDataForThread(dAllocateMaskAll);
    world = dWorldCreate();
    space = dHashSpaceCreate (0);

    //dWorldSetAutoDisableFlag(World, 1);

    // The parameter needs to be zero.
    contactgroup = dJointGroupCreate (0);
    dWorldSetGravity (world,0,-9.81f,0);
    dWorldSetCFM (world,1e-2f);   //1e-5f was the originally value used.
    dWorldSetERP(world,1.0f);   // 1 is Error Correction is applied.

    // Set Damping
    dWorldSetLinearDamping(world, 0.01f);  // 0.00001
    dWorldSetAngularDamping(world, 0.005f);     // 0.005
    dWorldSetMaxAngularSpeed(world, 200);

    // Set and get the depth of the surface layer around all geometry objects. Contacts are allowed to sink into the surface layer up to the given depth before coming to rest. The default value is zero. Increasing this to some small value (e.g. 0.001) can help prevent jittering problems due to contacts being repeatedly made and broken.
    dWorldSetContactSurfaceLayer (world,0.001f);

    ground = dCreatePlane (space,0,1,0,0);



    switch(testcase)
    {
    case 1:initIslands();test1();break;// Carrier stability
    case 2:initIslands();test1();test2();break;// Manta landing on island.
    case 3:initIslands();test1();test2();test3();break;// Manta crashing on structure
    case 4:initIslands();test1();test2();test3();test4();break;// Manta landing on runway
    case 5:initIslands();test1();test2();break;// Manta landing on aircraft
    case 6:initIslands();test1();test6();break;// Carrier stranded on island
    case 7:test1();test2();test7();break; //Manta crashing on water.
    case 8:initIslands();test1();test8();break; // Walrus reaching island.
    case 9:test1();test9();break; // Walrus stability.
    case 10:initIslands();test1();test10();break; // Walrus arrive to island and build the command center.
    case 11:initIslands();test11();break; // Carrier stability far away.
    case 12:initIslands();test1();test12();break; // Turret firing to Carrier.  Gunshot stability.
    case 13:initIslands();test13();break;   // Laser firing and hitting carrier.
    case 14:initIslands();;test14();break;  // Spawn Manta from Carrier, launch it and land back on Carrier, and dock.
    case 15:initIslands();test15();break;
    case 16:initIslands();test1();test12();break;   // Turret firing to the other Turret.
    case 17:initIslands();test1();test12();break;   // Turret automatically aiming at Carrier.
    case 18:initIslands();test1();test12();break;   // The other Turret aims automatically to the Carrier.
    case 19:initIslands();test1();test12();break;   // Both turrets aim to the carrier and start to fire.
    case 20:test20();test12();break;                // Both turrets aim to the carrier at a far away island.
    case 21:test21();break;
    case 22:test22();break;
    case 23:test23();break;                         // Set walrus to reach the shore and the turret to fire to it
    case 24:test24();break;                         // Check azimuth and declination calculation based on a forward vector.
    default:initIslands();test1();break;
    }

    testing = testcase;

}

void update(int value);

void worldStep(int value)
{
    timer++;
    update(value);

    switch(testing)
    {
    case 1:checktest1(timer);break; // Check system stability with islands and carrier.
    case 2:checktest2(timer);break; // Check Manta droping on island when trust is interrupted.
    case 3:checktest3(timer);break;
    case 4:checktest4(timer);break;
    case 5:checktest5(timer);break;
    case 6:checktest6(timer);break;
    case 7:checktest7(timer);break;
    case 8:checktest8(timer);break;
    case 9:checktest9(timer);break;
    case 10:checktest10(timer);break;
    case 11:checktest11(timer);break;
    case 12:checktest12(timer);break;
    case 13:checktest13(timer);break;
    case 14:checktest14(timer);break;
    case 15:checktest15(timer);break;
    case 16:checktest16(timer);break;
    case 17:checktest17(timer);break;
    case 18:checktest18(timer);break;
    case 19:checktest19(timer);break;
    case 20:checktest20(timer);break;
    case 21:checktest21(timer);break;
    case 22:checktest22(timer);break;
    case 23:checktest23(timer);break;
    case 24:checktest24(timer);break;
    default: break;
    }

}

void endWorldModelling()
{
    dJointGroupDestroy (contactgroup);
    dSpaceDestroy (space);
    dWorldDestroy (world);
    dCloseODE();

    printf("God knows his rules and he has determined that this world must be terminated.\n");
    printf("The World has been terminated.\n");
}

