#ifndef POINT_H_INCLUDED
#define POINT_H_INCLUDED

#include<cstdio>
#include<cmath>
#include<vector>
#include<algorithm>

struct Point
{
    double x, y;
    void swap(Point &p) 
    {
        Point temp(this); 
        x = p.x;
        y = p.y;
        p = temp;
    };
    Point() : x(0), y(0) {};
    Point(const double &x, const double &y) : x(x), y(y) {};
    Point(const Point &p) : x(p.x), y(p.y) {};
    Point(Point * p) : x(p->x), y(p->y) {};
    Point operator+(const Point &p) const {return Point(x+p.x,y+p.y);};
    Point operator-(const Point &p) const {return Point(x-p.x,y-p.y);};
	Point operator*(const double &l) const { return Point(x * l, y * l); };
	Point& operator*=(const double &l) { x *= l; y *= l; return *this; };
    double dot(const Point &p) {return x*p.x+y*p.y;};
	void normalize() { double norm = sqrt(pow(x, 2) + pow(y, 2)); x /= norm; y /= norm; }
	double length() { return sqrt(pow(x, 2) + pow(y, 2)); }
};


#endif // POINT_H_INCLUDED
