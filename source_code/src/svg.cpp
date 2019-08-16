#include <svg.h>

SVG::SVG(double width, double height) : width(width), height(height)
{
	svg = svgtext.append_child("svg");
	pugi::xml_attribute xmlns = svg.append_attribute("xmlns");
	pugi::xml_attribute xlink = svg.append_attribute("xmlns:xlink");
	pugi::xml_attribute wi = svg.append_attribute("width");
	pugi::xml_attribute hi = svg.append_attribute("height");
	xmlns.set_value("http://www.w3.org/2000/svg");
	xlink.set_value("http://www.w3.org/1999/xlink");
	wi.set_value(width);
	hi.set_value(height);
}

SVG::SVG(const SVG &s) : width(s.width), height(s.height) 
{
	svgtext.reset(s.svgtext);
	svg = svgtext.child("svg");
}

int SVG::save(string path_to_file)
{
	ofstream svgfile(path_to_file, ios::out | ios::trunc);
	svgtext.save(svgfile);
	svgfile.close();

	string message = "\n" + path_to_file + " (svg) saved!\n";
	printf("%s", message.c_str());

	return 0;
}

void SVG::add_filled_triangle(double x1, double y1, double x2, double y2, double x3, double y3, ALLEGRO_COLOR colour)
{
	pugi::xml_node tri = svg.append_child("polygon");
	pugi::xml_attribute points = tri.append_attribute("points");
	pugi::xml_attribute style = tri.append_attribute("style");

	string p = to_string(x1) + "," + to_string(y1) + " " + to_string(x2) + "," + to_string(y2) + " " + to_string(x3) + "," + to_string(y3) + " ";
	unsigned char R, G, B, A;
	al_unmap_rgba(colour, &R, &G, &B, &A);
	string col = "rgb(" + to_string(R) + "," + to_string(G) + "," + to_string(B) + ")";
	string s = "stroke:none; fill:" + col + "; fill-opacity:" + to_string((double) A / 255.);

	points.set_value(p.c_str());
	style.set_value(s.c_str());
}

void SVG::add_filled_rectangle(double x1, double y1, double x2, double y2, ALLEGRO_COLOR colour)
{
	pugi::xml_node rect = svg.append_child("rect");
	pugi::xml_attribute x = rect.append_attribute("x");
	pugi::xml_attribute y = rect.append_attribute("y");
	pugi::xml_attribute width = rect.append_attribute("width");
	pugi::xml_attribute height = rect.append_attribute("height");
	pugi::xml_attribute style = rect.append_attribute("style");

	unsigned char R, G, B, A;
	al_unmap_rgba(colour, &R, &G, &B, &A);
	string col = "rgb(" + to_string(R) + "," + to_string(G) + "," + to_string(B) + ")";
	string s = "stroke:none; fill:" + col + "; fill-opacity:" + to_string((double) A / 255.);

	x.set_value(x1);
	y.set_value(y1);
	width.set_value(x2-x1);
	height.set_value(y2-y1);
	style.set_value(s.c_str());
}


void SVG::add_filled_circle(double cx, double cy, double r, ALLEGRO_COLOR colour)
{
	pugi::xml_node cir = svg.append_child("circle");
	pugi::xml_attribute cx_ = cir.append_attribute("cx");
	pugi::xml_attribute cy_ = cir.append_attribute("cy");
	pugi::xml_attribute r_ = cir.append_attribute("r");
	pugi::xml_attribute style = cir.append_attribute("style");

	unsigned char R, G, B, A;
	al_unmap_rgba(colour, &R, &G, &B, &A);
	string col = "rgb(" + to_string(R) + "," + to_string(G) + "," + to_string(B) + ")";
	string s = "stroke:none; fill:" + col + "; fill-opacity:" + to_string((double) A / 255.);

	cx_.set_value(cx);
	cy_.set_value(cy);
	r_.set_value(r);
	style.set_value(s.c_str());
}

void SVG::add_circle(double cx, double cy, double r, ALLEGRO_COLOR colour, double thickness)
{
	pugi::xml_node cir = svg.append_child("circle");
	pugi::xml_attribute cx_ = cir.append_attribute("cx");
	pugi::xml_attribute cy_ = cir.append_attribute("cy");
	pugi::xml_attribute r_ = cir.append_attribute("r");
	pugi::xml_attribute style = cir.append_attribute("style");

	unsigned char R, G, B, A;
	al_unmap_rgba(colour, &R, &G, &B, &A);
	string col = "rgb(" + to_string(R) + "," + to_string(G) + "," + to_string(B) + ")";
	string s = "stroke:" + col + "; stroke-opacity:" + to_string((double) A / 255.) + "; stroke-width:" + to_string(thickness) + "; fill:none";

	cx_.set_value(cx);
	cy_.set_value(cy);
	r_.set_value(r);
	style.set_value(s.c_str());
}

void SVG::add_line(double x1, double y1, double x2, double y2, ALLEGRO_COLOR colour, double thickness)
{
	pugi::xml_node lin = svg.append_child("line");
	pugi::xml_attribute x1_ = lin.append_attribute("x1");
	pugi::xml_attribute y1_ = lin.append_attribute("y1");
	pugi::xml_attribute x2_ = lin.append_attribute("x2");
	pugi::xml_attribute y2_ = lin.append_attribute("y2");
	pugi::xml_attribute style = lin.append_attribute("style");

	unsigned char R, G, B, A;
	al_unmap_rgba(colour, &R, &G, &B, &A);
	string col = "rgb(" + to_string(R) + "," + to_string(G) + "," + to_string(B) + ")";
	string s = "stroke:" + col + "; stroke-opacity:" + to_string((double) A / 255.) + "; stroke-width:" + to_string((int)thickness);

	x1_.set_value(x1);
	y1_.set_value(y1);
	x2_.set_value(x2);
	y2_.set_value(y2);
	style.set_value(s.c_str());
}

void SVG::add_filled_cross(float x, float y, float arm_l, float arm_w, ALLEGRO_COLOR c)
{
	float mult = 1.0 / sqrt(2.0);
	add_line(x-mult*arm_l, y+mult*arm_l, x+mult*arm_l, y-mult*arm_l, c, arm_w);
	add_line(x-mult*arm_l, y-mult*arm_l, x-mult*arm_w*0.5, y-mult*arm_w*0.5, c, arm_w);
	add_line(x+mult*arm_w*0.5, y+mult*arm_w*0.5, x+mult*arm_l, y+mult*arm_l, c, arm_w);
}
