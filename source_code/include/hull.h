#ifndef HULL_H_INCLUDED
#define HULL_H_INCLUDED

#include<cstdio>
#include<cmath>
#include<vector>
#include<algorithm>
#include<point.h>

using namespace std;

double ccw(Point p1, Point p2, Point p3);
vector<Point> convex_hull(const vector<Point> &ps);

#endif // HULL_H_INCLUDED
