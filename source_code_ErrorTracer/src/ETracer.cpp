#include <ETracer.h>

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

ETracer::ETracer(string filename, const vector<string> &flags, int &result) : flags(flags)
{
	if (!filename.empty())
	{
		result = load_FBA_model(filename);
	}

	// If model succesfully loaded, save the error tracing results to file
	if(result==0)
	{

		boost::filesystem::path p(filename);
		ifname = p.stem().string();
		results_to_XML();
	}
}

ETracer::~ETracer()
{
    delete model;
}

void ETracer::results_to_XML()
{
	string output_fpath;
	if (ofname.empty())
	{
		output_fpath = (boost::filesystem::path(odir)/boost::filesystem::path("ErrorTracer_results_"+ifname+".xml")).string();
	} 
	else
	{
		output_fpath = (boost::filesystem::path(odir)/boost::filesystem::path(ofname)).string();
	}
	
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

int ETracer::load_FBA_model(string filename)
{
	model = new FBA_model();
	int ercode = model->load(filename);
	if (ercode != 0) return ercode;

	// Show calculation times
	if(find(flags.begin(),flags.end(),"-time")!=flags.end()) print_times = true;
	else print_times = false;

	// Set output directory
	std::vector<string>::iterator ofileit = find(flags.begin(),flags.end(),"-o");
	if(ofileit!=flags.end() && ofileit!=prev(flags.end()))
	{
		string path = *next(ofileit);
		if(boost::filesystem::exists(path) && boost::filesystem::is_directory(path))
		{
			boost::filesystem::path p(path);
			odir = p.string();
		}
		else if(boost::filesystem::exists(boost::filesystem::path(path).parent_path()))
		{
			boost::filesystem::path p(path);
			odir = p.parent_path().string();
			ofname = p.filename().string();
		}
		else
		{
			boost::filesystem::path p(path);
			ofname = p.filename().string();
		}
		
	}

    update_arrays();
    update_blocked();

    return 0;
}

void ETracer::find_subgraph(const int &n, const int &curr_group,const vector<vector<int> > &adjacency,vector<int> &subgraphs)
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


void ETracer::update_arrays()
{
	sumreac = model->reactions.size();
    sumspec = model->species.size();
    sumtot = sumreac+sumspec;

    links.clear();
	neighbours.clear();
    reactants.clear();
    products.clear();
    reversible.clear();
    index_to_id.clear();
    r_id_to_index.clear();
    s_id_to_index.clear();

    /// Update the deadness of each species using the data stored in the model
    FBA_dead_specs.clear();
    FBA_dead.clear();
    for(auto &i:model->species) FBA_dead_specs.push_back(i.second.dead[0]);
    for(auto &i:model->reactions) FBA_dead.push_back(i.second.dead[0]);


    /// Make links from indicies in the coordinates array to names in the species and reactions maps
    for(const auto &i:model->reactions) index_to_id.push_back(i.first);
    for(const auto &i:model->species) index_to_id.push_back(i.first);
	
    /// Make a map from id to index
    for(int i=0;i<sumreac;i++) r_id_to_index.emplace(index_to_id[i],i);
    for(int i=sumreac;i<sumtot;i++) s_id_to_index.emplace(index_to_id[i],i-sumreac);

    /// Add the reversible array
    for(const auto &i:model->reactions) reversible.push_back(i.second.reversible);

    /// Make links from reactions to species
    reactants.resize(sumreac);
    products.resize(sumreac);
    int iter = 0;
    for(const auto &i:model->reactions) 
    {
		for(const auto &j:i.second.reactants) 
		{
			reactants[iter].push_back(s_id_to_index.at(j.first)+sumreac); 
		}
		iter++;
    }
    iter = 0;

    for(const auto &i:model->reactions) 
    {
		for(const auto &j:i.second.products) 
		{
			products[iter].push_back(s_id_to_index.at(j.first)+sumreac); 
		}
		iter++;
    }

    /// Find links between the species/reactions
    for(int i=0;i<sumreac;i++)
    {
        for(const int &j:reactants[i]) links.push_back({j,i,reversible[i]});
        for(const int &j:products[i]) links.push_back({i,j,reversible[i]});
    }

    /// Find neighbours of each species/reaction 
	neighbours.resize(sumtot);
    for(auto &i:links)
    {
		neighbours[i[0]].push_back(i[1]);
		neighbours[i[1]].push_back(i[0]);
    }

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
}