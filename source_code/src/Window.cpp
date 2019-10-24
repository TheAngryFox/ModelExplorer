#include <Explorer.h>

template class Window<Explorer>;

map<pair<int,string>, pair<FONT*,set<FONT**>>> font_storage; // base font size, font file name, font pointer, number of users

FONT* load_ttf_font(const string &font_path, int font_size, FONT** back_pointer)
{
    auto foundit = font_storage.find(make_pair(font_size,font_path));
    if(foundit!=font_storage.end()) // if already loaded
    {
        foundit->second.second.insert(back_pointer); // add to the number of users
        return foundit->second.first;
    }
    else
    {
        FONT * tempf = al_load_ttf_font(font_path.c_str(),font_size,0);
        font_storage[make_pair(font_size,font_path)] = make_pair(tempf,set<FONT**>{back_pointer});
        return tempf;
    }
};
void attempt_destroy_font(const string &font_path, int font_size, FONT** back_pointer)
{
    auto foundit = font_storage.find(make_pair(font_size,font_path));
    if(foundit!=font_storage.end()) // if loaded
    {
        (*foundit).second.second.erase(back_pointer);
        if((*foundit).second.second.size()==0) // if the current deleter is the last user
        {
            if((*foundit).second.first!=NULL) al_destroy_font((*foundit).second.first);
            font_storage.erase(foundit);
        }
    }
};
void clear_font_storage()
{
    for(const auto &i:font_storage) al_destroy_font(i.second.first);
    font_storage.clear();
};
void set_font_scaling (double factor)
{
    for(auto &i:font_storage)
    {
        if(i.second.first!=NULL) al_destroy_font(i.second.first);
        i.second.first = al_load_ttf_font(i.first.second.c_str(),(int)(factor*i.first.first),0);
        for(auto &j:i.second.second) *j = i.second.first;
    }
}



Scroll::Scroll(double x0, double y0, double x1, double y1, bool on, double frac) : Area(x0,y0,x1,y1,on)
{
    fraction = frac;
    bar_height = max(fraction * (y1-y0-2*v_margin),x1-x0-2*h_margin);
    bar_pos = y0+v_margin;
}

void Scroll::Resize(double x_0, double y_0, double x_1, double y_1)
{
    double o_bar_height = bar_height;
    double o_y0 = y0;
    double o_y1 = y1;
    x0=x_0;
    y0=y_0;
    x1=x_1;
    y1=y_1;
    bar_height = max(fraction * (y1-y0-2*v_margin*scaling),x1-x0-2*h_margin*scaling);
    if((o_y1-o_y0-2*v_margin*scaling-o_bar_height)>0) bar_pos = y0+v_margin*scaling+(bar_pos-o_y0-v_margin*scaling)/(o_y1-o_y0-2*v_margin*scaling-o_bar_height)*(y1-y0-2*v_margin*scaling-bar_height);
    else bar_pos = y0 + v_margin*scaling;
}

void Scroll::change_scroll(double y_0, double y_1, double frac)
{
    fraction = frac;
    double o_bar_height = bar_height;
    double o_y0 = y0;
    double o_y1 = y1;
    y0=y_0;
    y1=y_1;
    bar_height = max(fraction * (y1-y0-2*v_margin*scaling),x1-x0-2*h_margin*scaling);
    if((o_y1-o_y0-2*v_margin*scaling-o_bar_height)!=0) bar_pos = y0+v_margin*scaling+(bar_pos-o_y0-v_margin*scaling)/(o_y1-o_y0-2*v_margin*scaling-o_bar_height)*(y1-y0-2*v_margin*scaling-bar_height);
}

void Scroll::set_barpos_by_rat(double new_rat)
{
	bar_pos = y0 + v_margin * scaling + new_rat*(y1 - y0 - 2 * v_margin * scaling - bar_height);
}

response_type Scroll::respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in, pair<char, char> input)
{
    return response_type::do_nothing;
}

double Scroll::where_is_bar()
{
    if(fraction==1) return 0;
    return (bar_pos-v_margin*scaling-y0)/(y1-y0-2*v_margin*scaling-bar_height);
}

double Scroll::mov(double d_pos)
{
    double new_pos = bar_pos+d_pos;
    double min_pos = y0+v_margin*scaling;
    double max_pos = y1-v_margin*scaling-bar_height;
    if(new_pos>max_pos) bar_pos = max_pos;
    else if (new_pos<min_pos) bar_pos = min_pos;
    else bar_pos = new_pos;
    return (bar_pos-v_margin*scaling-y0)/(y1-y0-2*v_margin*scaling-bar_height);
}

void Scroll::reset_bar()
{
	bar_pos = y0 + v_margin*scaling;
}

void Scroll::draw(ALLEGRO_DISPLAY * disp)
{
    al_set_target_backbuffer(disp);
    double bar_width = (x1-x0)-2*h_margin*scaling;
    al_draw_filled_rectangle(x0,y0,x1,y1,margin_c);
    al_draw_filled_rounded_rectangle(x0+h_margin*scaling,y0+v_margin*scaling,x1-h_margin*scaling,y1-v_margin*scaling,bar_width*0.5,bar_width*0.5,whitespace_c);
    al_draw_filled_rounded_rectangle(x0+h_margin*scaling,bar_pos,x1-h_margin*scaling,bar_pos+bar_height,bar_width*0.5,bar_width*0.5,(bar_mov) ? mov_bar_c : bar_c);
}

void Scroll::set_scaling(double factor)
{
    scaling = factor;
}

int Scroll::where_is_mouse(double m_x,double m_y)
{
    if(m_x>(x0+h_margin*scaling) && m_y>bar_pos && m_x<(x1-h_margin*scaling) && m_y<(bar_pos+bar_height)) return 1;
    else return 0;
}

template<class T>
Text<T>::Text(double x0, double y0, double x1, double y1, bool on, vector<string> str, int font_size, int title_font_size) : Area(x0,y0,x1,y1,on), font_size(font_size), title_font_size(title_font_size)
{
    raw_str=str;
    font = load_ttf_font(font_file_name,font_size,&font);
    title_font = load_ttf_font(font_file_name,title_font_size,&title_font);
    h_space = 0.5*font_size; 
    separator_h = 0.1*font_size;
    title_h_space = 0.5*title_font_size;
    Change_text(str);
    full_text = al_create_bitmap(x1-x0,y1-y0-2*v_margin-title_height);
}

template<class T>
Text<T>::Text(double x0, double y0, double x1, double y1, bool on, vector<string> str, cb_fun_ptr cb_fun, T * cb_ob, int id, int font_size, int title_font_size) : Area(x0, y0, x1, y1, on), cb_fun(cb_fun), cb_ob(cb_ob), id(id), font_size(font_size), title_font_size(title_font_size)
{
    h_space = 0.5*font_size; 
    title_h_space = 0.5*title_font_size;
	selectable = true;
	separator_h = selectable_separator_h;
	separator_c = selectable_separator_c;
	raw_str = str;
	font = load_ttf_font(font_file_name, font_size, &font);
	title_font = load_ttf_font(font_file_name, title_font_size, &title_font);
	Change_text(str);
	full_text = al_create_bitmap(x1 - x0, y1 - y0 - 2 * v_margin - title_height);
}

template<class T>
void Text<T>::Resize(double x_0, double y_0, double x_1, double y_1)
{
    h_space = 0.5*font_size*scaling; 
    title_h_space = 0.5*title_font_size*scaling;
    separator_h = (separator_h==0.0) ? 0.0 : 0.1*font_size*scaling; 
    x0=x_0;
    y0=y_0;
    x1=x_1;
    y1=y_1;
    Change_text(raw_str);
    if(full_text!=NULL) al_destroy_bitmap(full_text);
    full_text = al_create_bitmap(x1-x0,y1-y0-2*v_margin*scaling-title_height);
	
}

template<class T>
void Text<T>::Change_text(vector<string> str)
{
	raw_str = str;
    title.clear();
	tab_line_links.clear();
    tex.clear();
    tex_c.clear();
    double width = x1-x0-2*h_space*scaling-2*h_margin*scaling;
    double title_width = x1-x0-2*title_h_space*scaling-2*h_margin*scaling;

    /// Get the title

    if(str.size()>0 && str[0].length()>2 && str[0][0]=='$' && str[0][1]=='t')
    {
        string title_temp = str[0].substr(2);
        str.erase(str.begin());

        double line_w = al_get_text_width(title_font,title_temp.c_str());
        double num_br = line_w/title_width;
        if(num_br<1) title.push_back(title_temp);
        else
        {
            int pos = 0;
            int guess = floor(title_temp.size()/num_br);
            while(pos<(int)title_temp.size())
            {
                string temp = title_temp.substr(pos,guess);
                int iter = 0;
                if(al_get_text_width(title_font,temp.c_str())<title_width)
                {
                    do
                    {
                        iter++;
                        temp = title_temp.substr(pos,guess+iter);
                    }while(al_get_text_width(title_font,temp.c_str())<title_width && (pos+guess+iter-1)<(int)title_temp.size());
                    title.push_back(title_temp.substr(pos,guess+iter-1));
                    pos += guess+iter-1;
                }
                else
                {
                    do
                    {
                        iter--;
                        temp = title_temp.substr(pos,guess+iter);
                    }while(al_get_text_width(title_font,temp.c_str())>title_width);
                    title.push_back(title_temp.substr(pos,guess+iter));
                    pos += guess+iter;
                }
            }
        }
    }

    /// Split the lines when needed
	int beg_tab_link = 0;
	int tab_count = 0;
    for(string i:str)
    {
        COL line_col = def_text_c;
        if(i.size()>1 && i[0]=='$')
        {
            switch (i[1])
            {
            case 'r':
                i = i.substr(2);
                line_col=al_map_rgb(255,0,0);
                break;
            case 'g':
                i = i.substr(2);
                line_col=al_map_rgb(0,255,0);
                break;
            case 'b':
                i = i.substr(2);
                line_col=al_map_rgb(0,0,255);
                break;
            case '{':
                i = i.substr(2);
                stringstream ss(i);
                int j;
                vector<int> col;
                int end_found = -1;
                while(ss >> j)
                {
                    col.push_back(j);
                    if(ss.peek()==';') ss.ignore();
                    if(col.size()>3) break;
                    if(ss.peek()=='}') {end_found = ss.tellg(); break;}
                }
                if(col.size()==3 && end_found!=-1)
                {
                    line_col=al_map_rgb(col[0],col[1],col[2]);
                    i=i.substr(end_found+1);
                }
                break;
            }
        }

        double line_w = al_get_text_width(font,i.c_str());
        double num_br = line_w/width;
        if(num_br<1) {tex.push_back(i);tex_c.push_back(line_col);}
        else
        {
            int pos = 0;
            int guess = floor(i.size()/num_br);
            while(pos<(int)i.size())
            {
                string temp = i.substr(pos,guess);
                int iter = 0;
                if(al_get_text_width(font,temp.c_str())<width)
                {
                    do
                    {
                        iter++;
                        temp = i.substr(pos,guess+iter);
                    }while(al_get_text_width(font,temp.c_str())<width && (pos+guess+iter-1)<(int)i.size());
                    tex.push_back(i.substr(pos,guess+iter-1));
                    tex_c.push_back(line_col);
                    pos += guess+iter-1;
                }
                else
                {
                    do
                    {
                        iter--;
                        temp = i.substr(pos,guess+iter);
                    }while(al_get_text_width(font,temp.c_str())>width);
                    tex.push_back(i.substr(pos,guess+iter));
                    tex_c.push_back(line_col);
                    pos += guess+iter;
                }
            }
        }

		if (i == "\t")
		{
			tab_count++;
			tab_line_links.push_back(pair<int, int>(beg_tab_link, tex.size() - tab_count - 1));
			beg_tab_link = tex.size()-tab_count;
		}
    }

	if((beg_tab_link+tab_count)!=tex.size()) tab_line_links.push_back(pair<int, int>(beg_tab_link, tex.size() - 1 - tab_count));

    /// Get title height
    title_height = 0;
    for(string& i:title) title_height+=title_font_size*line_spacing*scaling;

    /// What bitmap height do we need?
    text_height = 0;
    for(string& i:tex) if(i!="\t") text_height+=font_size*line_spacing*scaling; 
}

template<class T>
int Text<T>::tab_from_pos(double mouse_x, double mouse_y)
{
	if (mouse_x > x0 && mouse_x < x1 && mouse_y >(y0 + v_margin*scaling + title_height) && mouse_y < (y0 + v_margin*scaling + title_height + text_height))
	{
		double drawing_h = y1 - y0 - 2 * v_margin * scaling - title_height;
		double draw_pos = where_bar_is * (text_height - drawing_h);
		int current_line = (int)(floor)(mouse_y - (y0 + v_margin*scaling + title_height) + draw_pos) / (font_size * line_spacing * scaling);
		for (size_t i = 0; i < tab_line_links.size(); i++)
		{
			if (current_line >= tab_line_links[i].first && current_line <= tab_line_links[i].second)
			{
				return i;
			}
		}
	}
	return -1;
}

template<class T>
response_type Text<T>::respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in, pair<char, char> input)
{
	if (selectable && (mouse_inside(mouse_x, mouse_y) || lock != -1))
	{
		if (evt == event_type::lmb_down)
		{
			if (tab_from_pos(mouse_x, mouse_y)!=-1)
			{
				hold_event = event_type::lmb_down;
				lock = in;
				tab = tab_from_pos(mouse_x, mouse_y);
				return response_type::at_least_inside;
			}
		}
		else if (evt == event_type::lmb_up)
		{
			if (lock != -1 && hold_event == event_type::lmb_down)
			{
				if (tab == tab_from_pos(mouse_x, mouse_y))
				{
					vector<string> str;
					bool new_text = false;
					(cb_ob->*cb_fun)(id, str, tab, new_text);
					if (new_text) Change_text(str);
				}
				lock = -1;
				hold_event = event_type::idle;
			}
			tab = -1;
		}
	}
    return response_type::do_nothing;
}

template<class T>
void Text<T>::draw(ALLEGRO_DISPLAY* disp)
{
    al_set_target_backbuffer(disp);
    double drawing_h = y1-y0-2*v_margin*scaling-title_height;
    double draw_pos = where_bar_is*(text_height-drawing_h);
    double first_line = (int)floor(draw_pos/(font_size*line_spacing*scaling));
    double last_line = (int)floor((draw_pos+drawing_h)/(font_size*line_spacing*scaling));


    int act_fir_line = 0;
    for(int i=0;i<(int)tex.size();i++)
    {
        if(act_fir_line==first_line) {act_fir_line=i; break;}
        if(tex[i]!="\t") act_fir_line++;
    }

    double pos = first_line*font_size*line_spacing*scaling - draw_pos;
    double end_pos = pos+(last_line-first_line+0.1)*font_size*line_spacing*scaling;
    al_set_target_bitmap(full_text);
    al_clear_to_color(background_c);

	/// highlight the currently pressed button
	if (selectable)
	{
		if (tab != -1)
		{
			double but_pos_1 = tab_line_links[tab].first * font_size * line_spacing * scaling - draw_pos;
			double but_pos_2 = but_pos_1 + (tab_line_links[tab].second - tab_line_links[tab].first + 1) * font_size * line_spacing * scaling;
			al_draw_filled_rectangle(0, but_pos_1 - 0.5*separator_h*scaling - 0.25*(line_spacing - 1.)*font_size * scaling, x1 - x0, but_pos_2 + 0.5*separator_h*scaling - 0.25*(line_spacing - 1.)*font_size*scaling, separator_c);
		}
	}

    int iter = act_fir_line;
    while(pos<end_pos && iter<(int)tex.size())
    {
        if(tex[iter]=="\t") 
        {
            al_draw_filled_rectangle(0,pos-0.5*separator_h*scaling-0.25*(line_spacing-1.)*font_size*scaling,x1-x0,pos+0.5*separator_h*scaling-0.25*(line_spacing-1.)*font_size*scaling,separator_c);
        }
        else
        {
            al_draw_textf(font,tex_c[iter],(h_margin+h_space)*scaling,pos,ALLEGRO_ALIGN_LEFT,"%s",tex[iter].c_str());
            pos+=font_size*line_spacing*scaling;
        }
        iter++;
    }

    al_set_target_backbuffer(disp);
    al_draw_bitmap(full_text,x0,y0+v_margin*scaling+title_height,0);
    al_draw_filled_rectangle(x0,y0,x1,y0+v_margin*scaling+title_height,title_background_c);
    for(int i=0;i<(int)title.size();i++)
    {
        double pos = v_margin*scaling + i*title_font_size*line_spacing*scaling;
        al_draw_textf(title_font,title_text_c,x0+(h_margin+title_h_space)*scaling,y0+pos,ALLEGRO_ALIGN_LEFT,"%s",title[i].c_str());
    }

    al_draw_filled_rectangle(x0,y0,x0+h_margin*scaling,y1,margin_c);
    al_draw_filled_rectangle(x1-h_margin*scaling,y0,x1,y1,margin_c);
    al_draw_filled_rectangle(x0,y0,x1,y0+v_margin*scaling,margin_c);
    al_draw_filled_rectangle(x0,y1-v_margin*scaling,x1,y1,margin_c);
}

template<class T>
void Text<T>::set_scaling(double factor)
{
    scaling = factor;
}

template<class T>
void Text<T>::redraw(ALLEGRO_DISPLAY* disp)
{
    al_set_target_bitmap(al_get_backbuffer(disp));
    al_draw_bitmap(full_text,x0,y0+v_margin+title_height,0);
    al_draw_filled_rectangle(x0,y0,x1,y0+v_margin+title_height,title_background_c);
    for(int i=0;i<(int)title.size();i++)
    {
        double pos = v_margin + i*title_font_size*line_spacing;
        al_draw_textf(title_font,title_text_c,x0+h_margin+title_h_space,y0+pos,ALLEGRO_ALIGN_LEFT,"%s",title[i].c_str());
    }

    al_draw_filled_rectangle(x0,y0,x0+h_margin,y1,margin_c);
    al_draw_filled_rectangle(x1-h_margin,y0,x1,y1,margin_c);
    al_draw_filled_rectangle(x0,y0,x1,y0+v_margin,margin_c);
    al_draw_filled_rectangle(x0,y1-v_margin,x1,y1,margin_c);
}

template <class T>
Text_view<T>::Text_view(double x0, double y0, double x1, double y1, bool on, vector<string> &str, int font_size, int title_font_size, double v_margin, double h_margin, COL margin_c, double max_size, cb_fun_ptr cb_fun, T * cb_ob, int id) 
	: Area(x0,y0,x1,y1,on), v_margin(v_margin), h_margin(h_margin), margin_c(margin_c), max_size(max_size)
{
    scroll_w = font_size / 5 * 8;
	if (cb_ob != NULL)
	{
		selectable = true;
		t = new Text<T>(x0 + h_margin, y0 + v_margin, x1 - scroll_w - h_margin, y1 - v_margin, true, str, cb_fun, cb_ob, id, font_size, title_font_size);
	}
    else t = new Text<T>(x0+h_margin,y0+v_margin,x1-scroll_w-h_margin,y1-v_margin,true,str, font_size, title_font_size);
	s = new Scroll(x1 - scroll_w - h_margin, t->get_y0(), x1 - h_margin, t->get_y1(), true, min(t->get_frac(), 1.));

	if (max_size != -1)
	{
		if (get_min_tot_height() < max_size)
		{
			Resize(x0, y0, x1, y0 + 2 * v_margin + get_min_tot_height());
		}
	}
}

template <class T>
void Text_view<T>::Resize (double x_0, double y_0, double x_1, double y_1)
{
    x0=x_0;
    y0=y_0;
    x1=x_1;
    y1=y_1;
    t->Resize(x0+h_margin*scaling,y0+v_margin*scaling,x1-scroll_w*scaling-h_margin*scaling,y1-v_margin*scaling);
    s->Resize(x1-scroll_w*scaling+h_margin*scaling,y0+v_margin*scaling,x1-h_margin*scaling,y1-v_margin*scaling);
	s->change_scroll(s->get_y0(), s->get_y1(), min(t->get_frac(), 1.));
}

template <class T>
void Text_view<T>::clean()
{ 
	vector<string> emp;
	Change_text(emp); 
}

template <class T>
void Text_view<T>::Change_text(vector<string> &str)
{
    t->Change_text(str);
	if (max_size != -1)
	{
		if (get_min_tot_height() < max_size)
		{
			Resize(x0, y0, x1, y0 + 2 * v_margin * scaling + get_min_tot_height());
		}
		else Resize(x0, y0, x1, y0 + max_size);
	}
	s->reset_bar();
    s->rescale_bar(min(t->get_frac(), 1.));
};

template <class T>
response_type Text_view<T>::respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in, pair<char, char> input)
{
    if(mouse_inside(mouse_x,mouse_y) || lock==in)
    {
        if(evt==event_type::lmb_down)
        {
            if(s->where_is_mouse(mouse_x,mouse_y)==1)
            {
                s->bar_moving(true);
                prev_mouse_x=mouse_x;
                prev_mouse_y=mouse_y;
                lock=in;
                hold_event = event_type::lmb_down;
                return response_type::at_least_inside;
            }
			else if (selectable && lock==-1 && text_lock == -1)
			{
				t->respond_to_event(disp, evt, mouse_x, mouse_y, mouse_dz, text_lock, hold_event, -2, input);
				if (text_lock == -2) lock = in;
			}
        }
        else if(evt==event_type::move_mouse)
        {
            if(lock==in && text_lock==-1 && hold_event==event_type::lmb_down)
            {
                s->mov(mouse_y-prev_mouse_y);
                t->set_pos(s->where_is_bar());
                prev_mouse_x=mouse_x;
                prev_mouse_y=mouse_y;
            }
        }
        else if(evt==event_type::lmb_up)
        {
            if(lock==in && text_lock==-1 && hold_event==event_type::lmb_down)
            {
                s->bar_moving(false);
                s->mov(mouse_y-prev_mouse_y);
                t->set_pos(s->where_is_bar());
                prev_mouse_x=mouse_x;
                prev_mouse_y=mouse_y;
                lock=-1;
                hold_event=event_type::idle;
            }
			else if (text_lock == -2)
			{
				t->respond_to_event(disp, evt, mouse_x, mouse_y, mouse_dz, text_lock, hold_event, -2, input);
				if (text_lock == -1) lock = -1;
			}
        }
        else if(evt==event_type::scroll && text_lock==-1)
        {
            if(t->mouse_inside(mouse_x,mouse_y))
            {
                s->mov(-mouse_dz*t->get_scroll_lines()*t->get_font_size()*t->get_line_spacing()*t->get_frac());
                t->set_pos(s->where_is_bar());
            }
        }
    }
    return response_type::do_nothing;
}

template <class T>
void Text_view<T>::check_height(ALLEGRO_DISPLAY * disp)
{
	if (max_size != -1)
	{
		if (y1 > al_get_display_height(disp))
		{
			Resize(x0, y0, x1, max(y0 + t->get_font_size()*t->get_line_spacing(), al_get_display_height(disp) - t->get_font_size()*t->get_line_spacing()));
		}
	}
}

template <class T>
void Text_view<T>::draw(ALLEGRO_DISPLAY * disp)
{
    al_set_target_backbuffer(disp);
    al_draw_filled_rectangle(x0,y0,x1,y0+v_margin*scaling,margin_c);
    al_draw_filled_rectangle(x0,y1,x1,y1-v_margin*scaling,margin_c);
    al_draw_filled_rectangle(x0,y0,x0+h_margin*scaling,y1,margin_c);
    al_draw_filled_rectangle(x1,y0,x1-h_margin*scaling,y1,margin_c);

    s->draw(disp);
    t->set_pos(s->where_is_bar());
    t->draw(disp);
}

template<class T>
void Text_view<T>::set_scaling(double factor)
{
    scaling = factor; 

    t->set_scaling(scaling);
    s->set_scaling(scaling);

    Resize(x0,y0,x1,y1);
}

template<class T>
void Text_view<T>::shift(double dx,double dy)
{
    x0 += dx; x1 +=dx; y0 +=dy; y1 +=dy;
    t->shift(dx,dy);
    s->shift(dx,dy);
}



template <class T>
class inp_view_template;

template <class T>
Input_view<T>::Input_view(double x0, double y0, double x1, double y1, bool on, string title, inp_view_template<T> t)
	: Area(x0, y0, x1, y1, on), title(title), order(t.order)
{
	title_font = load_ttf_font(font_file_name, title_font_height, &title_font);

	typename Text_input<T>::graphics g;
	g.margin = margin;
	g.corner_r = corner_r;
	g.box_margin = margin;
	g.box_corner_r = corner_r;
	g.space = space;
	g.font_size = font_size;

	g.backgr_c = button_c;
	g.p_backgr_c = p_button_c;
	g.margin_c = margin_c;
	g.p_margin_c = margin_c;
	g.box_backgr_c = inp_box_backgr_c;
	g.title_c = font_c;
	g.p_title_c = p_font_c;
	g.inp_c = inp_font_c;
	g.bar_c = inp_bar_c;

	double pos_y = y0 + v_margin + title_font_height * title_line_spacing + content_space;

	for (const auto &i : order)
	{
		double pos_x = x0 + h_margin + content_space;
		double delta_y = 0;
		for (const pair<char, int> &j : i)
		{
			if (j.first == 'b')
			{
				buttons.push_back(new Button<T>(pos_x, pos_y, font_size, font_file_name, t.buttons[j.second], margin, space, corner_r, button_c, p_button_c, margin_c, p_margin_c, font_c, p_font_c));
				pos_x += buttons.back()->get_x1() - buttons.back()->get_x0() + content_space;
				delta_y = max(delta_y, buttons.back()->get_y1() - buttons.back()->get_y0());
			}
			else if (j.first == 'i')
			{
				text_inputs.push_back(new Text_input<T>(pos_x, pos_y, pos_x + text_inp_width, font_file_name, t.text_inputs[j.second], g, true, true));
				pos_x += text_inputs.back()->get_x1() - text_inputs.back()->get_x0() + content_space;
				delta_y = max(delta_y, text_inputs.back()->get_y1() - text_inputs.back()->get_y0());
			}
			else if (j.first == 'l')
			{
				input_lists.push_back(new Input_list<T>(pos_x, pos_y, t.list_widths[j.second], font_file_name, t.input_lists[j.second], g, t.list_titles[j.second]));
				pos_x += input_lists.back()->get_x1() - input_lists.back()->get_x0() + content_space;
				delta_y = max(delta_y, input_lists.back()->get_y1() - input_lists.back()->get_y0());
			}
		}
		pos_y += delta_y + content_space;
	}
	tot_h = pos_y - y0 - v_margin - title_font_height * title_line_spacing;

	s = new Scroll(x1 - scroll_w - h_margin, y0+v_margin, x1 - h_margin, y1-v_margin, true, min((y1-y0-2*v_margin-title_font_height*title_line_spacing)/tot_h, 1.));
}

template <class T>
void Input_view<T>::Resize(double x_0, double y_0, double x_1, double y_1)
{
	double deltaY = (y_1 - y_0) - (y1 - y0);
	double delx = x_0 - x0;
	double dely = y_0 - y0;
	double orig_rat = max(1.0 - (y1 - y0 - 2*v_margin*scaling - title_font_height * title_line_spacing*scaling)/tot_h,0.);
	double new_rat = max(1.0 - (y_1 - y_0 - 2 * v_margin*scaling - title_font_height * title_line_spacing*scaling)/tot_h,0.);
	double upper_y = 0;
	if (order[0][0].first == 'b') upper_y = buttons[order[0][0].second]->get_y0() - content_space*scaling;
	else if (order[0][0].first == 'i') upper_y = text_inputs[order[0][0].second]->get_y0() - content_space*scaling;
	else if (order[0][0].first == 'l') upper_y = input_lists[order[0][0].second]->get_y0() - content_space*scaling;
	if (new_rat < orig_rat)
	{
		dely += deltaY * (y0 + v_margin*scaling + title_font_height * title_line_spacing*scaling  - upper_y) / (upper_y + tot_h - (y1 - v_margin*scaling));
	}

	x0 = x_0;
	y0 = y_0;
	x1 = x_1;
	y1 = y_1;
	for (const auto &i : buttons) i->shift(delx, dely);
	for (const auto &i : text_inputs) i->shift(delx, dely);
	for (const auto &i : input_lists) i->shift(delx, dely);
	s->Resize(x1 - scroll_w*scaling - h_margin*scaling, y0 + v_margin*scaling, x1 - h_margin*scaling, y1 - v_margin*scaling);
	s->change_scroll(s->get_y0(), s->get_y1(), min((y1 - y0 - 2 * v_margin*scaling - title_font_height * title_line_spacing*scaling) / tot_h, 1.));

	if (order[0][0].first == 'b') upper_y = buttons[order[0][0].second]->get_y0() - content_space*scaling;
	else if (order[0][0].first == 'i') upper_y = text_inputs[order[0][0].second]->get_y0() - content_space*scaling;
	else if (order[0][0].first == 'l') upper_y = input_lists[order[0][0].second]->get_y0() - content_space*scaling;
	s->set_barpos_by_rat((y0 + v_margin*scaling + title_font_height*title_line_spacing*scaling - upper_y) / (tot_h - (y1 - y0 - 2* v_margin*scaling - title_font_height * title_line_spacing*scaling)));
}

template <class T>
response_type Input_view<T>::respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt, double mouse_x, double mouse_y, double mouse_dz, int &lock, event_type &hold_event, int in, pair<char, char> input)
{
	if (mouse_inside(mouse_x, mouse_y) || lock == in)
	{
		if (evt == event_type::lmb_down)
		{
			if (s->where_is_mouse(mouse_x, mouse_y) == 1 && input_lock==-1)
			{
				s->bar_moving(true);
				prev_mouse_x = mouse_x;
				prev_mouse_y = mouse_y;
				lock = in;
				hold_event = event_type::lmb_down;
				return response_type::at_least_inside;
			}
		}
		else if (evt == event_type::move_mouse)
		{
			if (lock == in && input_lock == -1 && hold_event == event_type::lmb_down)
			{
				double inpos = s->where_is_bar()*(tot_h - (y1 - y0 - 2 * v_margin*scaling - title_font_height*title_line_spacing*scaling));
				s->mov(mouse_y - prev_mouse_y);
				double d = -(s->where_is_bar()*(tot_h - (y1 - y0 - 2 * v_margin*scaling - title_font_height*title_line_spacing*scaling)) - inpos);
				for (const auto &i : buttons) i->shift(0,d);
				for (const auto &i : text_inputs) i->shift(0, d);
				for (const auto &i : input_lists) i->shift(0, d);

				prev_mouse_x = mouse_x;
				prev_mouse_y = mouse_y;
			}
		}
		else if (evt == event_type::lmb_up)
		{
			if (lock == in && input_lock == -1 && hold_event == event_type::lmb_down)
			{
				double inpos = s->where_is_bar()*(tot_h - (y1 - y0 - 2*v_margin*scaling - title_font_height*title_line_spacing*scaling));
				s->bar_moving(false);
				s->mov(mouse_y - prev_mouse_y);
				double d = -(s->where_is_bar()*(tot_h - (y1 - y0 - 2*v_margin*scaling - title_font_height*title_line_spacing*scaling)) - inpos);
				for (const auto &i : buttons) i->shift(0, d);
				for (const auto &i : text_inputs) i->shift(0, d);
				for (const auto &i : input_lists)  i->shift(0, d);
				prev_mouse_x = mouse_x;
				prev_mouse_y = mouse_y;
				lock = -1;
				hold_event = event_type::idle;
			}
		}
		else if (evt == event_type::scroll && input_lock == -1)
		{
			if (mouse_inside(mouse_x, mouse_y))
			{
				double inpos = s->where_is_bar()*(tot_h - (y1 - y0 - 2*v_margin*scaling - title_font_height*title_line_spacing*scaling ));
				s->mov(-mouse_dz * scroll_speed);
				double d = -(s->where_is_bar()*(tot_h - (y1 - y0 - 2*v_margin*scaling - title_font_height*title_line_spacing*scaling )) - inpos);
				for (const auto &i : buttons) i->shift(0, d);
				for (const auto &i : text_inputs) i->shift(0, d);
				for (const auto &i : input_lists)  i->shift(0, d);
			}
		}
		
		int prev_lock = input_lock;
		int iter = 0;
		for (const auto &i : buttons)
		{
			i->respond_to_event(disp, evt, mouse_x, mouse_y, mouse_dz, input_lock, hold_event, iter, input);
			iter++;
		}
		for (const auto &i : text_inputs)
		{
			i->respond_to_event(disp, evt, mouse_x, mouse_y, mouse_dz, input_lock, hold_event, iter, input);
			iter++;
		}
		for (const auto &i : input_lists)
		{
			double orig_dimy = i->get_y1() - i->get_y0();
			i->respond_to_event(disp, evt, mouse_x, mouse_y, mouse_dz, input_lock, hold_event, iter, input);
			adjust_y(orig_dimy,i);
			iter++;
		}
		if (input_lock != -1) lock = in;
		else if (prev_lock != -1) lock = -1;
	}
	return response_type::do_nothing;
}

template<class T>
void Input_view<T>::adjust_y(double orig_dimy, Input_list<T> *i)
{
	if ((i->get_y1() - i->get_y0()) != orig_dimy)
	{
		double upper_y = 0;
		if (order[0][0].first == 'b') upper_y = buttons[order[0][0].second]->get_y0() - content_space*scaling;
		else if (order[0][0].first == 'i') upper_y = text_inputs[order[0][0].second]->get_y0() - content_space*scaling;
		else if (order[0][0].first == 'l') upper_y = input_lists[order[0][0].second]->get_y0() - content_space*scaling;

		double pos_y = upper_y + content_space*scaling;
		for (const auto &i : order)
		{
			double delta_y = 0;
			for (const pair<char, int> &j : i)
			{
				if (j.first == 'b')
				{
					if (buttons[j.second]->get_y0() != pos_y) buttons[j.second]->shift(0, pos_y - buttons[j.second]->get_y0());
					delta_y = max(delta_y, buttons[j.second]->get_y1() - buttons[j.second]->get_y0());
				}
				else if (j.first == 'i')
				{
					if (text_inputs[j.second]->get_y0() != pos_y) text_inputs[j.second]->shift(0, pos_y - text_inputs[j.second]->get_y0());
					delta_y = max(delta_y, text_inputs[j.second]->get_y1() - text_inputs[j.second]->get_y0());
				}
				else if (j.first == 'l')
				{
					if (input_lists[j.second]->get_y0() != pos_y) input_lists[j.second]->shift(0, pos_y - text_inputs[j.second]->get_y0());
					delta_y = max(delta_y, input_lists[j.second]->get_y1() - input_lists[j.second]->get_y0());
				}
			}
			pos_y += delta_y + content_space*scaling;
		}
		double temp_tot_h = pos_y - upper_y;
		if (temp_tot_h != tot_h)
		{
			s->rescale_bar((y1 - y0 - 2 * v_margin*scaling - title_font_height * title_line_spacing*scaling) / temp_tot_h);

			if (temp_tot_h < tot_h)
			{
				if (upper_y + temp_tot_h < (y1 - v_margin*scaling))
				{
					double d = min(y0 + v_margin*scaling + title_font_height * title_line_spacing*scaling - upper_y, y1 - v_margin*scaling - (upper_y + temp_tot_h));
					for (const auto &i : buttons) i->shift(0, d);
					for (const auto &i : text_inputs) i->shift(0, d);
					for (const auto &i : input_lists)  i->shift(0, d);
				}
			}

			double upper_y = 0;
			if (order[0][0].first == 'b') upper_y = buttons[order[0][0].second]->get_y0() - content_space*scaling;
			else if (order[0][0].first == 'i') upper_y = text_inputs[order[0][0].second]->get_y0() - content_space*scaling;
			else if (order[0][0].first == 'l') upper_y = input_lists[order[0][0].second]->get_y0() - content_space*scaling;
			s->set_barpos_by_rat((y0 + v_margin*scaling + title_font_height * title_line_spacing*scaling - upper_y) / (temp_tot_h - (y1 - y0 - 2 * v_margin*scaling - title_font_height * title_line_spacing*scaling)));

			tot_h = temp_tot_h;
		}
	}
}

template <class T>
bool Input_view<T>::apply()
{
	bool ret = true;
	for (const auto &i : text_inputs) if (!i->apply()) ret = false;
	for (const auto &i : input_lists) if (!i->apply()) ret = false;
	return ret;
}

template <class T>
void Input_view<T>::cleanup()
{
	for (const auto &i : text_inputs) i->cleanup();
	for (const auto &i : input_lists) i->cleanup();
}

template <class T>
void Input_view<T>::move_into_position()
{
	double upper_y = 0;
	if (order[0][0].first == 'b') upper_y = buttons[order[0][0].second]->get_y0() - content_space*scaling;
	else if (order[0][0].first == 'i') upper_y = text_inputs[order[0][0].second]->get_y0() - content_space*scaling;
	else if (order[0][0].first == 'l') upper_y = input_lists[order[0][0].second]->get_y0() - content_space*scaling;
	double pos_y = upper_y + content_space*scaling;

	for (const auto &i : order)
	{
		double delta_y = 0;
		for (const pair<char, int> &j : i)
		{
			if (j.first == 'b')
			{
				buttons[j.second]->shift(0, pos_y - buttons[j.second]->get_y0());
				delta_y = max(delta_y, buttons[j.second]->get_y1() - buttons[j.second]->get_y0());
			}
			else if (j.first == 'i')
			{
				text_inputs[j.second]->shift(0, pos_y - text_inputs[j.second]->get_y0());
				delta_y = max(delta_y, text_inputs[j.second]->get_y1() - text_inputs[j.second]->get_y0());
			}
			else if (j.first == 'l')
			{
				input_lists[j.second]->shift(0, pos_y - input_lists[j.second]->get_y0());
				delta_y = max(delta_y, input_lists[j.second]->get_y1() - input_lists[j.second]->get_y0());
			}
		}
		pos_y += delta_y + content_space*scaling;
	}
	tot_h = pos_y - upper_y;
}

template <class T>
void Input_view<T>::draw(ALLEGRO_DISPLAY * disp)
{
    al_set_target_backbuffer(disp);

	// If the size of items has changed, move items 
	move_into_position();

	int x, y, w, h;
	al_get_clipping_rectangle(&x, &y, &w, &h);
	al_set_clipping_rectangle(x0, y0, x1-x0, y1-y0);

	int iter = 0;
	for (const auto& i : buttons)
	{
		if (iter != input_lock) i->draw(disp);
		iter++;
	}
	for (const auto& i : text_inputs)
	{
		if (iter != input_lock) i->draw(disp);
		iter++;
	}
	for (const auto& i : input_lists)
	{
		if (iter != input_lock) i->draw(disp);
		iter++;
	}

	iter = 0;
	for (const auto& i : buttons) {if (iter == input_lock) i->draw(disp); iter++;}
	for (const auto& i : text_inputs) { if (iter == input_lock) i->draw(disp); iter++; }
	for (const auto& i : input_lists) { if (iter == input_lock) i->draw(disp); iter++; }

	al_draw_filled_rectangle(x0, y0, x1, y0 + v_margin*scaling + title_font_height * title_line_spacing *scaling, title_background_c);
	al_draw_textf(title_font, title_text_c, x0 + h_margin*scaling + content_space*scaling, y0 + v_margin*scaling, ALLEGRO_ALIGN_LEFT, "%s", title.c_str());

	s->draw(disp);

	al_set_clipping_rectangle(x, y, w, h);

	al_draw_filled_rectangle(x0, y0, x1, y0 + v_margin*scaling, margin_c);
	al_draw_filled_rectangle(x0, y1, x1, y1 - v_margin*scaling, margin_c);
	al_draw_filled_rectangle(x0, y0, x0 + h_margin*scaling, y1, margin_c);
	al_draw_filled_rectangle(x1, y0, x1 - h_margin*scaling, y1, margin_c);
}

template<class T>
void Input_view<T>::set_scaling(double factor)
{
    scaling = factor; 

    for(auto &i:buttons) i->set_scaling(scaling);
    for(auto &i:text_inputs) i->set_scaling(scaling);
    for(auto &i:input_lists) i->set_scaling(scaling);
    s->set_scaling(scaling);

    double pos_y = y0 + v_margin*scaling + title_font_height*title_line_spacing*scaling + content_space*scaling;

    int iterB = 0;
    int iterI = 0;
    int iterL = 0;
	for (const auto &i : order)
	{
		double pos_x = x0 + h_margin*scaling + content_space*scaling;
		double delta_y = 0;
		for (const pair<char, int> &j : i)
		{
			if (j.first == 'b')
			{
				delta_y = max(delta_y, buttons[iterB]->get_y1() - buttons[iterB]->get_y0());
                buttons[iterB]->shift(pos_x-buttons[iterB]->get_x0(),pos_y-buttons[iterB]->get_y0());
                pos_x += buttons[iterB]->get_x1() - buttons[iterB]->get_x0() + content_space*scaling;
                iterB++;
			}
			else if (j.first == 'i')
			{
				delta_y = max(delta_y, text_inputs[iterI]->get_y1() - text_inputs[iterI]->get_y0());
                text_inputs[iterI]->shift(pos_x-text_inputs[iterI]->get_x0(),pos_y-text_inputs[iterI]->get_y0());
                pos_x += text_inputs[iterI]->get_x1() - text_inputs[iterI]->get_x0() + content_space*scaling;
                iterI++;
			}
			else if (j.first == 'l')
			{
				delta_y = max(delta_y, input_lists[iterL]->get_y1() - input_lists[iterL]->get_y0());
                input_lists[iterL]->shift(pos_x-input_lists[iterL]->get_x0(),pos_y-input_lists[iterL]->get_y0());
                pos_x += input_lists[iterL]->get_x1() - input_lists[iterL]->get_x0() + content_space*scaling;
                iterL++;
			}
		}
		pos_y += delta_y + content_space*scaling;
	}
	tot_h = pos_y - y0 - v_margin*scaling - title_font_height*title_line_spacing*scaling;

    Resize(x0, y0, x1, y1);


/* 	for (const auto &i : order)
	{
		double pos_x = x0 + h_margin + content_space;
		double delta_y = 0;
		for (const pair<char, int> &j : i)
		{
			if (j.first == 'b')
			{
				buttons.push_back(new Button<T>(pos_x, pos_y, font_size, font_file_name, t.buttons[j.second], margin, space, corner_r, button_c, p_button_c, margin_c, p_margin_c, font_c, p_font_c));
				pos_x += buttons.back()->get_x1() - buttons.back()->get_x0() + content_space;
				delta_y = max(delta_y, buttons.back()->get_y1() - buttons.back()->get_y0());
			}
			else if (j.first == 'i')
			{
				text_inputs.push_back(new Text_input<T>(pos_x, pos_y, pos_x + text_inp_width, font_file_name, t.text_inputs[j.second], g, true, true));
				pos_x += text_inputs.back()->get_x1() - text_inputs.back()->get_x0() + content_space;
				delta_y = max(delta_y, text_inputs.back()->get_y1() - text_inputs.back()->get_y0());
			}
			else if (j.first == 'l')
			{
				input_lists.push_back(new Input_list<T>(pos_x, pos_y, t.list_widths[j.second], font_file_name, t.input_lists[j.second], g, t.list_titles[j.second]));
				pos_x += input_lists.back()->get_x1() - input_lists.back()->get_x0() + content_space;
				delta_y = max(delta_y, input_lists.back()->get_y1() - input_lists.back()->get_y0());
			}
		}
		pos_y += delta_y + content_space;
	}
	tot_h = pos_y - y0 - v_margin - title_font_height * title_line_spacing; */

/*     s->Resize(x1 - scroll_w*scaling - h_margin*scaling, y0 + v_margin*scaling, x1 - h_margin*scaling, y1 - v_margin*scaling);
	s->change_scroll(s->get_y0(), s->get_y1(), min((y1 - y0 - 2 * v_margin*scaling-title_font_height * title_line_spacing*scaling) / tot_h, 1.));

    double upper_y = 0;
	if (order[0][0].first == 'b') upper_y = buttons[order[0][0].second]->get_y0() - content_space*scaling;
	else if (order[0][0].first == 'i') upper_y = text_inputs[order[0][0].second]->get_y0() - content_space*scaling;
	else if (order[0][0].first == 'l') upper_y = input_lists[order[0][0].second]->get_y0() - content_space*scaling;
	s->set_barpos_by_rat((y0 + v_margin*scaling + title_font_height*title_line_spacing*scaling - upper_y) / 
                            (tot_h - (y1 - y0 - 2* v_margin*scaling - title_font_height * title_line_spacing*scaling)));  */
}


template <class T>
Button<T>::Button(double x_0, double y_0, double font_size, string font_file_name, button_template<T> t, double margin, double space, double corner_r, COL button_c, COL p_button_c, COL margin_c, COL p_margin_c, COL font_c, COL p_font_c)
    : k(t.k), cb_fun(t.cb_fun), cb_ob(t.cb_ob), title(t.title), id(t.id), margin(margin), corner_r(corner_r), button_c(button_c), p_button_c(p_button_c), margin_c(margin_c), p_margin_c(p_margin_c), font_c(font_c), p_font_c(p_font_c), space(space), font_size(font_size), font_file_name(font_file_name)
{
    font = load_ttf_font(font_file_name,font_size,&font);
    double w = al_get_text_width(font,title.c_str());
    x0 = x_0;
    y0 = y_0;
    x1 = x_0+w+2.0*space;
    y1 = y_0+2.0*space+al_get_font_line_height(font);
    if(k==button_type::palette)
    {
        string message = "get";
        (cb_ob->*cb_fun)(id,message);
        def_palette_c=al_map_rgb(stoi(message.substr(0,3)),stoi(message.substr(3,3)),stoi(message.substr(6,3)));
        palette_c = def_palette_c;
        prev_palette_c = def_palette_c;
        /// translate the palette colour into hue bar and palette position

        // Create control buttons
        button_template<T> pick (button_type::click," Pick ");
        button_template<T> reset (button_type::click," Reset ");
        button_template<T> deflt (button_type::click," Default ");

        Pick = new Button<T>(x1,y0+2*space+palette_dim,font_size,font_file_name,pick,0.25*space,0.75*space,0.5*space,button_c,p_button_c,p_button_c,p_button_c,font_c,p_font_c);
        Reset = new Button<T>(x1,y0+2*space+palette_dim,font_size,font_file_name,reset,0.25*space,0.75*space,0.5*space,button_c,p_button_c,p_button_c,p_button_c,font_c,p_font_c);
        Default = new Button<T>(x1,y0+2*space+palette_dim,font_size,font_file_name,deflt,0.25*space,0.75*space,0.5*space,button_c,p_button_c,p_button_c,p_button_c,font_c,p_font_c);

        double total_width = palette_dim + colourbar_w + 3*space;

        Pick->shift(space,0);
        Reset->shift(2*space + (Pick->get_x1()-Pick->get_x0()),0);
        Default->shift(total_width - space - (Default->get_x1()-Default->get_x0()),0);

        /// Create huebar bitmap
        huebar = al_create_bitmap(colourbar_w,palette_dim);
        int numrows = (int)al_get_bitmap_height(huebar);
        int numcols = (int)al_get_bitmap_width(huebar);
        al_set_target_bitmap(huebar);
        for(int i=0;i<numrows;i++) al_draw_line(0,i,numcols,i,hue_to_col((double)i/numrows),-1);
    }
    else if(k==button_type::principal_menu_root) menu = new Menu<T>(t.menu,x0,y1);
}

template <class T>
Button<T>::Button(double x0, double y0, double x1, double y1, double font_size, string font_file_name, button_template<T> t, double margin, double space, double corner_r, COL button_c, COL p_button_c, COL margin_c, COL p_margin_c, COL font_c, COL p_font_c, COL tick_c)
    : Area(x0,y0,x1,y1,false), k(t.k), cb_fun(t.cb_fun), cb_ob(t.cb_ob), title(t.title), id(t.id), margin(margin), corner_r(corner_r), button_c(button_c), p_button_c(p_button_c), margin_c(margin_c), p_margin_c(p_margin_c), font_c(font_c), p_font_c(p_font_c), tick_c(tick_c), space(space), font_size(font_size), font_file_name(font_file_name)
{
    font = load_ttf_font(font_file_name,font_size,&font);
    if(k==button_type::palette)
    {
        string message = "get";
        (cb_ob->*cb_fun)(id,message);
        def_palette_c=al_map_rgb(stoi(message.substr(0,3)),stoi(message.substr(3,3)),stoi(message.substr(6,3)));
        palette_c = def_palette_c;
        prev_palette_c = def_palette_c;
        /// translate the palette colour into hue bar and palette position


        // Create control buttons
        button_template<T> pick (button_type::click," Pick ");
        button_template<T> reset (button_type::click," Reset ");
        button_template<T> deflt (button_type::click," Default ");

        Pick = new Button<T>(x1,y0+2*space+palette_dim,font_size,font_file_name,pick,0.25*space,0.75*space,0.5*space,button_c,p_button_c,p_button_c,p_button_c,font_c,p_font_c);
        Reset = new Button<T>(x1,y0+2*space+palette_dim,font_size,font_file_name,reset,0.25*space,0.75*space,0.5*space,button_c,p_button_c,p_button_c,p_button_c,font_c,p_font_c);
        Default = new Button<T>(x1,y0+2*space+palette_dim,font_size,font_file_name,deflt,0.25*space,0.75*space,0.5*space,button_c,p_button_c,p_button_c,p_button_c,font_c,p_font_c);

        double total_width = palette_dim + colourbar_w + 3*space;

        Pick->shift(space,0);
        Reset->shift(2*space + (Pick->get_x1()-Pick->get_x0()),0);
        Default->shift(total_width - space - (Default->get_x1()-Default->get_x0()),0);

        /// Create huebar bitmap
        huebar = al_create_bitmap(colourbar_w,palette_dim);
        int numrows = (int)al_get_bitmap_height(huebar);
        int numcols = (int)al_get_bitmap_width(huebar);
        al_set_target_bitmap(huebar);
        for(int i=0;i<numrows;i++) al_draw_line(0,i,numcols,i,hue_to_col((double)i/numrows),-1);

        // Set palette parameters from the color
        set_params_from_col();
    }
    else if(k==button_type::menu_root) menu = new Menu<T>(t.menu,x1,y0);
}

template <class T>
response_type Button<T>::respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in, pair<char, char> input)
{
    if(k==button_type::click)
    {
        if(mouse_inside(mouse_x,mouse_y) && evt==event_type::lmb_down && lock==-1)
        {
            lock=in;
            highlight = true;
            return response_type::at_least_inside;
        }
        else if(evt==event_type::lmb_up && lock==in)
        {
            lock=-1;
            highlight = false;
            string reply;
            if(mouse_inside(mouse_x,mouse_y))
            {
                if(cb_ob!=NULL && cb_fun!=NULL) (cb_ob->*cb_fun)(id,reply);
                return response_type::lmb_click;
            }
        }
    }
    else if(k==button_type::palette)
    {
        if(highlight)
        {
            double pal_x0 = x1;
            double pal_x1 = x1+(3*space+colourbar_w+palette_dim)*scaling;
            double pal_y0 = y0;
            double pal_y1 = y0 + (3*space + palette_dim)*scaling + (Pick->get_y1()-Pick->get_y0());
            bool inside_whole_palette = mouse_inside(mouse_x,mouse_y) || (mouse_x<pal_x1 && mouse_x>pal_x0 && mouse_y<pal_y1 && mouse_y>pal_y0);
            if(palette_lock>-1 && highlight && evt==event_type::lmb_down && !inside_whole_palette) 
            {
                palette_lock = -1;
                palette_c = prev_palette_c;
                send_colour();
                set_params_from_col();
                return response_type::do_nothing;
            }

            if(palette_lock<1)
            {
                int lock_save = -1;
                Pick->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock_save,hold_event,3,input);
                Reset->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock_save,hold_event,4,input);
                Default->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock_save,hold_event,5,input);
                palette_lock = max(lock_save,palette_lock);

                if(palette_lock<1)
                {
                    double palette_x0 = x1 + space*scaling;
                    double palette_x1 = x1 + space*scaling + palette_dim*scaling;
                    double palette_y0 = y0 + space*scaling;
                    double palette_y1 = y0 + space*scaling + palette_dim*scaling;

                    double huebar_x0 = palette_x1 + space*scaling;
                    double huebar_x1 = palette_x1 + space*scaling + colourbar_w*scaling;
                    double huebar_y0 = palette_y0;
                    double huebar_y1 = palette_y1;

                    bool inside_palette = (mouse_x<palette_x1 && mouse_x>palette_x0 && mouse_y<palette_y1 && mouse_y>palette_y0);
                    bool inside_bar = (mouse_x<huebar_x1 && mouse_x>huebar_x0 && mouse_y<huebar_y1 && mouse_y>huebar_y0);

                    if(inside_palette && evt==event_type::lmb_down) 
                    {
                        // Find palette position
                        palette_lock = 1;
                        double palette_x0 = x1 + space*scaling;
                        double palette_x1 = x1 + (space + palette_dim)*scaling;
                        double palette_y0 = y0 + space*scaling;
                        double palette_y1 = y0 + (space + palette_dim)*scaling;
                        if(mouse_x>palette_x1) palette_pos[0] = 0;
                        else if(mouse_x<palette_x0) palette_pos[0] = 1;
                        else palette_pos[0] = (mouse_x-palette_x1)/(palette_x0-palette_x1);
                        if(mouse_y>palette_y1) palette_pos[1] = 1;
                        else if(mouse_y<palette_y0) palette_pos[1] = 0;
                        else palette_pos[1] = (mouse_y-palette_y0)/(palette_y1-palette_y0);

                        // Find color from palette position
                        COL hue_colour = hue_to_col(hue);
                        float R, G, B;
                        al_unmap_rgb_f(hue_colour,&R,&G,&B);
                        double w = palette_pos[0];
                        double b = palette_pos[1];
                        palette_c = al_map_rgb_f((1-b)*(w+R*(1-w)),(1-b)*(w+G*(1-w)),(1-b)*(w+B*(1-w)));
                        send_colour();
                    }
                    else if(inside_bar && evt==event_type::lmb_down) 
                    {
                        // Find huebar position
                        palette_lock = 2;
                        double huebar_x0 = x1 + (space + palette_dim + space)*scaling;
                        double huebar_x1 = x1 + (space + palette_dim + space + colourbar_w)*scaling;
                        double huebar_y0 = y0 + space*scaling;
                        double huebar_y1 = y0 + (space + palette_dim)*scaling;
                        if(mouse_y>huebar_y1) hue = 1;
                        else if(mouse_y<huebar_y0) hue = 0;
                        else hue = (mouse_y-huebar_y0)/(huebar_y1-huebar_y0);

                        // Find color from palette position
                        COL hue_colour = hue_to_col(hue);
                        float R, G, B;
                        al_unmap_rgb_f(hue_colour,&R,&G,&B);
                        double w = palette_pos[0];
                        double b = palette_pos[1];
                        palette_c = al_map_rgb_f((1-b)*(w+R*(1-w)),(1-b)*(w+G*(1-w)),(1-b)*(w+B*(1-w)));
                        send_colour();
                    }
                }

                if(palette_lock==-1 && inside_whole_palette) return response_type::at_least_inside;
                else if(palette_lock==0) return response_type::special_lock;
            }
            else
            {
                if(palette_lock==1) // palette
                {
                    if(evt==event_type::lmb_up) palette_lock = 0;
                    else if(evt==event_type::move_mouse)
                    {
                        // Find palette position
                        palette_lock = 1;
                        double palette_x0 = x1 + space*scaling;
                        double palette_x1 = x1 + (space + palette_dim)*scaling;
                        double palette_y0 = y0 + space*scaling;
                        double palette_y1 = y0 + (space + palette_dim)*scaling;
                        if(mouse_x>palette_x1) palette_pos[0] = 0;
                        else if(mouse_x<palette_x0) palette_pos[0] = 1;
                        else palette_pos[0] = (mouse_x-palette_x1)/(palette_x0-palette_x1);
                        if(mouse_y>palette_y1) palette_pos[1] = 1;
                        else if(mouse_y<palette_y0) palette_pos[1] = 0;
                        else palette_pos[1] = (mouse_y-palette_y0)/(palette_y1-palette_y0);

                        // Find color from palette position
                        COL hue_colour = hue_to_col(hue);
                        float R, G, B;
                        al_unmap_rgb_f(hue_colour,&R,&G,&B);
                        double w = palette_pos[0];
                        double b = palette_pos[1];
                        palette_c = al_map_rgb_f((1-b)*(w+R*(1-w)),(1-b)*(w+G*(1-w)),(1-b)*(w+B*(1-w)));
                        send_colour();
                    }
                }
                else if(palette_lock==2) // bar
                {
                    if(evt==event_type::lmb_up) palette_lock = 0;
                    else if(evt==event_type::move_mouse)
                    {
                        double huebar_x0 = x1 + (space + palette_dim + space)*scaling;
                        double huebar_x1 = x1 + (space + palette_dim + space + colourbar_w)*scaling;
                        double huebar_y0 = y0 + space*scaling;
                        double huebar_y1 = y0 + (space + palette_dim)*scaling;
                        if(mouse_y>huebar_y1) hue = 1;
                        else if(mouse_y<huebar_y0) hue = 0;
                        else hue = (mouse_y-huebar_y0)/(huebar_y1-huebar_y0);

                        // Find color from palette position
                        COL hue_colour = hue_to_col(hue);
                        float R, G, B;
                        al_unmap_rgb_f(hue_colour,&R,&G,&B);
                        double w = palette_pos[0];
                        double b = palette_pos[1];
                        palette_c = al_map_rgb_f((1-b)*(w+R*(1-w)),(1-b)*(w+G*(1-w)),(1-b)*(w+B*(1-w)));
                        send_colour();
                    }
                }
                else if(palette_lock==3) // pick (can unlock)
                {
                    response_type r = Pick->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,palette_lock,hold_event,3,input);
                    if(r!=response_type::lmb_click && palette_lock==-1) palette_lock=0;
                }
                else if(palette_lock==4) // reset (can unlock)
                {
                    response_type r = Reset->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,palette_lock,hold_event,4,input);
                    if(r==response_type::lmb_click)
                    {
                        palette_c = prev_palette_c;
                        send_colour();
                        set_params_from_col();
                    }
                    else if(palette_lock==-1) palette_lock=0;
                }
                else if(palette_lock==5) // default (can unlock)
                {
                    response_type r = Default->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,palette_lock,hold_event,5,input);
                    if(r==response_type::lmb_click)
                    {
                        palette_c = def_palette_c;
                        send_colour();
                        set_params_from_col();
                    }
                    else if(palette_lock==-1) palette_lock=0;
                }
                return response_type::special_lock;
            } 
        }
        else if(mouse_inside(mouse_x,mouse_y))
        {
            highlight=true;
            prev_palette_c = palette_c;
            return response_type::at_least_inside;
        }
    } 
    else
    {
        if(mouse_inside(mouse_x,mouse_y))
        {
            highlight=true;
            if(evt==event_type::move_mouse)
            {
                if((k==button_type::principal_menu_root || k==button_type::menu_root) && menu->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock,hold_event,in,input)!=response_type::at_least_inside) menu->unhighlight();
            }
            else if(evt==event_type::lmb_up)
            {
                string reply;
                if(k==button_type::menu_click) lock=-1;
                if(k==button_type::menu_click || k==button_type::switcher) 
				{
					(cb_ob->*cb_fun)(id,reply);
					if(k==button_type::switcher) 
					{
						 if(reply!="BLOCK") activate();
					}
				}
            }
            return response_type::at_least_inside;
        }
        else if(highlight && menu!=NULL) return menu->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock,hold_event,in,input);
    }

    return response_type::do_nothing;
}

template <class T>
void Button<T>::draw(ALLEGRO_DISPLAY * disp)
{
    al_set_target_backbuffer(disp);
    double font_height = al_get_font_line_height(font);
    if(margin<=corner_r)
    {
        al_draw_filled_rounded_rectangle(x0,y0,x1,y1,corner_r*scaling,corner_r*scaling,(highlight) ? p_margin_c : margin_c);
        al_draw_filled_rounded_rectangle(x0+margin*scaling,y0+margin*scaling,x1-margin*scaling,y1-margin*scaling,(corner_r-margin)*scaling,(corner_r-margin)*scaling,(highlight) ? p_button_c : button_c);
    }
    else
    {
        al_draw_filled_rectangle(x0,y0,x1,y1,(highlight) ? p_margin_c : margin_c);
        al_draw_filled_rectangle(x0+margin*scaling,y0+margin*scaling,x1-margin*scaling,y1-margin*scaling,(highlight) ? p_button_c : button_c);
    }
    if(k==button_type::principal_menu_root || k==button_type::click) al_draw_text(font,(highlight) ? p_font_c : font_c,x0+space*scaling,y0+space*scaling,0,title.c_str());
    else al_draw_text(font,(highlight) ? p_font_c : font_c,x0+2*space*scaling+font_height,y0+space*scaling,0,title.c_str());

    if(k==button_type::switcher && active) /// Draw tick if an active switcher
    {
        float v [8];

        v[0] = x0+space*scaling;                    v[1] = y0+space*scaling+0.38*font_height;
        v[2] = x0+space*scaling+0.38*font_height;   v[3] = y0+space*scaling+font_height;
        v[4] = x0+space*scaling+font_height;        v[5] = y0+space*scaling;
        v[6] = x0+space*scaling+0.38*font_height;   v[7] = y0+space*scaling+0.62*font_height;

        al_draw_filled_polygon(v,4,tick_c);
    }

    if(k==button_type::palette) /// Draw chosen colour if a palette
    {
        /// draw the palette colour  
        al_draw_filled_rounded_rectangle(x1-1.25*space*scaling-font_height,y0+0.75*space*scaling,x1-0.75*space*scaling,y1-0.75*space*scaling,0.75*space*scaling,0.75*space*scaling,font_c);
        al_draw_filled_rounded_rectangle(x1-space*scaling-font_height,y0+space*scaling,x1-space*scaling,y1-space*scaling,0.5*space*scaling,0.5*space*scaling,palette_c);

        if(highlight)
        {
            double total_height = 3*space*scaling+palette_dim*scaling+Pick->get_y1()-Pick->get_y0();
            // Draw the main square
            al_draw_filled_rectangle(x1,y0,x1+2*space*scaling,y0+2*space*scaling,p_button_c);
            al_draw_filled_rounded_rectangle(x1, y0, x1+(3*space+palette_dim+colourbar_w)*scaling, y0+total_height, space*scaling,space*scaling,p_button_c);
            al_draw_filled_rounded_rectangle(x1+0.5*space*scaling, y0+0.5*space*scaling, x1+(2.5*space+palette_dim+colourbar_w)*scaling, y0+total_height-0.5*space*scaling, 0.5*space*scaling,0.5*space*scaling,font_c);
            // Draw buttons
            Pick->draw(disp);
            Reset->draw(disp);
            Default->draw(disp);

            // Draw hue bar
            al_draw_bitmap(huebar,x1+(2*space+palette_dim)*scaling,y0+space*scaling,0);

            // Draw the hue bar selector
            COL hue_colour = hue_to_col(hue);
            float R, G, B;
            al_unmap_rgb_f(hue_colour,&R,&G,&B);
            al_draw_filled_rounded_rectangle(x1+(palette_dim+1.75*space)*scaling,y0+(space+palette_dim*hue-0.75*space)*scaling,x1+(palette_dim+2.25*space+colourbar_w)*scaling,y0+(space+palette_dim*hue+0.75*space)*scaling,0.25*space*scaling,0.25*space*scaling,al_map_rgb(255,255,255));
            al_draw_filled_rounded_rectangle(x1+(palette_dim+1.95*space)*scaling,y0+(space+palette_dim*hue-0.55*space)*scaling,x1+(palette_dim+2.05*space+colourbar_w)*scaling,y0+(space+palette_dim*hue+0.55*space)*scaling,0.10*space*scaling,0.10*space*scaling,hue_colour);

            // Draw the palette

            Nikal n;
            for(int i=0;i<(int)(palette_dim*scaling);i++)
            {
                double bl=(double)i/(palette_dim*scaling);
                n.add_gradient_line(x1+space*scaling,y0+space*scaling+i,x1+(space+palette_dim)*scaling,y0+space*scaling+i,al_map_rgb_f(1.0-bl,1.0-bl,1.0-bl),al_map_rgb_f((1.0-bl)*R,(1.0-bl)*G,(1.0-bl)*B),1);
            } 
            n.draw(); 

            // Draw the palette picker
            al_draw_filled_circle(x1+space*scaling+(1-palette_pos[0])*palette_dim*scaling,y0+space*scaling+palette_pos[1]*palette_dim*scaling,0.75*space*scaling,al_map_rgb(255,255,255)); 
            al_draw_filled_circle(x1+space*scaling+(1-palette_pos[0])*palette_dim*scaling,y0+space*scaling+palette_pos[1]*palette_dim*scaling,0.60*space*scaling,palette_c); 
        }
    }

    if(k==button_type::menu_root) /// Draw arrow if an active menu
    {
        al_draw_filled_triangle(x1-space*scaling-0.81*font_height,y0+space*scaling+0.19*font_height,x1-space*scaling-0.81*font_height,y1-space*scaling-0.19*font_height,x1-space*scaling-0.19*font_height,y0+0.5*(y1-y0),tick_c);
    }
    if(highlight && menu!=NULL) menu->draw(disp);
}

template<class T>
void Button<T>::set_scaling(double factor)
{
    scaling = factor; 

    // First resize all the elements appropriately 
    if(menu!=NULL) menu->set_scaling(scaling);

    // Then shift them in place accordingly
    double w = al_get_text_width(font,title.c_str());
    x1 = x0+w+2.0*space*scaling;
    y1 = y0+2.0*space*scaling+al_get_font_line_height(font);

    if(k==button_type::menu_root) 
    {
        menu->shift(x1-menu->get_x0(), y0-menu->get_y0());
    }
    else if(k==button_type::principal_menu_root)
    {
        menu->shift(x0-menu->get_x0(), y1-menu->get_y0());
    }
    else if(k==button_type::palette) 
    {
        Pick->set_scaling(scaling);
        Reset->set_scaling(scaling);
        Default->set_scaling(scaling);

        double total_width = scaling*(palette_dim + colourbar_w + 3*space);
        Pick->shift((x1 + space*scaling)-Pick->get_x0(),
            (y0+(2*space+palette_dim)*scaling)-Pick->get_y0());
        Reset->shift((space*scaling + Pick->get_x1())-Reset->get_x0(),
            Pick->get_y0()-Reset->get_y0());
        Default->shift((x1+total_width - space*scaling - (Default->get_x1()-Default->get_x0()))-Default->get_x0(),
            Pick->get_y0()-Default->get_y0());

        /// Recreate huebar bitmap
        al_destroy_bitmap(huebar);
        huebar = al_create_bitmap(colourbar_w*scaling,palette_dim*scaling);
        int numrows = (int)al_get_bitmap_height(huebar);
        int numcols = (int)al_get_bitmap_width(huebar);
        al_set_target_bitmap(huebar);
        for(int i=0;i<numrows;i++) al_draw_line(0,i,numcols,i,hue_to_col((double)i/numrows),-1);

        // Set palette parameters from the color
        set_params_from_col();
    }
}

template <class T>
void Button<T>::Resize (double x_0, double y_0, double x_1, double y_1) 
{ 
    x0=x_0; y0=y_0; x1=x_1; y1=y_1;
    if(menu!=NULL) menu->shift(x1-menu->get_x0(),y0-menu->get_y0());
    if(Pick!=NULL && Reset!=NULL && Default!=NULL)
    {
        double total_width = scaling*(palette_dim + colourbar_w + 3*space);
        Pick->shift((x1 + space*scaling)-Pick->get_x0(),
            (y0+(2*space+palette_dim)*scaling)-Pick->get_y0());
        Reset->shift((space*scaling + Pick->get_x1())-Reset->get_x0(),
            Pick->get_y0()-Reset->get_y0());
        Default->shift((x1+total_width - space*scaling - (Default->get_x1()-Default->get_x0()))-Default->get_x0(),
            Pick->get_y0()-Default->get_y0());
    }
}

template <class T>
void Button<T>::unhighlight()
{
    if(highlight)
    {
        highlight=false;
        if(menu!=NULL) menu->unhighlight();
    }
}

template <class T>
void Button<T>::shift(double dx,double dy)
{
    x0+=dx;
    y0+=dy;
    x1+=dx;
    y1+=dy;
    if(menu!=NULL) menu->shift(dx,dy);
    if(Pick!=NULL) Pick->shift(dx,dy);
    if(Reset!=NULL) Reset->shift(dx,dy);
    if(Default!=NULL) Default->shift(dx,dy);     
}

template <class T>
COL Button<T>::hue_to_col (double hue) // 0<=hue<=1
{
    if(hue>1 || hue<0) return al_map_rgb_f(1,0,0);
    else if (hue<1./6.) return al_map_rgb_f(1,hue*6.,0);
    else if (hue<2./6.) return al_map_rgb_f(1.-(hue-1./6.)*6.,1,0);
    else if (hue<3./6.) return al_map_rgb_f(0,1,(hue-2./6.)*6.);
    else if (hue<4./6.) return al_map_rgb_f(0,1.-(hue-3./6.)*6.,1);
    else if (hue<5./6.) return al_map_rgb_f((hue-4./6.)*6.,0,1);
    else return al_map_rgb_f(1,0,1.-(hue-5./6.)*6.);
}

template <class T>
double Button<T>::col_to_hue(COL c)
{
    float r,g,b;
    al_unmap_rgb_f(c,&r,&g,&b);
    if(r==1 && b==0) return g/6.;
    else if(g==1 && b==0) return (2.-r)/6.;
    else if(r==0 && g==1) return (2.+b)/6.;
    else if(r==0 && b==1) return (4.-g)/6.;
    else if(g==0 && b==1) return (4.+r)/6.;
    else return (6.-b)/6.;
}

template <class T>
void Button<T>::set_params_from_col()
{
    float r,g,b;
    al_unmap_rgb_f(palette_c,&r,&g,&b);
    
    if(r==g && g==b) 
    {
        hue = 0;
        palette_pos[0] = 1;
        palette_pos[1] = 1-r;
    }
    else
    {
        double beta = min(min(r,g),b);
        double alpha = max(max(r,g),b)-beta;

        float R = (r - beta)/alpha;
        float G = (g - beta)/alpha;
        float B = (b - beta)/alpha;

        hue = col_to_hue(al_map_rgb_f(R,G,B));

        // Get the palette position

        palette_pos[0] = beta / (alpha+beta);
        palette_pos[1] = 1.0 - alpha - beta;
    }
}

template <class T>
void Button<T>::send_colour()
{
    auto lengthen = [] (int n)
    {
        string s = to_string(n);
        if(s.length()==1) return "00"+s;
        else if(s.length()==2) return "0"+s;
        else return s;
    };
    auto conv = [&lengthen] (COL &c)
    {
        unsigned char r, g, b;
        al_unmap_rgb(c,&r,&g,&b);
        return lengthen((int)r)+lengthen((int)g)+lengthen((int)b);
    };
    string c = conv(palette_c);
    (cb_ob->*cb_fun)(id,c);
}

template <class T>
Text_input<T>::Text_input(double x_0, double y_0, double x_1, string font_file_name, text_inp_template<T> t, graphics g, bool switchable, bool persistent)
    : cb_ob(t.cb_ob), switchable(switchable), persistent(persistent), title(t.title), id(t.id), g(g), font_file_name(font_file_name)
{
    font = load_ttf_font(font_file_name,g.font_size,&font);
    double w = al_get_text_width(font,title.c_str());
    x0 = x_0; y0 = y_0; x1 = x_1; y1 = y_0+2*g.space+al_get_font_line_height(font);
    double box_space = 0.5*(y1-y0-al_get_font_line_height(font));
    box_x0 = x0 + 2*g.space + w; box_y0 = y0 + box_space - g.box_margin; box_x1 = x1 - g.space; box_y1 = y1 - box_space + g.box_margin;
    input_w = box_x1-box_x0;

	if (t.cb_fun1 != NULL) cb_fun1 = t.cb_fun1;
	else if (t.cb_fun2 != NULL)
	{
		switchable = true;
		cb_fun2 = t.cb_fun2;
		with_list = true;
		list_space = min(g.space, box_space);
		vector<string> str;
		t_view = new Text_view<Text_input>(box_x0 + g.box_margin, y1+list_space, box_x0 + g.box_margin + t.width, y1 + list_space + t.max_h, true, str, g.font_size, g.font_size, 0, 0, al_map_rgb(255, 255, 255), t.max_h, &Text_input::respond_to_list, this, t.id);
        text_view_w = t.width;
        text_max_h = t.max_h;
    }

	if (!switchable) high();
}

template <class T>
void Text_input<T>::initialize()
{
	if (with_list)
	{
		bool new_text = true;
		vector<string> str;
		(cb_ob->*cb_fun2)(id, str, -2, new_text);
		if (str.size() > 0)
		{
			content = str[0];
			valid_content = content;
			stat = valid;
		}
		else valid_content = "\r\b";
	}
	else
	{
		content = "\r";
		(cb_ob->*cb_fun1)(id, content);
		if (content == "\r") content.clear();
		else
		{
			valid_content = content;
			stat = valid;
		}
	}
}

template <class T>
void Text_input<T>::respond_to_list(int n, vector<string> &s, int select, bool &new_text)
{
	(cb_ob->*cb_fun2)(n,s,select,new_text);
	unlock_next_round = true;
	stat = valid;
	valid_content = content;
	if(!s.empty()) content = s[0];
}

template <class T>
response_type Text_input<T>::respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in, pair<char, char> input)
{
	if (!initialized)
	{
		initialize();
		initialized = true;
	}

	if (draw_list)
	{
		t_view->Resize(box_x0 + g.box_margin, t_view->get_y0(), box_x0 + g.box_margin + t_view->get_x1() - t_view->get_x0(), t_view->get_y1());
		double delta = max(t_view->get_x1() - al_get_display_width(disp) + t_view->get_scroll_w()*1.61, 0.0);
		if (delta > 0) t_view->Resize(t_view->get_x0() - delta, t_view->get_y0(), t_view->get_x1() - delta, t_view->get_y1());
	}

	auto handle_keyboard_inp = [&]()
	{
		if (lock == in || !switchable)
		{
			if (evt == event_type::keyboard)
			{
				if (!with_list && input.first == '\r' && input.second == 'n')
				{
					if (switchable)
					{
						unhigh();
						lock = -1;
					}

					string temp = "\b" + content;
					(cb_ob->*cb_fun1)(id, temp);
					if (temp == "valid")
					{
						valid_content = content;
						stat = valid;
					}
					else if (temp == "invalid")
					{
						stat = invalid;
					}
					else stat = unknown;

					(cb_ob->*cb_fun1)(id, content);
					if (!persistent) content.clear();
				}
				else if (!with_list && input.first == '\t' && input.second == 'n')
				{
					(cb_ob->*cb_fun1)(-id, content);
				}
				else if (input.first == '\b' && input.second == 'n')
				{
					if (!content.empty()) content.pop_back();
				}
				else if (input.first == 'c' && input.second == 'c') // copy
				{
					al_set_clipboard_text(disp, content.c_str());
				}
				else if (input.first == 'v' && input.second == 'c') // paste
				{
					content += string(al_get_clipboard_text(disp));
				}
				else if (isprint(input.first))
				{
					content.push_back(input.first);
				}

				if (with_list)
				{
					bool new_text = false;
					vector<string> str;
					if (!content.empty()) str.push_back(content);
					(cb_ob->*cb_fun2)(id, str, -1, new_text);
					t_view->Change_text(str);
					if (!t_view->text_empty()) draw_list = true;
					else draw_list = false;
				}
				if (stat!=invalid && content != valid_content) stat = unknown;
			}
		}
	};

    if(switchable)
    {
		if(mouse_x>=box_x0 && mouse_x<=box_x1 && mouse_y>=box_y0 && mouse_y<=box_y1 && lock==-1)
		{
			if(evt==event_type::lmb_down && lock==-1)
			{
				high();
				lock=in;
				if (with_list)
				{
					bool new_text = false;
					vector<string> str;
					if (!content.empty()) str.push_back(content);
					(cb_ob->*cb_fun2)(id, str, -1, new_text);
					t_view->Change_text(str);
					if (!t_view->text_empty()) draw_list = true;
					else draw_list = false;
				}
				return response_type::at_least_inside;
			}
		}
		else if(lock==in)
		{
			if(with_list) t_view->respond_to_event(disp, evt, mouse_x, mouse_y, mouse_dz, list_lock, hold_event, 1, input);
			if (unlock_next_round)
			{
				draw_list = false;
				unhigh();
				unlock_next_round = false;
				lock = -1;
			}
			else if(list_lock == -1 && evt==event_type::lmb_down)
			{
				if (!with_list)
				{
					string temp = "\b" + content;
					(cb_ob->*cb_fun1)(id, temp);
					if (temp == "valid") stat = valid;
					else if (temp == "invalid") stat = invalid;
					else stat = unknown;
				}
				else
				{
					if (valid_content != "\r\b" && content == valid_content) stat = valid;
				}

				unhigh();
				if(with_list) draw_list = false;
				lock = -1;
			}
		}
    }

    handle_keyboard_inp();

    return response_type::do_nothing;
}

template <class T>
void Text_input<T>::draw(ALLEGRO_DISPLAY * disp)
{
    al_set_target_backbuffer(disp);

	if (draw_list)
	{
		t_view->Resize(box_x0 + g.box_margin*scaling, t_view->get_y0(), box_x0 + g.box_margin*scaling + t_view->get_x1() - t_view->get_x0(), t_view->get_y1());
		double delta = max(t_view->get_x1() - al_get_display_width(disp) + t_view->get_scroll_w()*1.61,0.0);
		if (delta > 0) t_view->Resize(t_view->get_x0()-delta, t_view->get_y0(), t_view->get_x1()-delta, t_view->get_y1());
		t_view->check_height(disp);
	}

    /// Draw input background
    if(g.margin<=g.corner_r)
    {
        al_draw_filled_rounded_rectangle(x0,y0,x1,y1,g.corner_r*scaling,g.corner_r*scaling,(highlight) ? g.p_margin_c : g.margin_c);
		if (draw_list)
		{
			al_draw_filled_rectangle(max(t_view->get_x0() - list_space*scaling,x0), y1- g.corner_r*scaling, min(x1, t_view->get_x1() + list_space*scaling) , y1+g.corner_r*scaling, (highlight) ? g.p_margin_c : g.margin_c);
			al_draw_filled_rounded_rectangle(t_view->get_x0()-list_space*scaling, y1, t_view->get_x1() + list_space*scaling, t_view->get_y1() + list_space*scaling, g.corner_r*scaling, g.corner_r*scaling, (highlight) ? g.p_margin_c : g.margin_c);
		}
        al_draw_filled_rounded_rectangle(x0+g.margin*scaling,y0+g.margin*scaling,x1-g.margin*scaling,y1-g.margin*scaling,(g.corner_r-g.margin)*scaling,(g.corner_r-g.margin)*scaling, (highlight) ? g.p_backgr_c : g.backgr_c);
		if (draw_list)
		{
			al_draw_filled_rounded_rectangle(t_view->get_x0() - list_space*scaling + g.margin*scaling, y1 + g.margin*scaling, t_view->get_x1() + list_space*scaling - g.margin*scaling, t_view->get_y1() + (list_space - g.margin)*scaling, (g.corner_r - g.margin)*scaling, (g.corner_r - g.margin)*scaling, (highlight) ? g.p_backgr_c : g.backgr_c);
			al_draw_filled_rectangle(max(t_view->get_x0() - list_space*scaling, x0) + g.margin*scaling, y1 - g.corner_r*scaling, min(x1, t_view->get_x1() + list_space*scaling) - g.margin*scaling, t_view->get_y0() - list_space*scaling + (g.corner_r + g.margin)*scaling, (highlight) ? g.p_backgr_c : g.backgr_c);
		}
    }
    else
    {
        al_draw_filled_rectangle(x0,y0,x1,y1,(highlight) ? g.p_margin_c : g.margin_c);
        al_draw_filled_rectangle(x0+g.margin*scaling,y0+g.margin*scaling,x1-g.margin*scaling,y1-g.margin*scaling, (highlight) ? g.p_backgr_c : g.backgr_c);
    }

    /// Draw title
    al_draw_text(font,(highlight) ? g.p_title_c : g.title_c,x0+g.space*scaling,y0+g.space*scaling,0,title.c_str());

    /// Draw box background
    if(g.box_margin<=g.box_corner_r) al_draw_filled_rounded_rectangle(box_x0,box_y0,box_x1,box_y1,(g.box_corner_r-g.box_margin)*scaling,(g.box_corner_r-g.box_margin)*scaling,(stat== unknown) ? g.box_backgr_c : (stat==valid) ? valid_c : invalid_c);
	else al_draw_filled_rectangle(box_x0, box_y0, box_x1, box_y1, (stat == unknown) ? g.box_backgr_c : (stat == valid) ? valid_c : invalid_c);


    /// Draw text
    double w = al_get_text_width(font,content.c_str());
    double free_box_w = box_x1-box_x0-2*max(g.box_margin,g.box_corner_r)*scaling;
    double text_tab = max(g.box_margin,g.box_corner_r)*scaling;
    if(w<free_box_w)
    {
        al_draw_text(font,g.inp_c,box_x0+text_tab,y0+g.space*scaling,0,content.c_str());
    }
    else
    {
        al_set_clipping_rectangle(box_x0+text_tab,box_y0,free_box_w,box_y1);
        al_draw_text(font,g.inp_c,box_x1-text_tab,y0+g.space*scaling,ALLEGRO_ALIGN_RIGHT,content.c_str());
        al_set_clipping_rectangle(0,0,al_get_bitmap_width(al_get_backbuffer(disp)),al_get_bitmap_height(al_get_backbuffer(disp)));
    }

    /// Draw bar
    if(highlight)
    {
        double bar_h = al_get_font_line_height(font);
        double bar_w = bar_w_h_rat*bar_h;
        double delta = 0.5*(box_y1-box_y0-bar_h);
        al_draw_filled_rectangle(box_x0+text_tab+min(w,free_box_w),box_y0+delta,box_x0+text_tab+min(w,free_box_w)+bar_w,box_y1-delta,g.bar_c);
    }

    /// Draw margin around text input area
    if(g.box_margin<=g.box_corner_r)
    {
        al_draw_rounded_rectangle(box_x0,box_y0,box_x1,box_y1,g.box_corner_r*scaling,g.box_corner_r*scaling,(highlight) ? g.margin_c : g.p_margin_c,g.box_margin*scaling);
    }
    else
    {
        al_draw_rectangle(box_x0,box_y0,box_x1,box_y1,(highlight) ? g.margin_c : g.p_margin_c,g.box_margin*scaling);
    }

	// Draw list

	if (draw_list) t_view->draw(disp);
}

template<class T>
void Text_input<T>::set_scaling(double factor)
{
    scaling = factor; 

    y1 = y0+2*g.space*scaling+al_get_font_line_height(font);
    
    double w = al_get_text_width(font,title.c_str());
    double box_space = 0.5*(y1-y0-al_get_font_line_height(font));

    x1 = x0 + w + (input_w+3*g.space)*scaling;

    box_x0 = x0 + 2*g.space*scaling + w; 
    box_y0 = y0 + box_space - g.box_margin*scaling; 
    box_x1 = x1 - g.space*scaling;
    box_y1 = y1 - box_space + g.box_margin*scaling;

    if(t_view!=NULL) 
    {
        t_view->set_scaling(scaling);
        //t_view->shift(box_x0 + g.box_margin*scaling-t_view->get_x0(),y1 + list_space*scaling - t_view->get_y0());
        t_view->Resize(box_x0 + g.box_margin*scaling, y1 + list_space*scaling, box_x0 + g.box_margin*scaling + text_view_w*(0.5*(scaling-1.0)+1.0), y1 + list_space*scaling + text_max_h*(0.5*(scaling-1.0)+1.0));
    }
}

template<class T>
void Text_input<T>::Resize(double x_0, double y_0, double x_1, double y_1) 
{
    x0 = x_0; x1 = x_1; y0 = y_0; y1 = y_1;
    double w = al_get_text_width(font,title.c_str());
    double box_space = 0.5*(y1-y0-al_get_font_line_height(font));
    box_x0 = x0 + 2*g.space*scaling + w; 
    box_y0 = y0 + box_space - g.box_margin*scaling; 
    box_x1 = x1 - g.space*scaling;
    box_y1 = y1 - box_space + g.box_margin*scaling;
}

template <class T>
void Text_input<T>::shift(double dx,double dy)
{
    x0+=dx; y0+=dy; x1+=dx; y1+=dy;
    box_x0+=dx; box_y0+=dy; box_x1+=dx; box_y1+=dy;
	if(with_list) t_view->Resize(t_view->get_x0() + dx, t_view->get_y0() + dy, t_view->get_x1() + dx, t_view->get_y1() + dy);
}

template <class T>
bool Text_input<T>::apply()
{
	validate();
	if (stat == valid) return true;
	else return false;
}

template <class T>
void Text_input<T>::validate()
{
	if (!with_list)
	{
		string temp = "\b" + content;
		(cb_ob->*cb_fun1)(id, temp);
		if (temp == "valid")
		{
			valid_content = content;
			stat = valid;
		}
		else stat = invalid;
	}
	else if (stat == unknown)
	{
		if (valid_content != content) stat = invalid;
		else stat = valid;
	}
}

template <class T>
Input_list<T>::Input_list(double x_0, double y_0, vector<double> widths, string font_file_name, vector<text_inp_template<T>> t, typename Text_input<T>::graphics g, string title)
	: t(t), g(g), font_file_name(font_file_name), widths(widths)
{
	button_template<Input_list> tadd(button_type::click, &Input_list::button_response, this, "Add " + title, 0);
	button_template<Input_list> tremove(button_type::click, &Input_list::button_response, this, "Remove " + title, 1);
	Button<Input_list> tempb(0.0, 0.0, g.font_size, font_file_name, tadd, g.margin, g.space, g.corner_r, g.backgr_c, g.p_backgr_c, g.margin_c, g.p_margin_c, g.title_c, g.p_title_c);

	x0 = x_0;
	y0 = y_0;
	x1 = x_0;
	for (const double& i : widths) x1 += i;
	y1 = y_0 + tempb.get_y1();

	add = new Button<Input_list>(x0, y0, 0.5*(x0 + x1), y0 + tempb.get_y1(), g.font_size, font_file_name, tadd, g.margin, g.space, g.corner_r, g.backgr_c, g.p_backgr_c, g.margin_c, g.p_margin_c, g.title_c, g.p_title_c, g.title_c);
	remove = new Button<Input_list>(0.5*(x0 + x1), y0, x1, y0 + tempb.get_y1(), g.font_size, font_file_name, tremove, g.margin, g.space, g.corner_r, g.backgr_c, g.p_backgr_c, g.margin_c, g.p_margin_c, g.title_c, g.p_title_c, g.title_c);
}

template <class T>
void Input_list<T>::button_response(int n, string &s)
{
	if (n == 0) // add new input 
	{
		double pos_x = x0;
		int cur_size = inputs.size() / t.size() + 1;
		for (size_t i=0;i<t.size();i++)
		{
			text_inp_template<T> temp = t[i];
			temp.id = cur_size;
			inputs.push_back(new Text_input<T>(pos_x, y1 - (add->get_y1() - add->get_y0()), pos_x + widths[i], font_file_name, temp, g, true, true));
			inputs.back()->set_scaling(scaling);
            inputs.back()->Resize(pos_x, y1 - (add->get_y1() - add->get_y0()), pos_x + widths[i]*scaling, inputs.back()->get_y1());
            pos_x += widths[i]*scaling;
		}
		y1 += inputs.back()->get_y1() - inputs.back()->get_y0();
		add->shift(0, inputs.back()->get_y1() - inputs.back()->get_y0());
		remove->shift(0, inputs.back()->get_y1() - inputs.back()->get_y0());
	}
	else if (n == 1) // remove input
	{
		if (inputs.size() > 0)
		{
			y1 -= inputs.back()->get_y1() - inputs.back()->get_y0();
			add->shift(0, -(inputs.back()->get_y1() - inputs.back()->get_y0()));
			remove->shift(0, -(inputs.back()->get_y1() - inputs.back()->get_y0()));
			for (size_t i = 0; i < t.size(); i++)
			{
				inputs.back()->sign_upon_destr(true);
				delete inputs.back();
				inputs.pop_back();
			}
		}
	}
}

template <class T>
void Input_list<T>::set_length(int len)
{
	if (!inputs.empty())
	{
		for (int i = ((int)inputs.size() - 1); i >= 0; i--)
		{
			inputs[i]->sign_upon_destr(false);
			delete inputs[i];
		}
	}
	inputs.clear();
	y1 = y0 + add->get_y1() - add->get_y0();
	add->shift(0,y0 - add->get_y0());
	remove->shift(0,y0 - remove->get_y0());
	string s;
	for (int i = 0; i < len; i++)  button_response(0, s);
}

template <class T>
response_type Input_list<T>::respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt, double mouse_x, double mouse_y, double mouse_dz, int &lock, event_type &hold_event, int in, pair<char, char> input)
{
	if (lock == in || lock ==-1)
	{
		add->respond_to_event(disp, evt, mouse_x, mouse_y, mouse_dz, input_lock, hold_event, 0, input);
		remove->respond_to_event(disp, evt, mouse_x, mouse_y, mouse_dz, input_lock, hold_event, 1, input);
		int iter = 2;
		for (const auto &i : inputs)
		{
			i->respond_to_event(disp, evt, mouse_x, mouse_y, mouse_dz, input_lock, hold_event, iter, input);
			iter++;
		}
		if (input_lock != -1) lock = in;
		else lock = -1;
	}
	return response_type::do_nothing;
}

template <class T>
void Input_list<T>::draw(ALLEGRO_DISPLAY * disp)
{
	al_set_target_backbuffer(disp);
	add->draw(disp);
	remove->draw(disp);
	int iter = 2;
	for (const auto &i : inputs) 
	{
		if (iter != input_lock) i->draw(disp); 
		iter++;
	}
	iter = 2;
	for (const auto &i : inputs)
	{
		if (iter == input_lock) i->draw(disp);
		iter++;
	}
}

template<class T>
void Input_list<T>::set_scaling(double factor)
{
    scaling = factor;

    add->set_scaling(scaling);
    remove->set_scaling(scaling);
    int iter = 0;
    for(size_t i=0;i<(inputs.size()/t.size());i++) 
    {
        for (size_t j=0;j<t.size();j++)
        {
            inputs[iter]->set_scaling(scaling);
            inputs[iter]->Resize(0, inputs[iter]->get_y0(), widths[j]*scaling, inputs[iter]->get_y1());
            iter++;
        }
    }

    y1 = 0;
    double pos_y = y0;
    iter = 0;
    for(size_t i=0;i<(inputs.size()/t.size());i++)
    {
        double pos_x = x0;
        for (size_t j=0;j<t.size();j++)
        {
            inputs[iter]->shift(pos_x-inputs[iter]->get_x0(),pos_y-inputs[iter]->get_y0());
            pos_x += inputs[iter]->get_x1()-inputs[iter]->get_x0();
            iter++;
        }
        pos_y=inputs[iter-1]->get_y1();
    }
    x1 = x0;
    for(const auto &i:widths) x1+=i*scaling;
    add->Resize(x0,pos_y,0.5*(x0 + x1),pos_y+add->get_y1()-add->get_y0());
    remove->Resize(0.5*(x0 + x1), pos_y, x1 , pos_y+remove->get_y1()-remove->get_y0());
    y1 = add->get_y1();
}   

template <class T>
void Input_list<T>::shift(double dx, double dy)
{
	x0 += dx;
	y0 += dy;
	x1 += dx;
	y1 += dy;
	for (const auto &i : inputs) i->shift(dx,dy);
	add->shift(dx, dy);
	remove->shift(dx, dy);
}









template <class T>
Menu<T>::Menu(vector<button_template<T> > b_temps,double x_0, double y_0)
{
    x0 = x_0;
    y0 = y_0;
    FONT * font = load_ttf_font(font_file_name,font_size,&font);
    double font_height = al_get_font_line_height(font);
    double max_w = 0;
    for(auto &i:b_temps) max_w = max((double)al_get_text_width(font,i.title.c_str()),max_w);
    attempt_destroy_font(font_file_name,font_size,&font);
    x1 = x_0 + max_w + 4*space + 2*font_height;
    y1 = y_0 + b_temps.size()*(2*space+font_height);

    int iter = 0;
    bool found_first_switcher = false;
    for(auto &i:b_temps)
    {
        buttons.push_back(new Button<T>(x_0,y_0+iter*(2*space+font_height),x1,y_0+(1+iter)*(2*space+font_height),font_size,font_file_name,i,margin,space,corner_r,button_c,p_button_c,margin_c,p_margin_c,font_c,p_font_c,tick_c));
        if(i.k==button_type::switcher && !found_first_switcher)
        {
            buttons.back()->activate();
            buttons.back()->run_function();
            found_first_switcher=true;
        }
        iter++;
    }
}

template <class T>
response_type Menu<T>::respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt, double mouse_x, double mouse_y, double mouse_dz, int &lock, event_type &hold_event, int in, pair<char, char> input)
{
    if(button_lock==-1)
    {
        for(int i=0;i<(int)buttons.size();i++)
        {
            response_type r =  buttons[i]->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock,hold_event,in,input);
            if(r==response_type::at_least_inside)
            {
                /// unhighlight every other button in the menu (with all its subbottons (write a function for this))
                for(int j=0;j<(int)buttons.size();j++) if(j!=i) buttons[j]->unhighlight();
                /// if the button is a switcher, deactivate all other switchers in this menu
                if(buttons[i]->get_k()==button_type::switcher && buttons[i]->is_active() && evt==event_type::lmb_up) for(int j=0;j<(int)buttons.size();j++) if(j!=i) buttons[j]->deactivate();
                return r;
            }
            else if(r==response_type::special_lock) 
            {
                button_lock = i;
                return response_type::at_least_inside;
            }
        }
    }
    else 
    {
        response_type r =  buttons[button_lock]->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock,hold_event,in,input);
        if(r!=response_type::special_lock) 
        {
            button_lock=-1;
        }
        return response_type::at_least_inside;
    }
    return response_type::do_nothing;
}

template <class T>
void Menu<T>::draw(ALLEGRO_DISPLAY * disp)
{
    al_set_target_backbuffer(disp);
    al_draw_rectangle(x0,y0,x1,y1,margin_c,margin*scaling);
    for(auto &i:buttons) i->draw(disp);
}

template<class T>
void Menu<T>::set_scaling(double factor)
{
    scaling = factor; 

    // First resize all the elements appropriately 
    for(auto& i:buttons) i->set_scaling(scaling);

    // Then shift them in place accordingly
    double font_height = 0;
    if(buttons.size()>0) font_height = al_get_font_line_height(buttons.front()->get_font());

    double max_w = 0;
    for(auto &i:buttons) max_w = max((double)al_get_text_width(i->get_font(),i->get_title().c_str()),max_w);
    x1 = x0 + max_w + scaling*4*space + 2*font_height;

    double pos_x = x0;
    double pos_y = y0;

   // int iter = 0;
    for(auto &i:buttons) 
    {
      //  i->shift(pos_x-i->get_x0(),pos_y-i->get_y0());
        i->Resize(x0,pos_y,x1,pos_y+i->get_y1()-i->get_y0());
        pos_y+=i->get_y1()-i->get_y0();
     //   iter++;
    }

    y1 = buttons.size()*(scaling*2*space+font_height);
}

template <class T>
void Menu<T>::unhighlight()
{
    for(auto &i:buttons) i->unhighlight();
}


template <class T>
void Menu<T>::shift(double dx,double dy)
{
    x0+=dx;
    y0+=dy;
    x1+=dx;
    y1+=dy;
    for(auto &i:buttons) i->shift(dx,dy);
}









template <class T>
Panel<T>::Panel(double x_0, double y_0, double x_1, bool on, vector<button_template<T> > but, vector<text_inp_template<T> > tex)
{
    double pos_x = x_0+h_margin;
    double pos_y = y_0+v_margin;

    for(auto &i:but)
    {
        buttons.push_back(new Button<T>(pos_x,pos_y,font_size,font_file_name,i,margin,space,corner_r,button_c,p_button_c,margin_c,p_margin_c,font_c,p_font_c));
        pos_x+=buttons.back()->get_x1()-buttons.back()->get_x0();
    }

    typename Text_input<T>::graphics g;
    g.margin = margin;
    g.corner_r = corner_r;
    g.box_margin = margin;
    g.box_corner_r = corner_r;
    g.space = space;
    g.font_size = font_size;

    g.backgr_c = button_c;
	g.p_backgr_c = p_button_c;
    g.margin_c = margin_c;
    g.p_margin_c = margin_c;
    g.box_backgr_c = inp_box_backgr_c;
    g.title_c = font_c;
    g.p_title_c = p_font_c;
    g.inp_c = inp_font_c;
    g.bar_c = inp_bar_c;

    FONT * font = load_ttf_font(font_file_name,font_size,&font);
    for(auto &i:tex)
    {
        text_inputs.push_back(new Text_input<T>(pos_x,pos_y,pos_x+text_inp_width+3*space+al_get_text_width(font,i.title.c_str()),font_file_name,i,g,true,true));
        pos_x+=text_inputs.back()->get_x1()-text_inputs.back()->get_x0();
    }
    attempt_destroy_font(font_file_name,font_size,&font);

    double but_y = 0; double tex_y = 0;
    if(!buttons.empty()) but_y=buttons[0]->get_y1()-buttons[0]->get_y0();
    if(!text_inputs.empty()) tex_y=text_inputs[0]->get_y1()-text_inputs[0]->get_y0();

    x0=x_0;
    y0=y_0;
    x1=x_1;
    y1=y0+2*v_margin+max(but_y,tex_y);
}

template <class T>
void Panel<T>::Resize (double x_0, double y_0, double x_1, double y_1)
{
    double dx = x_0-x0;
    double dy = y_1-y1;
    x0=x_0;
    y0=y_0;
    x1=x_1;
    y1=y_1;
    for(auto& i:buttons) i->shift(dx,dy);
    for(auto& i:text_inputs) i->shift(dx,dy);
}

template <class T>
response_type Panel<T>::respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in, pair<char, char> input)
{
    if(!buttons_active && !text_inputs_active)
    {
        if(mouse_inside(mouse_x,mouse_y))
        {
            if(evt==event_type::lmb_down)
            {
                for(auto &i:buttons)
                {
                    if(i->mouse_inside(mouse_x,mouse_y))
                    {
                        buttons_active = true;
                        lock = in;
                        return i->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock,hold_event,in,input);
                    }
                }
                if(!buttons_active)
                {
                    for(int i=0;i<(int)text_inputs.size();i++)
                    {
                        response_type r = text_inputs[i]->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock,hold_event,in,input);
                        if(lock!=-1)
                        {
                            text_inp_lock = i;
                            text_inputs_active = true;
                            return r;
                        }
                    }
                }
            }
        }
    }
    else if(buttons_active)
    {
        response_type r = response_type::do_nothing;
        int response_index = 0;
        vector<int> indeces;
        for(auto &i:buttons)
        {
            r = i->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock,hold_event,in,input);
            if(r==response_type::at_least_inside) indeces.push_back(response_index);
            response_index++;
        }
        if(indeces.size()>0) r = response_type::at_least_inside;
        if(indeces.size()==1) prev_enabled_button = indeces[0];
        else if(indeces.size()>1) 
        {
            indeces.resize(1);
            indeces[0] = prev_enabled_button;
        }

        if(evt==event_type::lmb_up)
        {
            if(r==response_type::do_nothing) lock = -1;
            if(lock==-1)
            {
                for(auto& i:buttons) i->unhighlight();
                buttons_active=false;
            }
        }
        if(r!=response_type::do_nothing) for(int i=0;i<(int)buttons.size();i++) if(i!=indeces[0]) buttons[i]->unhighlight();
        return r;
    }
    else if(text_inputs_active)
    {
        response_type r = text_inputs[text_inp_lock]->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock,hold_event,in,input);
        if(lock!=-1)
        {
            return r;
        }
        text_inputs_active=false;
        text_inp_lock = -1;
        return r;
    }
    return response_type::do_nothing;
}

template <class T>
double Panel<T>::get_min_x_dim()
{
    double tot_w = 0;
    for(auto& i:buttons) tot_w+=i->get_x1()-i->get_x0();
    for(auto& i:text_inputs) tot_w+=i->get_x1()-i->get_x0();
    return tot_w+2*h_margin;
}

template <class T>
double Panel<T>::get_min_y_dim()
{
    double button_h = 0, t_input_h = 0;
    if(!buttons.empty()) button_h = buttons[0]->get_y1()-buttons[0]->get_y0();
    if(!text_inputs.empty()) t_input_h = text_inputs[0]->get_y1()-text_inputs[0]->get_y0();
    return max(button_h,t_input_h)+2*v_margin;
}

template <class T>
void Panel<T>::draw(ALLEGRO_DISPLAY * disp)
{
    al_set_target_backbuffer(disp);
    al_draw_filled_rectangle(x0,y0,x1,y1,background_c);
    al_draw_filled_rectangle(x0,y0,x0+scaling*h_margin,y1,panel_margin_c);
    al_draw_filled_rectangle(x1-scaling*h_margin,y0,x1,y1,panel_margin_c);
    al_draw_filled_rectangle(x0,y0,x1,y0+scaling*v_margin,panel_margin_c);
    al_draw_filled_rectangle(x0,y1-scaling*v_margin,x1,y1,panel_margin_c);

    for(auto& i:buttons) i->draw(disp);
    for(auto& i:text_inputs) i->draw(disp);
}

template<class T>
void Panel<T>::set_scaling(double factor)
{
    scaling = factor; 

    // First resize all the elements appropriately 
    for(auto& i:buttons) i->set_scaling(scaling);
    for(auto& i:text_inputs) i->set_scaling(scaling);

    // Then shift them in place accordingly
    double pos_x = x0+h_margin*scaling;
    double pos_y = y0+v_margin*scaling;

    for(auto &i:buttons) 
    {
        i->shift(pos_x-i->get_x0(),pos_y-i->get_y0());
        pos_x+=i->get_x1()-i->get_x0();
    }
    for(auto &i:text_inputs) 
    {
        i->shift(pos_x-i->get_x0(),pos_y-i->get_y0());
        pos_x+=i->get_x1()-i->get_x0();
    }

    double but_y = 0; double tex_y = 0;
    if(!buttons.empty()) but_y=buttons[0]->get_y1()-buttons[0]->get_y0();
    if(!text_inputs.empty()) tex_y=text_inputs[0]->get_y1()-text_inputs[0]->get_y0();

    y1=y0+2*scaling*v_margin+max(but_y,tex_y);
}

Resize_bar::Resize_bar(vector<Area *> areas,vector<int> dims) : areas(areas), dims(dims)
{
    if(areas.size()!=dims.size()) printf("\n\nERROR!!! In Resize_bar::Resize_bar the dims and areas sizes are not equal!! \n\n");
    else fit();
}

response_type Resize_bar::respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in, pair<char, char> input)
{
    if(mouse_inside(mouse_x,mouse_y) || lock==in)
    {
        if(evt==event_type::lmb_down)
        {
            prev_mouse_x=mouse_x;
            prev_mouse_y=mouse_y;
            lock=in;
            hold_event = event_type::lmb_down;
        }
        else if(evt==event_type::move_mouse)
        {
            if(lock==in && hold_event==event_type::lmb_down)
            {
                if(ver_or_hor)
                {
                    double dx = mouse_x-prev_mouse_x;
                    /// get distance you are allowed to move into the different areas
                    vector<double> max_dx (areas.size(),0);
                    for(int i=0;i<(int)areas.size();i++)
                    {
                        if(dims[i]==0) max_dx[i] = areas[i]->get_x1()-areas[i]->get_x0()-areas[i]->get_min_x_dim();
                        else max_dx[i] = -1.0*(areas[i]->get_x1()-areas[i]->get_x0()-areas[i]->get_min_x_dim());
                    }
                    for(int i=0;i<(int)areas.size();i++)
                    {
                        if(dims[i]==0)
                        {
                            if(dx>max_dx[i]) dx = max_dx[i];
                        }
                        else if(dx<max_dx[i]) dx = max_dx[i];
                    }
                    /// Move bar and areas accordingly
                    x0+=dx;
                    x1+=dx;
                    for(int i=0;i<(int)areas.size();i++)
                    {
                        if(dims[i]==0) areas[i]->Resize(0.5*(x0+x1),areas[i]->get_y0(),areas[i]->get_x1(),areas[i]->get_y1());
                        else areas[i]->Resize(areas[i]->get_x0(),areas[i]->get_y0(),0.5*(x0+x1),areas[i]->get_y1());
                    }
                }
                else
                {
                    double dy = mouse_y-prev_mouse_y;
                    /// get distance you are allowed to move into the different areas
                    vector<double> max_dy (areas.size(),0);
                    for(int i=0;i<(int)areas.size();i++)
                    {
                        if(dims[i]==1) max_dy[i] = areas[i]->get_y1()-areas[i]->get_y0()-areas[i]->get_min_y_dim();
                        else max_dy[i] = -1.0*(areas[i]->get_y1()-areas[i]->get_y0()-areas[i]->get_min_y_dim());
                    }
                    for(int i=0;i<(int)areas.size();i++)
                    {
                        if(dims[i]==1)
                        {
                            if(dy>max_dy[i]) dy = max_dy[i];
                        }
                        else if(dy<max_dy[i]) dy = max_dy[i];
                    }
                    /// Move bar and areas accordingly
                    y0+=dy;
                    y1+=dy;
                    for(int i=0;i<(int)areas.size();i++)
                    {
                        if(dims[i]==1) areas[i]->Resize(areas[i]->get_x0(),0.5*(y0+y1),areas[i]->get_x1(),areas[i]->get_y1());
                        else areas[i]->Resize(areas[i]->get_x0(),areas[i]->get_y0(),areas[i]->get_x1(),0.5*(y0+y1));
                    }
                }

                prev_mouse_x=mouse_x;
                prev_mouse_y=mouse_y;
                return (ver_or_hor) ? response_type::ver_area_resize : response_type::hor_area_resize;
            }
        }
        else if(evt==event_type::lmb_up)
        {
            if(lock==in && hold_event==event_type::lmb_down)
            {
                if(ver_or_hor)
                {
                    double dx = mouse_x-prev_mouse_x;
                    /// get distance you are allowed to move into the different areas
                    vector<double> max_dx (areas.size(),0);
                    for(int i=0;i<(int)areas.size();i++)
                    {
                        if(dims[i]==0) max_dx[i] = areas[i]->get_x1()-areas[i]->get_x0()-areas[i]->get_min_x_dim();
                        else max_dx[i] = -1.0*(areas[i]->get_x1()-areas[i]->get_x0()-areas[i]->get_min_x_dim());
                    }
                    for(int i=0;i<(int)areas.size();i++)
                    {
                        if(dims[i]==0)
                        {
                            if(dx>max_dx[i]) dx = max_dx[i];
                        }
                        else if(dx<max_dx[i]) dx = max_dx[i];
                    }
                    /// Move bar and areas accordingly
                    x0+=dx;
                    x1+=dx;
                    for(int i=0;i<(int)areas.size();i++)
                    {
                        if(dims[i]==0) areas[i]->Resize(0.5*(x0+x1),areas[i]->get_y0(),areas[i]->get_x1(),areas[i]->get_y1());
                        else areas[i]->Resize(areas[i]->get_x0(),areas[i]->get_y0(),0.5*(x0+x1),areas[i]->get_y1());
                    }
                }
                else
                {
                    double dy = mouse_y-prev_mouse_y;
                    /// get distance you are allowed to move into the different areas
                    vector<double> max_dy (areas.size(),0);
                    for(int i=0;i<(int)areas.size();i++)
                    {
                        if(dims[i]==1) max_dy[i] = areas[i]->get_y1()-areas[i]->get_y0()-areas[i]->get_min_y_dim();
                        else max_dy[i] = -1.0*(areas[i]->get_y1()-areas[i]->get_y0()-areas[i]->get_min_y_dim());
                    }
                    for(int i=0;i<(int)areas.size();i++)
                    {
                        if(dims[i]==1)
                        {
                            if(dy>max_dy[i]) dy = max_dy[i];
                        }
                        else if(dy<max_dy[i]) dy = max_dy[i];
                    }
                    /// Move bar and areas accordingly
                    y0+=dy;
                    y1+=dy;
                    for(int i=0;i<(int)areas.size();i++)
                    {
                        if(dims[i]==1) areas[i]->Resize(areas[i]->get_x0(),0.5*(y0+y1),areas[i]->get_x1(),areas[i]->get_y1());
                        else areas[i]->Resize(areas[i]->get_x0(),areas[i]->get_y0(),areas[i]->get_x1(),0.5*(y0+y1));
                    }
                }

                prev_mouse_x=mouse_x;
                prev_mouse_y=mouse_y;
                lock=-1;
                hold_event=event_type::idle;

                return (ver_or_hor) ? response_type::ver_area_resize : response_type::hor_area_resize;
            }
        }
        return (ver_or_hor) ? response_type::ver_area_resize_hov : response_type::hor_area_resize_hov;
    }
    return response_type::do_nothing;
}

void Resize_bar::fit()
{
    bool check_equal = true;
    auto get_dim = [] (int d, Area * a)
    {
        if(d==0) return a->get_x0();
        else if(d==1) return a->get_y0();
        else if(d==2) return a->get_x1();
        else if(d==3) return a->get_y1();
        else printf("\n\nError! Wrong index in get_prim (Resize_bar::Resize_bar)!\n\n");
        return -1.0;
    };

    if(areas.size()>1)
    {
        for(int i=1;i<(int)dims.size();i++)
        {
            if(dims[i-1]%2!=dims[i]%2 || get_dim(dims[i-1],areas[i-1])!=get_dim(dims[i],areas[i])) {check_equal=false; break;}
        }
    }
    if(check_equal)
    {
        double val = get_dim(dims[0],areas[0]);
        if(dims[0]%2==0) /// vertical
        {
            ver_or_hor=true;
            vector<double> y0s;
            vector<double> y1s;
            for(int i=0;i<(int)dims.size();i++)
            {
                y0s.push_back(get_dim(1,areas[i]));
                y1s.push_back(get_dim(3,areas[i]));
            }
            x0 = val-0.5*width*scaling;
            y0 = *min_element(y0s.begin(),y0s.end());
            x1 = val+0.5*width*scaling;
            y1 = *max_element(y1s.begin(),y1s.end());
        }
        else /// horizontal
        {
            ver_or_hor=false;
            vector<double> x0s;
            vector<double> x1s;
            for(int i=0;i<(int)dims.size();i++)
            {
                x0s.push_back(get_dim(0,areas[i]));
                x1s.push_back(get_dim(2,areas[i]));
            }
            x0 = *min_element(x0s.begin(),x0s.end());
            y0 = val-0.5*width*scaling;
            x1 = *max_element(x1s.begin(),x1s.end());
            y1 = val+0.5*width*scaling;
        }
    }
    else printf("\n\nERROR!!! The dimensions of your Areas in Resize_bar::fit are not equal!!\n\n");
}

void Resize_bar::set_scaling(double factor)
{
    scaling = factor;
}

Custom::Custom(double x0, double y0, double x1, double y1, bool on) : Area(x0,y0,x1,y1,on) {}

void Custom::Resize (double x_0, double y_0, double x_1, double y_1)
{
    x0=x_0;
    y0=y_0;
    x1=x_1;
    y1=y_1;
}

response_type Custom::respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,int &lock,event_type &hold_event,int in, pair<char, char> input)
{
    if(mouse_inside(mouse_x,mouse_y)  || lock==in)
    {
        if(evt==event_type::lmb_down)
        {
            prev_mouse_x=mouse_x;
            prev_mouse_y=mouse_y;
            lock=in;
            hold_event = event_type::lmb_down;
            return response_type::lmb_down_custom;
        }
        else if(evt==event_type::lmb_up)
        {
            if(lock==in && hold_event==event_type::lmb_down)
            {
                lock=-1;
                hold_event=event_type::idle;
                return response_type::lmb_up_custom;
            }
        }
        else if(evt==event_type::rmb_down)
        {
            prev_mouse_x=mouse_x;
            prev_mouse_y=mouse_y;
            lock=in;
            hold_event = event_type::rmb_down;
            return response_type::rmb_down_custom;
        }
        else if(evt==event_type::rmb_up)
        {
            if(lock==in && hold_event==event_type::rmb_down)
            {
                lock=-1;
                hold_event=event_type::idle;
                return response_type::rmb_up_custom;
            }
        }
        else if(evt==event_type::move_mouse)
        {
            if(lock==in && hold_event==event_type::rmb_down)
            {
                return response_type::rmb_move_mouse;
            }
            else if(lock==in && hold_event==event_type::lmb_down)
            {
                return response_type::lmb_move_mouse;
            }
            else return response_type::cust_move_mouse;
        }
        else if (evt == event_type::scroll) return response_type::zoom;
		else if(input.first=='\t') return response_type::tab_key;
    }
    return response_type::do_nothing;
}

void Custom::draw(ALLEGRO_DISPLAY * disp)
{
    al_set_target_backbuffer(disp);
    al_draw_filled_rectangle(x0,y0,x0+h_margin*scaling,y1,margin_c);
    al_draw_filled_rectangle(x1-h_margin*scaling,y0,x1,y1,margin_c);
    al_draw_filled_rectangle(x0,y0,x1,y0+v_margin*scaling,margin_c);
    al_draw_filled_rectangle(x0,y1-v_margin*scaling,x1,y1,margin_c);
}

void Custom::set_scaling(double factor) { scaling = factor; };

template <class T>
Window<T>::Window(ALLEGRO_DISPLAY* disp,vector<button_template<T> > buttons,vector<text_inp_template<T> > text_inputs, vector<string> ititles, vector<inp_view_template<T> > itemplates) : disp(disp)
{
    double d_width = al_get_display_width(disp);
    double d_height = al_get_display_height(disp);
    size_x = d_width;
    size_y = d_height;
    vector<string> str;
    areas.push_back(new Panel<T>(0,0,d_width,true,buttons,text_inputs));
    panel = static_cast<Panel<T> *>(areas.back());
    panel_h = panel->get_y1()-panel->get_y0();
    
    areas.push_back(new Text_view<T>(rat_x*d_width,panel_h,d_width,d_height-l_margin,true,str));
    t_view = static_cast<Text_view<T> *>(areas.back());

	areas.push_back(new Input_view<T>(rat_x*d_width, panel_h, d_width, d_height - l_margin, false, ititles[0], itemplates[0]));
	espec_view = static_cast<Input_view<T> *>(areas.back());
	areas.push_back(new Input_view<T>(rat_x*d_width, panel_h, d_width, d_height - l_margin, false, ititles[1], itemplates[1]));
	ereac_view = static_cast<Input_view<T> *>(areas.back());
	areas.push_back(new Input_view<T>(rat_x*d_width, panel_h, d_width, d_height - l_margin, false, ititles[2], itemplates[2]));
	ecompart_view = static_cast<Input_view<T> *>(areas.back());
	areas.push_back(new Input_view<T>(rat_x*d_width, panel_h, d_width, d_height - l_margin, false, ititles[3], itemplates[3]));
	aspec_view = static_cast<Input_view<T> *>(areas.back());
	areas.push_back(new Input_view<T>(rat_x*d_width, panel_h, d_width, d_height - l_margin, false, ititles[4], itemplates[4]));
	areac_view = static_cast<Input_view<T> *>(areas.back());
	areas.push_back(new Input_view<T>(rat_x*d_width, panel_h, d_width, d_height - l_margin, false, ititles[5], itemplates[5]));
	acompart_view = static_cast<Input_view<T> *>(areas.back());


    areas.push_back(new Custom(0,panel_h,rat_x*d_width,d_height-l_margin,true));
    custom = static_cast<Custom *>(areas.back());

    vector<Area *> asv (areas.begin()+1,areas.begin()+3+itemplates.size());
	vector<int> dsv;
	for (size_t i = 0; i < (itemplates.size() + 1); i++) dsv.push_back(0);
	dsv.push_back(2);

    areas.push_back(new Resize_bar(asv,dsv));
    v_res = static_cast<Resize_bar *>(areas.back());

	set_mode(neighbour_info);
}

template <class T>
void Window<T>::Resize(ALLEGRO_DISPLAY* disp)
{
    double split_x = custom->get_x1()/size_x;
    double split_y = custom->get_y0()/size_y;
    double d_width = al_get_display_width(disp);
    double d_height = al_get_display_height(disp);
    size_x = d_width;
    size_y = d_height;
	for (int i = 1; i < 8; i++) areas[i]->Resize(split_x*d_width, panel_h, d_width, d_height - l_margin);
    custom->Resize(0,panel_h,split_x*d_width,d_height-l_margin);
    panel->Resize(0,0,d_width,panel_h);
    v_res->fit();
}

template <class T>
response_type Window<T>::respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz, pair<char, char> input)
{
    if(evt==event_type::m_leave_disp) {mouse_x=-1; mouse_y=-1; return response_type::do_nothing;} // if went outside, unlock
    if(lock==-1) // if unlocked
    {
        for(int i=(int)areas.size()-1;i>=0;i--)
        {
			if (areas[i]->is_on())
			{
				response_type r = areas[i]->respond_to_event(disp, evt, mouse_x, mouse_y, mouse_dz, lock, hold_event, i, input);
				if (r != response_type::do_nothing) return r;
			}
        }
        return response_type::do_nothing;
    }
    else // if locked
    {
        return areas[lock]->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock,hold_event,lock,input);
    }
}

template <class T>
void Window<T>::clear_current_mode()
{
	if (mode == edit_spec) espec_view->cleanup();
	else if (mode == edit_reac) ereac_view->cleanup();
	else if (mode == edit_compart) ecompart_view->cleanup();
	else if (mode == add_spec) aspec_view->cleanup();
	else if (mode == add_reac) areac_view->cleanup();
	else if (mode == add_compart) acompart_view->cleanup();
}

template <class T>
void Window<T>::set_mode(side_pane s, vector<int> sizes)
{
	clear_current_mode();
	for (int i = 1; i < 8; i++) areas[i]->turn_off();
	mode = s;
	if (mode == neighbour_info) t_view->turn_on();
	else if (mode == edit_spec) 
	{
		espec_view->turn_on(); 
		espec_view->initialize(sizes);
	}
	else if (mode == edit_reac)
	{
		ereac_view->turn_on();
		ereac_view->initialize(sizes);
	}
	else if (mode == edit_compart)
	{
		ecompart_view->turn_on();
		ecompart_view->initialize(sizes);
	}
	else if (mode == add_spec) 
	{
		aspec_view->turn_on();
		aspec_view->initialize(sizes);
	}
	else if (mode == add_reac)
	{
		areac_view->turn_on();
		areac_view->initialize(sizes);
	}
	else if (mode == add_compart)
	{
		acompart_view->turn_on();
		acompart_view->initialize(sizes);
	}
}

template <class T>
bool Window<T>::apply_current_mode()
{
	if (mode == edit_spec) return espec_view->apply();
	else if (mode == edit_reac) return ereac_view->apply();
	else if (mode == edit_compart) return ecompart_view->apply();
	else if (mode == add_spec) return aspec_view->apply();
	else if (mode == add_reac) return areac_view->apply();
	else return acompart_view->apply();
}

template <class T>
void Window<T>::draw()
{
    al_set_target_backbuffer(disp);
    for(size_t i=1;i<areas.size();i++) if(areas[i]->is_on()) areas[i]->draw(disp);
    areas[0]->draw(disp);
    al_draw_filled_rectangle(0,size_y-l_margin*scaling,size_x,size_y,margin_c);
}

template <class T>
void Window<T>::set_scaling(double factor)
{
    scaling = factor;
    for(auto &i:areas) i->set_scaling(scaling);
    panel_h = panel->get_y1()-panel->get_y0();

    custom->Resize(custom->get_x0(),panel_h,custom->get_x1(),size_y-l_margin*scaling);
    t_view->Resize(t_view->get_x0(),panel_h,t_view->get_x1(),size_y-l_margin*scaling);
    espec_view->Resize(espec_view->get_x0(),panel_h,espec_view->get_x1(),size_y-l_margin*scaling);
    ereac_view->Resize(ereac_view->get_x0(),panel_h,ereac_view->get_x1(),size_y-l_margin*scaling);
    ecompart_view->Resize(ecompart_view->get_x0(),panel_h,ecompart_view->get_x1(),size_y-l_margin*scaling);
    aspec_view->Resize(aspec_view->get_x0(),panel_h,aspec_view->get_x1(),size_y-l_margin*scaling);
    areac_view->Resize(areac_view->get_x0(),panel_h,areac_view->get_x1(),size_y-l_margin*scaling);
    acompart_view->Resize(acompart_view->get_x0(),panel_h,acompart_view->get_x1(),size_y-l_margin*scaling);
}


Filehandler::Filehandler(ALLEGRO_DISPLAY * disp, mode mod, action * act, string * file_name, double scaling) : mod(mod), act(act), file_name(file_name), disp(disp), scaling(scaling)
{
    *act = none;
    d_width = al_get_display_width(disp);
    double d_height = al_get_display_height(disp);
    double input_y0 = 0;
    double button_y1 = d_height;
    double button_split_x;

    string button_name = (mod==open) ? "Open file" : (mod==save_as) ? "Save file as" : "Random shit";
    int n = (mod==open) ? 1 : (mod==save_as) ? 2 : 3;
    button_template<Filehandler> doit_templ(button_type::click,&Filehandler::do_file_action,this,button_name,n);
    areas.push_back(new Button<Filehandler>(d_width,d_height,scaling*font_size,font_file_name,doit_templ,scaling*but_margin,scaling*but_space,scaling*but_corner_r,button_c,p_button_c,margin_c,p_margin_c,font_c,p_font_c));
    doit = static_cast<Button<Filehandler> *>(areas.back());

    button_y0 = button_y1 - 1.62*(doit->get_y1()-doit->get_y0());
    text_y1 = button_y0;
    double delta = ((doit->get_y1()-doit->get_y0()))*0.31;
    double dy = -((doit->get_y1()-doit->get_y0())) - delta;
    double dx = -((doit->get_x1()-doit->get_x0())) - delta;
    button_split_x = d_width - ((doit->get_x1()-doit->get_x0())) - delta;
    doit->shift(dx,dy);

    button_template<Filehandler> canc_templ(button_type::click,&Filehandler::do_file_action,this,"Cancel",-1);
    areas.push_back(new Button<Filehandler>(button_split_x,d_height,scaling*font_size,font_file_name,canc_templ,scaling*but_margin,scaling*but_space,scaling*but_corner_r,button_c,p_button_c,margin_c,p_margin_c,font_c,p_font_c));
    canc = static_cast<Button<Filehandler> *>(areas.back());
    dx = -(canc->get_x1()-canc->get_x0()) - delta;
    dy = -(canc->get_y1()-canc->get_y0()) - delta;
    canc->shift(dx,dy);

    text_inp_template<Filehandler> inp_templ(&Filehandler::receive_text,this,"Name:",1);

    Text_input<Filehandler>::graphics g;
    g.margin = scaling*inp_margin;
    g.corner_r = scaling*0;
    g.box_margin = scaling*inp_margin;
    g.box_corner_r = scaling*inp_corner_r;
    g.space = scaling*inp_space;
    g.font_size = scaling*font_size;

    g.backgr_c = button_c;
	g.p_backgr_c = p_button_c;
    g.margin_c = margin_c;
    g.p_margin_c = margin_c;
    g.box_backgr_c = inp_box_backgr_c;
    g.title_c = font_c;
    g.p_title_c = p_font_c;
    g.inp_c = inp_font_c;
    g.bar_c = inp_bar_c;


    areas.push_back(new Text_input<Filehandler>(0,input_y0,d_width,font_file_name,inp_templ,g,false,true));
    input = static_cast<Text_input<Filehandler> *>(areas.back());

    text_y0 = input->get_y1();

    /// Add directory information

    current_path = boost::filesystem::current_path().string();
	change_folder(current_path);

}

void Filehandler::change_folder(string folder_path)
{
	files.clear();
	folders.clear();
	relevant.clear();

	try
	{
		using namespace boost::filesystem;

		boost::filesystem::path p0(folder_path);
		boost::filesystem::path p = canonical(p0);
		current_path = p.string();

		if (exists(p) && is_directory(p))
		{
			for (auto &x: directory_iterator(p))
			{
				if (is_directory(x.path())) 
                {
                    string s = x.path().filename().string();
                    folders.push_back(s);
                }
				else if (is_regular_file(x.path())) 
                {
                    string s = x.path().filename().string();
                    files.push_back(s);
                }
			}

			sort(folders.begin(), folders.end());
			sort(files.begin(), files.end());

			for (string &s : files)
			{
				if (extension(&s[0]) == ".xml" || extension(&s[0]) == ".sbml") relevant.push_back(true);
				else relevant.push_back(false);
			}
		}
	}
	catch (const boost::filesystem::filesystem_error& ex)
	{
		printf("\nERROR: %s\n", ex.what());
	}

	full_list.clear();
	full_list.push_back("..");
	full_list.insert(full_list.end(), folders.begin(), folders.end());
	full_list.insert(full_list.end(), files.begin(), files.end());

	vector<string> str;
	str.push_back("$t" + current_path);

	auto col_to_str = [](COL col)
	{
		unsigned char R, G, B;
		string r, g, b;
		al_unmap_rgb(col, &R, &G, &B);
		r = to_string((int)R);
		g = to_string((int)G);
		b = to_string((int)B);
		return "{" + r + ";" + g + ";" + b + "}";
	};

	str.push_back("$" + col_to_str(folder_c) + ".."); // up one folder
	str.push_back("\t");

	for (string &s : folders) { str.push_back("$" + col_to_str(folder_c) + s); str.push_back("\t"); }
	for (int i = 0; i<(int)files.size(); i++)
	{
		if (relevant[i]) str.push_back("$" + col_to_str(rel_file_c) + files[i]);
		else str.push_back("$" + col_to_str(file_c) + files[i]);
		str.push_back("\t");
	}
	str.pop_back();

    if(t_view==NULL)
    {
        areas.push_back(new Text_view<Filehandler>(0, text_y0, d_width, text_y1, true, str, scaling*font_size, scaling*font_size, scaling*text_v_margin, scaling*text_h_margin, margin_c,-1.0,&Filehandler::click_folder,this,0));
        t_view = static_cast<Text_view<Filehandler> *>(areas.back());
    }
    else t_view->Change_text(str);
}

void Filehandler::click_folder(int n, vector<string> &s, int select, bool &new_text)
{
	boost::filesystem::path p(full_list[select]);
	string f_name;
	if (!p.is_absolute()) f_name = (boost::filesystem::path(get_current_path()) / p).string();
	bool exists = boost::filesystem::exists(f_name);
	bool is_directory = boost::filesystem::is_directory(f_name);
	if (exists)
	{
		if(is_directory) 
        {
            change_folder(f_name);
        }
		else if (relevant[select - folders.size() - 1])
		{
			*file_name = f_name;
			*act = do_it;
		}
	}
}

response_type Filehandler::respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz,pair<char,char> input)
{
    if(evt==event_type::m_leave_disp) {mouse_x=-1; mouse_y=-1; return response_type::do_nothing;}
    if(lock==-1) // if unlocked
    {
        string reply;
        if(evt==event_type::keyboard && input.first == '\r' && input.second == 'n') do_file_action(-2,reply);
        for(int i=(int)areas.size()-1;i>=0;i--)
        {
            response_type r = areas[i]->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock,hold_event,i,input);
            if(r!=response_type::do_nothing) return r;
        }
        return response_type::do_nothing;
    }
    else // if locked
    {
        return areas[lock]->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock,hold_event,lock,input);
    }
}

void Filehandler::draw()
{
    al_set_target_backbuffer(disp);
    al_draw_filled_rectangle(0,button_y0,al_get_display_width(disp),al_get_display_height(disp),button_c);
    al_draw_rectangle(0,button_y0,al_get_display_width(disp),al_get_display_height(disp),margin_c,inp_margin);
    for(auto& a:areas) a->draw(disp);
}

void Filehandler::do_file_action(int n, string &s)
{
    if(n==-1) *act = cancel;
    else
    {
        *file_name = input->get_text();
        *act = do_it;
    }
}

void Filehandler::receive_text(int n, string &s)
{
	if (!s.empty())
	{
		if (n == 1 && s[0] != '\r' && s[0]!='\b')
		{
			*file_name = s;
			*act = do_it;
		}
		else if (n == -1 && s[0] != '\r' && s[0] != '\b')
		{
			try
			{
				string fname;
				boost::filesystem::path p0(s);
				boost::filesystem::path dirpath0 = p0.parent_path();
				if (p0.has_filename())
				{
					boost::filesystem::path filename0 = p0.filename();
					if (!p0.is_absolute()) fname = (boost::filesystem::path(get_current_path()) / boost::filesystem::path(s)).string();
					boost::filesystem::path p(fname);
					boost::filesystem::path dirpath = canonical(p.parent_path());

					using namespace boost::filesystem;

					if (exists(dirpath) && is_directory(dirpath))
					{
						vector<string> content;
						for (auto& x : directory_iterator(dirpath))
						{
							content.push_back(x.path().filename().string());
						}
						sort(content.begin(), content.end());

						vector<string> matches;
						string filenames = filename0.string();
						for (string &s : content)
						{
							if (s.substr(0, filenames.size()) == filenames)
							{
								matches.push_back(s);
							}
						}
						if (matches.size() > 0)
						{
							size_t sssize = 10000;
							for (string &s : matches) sssize = min(sssize, s.size());
							string newfilename = filenames;
							for (size_t i = filenames.size(); i < sssize; i++)
							{
								bool allalike = true;
								for (size_t j = 0; j < matches.size(); j++) if (matches[0][i] != matches[j][i]) allalike = false;
								if (allalike) newfilename += matches[0][i];
								else break;
							}
							s = (dirpath0 / boost::filesystem::path(newfilename)).string();
						}
					}
				}
			}
			catch (const boost::filesystem::filesystem_error& ex)
			{
				printf("\nERROR: %s\n", ex.what());
			}
		}
	}
}

Warning::Warning(ALLEGRO_EVENT_QUEUE * event_queue, string message, vector<string> b_text, int * action, double scaling) : action(action)
{
    if(b_text.empty()) b_text.push_back("Thou didst not give me buttons, o silly person!");

    *action = -1;
    al_pause_event_queue(event_queue,true);
    al_flush_event_queue(event_queue);

    FONT * font = load_ttf_font(font_file_name,font_size*scaling,&font);

    for(int i=0;i<(int)b_text.size();i++)
    {
        button_template<Warning> templ(button_type::click,&Warning::do_action,this,b_text[i],i);
        buttons.push_back(new Button<Warning>(0,0,but_font_size*scaling,font_file_name,templ,but_margin*scaling,but_space*scaling,but_corner_r*scaling,button_c,p_button_c,margin_c,p_margin_c,font_c,p_font_c));
    }

    /// Decide on margins
    double but_h = buttons[0]->get_y1()-buttons[0]->get_y0();
    double delta = but_h*0.31;
    text_v_space = delta;
    text_h_space = delta;

    /// Find warning height and width
    double but_width = delta;
    for(auto &i:buttons) but_width+=i->get_x1()-i->get_x0()+delta;
    double d_width = max(but_width,2*delta+(double)al_get_text_width(font,message.c_str()));

    int num_lines = 0;
    string temp_mes = message;
    auto line_builder = [] (int line_num, const char *line, int size, void *num_l) -> bool
    {
        (*(int*)num_l)++;
        return true;
    };
    al_do_multiline_text(font,d_width-2*text_h_space,temp_mes.c_str(),line_builder,&num_lines);

    double d_height = but_h*1.62 + num_lines*al_get_font_line_height(font)*line_spacing + text_v_space;

    double dx = d_width;
    double dy = d_height - but_h - delta;
    for(int i=0;i<(int)buttons.size();i++)
    {
        dx -= delta + (buttons[i]->get_x1()-buttons[i]->get_x0());
        buttons[i]->shift(dx,dy);
    }

    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 2, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 4, ALLEGRO_SUGGEST);

    int dis_num = al_get_num_video_adapters();
    ALLEGRO_MONITOR_INFO inf;
    al_get_monitor_info(dis_num-1, &inf);

    al_set_new_window_position(0.5*((inf.x2-inf.x1)-d_width),0.5*((inf.y2-inf.y1)-d_height));
    al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_OPENGL);

    al_set_new_window_title("Warning");
    disp = al_create_display(d_width,d_height);
    if(!disp) fprintf(stderr, "failed to create file display!\n");
    al_set_target_bitmap(al_get_backbuffer(disp));

    BITMAP * textmap = al_create_bitmap(d_width-2*text_h_space,num_lines*al_get_font_line_height(font)*line_spacing);
    al_set_target_bitmap(textmap);
    al_clear_to_color(al_map_rgb(255,255,255));
    al_draw_multiline_text(font,warn_c,0,0,d_width-2*text_h_space,line_spacing*al_get_font_line_height(font),ALLEGRO_ALIGN_LEFT,message.c_str());

    ALLEGRO_TIMER * tim = al_create_timer(1.0 / FPS);
    if(!tim) fprintf(stderr, "failed to create timer!\n");
    ALLEGRO_EVENT_QUEUE * ev_q = al_create_event_queue();
    if(!ev_q) fprintf(stderr, "failed to create event queue!\n");

    al_set_target_bitmap(al_get_backbuffer(disp));

    al_register_event_source(ev_q, al_get_display_event_source(disp));
    al_register_event_source(ev_q, al_get_timer_event_source(tim));
    al_register_event_source(ev_q, al_get_mouse_event_source());
    al_register_event_source(ev_q, al_get_keyboard_event_source());
    al_start_timer(tim);

    bool redraw = true;
    queue<event_type> events;
    queue<pair<char,char>> key_inps;
    double m_x = -1;
    double m_y = -1;
    double m_z = 0;

    while(*action==-1)
    {
        ALLEGRO_EVENT ev;
        al_wait_for_event(ev_q, &ev);

        if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) break;
        if(ev.type == ALLEGRO_EVENT_MOUSE_AXES || ev.type == ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY)
        {
            redraw = true;
            m_x = (double)ev.mouse.x;
            m_y = (double)ev.mouse.y;
            events.push(event_type::move_mouse);
            if(!ev.mouse.dz==0)
            {
                m_z = ev.mouse.dz;
                events.push(event_type::scroll);
            }
        }
        if (ev.type == ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY)
        {
            redraw = true;
            events.push(event_type::m_leave_disp);
        }
        if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)
        {
            redraw = true;
            if(ev.mouse.button==1) events.push(event_type::lmb_down);
            else if(ev.mouse.button==2) events.push(event_type::rmb_down);
        }
        if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP)
        {
            redraw = true;
            if(ev.mouse.button==1) events.push(event_type::lmb_up);
            else if(ev.mouse.button==2) events.push(event_type::rmb_up);
        }
        if(ev.type == ALLEGRO_EVENT_KEY_CHAR)
        {
            redraw = true;
            if(ev.keyboard.keycode==ALLEGRO_KEY_ESCAPE) break;
            else if(ev.keyboard.keycode==ALLEGRO_KEY_ENTER)
            {
                *action = 1;
                break;
            }
            else
            {
				char mod = 'n';
				if (ev.keyboard.modifiers == ALLEGRO_KEYMOD_CTRL) mod = 'c';
				else if (ev.keyboard.modifiers == ALLEGRO_KEYMOD_SHIFT) mod = 's';
                events.push(event_type::keyboard);
                key_inps.push(pair<char,char>(ev.keyboard.unichar,mod));
            }
        }

        if(redraw && al_is_event_queue_empty(ev_q))
        {
            al_set_target_bitmap(al_get_backbuffer(disp));
            al_clear_to_color(al_map_rgb(255,255,255));

            while(!events.empty())
            {
                if(*action!=-1) break;
                respond_to_event(disp,events.front(),m_x,m_y,m_z,(events.front()==event_type::keyboard) ? key_inps.front() : pair<char,char>('\0','n'));
                if(events.front()==event_type::keyboard) key_inps.pop();
                events.pop();
                ///respond
            }

            al_draw_bitmap(textmap,text_h_space,text_v_space,0);
            draw();

            al_flip_display();
            redraw=false;
        }
    }

    attempt_destroy_font(font_file_name,font_size,&font);
    if(textmap!=NULL) al_destroy_bitmap(textmap);
    if(disp!=NULL) al_destroy_display(disp);
    if(tim!=NULL) al_destroy_timer(tim);
    if(ev_q!=NULL) al_destroy_event_queue(ev_q);

    al_pause_event_queue(event_queue,false);
}

response_type Warning::respond_to_event(ALLEGRO_DISPLAY * disp, event_type evt,double mouse_x,double mouse_y,double mouse_dz, pair<char, char> input)
{
    if(evt==event_type::m_leave_disp) {mouse_x=-1; mouse_y=-1; return response_type::do_nothing;}
    if(lock==-1) // if unlocked
    {
        string reply;
        if(evt==event_type::keyboard && input.first == '\r' && input.second == 'n') do_action(0,reply);
        for(int i=(int)buttons.size()-1;i>=0;i--)
        {
            response_type r = buttons[i]->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock,hold_event,i,input);
            if(r!=response_type::do_nothing) return r;
        }
        return response_type::do_nothing;
    }
    else // if locked
    {
        return buttons[lock]->respond_to_event(disp,evt,mouse_x,mouse_y,mouse_dz,lock,hold_event,lock,input);
    }

}

void Warning::draw()
{
    al_set_target_backbuffer(disp);
    for(auto& a:buttons) a->draw(disp);
}

void Warning::do_action(int n, string &s)
{
    *action = n;
}


template class Text_view<Explorer>;
template class Text<Explorer>;

