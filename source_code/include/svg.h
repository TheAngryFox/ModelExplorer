#ifndef SVG_H_INCLUDED
#define SVG_H_INCLUDED

#define ALLEGRO_STATICLINK

#include<string>
#include<cstdio>
#include<iostream>
#define _USE_MATH_DEFINES
#include<math.h>
#include<vector>
#include<fstream>
#include <pugixml.hpp>	

#include "allegro5/allegro.h"
#include "allegro5/allegro_color.h"

using namespace std;

class SVG
{
	double height, width;
	pugi::xml_document svgtext;
	pugi::xml_node svg;
public:
	const string type = "SVG";
	SVG(double width,double height);
	SVG(const SVG &s);
	int save(string path_to_file);
	void add_filled_triangle(double x1, double y1, double x2, double y2, double x3, double y3, ALLEGRO_COLOR colour);
	void add_filled_rectangle(double x1, double y1, double x2, double y2, ALLEGRO_COLOR colour);
	void add_filled_circle(double cx, double cy, double r, ALLEGRO_COLOR colour);
	void add_circle(double cx, double cy, double r, ALLEGRO_COLOR colour, double thickness);
	void add_line(double x1, double y1, double x2, double y2, ALLEGRO_COLOR colour, double thickness);
	void add_filled_cross(float x, float y, float arm_l, float arm_w, ALLEGRO_COLOR c);
};

#endif // SVG_H_INCLUDED
