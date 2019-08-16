#ifndef NIKAL_H_INCLUDED
#define NIKAL_H_INCLUDED

#define ALLEGRO_STATICLINK

#include<string>
#include<cstdio>
#include<iostream>
#define _USE_MATH_DEFINES
#include<math.h>
#include<vector>
#include<algorithm>

#include "allegro5/allegro.h"
#include "allegro5/allegro_primitives.h"

using namespace std;

class Nikal
{
    vector<ALLEGRO_VERTEX> vtxs;
    vector<int> indcs;
    bool fast;
    void add_sing_vert_triangle(int c1, int c2, float x, float y, ALLEGRO_COLOR colour);
public:
	const string type = "Nikal";

    Nikal ();
    void add_filled_triangle(float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR colour);
    void add_filled_gradient_triangle(float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR colour_1,  ALLEGRO_COLOR colour_2,  ALLEGRO_COLOR colour_3);
    void add_filled_circle(float cx, float cy, float r, ALLEGRO_COLOR colour);
    void add_circle(float cx, float cy, float r, ALLEGRO_COLOR colour, float thickness);
    void add_line(float x1, float y1, float x2, float y2, ALLEGRO_COLOR colour, float thickness);
    void add_gradient_line(float x1, float y1, float x2, float y2, ALLEGRO_COLOR colour_1, ALLEGRO_COLOR colour_2, float thickness);
    void add_filled_cross(float x, float y, float arm_l, float arm_w, ALLEGRO_COLOR c);
	void draw();
    int get_buffer_size() {return (int) vtxs.size();};
};


#endif // NIKAL_H_INCLUDED
