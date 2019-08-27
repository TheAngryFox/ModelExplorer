#include <Explorer.h>

#ifdef _WIN32
#define A_RED ""
#define A_GRE ""
#define A_YEL ""
#define A_BLU ""
#define A_MAG ""
#define A_CYA ""
#define A_RES ""
#else
#define A_RED "\x1b[31m"
#define A_GRE "\x1b[32m"
#define A_YEL "\x1b[33m"
#define A_BLU "\x1b[34m"
#define A_MAG "\x1b[35m"
#define A_CYA "\x1b[36m"
#define A_RES "\x1b[0m"
#endif

Explorer::Explorer(string filename, const set<string> &flags, double res_X, double res_Y) : res_x(res_X), res_y(res_Y), flags(flags)
{
	clear_FBA_model();
	int loaded = -1;
	if (!filename.empty())
	{
		loaded = load_FBA_model(filename);
	}

	al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 2, ALLEGRO_SUGGEST);
	al_set_new_display_option(ALLEGRO_SAMPLES, 4, ALLEGRO_SUGGEST);

	int dis_num = al_get_num_video_adapters();
	al_get_monitor_info(dis_num - 1, &info);
	double height = (info.y2 - info.y1)*res_y;
	double width = (info.x2 - info.x1)*res_x;
	dispMod.width = width;
	dispMod.height = height;

	al_set_new_window_position(0.5*((info.x2 - info.x1) - width), 0.5*((info.y2 - info.y1) - height));
	al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE | ALLEGRO_OPENGL | ALLEGRO_GENERATE_EXPOSE_EVENTS);

	display = al_create_display(width, height);
	disp_map = al_create_bitmap(width, height);
	if (!display) fprintf(stderr, "failed to create display!\n");
	al_set_target_backbuffer(display);

	if (!al_install_mouse()) fprintf(stderr, "failed to initialize the mouse!\n");
	if (!al_install_keyboard()) fprintf(stderr, "failed to initialize the keyboard!\n");
	timer = al_create_timer(1.0 / FPS);
	if (!timer) fprintf(stderr, "failed to create timer!\n");
	event_queue = al_create_event_queue();
	if (!event_queue) fprintf(stderr, "failed to create event queue!\n");

	
	vector<button_template<Explorer> > buttons;
	vector<text_inp_template<Explorer> > text_inputs;

	/// Create the menus for the panel
	/// File loading/saving
	buttons.push_back(button_template<Explorer>(button_type::principal_menu_root, "File"));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::file_action_dialogue, this, "New", 0));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::file_action_dialogue, this, "Open", 1));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::file_action_dialogue, this, "Save", 2));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::file_action_dialogue, this, "Save as", 3));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::file_action_dialogue, this, "Save as SVG", 4));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click,&Explorer::file_action_dialogue,this,"Save blocked modules",5));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click,&Explorer::file_action_dialogue,this,"Save error tracing",6));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::file_action_dialogue, this, "Quit", 7));

	/// Edit - undo/redo , recalculate, replot
	buttons.push_back(button_template<Explorer>(button_type::principal_menu_root, "Edit"));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::edit, this, "Undo", 0));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::edit, this, "Redo", 1));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::edit, this, "Replot", 2));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::edit, this, "Recalc all", 3));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::edit, this, "Recalc FBA", 4));

	/// View only the blocked module or the whole netork
	buttons.push_back(button_template<Explorer>(button_type::principal_menu_root, "View"));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::view, this, "Blocked module", 1));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::view, this, "Whole model", 2));

	/// Add reactions / species / compartments
	buttons.push_back(button_template<Explorer>(button_type::principal_menu_root, "Add"));
	//buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::add_or_edit, this, "Edit selected", 0));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::add_or_edit, this, "Species", 1));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::add_or_edit, this, "Reaction", 2));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::add_or_edit, this, "Compartment", 3));

	/// Modify - Purge disconnected reactions / species, purge boundary and extracellular species
	buttons.push_back(button_template<Explorer>(button_type::principal_menu_root, "Purge"));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::purge, this, "Selected", 5));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::purge, this, "Boundary species", 0));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::purge, this, "Extracellular species", 1));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::purge, this, "Disconneced species", 2));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::purge, this, "Disconneced reactions", 3));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::purge, this, "Disconneced clusters", 4));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::menu_click, &Explorer::purge, this, "Blocked reactions and species", 6));

	/// Dead reactions search
	buttons.push_back(button_template<Explorer>(button_type::principal_menu_root, "Reaction status"));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::switcher, &Explorer::blocking, this, "Always on", 0));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::switcher, &Explorer::blocking, this, "FBA", 1));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::switcher, &Explorer::blocking, this, "Bi-directional", 2));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::switcher, &Explorer::blocking, this, "Dynamic", 3));

	/// Tracking mode switching
	buttons.push_back(button_template<Explorer>(button_type::principal_menu_root, "Neighbour view"));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::switcher, &Explorer::tracking, this, "None", 0));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::switcher, &Explorer::tracking, this, "Ego-centric", 1));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::switcher, &Explorer::tracking, this, "Node ancestry", 2));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::switcher, &Explorer::tracking, this, "Blocked module", 3));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::switcher, &Explorer::tracking, this, "Error Tracer", 4));


	/// Graphics modes
	buttons.push_back(button_template<Explorer>(button_type::principal_menu_root, "Graphics mode"));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::switcher, &Explorer::graphics, this, "High resolution", 1));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::switcher, &Explorer::graphics, this, "Low resolution", 0));

	/// Compartment related commands
	buttons.push_back(button_template<Explorer>(button_type::principal_menu_root, "Compartments"));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::switcher, &Explorer::compartments, this, "Show", 0));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::switcher, &Explorer::compartments, this, "Hide", 1));

	/// Add palette
	buttons.push_back(button_template<Explorer>(button_type::principal_menu_root, "Colours"));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::palette, &Explorer::palettes, this, "Active reactions", 0));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::palette, &Explorer::palettes, this, "Active species", 1));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::palette, &Explorer::palettes, this, "Blocked reactions", 2));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::palette, &Explorer::palettes, this, "Blocked species", 3));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::palette, &Explorer::palettes, this, "Endpoint species", 4));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::palette, &Explorer::palettes, this, "Disjoint species", 5));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::palette, &Explorer::palettes, this, "Compartments", 6));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::palette, &Explorer::palettes, this, "Constraining reactions", 7));
	buttons.back().menu.push_back(button_template<Explorer>(button_type::palette, &Explorer::palettes, this, "Constraining species", 8));

	/// Add a search
	text_inputs.push_back(text_inp_template<Explorer>(&Explorer::search_for_word, this, "Search:", 0, height*0.617, height*0.38));

	/// Add the reaction, species and compartment addition and editing templates

	vector<inp_view_template<Explorer>> input_templates;
	vector<string> input_template_names{ "Edit Species",
										 "Edit Reaction",
										 "Edit Compartment",
										 "Add Species",
										 "Add Reaction",
										 "Add Compartment"};

	vector<button_template<Explorer> > b_temp;
	vector<text_inp_template<Explorer> > ti_temp;
	vector<vector<text_inp_template<Explorer>>> il_temp;
	vector<vector<double>> il_widths;
	vector<string> il_titles;
	vector<vector<pair<char, int>>> order;

	// Edit / add species

	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::edit_species, this, "*ID:", 1));
	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::edit_species, this, "Name:", 2));
	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::find_compart, this, "Compartment:", 1, height*0.617, height*0.38));
	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::edit_bound_cond, this, "Boundary Condition:", 1, height*0.617, height*0.38));
	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::edit_species, this, "Formula:", 3));
	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::edit_species, this, "Kegg ID:", 4));
	
	b_temp.push_back(button_template<Explorer>(button_type::click, &Explorer::spec_reac_manipulate, this, "Save species", 1));
	b_temp.push_back(button_template<Explorer>(button_type::click, &Explorer::spec_reac_manipulate, this, "Reset", 0));
	b_temp.push_back(button_template<Explorer>(button_type::click, &Explorer::spec_reac_manipulate, this, "Cancel", 10));

	order.resize(order.size() + 1);
	order.back().push_back(pair<char, int>('i', 0));
	order.back().push_back(pair<char, int>('i', 1));
	order.resize(order.size() + 1);
	order.back().push_back(pair<char, int>('i', 2));
	order.back().push_back(pair<char, int>('i', 3));
	order.resize(order.size() + 1);
	order.back().push_back(pair<char, int>('i', 4));
	order.back().push_back(pair<char, int>('i', 5));
	order.resize(order.size() + 1);
	order.back().push_back(pair<char, int>('b', 0));
	order.back().push_back(pair<char, int>('b', 1));
	order.back().push_back(pair<char, int>('b', 2));

	inp_view_template<Explorer> espec(b_temp, ti_temp, il_temp, il_widths, il_titles, order);

	b_temp.clear();
	order.back().pop_back();
	b_temp.push_back(button_template<Explorer>(button_type::click, &Explorer::spec_reac_manipulate, this, "Save species", 2));
	b_temp.push_back(button_template<Explorer>(button_type::click, &Explorer::spec_reac_manipulate, this, "Cancel", 10));

	inp_view_template<Explorer> aspec(b_temp, ti_temp, il_temp, il_widths, il_titles, order);

	// Edit / add reaction

	ti_temp.clear();
	il_temp.clear();
	il_widths.clear();
	il_titles.clear();
	b_temp.clear();
	order.clear();

	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::edit_reaction, this, "*ID:", 1));
	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::edit_reaction, this, "Name:", 2));
	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::edit_reversibility, this, "Reversible:", 1, height*0.617, height*0.38));
	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::edit_reaction, this, "Objective coeff:", 5));
	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::edit_reaction, this, "Flux UB:", 3));
	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::edit_reaction, this, "Flux LB:", 4));
	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::edit_reaction, this, "K equilibrium:", 6));
	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::edit_reaction, this, "Kegg ID:", 7));

	il_temp.push_back({ text_inp_template<Explorer>(&Explorer::find_reactant, this, "*Reactant:", 0, height*0.617, height*0.38), text_inp_template<Explorer>(&Explorer::edit_reactant_st, this, "Stoichiometry:", 0) });
	il_temp.push_back({ text_inp_template<Explorer>(&Explorer::find_product, this, "*Product:", 0, height*0.617, height*0.38), text_inp_template<Explorer>(&Explorer::edit_product_st, this, "Stoichiometry:", 0) });
	il_temp.push_back({ text_inp_template<Explorer>(&Explorer::edit_gene, this, "*Gene:",0)});

	il_widths.push_back({ 305,305 });
	il_widths.push_back({ 305,305 });
	il_widths.push_back({ 305 });
	il_titles.push_back("reactant");
	il_titles.push_back("product");
	il_titles.push_back("gene");

	b_temp.push_back(button_template<Explorer>(button_type::click, &Explorer::spec_reac_manipulate, this, "Save reaction", 3));
	b_temp.push_back(button_template<Explorer>(button_type::click, &Explorer::spec_reac_manipulate, this, "Reset", 0));
	b_temp.push_back(button_template<Explorer>(button_type::click, &Explorer::spec_reac_manipulate, this, "Cancel", 10));

	order.resize(order.size() + 1);
	order.back().push_back(pair<char, int>('i', 0));
	order.back().push_back(pair<char, int>('i', 1));
	order.resize(order.size() + 1);
	order.back().push_back(pair<char, int>('i', 2));
	order.back().push_back(pair<char, int>('i', 3));
	order.resize(order.size() + 1);
	order.back().push_back(pair<char, int>('i', 4));
	order.back().push_back(pair<char, int>('i', 5));
	order.resize(order.size() + 1);
	order.back().push_back(pair<char, int>('i', 6));
	order.back().push_back(pair<char, int>('i', 7));
	order.resize(order.size() + 1);
	order.back().push_back(pair<char, int>('l', 0));
	order.resize(order.size() + 1);
	order.back().push_back(pair<char, int>('l', 1));
	order.resize(order.size() + 1);
	order.back().push_back(pair<char, int>('l', 2));
	order.resize(order.size() + 1);
	order.back().push_back(pair<char, int>('b', 0));
	order.back().push_back(pair<char, int>('b', 1));
	order.back().push_back(pair<char, int>('b', 2));

	inp_view_template<Explorer> ereac(b_temp, ti_temp, il_temp, il_widths, il_titles, order);

	b_temp.clear();
	order.back().pop_back();
	b_temp.push_back(button_template<Explorer>(button_type::click, &Explorer::spec_reac_manipulate, this, "Save reaction", 4));
	b_temp.push_back(button_template<Explorer>(button_type::click, &Explorer::spec_reac_manipulate, this, "Cancel", 10));

	inp_view_template<Explorer> areac(b_temp, ti_temp, il_temp, il_widths, il_titles, order);

	// Edit / add compartment

	ti_temp.clear();
	il_temp.clear();
	il_widths.clear();
	il_titles.clear();
	b_temp.clear();
	order.clear();

	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::edit_compart, this, "*ID:", 1));
	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::edit_compart, this, "Name:", 2));
	ti_temp.push_back(text_inp_template<Explorer>(&Explorer::find_outside, this, "Outside:", 3, height*0.617, height*0.38));

	b_temp.push_back(button_template<Explorer>(button_type::click, &Explorer::spec_reac_manipulate, this, "Save compartment", 5));
	b_temp.push_back(button_template<Explorer>(button_type::click, &Explorer::spec_reac_manipulate, this, "Reset", 0));
	b_temp.push_back(button_template<Explorer>(button_type::click, &Explorer::spec_reac_manipulate, this, "Cancel", 10));

	order.resize(order.size() + 1);
	order.back().push_back(pair<char, int>('i', 0));
	order.back().push_back(pair<char, int>('i', 1));
	order.resize(order.size() + 1);
	order.back().push_back(pair<char, int>('i', 2));
	order.resize(order.size() + 1);
	order.back().push_back(pair<char, int>('b', 0));
	order.back().push_back(pair<char, int>('b', 1));
	order.back().push_back(pair<char, int>('b', 2));

	inp_view_template<Explorer> ecomp(b_temp, ti_temp, il_temp, il_widths, il_titles, order);

	b_temp.clear();
	order.back().pop_back();
	b_temp.push_back(button_template<Explorer>(button_type::click, &Explorer::spec_reac_manipulate, this, "Save compartment", 6));
	b_temp.push_back(button_template<Explorer>(button_type::click, &Explorer::spec_reac_manipulate, this, "Cancel", 10));

	inp_view_template<Explorer> acomp(b_temp, ti_temp, il_temp, il_widths, il_titles, order);

	/// put everything together 

	input_templates.push_back(espec);
	input_templates.push_back(ereac);
	input_templates.push_back(ecomp);
	input_templates.push_back(aspec);
	input_templates.push_back(areac);
	input_templates.push_back(acomp);

    /// //////////////////////////////
	win = new Window<Explorer>(display, buttons, text_inputs, input_template_names, input_templates);

    double x0, y0, x1, y1;
    win->Get_cust_coors(x0,y0,x1,y1);
    canvas = al_create_bitmap(x1-x0,y1-y0);
    if(!canvas) fprintf(stderr, "failed to create canvas!\n");
    fastg_sprite = al_create_bitmap(q_coeff*(x1-x0),q_coeff*(y1-y0));

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_mouse_event_source());
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_start_timer(timer);

    specinfo_font = al_load_ttf_font(font_file_name.c_str(),specinfo_font_size,0);
}

Explorer::~Explorer()
{
    delete model;
    delete win;
    if(canvas!=NULL) al_destroy_bitmap(canvas);
    if(fastg_sprite!=NULL) al_destroy_bitmap(fastg_sprite);
    if(specinfo_font!=NULL) al_destroy_font(specinfo_font);
    if(timer!=NULL) al_destroy_timer(timer);
    if(display!=NULL) al_destroy_display(display);
    if(event_queue!=NULL) al_destroy_event_queue(event_queue);
}

string col_to_str(COL col)
{
	unsigned char R, G, B;
	string r, g, b;
	al_unmap_rgb(col, &R, &G, &B);
	r = to_string((int)R);
	g = to_string((int)G);
	b = to_string((int)B);
	return "{" + r + ";" + g + ";" + b + "}";
}

int Explorer::run()
{
    COL red = al_map_rgb(255,0,0);
    COL green = al_map_rgb(0,255,0);
    COL blue = al_map_rgb(0,0,255);
    COL purple = al_map_rgb(255,0,255);
    COL yellow = al_map_rgb(255,255,0);
    COL orange = al_map_rgb(255,127,0);
    COL black = al_map_rgb(0,0,0);
    COL white = al_map_rgb(255,255,255);

    bool redraw = true;
    bool left_disp = false;
    queue<event_type> events;
    queue<pair<char,char>> key_inps;

    double fps = 0;
    int frames_done = 0;
    double old_time = al_get_time();
    string old_name = FILE_NAME;
	bool scrolled_already = false;
	bool moved_mouse_already = false;

	bool first_time = true;

    while(1)
    {
        if(quit) break;

        ALLEGRO_EVENT ev;
        al_wait_for_event(event_queue, &ev);

        if(FILE_NAME!=old_name)
        {
            string name = FILE_NAME + " - " + VERSION;
            al_set_window_title(display,name.c_str());
            old_name=FILE_NAME;
        }

        if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) break;
        if(ev.type == ALLEGRO_EVENT_DISPLAY_RESIZE)
        {
            scr_upd = true;
            al_acknowledge_resize(display);
            win->Resize(display);
            if(disp_map!=NULL) al_destroy_bitmap(disp_map);
            disp_map = al_create_bitmap(al_get_display_width(display),al_get_display_height(display));
            if(canvas!=NULL) al_destroy_bitmap(canvas);
            if(fastg_sprite!=NULL) al_destroy_bitmap(fastg_sprite);
            double x0, y0, x1, y1;
            win->Get_cust_coors(x0,y0,x1,y1);
            canvas = al_create_bitmap(x1-x0,y1-y0);
            if(!canvas) fprintf(stderr, "failed to recreate canvas!\n");
            fastg_sprite = al_create_bitmap(q_coeff*(x1-x0),q_coeff*(y1-y0));
        }

		if (ev.type == ALLEGRO_EVENT_DISPLAY_EXPOSE)
		{
			redraw = true;
		}

        if(ev.type == ALLEGRO_EVENT_MOUSE_AXES || ev.type == ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY)
        {
            left_disp = false;
            redraw = true;
            mouse_x = (double)ev.mouse.x;
            mouse_y = (double)ev.mouse.y;
			if (!moved_mouse_already)
			{
				events.push(event_type::move_mouse);
				moved_mouse_already = true;
			}
            if(!ev.mouse.dz==0)
            {
                mouse_dz += ev.mouse.dz;
				if (!scrolled_already)
				{
					events.push(event_type::scroll);
					scrolled_already = true;
				}
            }
        }
        if (ev.type == ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY)
        {
            left_disp = true;
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
            if(ev.keyboard.keycode==ALLEGRO_KEY_ESCAPE) {/*break;*/}
			else if ((ev.keyboard.modifiers & ALLEGRO_KEYMOD_SHIFT) && ev.keyboard.keycode == ALLEGRO_KEY_DELETE) purge_selection_weak();
			else if (ev.keyboard.keycode == ALLEGRO_KEY_DELETE) purge_selection();
			else if ((ev.keyboard.modifiers & ALLEGRO_KEYMOD_CTRL) && ev.keyboard.keycode == ALLEGRO_KEY_Z)
			{
				model->undo();
				update_arrays();
				scr_upd = true;
				update_compartments();
			}
			else if ((ev.keyboard.modifiers & ALLEGRO_KEYMOD_CTRL) && ev.keyboard.keycode == ALLEGRO_KEY_Y)
			{
				model->redo();
				update_arrays();
				scr_upd = true;
				update_compartments();
			}
            else
            {
				char mod = 'n';
				if (ev.keyboard.modifiers & ALLEGRO_KEYMOD_CTRL) mod = 'c';
				else if(ev.keyboard.modifiers & ALLEGRO_KEYMOD_SHIFT) mod = 's';
                events.push(event_type::keyboard);
                key_inps.push(pair<char,char>(ev.keyboard.unichar,mod));
            }
        }

        if((redraw && al_is_event_queue_empty(event_queue)) || scr_upd)
        {
			moved_mouse_already = false;
			scrolled_already = false;

            al_set_target_bitmap(al_get_backbuffer(display));
            al_clear_to_color(graph_bckgr_c);

            bool got_any_cust_move_mouse = false;
			bool got_resize = false;
            while(!events.empty())
            {
                response_type response = win->respond_to_event(display,events.front(),mouse_x,mouse_y,mouse_dz,(events.front()==event_type::keyboard) ? key_inps.front() : pair<char,char>('\0','n'));
                if(events.front()==event_type::keyboard) key_inps.pop();
                events.pop();
                ///respond
                react_to_response(response);
                if(response==response_type::cust_move_mouse || response==response_type::tab_key) got_any_cust_move_mouse=true;
				if(response==response_type::hor_area_resize
				   || response==response_type::ver_area_resize) got_resize=true;
            }
			mouse_dz = 0;
            if(model!=NULL && !first_time)
            {
				double x0, y0, x1, y1;
                win->Get_cust_coors(x0,y0,x1,y1);
                draw_to_screen(x0,y0,x1,y1);
                draw_information(got_any_cust_move_mouse);
            }

            win->draw();
            if(!left_disp)
            {
                if(cursor!=normal)
                {
                    if(cursor == hor_resize)
                    {
                        al_draw_filled_rectangle(mouse_x-curs_bar_w*0.5,mouse_y-curs_bar_l*0.5,mouse_x+curs_bar_w*0.5,mouse_y+curs_bar_l*0.5,resize_curs_c);
                        al_draw_filled_triangle(mouse_x-curs_arr_w*0.5,mouse_y+curs_bar_l*0.5,mouse_x+curs_arr_w*0.5,mouse_y+curs_bar_l*0.5,mouse_x,mouse_y+curs_bar_l*0.5+curs_arr_l,resize_curs_c);
                        al_draw_filled_triangle(mouse_x-curs_arr_w*0.5,mouse_y-curs_bar_l*0.5,mouse_x+curs_arr_w*0.5,mouse_y-curs_bar_l*0.5,mouse_x,mouse_y-curs_bar_l*0.5-curs_arr_l,resize_curs_c);
                    }
                    else if(cursor == ver_resize)
                    {
                        al_draw_filled_rectangle(mouse_x-curs_bar_l*0.5,mouse_y-curs_bar_w*0.5,mouse_x+curs_bar_l*0.5,mouse_y+curs_bar_w*0.5,resize_curs_c);
                        al_draw_filled_triangle(mouse_x+curs_bar_l*0.5,mouse_y-curs_arr_w*0.5,mouse_x+curs_bar_l*0.5,mouse_y+curs_arr_w*0.5,mouse_x+curs_bar_l*0.5+curs_arr_l,mouse_y,resize_curs_c);
                        al_draw_filled_triangle(mouse_x-curs_bar_l*0.5,mouse_y-curs_arr_w*0.5,mouse_x-curs_bar_l*0.5,mouse_y+curs_arr_w*0.5,mouse_x-curs_bar_l*0.5-curs_arr_l,mouse_y,resize_curs_c);
                    }
                    cursor=normal;
                }
            }

            al_set_target_bitmap(disp_map);
            al_draw_bitmap(al_get_backbuffer(display),0,0,0);
            al_set_target_bitmap(al_get_backbuffer(display));
            al_flip_display();
            scr_upd = false;
            redraw=false;

			if(first_time && !model->empty()) 
			{
				Loading l(display, disp_map, 100, al_get_display_width(display)*0.5, al_get_display_height(display)*0.5, this, &Explorer::make_layout); 
			}
			first_time = false;

			// Print fps if required
			if(print_fps) 
			{
				double game_time = al_get_time();
				if(game_time - old_time >= 1.0)
				{
					fps = frames_done / (game_time - old_time);
					printf("\nFPS: %f\n",fps);
					frames_done = 0;
					old_time = game_time;
				}
				frames_done++;
			}
        }
    }
    return 0;
}

int Explorer::react_to_response(response_type res)
{
    if(res==response_type::zoom)
    {
        double zoom = pow(zoom_per_scroll,mouse_dz);
        double x_0, y_0, x_1, y_1;
        win->Get_cust_coors(x_0,y_0,x_1,y_1);
        double rat = (x_1-x_0)/scr_w;
        double c_x = 0.5*(x_0+x_1);
        double c_y = 0.5*(y_0+y_1);
        scr_c_x += (mouse_x-c_x)/rat*(1-zoom);
        scr_c_y -= (mouse_y-c_y)/rat*(1-zoom);
        scr_w *= zoom;
        scr_upd = true;
        mouse_dz=0;
    }
    else if(res==response_type::rmb_down_custom)
    {
		int hit = -1;
		if(hit_node(hit))
		{
			freeze = hit;
			draw_spec_relatives(freeze,true);
		}
		else freeze = -1;
    }
    else if(res==response_type::lmb_down_custom)
    {
        scr_c_x_save = scr_c_x;
        scr_c_y_save = scr_c_y;
        lmb_move=true;
    }
    else if(res==response_type::lmb_move_mouse)
    {
        if(lmb_move)
        {
            double x_0, y_0, x_1, y_1;
            win->Get_cust_coors(x_0,y_0,x_1,y_1);
            double rat = (x_1-x_0)/scr_w;
            double prev_mx, prev_my;
            win->Get_cust_prev_mouse_x(prev_mx,prev_my);
            scr_c_x = scr_c_x_save - (mouse_x-prev_mx)/rat;
            scr_c_y = scr_c_y_save + (mouse_y-prev_my)/rat;
            scr_upd = true;
        }
    }
    else if(res==response_type::lmb_up_custom)
    {
        if(lmb_move) lmb_move = false;
    }
    else if(res==response_type::cust_move_mouse)
    {
        int hit = -1;

        if(hit_node(hit))
        {
            if(hit!=prev_hit && freeze==-1)
            {
                draw_spec_relatives(hit,true);
                prev_hit=hit;
            }
        }
		if(T_MODE==error_source && freeze==ereac_frozen && freeze!=-1)
		{
			if(espec_group.find(hit)!=espec_group.end()) draw_spec_relatives(hit,true);
			else draw_spec_relatives(freeze,true);
		}
        else prev_hit=-1;
    }
    else if(res==response_type::ver_area_resize || res==response_type::hor_area_resize)
    {
        scr_upd=true;
        double x0, y0, x1, y1;
        win->Get_cust_coors(x0,y0,x1,y1);

        if(res==response_type::ver_area_resize) scr_w *= (x1-x0)/(al_get_bitmap_width(canvas));

        if(canvas!=NULL) al_destroy_bitmap(canvas);
        if(fastg_sprite!=NULL) al_destroy_bitmap(fastg_sprite);
        canvas = al_create_bitmap(x1-x0,y1-y0);
        if(!canvas) fprintf(stderr, "failed to recreate canvas!\n");
        fastg_sprite = al_create_bitmap(q_coeff*(x1-x0),q_coeff*(y1-y0));

        if(res==response_type::ver_area_resize) cursor=ver_resize;
        if(res==response_type::hor_area_resize) cursor=hor_resize;
    }
    else if(res==response_type::ver_area_resize_hov) cursor=ver_resize;
    else if(res==response_type::hor_area_resize_hov) cursor=hor_resize;
	else if(res==response_type::tab_key)
	{
		if(T_MODE==ancestral)
		{
			tree_type = 1 - tree_type;
			if(frozen_tree_centre!=-1)
			{
				if(tree_type==0) make_ancestry_tree(parents,frozen_tree,reactants,products,reversible,dead,frozen_tree_centre,sumreac,1e6);
				else if(tree_type==1) make_ancestry_tree(children,frozen_tree,products,reactants,reversible,dead_out,frozen_tree_centre,sumreac,1e6);
				draw_spec_relatives(frozen_tree_centre,true);
			}
			else
			{
				
         		int hit = -1;
				if(hit_node(hit)) 
				{
					draw_spec_relatives(hit,true);
					prev_hit=hit;
				} 
			}
			
		}
	}
    return 0;
}

bool inside_triangle(const Point &p, const Point &a, const Point &b, const Point &c)
{
    double Area = 0.5 *(-b.y*c.x + a.y*(-b.x + c.x) + a.x*(b.y - c.y) + b.x*c.y);
    double s = 1/(2*Area)*(a.y*c.x - a.x*c.y + (c.y - a.y)*p.x + (a.x - c.x)*p.y);
    double t = 1/(2*Area)*(a.x*b.y - a.y*b.x + (a.y - b.y)*p.x + (b.x - a.x)*p.y);
    if(s>0 && t>0 && (1-s-t)>0) return true;
    else return false;
}

bool inside_line(const Point &p, const Point &a, const Point &b, double thickness)
{
	Point perp_vec(-(b - a).y, (b - a).x);
	perp_vec.normalize();
	perp_vec *= 0.5*thickness;
	if (inside_triangle(p, a + perp_vec, a - perp_vec, b + perp_vec) || inside_triangle(p, a - perp_vec, b + perp_vec, b - perp_vec)) return true;
	else return false;
}

void Explorer::draw_information(bool got_any_cust_move_mouse)
{
    int hit = -1;
    if(hit_node(hit)) draw_spec_info(hit);
    if(freeze==-1)
    {
        if(hit!=-1 && got_any_cust_move_mouse) {draw_spec_relatives(hit,false);}
        else draw_spec_relatives(-1,true);
    }
    else
    {
        if(hit!=-1 && got_any_cust_move_mouse) {draw_spec_relatives(hit,false); draw_spec_info(hit);}
        draw_spec_relatives(freeze,false);
        if(scr_upd) draw_spec_relatives(freeze,true);
    }
    // draw compartments
    {
        double x_0, y_0, x_1, y_1;
        win->Get_cust_coors(x_0,y_0,x_1,y_1);
        double x_res = x_1-x_0;
        double y_res = y_1-y_0;
        double scr_x0 = scr_c_x-0.5*scr_w;
        double scr_y0 = scr_c_y-0.5*scr_w*y_res/x_res;
        double rat = x_res/scr_w;
        
        if(mouse_x>x_0 && mouse_x<x_1 && mouse_y>y_0 && mouse_y<y_1)
        {
            vector<string> hits;
            for(const auto &i:cmprts)
            {
                vector<Point> polygon;
                for(size_t j=0;j<i.second.size();j++) polygon.push_back(Point(x_0 + (i.second[j].x-scr_x0)*rat, y_1 - (i.second[j].y-scr_y0)*rat));

                 if(polygon.size()==1) 
                {
                    if(sqrt(pow(mouse_x-polygon[0].x,2)+pow(mouse_y-polygon[0].y,2))<compart_r) hits.push_back(i.first);
                }
                else if(polygon.size()==2)
                {
                    if(sqrt(pow(mouse_x-polygon[0].x,2)+pow(mouse_y-polygon[0].y,2))<compart_r) hits.push_back(i.first);
                    else if(sqrt(pow(mouse_x-polygon[1].x,2)+pow(mouse_y-polygon[1].y,2))<compart_r) hits.push_back(i.first);
                }
                else
                {
                    for(int j=2;j<polygon.size();j++)
                    {
                        Point m(mouse_x,mouse_y);
                        if(inside_triangle(m,polygon[0],polygon[j-1],polygon[j])) { hits.push_back(i.first); break; }
                    }
                } 
            }
            if(!hits.empty()) draw_comp_info(hits);
        }
    }
}

void Explorer::draw_spec_relatives(int hit, bool draw_legend_or_line)
{
	if(T_MODE!=non_curious && win->get_mode()!=Window<Explorer>::neighbour_info && !editing_entity) win->set_mode(Window<Explorer>::neighbour_info);
    if(hit==-1) // if does not hover over anything of importance
    {
        vector<string> info;
        win->Change_text(info);
		if(!editing_entity && win->get_mode()!=Window<Explorer>::neighbour_info && freeze == -1) win->set_mode(Window<Explorer>::neighbour_info);
    }
	else if (hit < -1 && T_MODE==non_curious) // if hovers over compartment
	{
		Nikal n;
		double x_0, y_0, x_1, y_1;
		win->Get_cust_coors(x_0, y_0, x_1, y_1);
		al_set_clipping_rectangle(x_0, y_0, x_1, y_1);

		al_set_target_bitmap(al_get_backbuffer(display));

		double x_res = x_1 - x_0;
		double y_res = y_1 - y_0;

		double scr_x0 = scr_c_x - 0.5*scr_w;
		double scr_y0 = scr_c_y - 0.5*scr_w*y_res / x_res;
		double rat = x_res / scr_w;
		/// Draw compartments
		if (draw_comparts)
		{
			int iter = -2;
			for (const auto &i : cmprts)
			{
				if (hit == iter)
				{
					vector<Point> polygon;
					for (size_t j = 0; j < i.second.size(); j++) polygon.push_back(Point(x_0 + (i.second[j].x - scr_x0)*rat, y_1 - (i.second[j].y - scr_y0)*rat));
					if (i.second.size() > 2)
					{
						for (size_t j = 1; j < i.second.size(); j++) n.add_line(polygon[j].x, polygon[j].y, polygon[j - 1].x, polygon[j - 1].y, compart_line_c, compart_line_on_t);
						n.add_line(polygon.back().x, polygon.back().y, polygon.front().x, polygon.front().y, compart_line_c, compart_line_on_t);
					}
					else if (i.second.size() == 2)
					{
						n.add_circle(polygon[0].x, polygon[0].y, compart_r, compart_line_c, compart_line_on_t);
						n.add_circle(polygon[1].x, polygon[1].y, compart_r, compart_line_c, compart_line_on_t);
					}
					else if (i.second.size() == 1) n.add_circle(polygon[0].x, polygon[0].y, compart_r, compart_line_c, compart_line_on_t);
				}
				iter--;
			}
		}
		n.draw();
		al_set_clipping_rectangle(0, 0, al_get_bitmap_width(al_get_backbuffer(display)), al_get_bitmap_height(al_get_backbuffer(display)));

		string s;
		if (draw_legend_or_line && freeze == -1)
		{
			if (win->get_mode() != Window<Explorer>::add_compart && win->get_mode() != Window<Explorer>::add_reac && win->get_mode() != Window<Explorer>::add_spec)
			{
				freeze = hit;
				add_or_edit(0, s);
				freeze = -1;
			}
		}
		if (draw_legend_or_line && freeze != -1)
		{
			if (win->get_mode() == Window<Explorer>::neighbour_info) add_or_edit(0, s);
		}
	}
    else if(hit>=0)
    {
        Nikal n;

        /// Draw the names of the neighbours
        vector<string> info;
        if(hit<sumreac) info.push_back("$t"+model->reactions.at(index_to_id[hit]).name+" ["+index_to_id[hit]+"]");
        else 
        {
            string compart_name = (model->species.at(index_to_id[hit]).compart.empty()) ? "Unassigned compartment" : model->compartments.at(model->species.at(index_to_id[hit]).compart).name;
            info.push_back("$t"+model->species.at(index_to_id[hit]).name+" ["+index_to_id[hit-sumreac]+"] ("+compart_name+")");
        }

        double x_0, y_0, x_1, y_1;
        win->Get_cust_coors(x_0,y_0,x_1,y_1);
        al_set_clipping_rectangle(x_0,y_0,x_1,y_1);

        al_set_target_bitmap(al_get_backbuffer(display));

        double x_res = x_1-x_0;
        double y_res = y_1-y_0;

        double scr_x0 = scr_c_x-0.5*scr_w;
        double scr_y0 = scr_c_y-0.5*scr_w*y_res/x_res;
        double rat = x_res/scr_w;

        double bound_x0 = x_0-node_r;
        double bound_x1 = x_1+node_r;
        double bound_y0 = y_0-node_r;
        double bound_y1 = y_1+node_r;

        double x0 = x_0 + (coors[0][hit]-scr_x0)*rat;
        double y0 = y_1 - (coors[1][hit]-scr_y0)*rat;

        if(T_MODE==neighbour)
        {
            /// Draw the connecting lines to the neighbours

            if(!draw_legend_or_line)
            {
                if(hit<sumreac) /// draw reaction reactants and products
                {
                    for(const int &i:reactants[hit])
                    {
                        double x1 = x_0 + (coors[0][i]-scr_x0)*rat; 
                        double y1 = y_1 - (coors[1][i]-scr_y0)*rat;
                        n.add_line(x0,y0,x1,y1,reactant_line_c,bold_line_w);
                    }
                    for(const int &i:products[hit])
                    {
                        double x1 = x_0 + (coors[0][i]-scr_x0)*rat; 
                        double y1 = y_1 - (coors[1][i]-scr_y0)*rat;
                        n.add_line(x0,y0,x1,y1,product_line_c,bold_line_w);
                    }
                }
                else /// draw generating and consuming reactions
                {
                    for(const int &i:o_products_of[hit-sumreac])
                    {
                        double x1 = x_0 + (coors[0][i]-scr_x0)*rat;
                        double y1 = y_1 - (coors[1][i]-scr_y0)*rat;
                        n.add_line(x0,y0,x1,y1,reactant_line_c,bold_line_w);
                    }
                    for(const int &i:o_reactants_of[hit-sumreac])
                    {
                        double x1 = x_0 + (coors[0][i]-scr_x0)*rat;
                        double y1 = y_1 - (coors[1][i]-scr_y0)*rat;
                        n.add_line(x0,y0,x1,y1,product_line_c,bold_line_w);
                    }
                    for(const int &i:p_and_r_of[hit-sumreac])
                    {
                        double x1 = x_0 + (coors[0][i]-scr_x0)*rat;
                        double y1 = y_1 - (coors[1][i]-scr_y0)*rat;
                        n.add_line(x0,y0,x1,y1,reversbl_line_c,bold_line_w);
                    }
                }
            }
            else
            {
                if(hit<sumreac)
                {
                    bool rev = model->reactions.at(index_to_id[hit]).reversible;
                    string col;

                    if(model->reactions.at(index_to_id[hit]).reactants.size()>0)
                    {
                        col = col_to_str(reactant_line_c);
                        if(rev) info.push_back("$"+col+"Reactants (reversible):");
                        else info.push_back("$"+col+"Reactants:");
                        info.push_back("\t");
                        for(const auto &i:model->reactions.at(index_to_id[hit]).reactants)
                        {
                            string compart_name = (model->species.at(i.first).compart.empty()) ? "Unassigned compartment" : model->compartments.at(model->species.at(i.first).compart).name;
                            info.push_back(model->species.at(i.first).name+" ["+i.first+"] ("+compart_name+")");
                        }
                    }
                    if(model->reactions.at(index_to_id[hit]).products.size()>0)
                    {
                        col = col_to_str(product_line_c);
                        if(rev) info.push_back("$"+col+"Products (reversible):");
                        else info.push_back("$"+col+"Products:");
                        info.push_back("\t");
                        for(const auto &i:model->reactions.at(index_to_id[hit]).products)
                        {
                            string compart_name = (model->species.at(i.first).compart.empty()) ? "Unassigned compartment" : model->compartments.at(model->species.at(i.first).compart).name;
                            info.push_back(model->species.at(i.first).name+" ["+i.first+"] ("+compart_name+")");
                        }
                    }
                }
                else
                {
                    string col;
                    if(!o_products_of[hit-sumreac].empty())
                    {
                        col = col_to_str(reactant_line_c);
                        info.push_back("$"+col+"Producing reactions:");
                        info.push_back("\t");
                        for(int i:o_products_of[hit-sumreac])
                        {
                            info.push_back(model->reactions.at(index_to_id[i]).name+" ["+index_to_id[i]+"]");
                        }
                    }
                    if(!o_reactants_of[hit-sumreac].empty())
                    {
                        col = col_to_str(product_line_c);
                        info.push_back("$"+col+"Consuming reactions:");
                        info.push_back("\t");
                        for(int i:o_reactants_of[hit-sumreac])
                        {
                            info.push_back(model->reactions.at(index_to_id[i]).name+" ["+index_to_id[i]+"]");
                        }
                    }
                    if(!p_and_r_of[hit-sumreac].empty())
                    {
                        col = col_to_str(reversbl_line_c);
                        info.push_back("$"+col+"Bidirectional reactions:");
                        info.push_back("\t");
                        for(int i:p_and_r_of[hit-sumreac])
                        {
                            info.push_back(model->reactions.at(index_to_id[i]).name+" ["+index_to_id[i]+"]");
                        }
                    }
                }

                /// Write down the neighbours
                win->Change_text(info);
            }
        }
        else if(T_MODE==ancestral)
        {
            Array<ancestor> tree; 
            if(hit!=freeze || (hit==freeze && frozen_tree_centre!=hit))
            {
                if(tree_type==0) make_ancestry_tree(parents,tree,reactants,products,reversible,dead,hit,sumreac,1e6);
				else if(tree_type==1) make_ancestry_tree(children,tree,products,reactants,reversible,dead_out,hit,sumreac,1e6);
			}
            else if(hit==freeze && !frozen_tree.empty())
            {
                tree = frozen_tree;
            }

            if(hit==freeze && frozen_tree_centre!=hit) {frozen_tree = tree; frozen_tree_centre = hit; }
            else if(hit!=freeze && !frozen_tree.empty()) { frozen_tree.clear(); frozen_tree_centre = -1;}

            if(!draw_legend_or_line)
            {
                /// Draw the ancestors of the species
                COL line_col = (tree_type==0 ? dead[hit] : dead_out[hit]) ? bold_line_c : liv_bold_line_c;
                for(auto& q:tree) for(ancestor& i:q)
                {
                    if(!i.killed)
                    {
                        double x1 = x_0 + (coors[0][i.num]-scr_x0)*rat;
                        double y1 = y_1 - (coors[1][i.num]-scr_y0)*rat;
                        for(int_int j:i.parents)
                        {
                            if(!tree[j.x][j.y].killed)
                            {
                                double x2 = x_0 + (coors[0][tree[j.x][j.y].num]-scr_x0)*rat;
                                double y2 = y_1 - (coors[1][tree[j.x][j.y].num]-scr_y0)*rat;
                                bool node_1_inside = (x1>bound_x0 && x1<bound_x1 && y1>bound_y0 && y1<bound_y1);
                                bool node_2_inside = (x2>bound_x0 && x2<bound_x1 && y2>bound_y0 && y2<bound_y1);
                                double scal_vec [4];
                                double scal;
                                if(!node_1_inside && node_2_inside)
                                {
                                    scal_vec[0] = (x2-x1) / (x2-bound_x0);
                                    scal_vec[1] = (x2-x1) / (x2-bound_x1);
                                    scal_vec[2] = (y2-y1) / (y2-bound_y0);
                                    scal_vec[3] = (y2-y1) / (y2-bound_y1);
                                    scal = 1.0 -1.0 / *max_element(scal_vec,scal_vec+4);
                                    n.add_line(x1+(x2-x1)*scal,y1+(y2-y1)*scal,x2,y2,line_col,bold_line_w);
                                }
                                else if(node_1_inside && !node_2_inside)
                                {
                                    scal_vec[0] = (x1-x2) / (x1-bound_x0);
                                    scal_vec[1] = (x1-x2) / (x1-bound_x1);
                                    scal_vec[2] = (y1-y2) / (y1-bound_y0);
                                    scal_vec[3] = (y1-y2) / (y1-bound_y1);
                                    scal = 1.0 -1.0 / *max_element(scal_vec,scal_vec+4);
                                    n.add_line(x1,y1,x2-(x2-x1)*scal,y2-(y2-y1)*scal,line_col,bold_line_w);
                                }
                                else if(node_1_inside && node_2_inside)
                                {
                                    n.add_line(x1,y1,x2,y2,line_col,bold_line_w);
                                }
                            }
                        }
                    }
                }
                for(auto& q:tree) for(ancestor& i:q)
                {
                    if(!i.reac_or_mol && !i.killed && uninitiated[i.num-sumreac])
                    {
                        double x1 = x_0 + (coors[0][i.num]-scr_x0)*rat;
                        double y1 = y_1 - (coors[1][i.num]-scr_y0)*rat;
                        bool node_inside = (x1>bound_x0 && x1<bound_x1 && y1>bound_y0 && y1<bound_y1);
                        if(node_inside) n.add_filled_circle(x1,y1,high_node_r,uninit_c);
                    }
                }
            }
            else
            {
                /// Draw the names of the ancestors

                for(size_t i=1;i<tree.size();i++)
                {
                    bool found = 0;
                    for(ancestor &j:tree[i]) if(!j.killed) {found=true;break;}
                    if(found)
                    {
                        string generation;
                        if(i==1) generation = (tree_type==0) ? "Parents" : "Children";
                        else if(i==2) generation = (tree_type==0) ? "Grandparents" : "Grandchildren";
                        else if(i==3) generation = (tree_type==0) ? "Great-grandparents" : "Great-grandchildren";
                        else generation = "Great-" + to_string((int)i-2) + ((tree_type==0) ? "-grandparents" : "-grandchildren");

                        info.push_back("$r" + generation + ":");
                        info.push_back("\t");

                        for(size_t j=0;j<tree[i].size();j++)
                        {
                            if(!tree[i][j].killed)
                            {
                                int u = tree[i][j].num;
                                if(u<sumreac) info.push_back(model->reactions.at(index_to_id[u]).name+" ["+index_to_id[u]+"]");
                                else 
                                {
                                    string compart_name = (model->species.at(index_to_id[u]).compart.empty()) ? "Unassigned compartment" : model->compartments.at(model->species.at(index_to_id[u]).compart).name;
                                    info.push_back(model->species.at(index_to_id[u]).name+" ["+index_to_id[u]+"] ("+compart_name+")");
                                }
                            }
                        }
                    }
                }
                win->Change_text(info);
            }
        }
		else if (T_MODE == blocked_module && B_MODE!=none)
		{
            set<int> bmodule; 
            if(hit!=freeze || (hit==freeze && frozen_bmodule_centre!=hit))
            {
				vector<bool> tested(neighbours.size(),false);
                find_bmodule(bmodule,neighbours,dead_r_and_s,hit,tested);
            }
            else if(hit==freeze && !frozen_bmodule.empty())
            {
                bmodule = frozen_bmodule;
            }

            if(hit==freeze && frozen_bmodule_centre!=hit) { frozen_bmodule = bmodule; frozen_bmodule_centre = hit; }
            else if(hit!=freeze && !frozen_bmodule.empty()) { frozen_bmodule.clear(); frozen_bmodule_centre = -1; }


            if(!draw_legend_or_line)
            {
                /// Draw the blocked moduule, so that each line is only drawn once
                COL line_col = bold_line_c;
                for(const auto& i:bmodule) 
                {
					if(i<sumreac) /// iterate through every reaction in the dead module
					{
						double x1 = x_0 + (coors[0][i]-scr_x0)*rat;
						double y1 = y_1 - (coors[1][i]-scr_y0)*rat;
						for(const auto &j:neighbours[i]) 
						{
							if(bmodule.find(j)!=bmodule.end()) /// iterate through all dead species connected to the dead module
							{
								double x2 = x_0 + (coors[0][j]-scr_x0)*rat;
								double y2 = y_1 - (coors[1][j]-scr_y0)*rat;
								bool node_1_inside = (x1>bound_x0 && x1<bound_x1 && y1>bound_y0 && y1<bound_y1);
								bool node_2_inside = (x2>bound_x0 && x2<bound_x1 && y2>bound_y0 && y2<bound_y1);
								double scal_vec [4];
								double scal;
								if(!node_1_inside && node_2_inside)
								{
									scal_vec[0] = (x2-x1) / (x2-bound_x0);
									scal_vec[1] = (x2-x1) / (x2-bound_x1);
									scal_vec[2] = (y2-y1) / (y2-bound_y0);
									scal_vec[3] = (y2-y1) / (y2-bound_y1);
									scal = 1.0 -1.0 / *max_element(scal_vec,scal_vec+4);
									n.add_line(x1+(x2-x1)*scal,y1+(y2-y1)*scal,x2,y2,line_col,bold_line_w);
								}
								else if(node_1_inside && !node_2_inside)
								{
									scal_vec[0] = (x1-x2) / (x1-bound_x0);
									scal_vec[1] = (x1-x2) / (x1-bound_x1);
									scal_vec[2] = (y1-y2) / (y1-bound_y0);
									scal_vec[3] = (y1-y2) / (y1-bound_y1);
									scal = 1.0 -1.0 / *max_element(scal_vec,scal_vec+4);
									n.add_line(x1,y1,x2-(x2-x1)*scal,y2-(y2-y1)*scal,line_col,bold_line_w);
								}
								else if(node_1_inside && node_2_inside)
								{
									n.add_line(x1,y1,x2,y2,line_col,bold_line_w);
								}
							}
						}
					}
                }
            }
            else
            {
                /// Draw the names of the species and reactions in the blocked module
				bool found_reactions = false;
				for(const auto &i:bmodule) if(i<sumreac) {found_reactions=true; break;}
				bool placed_spectitle = false;

				vector<string> info;
				if(hit<sumreac) info.push_back("$tBlocked Module around "+model->reactions.at(index_to_id[hit]).name+" ["+index_to_id[hit]+"]");
				else 
				{
					string compart_name = (model->species.at(index_to_id[hit]).compart.empty()) ? "Unassigned compartment" : model->compartments.at(model->species.at(index_to_id[hit]).compart).name;
					info.push_back("$tBlocked Module around "+model->species.at(index_to_id[hit]).name+" ["+index_to_id[hit-sumreac]+"] ("+compart_name+")");
				}
				if(found_reactions) 
				{
					info.push_back("$" + col_to_str(reac_off_c) + "Reactions:");
					info.push_back("\t");
				}
                for(const auto &i:bmodule)
                {
					if(i>=sumreac && !placed_spectitle) // Place title over species
					{
						info.push_back("$" + col_to_str(spec_off_c) + "Species:");
						info.push_back("\t");
						placed_spectitle=true;
					}

					if(i<sumreac) // if reaction
					{
						info.push_back(model->reactions.at(index_to_id[i]).name+" ["+index_to_id[i]+"]");
					}
					else // if species
					{
						string compart_name = (model->species.at(index_to_id[i]).compart.empty()) ? "Unassigned compartment" : model->compartments.at(model->species.at(index_to_id[i]).compart).name;
						info.push_back(model->species.at(index_to_id[i]).name+" ["+index_to_id[i]+"] ("+compart_name+")");
					}
                }
                win->Change_text(info);
            }
		}
		else if (T_MODE == error_source && B_MODE==FBA && arrays_upd_after_ccheck==0)
		{
			// First find out if freeze status has changed
			if(freeze==-1) 
			{
				// if nothing frozen, remove the freezes from reacs and specs
				ereac_frozen = -1;
				espec_frozen = -1;
				espec_group.clear();
				// The mouse is still hovering over a reaction we need to create the espec_group
				if(reacs_to_erspecs.find(hit)!=reacs_to_erspecs.end())
				{
					for(const auto &i:reacs_to_erspecs.at(hit)) espec_group.insert(i.first);
				}
			}
			else
			{
				// if the freeze is a reac, save this and reload the surrounding specs
				if(freeze<sumreac && freeze!=ereac_frozen)
				{
					if(reacs_to_erspecs.find(freeze)!=reacs_to_erspecs.end())
					{
						ereac_frozen = freeze;
						espec_group.clear();
						espec_frozen = -1;
						for(const auto &i:reacs_to_erspecs.at(ereac_frozen)) espec_group.insert(i.first);
					}
					else
					{
						// if selecting a spec outside the current list, abandon the current reacs
						ereac_frozen = -1;
						espec_frozen = -1;
						espec_group.clear();
						//freeze = -1;
					}
				}
				else if(freeze>=sumreac) 
				{
					if(espec_group.find(freeze)!=espec_group.end())
					{
						// if freeze is a spec and it is one of the frozen reac's causing specs
						espec_frozen = freeze;
					}
					else
					{
						// if selecting a spec outside the current list, abandon the current reacs
						ereac_frozen = -1;
						espec_frozen = -1;
						espec_group.clear();
						//freeze = -1;
					}
					 
				}
			}

			// The legend for BM/LM or reaction + causing specs
			// If the BM/LM is drawn, it allways overrides the reaction info
			vector<string> BMLM_info;
			vector<string> reac_info;
			
			// Draw BM-related stuff
			if((espec_frozen!=-1 && hit==freeze) || (espec_frozen==-1 && espec_group.find(hit)!=espec_group.end())) 
			{
				int target;
				if(espec_frozen!=-1) target = espec_frozen;
				else target = hit;

				int BM = erspecs.at(target).first.at(ereac_frozen).first;
				int LM = erspecs.at(target).first.at(ereac_frozen).second;

				// if the purpose is to draw someting else than the frozen stuff
				// and the mouse hovers over one of the causing specs, draw its BM(/LM)
				if(!draw_legend_or_line) // draw graphics (spec)
				{
					// Draw LM
					if(LM!=-1)
					{
 						for(const auto &i:LMs.at(LM).first)
						{
							double x_r = x_0 + (coors[0][i]-scr_x0)*rat;
							double y_r = y_1 - (coors[1][i]-scr_y0)*rat;
							for(const auto &j:neighbours[i])
							{
								if(LMs.at(LM).second.find(j)!=LMs.at(LM).second.end())
								{
									string r = index_to_id[i];
									string s = index_to_id[j];
									bool rop = model->reactions.at(r).reactants.find(s)!=model->reactions.at(r).reactants.end();
									draw_arrow(n,LM_arrow_c,graphics_mode,x_0,y_0,x_1,y_1,bound_x0,bound_y0,bound_x1,bound_y1,rat,scr_x0,scr_y0,rop ? j : i,rop ? i : j);
								}
							}
		
						} 

						for(size_t i=0;i<sumreac;i++)
						{
							if(LMs.at(LM).first.find(i)==LMs.at(LM).first.end())
							{
								double cir_x = x_0 + (coors[0][i]-scr_x0)*rat;
								double cir_y = y_1 - (coors[1][i]-scr_y0)*rat;
								bool node_inside = (cir_x>bound_x0 && cir_x<bound_x1 && cir_y>bound_y0 && cir_y<bound_y1);
								if(node_inside) n.add_filled_circle(cir_x,cir_y,node_r,(dead_reacs[i]) ? reac_off_c : reac_on_c);
							}
						}	
						for(size_t i=sumreac;i<sumtot;i++)
						{
							if(LMs.at(LM).second.find(i)==LMs.at(LM).second.end())
							{
								double cir_x = x_0 + (coors[0][i]-scr_x0)*rat;
								double cir_y = y_1 - (coors[1][i]-scr_y0)*rat;
								bool node_inside = (cir_x>bound_x0 && cir_x<bound_x1 && cir_y>bound_y0 && cir_y<bound_y1);
								if(node_inside) 
								{
									COL col = (spec_void[i-sumreac]) ? void_c : ((dead_specs[i-sumreac]) ? spec_off_c : spec_on_c);
									n.add_filled_circle(cir_x,cir_y,node_r,col);
									if (uninitiated[i - sumreac])
									{
										double t = 0.5*(uninit_node_r - node_r);
										n.add_circle(cir_x, cir_y, uninit_node_r - t, uninit_c, t*2.0);
									} 
								}
							}
						}

						for(const auto &i:LMs.at(LM).first) // draw the reactions over
						{
							double cir_x = x_0 + (coors[0][i]-scr_x0)*rat;
							double cir_y = y_1 - (coors[1][i]-scr_y0)*rat;
							bool node_inside = (cir_x>bound_x0 && cir_x<bound_x1 && cir_y>bound_y0 && cir_y<bound_y1);
							if(node_inside) n.add_filled_circle(cir_x,cir_y,node_r,LM_reac_c);
						}	
						for(const auto &i:LMs.at(LM).second) // draw the specs over
						{
							double cir_x = x_0 + (coors[0][i]-scr_x0)*rat;
							double cir_y = y_1 - (coors[1][i]-scr_y0)*rat;
							bool node_inside = (cir_x>bound_x0 && cir_x<bound_x1 && cir_y>bound_y0 && cir_y<bound_y1);
							if(node_inside) 
							{
								COL col = LM_spec_c;
								n.add_filled_circle(cir_x,cir_y,node_r,col);
							}
						}
					}

					// Draw BM
					for(const auto &i:BMs.at(BM).first)
					{
						double x_r = x_0 + (coors[0][i]-scr_x0)*rat;
						double y_r = y_1 - (coors[1][i]-scr_y0)*rat;
						for(const auto &j:neighbours[i])
						{
							double x_s = x_0 + (coors[0][j]-scr_x0)*rat;
							double y_s = y_1 - (coors[1][j]-scr_y0)*rat;
							if(BMs.at(BM).second.find(j)!=BMs.at(BM).second.end()) // if internal spec
							{
								n.add_line(x_r,y_r,x_s,y_s,BM_line_c,BM_line_w);
								n.add_filled_circle(x_s,y_s,0.5*BM_line_w,BM_line_c); // Draw circes
							}
							else // if peripheral spec
							{
								double L = sqrt(pow(x_s-x_r,2)+pow(y_s-y_r,2));
								double r = 0.5 * BM_line_w;
								double dy = r * (x_s - x_r) / L;
								double dx = r * (y_s - y_r) / L;
								n.add_filled_triangle(x_r-dx,y_r+dy,x_r+dx,y_r-dy,x_s,y_s,BM_line_c);
							}
						}
						n.add_filled_circle(x_r,y_r,0.5*BM_line_w,BM_line_c);
					}
				}
				else // draw legend
				{
					// The spec legend always overrides the reaction legend
					int target;
					if(espec_frozen!=-1) target = espec_frozen;
					else target = hit;

					string comp_name = (model->species.at(index_to_id[target]).compart.empty()) ? "Unassigned compartment" : model->compartments.at(model->species.at(index_to_id[target]).compart).name;
					BMLM_info.push_back("$tInconsistency-inducing species - "+model->species.at(index_to_id[target]).name+" ["+index_to_id[target]+"] (" + comp_name + ")");

					int er = erspecs.at(target).second;
					string error_type = er==0 ? "reversibility" : er==1 ? "source" : er==2 ? "stoichiometry" : "cycle";
					BMLM_info.push_back("$" + col_to_str(reac_off_c) + "Blocked reactions (" + error_type + "):");
					BMLM_info.push_back("\t");
					for(const auto &i:BMs.at(BM).first)
					{
						BMLM_info.push_back(model->reactions.at(index_to_id[i]).name+" ["+index_to_id[i]+"]");
					}
					if(LM!=-1)
					{
						BMLM_info.resize(BMLM_info.size()+1);
						int ler = LMtype.at(LM);
						string lerror_type = ler==0 ? "sink - unsupp" : ler==1 ? "source - unsupp" : "sink&source - unsupp";
						BMLM_info.push_back("$" + col_to_str(LM_spec_c) + "Constraining species group (" + lerror_type + "):");
						BMLM_info.push_back("\t");

						for(const auto &i:LMs.at(LM).second)
						{
							string compart_name = (model->species.at(index_to_id[i]).compart.empty()) ? "Unassigned compartment" : model->compartments.at(model->species.at(index_to_id[i]).compart).name;
							BMLM_info.push_back(model->species.at(index_to_id[i]).name+" ["+index_to_id[i]+"] ("+compart_name+")");
						}
					}
				}
			}

			// Draw reaction-related stuff
			if((freeze!=-1 && (ereac_frozen==freeze || espec_frozen==freeze)) 
				|| (freeze==-1 && reacs_to_erspecs.find(hit)!=reacs_to_erspecs.end())) // if the purpose is to draw the frozen stuff, draw it
			{
				int target;
				if(ereac_frozen!=-1) target = ereac_frozen;
				else target = hit;
				// if reaction frozen, draw it and its causing specs
				if(!draw_legend_or_line) // draw graphics (reaction)
				{
					// Draw the reaction

					double xr = x_0 + (coors[0][target]-scr_x0)*rat;
					double yr = y_1 - (coors[1][target]-scr_y0)*rat;
					n.add_filled_circle(xr,yr,high_node_r,reac_off_c);

					// Draw its causing specs as crosses
					for(const auto &i:espec_group)
					{
						double x_c = x_0 + (coors[0][i]-scr_x0)*rat;
						double y_c = y_1 - (coors[1][i]-scr_y0)*rat;

						n.add_filled_cross(x_c, y_c, ers_crosshand_l, ers_crosshand_w, error_border_c);
						n.add_filled_cross(x_c, y_c, ers_crosshand_l-ers_cross_border_w*0.5, ers_crosshand_w-ers_cross_border_w, al_map_rgb(255,255,255));
						n.add_filled_cross(x_c, y_c, ers_crosshand_l-ers_cross_border_w, ers_crosshand_w-ers_cross_border_w*2, error_cross_c);
					}
				}
				else // draw legend
				{
					reac_info.push_back("$tSpecies inducing inconsitency in "+model->reactions.at(index_to_id[target]).name+" ["+index_to_id[target]+"]");
					for(const auto &i:espec_group)
					{
						string compart_name = (model->species.at(index_to_id[i]).compart.empty()) ? "Unassigned compartment" : model->compartments.at(model->species.at(index_to_id[i]).compart).name;
						reac_info.push_back(model->species.at(index_to_id[i]).name+" ["+index_to_id[i]+"] ("+compart_name+")");
					}
				}
				
			}

			if(draw_legend_or_line)
			{
				if(BMLM_info.size()==0) // draw reaction - causing specs info
				{
					win->Change_text(reac_info);
				}
				else // draw bm / lm info
				{
					win->Change_text(BMLM_info);
				}
			}
		}
		else if (T_MODE == non_curious)
		{
			COL c = (hit < sumreac) ? ((dead_reacs[hit]) ? reac_off_c : reac_on_c) : ((dead_specs[hit - sumreac]) ? spec_off_c : spec_on_c);
			n.add_filled_circle(x0, y0, node_r*1.61, c);
			string s;
			if (draw_legend_or_line && freeze == -1)
			{
				if (win->get_mode() != Window<Explorer>::add_compart && win->get_mode() != Window<Explorer>::add_reac && win->get_mode() != Window<Explorer>::add_spec)
				{
					freeze = hit;
					add_or_edit(0, s);
					freeze = -1;
				}
			}
			if (draw_legend_or_line && freeze != -1)
			{
				if(win->get_mode()==Window<Explorer>::neighbour_info) add_or_edit(0, s);
			}
		}
        n.draw();
        al_set_clipping_rectangle(0,0,al_get_bitmap_width(al_get_backbuffer(display)),al_get_bitmap_height(al_get_backbuffer(display)));
    }
}

void Explorer::draw_spec_info(int hit)
{
	if (hit >= 0)
	{
		double x_0, y_0, x_1, y_1;
		win->Get_cust_coors(x_0, y_0, x_1, y_1);

		string info;
		if (hit < sumreac) info += model->reactions.at(index_to_id[hit]).name + " [" + index_to_id[hit] + "]";
		else
		{
			string compart_name = (model->species.at(index_to_id[hit]).compart.empty()) ? "Unassigned compartment" : model->compartments.at(model->species.at(index_to_id[hit]).compart).name;
			info += model->species.at(index_to_id[hit]).name + " [" + index_to_id[hit] + "] (" + compart_name + ")";
		}

		BITMAP * temp;
		temp = al_create_bitmap(al_get_text_width(specinfo_font, info.c_str()) + specinfo_font_size, specinfo_font_size*1.5);
		al_set_target_bitmap(temp);
		al_clear_to_color(graph_bckgr_c);
		double h_mar, v_mar;
		win->Get_cust_margins(h_mar, v_mar);
		al_draw_textf(specinfo_font, specinfo_c, specinfo_font_size*0.5, 0, ALLEGRO_ALIGN_LEFT, "%s", info.c_str());
		al_set_target_bitmap(al_get_backbuffer(display));
		al_draw_bitmap(temp, x_0 + h_mar, y_0 + v_mar, 0);
		if(temp!=NULL) al_destroy_bitmap(temp);
	}
}

void Explorer::draw_comp_info(const vector<string> &hits)
{
    double x_0, y_0, x_1, y_1;
    win->Get_cust_coors(x_0,y_0,x_1,y_1);

    string info;
    for(const string &s:hits)
    {
        if(model->compartments.at(s).name.empty()) info += s + ", ";
        else info += model->compartments.at(s).name + ", ";
    }
    for(int i=0;i<2;i++) info.pop_back();

    BITMAP * temp;
    temp = al_create_bitmap(al_get_text_width(specinfo_font,info.c_str())+specinfo_font_size,specinfo_font_size*1.5);
    al_set_target_bitmap(temp);
    al_clear_to_color(graph_bckgr_c);
    double h_mar, v_mar;
    win->Get_cust_margins(h_mar,v_mar);
    al_draw_textf(specinfo_font,compart_line_c,specinfo_font_size*0.5,0,ALLEGRO_ALIGN_LEFT,"%s",info.c_str());
    al_set_target_bitmap(al_get_backbuffer(display));
    al_draw_bitmap(temp,x_0+h_mar,y_0+v_mar+specinfo_font_size*1.5,0);
    if(temp!=NULL) al_destroy_bitmap(temp);
}

bool Explorer::hit_node(int &hit)
{
    double x_0, y_0, x_1, y_1;
    win->Get_cust_coors(x_0,y_0,x_1,y_1);
    if(mouse_x>x_0 && mouse_x<x_1 && mouse_y>y_0 && mouse_y<y_1)
    {
        double x_res = x_1-x_0;
        double y_res = y_1-y_0;
        double scr_x0 = scr_c_x-0.5*scr_w;
        double scr_y0 = scr_c_y-0.5*scr_w*y_res/x_res;
        double rat = x_res/scr_w;
        for(int i=0;i<sumtot;i++)
        {
            double cir_x = x_0 + (coors[0][i]-scr_x0)*rat;
            double cir_y = y_1 - (coors[1][i]-scr_y0)*rat;
            bool node_inside = (cir_x>x_0 && cir_x<x_1 && cir_y>y_0 && cir_y<y_1);
            if(node_inside)
            {
                double dist = sqrt(pow(mouse_x-cir_x,2)+pow(mouse_y-cir_y,2));
                if(dist<node_r)
                {
                    hit = i;
                    return true;
                }
            }
        }
		int iter = -2;
		for (const auto& i : cmprts)
		{
			vector<Point> polygon;
			for (size_t j = 0; j<i.second.size(); j++) polygon.push_back(Point(x_0 + (i.second[j].x - scr_x0)*rat, y_1 - (i.second[j].y - scr_y0)*rat));
			if (i.second.size()>2)
			{
				for (size_t j = 1; j < i.second.size(); j++)
				{
					if (inside_line(Point(mouse_x, mouse_y), polygon[j], polygon[j - 1], compart_line_t))
					{
						hit = iter; 
						return true;
					}
				}
				if (inside_line(Point(mouse_x, mouse_y), polygon.back(), polygon.front(), compart_line_t))
				{
					hit = iter;
					return true;
				}
			}
			else if (i.second.size() == 2)
			{
				if ((Point(mouse_x,mouse_y) - polygon[0]).length()<compart_line_t || (Point(mouse_x, mouse_y) - polygon[1]).length()<compart_line_t)
				{
					hit = iter;
					return true;
				}
			}
			else if (i.second.size() == 1 && (Point(mouse_x, mouse_y) - polygon[0]).length() < compart_line_t)
			{
				hit = iter;
				return true;
			}
			iter--;
		}

        return false;
    }
    else return false;
};

int Explorer::load_FBA_model(string filename)
{
    clear_FBA_model();
	if (model->load(filename) != 0) return 1;

    string name = VERSION + " - " + ((filename.empty()) ? "untitled" : filename);
    if(display==NULL) al_set_new_window_title(&name[0]);
    else al_set_window_title(display,&name[0]);

    if(flags.find("-ps")!=flags.end()) 
    {
        printf(A_YEL "\nPurged %i disconnected species before processing!\n" A_RES, model->purge_disconnected_species());
    }
	if(flags.find("-time")!=flags.end()) print_times = true;
	else print_times = false;
	if(flags.find("-FPS")!=flags.end()) print_fps = true;

    FILE_NAME = filename;
    update_arrays();

    if(flags.find("-FBAonly")!=flags.end()) 
    {
        update_blocked(false);
    }
    else update_blocked(true);

    return 0;
}

int Explorer::save_FBA_model(string filename)
{
    if(model!=NULL)
    {
        if(model->save(filename)!=0) return 1;
        FILE_NAME = filename;
    }
    return 0;
}

int Explorer::save_SVG(string path_to_file)
{
	double x0, y0, x1, y1;
	win->Get_cust_coors(x0, y0, x1, y1);
	SVG s = draw_to_SVG(x1-x0, y1-y0);
	s.save(path_to_file);
	return 0;
}

int Explorer::clear_FBA_model()
{
    search_target = -1;
    freeze = -1;
    prev_hit = -1;
    if(model!=NULL) delete model;
    model = new FBA_model;
    return 0;
}

void Explorer::find_subgraph(const int &n, const int &curr_group,const vector<vector<int> > &adjacency,vector<int> &subgraphs)
{
    if(subgraphs[n]==0)
    {
        subgraphs[n]=curr_group;
        for(const int &i:adjacency[n]) 
        {
            find_subgraph(i,curr_group,adjacency,subgraphs);
        }
    }
}

int Explorer::make_layout()
{
    sumreac = model->get_unmasked_rnum();
	sumspec = model->get_unmasked_snum();
	sumtot = sumreac+sumspec;

	/// Find the largest subgraph

    vector<int> subgraph_sizes(num_subgraphs,0);
    for(const int &i:subgraphs) subgraph_sizes[i-1]++;
    int largest_cluster = 1;
    double largest_size = subgraph_sizes[0];
    for(size_t i=0;i<subgraph_sizes.size();i++)
    {
        if(subgraph_sizes[i]>largest_size) 
        {
            largest_cluster = i+1;
            largest_size = subgraph_sizes[i];
        }
    }

    /// Find the boundary species for the different compartments
    map<string,set<int>> compart_boundary_species;
    for(const auto &i:model->compartments) compart_boundary_species.emplace(i.first,set<int>());
    for(const auto &i:model->reactions)
    {
		if(!i.second.ismasked())
		{
			set<string> comps; 
			for(const auto &j:model->reactions.at(i.first).reactants) 
			{
				if(!model->species.at(j.first).ismasked())
				{
					string c = model->species.at(j.first).compart;
					if(!c.empty()) comps.insert(c);
				}
			}
			for(const auto &j:model->reactions.at(i.first).products) 
			{
				if(!model->species.at(j.first).ismasked())
				{
					string c = model->species.at(j.first).compart;
					if(!c.empty()) comps.insert(c);
				}
			}
			if(comps.size()>1)
			{
				for(const auto &j:model->reactions.at(i.first).reactants) 
				{
					if(!model->species.at(j.first).ismasked())
					{
						if(subgraphs[sumreac+s_id_to_index.at(j.first)]==largest_cluster) compart_boundary_species.at(model->species.at(j.first).compart).insert(s_id_to_index.at(j.first));
					}
				}
				for(const auto &j:model->reactions.at(i.first).products) 
				{
					if(!model->species.at(j.first).ismasked())
					{
						if(subgraphs[sumreac+s_id_to_index.at(j.first)]==largest_cluster) compart_boundary_species.at(model->species.at(j.first).compart).insert(s_id_to_index.at(j.first));
					}
				}
			}
		}
    }
 
    /// ///////////////////// Create a visual layout of the model ////////////////////////////////////////

    coors.clear();

    GVC_t * gvc;
    gvc = gvContext();

    Agraph_t * G;
    char G_name [2] = "G";
    G = agopen(G_name,Agundirected,NULL);

    char r [20] = "repulsiveforce";
    char rv [10] = "1.10";
    agsafeset(G,r,rv,rv);

    vector<Agnode_t *> VN (sumtot);
    vector<Agedge_t *> VE (links.size());
    for(int i=0;i<sumtot;i++)
    {
        char name [128];
        sprintf(name,"%i",i);
        VN[i] = agnode(G,name,TRUE);
    }

    for(int i=0;i<(int)links.size();i++)
    {
        char name [128];
        sprintf(name,"%i",i);
        VE[i] = agedge(G,VN[links[i][0]],VN[links[i][1]],name,TRUE);
    }

    for(const auto &c:compart_boundary_species)
    {
        string id = c.first;
        string name = model->compartments.at(c.first).name;
        if(c.second.size()>0 && id!="e" && id!="E" && id.find("extracell")==string::npos && id.find("Extracell")==string::npos && name.find("extracell")==string::npos && name.find("Extracell")==string::npos && name.find("Extracell")==string::npos)
        {
            for(int k=0;k<2;k++)
            {
                char name [128];
                sprintf(name,"%s-%i-center",c.first.c_str(),k);
                VN.resize(VN.size()+1);
                VN.back() = agnode(G,name,TRUE);
                for(const int &i:c.second)
                {
                    char name [128];
                    sprintf(name,"%s-%i-%i",c.first.c_str(),k,i);
                    VE.resize(VE.size()+1);
                    VE.back() = agedge(G,VN.back(),VN[i+sumreac],name,TRUE);  
                }
            }
        }
    }

    gvLayout(gvc,G,"sfdp");
    gvRender(gvc,G,"dot",NULL);

    coors.resize(2);
    for(auto& i:coors) i.resize(sumtot);
    for(int i=0;i<sumtot;i++)
    {
        string coor = agget(VN[i],(char*)"pos");
        int pos = coor.find(",");
        string x = coor.substr(0,pos);
        string y = coor.substr(pos+1);

        coors[0][i]=atof(x.c_str());
        coors[1][i]=atof(y.c_str());
    }

    /// Update internal coordinates inside the model 
    int iter = 0;
    for(auto &i:model->reactions)
    {
		if(!i.second.ismasked())
		{
			i.second.setx(coors[0][iter]);
			i.second.sety(coors[1][iter]);
			iter++;
		}
    }
    for(auto &i:model->species)
    {
		if(!i.second.ismasked())
		{
			i.second.setx(coors[0][iter]);
			i.second.sety(coors[1][iter]);
			iter++;
		}
    }

    /// Update the compartment drawing vectors

	update_compartments();

    double max_x = *max_element(coors[0].begin(),coors[0].end());
    double min_x = *min_element(coors[0].begin(),coors[0].end());
    double max_y = *max_element(coors[1].begin(),coors[1].end());
    double min_y = *min_element(coors[1].begin(),coors[1].end());

    scr_c_x = 0.5*(min_x+max_x);
    scr_c_y = 0.5*(min_y+max_y);

    double x0, y0, x1, y1;
    win->Get_cust_coors(x0,y0,x1,y1);

    double del_x = max_x-min_x;
    double del_y = max_y-min_y;
    if((del_y/del_x)<((y1-y0)/(x1-x0))) scr_w = del_x*1.05;
    else scr_w = del_y*(x1-x0)/(y1-y0)*1.05;

    gvFreeLayout(gvc,G);
    agclose(G);

    /// ///////////////////////////////////////////////////////////////

    scr_upd = true;
    return 0;
}

void Explorer::update_compartments()
{
	cmprts.clear();

	if (num_subgraphs > 0)
	{
		vector<int> subgraph_sizes(num_subgraphs, 0);
		for (const int &i : subgraphs) subgraph_sizes[i - 1]++;
		int largest_cluster = 1;
		double largest_size = subgraph_sizes[0];
		for (size_t i = 0; i < subgraph_sizes.size(); i++)
		{
			if (subgraph_sizes[i] > largest_size)
			{
				largest_cluster = i + 1;
				largest_size = subgraph_sizes[i];
			}
		}

		for (const auto &i : model->compartments) cmprts[i.first] = vector<Point>();
		for (const auto &i : model->species)
		{
			if (!i.second.ismasked() && !i.second.compart.empty()) cmprts.at(i.second.compart).push_back(Point(i.second.getx(), i.second.gety()));
		}
		for (auto &i : cmprts) i.second = convex_hull(i.second);

		vector<string> empty_hulls;
		for (const auto &i : cmprts)
		{
			if (i.second.empty()) empty_hulls.push_back(i.first);
			else
			{
				string id = i.first;
				string name = model->compartments.at(i.first).name;
				if (id == "e" || id == "E" || id.find("extracell") != string::npos || id.find("Extracell") != string::npos || name.find("extracell") != string::npos || name.find("Extracell") != string::npos) empty_hulls.push_back(i.first);
			}
		}
		for (const string &s : empty_hulls) cmprts.erase(s);
	}
}

int Explorer::draw_to_screen(double x_0,double y_0,double x_1,double y_1)
{
    if(scr_upd)
    {
        al_set_target_bitmap(al_get_backbuffer(display));
        al_set_clipping_rectangle(x_0,y_0+1.0,x_1,y_1);
        al_draw_filled_rectangle(x_0,y_0,x_1,y_1,graph_bckgr_c);

        Nikal n;
        Nikal f;

        /// ///////////////////////////////////////////////////////// ///

        double x_res = x_1-x_0;
        double y_res = y_1-y_0;

        double scr_x0 = scr_c_x-0.5*scr_w;
        double scr_y0 = scr_c_y-0.5*scr_w*y_res/x_res;
        double rat = x_res/scr_w;

        double bound_x0 = x_0-node_r; /// Arrow Bounds
        double bound_x1 = x_1+node_r;
        double bound_y0 = y_0-node_r;
        double bound_y1 = y_1+node_r;

        /// Draw arrows
        for(int i=0;i<(int)links.size();i++)
        {
			if(graphics_mode==1) draw_arrow(n,arrow_c,graphics_mode,x_0,y_0,x_1,y_1,bound_x0,bound_y0,bound_x1,bound_y1,rat,scr_x0,scr_y0,links[i][0],links[i][1]);
			else draw_arrow(f,arrow_c,graphics_mode,x_0,y_0,x_1,y_1,bound_x0,bound_y0,bound_x1,bound_y1,rat,scr_x0,scr_y0,links[i][0],links[i][1]);
        }

        if(graphics_mode==0)
        {

            f.draw();
            al_set_target_bitmap(fastg_sprite);
            al_clear_to_color(graph_bckgr_c);
            al_draw_bitmap_region(al_get_backbuffer(display),x_0,y_0,(x_1-x_0)*q_coeff,(y_1-y_0)*q_coeff,0,0,0);
            al_set_target_bitmap(al_get_backbuffer(display));
            al_draw_filled_rectangle(x_0,y_0,x_1,y_1,graph_bckgr_c);

            int a, b, c, d, e, f;
            al_get_separate_blender(&a,&b,&c,&d,&e,&f);
            al_set_separate_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO, ALLEGRO_ADD, ALLEGRO_ZERO, ALLEGRO_ONE);
            al_draw_scaled_bitmap(fastg_sprite,0,0,(x_1-x_0)*q_coeff,(y_1-y_0)*q_coeff,x_0,y_0,x_1-x_0,y_1-y_0,0);
            al_set_separate_blender(a,b,c,d,e,f);
        }
        else  n.draw();

        /// Draw nodes

        for(int i=0;i<sumtot;i++)
        {
            double cir_x = x_0 + (coors[0][i]-scr_x0)*rat;
            double cir_y = y_1 - (coors[1][i]-scr_y0)*rat;
            bool node_inside = (cir_x>bound_x0 && cir_x<bound_x1 && cir_y>bound_y0 && cir_y<bound_y1);
            if(node_inside)
            {
                if(i<sumreac) n.add_filled_circle(cir_x,cir_y,node_r,(dead_reacs[i]) ? reac_off_c : reac_on_c);
                else
                {
                    COL col = (spec_void[i-sumreac]) ? void_c : ((dead_specs[i-sumreac]) ? spec_off_c : spec_on_c);
                    n.add_filled_circle(cir_x,cir_y,node_r,col);
					if (uninitiated[i - sumreac])
					{
						double t = 0.5*(uninit_node_r - node_r);
						n.add_circle(cir_x, cir_y, uninit_node_r - t, uninit_c, t*2.0);
					}

                }
            }
        }

        /// Draw compartments
        if(draw_comparts)
        {
            for(const auto &i:cmprts)
            {
                vector<Point> polygon;
                for(size_t j=0;j<i.second.size();j++) polygon.push_back(Point(x_0 + (i.second[j].x-scr_x0)*rat, y_1 - (i.second[j].y-scr_y0)*rat));
                if(i.second.size()>2)
                {
                    for(size_t j=1;j<i.second.size();j++) n.add_line(polygon[j].x,polygon[j].y,polygon[j-1].x,polygon[j-1].y,compart_line_c,compart_line_t);
                    n.add_line(polygon.back().x,polygon.back().y,polygon.front().x,polygon.front().y,compart_line_c,compart_line_t);
                }
                else if(i.second.size()==2)
                {
                    n.add_circle(polygon[0].x,polygon[0].y,compart_r,compart_line_c,compart_line_t);
                    n.add_circle(polygon[1].x,polygon[1].y,compart_r,compart_line_c,compart_line_t);
                }
                else if(i.second.size()==1) n.add_circle(polygon[0].x,polygon[0].y,compart_r,compart_line_c,compart_line_t);
            }
        }

        /// Draw objective(s)

        for(int i:objectives)
        {
            double cir_x = x_0 + (coors[0][i]-scr_x0)*rat;
            double cir_y = y_1 - (coors[1][i]-scr_y0)*rat;
            bool node_inside = (cir_x>bound_x0 && cir_x<bound_x1 && cir_y>bound_y0 && cir_y<bound_y1);
            if(node_inside)
            {
                n.add_filled_circle(cir_x,cir_y,obj_node_r,al_map_rgb(0,0,0));
                n.add_filled_circle(cir_x,cir_y,obj_node_r-1,obj_reac_c);
                n.add_filled_circle(cir_x,cir_y,node_r,(dead_reacs[i]) ? reac_off_c : reac_on_c);
            }
        }

        /// Draw search target
        if(search_target!=-1)
        {
            double cir_x = x_0 + (coors[0][search_target]-scr_x0)*rat;
            double cir_y = y_1 - (coors[1][search_target]-scr_y0)*rat;
            bool node_inside = (cir_x>bound_x0 && cir_x<bound_x1 && cir_y>bound_y0 && cir_y<bound_y1);
            if(node_inside)
            {
                double len = sqrt(pow(x_0-cir_x,2)+pow(y_1-cir_y,2));
                double tar_x = cir_x + (x_0-cir_x)/len*search_node_r;
                double tar_y = cir_y + (y_1-cir_y)/len*search_node_r;
                n.add_line(x_0,y_1,tar_x,tar_y,search_c,search_node_th);
                n.add_circle(cir_x,cir_y,search_node_r,search_c,search_node_th);
            }
        }

        n.draw();

        al_set_clipping_rectangle(0,0,al_get_bitmap_width(al_get_backbuffer(display)),al_get_bitmap_height(al_get_backbuffer(display)));

        al_set_target_bitmap(canvas);
        al_draw_bitmap(al_get_backbuffer(display),0,-y_0,0);
        al_set_target_bitmap(al_get_backbuffer(display));
        al_draw_bitmap(canvas,0,y_0,0);
    }
    else
    {
        al_set_target_bitmap(al_get_backbuffer(display));
        al_draw_bitmap(canvas,0,y_0,0);
    }
    return 0;
}

template<typename T>
void Explorer::draw_arrow(T &n, COL ar_c, int g_mode, double x_0,double y_0,double x_1,double y_1, double bound_x0, double bound_y0, double bound_x1, double bound_y1, double rat, double scr_x0, double scr_y0, int beg, int end)
{
	double x0 = x_0 + (coors[0][beg]-scr_x0)*rat;
	double y0 = y_1-(coors[1][beg]-scr_y0)*rat;
	double x1 = x_0 + (coors[0][end]-scr_x0)*rat;
	double y1 = y_1-(coors[1][end]-scr_y0)*rat;

	/// Check if either node is inside the view

	bool node_0_inside = (x0>bound_x0 && x0<bound_x1 && y0>bound_y0 && y0<bound_y1);
	bool node_1_inside = (x1>bound_x0 && x1<bound_x1 && y1>bound_y0 && y1<bound_y1);
	if(node_0_inside || node_1_inside)
	{
		double a = arrow_l;
		double b = arrow_w;

		double vec [2] = {x1-x0,y1-y0};
		double l = sqrt(pow(vec[0],2)+pow(vec[1],2));
		double u [2] = {vec[0]/l,vec[1]/l};
		double v [2] = {-u[1],u[0]};

		double xa, ya, xb, yb, xc, yc;
		double scal_vec [4];
		double scal;
 
		bool rever = false;
		if(beg<sumreac) rever = reversible[beg];
		else if(end<sumreac) rever = reversible[end];

		if(rever) /// reversible
		{
			if(node_1_inside && !node_0_inside)
			{
				xa = x1;
				ya = y1;
				xb = x0+(l-a)*u[0] + b*v[0];
				yb = y0+(l-a)*u[1] + b*v[1];
				xc = x0+(l-a)*u[0] - b*v[0];
				yc = y0+(l-a)*u[1] - b*v[1];

				if(g_mode==1) n.add_filled_triangle(xa,ya,xb,yb,xc,yc,ar_c);
				else n.add_filled_triangle((xa-x_0)*q_coeff+x_0,(ya-y_0)*q_coeff+y_0,(xb-x_0)*q_coeff+x_0,(yb-y_0)*q_coeff+y_0,(xc-x_0)*q_coeff+x_0,(yc-y_0)*q_coeff+y_0,ar_c);

				scal_vec[0] = (x1-x0) / (x1-bound_x0);
				scal_vec[1] = (x1-x0) / (x1-bound_x1);
				scal_vec[2] = (y1-y0) / (y1-bound_y0);
				scal_vec[3] = (y1-y0) / (y1-bound_y1);
				scal = 1.0 -1.0 / *max_element(scal_vec,scal_vec+4);

				xa = x0+vec[0]*scal;
				ya = y0+vec[1]*scal;
				xb = x1-u[0]*a;
				yb = y1-u[1]*a;
			}
			else if(!node_1_inside && node_0_inside)
			{
				u[0]=-u[0];
				u[1]=-u[1];
				v[0]=-v[0];
				v[1]=-v[1];

				xa = x0;
				ya = y0;
				xb = x1+(l-a)*u[0] + b*v[0];
				yb = y1+(l-a)*u[1] + b*v[1];
				xc = x1+(l-a)*u[0] - b*v[0];
				yc = y1+(l-a)*u[1] - b*v[1];

				if(g_mode==1) n.add_filled_triangle(xa,ya,xb,yb,xc,yc,ar_c);
				else n.add_filled_triangle((xa-x_0)*q_coeff+x_0,(ya-y_0)*q_coeff+y_0,(xb-x_0)*q_coeff+x_0,(yb-y_0)*q_coeff+y_0,(xc-x_0)*q_coeff+x_0,(yc-y_0)*q_coeff+y_0,ar_c);


				scal_vec[0] = (x0-x1) / (x0-bound_x0);
				scal_vec[1] = (x0-x1) / (x0-bound_x1);
				scal_vec[2] = (y0-y1) / (y0-bound_y0);
				scal_vec[3] = (y0-y1) / (y0-bound_y1);
				scal = 1.0 -1.0 / *max_element(scal_vec,scal_vec+4);

				xa = x0-u[0]*a;
				ya = y0-u[1]*a;
				xb = x1-vec[0]*scal;
				yb = y1-vec[1]*scal;
			}
			else
			{
				if(l>(2*a))
				{
					xa = x1;
					ya = y1;
					xb = x0+(l-a)*u[0] + b*v[0];
					yb = y0+(l-a)*u[1] + b*v[1];
					xc = x0+(l-a)*u[0] - b*v[0];
					yc = y0+(l-a)*u[1] - b*v[1];

					if(g_mode==1) n.add_filled_triangle(xa,ya,xb,yb,xc,yc,ar_c);
					else n.add_filled_triangle((xa-x_0)*q_coeff+x_0,(ya-y_0)*q_coeff+y_0,(xb-x_0)*q_coeff+x_0,(yb-y_0)*q_coeff+y_0,(xc-x_0)*q_coeff+x_0,(yc-y_0)*q_coeff+y_0,ar_c);

					u[0]=-u[0];
					u[1]=-u[1];
					v[0]=-v[0];
					v[1]=-v[1];

					xa = x0;
					ya = y0;
					xb = x1+(l-a)*u[0] + b*v[0];
					yb = y1+(l-a)*u[1] + b*v[1];
					xc = x1+(l-a)*u[0] - b*v[0];
					yc = y1+(l-a)*u[1] - b*v[1];

					if(g_mode==1) n.add_filled_triangle(xa,ya,xb,yb,xc,yc,ar_c);
					else n.add_filled_triangle((xa-x_0)*q_coeff+x_0,(ya-y_0)*q_coeff+y_0,(xb-x_0)*q_coeff+x_0,(yb-y_0)*q_coeff+y_0,(xc-x_0)*q_coeff+x_0,(yc-y_0)*q_coeff+y_0,ar_c);

					xa = x0-u[0]*a;
					ya = y0-u[1]*a;
					xb = x1+u[0]*a;
					yb = y1+u[1]*a;
				}
				else
				{
					xa = x0;
					ya = y0;
					xb = x1;
					yb = y1;
				}
			}
		}
		else
		{
			if(node_1_inside && !node_0_inside)
			{
				xa = x1;
				ya = y1;
				xb = x0+(l-a)*u[0] + b*v[0];
				yb = y0+(l-a)*u[1] + b*v[1];
				xc = x0+(l-a)*u[0] - b*v[0];
				yc = y0+(l-a)*u[1] - b*v[1];

				if(g_mode==1) n.add_filled_triangle(xa,ya,xb,yb,xc,yc,ar_c);
				else n.add_filled_triangle((xa-x_0)*q_coeff+x_0,(ya-y_0)*q_coeff+y_0,(xb-x_0)*q_coeff+x_0,(yb-y_0)*q_coeff+y_0,(xc-x_0)*q_coeff+x_0,(yc-y_0)*q_coeff+y_0,ar_c);

				scal_vec[0] = (x1-x0) / (x1-bound_x0);
				scal_vec[1] = (x1-x0) / (x1-bound_x1);
				scal_vec[2] = (y1-y0) / (y1-bound_y0);
				scal_vec[3] = (y1-y0) / (y1-bound_y1);
				scal = 1.0 -1.0 / *max_element(scal_vec,scal_vec+4);

				xa = x0-vec[0]*scal;
				ya = y0-vec[1]*scal;
				xb = x1-u[0]*a;
				yb = y1-u[1]*a;
			}
			else if(!node_1_inside && node_0_inside)
			{
				scal_vec[0] = (x0-x1) / (x0-bound_x0);
				scal_vec[1] = (x0-x1) / (x0-bound_x1);
				scal_vec[2] = (y0-y1) / (y0-bound_y0);
				scal_vec[3] = (y0-y1) / (y0-bound_y1);
				scal = 1.0 -1.0 / *max_element(scal_vec,scal_vec+4);

				xa = x0;
				ya = y0;
				xb = x1-vec[0]*scal;
				yb = y1-vec[1]*scal;
			}
			else
			{
				if(l>a)
				{
					xa = x1;
					ya = y1;
					xb = x0+(l-a)*u[0] + b*v[0];
					yb = y0+(l-a)*u[1] + b*v[1];
					xc = x0+(l-a)*u[0] - b*v[0];
					yc = y0+(l-a)*u[1] - b*v[1];

					if(g_mode==1) n.add_filled_triangle(xa,ya,xb,yb,xc,yc,ar_c);
					else n.add_filled_triangle((xa-x_0)*q_coeff+x_0,(ya-y_0)*q_coeff+y_0,(xb-x_0)*q_coeff+x_0,(yb-y_0)*q_coeff+y_0,(xc-x_0)*q_coeff+x_0,(yc-y_0)*q_coeff+y_0,ar_c);

					xa = x0;
					ya = y0;
					xb = x1-u[0]*a;
					yb = y1-u[1]*a;
				}
				else
				{
					xa = x0;
					ya = y0;
					xb = x1;
					yb = y1;
				}
			}
		}

		x0 = xa;
		x1 = xb;
		y0 = ya;
		y1 = yb;

		if(node_1_inside && !node_0_inside)
		{
			double slope = (y0-y1)/(x0-x1);
			if(x0<bound_x0) 
			{
				y0 = y1 + (bound_x0-x1)*slope;
				x0 = bound_x0;
			}
			if(y0<bound_y0)
			{
				x0 = x1 + (bound_y0-y1)/slope;
				y0 = bound_y0;
			}
		}
		else if(!node_1_inside && node_0_inside)
		{
			double slope = (y1-y0)/(x1-x0);
			if(x1>bound_x1) 
			{
				y1 = y0 + (bound_x1-x0)*slope;
				x1 = bound_x1;
			}
			if(y1>bound_y1)
			{
				x1 = x0 + (bound_y1-y0)/slope;
				y1 = bound_y1;
			}
		}

		if(g_mode==1) n.add_line(x0,y0,x1,y1,ar_c,line_w);
		else n.add_line((x0-x_0)*q_coeff+x_0,(y0-y_0)*q_coeff+y_0,(x1-x_0)*q_coeff+x_0,(y1-y_0)*q_coeff+y_0,ar_c,line_w*q_coeff);
	}
}

SVG Explorer::draw_to_SVG(double x_res, double y_res)
{
	SVG n(x_res, y_res);

	n.add_filled_rectangle(0, 0, x_res, y_res, graph_bckgr_c);

	/// ///////////////////////////////////////////////////////// ///

	double scr_x0 = scr_c_x - 0.5*scr_w;
	double scr_y0 = scr_c_y - 0.5*scr_w*y_res / x_res;
	double rat = x_res / scr_w;

	double bound_x0 = 0 - node_r; /// Arrow Bounds
	double bound_x1 = x_res + node_r;
	double bound_y0 = 0 - node_r;
	double bound_y1 = y_res + node_r;

	/// Draw arrows
	for (int i = 0; i<(int)links.size(); i++)
	{
		draw_arrow(n,arrow_c,1,0,0,x_res,y_res,bound_x0,bound_y0,bound_x1,bound_y1,rat,scr_x0,scr_y0,links[i][0],links[i][1]);
	}

	/// Draw nodes

	for (int i = 0; i<sumtot; i++)
	{
		double cir_x = 0 + (coors[0][i] - scr_x0)*rat;
		double cir_y = y_res - (coors[1][i] - scr_y0)*rat;
		bool node_inside = (cir_x>bound_x0 && cir_x<bound_x1 && cir_y>bound_y0 && cir_y<bound_y1);
		if (node_inside)
		{
			if (i<sumreac) n.add_filled_circle(cir_x, cir_y, node_r, (dead_reacs[i]) ? reac_off_c : reac_on_c);
			else
			{
				COL col = (spec_void[i - sumreac]) ? void_c : ((dead_specs[i - sumreac]) ? spec_off_c : spec_on_c);
				n.add_filled_circle(cir_x, cir_y, node_r, col);
				if (uninitiated[i - sumreac])
				{
					double t = 0.5*(uninit_node_r - node_r);
					n.add_circle(cir_x, cir_y, uninit_node_r - t, uninit_c,t*2.0);
				}
			}
		}
	}

	/// Draw compartments
	if (draw_comparts)
	{
		for (const auto &i : cmprts)
		{
			vector<Point> polygon;
			for (size_t j = 0; j < i.second.size(); j++) polygon.push_back(Point(0 + (i.second[j].x - scr_x0)*rat, y_res - (i.second[j].y - scr_y0)*rat));
			if (i.second.size()>2)
			{
				for (size_t j = 1; j<i.second.size(); j++) n.add_line(polygon[j].x, polygon[j].y, polygon[j - 1].x, polygon[j - 1].y, compart_line_c, compart_line_t);
				n.add_line(polygon.back().x, polygon.back().y, polygon.front().x, polygon.front().y, compart_line_c, compart_line_t);
			}
			else if (i.second.size() == 2)
			{
				n.add_circle(polygon[1].x, polygon[1].y, compart_r, compart_line_c, compart_line_t);
				n.add_circle(polygon[1].x, polygon[1].y, compart_r, compart_line_c, compart_line_t);
			}
			else if (i.second.size() == 1) n.add_circle(polygon[0].x, polygon[0].y, compart_r, compart_line_c, compart_line_t);
		}
	}

	/// Draw objective(s)

	for (int i : objectives)
	{
		double cir_x = 0 + (coors[0][i] - scr_x0)*rat;
		double cir_y = y_res - (coors[1][i] - scr_y0)*rat;
		bool node_inside = (cir_x>bound_x0 && cir_x<bound_x1 && cir_y>bound_y0 && cir_y<bound_y1);
		if (node_inside)
		{
			n.add_filled_circle(cir_x, cir_y, obj_node_r, al_map_rgb(0, 0, 0));
			n.add_filled_circle(cir_x, cir_y, obj_node_r - 1, obj_reac_c);
			n.add_filled_circle(cir_x, cir_y, node_r, (dead_reacs[i]) ? reac_off_c : reac_on_c);
		}
	}

	/// Draw search target
	if (search_target != -1)
	{
		double cir_x = 0 + (coors[0][search_target] - scr_x0)*rat;
		double cir_y = y_res - (coors[1][search_target] - scr_y0)*rat;
		bool node_inside = (cir_x>bound_x0 && cir_x<bound_x1 && cir_y>bound_y0 && cir_y<bound_y1);
		if (node_inside)
		{
			double len = sqrt(pow(0 - cir_x, 2) + pow(y_res - cir_y, 2));
			double tar_x = cir_x + (0 - cir_x) / len * search_node_r;
			double tar_y = cir_y + (y_res - cir_y) / len * search_node_r;
			n.add_line(0, y_res, tar_x, tar_y, search_c, search_node_th);
			n.add_circle(cir_x, cir_y, search_node_r, search_c, search_node_th);
		}
	}

	if (freeze >= 0)
	{
		double scr_x0 = scr_c_x - 0.5*scr_w;
		double scr_y0 = scr_c_y - 0.5*scr_w*y_res / x_res;
		double rat = x_res / scr_w;

		double bound_x0 = 0 - node_r;
		double bound_x1 = x_res + node_r;
		double bound_y0 = 0 - node_r;
		double bound_y1 = y_res + node_r;

		double x0 = 0 + (coors[0][freeze] - scr_x0)*rat;
		double y0 = y_res - (coors[1][freeze] - scr_y0)*rat;

		if (T_MODE == neighbour)
		{
			if (freeze<sumreac) /// draw reaction reactants and products
			{
				for (const int &i : reactants[freeze])
				{
					double x1 = 0 + (coors[0][i] - scr_x0)*rat;
					double y1 = y_res - (coors[1][i] - scr_y0)*rat;
					n.add_line(x0, y0, x1, y1, reactant_line_c, bold_line_w);
				}
				for (const int &i : products[freeze])
				{
					double x1 = 0 + (coors[0][i] - scr_x0)*rat;
					double y1 = y_res - (coors[1][i] - scr_y0)*rat;
					n.add_line(x0, y0, x1, y1, product_line_c, bold_line_w);
				}
			}
			else /// draw generating and consuming reactions
			{
				for (const int &i : o_products_of[freeze - sumreac])
				{
					double x1 = 0 + (coors[0][i] - scr_x0)*rat;
					double y1 = y_res - (coors[1][i] - scr_y0)*rat;
					n.add_line(x0, y0, x1, y1, reactant_line_c, bold_line_w);
				}
				for (const int &i : o_reactants_of[freeze - sumreac])
				{
					double x1 = 0 + (coors[0][i] - scr_x0)*rat;
					double y1 = y_res - (coors[1][i] - scr_y0)*rat;
					n.add_line(x0, y0, x1, y1, product_line_c, bold_line_w);
				}
				for (const int &i : p_and_r_of[freeze - sumreac])
				{
					double x1 = 0 + (coors[0][i] - scr_x0)*rat;
					double y1 = y_res - (coors[1][i] - scr_y0)*rat;
					n.add_line(x0, y0, x1, y1, reversbl_line_c, bold_line_w);
				}
			}
		}
		else if (T_MODE == ancestral)
		{
			Array<ancestor> tree = frozen_tree;

			/// Draw the ancestors of the species
			COL line_col = (tree_type==0 ? dead[freeze] : dead_out[freeze]) ? bold_line_c : liv_bold_line_c;
			for (auto& q : tree) for (ancestor& i : q)
			{
				if (!i.killed)
				{
					double x1 = 0 + (coors[0][i.num] - scr_x0)*rat;
					double y1 = y_res - (coors[1][i.num] - scr_y0)*rat;
					for (int_int j : i.parents)
					{
						if (!tree[j.x][j.y].killed)
						{
							double x2 = 0 + (coors[0][tree[j.x][j.y].num] - scr_x0)*rat;
							double y2 = y_res - (coors[1][tree[j.x][j.y].num] - scr_y0)*rat;
							bool node_1_inside = (x1>bound_x0 && x1<bound_x1 && y1>bound_y0 && y1<bound_y1);
							bool node_2_inside = (x2>bound_x0 && x2<bound_x1 && y2>bound_y0 && y2<bound_y1);
							double scal_vec[4];
							double scal;
							if (!node_1_inside && node_2_inside)
							{
								scal_vec[0] = (x2 - x1) / (x2 - bound_x0);
								scal_vec[1] = (x2 - x1) / (x2 - bound_x1);
								scal_vec[2] = (y2 - y1) / (y2 - bound_y0);
								scal_vec[3] = (y2 - y1) / (y2 - bound_y1);
								scal = 1.0 - 1.0 / *max_element(scal_vec, scal_vec + 4);
								n.add_line(x1 + (x2 - x1)*scal, y1 + (y2 - y1)*scal, x2, y2, line_col, bold_line_w);
							}
							else if (node_1_inside && !node_2_inside)
							{
								scal_vec[0] = (x1 - x2) / (x1 - bound_x0);
								scal_vec[1] = (x1 - x2) / (x1 - bound_x1);
								scal_vec[2] = (y1 - y2) / (y1 - bound_y0);
								scal_vec[3] = (y1 - y2) / (y1 - bound_y1);
								scal = 1.0 - 1.0 / *max_element(scal_vec, scal_vec + 4);
								n.add_line(x1, y1, x2 - (x2 - x1)*scal, y2 - (y2 - y1)*scal, line_col, bold_line_w);
							}
							else if (node_1_inside && node_2_inside)
							{
								n.add_line(x1, y1, x2, y2, line_col, bold_line_w);
							}
						}
					}
				}
			}
			for (auto& q : tree) for (ancestor& i : q)
			{
				if (!i.reac_or_mol && !i.killed && uninitiated[i.num - sumreac])
				{
					double x1 = 0 + (coors[0][i.num] - scr_x0)*rat;
					double y1 = y_res - (coors[1][i.num] - scr_y0)*rat;
					bool node_inside = (x1>bound_x0 && x1<bound_x1 && y1>bound_y0 && y1<bound_y1);
					if (node_inside) n.add_filled_circle(x1, y1, high_node_r, uninit_c);
				}
			}
		}
		else if (T_MODE == blocked_module)
		{
			/// Draw the blocked moduule, so that each line is only drawn once
			COL line_col = bold_line_c;
			for(const auto& i:frozen_bmodule) 
			{
				if(i<sumreac) /// iterate through every reaction in the dead module
				{
					double x1 = (coors[0][i]-scr_x0)*rat;
					double y1 = y_res - (coors[1][i]-scr_y0)*rat;
					for(const auto &j:neighbours[i]) 
					{
						if(frozen_bmodule.find(j)!=frozen_bmodule.end()) /// iterate through all dead species connected to the dead module
						{
							double x2 = (coors[0][j]-scr_x0)*rat;
							double y2 = y_res - (coors[1][j]-scr_y0)*rat;
							bool node_1_inside = (x1>bound_x0 && x1<bound_x1 && y1>bound_y0 && y1<bound_y1);
							bool node_2_inside = (x2>bound_x0 && x2<bound_x1 && y2>bound_y0 && y2<bound_y1);
							double scal_vec [4];
							double scal;
							if(!node_1_inside && node_2_inside)
							{
								scal_vec[0] = (x2-x1) / (x2-bound_x0);
								scal_vec[1] = (x2-x1) / (x2-bound_x1);
								scal_vec[2] = (y2-y1) / (y2-bound_y0);
								scal_vec[3] = (y2-y1) / (y2-bound_y1);
								scal = 1.0 -1.0 / *max_element(scal_vec,scal_vec+4);
								n.add_line(x1+(x2-x1)*scal,y1+(y2-y1)*scal,x2,y2,line_col,bold_line_w);
							}
							else if(node_1_inside && !node_2_inside)
							{
								scal_vec[0] = (x1-x2) / (x1-bound_x0);
								scal_vec[1] = (x1-x2) / (x1-bound_x1);
								scal_vec[2] = (y1-y2) / (y1-bound_y0);
								scal_vec[3] = (y1-y2) / (y1-bound_y1);
								scal = 1.0 -1.0 / *max_element(scal_vec,scal_vec+4);
								n.add_line(x1,y1,x2-(x2-x1)*scal,y2-(y2-y1)*scal,line_col,bold_line_w);
							}
							else if(node_1_inside && node_2_inside)
							{
								n.add_line(x1,y1,x2,y2,line_col,bold_line_w);
							}
						}
					}
				}
			}
		}
		else if (T_MODE == error_source && B_MODE==FBA && arrays_upd_after_ccheck==0)
		{
			// Draw BM-related stuff
			if(espec_frozen!=-1) 
			{
				int target = espec_frozen;

				int BM = erspecs.at(target).first.at(ereac_frozen).first;
				int LM = erspecs.at(target).first.at(ereac_frozen).second;
				
				// if the purpose is to draw someting else than the frozen stuff
				// and the mouse hovers over one of the causing specs, draw its BM(/LM)
				// Draw LM
				if(LM!=-1)
				{
					for(const auto &i:LMs.at(LM).first)
					{
						double x_r = (coors[0][i]-scr_x0)*rat;
						double y_r = y_res - (coors[1][i]-scr_y0)*rat;
						for(const auto &j:neighbours[i])
						{
							if(LMs.at(LM).second.find(j)!=LMs.at(LM).second.end())
							{
								string r = index_to_id[i];
								string s = index_to_id[j];
								bool rop = model->reactions.at(r).reactants.find(s)!=model->reactions.at(r).reactants.end();
								draw_arrow(n,LM_arrow_svg_c,1,0,0,x_res,y_res,bound_x0,bound_y0,bound_x1,bound_y1,rat,scr_x0,scr_y0,rop ? j : i,rop ? i : j);
							}
						}
	
					} 

					for(size_t i=0;i<sumreac;i++)
					{
						if(LMs.at(LM).first.find(i)==LMs.at(LM).first.end())
						{
							double cir_x = (coors[0][i]-scr_x0)*rat;
							double cir_y = y_res - (coors[1][i]-scr_y0)*rat;
							bool node_inside = (cir_x>bound_x0 && cir_x<bound_x1 && cir_y>bound_y0 && cir_y<bound_y1);
							if(node_inside) n.add_filled_circle(cir_x,cir_y,node_r,(dead_reacs[i]) ? reac_off_c : reac_on_c);
						}
					}	
					for(size_t i=sumreac;i<sumtot;i++)
					{
						if(LMs.at(LM).second.find(i)==LMs.at(LM).second.end())
						{
							double cir_x = (coors[0][i]-scr_x0)*rat;
							double cir_y = y_res - (coors[1][i]-scr_y0)*rat;
							bool node_inside = (cir_x>bound_x0 && cir_x<bound_x1 && cir_y>bound_y0 && cir_y<bound_y1);
							if(node_inside) 
							{
								COL col = (spec_void[i-sumreac]) ? void_c : ((dead_specs[i-sumreac]) ? spec_off_c : spec_on_c);
								n.add_filled_circle(cir_x,cir_y,node_r,col);
								if (uninitiated[i - sumreac])
								{
									double t = 0.5*(uninit_node_r - node_r);
									n.add_circle(cir_x, cir_y, uninit_node_r - t, uninit_c, t*2.0);
								} 
							}
						}
					}

					for(const auto &i:LMs.at(LM).first) // draw the reactions over
					{
						double cir_x = (coors[0][i]-scr_x0)*rat;
						double cir_y = y_res - (coors[1][i]-scr_y0)*rat;
						bool node_inside = (cir_x>bound_x0 && cir_x<bound_x1 && cir_y>bound_y0 && cir_y<bound_y1);
						if(node_inside) n.add_filled_circle(cir_x,cir_y,node_r,LM_reac_c);
					}	
					for(const auto &i:LMs.at(LM).second) // draw the specs over
					{
						double cir_x = (coors[0][i]-scr_x0)*rat;
						double cir_y = y_res - (coors[1][i]-scr_y0)*rat;
						bool node_inside = (cir_x>bound_x0 && cir_x<bound_x1 && cir_y>bound_y0 && cir_y<bound_y1);
						if(node_inside) 
						{
							COL col = LM_spec_c;
							n.add_filled_circle(cir_x,cir_y,node_r,col);
						}
					}
				}

				// Draw BM
				for(const auto &i:BMs.at(BM).first)
				{
					double x_r = (coors[0][i]-scr_x0)*rat;
					double y_r = y_res - (coors[1][i]-scr_y0)*rat;
					for(const auto &j:neighbours[i])
					{
						double x_s = (coors[0][j]-scr_x0)*rat;
						double y_s = y_res - (coors[1][j]-scr_y0)*rat;
						if(BMs.at(BM).second.find(j)!=BMs.at(BM).second.end()) // if internal spec
						{
							n.add_line(x_r,y_r,x_s,y_s,BM_line_c,BM_line_w);
							n.add_filled_circle(x_s,y_s,0.5*BM_line_w,BM_line_c); // Draw circes
						}
						else // if peripheral spec
						{
							double L = sqrt(pow(x_s-x_r,2)+pow(y_s-y_r,2));
							double r = 0.5 * BM_line_w;
							double dy = r * (x_s - x_r) / L;
							double dx = r * (y_s - y_r) / L;
							n.add_filled_triangle(x_r-dx,y_r+dy,x_r+dx,y_r-dy,x_s,y_s,BM_line_c);
						}
					}
					n.add_filled_circle(x_r,y_r,0.5*BM_line_w,BM_line_c);
				}
			}

			// Draw reaction-related stuff
			if(ereac_frozen!=-1) 
			{
				int target = ereac_frozen;

				// Draw the reaction
				double xr = (coors[0][target]-scr_x0)*rat;
				double yr = y_res - (coors[1][target]-scr_y0)*rat;
				n.add_filled_circle(xr,yr,high_node_r,reac_off_c);

				// Draw its causing specs as crosses
				for(const auto &i:espec_group)
				{
					double x_c = (coors[0][i]-scr_x0)*rat;
					double y_c = y_res - (coors[1][i]-scr_y0)*rat;

					n.add_filled_cross(x_c, y_c, ers_crosshand_l, ers_crosshand_w, error_border_c);
					n.add_filled_cross(x_c, y_c, ers_crosshand_l-ers_cross_border_w*0.5, ers_crosshand_w-ers_cross_border_w, al_map_rgb(255,255,255));
					n.add_filled_cross(x_c, y_c, ers_crosshand_l-ers_cross_border_w, ers_crosshand_w-ers_cross_border_w*2, error_cross_c);
				}			
			}
		}
	}

	return n;
}

void Explorer::update_arrays()
{
	arrays_upd_after_ccheck++; 
	ereac_frozen = -1;
	espec_group.clear();
	espec_frozen = -1;
	if(T_MODE==error_source) freeze = -1;

	sumreac = model->get_unmasked_rnum();
    sumspec = model->get_unmasked_snum();
    sumtot = sumreac+sumspec;

	auto reindex = [&] (int &target)
	{
		string sname = index_to_id[target];
		bool found = false;
		int iter = 0;
		for (const auto &i : model->reactions)
		{
			if(!i.second.ismasked())
			{
				if (i.first == sname)
				{
					target = iter;
					found = true;
					break;
				}
				iter++;
			}
		}
		if (!found)
		{
			iter = sumreac;
			for (const auto &i : model->species)
			{
				if(!i.second.ismasked())
				{
					if (i.first == sname)
					{
						target = iter;
						found = true;
						break;
					}
					iter++;
				}
			}
		}
		if (!found) target = -1;
	};

	if (search_target != -1)  // reindex the search target
	{
		reindex(search_target);
	}

	// reindex the freeze and the frozen bmodule
	if(freeze != -1)
	{
		reindex(freeze);
		if(!frozen_bmodule.empty()) 
		{
			set<int> new_frm;
			for(const int &i:frozen_bmodule) 
			{
				int target = i;
				reindex(target);
				if(target!=-1) new_frm.insert(target);
			}
			frozen_bmodule = new_frm;
		}
	}

    links.clear();
	neighbours.clear();
    parents.clear();
	children.clear();
    reactants.clear();
    products.clear();
    reversible.clear();
    dead.clear();
	dead_out.clear();
    objectives.clear();
    index_to_id.clear();
	index_to_id_full.clear();
    r_id_to_index.clear();
    s_id_to_index.clear();
    p_and_r_of.clear();
    o_products_of.clear();
    o_reactants_of.clear();
    reac_order.clear();
    spec_order.clear();
    reac_order_out.clear();
    spec_order_out.clear();
    coors.clear();

    /// Update the deadness of each species using the data stored in the model

    FBA_dead_specs.clear();
    FBA_dead.clear();
    BIDIR_dead_specs.clear();
    BIDIR_dead.clear();
    SINK_dead_specs.clear();
    SINK_dead.clear();
    for(auto &i:model->species) FBA_dead_specs.push_back(i.second.dead[0]);
    for(auto &i:model->reactions) FBA_dead.push_back(i.second.dead[0]);
    for(auto &i:model->species) BIDIR_dead_specs.push_back(i.second.dead[1]);
    for(auto &i:model->reactions) BIDIR_dead.push_back(i.second.dead[1]);
    for(auto &i:model->species) SINK_dead_specs.push_back(i.second.dead[2]);
    for(auto &i:model->reactions) SINK_dead.push_back(i.second.dead[2]);

	string dummy = "";
	blocking(B_MODE,dummy);

    /// Update the coordinates of reactants and species
    coors.resize(2);
    for(const auto &i:model->reactions) 
    {
		if (!i.second.ismasked())
		{
			coors[0].push_back(i.second.getx());
			coors[1].push_back(i.second.gety());
		}
    }
    for(const auto &i:model->species) 
    {
		if (!i.second.ismasked())
		{
			coors[0].push_back(i.second.getx());
			coors[1].push_back(i.second.gety());
		}
    }
    /// Make links from indicies in the coordinates array to names in the species and reactions maps
    for(const auto &i:model->reactions) if (!i.second.ismasked()) index_to_id.push_back(i.first);
    for(const auto &i:model->species) if (!i.second.ismasked()) index_to_id.push_back(i.first);

	/// Make a full index_to_id list
    for(const auto &i:model->reactions) index_to_id_full.push_back(i.first);
    for(const auto &i:model->species) index_to_id_full.push_back(i.first);
	
    /// Make a map from id to index
    for(int i=0;i<sumreac;i++) r_id_to_index.emplace(index_to_id[i],i);
    for(int i=sumreac;i<sumtot;i++) s_id_to_index.emplace(index_to_id[i],i-sumreac);

    /// Add the reversible array
    for(const auto &i:model->reactions) if (!i.second.ismasked()) reversible.push_back(i.second.reversible);

    /// Make links from reactions to species
    reactants.resize(sumreac);
    products.resize(sumreac);
    int iter = 0;
    for(const auto &i:model->reactions) 
    {
		if (!i.second.ismasked())
		{
			for(const auto &j:i.second.reactants) 
			{
				if(!model->species.at(j.first).ismasked()) reactants[iter].push_back(s_id_to_index.at(j.first)+sumreac); 
			}
			iter++;
		}
    }
    iter = 0;

    for(const auto &i:model->reactions) 
    {
		if (!i.second.ismasked())
		{
			for(const auto &j:i.second.products) 
			{
				if(!model->species.at(j.first).ismasked()) products[iter].push_back(s_id_to_index.at(j.first)+sumreac); 
			}
			iter++;
		}
    }

    /// Make links from species to reactions (products and reactants)
    p_and_r_of.resize(sumspec);
    o_products_of.resize(sumspec);
    o_reactants_of.resize(sumspec);

    for(int i=0;i<sumreac;i++)
    {
        if(!reversible[i])
        {
            for(const int &j:products[i]) o_products_of[j-sumreac].push_back(i);
            for(const int &j:reactants[i]) o_reactants_of[j-sumreac].push_back(i);
        }
        else
        {
            for(const int &j:products[i]) p_and_r_of[j-sumreac].push_back(i);
            for(const int &j:reactants[i]) p_and_r_of[j-sumreac].push_back(i);
        }
    }

    /// Find links between the species/reactions
    for(int i=0;i<sumreac;i++)
    {
        for(const int &j:reactants[i]) links.push_back({j,i,reversible[i]});
        for(const int &j:products[i]) links.push_back({i,j,reversible[i]});
    }

    /// Find neighbours of each species/reaction and parents of each species/reaction
	neighbours.resize(sumtot);
    parents.resize(sumtot);
	children.resize(sumtot);
    for(auto &i:links)
    {
		neighbours[i[0]].push_back(i[1]);
		neighbours[i[1]].push_back(i[0]);
        parents[i[1]].push_back(i[0]);
		children[i[0]].push_back(i[1]);
        if(i[2]) // if reversible
		{
			parents[i[0]].push_back(i[1]); 
			children[i[1]].push_back(i[0]);
		}
    }

    /// Find molecules that are not products of any reaction
    uninitiated.resize(sumspec);
    fill(uninitiated.begin(),uninitiated.end(),false);
    for(int i=0;i<sumspec;i++)     if((p_and_r_of[i].size()==1 && o_products_of[i].size()==0 && o_reactants_of[i].size()==0) 
                                    || (o_products_of[i].size()==0 && o_reactants_of[i].size()>0 && p_and_r_of[i].size()==0) 
                                    || (o_products_of[i].size()>0 && o_reactants_of[i].size()==0 && p_and_r_of[i].size()==0)) uninitiated[i]=true;

    /// Find molecules that are not found in any reaction
    spec_void.resize(sumspec);
    fill(spec_void.begin(),spec_void.end(),false);
    for(int i=0;i<sumspec;i++) if(o_products_of[i].size()==0 && o_reactants_of[i].size()==0 && p_and_r_of[i].size()==0) spec_void[i]=true;

    /// Find the inflows
    vector<int> infl;
    for(int i=0;i<sumreac;i++) if((reactants[i].size()==0 && products[i].size()>0) || (reactants[i].size()>0 && products[i].size()==0 && reversible[i])) infl.push_back(i);

	/// Find the outflows
	vector<int> outf;
	for(int i=0;i<sumreac;i++) if((reactants[i].size()>0 && products[i].size()==0) || (reactants[i].size()==0 && products[i].size()>0 && reversible[i])) infl.push_back(i);

    /// Find the objectives
    for(const auto &i:model->reactions) if(!i.second.ismasked() && i.second.obj_coeff!=0) objectives.push_back(r_id_to_index.at(i.first));


    /// Find molecules that can be directly derived from the inflows

    reac_order.resize(sumreac);     /// reaction distance to inflow
    fill(reac_order.begin(),reac_order.end(),-1);
    spec_order.resize(sumspec);       /// species distance to inflow
    fill(spec_order.begin(),spec_order.end(),-1);

    for(int i:infl)
    {
        reac_order[i]=1;
        for(const int &j:products[i]) spec_order[j-sumreac]=1;
		for(const int &j:reactants[i]) if(reversible[i]) spec_order[j-sumreac]=1;
    }

    bool f_n_reac;
    int c_ord = 1;
    do
    {
        f_n_reac = false;
        for(int i=0;i<sumreac;i++)
        {
            if(reac_order[i]==-1)
            {
                bool all_present = true;
                if(reversible[i])
                {
                    bool first_present = true;
                    bool second_present = true;
                    for(const int &j:reactants[i]) if(spec_order[j-sumreac]==-1) first_present=false;
                    if(!first_present) for(const int &j:products[i]) if(spec_order[j-sumreac]==-1) second_present=false;
                    if(!first_present && !second_present) all_present=false;
                }
                else for(const int &j:reactants[i]) if(spec_order[j-sumreac]==-1) all_present=false;
                if(all_present) {reac_order[i]=c_ord; f_n_reac=true;}
            }
        }
        for(int i=0;i<sumreac;i++)
        {
            if(reac_order[i]==c_ord)
            {
                for(const int &j:products[i]) spec_order[j-sumreac]=c_ord;
                if(reversible[i]) for(const int &j:reactants[i]) spec_order[j-sumreac]=c_ord;
            }
        }
        c_ord++;
    } while(f_n_reac);

    /// Find species that cannot be directly derived from the inflows (for the ancestry searching)
    dead.resize(sumtot);
    for(int i=0;i<sumreac;i++) dead[i]=(reac_order[i]==-1) ? true : false;
    for(int i=0;i<sumspec;i++) dead[i+sumreac]=(spec_order[i]==-1);

	/// Find molecules that can be directly excreted from the outflows

    reac_order_out.resize(sumreac);     /// reaction distance to outflow
    fill(reac_order_out.begin(),reac_order_out.end(),-1);
    spec_order_out.resize(sumspec);       /// species distance to outflow
    fill(spec_order_out.begin(),spec_order_out.end(),-1);

    for(int i:outf)
    {
        reac_order_out[i]=1;
        for(const int &j:reactants[i]) spec_order_out[j-sumreac]=1;
		for(const int &j:products[i]) if(reversible[i]) spec_order_out[j-sumreac]=1;
    }

    c_ord = 1;
    do
    {
        f_n_reac = false;
        for(int i=0;i<sumreac;i++)
        {
            if(reac_order_out[i]==-1)
            {
                bool all_present = true;
                if(reversible[i])
                {
                    bool first_present = true;
                    bool second_present = true;
                    for(const int &j:products[i]) if(spec_order_out[j-sumreac]==-1) first_present=false;
                    if(!first_present) for(const int &j:reactants[i]) if(spec_order_out[j-sumreac]==-1) second_present=false;
                    if(!first_present && !second_present) all_present=false;
                }
                else for(const int &j:products[i]) if(spec_order_out[j-sumreac]==-1) all_present=false;
                if(all_present) {reac_order_out[i]=c_ord; f_n_reac=true;}
            }
        }
        for(int i=0;i<sumreac;i++)
        {
            if(reac_order_out[i]==c_ord)
            {
                for(const int &j:reactants[i]) spec_order_out[j-sumreac]=c_ord;
                if(reversible[i]) for(const int &j:products[i]) spec_order_out[j-sumreac]=c_ord;
            }
        }
        c_ord++;
    } while(f_n_reac);

    /// Find species that cannot be directly derived from the inflows (for the ancestry searching)
    dead_out.resize(sumtot);
    for(int i=0;i<sumreac;i++) dead_out[i]=(reac_order_out[i]==-1) ? true : false;
    for(int i=0;i<sumspec;i++) dead_out[i+sumreac]=(spec_order_out[i]==-1);

	/// Find all the disconnected subgraphs in the model
	vector<vector<int> > adjacency(sumtot);
	subgraphs.clear();
	subgraphs.resize(sumtot, 0);
	for (size_t i = 0; i<links.size(); i++)
	{
		adjacency[links[i][0]].push_back(links[i][1]);
		adjacency[links[i][1]].push_back(links[i][0]);
	}
	int front = 0;
	int curr_group = 0;
	while (front<sumtot)
	{
		curr_group++;
		find_subgraph(front, curr_group, adjacency, subgraphs);
		while (front<sumtot && subgraphs[front] != 0) front++;
	}
	num_subgraphs = curr_group;

	update_compartments();
}

void Explorer::file_action_dialogue(int n, string &s)
{
    if((n<5 && n!=2 && n!=0) || n==6) /// New, Open, Save as
    {
        al_pause_event_queue(event_queue,true);
        al_flush_event_queue(event_queue);

		al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 2, ALLEGRO_SUGGEST);
		al_set_new_display_option(ALLEGRO_SAMPLES, 4, ALLEGRO_SUGGEST);

        int dis_num = al_get_num_video_adapters();
        ALLEGRO_MONITOR_INFO inf;
        al_get_monitor_info(dis_num-1, &inf);
        double height = 600;
        double width = 800;

        al_set_new_window_position(0.5*((inf.x2-inf.x1)-width),0.5*((inf.y2-inf.y1)-height));
        al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_OPENGL);

        string name;
        if(n==0) name = "New model";
        else if(n==1) name = "Open model";
        else if(n==3) name = "Save model as";
        else if(n==4) name = "Save SVG as";
		else if (n==6) name = "Save error tracing as";

        al_set_new_window_title(&name[0]);
        ALLEGRO_DISPLAY * file_disp = al_create_display(width,height);
        if(!file_disp) fprintf(stderr, "failed to create file display!\n");
        ALLEGRO_TIMER * tim = al_create_timer(1.0 / FPS);
        if(!tim) fprintf(stderr, "failed to create timer!\n");
        ALLEGRO_EVENT_QUEUE * ev_q = al_create_event_queue();
        if(!ev_q) fprintf(stderr, "failed to create event queue!\n");

        al_set_target_bitmap(al_get_backbuffer(file_disp));

        al_register_event_source(ev_q, al_get_display_event_source(file_disp));
        al_register_event_source(ev_q, al_get_timer_event_source(tim));
        al_register_event_source(ev_q, al_get_mouse_event_source());
        al_register_event_source(ev_q, al_get_keyboard_event_source());
        al_start_timer(tim);

        string file_name;
        Filehandler::action act;
        Filehandler::mode mod = (n==1) ? Filehandler::open : (n==3 || n==4 || n==6) ? Filehandler::save_as : Filehandler::save_as;
        Filehandler * filh = new Filehandler(file_disp,mod,&act,&file_name);

        bool redraw = true;
        queue<event_type> events;
        queue<pair<char,char>> key_inps;
        double m_x = -1;
        double m_y = -1;
        double m_z = 0;

		bool LOAD_MODEL = false;

        while(act==Filehandler::none)
        {
            ALLEGRO_EVENT ev;
            al_wait_for_event(ev_q, &ev);

            if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) break;
            //if(ev.type == ALLEGRO_EVENT_TIMER) redraw=true;
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
                while(!events.empty())
                {
                    if(act!=Filehandler::none) break;
                    filh->respond_to_event(display,events.front(),m_x,m_y,m_z,(events.front()==event_type::keyboard) ? key_inps.front() : pair<char,char>('\0','n'));
                    if(events.front()==event_type::keyboard) key_inps.pop();
                    events.pop();
                    ///respond
                }

                if(act==Filehandler::do_it)
                {
                    string error;
					boost::filesystem::path p(file_name);
					if(!p.is_absolute()) file_name = (boost::filesystem::path(filh->get_current_path()) / p).string();
					bool exists = boost::filesystem::exists(file_name);
					bool is_directory = boost::filesystem::is_directory(file_name);
					if (exists && is_directory)
					{
						filh->change_folder(file_name);
						filh->clear_input();
						act = Filehandler::none;
					}
					else
					{
						if (mod == Filehandler::open)
						{
							if (exists)
							{
								if (n == 1) // Open model
								{
									LOAD_MODEL = true;
								}
							}
							else
							{
								int OK;
								vector<string> buts = { "Cancel","Try another name" };
								Warning w(ev_q, "A file with this name does not exist!", buts, &OK);
								al_set_target_backbuffer(file_disp);
								if (OK == 1) act = Filehandler::none;
							}
						}
						else if (mod == Filehandler::save_as)
						{
							if (exists)
							{
								int OK;
								vector<string> buts = { "Cancel","Try another name","Save over" };
								Warning w(ev_q, "A file with this name already exists!", buts, &OK);
								al_set_target_backbuffer(file_disp);
								if (OK == 2)
								{
									if (n == 3) save_FBA_model(file_name);
									else if (n == 4) save_SVG(file_name);
									else if (n == 6) results_to_XML(file_name);
								}
								else if (OK == 1) act = Filehandler::none;
							}
							else
							{
								if (n == 3) save_FBA_model(file_name);
								else if (n == 4) save_SVG(file_name);
								else if (n == 6) results_to_XML(file_name);
							}
						}
					}
                }
                else if (act==Filehandler::cancel) break;

                al_set_target_bitmap(al_get_backbuffer(file_disp));
                al_clear_to_color(al_map_rgb(255,255,255));
                filh->draw();

                al_flip_display();
                redraw=false;
            }
        }

        delete filh;
        if(tim!=NULL) al_destroy_timer(tim);
        if(ev_q!=NULL) al_destroy_event_queue(ev_q);
		if(file_disp!=NULL) al_destroy_display(file_disp);


        al_pause_event_queue(event_queue,false);

		if (LOAD_MODEL)
		{
			Loading l(display, disp_map, 100, al_get_display_width(display)*0.5, al_get_display_height(display)*0.5, this, &Explorer::load_FBA_model, file_name);
			if (!model->empty()) Loading l(display, disp_map, 100, al_get_display_width(display)*0.5, al_get_display_height(display)*0.5, this, &Explorer::make_layout);
		}
    }
    else if(n==0)
    {
        clear_FBA_model();
        update_arrays();
        update_blocked(true);
        scr_upd = true;
    }
    else if(n==2) {if(!FILE_NAME.empty()) save_FBA_model(FILE_NAME);} /// Save
	else if (n == 5) // Save blocked modules
	{
		// Save blocked reactions and species in csv files, each column representing a blocked module
		// Do so for all modes
		for (int i = 0; i < 3; i++)
		{
			// Find the current dead reac and spec list
			vector<bool> dead_r_and_s = (i == 0) ? FBA_dead : (i == 1) ? BIDIR_dead : SINK_dead;
			dead_r_and_s.insert(dead_r_and_s.end(),
				((i == 0) ? FBA_dead_specs : (i == 1) ? BIDIR_dead_specs : SINK_dead_specs).begin(),
				((i == 0) ? FBA_dead_specs : (i == 1) ? BIDIR_dead_specs : SINK_dead_specs).end());

			struct sizecomp 
			{
				bool operator() (const set<int>& lhs, const set<int>& rhs) const
				{
					return lhs.size()>rhs.size();
				}
			} comp;

			vector<set<int>> bmodule_list;
			vector<bool> tested(sumtot, false);
			bool fully_tested = false;
			while (!fully_tested)
			{
				// find first non-tested dead
				int hit = -1;
				for (int j = 0; j < sumtot; j++) if (dead_r_and_s[j] && !tested[j]) { hit = j; break; }

				if (hit != -1) // if found anything not yet included
				{
					set<int> bmodule;
					find_bmodule(bmodule, neighbours, dead_r_and_s, hit, tested);
					bmodule_list.push_back(bmodule);
				}
				else fully_tested = true;
			}

			sort(bmodule_list.begin(), bmodule_list.end(), comp);

			Array<string> bmodr; // vectorize the set of sets
			Array<string> bmods;
			bmodr.resize(bmodule_list.size());
			bmods.resize(bmodule_list.size());
			int iter = 0;
			for (const auto &j : bmodule_list)
			{
				for (const auto &k : j)
				{
					if (k < sumreac) bmodr[iter].push_back(index_to_id[k]);
					else bmods[iter].push_back(index_to_id[k]);
				}
				iter++;
			}

			// Construct the files

			int num_cols = (int)bmodule_list.size();

			auto fill_file = [&](Array<string> content)
			{
				string file;

				if (num_cols > 0)
				{
					// Add the content
					bool some_left = false;
					int pos = 0;
					do
					{
						some_left = false;
						// Check if we have not yet written down all the sets
						for (const auto &j : content) if ((int)j.size() > pos) some_left = true;
						if (some_left)
						{
							for (const auto &j : content)
							{
								if ((int)j.size() > pos) file += j[pos];
								file += ",";
							}
							file.back() = '\n';
							pos++;
						}

					} while (some_left);
				}

				return file;
			};

			string reacfile = fill_file(bmodr);
			string specfile = fill_file(bmods);


			// Save the files 

			string prefix = (i == 0) ? "FBA" : (i == 1) ? "BIDIR" : "DYNAMIC";
			string_to_file(reacfile, prefix + "_blocked_reactions.csv");
			string_to_file(specfile, prefix + "_blocked_species.csv");
		}
	}
    else if(n==7) quit=true;
}

void Explorer::results_to_XML(string fpath)
{
	string output_fpath = fpath;
	
	/// Open file
    ofstream output_file (output_fpath,ios::out | ios::trunc);

    /// Make results tree
    pugi::xml_document rtree;

	pugi::xml_node decl = rtree.prepend_child(pugi::node_declaration);
	decl.append_attribute("version") = "1.0";
	decl.append_attribute("encoding") = "UTF-8";

    /// Add the algorithm version nodes
    pugi::xml_node algorithm = rtree.append_child("algorithm");
	pugi::xml_attribute alg_name =  algorithm.append_attribute("name");
    pugi::xml_attribute ET_version =  algorithm.append_attribute("version");
	alg_name.set_value("ErrorTracer");
	ET_version.set_value(1);

	int cycle_iter = 0;
	map<int,int> cycle_links;

    pugi::xml_node breacs = algorithm.append_child("blocked_reactions");
	for (const auto &br:reacs_to_erspecs)
	{
		pugi::xml_node reac = breacs.append_child("reaction");
		pugi::xml_attribute reac_id = reac.append_attribute("id");
        reac_id.set_value(index_to_id[br.first].c_str());


		pugi::xml_node esources = reac.append_child("error_sources");
		for(const auto &es:br.second) // loop over error causing species
		{
			// Set id and error type
			pugi::xml_node spec = esources.append_child("species");
			pugi::xml_attribute spec_id = spec.append_attribute("id");
			spec_id.set_value(index_to_id[es.first].c_str());
			pugi::xml_attribute er_type = spec.append_attribute("error_type");
			if(es.second==0)  er_type.set_value("reversibility");
			else if(es.second==1)  er_type.set_value("source");
			else if(es.second==2)  er_type.set_value("stoichiometry");
			else // If cycle error, show the id of the respective blocking cycle
			{
				int cur_cycle = 0;
				er_type.set_value("cycle");
				int LMindex = erspecs.at(es.first).first.begin()->second.second;
				if(cycle_links.find(LMindex)==cycle_links.end())
				{
					cycle_iter++;
					cycle_links.emplace(LMindex,cycle_iter);
				}
				cur_cycle = cycle_links.at(LMindex);
				
				pugi::xml_attribute cyc_id = spec.append_attribute("cycle_id");
				cyc_id.set_value(("Cycle_"+to_string(cur_cycle)).c_str());
			}
		}
	}


	map<int,int> reverse_cycle_links;
	for(const auto &i:cycle_links) reverse_cycle_links.emplace(i.second,i.first);

	// Add blocking cycles if any exist
	pugi::xml_node bcycles = algorithm.append_child("blocking_cycles");
	for(const auto &i:reverse_cycle_links)
	{
		pugi::xml_node cycle = bcycles.append_child("cycle");
		pugi::xml_attribute cyc_id = cycle.append_attribute("id");
        cyc_id.set_value(("Cycle_"+to_string(i.first)).c_str());

		// add blocking species
		for(const auto &j:LMs.at(i.second).second) 
		{
			pugi::xml_node cspec = cycle.append_child("species");
			pugi::xml_attribute spec_id = cspec.append_attribute("id");
			spec_id.set_value(index_to_id[j].c_str());
		}

	}

    rtree.save(output_file);
    output_file.close();
}

void Explorer::edit(int n, string &s)
{
    if(n==0)
    {
        model->undo();
        update_arrays();
        scr_upd=true;
		update_compartments();
    }
    else if(n==1)
    {
        model->redo();
        update_arrays();
        scr_upd=true;
		update_compartments();
    }
    else if(n==2)
    {
		if(!model->empty()) Loading l(display,disp_map,100,al_get_display_width(display)*0.5,al_get_display_height(display)*0.5,this,&Explorer::make_layout);
        scr_upd=true;
    }
    else if(n==3)
    {
		if(!model->empty()) Loading l(display,disp_map,100,al_get_display_width(display)*0.5,al_get_display_height(display)*0.5,this,&Explorer::update_blocked,true);
		scr_upd=true;
    }
    else if(n==4)
    {
		if(!model->empty()) Loading l(display,disp_map,100,al_get_display_width(display)*0.5,al_get_display_height(display)*0.5,this,&Explorer::update_blocked,false);
		scr_upd=true;
    }
}

void Explorer::view(int n, string &s)
{
	if(n==1)
	{
		/// If the system is in blocked module tracking mode and something is selected and has more than one element
		if(!show_subnetwork && T_MODE==blocked_module && freeze!=-1 && frozen_bmodule.size()>1)
		{
			show_subnetwork = true;
			// Swap the front and backbuffer of the model
			for(auto &i:model->reactions) i.second.flip();
			for(auto &i:model->species) i.second.flip();
			// Mask everything except the selected blocked buffer
			for(auto &i:model->reactions) i.second.mask();
			for(auto &i:model->species) i.second.mask();
			for(const auto &i:frozen_bmodule)
			{
				if(i<(int)model->reactions.size()) model->reactions.at(index_to_id[i]).unmask();
				else model->species.at(index_to_id[i]).unmask();
			}
			// Replot and update the arrays 
			update_arrays();
			Loading l(display, disp_map, 100, al_get_display_width(display)*0.5, al_get_display_height(display)*0.5, this, &Explorer::make_layout);
		}
	}
	else if(n==2)
	{
		if(show_subnetwork)
		{
			show_subnetwork = false;
			// Swap the front and backbuffer of the model
			for(auto &i:model->reactions) i.second.flip();
			for(auto &i:model->species) i.second.flip();
			update_arrays();
			// if nothing happened excepth switching to BM view and back, the 
			if(arrays_upd_after_ccheck==2) arrays_upd_after_ccheck = 0;

			// recenter the view 

			double max_x = *max_element(coors[0].begin(),coors[0].end());
			double min_x = *min_element(coors[0].begin(),coors[0].end());
			double max_y = *max_element(coors[1].begin(),coors[1].end());
			double min_y = *min_element(coors[1].begin(),coors[1].end());

			scr_c_x = 0.5*(min_x+max_x);
			scr_c_y = 0.5*(min_y+max_y);

			double x0, y0, x1, y1;
			win->Get_cust_coors(x0,y0,x1,y1);

			double del_x = max_x-min_x;
			double del_y = max_y-min_y;
			if((del_y/del_x)<((y1-y0)/(x1-x0))) scr_w = del_x*1.05;
			else scr_w = del_y*(x1-x0)/(y1-y0)*1.05;

		}
	}
}

void Explorer::purge(int n, string &s)
{
	if (!model->empty())
	{
		if (n == 0)
		{
			model->purge_species_by_tag("_b", FBA_model::AT_END);
			model->purge_species_by_tag("_boundary", FBA_model::AT_END);
			model->purge_species_by_compartment("Boundary");
			model->purge_species_by_compartment("boundary");
			update_arrays();
			scr_upd = true;
		}
		else if (n == 1)
		{
			model->purge_species_by_tag("_e", FBA_model::AT_END);
			model->purge_species_by_tag("_extracellular", FBA_model::AT_END);
			model->purge_species_by_compartment("Extracellular");
			model->purge_species_by_compartment("extracellular");
			update_arrays();
			scr_upd = true;
		}
		else if (n == 2)
		{
			model->purge_disconnected_species();
			update_arrays();
			scr_upd = true;
		}
		else if (n == 3)
		{
			model->purge_disconnected_reactions();
			update_arrays();
			scr_upd = true;
		}
		else if (n == 4)
		{
			model->purge_disconnected_clusters();
			update_arrays();
			scr_upd = true;
		}
		else if (n == 5)
		{
			purge_selection();
		}
		else if (n==6) 
		{
			if(!B_MODE==none)
			{
				model->purge_dead(B_MODE==FBA ? 0 : B_MODE==bidir ? 1 : 2);
				update_arrays();
				scr_upd = true;
			}
		}
		update_compartments();
	}
}

void Explorer::purge_selection()
{
	if (freeze != -1)
	{
		if (freeze < -1)
		{
			int iter = -2;
			for (const auto& i : cmprts)
			{
				if (iter == freeze)
				{
					model->purge_compartment(i.first);
				}
				iter--;
			}
		}
		else if (freeze < sumreac)
		{
			vector<string> p;
			p.push_back(index_to_id[freeze]);
			model->purge_reactions(p);
		}
		else
		{
			vector<string> p;
			p.push_back(index_to_id[freeze]);
			model->purge_species(p);
		}
		freeze = -1;
		scr_upd = true;		
		update_arrays();
		update_compartments();
	}
}

void Explorer::purge_selection_weak() // purge species without purging particiaptin reactions (for detangling)
{
	if (freeze >= sumreac)
	{
		vector<string> p;
		p.push_back(index_to_id[freeze]);
		model->purge_species_weak(p);

		freeze = -1;
		scr_upd = true;
		update_arrays();
		update_compartments();
	}
	else 
	{
		purge_selection();
	}
}

void Explorer::tracking(int n, string &s)
{
	if(n==0) T_MODE = non_curious;
	else if (n == 1) T_MODE = neighbour;
	else if (n == 2) T_MODE = ancestral;
	else if (n == 3) T_MODE = blocked_module;
	else if (n == 4) T_MODE = error_source;

	// reset the saved arrays
	frozen_bmodule.clear();
	frozen_bmodule_centre = -1;
	frozen_tree.clear();
	frozen_tree_centre = -1;
	ereac_frozen = -1;
	espec_group.clear();
	espec_frozen = -1;

	scr_upd = true;
}

void Explorer::blocking(int n, string &s)
{
	dead_specs.resize(sumspec);
	dead_reacs.resize(sumreac);
    if(n==0)
    {
        B_MODE = none;
		fill(dead_specs.begin(),dead_specs.end(),false);
		fill(dead_reacs.begin(),dead_reacs.end(),false);
    }
    else if(n==1)
    {
        B_MODE = FBA;
		model->copy_smasked(dead_specs,FBA_dead_specs);
		model->copy_rmasked(dead_reacs,FBA_dead);
    }
    else if(n==2)
    {
        B_MODE = bidir;
		model->copy_smasked(dead_specs,BIDIR_dead_specs);
		model->copy_rmasked(dead_reacs,BIDIR_dead);
    }
    else if(n==3)
    {
        B_MODE = sink;
		model->copy_smasked(dead_specs,SINK_dead_specs);
		model->copy_rmasked(dead_reacs,SINK_dead);
    }
	dead_r_and_s = dead_reacs;
	dead_r_and_s.insert(dead_r_and_s.end(),dead_specs.begin(),dead_specs.end());
    scr_upd = true;
}

void Explorer::graphics(int n, string &s)
{
    if(n==0) graphics_mode = 0;
    else if(n==1) graphics_mode = 1;
    scr_upd = true;
}

void Explorer::compartments(int n, string &s)
{
    if(n==0) draw_comparts = true;
    else if(n==1) draw_comparts = false;
    scr_upd = true;
}

void Explorer::palettes(int n, string &s)
{
    auto lengthen = [] (int n)
    {
        string s = to_string(n);
        if(s.length()==1) return "00"+s;
        else if(s.length()==2) return "0"+s;
        else if(s.length()==3) return s;
    };
    auto conv = [&lengthen] (COL &c)
    {
        unsigned char r, g, b;
        al_unmap_rgb(c,&r,&g,&b);
        return lengthen((int)r)+lengthen((int)g)+lengthen((int)b);
    };

    if(n==0)
    {
        if(s=="get") s = conv(reac_on_c);
        else reac_on_c = al_map_rgb(stoi(s.substr(0,3)),stoi(s.substr(3,3)),stoi(s.substr(6,3)));
    }
    else if(n==1)
    {
        if(s=="get") s = conv(spec_on_c);
        else spec_on_c = al_map_rgb(stoi(s.substr(0,3)),stoi(s.substr(3,3)),stoi(s.substr(6,3)));
    }
    else if(n==2)
    {
        if(s=="get") s = conv(reac_off_c);
        else reac_off_c = al_map_rgb(stoi(s.substr(0,3)),stoi(s.substr(3,3)),stoi(s.substr(6,3)));
    }
    else if(n==3)
    {
        if(s=="get") s = conv(spec_off_c);
        else spec_off_c = al_map_rgb(stoi(s.substr(0,3)),stoi(s.substr(3,3)),stoi(s.substr(6,3)));
    }
    else if(n==4)
    {
        if(s=="get") s = conv(uninit_c);
        else uninit_c = al_map_rgb(stoi(s.substr(0,3)),stoi(s.substr(3,3)),stoi(s.substr(6,3)));
    }
    else if(n==5)
    {
        if(s=="get") s = conv(void_c);
        else void_c = al_map_rgb(stoi(s.substr(0,3)),stoi(s.substr(3,3)),stoi(s.substr(6,3)));
    }
    else if(n==6)
    {
        if(s=="get") s = conv(compart_line_c);
        else compart_line_c = al_map_rgb(stoi(s.substr(0,3)),stoi(s.substr(3,3)),stoi(s.substr(6,3)));
    }
    else if(n==7)
    {
        if(s=="get") s = conv(LM_reac_c);
        else LM_reac_c = al_map_rgb(stoi(s.substr(0,3)),stoi(s.substr(3,3)),stoi(s.substr(6,3)));

    }
    else if(n==8)
    {
        if(s=="get") s = conv(LM_spec_c);
        else LM_spec_c = al_map_rgb(stoi(s.substr(0,3)),stoi(s.substr(3,3)),stoi(s.substr(6,3)));
    }
    scr_upd = true;
}

void Explorer::search_for_word(int n, vector<string> &s, int select, bool &new_text)
{
	if (!s.empty())
	{
		search_list.clear();
		string sname = s[0];
		s.clear();


		if (sname.size() > 1)
		{
			for (const auto &i : model->reactions)
			{
				if (!i.second.ismasked() && (i.second.name.find(sname)!=string::npos || i.first.find(sname)!=string::npos))
				{
					search_list.push_back(i.first);
					s.push_back("$" + col_to_str(spec_off_c) + i.first + " - " + i.second.name);
					s.push_back("\t");
				}
			}

			for (const auto &i : model->species)
			{
				if (!i.second.ismasked() && (i.second.name.find(sname) != string::npos || i.first.find(sname) != string::npos))
				{
					search_list.push_back(i.first);
					string input;
					input += "$" + col_to_str(spec_on_c) + i.first;
					if (!i.second.compart.empty()) input += " [" + model->compartments.at(i.second.compart).name + "]";
					if (!i.second.name.empty())  input += " - " + i.second.name;
					s.push_back(input);
					s.push_back("\t");
				}
			}
			if(s.size()>0) s.pop_back();
		}
	}
	else if(select>=0)
	{
		string sname = search_list[select];
		s.clear();
		s.push_back(sname);
		bool found = false;
		int iter = 0;
		for (const auto &i : model->reactions)
		{
			if(!i.second.ismasked())
			{
				if (i.first == sname)
				{
					search_target = iter;
					found = true;
					break;
				}
				iter++;
			}
		}
		if (!found)
		{
			iter = sumreac;
			for (const auto &i : model->species)
			{
				if(!i.second.ismasked())
				{
					if (i.first == sname)
					{
						search_target = iter;
						found = true;
						break;
					}
					iter++;
				}
			}
		}
	}
	else if(select==-1) search_target = -1;
    scr_upd = true;
}

void Explorer::add_or_edit(int n, string &s)
{
	current_aoe_action = n;
	if (n == 0)
	{
		if (freeze != -1)
		{
			edited_item = freeze;
			if (freeze < -1) // is a compartment
			{
				int iter = -2;
				for (const auto &i : model->compartments)
				{
					if (iter == edited_item)
					{
						temp_id = i.first;
						temp_compart = i.second;
					}
					iter--;
				}
				win->set_mode(Window<Explorer>::edit_compart);
			}
			else if(edited_item<sumreac) // is a reaction
			{
				//printf("\nClearing reaction!\n");
				temp_id = index_to_id[edited_item];
				temp_reac = model->reactions.at(temp_id);
				vector<int> sizes = { (int)temp_reac.reactants.size(),(int)temp_reac.products.size(),(int)temp_reac.genes.size() };

				temp_reactants.clear();
				temp_products.clear();
				temp_genes.clear();
				for (const auto &i : temp_reac.reactants) temp_reactants.push_back(pair<string, double>(i.first, i.second));
				for (const auto &i : temp_reac.products) temp_products.push_back(pair<string, double>(i.first, i.second));
				for (const auto &i : temp_reac.genes) temp_genes.push_back(i);

				win->set_mode(Window<Explorer>::edit_reac,sizes);
			}
			else // is a species
			{
				temp_id = index_to_id[edited_item];
				temp_spec = model->species.at(temp_id);
				win->set_mode(Window<Explorer>::edit_spec);
			}
		}
	}
	else if (n == 1)
	{
		temp_id = "";
		temp_spec.clear();
		win->set_mode(Window<Explorer>::add_spec);
	}
	else if (n == 2)
	{
		temp_id = "";
		temp_reac.clear();
		temp_reactants.clear();
		temp_products.clear();
		temp_genes.clear();
		win->set_mode(Window<Explorer>::add_reac);
	}
	else if (n == 3)
	{
		temp_id = "";
		temp_compart.clear();
		win->set_mode(Window<Explorer>::add_compart);
	}
}

void Explorer::edit_species(int n, string &s)
{
	if (!s.empty())
	{
		if (n == 1) //id
		{
			if (s[0] == '\r' || s[0] == '\b')
			{
				if (s.size() == 1) // initialize in case you are editing an existing species
				{
					if (s[0] == '\r' && win->get_mode() == Window<Explorer>::edit_spec)
					{
						s.clear();
						s = temp_id;
					}
					else if (s[0] == '\b') s = "invalid";
				}
				else if(s.size()>1)  // check for uniqueness of id 
				{
					string trial = s.substr(1, s.length() - 1);
					bool exists = model->species.find(trial) != model->species.end();
					if (win->get_mode() == Window<Explorer>::edit_spec)
					{
						if (exists && trial!= index_to_id[edited_item]) s = "invalid"; // found the species to exist and not be the current species
						else
						{
							editing_entity = true;
							temp_id = trial;
							s = "valid";
						}
					}
					else if (win->get_mode() == Window<Explorer>::add_spec)
					{
						if (exists) s = "invalid"; // found the species to exist 
						else
						{
							editing_entity = true;
							temp_id = trial;
							s = "valid";
						}
					}
				}
			}
		}
		else if (n == 2) //name
		{
			if (s[0] == '\r')
			{
				if (s.size() == 1 && win->get_mode() == Window<Explorer>::edit_spec) // initialize in case you are editing an existing species
				{
					s.clear();
					s = temp_spec.name;
				}
			}
			else if (s[0] == '\b')
			{
				editing_entity = true;
				string trial = s.substr(1, s.length() - 1);
				temp_spec.name = trial;
				s = "valid";
			}
		}
		else if (n == 3) // Formula
		{
			if (s[0] == '\r')
			{
				if (s.size() == 1 && win->get_mode() == Window<Explorer>::edit_spec) // initialize in case you are editing an existing species
				{
					s.clear();
					s = temp_spec.formula;
				}
			}
			else if (s[0] == '\b')
			{
				editing_entity = true;
				string trial = s.substr(1, s.length() - 1);
				temp_spec.formula = trial;
				s = "valid";
			}

		}
		else if (n == 4) // kegg
		{
			if (s[0] == '\r')
			{
				if (s.size() == 1 && win->get_mode() == Window<Explorer>::edit_spec) // initialize in case you are editing an existing species
				{
					s.clear();
					s = temp_spec.kegg;
				}
			}
			else if (s[0] == '\b')
			{
				editing_entity = true;
				string trial = s.substr(1, s.length() - 1);
				temp_spec.kegg = trial;
				s = "valid";
			}
		}
	}
}

void Explorer::edit_reaction(int n, string &s)
{
	if (!s.empty())
	{
		if (n == 1) //id
		{
			if (s[0] == '\r' || s[0] == '\b')
			{
				if (s.size() == 1) // initialize in case you are editing an existing species
				{
					if (s[0] == '\r' && win->get_mode() == Window<Explorer>::edit_reac)
					{
						s.clear();
						s = temp_id;
					}
					else if (s[0] == '\b') s = "invalid";
				}
				else if (s.size()>1)  // check for uniqueness of id 
				{
					string trial = s.substr(1, s.length() - 1);
					bool exists = model->reactions.find(trial) != model->reactions.end();
					if (win->get_mode() == Window<Explorer>::edit_reac)
					{
						if (exists && trial != index_to_id[edited_item]) s = "invalid"; // found the reaction to exist and not be the current reactions
						else
						{
							editing_entity = true;
							temp_id = trial;
							s = "valid";
						}
					}
					else if (win->get_mode() == Window<Explorer>::add_reac)
					{
						if (exists) s = "invalid"; // found the reactions to exist 
						else
						{
							editing_entity = true;
							temp_id = trial;
							s = "valid";
						}
					}
				}
			}
		}
		else if (n == 2) //name
		{
			if (s[0] == '\r')
			{
				if (s.size() == 1 && win->get_mode() == Window<Explorer>::edit_reac) // initialize in case you are editing an existing reactions
				{
					s.clear();
					s = temp_reac.name;
				}
			}
			else if (s[0] == '\b')
			{
				editing_entity = true;
				string trial = s.substr(1, s.length() - 1);
				temp_reac.name = trial;
				s = "valid";
			}
		}
		else if (n == 3) // Flux upper bound
		{
			if (s[0] == '\r')
			{
				if (s.size() == 1 && win->get_mode() == Window<Explorer>::edit_reac) // initialize in case you are editing an existing reactions
				{
					s.clear();
					s = to_string(temp_reac.up_bound);
				}
			}
			else if (s[0] == '\b')
			{
				string trial = s.substr(1, s.length() - 1);
				double temp;
				bool ok = true;
				try {
					temp = stod(trial);
				}
				catch (const std::invalid_argument& ia)
				{
					ok = false;
				}
				if (ok || trial.size()==0)
				{
					editing_entity = true;
					s = "valid";
					if (ok) temp_reac.up_bound = temp;
				} 
				else s = "invalid";
			}
		}
		else if (n == 4) // Flux lower bound
		{
			if (s[0] == '\r')
			{
				if (s.size() == 1 && win->get_mode() == Window<Explorer>::edit_reac) // initialize in case you are editing an existing reaction
				{
					s.clear();
					s = to_string(temp_reac.low_bound);
				}
			}
			else if (s[0] == '\b')
			{
				string trial = s.substr(1, s.length() - 1);
				double temp;
				bool ok = true;
				try {
					temp = stod(trial);
				}
				catch (const std::invalid_argument& ia)
				{
					ok = false;
				}
				if (ok || trial.size() == 0)
				{
					editing_entity = true;
					s = "valid";
					if(ok) temp_reac.low_bound = temp;
				}
				else s = "invalid";
			}
		}
		else if (n == 5) // Objective coefficient
		{
			if (s[0] == '\r')
			{
				if (s.size() == 1 && win->get_mode() == Window<Explorer>::edit_reac) // initialize in case you are editing an existing reaction
				{
					s.clear();
					s = to_string(temp_reac.obj_coeff);
				}
			}
			else if (s[0] == '\b')
			{
				string trial = s.substr(1, s.length() - 1);
				double temp;
				bool ok = true;
				try {
					temp = stod(trial);
				}
				catch (const std::invalid_argument& ia)
				{
					ok = false;
				}
				if (ok || trial.size() == 0)
				{
					editing_entity = true;
					s = "valid";
					if (ok) temp_reac.obj_coeff = temp;
				}
				else s = "invalid";
			}
		}
		else if (n == 6) // Equilibrium Constant
		{
			if (s[0] == '\r')
			{
				if (s.size() == 1 && win->get_mode() == Window<Explorer>::edit_reac) // initialize in case you are editing an existing reaction
				{
					s.clear();
					s = to_string(temp_reac.K_eq);
				}
			}
			else if (s[0] == '\b')
			{
				string trial = s.substr(1, s.length() - 1);
				double temp;
				bool ok = true;
				try {
					temp = stod(trial);
				}
				catch (const std::invalid_argument& ia)
				{
					ok = false;
				}
				if (ok || trial.size() == 0)
				{
					editing_entity = true;
					s = "valid";
					if (ok) temp_reac.K_eq = temp;
				}
				else s = "invalid";
			}
		}
		else if (n == 7) // kegg
		{
			if (s[0] == '\r')
			{
				if (s.size() == 1 && win->get_mode() == Window<Explorer>::edit_reac) // initialize in case you are editing an existing species
				{
					s.clear();
					s = temp_reac.kegg;
				}
			}
			else if (s[0] == '\b')
			{
				editing_entity = true;
				string trial = s.substr(1, s.length() - 1);
				temp_reac.kegg = trial;
				s = "valid";
			}
		}
	}
}

void Explorer::find_reactant(int n, vector<string> &s, int select, bool &new_text)
{
	if (n > temp_reactants.size())
	{
		temp_reactants.push_back(pair<string, double>("", 1.0));
	}
	if (select == -3)
	{
		temp_reactants.pop_back();
	}
	else if (select >= 0)
	{
		string sname = search_specs[select];
		s.clear();
		s.push_back(sname);
		temp_reactants[n - 1].first = sname;
	}
	else if (select == -1 && s.size()>0)
	{
		search_specs.clear();
		string sname = s[0];
		s.clear();
		if(temp_reactants[n-1].first!=sname) temp_reactants[n-1].first = ""; // if no longer the same as the well-selected product, allow searching for it
		if (sname.size() > 1)
		{
			for (const auto &i : model->species)
			{
				if (i.second.name.find(sname) != string::npos || i.first.find(sname) != string::npos)
				{
					bool found = false;
					for (const auto &j : temp_reactants) if (j.first != "" && j.first == i.first) { found = true; break; }
					if (!found)
					{
						search_specs.push_back(i.first);
						string input = "$" + col_to_str(spec_on_c) + i.first;
						if (!i.second.compart.empty()) input += " [" + model->compartments.at(i.second.compart).name + "]";
						if (!i.second.name.empty())  input += " - " + i.second.name;
						s.push_back(input);
						s.push_back("\t");
					}
				}
			}
			if (s.size()>0) s.pop_back();
		}
	}
	else if (select == -2)
	{
		if (win->get_mode() == Window<Explorer>::edit_reac)
		{
			s.clear();
			if(!temp_reactants[n - 1].first.empty()) s.push_back(temp_reactants[n-1].first);
		}
	}
	scr_upd = true;
}

void Explorer::edit_reactant_st(int n, string &s)
{
	if (s[0] == '\r')
	{
		if (s.size() == 1) // initialize in case you are editing an existing reaction
		{
			s.clear();
			s = (win->get_mode() == Window<Explorer>::edit_reac) ? to_string(temp_reactants[n - 1].second) : "1.0";
		}
	}
	else if (s[0] == '\b')
	{
		string trial = s.substr(1, s.length() - 1);
		double temp;
		bool ok = true;
		try {
			temp = stod(trial);
		}
		catch (const std::invalid_argument& ia)
		{
			ok = false;
		}
		if (ok)
		{
			editing_entity = true;
			s = "valid";
			temp_reactants[n - 1].second = temp;
		}
		else s = "invalid";
	}
}

void Explorer::find_product(int n, vector<string> &s, int select, bool &new_text)
{
	if (n > temp_products.size())
	{
		temp_products.push_back(pair<string, double>("", 1.0));
	}
	if (select == -3)
	{
		temp_products.pop_back();
	}
	else if (select >= 0)
	{
		string sname = search_specs[select];
		s.clear();
		s.push_back(sname);
		temp_products[n - 1].first = sname;
	}
	else if (select == -1 && s.size()>0)
	{
		search_specs.clear();
		string sname = s[0];
		s.clear();
		if(temp_products[n-1].first!=sname) temp_products[n-1].first = ""; // if no longer the same as the well-selected product, allow searching for it
		if (sname.size() > 1)
		{
			for (const auto &i : model->species)
			{
				bool found = false;
				for (const auto &j : temp_products) if (j.first != "" && j.first == i.first) { found = true; break; }
				if (!found)
				{
					if (i.second.name.find(sname) != string::npos || i.first.find(sname) != string::npos)
					{
						search_specs.push_back(i.first);
						string input = "$" + col_to_str(spec_on_c) + i.first;
						if (!i.second.compart.empty()) input += " [" + model->compartments.at(i.second.compart).name + "]";
						if (!i.second.name.empty())  input += " - " + i.second.name;
						s.push_back(input);
						s.push_back("\t");
					}
				}
			}
			if (s.size()>0) s.pop_back();
		}
	}
	else if (select == -2)
	{
		if (win->get_mode() == Window<Explorer>::edit_reac)
		{
			s.clear();
			if (!temp_products[n - 1].first.empty()) s.push_back(temp_products[n - 1].first);
		}
	}
	scr_upd = true;
}

void Explorer::edit_product_st(int n, string &s)
{
	if (s[0] == '\r')
	{
		if (s.size() == 1) // initialize in case you are editing an existing reaction
		{
			s.clear();
			s = (win->get_mode() == Window<Explorer>::edit_reac) ? to_string(temp_products[n - 1].second) : "1.0";
		}
	}
	else if (s[0] == '\b')
	{
		string trial = s.substr(1, s.length() - 1);
		double temp;
		bool ok = true;
		try {
			temp = stod(trial);
		}
		catch (const std::invalid_argument& ia)
		{
			ok = false;
		}
		if (ok)
		{
			editing_entity = true;
			s = "valid";
			temp_products[n - 1].second = temp;
		}
		else s = "invalid";
	}
}

void Explorer::find_compart(int n, vector<string> &s, int select, bool &new_text)
{
	if (select >= 0)
	{
		string sname = search_comparts[select];
		s.clear();
		s.push_back(sname);
		for (const auto &i : model->compartments)
		{
			if (i.first == sname)
			{
				temp_spec.compart = sname;
				break;
			}
		}
	}
	else if (select == -1)
	{
		search_comparts.clear();
		s.clear();

		for (const auto &i : model->compartments)
		{
			string input = "$" + col_to_str(spec_on_c) + i.first;
			if (!i.second.name.empty()) input += " - " + i.second.name;
			s.push_back(input);
			s.push_back("\t");
			search_comparts.push_back(i.first);
		}
		if (s.size()>0) s.pop_back();
	}
	else if (select == -2)
	{
		if (win->get_mode() == Window<Explorer>::edit_spec)
		{
			s.clear();
			string comp = model->species.at(index_to_id[edited_item]).compart;
			if(!comp.empty()) s.push_back(comp);
			else s.push_back("none");
		}
		else
		{
			s.push_back("none");
		}
	}
	scr_upd = true;
}

void Explorer::find_outside(int n, vector<string> &s, int select, bool &new_text)
{
	if (select >= 0)
	{
		string sname = search_comparts[select];
		s.clear();
		if(!sname.empty()) s.push_back(sname);
		else s.push_back("none");
		temp_compart.outside = sname;
	}
	else if (select == -1)
	{
		string current;
		int iter = -2;
		for (const auto &i : model->compartments)
		{
			if (edited_item == iter)
			{
				current = i.first;
				break;
			}
			iter--;
		}

		search_comparts.clear();
		s.clear();

		s.push_back("$" + col_to_str(spec_on_c) + "none");
		s.push_back("\t");
		search_comparts.push_back("");
		for (const auto &i : model->compartments)
		{
			if (i.first != current)
			{
				string input = "$" + col_to_str(spec_on_c) + i.first;
				if (!i.second.name.empty()) input += " - " + i.second.name;
				s.push_back(input);
				s.push_back("\t");
				search_comparts.push_back(i.first);
			}
		}
		if (s.size()>0) s.pop_back();
	}
	else if (select == -2)
	{
		if (win->get_mode() == Window<Explorer>::edit_compart)
		{
			string current;
			int iter = -2;
			for (const auto &i : model->compartments)
			{
				if (edited_item == iter)
				{
					current = i.second.outside;
					break;
				}
				iter--;
			}
			s.clear();
			if(!current.empty()) s.push_back(current);
			else s.push_back("none");
			
		}
		else
		{
			s.clear();
			s.push_back("none");
		}
	}
	scr_upd = true;
}

void Explorer::edit_gene(int n, string &s)
{
	if (n > temp_genes.size())
	{
		editing_entity = true;
		temp_genes.push_back("");
	}
	if (s == "\n")
	{
		editing_entity = true;
		temp_genes.pop_back();
	}
	else if (s[0] == '\b')
	{
		string trial = s.substr(1, s.length() - 1);
		s.clear();
		if (trial.size() > 0)
		{
			editing_entity = true;
			temp_genes[n - 1] = trial;
			s = "valid";
		}
		else s = "invalid";
	}
	else if (s[0] == '\r')
	{
		if (win->get_mode() == Window<Explorer>::edit_reac)
		{
			s.clear();
			if (!temp_genes[n - 1].empty()) s = temp_genes[n - 1];
			else s.push_back('\r');
		}
	}
	scr_upd = true;
}

void Explorer::edit_compart(int n, string &s)
{
	if (!s.empty())
	{
		if (n == 1) //id
		{
			if (s[0] == '\r' || s[0] == '\b')
			{
				if (s.size() == 1) // initialize in case you are editing an existing compartment
				{
					if (s[0] == '\r' && win->get_mode() == Window<Explorer>::edit_compart)
					{
						s.clear();
						s = temp_id;
					}
					else if (s[0] == '\b') s = "invalid";
				}
				else if (s.size() > 1)  // check for uniqueness of id 
				{
					string trial = s.substr(1, s.length() - 1);
					bool exists = model->compartments.find(trial) != model->compartments.end();
					if (win->get_mode() == Window<Explorer>::edit_compart)
					{
						bool eq_to_current = false;
						int iter = -2;
						for (const auto &i : model->compartments)
						{
							if (edited_item == iter && i.first==trial)
							{
								eq_to_current = true;
								break;
							}
							iter--;
						}
						if (exists && !eq_to_current) s = "invalid"; // found the compartment to exist and not be the current compartment
						else
						{
							editing_entity = true;
							temp_id = trial;
							s = "valid";
						}
					}
					else if (win->get_mode() == Window<Explorer>::add_compart)
					{
						if (exists) s = "invalid"; // found the compartment to exist 
						else
						{
							editing_entity = true;
							temp_id = trial;
							s = "valid";
						}
					}
				}
			}
		}
		else if (n == 2) //name
		{
			if (s[0] == '\r')
			{
				if (s.size() == 1 && win->get_mode() == Window<Explorer>::edit_compart) // initialize in case you are editing an existing compartment
				{
					s.clear();
					s = temp_compart.name;
				}
			}
			else if (s[0] == '\b')
			{
				string trial = s.substr(1, s.length() - 1);
				temp_compart.name = trial;
				s = "valid";
			}
		}
	}
	scr_upd = true;
}

void Explorer::edit_bound_cond(int n, vector<string> &s, int select, bool &new_text)
{
	if (select >= 0)
	{
		s.clear();
		if (select == 0)
		{
			editing_entity = true;
			temp_spec.boundary_condition = true;
			s.push_back("true");
		}
		else if (select == 1)
		{
			editing_entity = true;
			temp_spec.boundary_condition = false;
			s.push_back("false");
		}
	}
	else if (select == -1)
	{
		s.clear();
		s.push_back("$" + col_to_str(spec_on_c) + "true");
		s.push_back("\t");
		s.push_back("$" + col_to_str(spec_off_c) + "false");
	}
	else if (select == -2)
	{
		s.clear();
		if(win->get_mode() == Window<Explorer>::edit_spec) s.push_back(model->species.at(index_to_id[edited_item]).boundary_condition ? "true" : "false");
		else
		{
			editing_entity = true;
			s.push_back("false");
			temp_spec.boundary_condition = false;
		}
	}
	scr_upd = true;
}

void Explorer::edit_reversibility(int n, vector<string> &s, int select, bool &new_text)
{
	if (select >= 0)
	{
		s.clear();
		if (select == 0)
		{
			editing_entity = true;
			temp_reac.reversible = true;
			s.push_back("true");
		}
		else if (select == 1)
		{
			editing_entity = true;
			temp_reac.reversible = false;
			s.push_back("false");
		}
	}
	else if (select == -1)
	{
		s.clear();
		s.push_back("$" + col_to_str(spec_on_c) + "true");
		s.push_back("\t");
		s.push_back("$" + col_to_str(spec_off_c) + "false");
	}
	else if (select == -2)
	{
		s.clear();
		if (win->get_mode() == Window<Explorer>::edit_reac) s.push_back(model->reactions.at(index_to_id[edited_item]).reversible ? "true" : "false");
		else
		{
			editing_entity = true;
			s.push_back("false");
			temp_reac.reversible = false;
		}
	}
	scr_upd = true;
}

void Explorer::spec_reac_manipulate(int n, string &s)
{
	if (n == 0) // reset
	{
		string s;
		add_or_edit(current_aoe_action, s);
		editing_entity = false;
	}
	else if (n == 10) // cancel
	{
		win->set_mode(Window<Explorer>::neighbour_info);
		editing_entity = false;
	}
	else if(win->apply_current_mode())
	{
		if (n == 1 || n == 2) // edit or add species
		{
			if (n == 2) // add coordinates
			{
				default_random_engine generator;
				unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
				generator.seed(seed);
				uniform_real_distribution<double> distribution(-1.0, 1.0);

				double x1, y1, x2, y2;
				win->Get_cust_coors(x1, y1, x2, y2);
				double new_x = scr_c_x + 0.9 * scr_w * 0.5 * distribution(generator);
				double new_y = scr_c_y + 0.9 * scr_w / (x2 - x1) * (y2 - y1) * 0.5 * distribution(generator);
				temp_spec.setx(new_x);
				temp_spec.sety(new_y);
				temp_spec.set_backx(new_x);
				temp_spec.set_backy(new_y);
				
				model->add_species(temp_id, temp_id, { temp_spec });
			}
			else model->add_species(index_to_id[edited_item],temp_id, { temp_spec });
			win->apply_current_mode();
		}
		else if (n == 3 || n==4) // edit or add reaction
		{
			temp_reac.reactants.clear();
			temp_reac.products.clear();
			temp_reac.genes.clear();
			for (const auto &i : temp_reactants) temp_reac.reactants.emplace(i.first, i.second);
			for (const auto &i : temp_products) temp_reac.products.emplace(i.first, i.second);
			for (const string &i : temp_genes) temp_reac.genes.insert(i);

			if (n == 4) // add coordinates
			{
				int num_partic = 0;
				Point average(0.,0.);
				Point back_average(0.,0.);
				if ((temp_reac.reactants.size() + temp_reac.products.size()) > 1)
				{
					for (const auto &i : temp_reac.reactants)
					{
						num_partic++;
						average = average + Point(model->species.at(i.first).getx(), model->species.at(i.first).gety());
						back_average = back_average + Point(model->species.at(i.first).get_backx(), model->species.at(i.first).get_backy());
					}
					for (const auto &i : temp_reac.products)
					{
						num_partic++;
						average = average + Point(model->species.at(i.first).getx(), model->species.at(i.first).gety());
						back_average = back_average + Point(model->species.at(i.first).get_backx(), model->species.at(i.first).get_backy());
					}
					average *= 1. / (double)num_partic;
					back_average *= 1. / (double)num_partic;
				}
				else
				{
					default_random_engine generator;
					unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
					generator.seed(seed);
					uniform_real_distribution<double> distribution(-1.0, 1.0);

					double x1, y1, x2, y2;
					win->Get_cust_coors(x1, y1, x2, y2);
					average.x = scr_c_x + 0.9 * scr_w * 0.5 * distribution(generator);
					average.y = scr_c_y + 0.9 * scr_w / (x2 - x1) * (y2 - y1) * 0.5 * distribution(generator);

					back_average.x = average.x;
					back_average.y = average.y;
				}
				temp_reac.setx(average.x);
				temp_reac.sety(average.y);
				temp_reac.set_backx(back_average.x);
				temp_reac.set_backy(back_average.y);

				model->add_reaction(temp_id, temp_id, { temp_reac });
			}
			else model->add_reaction(index_to_id[edited_item], temp_id, { temp_reac });
			win->apply_current_mode();
		}
		else if (n == 5 || n==6) // edit or add compartment
		{
			string curedit;
			int iter = -2;
			for (const auto &i : model->compartments)
			{
				if (iter == edited_item) curedit = i.first;
				iter--;
			}
			if (curedit.empty()) curedit = temp_id;
			model->add_compartment(curedit, temp_id, { temp_compart });
			win->apply_current_mode();
		}
		editing_entity = false;
		update_arrays();
	}
	else printf("\nCHANGE COULD NOT BE APPLIED - check for red fields!\n");
	current_aoe_action = -1;
	scr_upd = true;
}

template<class R, class T, class... Args>
Loading::Loading(ALLEGRO_DISPLAY * disp, ALLEGRO_BITMAP * disp_map, double circle_r, double pos_x, double pos_y, T * cb_ob, R (T::*cb_fun_ptr)(Args...), Args... args)
{
    FONT * font = al_load_ttf_font(font_file_name.c_str(),circle_r*font_size_frac,0);
    bool check = false;
    auto waiting_thread = [] (bool &check,mutex &mtx, T * cb_ob, R (T::*cb_fun_ptr)(Args...), Args... args)
    {
        (cb_ob->*cb_fun_ptr)(args...);
        mtx.lock();
        check = true;
        mtx.unlock();
    };
    /// Launch job
    mutex mtx;
    thread th (waiting_thread,ref(check),ref(mtx),cb_ob,cb_fun_ptr,args...);
    /// Display the waiting animation
    mover m(pos_x,pos_y,circle_r-border_t*circle_r,FPS);

    ALLEGRO_TIMER * tim = al_create_timer(1.0 / FPS);
    if(!tim) fprintf(stderr, "\nLoading animation class failed to create timer!\n");
    ALLEGRO_EVENT_QUEUE * ev_q = al_create_event_queue();
    if(!ev_q) fprintf(stderr, "\nLoading animation class failed to create event queue!\n");

    al_set_target_bitmap(al_get_backbuffer(disp));
     
    al_register_event_source(ev_q, al_get_timer_event_source(tim));
    al_start_timer(tim);
    bool redraw = true;
    while(1)
    {
        if(mtx.try_lock())
        {
            if(check) {mtx.unlock(); break;}
            else mtx.unlock();
        }
        ALLEGRO_EVENT ev;
        al_wait_for_event(ev_q, &ev);
        if(ev.type == ALLEGRO_EVENT_TIMER) redraw=true;
        if(redraw && al_is_event_queue_empty(ev_q))
        {
            al_draw_bitmap(disp_map,0,0,0);
            al_draw_filled_circle(pos_x,pos_y,circle_r-border_t*circle_r,circle_c);
            al_draw_circle(pos_x,pos_y,circle_r-0.5*border_t*circle_r,circle_b_c,border_t*circle_r);
            al_draw_text(font,text_c,pos_x,pos_y-circle_r*font_size_frac*0.73,ALLEGRO_ALIGN_CENTRE,"Please wait");
            m.step_and_draw();
            al_flip_display();
            redraw=false;
        }
    }

    th.join();

    if(tim!=NULL) al_destroy_timer(tim);
    if(ev_q!=NULL) al_destroy_event_queue(ev_q);
    if(font!=NULL) al_destroy_font(font);
}

Loading::mover::mover(double pos_x,double pos_y,double circle_r,double FPS) : pos_x(pos_x), pos_y(pos_y), circle_r(circle_r), FPS(FPS)
{
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    generator.seed(seed);

    R = r * circle_r;
    segment_l = (2*l*R)/(double)num_segments;
    movs_per_step = (2*l*R)*speed / FPS / segment_l;

    for(int i=0;i<=num_segments;i++)
    {
		vector<double> sn = {pos_x-l*R+(double)i/num_segments*2*l*R,pos_y};
        snake.push_back(sn);
    }
    int iter = 1;
}

void Loading::mover::step_and_draw()
{
    normal_distribution<double> distribution (0.0,std_dev);
    /// move the centres of all circles
    auto sgn = [] (double val) { return (0.0 < val) - (val < 0.0); };

    for(int n=0;n<movs_per_step;n++)
    {
        double COS = (snake[num_segments][0]-snake[num_segments-1][0])/segment_l;
        COS = min(COS,1.0);
        COS = max(COS,-1.0);
        double old_angle = acos(COS)*sgn(snake[num_segments][1]-snake[num_segments-1][1]);
        for(int i=0;i<num_segments;i++) snake[i] = snake[i+1];
        double new_pos [2];
        do
        {
            double d_angle;
            do
            {
                d_angle = distribution(generator);
            } while(d_angle<(-M_PI) || d_angle>M_PI);
            double new_angle = old_angle + d_angle;
            new_pos[0] = snake.back()[0] + segment_l*cos(new_angle);
            new_pos[1] = snake.back()[1] + segment_l*sin(new_angle);
            old_angle=new_angle; 
        }while(sqrt(pow(new_pos[0]-pos_x,2)+pow(new_pos[1]-pos_y,2))>(circle_r-R));
        snake.back()[0] = new_pos[0];
        snake.back()[1] = new_pos[1];
    }

    /// draw the circles

    for(int i=0;i<=num_segments;i++) al_draw_filled_circle(snake[i][0],snake[i][1],(double)i/num_segments*R,mov_c);
}