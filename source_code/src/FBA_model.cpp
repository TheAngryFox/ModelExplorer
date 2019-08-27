#include <FBA_model.h>

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

bool FBA_model::empty()
{
	if ((species.size() + reactions.size()) == 0) return true;
	else return false;
}

void FBA_model::undo()
{
    if(!saves.empty() && current_pos>=0)
    {
        for(int i=(int)(saves.size()-1);i>=0;i--)
        {
            if(saves[i].num==current_pos)
            {
                if(saves[i].a==ADDED)
                {
                    for(const auto &j:saves[i].c) compartments.erase(j.first);
                    for(auto &j:saves[i].s) 
                    {
                        j.second.coor = species.at(j.first).coor;
						j.second.coor_back = species.at(j.first).coor_back;
						j.second.cm = species.at(j.first).cm;
						j.second.cbm = species.at(j.first).cbm;

                        j.second.dead = species.at(j.first).dead;
                        species.erase(j.first);
                    }
                    for(auto &j:saves[i].r) 
                    {
                        j.second.coor = reactions.at(j.first).coor;
						j.second.coor_back = reactions.at(j.first).coor_back;
						j.second.cm = reactions.at(j.first).cm;
						j.second.cbm = reactions.at(j.first).cbm;

                        j.second.dead = reactions.at(j.first).dead;
                        reactions.erase(j.first);
                    }
                }
                else if(saves[i].a==REMOVED)
                {
                    for(const auto &j:saves[i].c) compartments.insert(j);
                    for(const auto &j:saves[i].s) species.insert(j);
                    for(const auto &j:saves[i].r) reactions.insert(j);
                }
                else if(saves[i].a==CHANGED)
                {
                    for(auto &j:saves[i].c)
                    {
                        auto it = compartments.find(j.first);
                        if(it!=compartments.end()) 
                        {
                            compartment temp = (*it).second;
                            (*it).second = j.second;
                            j.second = temp;
                        }
                        else printf("\nWarning! In FBA_model::undo - could not find changed element!!\n\n");
                    }
                    for(auto &j:saves[i].s)
                    {
                        auto it = species.find(j.first);
                        if(it!=species.end()) 
                        {
                            j.second.coor = (*it).second.coor;
							j.second.coor_back = (*it).second.coor_back;
							j.second.cm = (*it).second.cm;
							j.second.cbm = (*it).second.cbm;

                            j.second.dead = (*it).second.dead;
                            specium temp = (*it).second;
                            (*it).second = j.second;
                            j.second = temp;
                        }
                        else printf("\nWarning! In FBA_model::undo - could not find changed element!!\n\n");
                    }
                    for(auto &j:saves[i].r)
                    {
                        auto it = reactions.find(j.first);
                        if(it!=reactions.end()) 
                        {
                            j.second.coor = (*it).second.coor;
							j.second.coor_back = (*it).second.coor_back;
							j.second.cm = (*it).second.cm;
							j.second.cbm = (*it).second.cbm;

                            j.second.dead = (*it).second.dead;
                            reaction temp = (*it).second;
                            (*it).second = j.second;
                            j.second = temp;
                        }
                        else printf("\nWarning! In FBA_model::undo - could not find changed element!!\n\n");
                    }
                }
            }
        }
        current_pos--;
    }
}

void FBA_model::redo()
{
    if(!saves.empty() && current_pos<saves.back().num)
    {
        for(int i=0;i<(int)saves.size();i++)
        {
            if(saves[i].num==(current_pos+1))
            {
                if(saves[i].a==ADDED)
                {
                    for(const auto &j:saves[i].c) compartments.insert(j);
                    for(const auto &j:saves[i].s) species.insert(j);
                    for(const auto &j:saves[i].r) reactions.insert(j);
                }
                else if(saves[i].a==REMOVED)
                {
                    for(const auto &j:saves[i].c) compartments.erase(j.first);
                    for(auto &j:saves[i].s) 
                    {
                        j.second.coor = species.at(j.first).coor;
						j.second.coor_back = species.at(j.first).coor_back;
						j.second.cm = species.at(j.first).cm;
						j.second.cbm = species.at(j.first).cbm;

                        j.second.dead = species.at(j.first).dead;
                        species.erase(j.first);
                    }
                    for(auto &j:saves[i].r) 
                    {
                        j.second.coor = reactions.at(j.first).coor;
						j.second.coor_back = reactions.at(j.first).coor_back;
						j.second.cm = reactions.at(j.first).cm;
						j.second.cbm = reactions.at(j.first).cbm;

                        j.second.dead = reactions.at(j.first).dead;
                        reactions.erase(j.first);
                    }
                }
                else if(saves[i].a==CHANGED)
                {
                    for(auto &j:saves[i].c)
                    {
                        auto it = compartments.find(j.first);
                        if(it!=compartments.end()) 
                        {
                            compartment temp = (*it).second;
                            (*it).second = j.second;
                            j.second = temp;
                        }
                        else printf("\nWarning! In FBA_model::undo - could not find changed element!!\n\n");
                    }
                    for(auto &j:saves[i].s)
                    {
                        auto it = species.find(j.first);
                        if(it!=species.end()) 
                        {
                            j.second.coor = (*it).second.coor;
							j.second.coor_back = (*it).second.coor_back;
							j.second.cm = (*it).second.cm;
							j.second.cbm = (*it).second.cbm;

                            j.second.dead = (*it).second.dead;
                            specium temp = (*it).second;
                            (*it).second = j.second;
                            j.second = temp;
                        }
                        else printf("\nWarning! In FBA_model::undo - could not find changed element!!\n\n");
                    }
                    for(auto &j:saves[i].r)
                    {
                        auto it = reactions.find(j.first);
                        if(it!=reactions.end()) 
                        {
                            j.second.coor = (*it).second.coor;
							j.second.coor_back = (*it).second.coor_back;
							j.second.cm = (*it).second.cm;
							j.second.cbm = (*it).second.cbm;

                            j.second.dead = (*it).second.dead;
                            reaction temp = (*it).second;
                            (*it).second = j.second;
                            j.second = temp;
                        }
                        else printf("\nWarning! In FBA_model::undo - could not find changed element!!\n\n");
                    }
                }
            }
        }
        current_pos++;
    }
}

void FBA_model::truncate_saves()
{
    for(size_t i=0;i<saves.size();i++)
    {
        if(saves[i].num>current_pos) 
        {
            saves.erase(saves.begin()+i,saves.end());
            break;
        }
    }
}

int FBA_model::purge_disconnected_species(bool truncate)
{
    if(truncate) truncate_saves();
    map<string,bool>mask;
    for(const auto &i:species) mask[i.first]=false;
    for(const auto &i:reactions)
    {
        for(auto &j:i.second.reactants) mask[j.first]=true;
        for(auto &j:i.second.products) mask[j.first]=true;
    }
    bool found_dead_specs = false;
    for(const auto &i:mask) if(i.second==false) {found_dead_specs=true; break;}

    int num_purged = 0;
    if(found_dead_specs) 
    {
        current_pos++;
        saves.resize(saves.size()+1);
        saves.back().a = REMOVED;
        saves.back().num = current_pos;
        for(const auto &i:mask) if(i.second==false) {saves.back().s.insert(*species.find(i.first)); species.erase(i.first); num_purged++;}
    }
    return num_purged;
}

vector<string> FBA_model::find_species_by_tag(string tag, int pos)
{
    vector<string> found_specs;
    if(pos==AT_END) 
    {
        for(const auto &i:species) 
        {
            if(i.first.length()>=tag.length() && i.first.substr(i.first.length()-tag.length(),tag.length())==tag)
            {
                found_specs.push_back(i.first);
            }
        }
    }
    else if(pos==ANYWHERE)
    {
        for(const auto &i:species) 
        {
            if(i.first.length()>=tag.length() && i.first.find(tag)!=string::npos)
            {
                found_specs.push_back(i.first);
            }
        }
    }
    return found_specs;
}

int FBA_model::purge_species_by_tag(string tag, int pos, bool truncate)
{
    if(truncate) truncate_saves();
    /// Find the species to be purged
    vector<string> species_to_purge = find_species_by_tag(tag,pos);

    if(species_to_purge.size()>0)
    {
        /// Make an array that for each species lists the names of reactions it is involved in (to remove the species from the corresponding reactions faster)
        map<string,set<string>> involved_in;
        for(const auto &i:reactions)
        {
            for(const auto &j:i.second.products) {involved_in[j.first].insert(i.first);}
            for(const auto &j:i.second.reactants) {involved_in[j.first].insert(i.first);}
        }
        for(const auto &i:species) involved_in.emplace(i.first,set<string>());
        /// Find the set of all reactions to be changed
        set<string> involved_reactions;
        for(const string &i:species_to_purge) for(const auto &j:involved_in.at(i)) involved_reactions.insert(j);

        current_pos++;
        /// Purge the species
        saves.resize(saves.size()+1);
        saves.back().a = REMOVED;
        int save_num = current_pos; 
        saves.back().num = save_num;
        for(const string &i:species_to_purge) {saves.back().s.insert(*species.find(i)); species.erase(i);}

        /// Save original reactions before changing them
        saves.resize(saves.size()+1);
        saves.back().a = CHANGED;
        saves.back().num = save_num;
        for(const string &i:involved_reactions) saves.back().r.insert(*reactions.find(i));

        /// Purge the references to the species in reactions
        for(const string &i:species_to_purge)
        {
            for(const auto &j:involved_in.at(i)) 
            {
                reactions.at(j).reactants.erase(i);
                reactions.at(j).products.erase(i);
            }
        }
    }

    return (int)species_to_purge.size();
}

bool FBA_model::purge_compartment(string id, bool truncate)
{
	if(truncate) truncate_saves();
	/// Find the compartment to be purged
	string purged_comp;
	vector<string> species_to_change;
	vector<string> compartments_to_change;
	for (const auto &i : compartments) if (i.first == id) { purged_comp = i.first; break; }

	if (!purged_comp.empty())
	{
		/// find the species clean for this compartment
		for (const auto &i : species) if (i.second.compart == purged_comp) species_to_change.push_back(i.first);
		for (const auto &i : compartments) if (i.second.outside == purged_comp) compartments_to_change.push_back(i.first);

		/// Purge the compartment and its reference
		
		/// clean the species
		current_pos++;
		saves.resize(saves.size() + 1);
		saves.back().a = CHANGED;
		int save_num = current_pos;
		saves.back().num = save_num;
		for (const string &i : species_to_change) { saves.back().s.insert(*species.find(i)); }
		for (const string &i : species_to_change) species.at(i).compart.clear();
		for (const string &i : compartments_to_change) { saves.back().c.insert(*compartments.find(i)); }
		for (const string &i : compartments_to_change) compartments.at(i).outside = "";

		/// remove the compartment 
		saves.resize(saves.size() + 1);
		saves.back().a = REMOVED;
		saves.back().num = save_num;
		saves.back().c.insert(*compartments.find(purged_comp));
		compartments.erase(purged_comp);

		return true;
	}
	else return false;
}

int FBA_model::purge_species_by_compartment(string name, bool truncate)
{
    if(truncate) truncate_saves();
    /// Find the compartment to be purged
    string purged_comp;
    vector<string> species_to_purge;
    for(const auto &i:compartments) if(i.first==name || i.second.name==name) {purged_comp=i.first; break;}

    if(!purged_comp.empty())
    {
        /// find the species to purge
        for(const auto &i:species) if(i.second.compart==purged_comp) species_to_purge.push_back(i.first);  

        /// Make an array that for each species lists the names of reactions it is involved in (to remove the species from the corresponding reactions faster)
        map<string,set<string>> involved_in;
        for(const auto &i:reactions)
        {
            for(const auto &j:i.second.products) {involved_in[j.first].insert(i.first);}
            for(const auto &j:i.second.reactants) {involved_in[j.first].insert(i.first);}
        }
        for(const auto &i:species) involved_in.emplace(i.first,set<string>());
        /// Find the set of all reactions to be changed
        set<string> involved_reactions;
        for(const string &i:species_to_purge) for(const auto &j:involved_in.at(i)) involved_reactions.insert(j);

        current_pos++;
        /// Purge the species
        saves.resize(saves.size()+1);
        saves.back().a = REMOVED;
        int save_num = current_pos; 
        saves.back().num = save_num;
        for(const string &i:species_to_purge) {saves.back().s.insert(*species.find(i)); species.erase(i);}

        /// Save original reactions before changing them
        saves.resize(saves.size()+1);
        saves.back().a = CHANGED;
        saves.back().num = save_num;
        for(const string &i:involved_reactions) saves.back().r.insert(*reactions.find(i));

        /// Purge the references to the species in reactions
        for(const string &i:species_to_purge)
        {
            for(const auto &j:involved_in.at(i)) 
            {
                reactions.at(j).reactants.erase(i);
                reactions.at(j).products.erase(i);
            }
        }
    }

    return (int)species_to_purge.size();
}

int FBA_model::purge_disconnected_reactions(bool truncate)
{
    if(truncate) truncate_saves();
    int num_reactions_before = (int)reactions.size();
    vector<string> rem_reacs;
    for(const auto &i:reactions) if(i.second.reactants.size()==0 && i.second.products.size()==0) rem_reacs.push_back(i.first);
    return purge_reactions(rem_reacs);
}

int FBA_model::purge_dead(int dead_type, bool truncate)
{
    if(truncate) truncate_saves();
    int num_reactions_before = (int)reactions.size();
	int num_species_before = (int)species.size();
    vector<string> rem_reacs;
	vector<string> rem_specs;
    for(const auto &i:reactions) if(i.second.dead[dead_type]) rem_reacs.push_back(i.first);
	for(const auto &i:species) if(i.second.dead[dead_type]) rem_specs.push_back(i.first);
    return purge_reactions(rem_reacs) + purge_species(rem_specs,true,true);
}

void find_subgraph(const string &n, const int &curr_group, map<string,set<string>> &adjacency,map<string,int> &subgraphs)
{
    if(subgraphs.at(n)==0)
    {
        subgraphs.at(n)=curr_group;
        for(const string &i:adjacency.at(n)) 
        {
            find_subgraph(i,curr_group,adjacency,subgraphs);
        }
    }
}

int FBA_model::purge_disconnected_clusters(bool truncate)
{
    if(truncate) truncate_saves();
    /// Find all the disconnected subgraphs in the model
    map<string,set<string>> adjacency;
    map<string,int> subgraphs;

    for(const auto &i:reactions) adjacency.emplace(i.first,set<string>());
    for(const auto &i:species) adjacency.emplace(i.first,set<string>());

    for(const auto &i:reactions) subgraphs.emplace(i.first,0);
    for(const auto &i:species) subgraphs.emplace(i.first,0);

    for(const auto &i:reactions)
    {
        for(const auto &j:i.second.reactants) 
        {
            adjacency.at(i.first).insert(j.first);
            adjacency.at(j.first).insert(i.first);
        }
        for(const auto &j:i.second.products) 
        {
            adjacency.at(i.first).insert(j.first);
            adjacency.at(j.first).insert(i.first);
        }
    } 
    auto front = subgraphs.begin();
    int curr_group = 0;
    while(front!=subgraphs.end())
    {
        curr_group++;
        find_subgraph(front->first,curr_group,adjacency,subgraphs);
        while(front!=subgraphs.end() && subgraphs.at(front->first)!=0) ++front;
    }
    vector<int> subgraph_sizes(curr_group,0);
    for(const auto &i:subgraphs) subgraph_sizes[i.second-1]++;
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
    vector<string> rem_reacs;
    vector<string> rem_specs;
    for(const auto &i:subgraphs) 
    {
        if(i.second!=largest_cluster) 
        {
            if(reactions.find(i.first)!=reactions.end()) rem_reacs.push_back(i.first);
            else rem_specs.push_back(i.first);
        }
    }
    int prev_pos = current_pos;
    int purged_reacs = reactions.size() - purge_reactions(rem_reacs);
    current_pos = prev_pos;
    int purged_specs = species.size() - purge_species(rem_specs,false);
    return purged_reacs + purged_specs;
}

int FBA_model::purge_reactions(const vector<string> &rem_reacs, bool truncate, bool add_to_previous)
{
	if(truncate) truncate_saves();
    if(rem_reacs.size()>0)
    {
		if(!add_to_previous)
		{
			current_pos++;
			saves.resize(saves.size()+1);
		}
		if(saves.back().a!=REMOVED && saves.back().a!=NAN_ACTION) printf(A_RED "\n\nWarning! FBA_model::purge_reactions trying to write over save of different type!\n\n" A_RES);
        saves.back().a = REMOVED;
        saves.back().num = current_pos;
        for(const string &i:rem_reacs) 
        {
            saves.back().r.insert(*reactions.find(i));
            reactions.erase(i);
        }
    }
    return rem_reacs.size();
}

int FBA_model::purge_species(const vector<string> &rem_specs, bool truncate, bool add_to_previous) /// Bruteforce purges all given species
{
	if(truncate) truncate_saves();
    if(rem_specs.size()>0)
    {
		if(!add_to_previous)
		{
			current_pos++;
			saves.resize(saves.size()+1);
		}
		if(saves.back().a!=REMOVED && saves.back().a!=NAN_ACTION) printf(A_RED "\n\nWarning! FBA_model::purge_species trying to write over save of different type!\n\n" A_RES);
        saves.back().a = REMOVED;
        saves.back().num = current_pos;
        for(const string &i:rem_specs) 
        {
			/// find reactions using this species
			vector<string> rem_reacs;
			for (const auto &j : reactions) if (j.second.products.find(i) != j.second.products.end() || j.second.reactants.find(i) != j.second.reactants.end()) rem_reacs.push_back(j.first);
			for (const string &i : rem_reacs)
			{
				saves.back().r.insert(*reactions.find(i));
				reactions.erase(i);
			}
            saves.back().s.insert(*species.find(i));
            species.erase(i);
        }
    }
    return rem_specs.size();
}

int FBA_model::purge_species_weak(const vector<string> &rem_specs, bool truncate) /// Purges all given species but not participating reactions
{
	if(truncate) truncate_saves();
    if(rem_specs.size()>0)
    {
        current_pos++;
        saves.resize(saves.size()+1);
        saves.back().a = REMOVED;
        saves.back().num = current_pos;
        for(const string &i:rem_specs) 
        {
            saves.back().s.insert(*species.find(i));
            species.erase(i);
        }

        saves.resize(saves.size()+1);
        saves.back().a = CHANGED;
        saves.back().num = current_pos;
        for(const string &i:rem_specs) 
        {
			/// find reactions using this species and remove the species from them
			for (auto &j : reactions) 
			{
				if (j.second.products.find(i) != j.second.products.end() || j.second.reactants.find(i) != j.second.reactants.end()) 
				{
					saves.back().r.insert(*reactions.find(j.first));
					j.second.reactants.erase(i);
					j.second.products.erase(i);
				}
			}
        }
    }
    return rem_specs.size();
}

int FBA_model::load(string file)
{
    saves.clear();
    current_pos=-1;

    string model_text;

    /// Load file
    ifstream model_file (file,ios::in);
    if(model_file.fail()) 
	{
		printf(A_RED "\n\n FBA_model::load: Could not load file with the given name!\n\n" A_RES);
		return 1;
	}
    model_file.seekg(0, ios::end);
    model_text.reserve(model_file.tellg());
    model_file.seekg(0, ios::beg);
    model_text.assign((istreambuf_iterator<char>(model_file)), istreambuf_iterator<char>());
    model_file.close();

    /// Make its tree
    pugi::xml_document model_tree;
    pugi::xml_parse_result result = model_tree.load_string(&model_text[0]);

    if (result)
    {
        /// Is the SBML and model there or not?
    
        if(!model_tree.child("sbml")) {printf(A_RED "\n\n FBA_model::load content error. No SBML branch found in the file!!!!\n\n" A_RES); return -2;}
        if(!model_tree.child("sbml").child("model")) {printf(A_RED "\n\n FBA_model::load content error. No model branch found in the file!!!!\n\n" A_RES); return -2;}

        pugi::xml_node model = model_tree.child("sbml").child("model");

        ///Find the model id

        if(!model.attribute("id")) model_id = "nan";
        else model_id = model.attribute("id").value();

        ///Find flux unit used

        if(!model.child("listOfUnitDefinitions").child("unitDefinition").attribute("id")) unit = "nan";
        else unit = model.child("listOfUnitDefinitions").child("unitDefinition").attribute("id").value();

        ///Find compartment list

        if(model.child("listOfCompartments"))
        {
            for (pugi::xml_node c = model.child("listOfCompartments").first_child(); c; c = c.next_sibling())
            {
                if((string)c.name()=="compartment")
                {
                    compartment comp;
                    string id;
                    if(c.attribute("id")) id = c.attribute("id").value();
                    else 
                    {
                        printf("\n\n" A_RED " Error!" A_RES " In FBA_model::load could not load compartment id!\n\n");
                        return -3;
                    }
                    if(c.attribute("name")) comp.name = c.attribute("name").value();
                    else printf("\n\n Warning! In FBA_model::load could not load compartment [%s] name!\n\n",id.c_str());
                    if(c.attribute("outside")) comp.outside = c.attribute("outside").value();

                    compartments.emplace(id,comp);
                }
            }
        }

        ///Find species list

        if(model.child("listOfSpecies"))
        {
            for (pugi::xml_node c = model.child("listOfSpecies").first_child(); c; c = c.next_sibling())
            {
                if((string)c.name()=="species")
                {

                    specium s;
                    string id;

                    if(c.attribute("id")) id = c.attribute("id").value();
                    else 
                    {
                        printf("\n\n " A_RED "Error!" A_RES " In FBA_model::load could not load species id!\n\n");
                        return -3;
                    }

                    if(c.attribute("name"))
                    {
                        string uncut_name = c.attribute("name").value();

                        size_t underscore = uncut_name.find("_");
                        if(underscore!=string::npos)
                        {
                            s.formula = uncut_name.substr(underscore+1);
                            s.name = uncut_name.substr(0,underscore);
                        }
                        else s.name = uncut_name;
                    }
                    else printf("\n\n Warning! In FBA_model::load could not load species name!\n\n");

                    if(c.attribute("kegg"))
                    {
                        string temp_kegg = c.attribute("kegg").value();
                        replace_mul(temp_kegg," ","");
                        if(!temp_kegg.empty()) s.kegg = c.attribute("kegg").value();
                    }

                    if(c.attribute("compartment"))
                    {
                        bool found = false;
                        s.compart = c.attribute("compartment").value();
                        if(compartments.find(s.compart)==compartments.end()) 
                        {
                            printf("\n " A_YEL "Warning!" A_RES " In FBA_model::load species " 
                                                A_YEL "%s" A_RES " belongs to an undefined compartment " 
                                                A_YEL "%s" A_RES " ! Skipping undefined compartment. ",id.c_str(),s.compart.c_str());
                            s.compart = "";
                        } 
                    }

                    if(c.attribute("boundaryCondition")) s.boundary_condition = ((string)c.attribute("boundaryCondition").value()=="true") ? true : false;

                    species.emplace(id,s);
                }
            }
        }
        else {printf(A_RED "\n\n FBA_model::load content error. No species are found in the SBML file!!!!\n\n" A_RES); return -3;}

        /// Find reactions list

        if(model.child("listOfReactions"))
        {
            for (pugi::xml_node c = model.child("listOfReactions").first_child(); c; c = c.next_sibling())
            {
                if((string)c.name()=="reaction")
                {
                    reaction r;
                    string id;

                    if(c.attribute("id")) id = c.attribute("id").value();
                    else 
                    {
                        printf("\n\n " A_RED "Error!" A_RES " In FBA_model::load could not load reaction id!\n\n");
                        return -3;
                    }

                    if(c.attribute("name")) r.name = c.attribute("name").value();
                    else printf("\n\n Warning! In FBA_model::load could not load reaction name!\n\n");

					if (c.attribute("kegg"))
					{
						string temp_kegg = c.attribute("kegg").value();
						replace_mul(temp_kegg, " ", "");
						if (!temp_kegg.empty()) r.kegg = c.attribute("kegg").value();
					}

                    if(c.attribute("reversible")) r.reversible = ((string)c.attribute("reversible").value()=="true") ? true : false;

                    /// Load the genetic association 

                    if(c.child("notes"))
                    {
                        string assoc = "GENE_ASSOCIATION:";
                        string notes = c.child("notes").child_value();
                        if(notes.find(assoc,0)!=string::npos)
                        {
                            smatch match;
                            replace_mul(notes,assoc,"");
                            regex t("([A-Z0-9-]+)");
                            while(regex_search(notes,match,t))
                            {
                                r.genes.insert(match[0]);
                                notes=match.suffix();
                            }
                        }
                    }

                    ///Add reactants

                    if(c.child("listOfReactants"))
                    {
                        for(pugi::xml_node s = c.child("listOfReactants").first_child(); s; s = s.next_sibling())
                        {
                            if((string)s.name()=="speciesReference")
                            {
                                if(s.attribute("species"))
                                {
                                    string spec = s.attribute("species").value();
                                    double stoich;
                                    if(species.find(spec)==species.end()) // if did not find species in the species list, give out a warning and skip species
                                    {
                                        printf("\n " A_YEL "Warning!" A_RES " In FBA_model::load could not find reactant " 
                                                A_YEL "%s" A_RES " from reaction " 
                                                A_YEL "%s" A_RES " in species list. Skipping empty reactant!",spec.c_str(),id.c_str());
                                    }
                                    else
                                    {
                                        if(s.attribute("stoichiometry")) stoich = atof(s.attribute("stoichiometry").value());
                                        else 
                                        {
                                            stoich = 1.0;
                                            printf("\n " A_YEL "Warning!" A_RES " In FBA_model::load could not load reactant stoichiometry for " A_YEL "%s" A_RES "! Assumed = 1.0.",id.c_str());
                                        }
                                        r.reactants.emplace(spec,stoich);
                                    }
                                }
                                else printf("\n\n Warning! In FBA_model::load could not load reactant species!\n\n");
                            }
                        }
                    }

                    ///Add products

                    if(c.child("listOfProducts"))
                    {
                        for(pugi::xml_node s = c.child("listOfProducts").first_child(); s; s = s.next_sibling())
                        {
                            if((string)s.name()=="speciesReference")
                            {
                                if(s.attribute("species"))
                                {
                                    string spec = s.attribute("species").value();
                                    double stoich;
                                    if(species.find(spec)==species.end()) // if did not find species in the species list, give out a warning and skip species
                                    {
                                        printf("\n " A_YEL "Warning!" A_RES " In FBA_model::load could not find product " 
                                                A_YEL "%s" A_RES " from reaction " 
                                                A_YEL "%s" A_RES " in species list. Skipping empty product!",spec.c_str(),id.c_str());
                                    }
                                    else
                                    {
                                        if(s.attribute("stoichiometry")) stoich = atof(s.attribute("stoichiometry").value());
                                        else 
                                        {
                                            stoich = 1.0;
                                            printf("\n " A_YEL "Warning!" A_RES " In FBA_model::load could not load product stoichiometry for " A_YEL "%s" A_RES "! Assumed = 1.0.",id.c_str());
                                        }
                                        r.products.emplace(spec,stoich);
                                    }
                                }
                                else printf("\n\n Warning! In FBA_model::load could not load reactant species!\n\n");
                            }
                        }
                    }

                    ///Add bounds and objective coefficients

                    if(c.child("kineticLaw"))
                    {
                        if(c.child("kineticLaw").child("listOfParameters"))
                        {
                            for (pugi::xml_node p = c.child("kineticLaw").child("listOfParameters").first_child(); p; p = p.next_sibling())
                            {
                                if((string)p.name()=="parameter")
                                {
                                    string id = p.attribute("id").value();
                                    if(id=="LOWER_BOUND") r.low_bound=atof(p.attribute("value").value());
                                    else if(id=="UPPER_BOUND") r.up_bound=atof(p.attribute("value").value());
                                    else if(id=="OBJECTIVE_COEFFICIENT") r.obj_coeff=atof(p.attribute("value").value());
                                }
                            }
                        }
                    }

                    reactions.emplace(id,r);
                }
            }

        }
        else {printf(A_RED "\n\n FBA_model::load content error. No reactions are found in the SBML file!!!!\n\n" A_RES); return -4;}

        printf(A_GRE "\n " A_RES);
        for(int i=0;i<95;i++) printf(A_GRE "/" A_RES);
        printf(A_GRE "\n%-94s//\n //   " A_CYA "%-23s%-23s%-23s%s" A_GRE "   //"," //","Opened Model:","Flux Unit:","Species Number:","Reaction Number:");
        printf("\n //   " A_YEL "%-23s%-23s%-23i%-19i" A_GRE "//\n%-94s//\n " A_RES,model_id.c_str(),unit.c_str(),(int)species.size(),(int)reactions.size()," //");
        for(int i=0;i<95;i++) printf(A_GRE "/" A_RES);
        printf("\n");

    }
    else
    {
        printf(A_RED "\n\n!!!!! FBA_model::load SBML parsing error: %s !!!!!\n\n",result.description());
        if((int)model_text.length()<(result.offset+100)) model_text.resize(result.offset+100);
        printf(A_YEL "Error in: \n %s \n\n" A_RES,&model_text[result.offset]);
        return -1;
    }

    return 0;
}

int FBA_model::save(string file)
{
    /// Open file
    ofstream model_file (file,ios::out | ios::trunc);

    /// Make tree
    pugi::xml_document model_tree;

	pugi::xml_node decl = model_tree.prepend_child(pugi::node_declaration);
	decl.append_attribute("version") = "1.0";
	decl.append_attribute("encoding") = "UTF-8";

    /// Add sbml and model nodes

    pugi::xml_node sbml = model_tree.append_child("sbml");
	pugi::xml_attribute xmlns = sbml.append_attribute("xmlns");
    pugi::xml_attribute sbml_level =  sbml.append_attribute("level");
    pugi::xml_attribute sbml_version =  sbml.append_attribute("version");
	xmlns.set_value("http://www.sbml.org/sbml/level2");
    sbml_level.set_value(2);
    sbml_version.set_value(1);

    pugi::xml_node model = sbml.append_child("model");
    pugi::xml_attribute mod_id = model.append_attribute("id");
    mod_id.set_value(&model_id[0]);

    ///Set flux unit used

    pugi::xml_node unit_defs = model.append_child("listOfUnitDefinitions");
    pugi::xml_node unit_def = unit_defs.append_child("unitDefinition");
    pugi::xml_attribute model_unit = unit_def.append_attribute("id");
    model_unit.set_value(&unit[0]);

    ///Make compartment list

    pugi::xml_node comp_list = model.append_child("listOfCompartments");
    for(const auto &c:compartments)
    {
        pugi::xml_node compart = comp_list.append_child("compartment");
        pugi::xml_attribute comp_id = compart.append_attribute("id");
        comp_id.set_value(&c.first[0]);
        pugi::xml_attribute comp_name = compart.append_attribute("name");
        comp_name.set_value(&c.second.name[0]);
        if(!c.second.outside.empty())
        {
            pugi::xml_attribute comp_out = compart.append_attribute("outside");
            comp_out.set_value(&c.second.outside[0]);
        }
    }

    ///Make species list

    pugi::xml_node spec_list = model.append_child("listOfSpecies");
    for(const auto &s:species)
    {
        pugi::xml_node spec = spec_list.append_child("species");
        pugi::xml_attribute spec_id = spec.append_attribute("id");
        spec_id.set_value(&s.first[0]);
        if(s.second.kegg.length()>0)
        {
            pugi::xml_attribute spec_kegg = spec.append_attribute("kegg");
            spec_kegg.set_value(&s.second.kegg[0]);
        }
        pugi::xml_attribute spec_name = spec.append_attribute("name");
        string aug_name = s.second.name + "_" + s.second.formula;
        replace_mul(aug_name,"'","&");
        spec_name.set_value(&aug_name[0]);
        if(!s.second.compart.empty())
        {
            pugi::xml_attribute spec_comp = spec.append_attribute("compartment");
            spec_comp.set_value(&s.second.compart[0]);
        }
    }

    /// Find reactions list

    pugi::xml_node reac_list = model.append_child("listOfReactions");
    for(const auto &r:reactions)
    {
        pugi::xml_node reac = reac_list.append_child("reaction");
        pugi::xml_attribute reac_id = reac.append_attribute("id");
        reac_id.set_value(&r.first[0]);
		if (r.second.kegg.length()>0)
		{
			pugi::xml_attribute spec_kegg = reac.append_attribute("kegg");
			spec_kegg.set_value(&r.second.kegg[0]);
		}
        pugi::xml_attribute reac_name = reac.append_attribute("name");
        string aug_name = r.second.name;
        replace_mul(aug_name,"'","&");
        reac_name.set_value(&aug_name[0]);
        pugi::xml_attribute reac_rev = reac.append_attribute("reversible");
        reac_rev.set_value((r.second.reversible) ? "true" : "false");

        /// add gene names
        pugi::xml_node reac_notes = reac.append_child("notes");
        string assoc = "GENE_ASSOCIATION:";
        for(const string &i:r.second.genes) assoc += " " + i;
        pugi::xml_node reac_genes = reac_notes.append_child(pugi::node_pcdata);
        reac_genes.set_value(&assoc[0]);

        /// add reactants

        if(r.second.reactants.size()>0)
        {
            pugi::xml_node reac_reac_list = reac.append_child("listOfReactants");
            for(const auto &s:r.second.reactants)
            {
                pugi::xml_node reac_reac = reac_reac_list.append_child("speciesReference");
                pugi::xml_attribute reac_reac_spec = reac_reac.append_attribute("species");
                reac_reac_spec.set_value(&s.first[0]);
                char val [256];
                sprintf(val,"%g",s.second);
                pugi::xml_attribute reac_reac_stoi = reac_reac.append_attribute("stoichiometry");
                reac_reac_stoi.set_value(val);
            }
        }

        /// add products
        if(r.second.products.size()>0)
        {
            pugi::xml_node reac_prod_list = reac.append_child("listOfProducts");
            for(const auto &s:r.second.products)
            {
                pugi::xml_node reac_prod = reac_prod_list.append_child("speciesReference");
                pugi::xml_attribute reac_prod_spec = reac_prod.append_attribute("species");
                reac_prod_spec.set_value(&s.first[0]);
                char val [256];
                sprintf(val,"%g",s.second);
                pugi::xml_attribute reac_prod_stoi = reac_prod.append_attribute("stoichiometry");
                reac_prod_stoi.set_value(val);
            }
        }

        ///Add bounds and objective coefficients

        pugi::xml_node reac_kin = reac.append_child("kineticLaw");
        pugi::xml_node reac_param_list = reac_kin.append_child("listOfParameters");
        if(r.second.low_bound!=0 || r.second.up_bound!=0)
        {
            pugi::xml_node reac_param_l = reac_param_list.append_child("parameter");
            pugi::xml_attribute param_id_l = reac_param_l.append_attribute("id");
            param_id_l.set_value("LOWER_BOUND");
            pugi::xml_attribute param_val_l = reac_param_l.append_attribute("value");
            param_val_l.set_value(r.second.low_bound);

            pugi::xml_node reac_param_u = reac_param_list.append_child("parameter");
            pugi::xml_attribute param_id_u = reac_param_u.append_attribute("id");
            param_id_u.set_value("UPPER_BOUND");
            pugi::xml_attribute param_val_u = reac_param_u.append_attribute("value");
            param_val_u.set_value(r.second.up_bound);
        }
        if(r.second.obj_coeff!=0)
        {
            pugi::xml_node reac_param = reac_param_list.append_child("parameter");
            pugi::xml_attribute param_id = reac_param.append_attribute("id");
            param_id.set_value("OBJECTIVE_COEFFICIENT");
            pugi::xml_attribute param_val = reac_param.append_attribute("value");
            param_val.set_value(r.second.obj_coeff);
        }
    }

    model_tree.save(model_file);
    model_file.close();

    printf("\n ");
    for(int i=0;i<95;i++) printf(A_MAG "/" A_RES);
    printf(A_MAG "\n%-94s//\n //   " A_BLU "%-23s%-23s%-23s%s" A_MAG "   //"," //","Saved Model:","Flux Unit:","Species Number:","Reaction Number:");
    printf("\n //   " A_MAG "%-23s%-23s%-23i%-19i" A_MAG "//\n%-94s//\n " A_RES,model_id.c_str(),unit.c_str(),(int)species.size(),(int)reactions.size()," //");
    for(int i=0;i<95;i++) printf(A_MAG "/" A_RES);
    printf("\n");

    return 0;
}

int replace_mul(string &str,string query,string replacement)
{
    int num = 0;
    size_t beg = str.find(query,0);
    while(beg!=string::npos)
    {
        num++;
        str.replace(beg,query.size(),replacement);
        beg = str.find(query,0);
    }
    return num;
}

bool FBA_model::add_compartment(string old_id, string id, const compartment &c)
{
	truncate_saves();
	/// Find the compartment to be replaced
	string replaced_comp;
	vector<string> species_to_change;
	vector<string> compartments_to_change;
	if (compartments.find(old_id) != compartments.end()) replaced_comp = old_id;

	if (!replaced_comp.empty())  // if compartment exists, replace it
	{
		/// find the species and refering compartments to change for this compartment
		for (const auto &i : species) if (i.second.compart == replaced_comp) species_to_change.push_back(i.first);
		for (const auto &i : compartments) if (i.second.outside == replaced_comp) compartments_to_change.push_back(i.first);

		/// Replace the compartment and its references

		/// change the species and refering compartments
		current_pos++;
		saves.resize(saves.size() + 1);
		saves.back().a = CHANGED;
		int save_num = current_pos;
		saves.back().num = save_num;
		for (const string &i : species_to_change) { saves.back().s.insert(*species.find(i)); }
		for (const string &i : species_to_change) species.at(i).compart = id;
		for (const string &i : compartments_to_change) { saves.back().c.insert(*compartments.find(i)); }
		for (const string &i : compartments_to_change) compartments.at(i).outside = id;

		/// replace the compartment 
		saves.resize(saves.size() + 1);
		saves.back().a = REMOVED;
		saves.back().num = save_num;
		saves.back().c.insert(*compartments.find(replaced_comp));
		compartments.erase(replaced_comp);

		saves.resize(saves.size() + 1);
		saves.back().a = ADDED;
		saves.back().num = save_num;
		saves.back().c.emplace(id, c);
		compartments.emplace(id, c);
	}
	else // if compartment does not exist, add a new one
	{
		/// add the compartment 
		current_pos++;
		saves.resize(saves.size() + 1);
		saves.back().a = ADDED;
		int save_num = current_pos;
		saves.back().num = save_num;
		saves.back().c.insert(pair<string,compartment>(id,c));
		compartments.emplace(id, c);
	}
	
	return true;
}

bool FBA_model::add_reaction(string old_id, string id, const reaction &r)
{
	truncate_saves();
	/// Find the reaction to be replaced
	string replaced_reac;
	if (reactions.find(old_id) != reactions.end()) replaced_reac = old_id;

	if (!replaced_reac.empty())  // if reaction exists, replace it
	{
		/// change the reactions
		current_pos++;
		saves.resize(saves.size() + 1);
		saves.back().a = REMOVED;
		int save_num = current_pos;
		saves.back().num = save_num;
		saves.back().r.insert(*reactions.find(replaced_reac));
		reactions.erase(replaced_reac);

		saves.resize(saves.size() + 1);
		saves.back().a = ADDED;
		saves.back().num = save_num;
		saves.back().r.emplace(id,r);
		reactions.emplace(id, r);
	}
	else // if reaction does not exist, add a new one
	{
		/// add the reaction 
		current_pos++;
		saves.resize(saves.size() + 1);
		saves.back().a = ADDED;
		int save_num = current_pos;
		saves.back().num = save_num;
		saves.back().r.insert(pair<string, reaction>(id, r));
		reactions.emplace(id, r);
	}
	return true;
}

bool FBA_model::add_species(string old_id, string id, const specium &s)
{
	truncate_saves();
	/// Find the species to be replaced
	string replaced_spec;
	vector<string> reactions_to_change;
	if (species.find(old_id) != species.end()) replaced_spec = old_id;

	if (!replaced_spec.empty())  // if speices exists, replace it
	{
		/// find the refering reactions to change them 
		for (const auto &i : reactions) if (i.second.reactants.find(replaced_spec)!=i.second.reactants.end() || i.second.products.find(replaced_spec) != i.second.products.end()) reactions_to_change.push_back(i.first);

		/// change the species
		current_pos++;
		saves.resize(saves.size() + 1);
		saves.back().a = CHANGED;
		int save_num = current_pos;
		saves.back().num = save_num;
		for (const string &i : reactions_to_change) { saves.back().r.insert(*reactions.find(i)); }
		for (const string &i : reactions_to_change)
		{
			if (reactions.at(i).reactants.find(replaced_spec) != reactions.at(i).reactants.end())
			{
				double stoich = reactions.at(i).reactants.at(replaced_spec);
				reactions.at(i).reactants.erase(replaced_spec);
				reactions.at(i).reactants.emplace(id, stoich);
			}
			if (reactions.at(i).products.find(replaced_spec) != reactions.at(i).products.end())
			{
				double stoich = reactions.at(i).products.at(replaced_spec);
				reactions.at(i).products.erase(replaced_spec);
				reactions.at(i).products.emplace(id, stoich);
			}
		}

		current_pos++;
		saves.resize(saves.size() + 1);
		saves.back().a = REMOVED;
		saves.back().num = save_num;
		saves.back().s.insert(*species.find(replaced_spec));
		species.erase(replaced_spec);

		current_pos++;
		saves.resize(saves.size() + 1);
		saves.back().a = ADDED;
		saves.back().num = save_num;
		saves.back().s.emplace(id, s);
		species.emplace(id, s);
	}
	else // if species does not exist, add a new one
	{
		/// add the species 
		current_pos++;
		saves.resize(saves.size() + 1);
		saves.back().a = ADDED;
		int save_num = current_pos;
		saves.back().num = save_num;
		saves.back().s.insert(pair<string, specium>(id, s));
		species.emplace(id, s);
	}
	return true;
}
