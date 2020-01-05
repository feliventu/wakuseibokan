#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdarg.h>
#include <math.h>
#include <string.h>

#include <GLUT/glut.h>

#include <ode/ode.h>

#include <vector>

#include <iostream>
#include <unordered_map>

#include "container.h"
#include "ThreeMaxLoader.h"

#include "font/DrawFonts.h"

#include "math/yamathutil.h"

#include "usercontrols.h"
#include "camera.h"

#include "openglutils.h"

#include "keplerivworld.h"

#include "units/BoxVehicle.h"
#include "units/Manta.h"

#include "openglutils.h"

#include "imageloader.h"
#include "terrain/Terrain.h"

#include "units/Vehicle.h"

#include "map.h"


extern std::vector<BoxIsland*> islands;

extern container<Vehicle*> entities;

std::unordered_map<const char *, GLuint> maptextures;

int mapzoom=1;
int cx=1200/2,cy=800/2;


void placeIsland(int x, int y, int size, const char* modelName, const char *name)
{
    char str[256];
    sprintf (str, name);
    //drawString(0+x-10,y-20,0,str,0.1f,1.0f,1.0f,1.0f);

    // FIXME: Textures should be loaded only once at the beginning.  There may be a leak here.
    //Image* image = loadBMP("terrain/vulcrum.bmp");


    GLuint _textureBox;

    if (maptextures.find(modelName) == maptextures.end())
    {
        Image* image = loadBMP(modelName);
        _textureBox = loadTexture(image);
        delete image;
    } else {
        _textureBox = maptextures[modelName];
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _textureBox);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glColor3f(1.0f, 1.0f, 0.0f);

    glBegin(GL_QUADS);

    int BOX_SIZE=size;

    //Front face
    glNormal3f(0.0, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-BOX_SIZE / 2 + x, -BOX_SIZE / 2 + y, 0);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(BOX_SIZE / 2 + x, -BOX_SIZE / 2 + y, 0);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(BOX_SIZE / 2 + x, BOX_SIZE / 2 + y, 0);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-BOX_SIZE / 2 + x, BOX_SIZE / 2 + y, 0);

    glEnd();
}

void zoommapin()
{
    mapzoom++;
}

void zoommapout()
{
    mapzoom--;
}

void centermap(int ccx, int ccy)
{
    int xsize = 1200/mapzoom;
    int ysize = 800/mapzoom;

    // @FIXME: This should be adapted according to the screen resolution.
    cx = (int)(ccx*(xsize)/1500.0)+cx-xsize/2;
    cy = (int)(ccy*(ysize)/900.0)+cy-ysize/2;
}


void drawMap()
{
    // This will make things dark.


    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    int xsize = 1200/mapzoom;
    int ysize = 800/mapzoom;

    if (mapzoom==1)
    {
        cx = 1200/2;
        cy = 800/2;
    }


    glOrtho(cx-xsize/2, cx+xsize/2, cy+ysize/2, cy-ysize/2, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor4f(1.0f, 1.0f, 1.0f, 1);
    glDisable(GL_DEPTH_TEST);
    glRotatef(180.0f,0,0,1);
    glRotatef(180.0f,0,1,0);




    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    char str[256];
    sprintf (str, "Kepler IV Sea");
    // width, height, 0 0 upper left
    drawString(0,-30,1,str,0.2f,1.0f,1.0f,1.0f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); {
        glTranslatef(0, -400, 1);


        for(int i=0;i<10;i++)
        {
            glLineWidth(2.5);
            glColor3f(0.0, 1.0, 0.0);
            glBegin(GL_LINES);
            glVertex3f(   1, -500+i*100, 0.0);
            glVertex3f(1200, -500+i*100, 0.0);
            glEnd();
        }

        for(int i=0;i<12;i++)
        {
            glLineWidth(2.5);
            glColor3f(0.0, 1.0, 0.0);
            glBegin(GL_LINES);
            glVertex3f(   1+i*100,  500, 0.0);
            glVertex3f(   1+i*100, -500, 0.0);
            glEnd();
        }

        glLineWidth(2.5);
        glColor3f(0.0, 1.0, 0.0);
        glBegin(GL_LINES);
        glVertex3f(   1200,  500, 0.0);
        glVertex3f(   1200, -500, 0.0);
        glEnd();


        /**
        glLineWidth(2.5);
        glColor3f(1.0, 0.0, 0.0);
        glBegin(GL_LINES);
        glVertex3f(590, 0.0, 0.0);
        glVertex3f(690, 0, 0);
        glEnd();


        glBegin(GL_LINES);
        glVertex3f(590, 1, 0.0);
        glVertex3f(690, + 1, 0);
        glEnd();

        **/

        for(int i=0;i<islands.size();i++)
        {
            BoxIsland *b = islands[i];

            drawString(600-(b->getX()/1000)-10,(b->getZ()/1000)-20,0,(char*)b->getName().c_str(),0.1f,1.0f,1.0f,1.0f);
        }


        synchronized(entities.m_mutex)
        {
            for(size_t i=entities.first();entities.exists(i);i=entities.next(i))
            {
                if (entities[i]->getType() == CARRIER)
                {
                    drawString(600-entities[i]->getPos()[0]/1000-10,entities[i]->getPos()[2]/1000,0,"B",0.1f,1.0f,1.0f,0.0f);
                } else if (entities[i]->getType() == WALRUS)
                {
                    drawString(600-entities[i]->getPos()[0]/1000-10,entities[i]->getPos()[2]/1000,0,"W",0.1f,0.0f,1.0f,1.0f);
                } else if (entities[i]->getType() == MANTA)
                {
                    drawString(600-entities[i]->getPos()[0]/1000-10,entities[i]->getPos()[2]/1000,0,"M",0.1f,1.0f,0.9f,0.5f);
                }
            }
        }




        for(int i=0;i<islands.size();i++)
        {
            BoxIsland *b = islands[i];

            placeIsland(600-(b->getX()/1000),0+(b->getZ()/1000),10, b->getModelName().c_str(), b->getName().c_str());
        }



    } glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();




    /**
    char str[256];


    if (isnan(Camera.pos[0])) exit(1);

    float fps = getFPS();

    sprintf (str, "fps %4.2f  Cam: (%5.2f\t,%5.2f\t,%5.2f\t)\n", fps, Camera.pos[0],Camera.pos[1],Camera.pos[2]);
    // width, height, 0 0 upper left
    drawString(0,-30,1,str,0.2f);

    //glPrint(1,10,10,"HUD");

    //glRectf(400.0f,400.0f,450.0f,400.0f);

    float speed=0;

    if (controller.controlling >0)
        speed = vehicles[controller.controlling-1]->getSpeed();

    sprintf (str, "Speed:%10.2f - X,Y,Z,P (%5.2f,%5.2f,%5.2f,%5.2f)\n", speed, controller.roll,controller.pitch,controller.yaw,controller.precesion);
    drawString(0,-60,1,str,0.2f);

    sprintf (str, "Vehicle:%d  - Thrust:%5.2f\n", controller.controlling,controller.thrust);
    drawString(0,-90,1,str,0.2f);

    if (controller.isTeletype())
    {
        sprintf(str, ">>>%s",controller.str.c_str());
        drawString(0,-180,1,str,0.2f);
    }

    // Message board
    if (messages.size()>0)
    {
        for(int i=0;i<messages.size();i++)
        {
            std::string line = messages[i];
            if (i==0)
                drawString(0,-700-i*25,1,(char*)line.c_str(),0.2f,1.0f,1.0f,0.0f);
            else
                drawString(0,-700-i*25,1,(char*)line.c_str(),0.2f);
        }

    }




    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); {
        glTranslatef(0, -400, 1);

        glLineWidth(2.5);
        //glColor3f(1.0, 0.0, 0.0);
        glBegin(GL_LINES);
        glVertex3f(590, 0.0, 0.0);
        glVertex3f(690, 0, 0);
        glEnd();


        glBegin(GL_LINES);
        glVertex3f(590, Camera.yAngle, 0.0);
        glVertex3f(690, + Camera.yAngle, 0);
        glEnd();

    } glPopMatrix();


    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    **/
}
