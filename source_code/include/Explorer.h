#ifndef EXPLORER_H_INCLUDED
#define EXPLORER_H_INCLUDED

#define ALLEGRO_STATICLINK

#include<regex>
#include<string>
#include<cstdio>
#include<iostream>
#include<fstream>
#define _USE_MATH_DEFINES
#include<cmath>
#include<vector>
#include<algorithm>
#include<queue>
#include<set>
#include<random>
#include<chrono>
#include<bitset>
#include<thread>
#include<mutex>
#include<sstream>

#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_ttf.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_primitives.h"

#include <boost/filesystem.hpp>

#include "ClpInterior.hpp"
#include "ClpSimplex.hpp"
#include "ClpPrimalColumnSteepest.hpp"
#include "ClpDualRowDantzig.hpp"
#include "ClpDualRowSteepest.hpp"
#include "ClpPresolve.hpp"
#include "CoinTime.hpp"
#include "CoinBuild.hpp"
#include "CoinModel.hpp"

#include <cgraph.h>
#include <gvc.h>
#include <cdt.h>

#include <FBA_model.h>
#include <Tree.h>
#include <nikal.h>
#include <fileops.h>
#include <hull.h>
#include <svg.h>

using namespace std;

#define COL ALLEGRO_COLOR
#define FONT ALLEGRO_FONT
#define BITMAP ALLEGRO_BITMAP

template<class T>
using Array = vector<vector<T> >;

class Explorer;

FONT* load_ttf_font(const string &font_path, int font_size, FONT** back_pointer); // Conservative loading function which ensures every font size/path combination is only loaded once
void attempt_destroy_font(const string &font_path, int font_size, FONT** back_pointer); 
void clear_font_storage();
void set_font_scaling (double factor);

enum struct event_type {idle,move_mouse,m_leave_disp,rmb_down,lmb_down,rmb_up,lmb_up,scroll,keyboard};
enum struct response_type {do_nothing,load_model,zoom,lmb_down_custom,lmb_up_custom,lmb_move_mouse,rmb_down_custom,rmb_up_custom,rmb_move_mouse,cust_move_mouse,
                    ver_area_resize,hor_area_resize,ver_area_resize_hov,hor_area_resize_hov,at_least_inside,special_lock,lmb_click,tab_key};
enum struct button_type {click,menu_click,switcher,palette,menu_root,principal_menu_root};

class Area
{
protected:
    double prev_mouse_x, prev_mouse_y;
    string name;
    double x0, y0, x1, y1;
    bool on;
    double scaling = 1.0;
public:
    Area(double x0, double y0, double x1, double y1, bool on) : x0(x0), y0(y0), x1(x1), y1(y1), on(on) {};
    Area() : x0(-1), y0(-1), x1(-1), y1(-1), on(true) {};
    virtual ~Area() {};
    bool mouse_inside(double mouse_x,double mouse_y) {if(mouse_x>=x0 && mouse_x<=x1 && mouse_y>=y0 && mouse_y<=y1) return true; else return false;};
    virtual void Resize (double x_0, double y_0, double x_1, double y_1)=0;
    virtual void draw (ALLEGRO_DISPLAY * disp)=0;
    virtual response_type respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in, pair<char,char> input)=0;
    virtual double get_min_x_dim()=0;
    virtual double get_min_y_dim()=0;
    virtual void set_scaling(double factor)=0;
    bool is_on() {return on;};
    void turn_on() {on = true;};
    void turn_off() {on = false;};
    double get_x0() {return x0;};
    double get_y0() {return y0;};
    double get_x1() {return x1;};
    double get_y1() {return y1;};
    double get_prev_mouse_x() {return prev_mouse_x;};
    double get_prev_mouse_y() {return prev_mouse_y;};
};

class Scroll: public Area
{
private:
    COL margin_c = al_map_rgb(200,200,200);
    COL whitespace_c = al_map_rgb(255,255,255);
    COL bar_c = al_map_rgb(0,127,0);
    COL mov_bar_c = al_map_rgb(0,100,0);
    double v_margin = 4;
    double h_margin = 4;
    double bar_pos;
    double bar_height;
    double fraction;
    bool bar_mov = false;
public:
    Scroll(double x0, double y0, double x1, double y1, bool on, double frac);
    ~Scroll() {};
    void Resize (double x_0, double y_0, double x_1, double y_1);
    void change_scroll(double y0, double y1, double frac);
    response_type respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in,pair<char,char> input);
    double get_min_x_dim() {return x1-x0;};
    double get_min_y_dim() {return 2*v_margin*scaling+2*((x1-x0)-2*h_margin*scaling);};
	double get_min_tot_height() { return 2 * v_margin + ((x1 - x0) - 2 * h_margin); };
    double where_is_bar();
	void reset_bar();
	void rescale_bar(double rat) { fraction = min(rat,1.0);  bar_height = max(fraction * (y1 - y0 - 2 * v_margin*scaling), x1 - x0 - 2 * h_margin*scaling); };
    int where_is_mouse(double m_x,double m_y);
    double mov(double d_pos);
	void set_barpos_by_rat(double new_rat);
    void draw(ALLEGRO_DISPLAY * disp);
    void set_scaling(double factor);
    void bar_moving(bool mov) {bar_mov=mov;};
    void shift(double dx,double dy) {x0+=dx; x1+=dx; y0+=dy; y1+=dy;}
};

template<class T>
class Text: public Area
{
private:
	bool selectable = false;
	typedef void (T::*cb_fun_ptr)(int n, vector<string> &s, int select, bool &new_text);
	cb_fun_ptr cb_fun;
	T * cb_ob = NULL;
	int id;

    COL def_text_c = al_map_rgb(0,0,0);
    COL title_text_c = al_map_rgb(255,255,255);
    COL separator_c = al_map_rgb(127,127,127);
	COL selectable_separator_c = al_map_rgb(200, 200, 200);
    COL margin_c = al_map_rgb(0,100,0);
    COL background_c = al_map_rgb(255,255,255);
    COL title_background_c = al_map_rgb(0,127,0);
    BITMAP * full_text = NULL;
    FONT * font = NULL;
    FONT * title_font = NULL;
    vector<string> title;
    vector<string> tex;
    vector<string> raw_str;
    vector<COL> tex_c;

	/// selectable parameters
	vector<pair<int, int>> tab_line_links;	// links from list entries to lines in the text
	int tab = -1;								// which list entry is being clicked
	/// 

    string font_file_name = "arial.ttf";
    int font_size;
    int title_font_size;
    double line_spacing = 1.5;

    double v_margin = 0;
    double h_margin = 0;
    double h_space; // 0.5 of font size 
    double title_h_space; // 0.5 of title font size
    double separator_h; // 0.1 of font size
	double selectable_separator_h = 0; 
    double scroll_lines = 1.7;

    double title_height;
    double text_height;
    double where_bar_is;
public:
    Text(double x0, double y0, double x1, double y1, bool on, vector<string> str, int font_size = 20, int title_font_size = 20);
	Text(double x0, double y0, double x1, double y1, bool on, vector<string> str, cb_fun_ptr cb_fun, T * cb_ob, int id, int font_size = 20, int title_font_size = 20);
    ~Text() {if(full_text!=NULL) al_destroy_bitmap(full_text); attempt_destroy_font(font_file_name,font_size,&font); attempt_destroy_font(font_file_name,title_font_size,&title_font);};
    void Change_text(vector<string> str);
    void Resize (double x_0, double y_0, double x_1, double y_1);
    response_type respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in,pair<char,char> input);
    double get_min_x_dim() {return 2*h_margin*scaling+2*max(h_space,title_h_space)*scaling+3*title_font_size*scaling;};
    double get_min_y_dim() {return 2*v_margin*scaling+3*title_font_size*scaling;};
    double get_frac() {return (y1-y0-2*v_margin*scaling-title_height)/text_height;};
	double get_min_tot_height () { return scaling * (2 * v_margin) + title_height + text_height; };
	bool text_empty() { return (tex.empty()) ? true : false; }
    void set_pos(double Where_bar_is) {where_bar_is=Where_bar_is;};
    void draw(ALLEGRO_DISPLAY * disp);
    void set_scaling(double factor);
    void redraw(ALLEGRO_DISPLAY * disp);
	int tab_from_pos(double mouse_x, double mouse_y);
    double get_scroll_lines() {return scroll_lines;};
    int get_font_size() {return font_size;};
    double get_line_spacing() {return line_spacing;};
    void shift(double dx,double dy) {x0+=dx; x1+=dx; y0+=dy; y1+=dy;}
};

template <class T>
class Text_view: public Area
{
private:
	bool selectable = false;
	typedef void (T::*cb_fun_ptr)(int n, vector<string> &s, int select, bool &new_text);
	double max_size;

    double scroll_w; // equal to font size / 5 * 8
    double v_margin, h_margin;
	COL margin_c;
    Text<T> * t = NULL;
    Scroll * s = NULL;

	int text_lock = -1;
public:
    Text_view(double x0, double y0, double x1, double y1, bool on, vector<string> &str, int font_size = 20, int title_font_size = 20, double v_margin = 0, double h_margin = 0, COL margin_c = al_map_rgb(0, 0, 0), double max_size = -1, cb_fun_ptr cb_fun = NULL, T * cb_ob = NULL, int id = -1);
    ~Text_view() {if(t!=NULL) delete t; if(s!=NULL) delete s;};
    void Resize (double x_0, double y_0, double x_1, double y_1);
    void Change_text(vector<string> &str);
	bool text_empty() { return t->text_empty();  };
	double get_scroll_w() { return s->get_min_x_dim(); };
    response_type respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in,pair<char,char> input);
    double get_min_x_dim() {return s->get_min_x_dim()+t->get_min_x_dim();};
    double get_min_y_dim() {return max(s->get_min_y_dim(),t->get_min_y_dim());};
	double get_min_tot_height() { return  max(s->get_min_tot_height(), t->get_min_tot_height());};
	void clean();
	void check_height(ALLEGRO_DISPLAY * disp);
    void draw(ALLEGRO_DISPLAY * disp);
    void set_scaling(double factor);
    void shift(double dx,double dy);
};

template<class T>
class Menu;

template<class T>
struct button_template
{
    button_type k;
    typedef void (T::*cb_fun_ptr)(int n, string &s);
    cb_fun_ptr cb_fun;
    T * cb_ob = NULL;
    string title;
    int id = -1;
    vector<button_template> menu;
    button_template(button_type k,cb_fun_ptr cb_fun,T * cb_ob,string title, int id) : k(k), cb_fun(cb_fun), cb_ob(cb_ob), title(title), id(id) {};
    button_template(button_type k,string title) : k(k), title(title) {};
};

template<class T>
struct text_inp_template
{
	typedef void (T::*cb_fun_ptr1)(int n, string &s);
	typedef void (T::*cb_fun_ptr2)(int n, vector<string> &s, int select, bool &new_text);
    cb_fun_ptr1 cb_fun1 = NULL;
	cb_fun_ptr2 cb_fun2 = NULL;

	double max_h;
	double width;

    T * cb_ob = NULL;
    string title;
    int id = -1;

    text_inp_template(cb_fun_ptr1 cb_fun1, T * cb_ob, string title, int id) : cb_fun1(cb_fun1), cb_ob(cb_ob), title(title), id(id) {};
	text_inp_template(cb_fun_ptr2 cb_fun2, T * cb_ob, string title, int id, double max_h, double width) : cb_fun2(cb_fun2), cb_ob(cb_ob), title(title), id(id), max_h(max_h), width(width) {};
};

template<class T>
struct inp_view_template
{
	vector<button_template<T> > buttons;
	vector<text_inp_template<T> > text_inputs;
	vector<vector<text_inp_template<T> >> input_lists;
	vector<vector<double>> list_widths;
	vector<string> list_titles;
	vector<vector<pair<char, int>>> order;

	inp_view_template(vector<button_template<T> > buttons, vector<text_inp_template<T> > text_inputs, vector<vector<text_inp_template<T> >> input_lists, vector<vector<double>> list_widths, vector<string> list_titles,  vector<vector<pair<char, int>>> order)
		: buttons(buttons), text_inputs(text_inputs), input_lists(input_lists), list_widths(list_widths), list_titles(list_titles), order(order) {};
};

template<class T>
class Button: public Area
{
private:
    button_type k;
    typedef void (T::*cb_fun_ptr)(int n, string &s);
    cb_fun_ptr cb_fun;
    T * cb_ob;

    Menu<T> * menu = NULL;

    string title;
    int id;
    double margin;
    double corner_r;
    COL button_c;
    COL p_button_c;
    COL margin_c;
    COL p_margin_c;
    COL font_c;
    COL p_font_c;
    COL tick_c;

    COL def_palette_c;
    COL prev_palette_c;
    COL palette_c;
    double palette_dim = 255.;
    double colourbar_w = 20.;
    Button<T> * Pick = NULL;
    Button<T> * Reset = NULL;
    Button<T> * Default = NULL;
    BITMAP * huebar = NULL;
    double hue = -1; // The position on he hue bar
    double palette_pos [2] = {-1,-1}; // The position of the hue bar 
    int palette_lock = -1; // -1 - no lock, 0 - generic lock, 1 - palette lock, 2 - huebar lock, 3 - pick, 4 - reset, 5 - default

    double space;
    double font_size;
    string font_file_name;
    FONT * font = NULL;

    bool highlight = false;
    bool active = false;
public:
    Button(double x_0, double y_0, double font_size, string font_file_name, button_template<T> t, double margin, double space, double corner_r, COL button_c, COL p_button_c, COL margin_c, COL p_margin_c, COL font_c, COL p_font_c);
    Button(double x0, double y0, double x1, double y1, double font_size, string font_file_name, button_template<T> t, double margin, double space, double corner_r, COL button_c, COL p_button_c, COL margin_c, COL p_margin_c, COL font_c, COL p_font_c, COL tick_c);
    ~Button() {attempt_destroy_font(font_file_name,font_size,&font); if(huebar!=NULL) al_destroy_bitmap(huebar); delete menu; delete Pick; delete Reset; delete Default;};
    response_type respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in,pair<char,char> input);
    void Resize (double x_0, double y_0, double x_1, double y_1);
    double get_min_x_dim() {return x1-x0;};
    double get_min_y_dim() {return y1-y0;};
    void draw(ALLEGRO_DISPLAY * disp);
    void set_scaling(double factor);
    void run_function() {string reply; (cb_ob->*cb_fun)(id,reply);};
    void unhighlight();
    void activate() {active=true;};
    void deactivate() {active=false;};
	bool is_active() {return active;};
    void shift(double dx,double dy);
    COL hue_to_col (double hue);
    double col_to_hue(COL c);
    void set_params_from_col();
    void send_colour();
    button_type get_k() {return k;};
    FONT * get_font() {return font;};
    string get_title() {return title;};
};

template<class T>
class Text_input : public Area
{
public:
	enum status { unknown, valid, invalid };
	struct graphics
	{
		double margin;
		double corner_r;
		double box_margin;
		double box_corner_r;
		double space;
		double font_size;
		COL backgr_c;
		COL p_backgr_c;
		COL margin_c;
		COL p_margin_c;
		COL box_backgr_c;
		COL title_c;
		COL p_title_c;
		COL inp_c;
		COL bar_c;
	};
private:
	status stat = unknown;
	string valid_content;
	COL valid_c = al_map_rgb(200, 255, 200);
	COL invalid_c = al_map_rgb(255, 200, 200);

	typedef void (T::*cb_fun_ptr1)(int n, string &s);
	typedef void (T::*cb_fun_ptr2)(int n, vector<string> &s, int select, bool &new_text);
	cb_fun_ptr1 cb_fun1 = NULL;
	cb_fun_ptr2 cb_fun2 = NULL;
	T * cb_ob;
	Text_view<Text_input> * t_view = NULL;
	int list_lock = -1;
	bool unlock_next_round = false;

	bool switchable; // wheter the text input is always acitve or can be switched off
	bool persistent; // wheter text stays after hitting enter
	bool with_list = false;
	bool draw_list = false;
	string content;
	string title;
	int id;

	graphics g;
	double bar_w_h_rat = 0.12;
	double list_space;
	string font_file_name;
	FONT * font = NULL;

	double box_x0, box_y0, box_x1, box_y1;
    double text_view_w, text_max_h, input_w;

	bool highlight = false;
	bool initialized = false;
	bool signal_when_destroyed = false;
public:
	Text_input(double x_0, double y_0, double x_1, string font_file_name, text_inp_template<T> t, graphics g, bool switchable, bool persistent);
	~Text_input()
	{
		if (signal_when_destroyed)
		{
			bool b; vector<string> vs;
			string s = "\n";
			if (with_list) (cb_ob->*cb_fun2)(id, vs, -3, b);
			else (cb_ob->*cb_fun1)(id, s);
		}
		attempt_destroy_font(font_file_name,g.font_size,&font);
		if (t_view!=NULL) delete t_view; 
	};
	response_type respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt, double mouse_x, double mouse_y, double mouse_dz, int &lock, event_type &hold_event, int in, pair<char, char> input);
	void Resize(double x_0, double y_0, double x_1, double y_1);
	double get_min_x_dim() { return x1 - x0; };
	double get_min_y_dim() { return y1 - y0; };
	void draw(ALLEGRO_DISPLAY * disp);
    void set_scaling(double factor);
    void set_just_scaling(double factor) {scaling = factor;};
	void high() { highlight = true; };
	void unhigh() { highlight = false; };
	void shift(double dx, double dy);
	void clear_input() { content.clear(); };
	void cleanup() { clear_input(); initialized = false; stat = unknown; valid_content.clear(); if (with_list) t_view->clean(); };
	void initialize();
	bool apply();
	void validate();
	void respond_to_list(int n, vector<string> &s, int select, bool &new_text);
	void sign_upon_destr(bool d) { signal_when_destroyed = d; };
    string get_text() {return content;};
};

template<class T>
class Input_list : public Area
{
private:
	string font_file_name;
	vector<text_inp_template<T>> t;
	typename Text_input<T>::graphics g;

	int input_lock = -1;
	vector<Text_input<T> *> inputs;
	vector<double> widths;
	Button<Input_list> * add;
	Button<Input_list> * remove;
public:
	Input_list(double x_0, double y_0, vector<double> widths, string font_file_name, vector<text_inp_template<T>> t, typename Text_input<T>::graphics g, string title);
	~Input_list() { if (!inputs.empty()) for (int i = ((int)inputs.size() - 1); i >= 0; i--) delete inputs[i]; delete add; delete remove; };
	void button_response(int n, string &s);
	response_type respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt, double mouse_x, double mouse_y, double mouse_dz, int &lock, event_type &hold_event, int in, pair<char, char> input);
	void Resize(double x_0, double y_0, double x_1, double y_1) {};
	double get_min_x_dim() { return x1 - x0; };
	double get_min_y_dim() { return y1 - y0; };
	void cleanup() { set_length(0); };
	bool apply() { bool ret = true;  for (const auto &i : inputs) if (i->apply() == false) ret = false; return ret; };
	void initialize() { for (const auto &i : inputs) i->initialize(); }
	void shift(double dx, double dy);
	void set_length(int len);
	void draw(ALLEGRO_DISPLAY * disp);
    void set_scaling(double factor);
};

template <class T>
class Input_view : public Area
{
private:
	int input_lock = -1;
	double tot_h;
	double v_margin = 0;
	double h_margin = 0;
	double scroll_w = 32;
	double scroll_speed = 20;
	double content_space = 10;

	/// Specs for principal buttons and text inputs
	string title;
	FONT * title_font;
	double title_font_height = 20;
	double title_line_spacing = 1.5;
	COL title_text_c = al_map_rgb(255, 255, 255);
	COL title_background_c = al_map_rgb(0, 127, 0);


	double font_size = 20;
	string font_file_name = "arial.ttf";
	double margin = 3.0;
	double space = 10.0;
	double corner_r = 10.0;
	double text_inp_width = 300;
	COL button_c = al_map_rgb(127, 127, 127);
	COL p_button_c = al_map_rgb(100, 100, 100);
	COL margin_c = al_map_rgb(160, 160, 160);
	COL p_margin_c = al_map_rgb(160, 160, 160);
	COL font_c = al_map_rgb(220, 220, 220);
	COL p_font_c = al_map_rgb(255, 255, 255);

	COL inp_box_backgr_c = al_map_rgb(255, 255, 255);
	COL inp_font_c = al_map_rgb(0, 0, 0);
	COL inp_bar_c = al_map_rgb(15, 15, 15);

	Scroll * s = NULL;
	vector<Button<T>* > buttons;
	vector<Text_input<T> *> text_inputs;
	vector<Input_list<T> *> input_lists;
	vector<vector<pair<char, int>>> order;

public:
	Input_view(double x0, double y0, double x1, double y1, bool on, string title, inp_view_template<T> t);
	~Input_view() { for (auto& i : buttons) delete i; for (auto& i : text_inputs) delete i; for(auto &i: input_lists) delete i; delete s; attempt_destroy_font(font_file_name,title_font_height,&title_font); };
	void Resize(double x_0, double y_0, double x_1, double y_1);
	response_type respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt, double mouse_x, double mouse_y, double mouse_dz, int &lock, event_type &hold_event, int in, pair<char, char> input);
	double get_min_x_dim() { return s->get_min_x_dim() * 2;  };
	double get_min_y_dim() { return s->get_min_y_dim();  };
	bool apply();
	void cleanup();
	void move_into_position();
	void initialize(vector<int> sizes = {}) 
	{ 
		for (int i = 0; i < (int)sizes.size(); i++)
		{
			double orig_dimy = input_lists[i]->get_y1() - input_lists[i]->get_y0();
			set_list_length(i, sizes[i]);
			adjust_y(orig_dimy, input_lists[i]);
		}
		for (const auto &i : text_inputs) i->initialize(); 
		for (const auto &i : input_lists) i->initialize(); 
	}
	void adjust_y(double orig_dimy, Input_list<T> *i);
	void set_list_length(int in, int len) { if (in < input_lists.size()) input_lists[in]->set_length(len); else printf("\nError: Trying to access non-existing list!\n\n"); };
	void draw(ALLEGRO_DISPLAY * disp);
    void set_scaling(double factor);
};

template<class T>
class Menu: public Area
{
private:
    vector<Button<T>* > buttons;

    double margin = 0.0;
    double space = 10.0;
    double font_size = 20;
    double corner_r = 0;
    string font_file_name = "arial.ttf";
    COL button_c = al_map_rgb(0,127,0);
    COL p_button_c = al_map_rgb(0,100,0);
    COL margin_c = al_map_rgb(80,127,80);
    COL p_margin_c = al_map_rgb(13,127,13);
    COL font_c = al_map_rgb(220,220,220);
    COL p_font_c = al_map_rgb(255,255,255);
    COL tick_c = al_map_rgb(200,200,200);

    int button_lock = -1; // index of the locked button

public:
    Menu(vector<button_template<T> > b_temps,double x_0, double y_0);
    response_type respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in,pair<char,char> input);
    void Resize (double x_0, double y_0, double x_1, double y_1) {};
    double get_min_x_dim() {return x1-x0;};
    double get_min_y_dim() {return y1-y0;};
    void draw(ALLEGRO_DISPLAY * disp);
    void set_scaling(double factor);
    void unhighlight();
    void shift(double dx,double dy);
};

template<class T>
class Panel: public Area
{
private:
    COL panel_margin_c = al_map_rgb(0,100,0);
    COL background_c = al_map_rgb(0,127,0);
    double v_margin = 4;
    double h_margin = 4;

    /// Specs for principal buttons and text inputs
    double font_size = 20;
    string font_file_name = "arial.ttf";
    double margin = 3.0;
    double space = 10.0;
    double corner_r = 10.0;
    double text_inp_width = 150;
    COL button_c = al_map_rgb(127,127,127);
    COL p_button_c = al_map_rgb(100,100,100);
    COL margin_c = al_map_rgb(160,160,160);
    COL p_margin_c = al_map_rgb(160,160,160);
    COL font_c = al_map_rgb(220,220,220);
    COL p_font_c = al_map_rgb(255,255,255);

    COL inp_box_backgr_c = al_map_rgb(255,255,255);
    COL inp_font_c = al_map_rgb(0,0,0);
    COL inp_bar_c = al_map_rgb(15,15,15);

    /// Buttons
    vector<Button<T>* > buttons;
    int prev_enabled_button = -1;
    /// Text inputs
    vector<Text_input<T> *> text_inputs;

    bool buttons_active = false;
    bool text_inputs_active  = false;
    int text_inp_lock = -1;
public:
    Panel(double x0, double y0, double x1, bool on, vector<button_template<T> > but, vector<text_inp_template<T> > tex);
    ~Panel() {for(auto& i:buttons) delete i; for(auto& i:text_inputs) delete i;};
    void Resize (double x_0, double y_0, double x_1, double y_1);
    response_type respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in,pair<char,char> input);
    double get_min_x_dim();
    double get_min_y_dim();
    void draw(ALLEGRO_DISPLAY * disp);
    void set_scaling(double factor);
};

class Resize_bar: public Area
{
private:
    vector<Area *> areas;
    vector<int> dims;
    bool ver_or_hor;
    double width = 8.0;
public:
    Resize_bar(vector<Area *> areas,vector<int> dims);
    void Resize(double x_0,double y_0,double x_1,double y_1) {x0=x_0;y0=y_0;x1=x_1;y1=y_1;};
    void draw(ALLEGRO_DISPLAY * disp) {};
    void set_scaling(double factor);
    response_type respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in,pair<char,char> input);
    double get_min_x_dim() {return width*scaling;};
    double get_min_y_dim() {return width*scaling;};
    void fit();
};

class Custom: public Area
{
private:
    COL margin_c = al_map_rgb(0,100,0);
    double h_margin = 4;
    double v_margin = 0;
public:
    Custom(double x0, double y0, double x1, double y1, bool on);
    ~Custom() {};
    void Resize (double x_0, double y_0, double x_1, double y_1);
    void get_margins(double &h_mar,double &v_mar) {h_mar=h_margin; v_mar=v_margin;};
    response_type respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in,pair<char,char> input);
    double get_min_x_dim() {return 4*h_margin*scaling;};
    double get_min_y_dim() {return 4*v_margin*scaling;};
    void draw(ALLEGRO_DISPLAY * disp);
    void set_scaling(double factor);
};

template<class T>
class Window
{
public:
	enum side_pane {neighbour_info,edit_spec, edit_reac, edit_compart, add_spec, add_reac, add_compart};
private:
    double scaling = 1.0;

	side_pane mode = neighbour_info;
    ALLEGRO_DISPLAY * disp = NULL;
    COL margin_c = al_map_rgb(0,100,0);
    double rat_x = 0.7;
    double panel_h;
    double size_x;
    double size_y;
    double l_margin = 4;
    vector<Area *> areas;

    Text_view<T> * t_view = NULL;
	Input_view<T> * espec_view = NULL;
	Input_view<T> * ereac_view = NULL;
	Input_view<T> * ecompart_view = NULL;
	Input_view<T> * aspec_view = NULL;
	Input_view<T> * areac_view = NULL;
	Input_view<T> * acompart_view = NULL;

    Custom * custom = NULL;
    Panel<T> * panel = NULL;
    Resize_bar * v_res = NULL;
    Resize_bar * h_res = NULL;
    int lock = -1;
    event_type hold_event = event_type::idle;
public:
    Window(ALLEGRO_DISPLAY * disp,vector<button_template<T> > buttons,vector<text_inp_template<T> > text_inputs, vector<string> ititles, vector<inp_view_template<T>> itemplates);
    ~Window() {for(auto i:areas) delete i;};
    void Resize(ALLEGRO_DISPLAY* disp);
    void Change_text(vector<string> &str) {t_view->Change_text(str);};
    void Get_cust_coors(double &x0,double &y0, double &x1, double &y1) {x0=custom->get_x0();y0=custom->get_y0();x1=custom->get_x1();y1=custom->get_y1();};
    void Get_cust_margins(double &h_mar,double &v_mar) {custom->get_margins(h_mar,v_mar);};
    void Get_cust_prev_mouse_x(double &m_x,double &m_y) {m_x=custom->get_prev_mouse_x();m_y=custom->get_prev_mouse_y();};
	void set_mode(side_pane s, vector<int> sizes = {});
	side_pane get_mode() { return mode; };
	bool apply_current_mode();
	void clear_current_mode();
    response_type respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,pair<char,char> input);
    void draw();
    void set_scaling(double factor);
};

class Warning
{
private:
    ALLEGRO_DISPLAY * disp = NULL;
    double FPS = 60;

    string font_file_name = "arial.ttf";
    double font_size = 25;
    double but_font_size = 20;
    double line_spacing = 1.2;

    double but_margin = 3;
    double but_space = 10;
    double but_corner_r = 10;
    double text_v_space = 3;
    double text_h_space = 3;

    /// button colours
    COL button_c = al_map_rgb(127,127,127);
    COL p_button_c = al_map_rgb(100,100,100);
    COL margin_c = al_map_rgb(160,160,160);
    COL p_margin_c = al_map_rgb(160,160,160);
    COL font_c = al_map_rgb(220,220,220);
    COL p_font_c = al_map_rgb(255,255,255);
    /// warning colours
    COL warn_c = al_map_rgb(0,0,0);

    vector<Button<Warning> *> buttons;

    int lock = -1;
    event_type hold_event = event_type::idle;
    int * action;

    response_type respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz, pair<char,char> input);
    void do_action(int n, string &s);
    void draw();

public:
    Warning(ALLEGRO_EVENT_QUEUE * event_queue, string message, vector<string> b_text, int * action, double scaling);
    ~Warning() {for(auto i:buttons) delete i;};
};

class Filehandler
{
public:
    enum mode {open,save_as};
    enum action {none,cancel,do_it};
private:
    double scaling = 1.0;
    mode mod;
    action * act = NULL;
    string * file_name = NULL;

    ALLEGRO_DISPLAY * disp = NULL;
    double rat_y_input = 1.0;
    double rat_y_text = 2.0;
    double rat_y_buttons = 1.0;

    string font_file_name = "arial.ttf";
    double font_size = 20;
    double but_margin = 3;
    double but_space = 10;
    double but_corner_r = 10;
    double inp_margin = 3;
    double inp_space = 10;
    double inp_corner_r = 10;
    double text_v_margin = 3;
    double text_h_margin = 3;

    double button_y0;
	double text_y0, d_width, text_y1;

	string current_path;
	vector<string> files;
	vector<string> folders;
	vector<bool> relevant;
	vector<string> full_list;

    COL button_c = al_map_rgb(127,127,127);
    COL p_button_c = al_map_rgb(100,100,100);
    COL margin_c = al_map_rgb(160,160,160);
    COL p_margin_c = al_map_rgb(160,160,160);
    COL p_font_c = al_map_rgb(255,255,255);
    COL font_c = al_map_rgb(220,220,220);

    COL inp_font_c = al_map_rgb(0,0,0);
    COL inp_bar_c = al_map_rgb(15,15,15);
    COL inp_box_backgr_c = al_map_rgb(255,255,255);

    COL folder_c = al_map_rgb(200,0,0);
    COL file_c = al_map_rgb(0,0,0);
    COL rel_file_c = al_map_rgb(0,200,0);

    vector<Area *> areas;
    Text_view<Filehandler> * t_view = NULL;
    Button<Filehandler> * canc = NULL;
    Button<Filehandler> * doit = NULL;
    Text_input<Filehandler> * input = NULL;

    int lock = -1;
    event_type hold_event = event_type::idle;
public:
    Filehandler(ALLEGRO_DISPLAY * disp,mode mod,action * act,string * file_name, double scaling);
    ~Filehandler() {for(auto i:areas) delete i;};
	void change_folder(string folder_name);
	void click_folder(int n, vector<string> &s, int select, bool &new_text);
    void Change_text(vector<string> &str) {t_view->Change_text(str);};
    void do_file_action(int n, string &s);
    void receive_text(int n, string &s);
	string get_current_path() { return current_path; }
    int get_lock() {return lock;};
	void clear_input() { if (input != NULL) input->clear_input(); }
    response_type respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz, pair<char,char> input);
    void draw();
};

class Loading 
{
    class mover 
    {
        COL mov_c = al_map_rgb(75,75,75);
        double r = 0.12; // Mover head radius as fraction of circle radius
        double l = 5.0; // Mover length as fraction of its head diameter
        double speed = 2.3; // Mover speed in lengths per second
        int num_segments = 100; // Number of segments in the mover
        double std_dev = 0.1; // Standard deviation of the mover direction change choice

        int movs_per_step; // Mover movements per step 
        double segment_l; // length of each segment
        double R; // actual mover head radius
        vector<vector<double> > snake; // Positions of the segments of the mover

        double FPS;
        double pos_x, pos_y, circle_r; // position and radius of containing circle

        default_random_engine generator;

        public:
        mover(double pos_x,double pos_y,double circle_r,double FPS);
        void step_and_draw();
    };

    COL circle_c = al_map_rgba(0,0,255,60);
    COL circle_b_c = al_map_rgba(0,0,0,100);
    COL text_c = al_map_rgb(255,255,255);
    double border_t = 0.05; // border thickness as a fraction of radius
    double FPS = 50.0;
    string font_file_name = "arial.ttf";
    double font_size_frac = 0.3; // fraction of font size to cicle radius

    public:
    template<class R, class T, class... Args>
    Loading(ALLEGRO_DISPLAY * disp, ALLEGRO_BITMAP * disp_map, double circle_r, double pos_x, double pos_y, double scaling, T * cb_ob, R (T::*cb_fun_ptr)(Args...), Args... args);
};

class Explorer
{
public:
    ALLEGRO_DISPLAY * display = NULL;
    BITMAP * disp_map = NULL;
private:
    double scaling = 1.0;
    double p_scaling = 1.0;

    string VERSION = "ModelExplorer 2.2";
    string FILE_NAME;
    COL graph_bckgr_c = al_map_rgb(255,255,255);

    ALLEGRO_EVENT_QUEUE *event_queue = NULL;
    ALLEGRO_TIMER *timer = NULL;
    BITMAP * canvas = NULL;
    BITMAP * fastg_sprite = NULL;
    ALLEGRO_DISPLAY_MODE dispMod;
    ALLEGRO_MONITOR_INFO info;
    double FPS = 60;
    int graphics_mode = 1; // Graphics mode ( 1 = HIGH RES, 0 = LOW RES )
    double q_coeff = 0.5;
	
	bool print_times = false;
	bool print_fps = false;

    Window<Explorer> * win = NULL;
    double res_x;
    double res_y;

	int sumreac = 0;
	int sumspec = 0;
	int sumtot = 0;

    FBA_model * model = NULL;
    set<string> flags;              /// User defined flags when (-ps == purge disconnected species)
    vector<bool> uninitiated;       /// species that do not have any input reactions but have outputs
    vector<bool> spec_void;         /// species that are not in any reactions
    Array<double> coors;            /// coordinates of nodes
    vector<string> index_to_id;     /// get species/reaction name from its index in the coordinates array above
	vector<string> index_to_id_full;/// get species/reaction name from its index in the coordinates array above (no masks)
    map<string,int> r_id_to_index;  /// inverse of index_to_id but only for the reactions
    map<string,int> s_id_to_index;  /// inverse of index_to_id but only for the species
    Array<int> links;               /// links between nodes

	int tree_type = 0;				/// Tree type. 0 - uptake, 1 - excretion (switched using tab)
    Array<ancestor> frozen_tree;    /// The tree structure in ancestral searching
    int frozen_tree_centre = -1;


	Array<int> parents;             /// parents of each species/reaction
	Array<int> children;			/// children of each species/reaction
	Array<int> neighbours;			/// Nearest neighbours of each node
    Array<int> reactants;           /// Reactants of each reaction
    Array<int> products;            /// Products of each reaction

    vector<int> reac_order;         /// The distanace of reaction from inflow
    vector<int> spec_order;         /// The distance of species from inflow
    vector<int> reac_order_out;     /// The distanace of reaction from outflow
    vector<int> spec_order_out;     /// The distance of species from outflow

    vector<bool> reversible;        /// Reaction reversible or not?
    map<string,vector<Point> > cmprts; /// Compartments for drawing
	vector<int> subgraphs;			/// Reactions and species divided into subgraphs (number indicates subgraph index)
	int num_subgraphs;
	set<int> frozen_bmodule;            /// Currently frozen blocked submodule
	int frozen_bmodule_centre = -1;

    vector<bool> FBA_dead;          /// Dead or living reaction (FBA)
    vector<bool> FBA_dead_specs;
    vector<bool> BIDIR_dead;        /// Dead or living reaction (bidir)
    vector<bool> BIDIR_dead_specs;
    vector<bool> SINK_dead;         /// Dead or living reaction (sink)
    vector<bool> SINK_dead_specs;
    vector<bool> dead;               /// Reactions and species which cannot be directly derived from inflows
	vector<bool> dead_out;		     /// Reactions and species which cannot be directly secreted through outflows
    vector<bool> dead_specs;         /// Dead or living SPECIES (the above three arrays are copied into it for use)
    vector<bool> dead_reacs;         /// Dead or living REACTIONS (the above three arrays are copied into it for use)
	vector<bool> dead_r_and_s;

    vector<int> objectives;

    Array<int> o_products_of;        /// Reactions that only produce each species
    Array<int> o_reactants_of;       /// Reactions that only consume each species
    Array<int> p_and_r_of;           /// Reactions that both consume and produce each species

	vector<string> search_list;      /// Temporary list that shows alternatives during search
	vector<string> search_comparts;   /// Temporary compartment search list
	vector<string> search_specs;		 /// Temporary speices search list

	vector<pair<string, double>> temp_reactants;
	vector<pair<string, double>> temp_products;
	vector<string> temp_genes;

	// Error - finding structures ////////

	map<int,int> LMtype; // the type of lm 0 - si, 1 - so, 2 - siso
	map<int,pair<set<int>,set<int>>> LMs; // LMs with their own indexing, and reac and spec ids 
	map<int,pair<set<int>,set<int>>> BMs; // Same as above but for BMs (indices are negative)

	int lm_count = 0;
	int bm_count = 0;

 	// specs with first - associated errors by querying reaction - first - BM, second - LM
	map<int,pair<map<int,pair<int,int>>,int>> erspecs; // second - type of error 0 - rev, 1 - sou, 2 - sto. If LM-based - 3
	map<int,map<int,int>> reacs_to_erspecs; // links from blocked reactions to their respective error specs 
 
	int ereac_frozen = -1;
	set<int> espec_group;
	int espec_frozen = -1;

	int arrays_upd_after_ccheck = 0;

	double ers_crosshand_l = 12; 
	double ers_crosshand_w = 9;
	double ers_cross_border_w = 3.0;
	double BM_line_w = 3;

	COL error_cross_c = al_map_rgb(222,138,1);
	COL error_border_c = al_map_rgb(0,0,0);
	COL BM_line_c = al_map_rgba(0,0,0,180);
	COL LM_line_c = al_map_rgba(0,200,0,210);

	COL LM_reac_c = al_map_rgb(31,117,254);
	COL LM_spec_c = al_map_rgb(15,58,127);
	COL LM_arrow_c = al_map_rgba(7,29,64,15);
	COL LM_arrow_svg_c = al_map_rgba(142,186,254,80);

	// ///////////////////////////////////

    double scr_c_x = 0.0, scr_c_y = 0.0, scr_w = 1.0, scr_c_x_save, scr_c_y_save; /// Screen center and width in the layout coordinates
    double gr_sprite_x1, gr_sprite_y1, gr_sprite_x2, gr_sprite_y2, gr_sprite_pixel_w; /// The coordinates of the corners of the fast graphics sprite
    bool scr_upd;
    bool lmb_move = false;
    bool quit = false;
    int search_target = -1;
    int freeze = -1;
    int prev_hit = -1;

    double node_r = 4;
    double uninit_node_r = 5.5;
    double obj_node_r = 7;
    double high_node_r = 6;
    double search_node_r = 7;
    double search_node_th = 2;
    double line_w = 2;
    double arrow_w = 3;
    double arrow_l = 11;
    COL arrow_c = al_map_rgba(0,0,0,50);
    COL obj_reac_c = al_map_rgb(255,255,0);
    COL reac_on_c = al_map_rgb(0,255,0);
    COL reac_off_c = al_map_rgb(255,0,0);
    COL spec_on_c = al_map_rgb(0,128,0);
    COL spec_off_c = al_map_rgb(128,0,0);
    COL uninit_c = al_map_rgb(0,221,255);
    COL void_c = al_map_rgb(0,0,0);
    COL search_c = al_map_rgb(255,0,255);

    COL compart_line_c = al_map_rgb(255,140,0);
    double compart_line_t = 4.0;
	double compart_line_on_t = 7.0;
    double compart_r = 5.0;
    bool draw_comparts = true;

    double bold_line_w = 3;
    COL reactant_line_c = al_map_rgba(255,0,0,180);
    COL product_line_c = al_map_rgba(0,0,255,180);
    COL reversbl_line_c = al_map_rgba(255,0,255,180);

    COL bold_line_c = al_map_rgba(0,0,0,180);
    COL liv_bold_line_c = al_map_rgba(75,0,130,180);

    string font_file_name = "arial.ttf";
    double specinfo_font_size = 22;
    FONT * specinfo_font = NULL;
    COL specinfo_c = al_map_rgb(255,0,255);

    enum tracking_mode {non_curious,neighbour,ancestral,blocked_module,error_source};
    tracking_mode T_MODE = non_curious;
    enum blocking_mode {none,FBA,bidir,sink};
    blocking_mode B_MODE = none;

    enum cursor_type {normal,ver_resize,hor_resize};
    cursor_type cursor = normal;
    double curs_bar_l = 20;
    double curs_bar_w = 10;
    double curs_arr_l = 10;
    double curs_arr_w = 20;
    double curs_r = 17;
    double curs_t = 3;
    COL resize_curs_c = al_map_rgb(200,200,200);
    COL norm_cur_c = al_map_rgb(255,0,0);

    double mouse_x = -100;    /// Mouse coordinates
    double mouse_y = -100;
    double mouse_dz = 0;
    double zoom_per_scroll = 0.9; /// Zoom per scroll click

	/// Temporary edited compartment, species, reaction

	bool editing_entity = false; // tells if the current item has been edited, restricting mode switching
	int edited_item = -1;
	int current_aoe_action = -1;
	string temp_id;
	specium temp_spec;
	reaction temp_reac;
	compartment temp_compart;

	/// Currently visualized subnetwork
	bool show_subnetwork = false;

    int react_to_response(response_type res);
    int load_FBA_model(string filename);
    int save_FBA_model(string filename);
	int save_SVG(string path_to_file);
    int clear_FBA_model();
    int make_layout();
	void update_compartments();
    int draw_to_screen(double x_0, double y_0, double x_1, double y_1);
	SVG draw_to_SVG(double x_res, double y_res);
    bool hit_node(int &hit);
    void draw_spec_info(int hit);
    void draw_comp_info(const vector<string> &hits);
    void draw_spec_relatives(int hit, bool draw_legend);
    void update_arrays();
    void update_blocked(bool full);
    void draw_information(bool got_any_cust_move_mouse);
    void file_action_dialogue(int n, string &s);
	void results_to_XML(string fpath);
    void edit(int n, string &s);
	void view(int n, string &s);
    void purge(int n, string &s);
	void purge_selection();
	void purge_selection_weak();
    void tracking(int n, string &s);
    void blocking(int n, string &s);
    void graphics(int n, string &s);
    void menu_scaling(int n, string &s);
    void plot_scaling(int n, string &s);
    void scaleMenus(double factor);
    void scalePlot(double factor);
    void compartments(int n, string &s);
    void palettes(int n, string &s);
    void button_dummy(int n, string &s) {};
    void search_for_word(int n, vector<string> &s, int select, bool &new_text);
    vector<bool> get_dead_specs(const vector<bool> &d_reacs,const Array<int> &reactants,const Array<int> &products);
    vector<bool> get_dead_reacs(const vector<bool> &d_specs,const Array<int> &reactants,const Array<int> &products);
	void find_subgraph(const int &n, const int &curr_group,const vector<vector<int> > &adjacency,vector<int> &subgraphs);
	
	template<typename T>
	void draw_arrow(T &n, COL ar_c, int g_mode, double x_0,double y_0,double x_1,double y_1, double bound_x0, double bound_y0, double bound_x1, double bound_y1, double rat, double scr_x0, double scr_y0, int beg, int end);

	/// functions pertaining to model building
	void add_or_edit(int n, string &s);

	void edit_species (int n, string &s);
	void edit_reaction(int n, string &s);
	void find_reactant(int n, vector<string> &s, int select, bool &new_text);
	void edit_reactant_st(int n, string &s);
	void find_product(int n, vector<string> &s, int select, bool &new_text);
	void edit_product_st(int n, string &s);
	void find_compart(int n, vector<string> &s, int select, bool &new_text);
	void find_outside(int n, vector<string> &s, int select, bool &new_text);
	void edit_gene(int n, string &s);
	void edit_compart(int n, string &s);
	void edit_bound_cond(int n, vector<string> &s, int select, bool &new_text);
	void edit_reversibility(int n, vector<string> &s, int select, bool &new_text);
	void spec_reac_manipulate(int n, string &s);

public:
    Explorer(string filename,const set<string> &flags, double res_X,double res_Y);
    Explorer() : Explorer("",set<string>(),0.9,0.9) {};
    ~Explorer();
    int run();
};

#endif // EXPLORER_H_INCLUDED
