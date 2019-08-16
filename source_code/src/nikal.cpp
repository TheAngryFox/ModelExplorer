#include <nikal.h>

Nikal::Nikal()
{
    ALLEGRO_VERTEX_BUFFER * vb = NULL;
    ALLEGRO_VERTEX v;
    vb = al_create_vertex_buffer(NULL,&v,1,ALLEGRO_PRIM_BUFFER_STATIC);
    fast = (vb==NULL) ? false : true;
    if(vb!=NULL) al_destroy_vertex_buffer(vb);
}

void Nikal::add_filled_triangle(float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR colour)
{
    for(int i=0;i<3;i++) indcs.push_back(vtxs.size()+i);
    vtxs.push_back({x1,y1,0,-1,-1,colour});
    vtxs.push_back({x2,y2,0,-1,-1,colour});
    vtxs.push_back({x3,y3,0,-1,-1,colour});
}

void Nikal::add_filled_gradient_triangle(float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR colour_1,  ALLEGRO_COLOR colour_2,  ALLEGRO_COLOR colour_3)
{
    for(int i=0;i<3;i++) indcs.push_back(vtxs.size()+i);
    vtxs.push_back({x1,y1,0,-1,-1,colour_1});
    vtxs.push_back({x2,y2,0,-1,-1,colour_2});
    vtxs.push_back({x3,y3,0,-1,-1,colour_3});
}

void Nikal::add_sing_vert_triangle(int c1, int c2, float x, float y, ALLEGRO_COLOR colour)
{
    indcs.push_back(c1);
    indcs.push_back(c2);
    indcs.push_back(vtxs.size());
    vtxs.push_back({x,y,0,-1,-1,colour});
}

void Nikal::add_circle(float cx, float cy, float r, ALLEGRO_COLOR colour, float thickness)
{
    double precision = 1.0;
    int num_segments = 2.0 * M_PI * r * precision;

    double theta = 2.0 * M_PI / ((double)num_segments);

    double t;
    double c = cos(theta);
    double s = sin(theta);
    double x = 1;
    double y = 0;

    double inner_r = r-thickness;

    double cur_x_outer = cx+r*x;
    double cur_y_outer = cy+r*y;
    double cur_x_inner = cx+inner_r*x;
    double cur_y_inner = cy+inner_r*y;

    int c1 = vtxs.size();
    int c2 = c1 + 1;
    vtxs.push_back({(float)cur_x_outer,(float)cur_y_outer,0,-1,-1,colour});
    vtxs.push_back({(float)cur_x_inner,(float)cur_y_inner,0,-1,-1,colour});

    for (int i=0;i<num_segments;i++)
    {
        t = x;
        x = c * x - s * y;
        y = s * t + c * y;

        cur_x_outer=cx+r*x;
        cur_y_outer=cy+r*y;
        cur_x_inner=cx+inner_r*x;
        cur_y_inner=cy+inner_r*y;

        add_sing_vert_triangle(c1,c2,cur_x_outer,cur_y_outer,colour);

        c1=vtxs.size()-2;
        c2=vtxs.size()-1;

        add_sing_vert_triangle(c1,c2,cur_x_inner,cur_y_inner,colour);

        c1=vtxs.size()-2;
        c2=vtxs.size()-1;
    } 
}

void Nikal::add_filled_circle(float cx, float cy, float r, ALLEGRO_COLOR colour)
{
    double precision = 1.0;
    int num_segments = 2.0 * M_PI * r * precision;
    double theta = 2.0 * M_PI / ((double)num_segments);

    double t;
    double c = cos(theta);
    double s = sin(theta);
    double x = 1;
    double y = 0;

    double cur_x = cx+r*x;
    double cur_y = cy+r*y;

    int c1 = vtxs.size();
    int c2 = c1 + 1;
    vtxs.push_back({cx,cy,0,-1,-1,colour});
    vtxs.push_back({(float)cur_x,(float)cur_y,0,-1,-1,colour});

    for (int i=0;i<num_segments;i++)
    {
        t = x;
        x = c * x - s * y;
        y = s * t + c * y;

        cur_x=cx+r*x;
        cur_y=cy+r*y;

        add_sing_vert_triangle(c1,c2,cur_x,cur_y,colour);
        c2=vtxs.size()-1;
    }
}

void Nikal::add_line(float x1, float y1, float x2, float y2, ALLEGRO_COLOR colour, float thickness)
{
    double tx, ty;
    double len = hypot(x2-x1,y2-y1);
    if (len == 0) return;

    tx = 0.5*thickness*(y2-y1)/len;
    ty = 0.5*thickness*-(x2-x1)/len;

    add_filled_triangle(x1+tx, y1+ty, x1-tx, y1-ty, x2-tx, y2-ty,colour);
    add_sing_vert_triangle(vtxs.size()-3,vtxs.size()-1, x2+tx, y2+ty,colour);
}

void Nikal::add_gradient_line(float x1, float y1, float x2, float y2, ALLEGRO_COLOR colour_1, ALLEGRO_COLOR colour_2, float thickness)
{
    double tx, ty;
    double len = hypot(x2-x1,y2-y1);
    if (len == 0) return;

    tx = 0.5*thickness*(y2-y1)/len;
    ty = 0.5*thickness*-(x2-x1)/len;

    add_filled_gradient_triangle(x1+tx, y1+ty, x1-tx, y1-ty, x2-tx, y2-ty, colour_1, colour_1, colour_2);
    add_sing_vert_triangle(vtxs.size()-3,vtxs.size()-1, x2+tx, y2+ty, colour_2);

}

void Nikal::add_filled_cross(float x, float y, float arm_l, float arm_w, ALLEGRO_COLOR c)
{
	float mult = 1.0 / sqrt(2.0);
	add_line(x-mult*arm_l, y+mult*arm_l, x+mult*arm_l, y-mult*arm_l, c, arm_w);
	add_line(x-mult*arm_l, y-mult*arm_l, x-mult*arm_w*0.5, y-mult*arm_w*0.5, c, arm_w);
	add_line(x+mult*arm_w*0.5, y+mult*arm_w*0.5, x+mult*arm_l, y+mult*arm_l, c, arm_w);
}

void Nikal::draw()
{
	if (vtxs.size() > 0)
	{
		if (fast)
		{
			ALLEGRO_VERTEX_BUFFER * vb;
			ALLEGRO_INDEX_BUFFER * ib;
			vb = al_create_vertex_buffer(NULL, &vtxs[0], vtxs.size(), ALLEGRO_PRIM_BUFFER_STATIC);
			ib = al_create_index_buffer(4, &indcs[0], indcs.size(), ALLEGRO_PRIM_BUFFER_STATIC);
			if (vb == NULL) printf("\n\nFAILED TO CREATE A VERTEX BUFFER! Your GPU may be f***ing things up...\n\n");
			else
			{
				al_draw_indexed_buffer(vb, NULL, ib, 0, indcs.size(), ALLEGRO_PRIM_TRIANGLE_LIST);
				al_destroy_vertex_buffer(vb);
				al_destroy_index_buffer(ib);
			}
			vtxs.clear();
			indcs.clear();
		}
		else
		{
			if (indcs.size() != 0) al_draw_indexed_prim(&vtxs[0], NULL, NULL, &indcs[0], indcs.size(), ALLEGRO_PRIM_TRIANGLE_LIST);
			vtxs.clear();
			indcs.clear();
		}
	}
}
