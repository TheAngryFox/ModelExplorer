#include <hull.h>

double ccw(Point p1, Point p2, Point p3)
{
    return (p2.x - p1.x)*(p3.y - p1.y) - (p2.y - p1.y)*(p3.x - p1.x);
}

vector<Point> convex_hull(const vector<Point> &ps)
{
    int N = ps.size();
    if(N>0)
    {
        vector<Point> points (N+1);
        for(int i=0;i<N;i++) points[i+1] = ps[i];
        pair<int,Point> lowest(1,Point(points[1].x,points[1].y));
        for(int i=2;i<(N+1);i++) 
        {
            if(points[i].y<lowest.second.y || (points[i].y==lowest.second.y && points[i].x<lowest.second.x)) 
            {
                lowest.first=i; 
                lowest.second.x=points[i].x; 
                lowest.second.y=points[i].y;
            }
        }
        points[1].swap(points[lowest.first]);
        sort(points.begin()+2,points.end(),[&points](const Point &a,const Point &b) {return ccw(points[1],a,b)>0;});
        points[0]=points[N];

        int M = 1;
        for(int i=2;i<(N+1);i++)
        {
            while(ccw(points[M-1], points[M], points[i])<=0)
            {
                if(M>1) M--;
                else if(i==N) break;
                else i++;
            }
            M++;
            points[M].swap(points[i]);
        }

        return vector<Point>(points.begin(),points.begin()+M);
    }
    else return vector<Point>();
}