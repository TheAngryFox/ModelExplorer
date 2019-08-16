#include <ETracer.h>
#include <find_blocked_helperfuns.h>
#include <find_blocked_optimizers.h>


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

struct node
{
	int spec_num;

	int index = -1;
	int lowlink = -1;
	bool onStack = false;
	node(int n) : spec_num(n) {}; 
};

void connect (node &n, int pos, int &index, vector<int> &stack, 
			  vector<node> &nodes, vector<vector<int>> &edges, map<int,int> &spectonode,
			  const map<int,map<int,pair<int,double>>> &nrtos,
			  map<int,bool> &expanded, 
			  const map<int,map<int,pair<int,double>>> &nstor,
			  map<int,set<int>> &STCs) 
{
	n.index = index;
	n.lowlink = index;
	index++;
	stack.push_back(pos);
	n.onStack = true;

	// Expand the surrounding reactions
	for(const auto &i:nstor.at(n.spec_num))
	{
		// if not already expanded into the edges and pointing out from node
		if(!expanded.at(i.first) && i.second.first!=1) 
		{
			int cur_dir = i.second.first;
			vector<int> reactants;
			vector<int> products;
			for(const auto &j:nrtos.at(i.first))
			{
				if(cur_dir%2==0)
				{
					if(j.second.first%2==0) reactants.push_back(spectonode[j.first]);
					else products.push_back(spectonode[j.first]);
				}
				else
				{
					if(j.second.first%2==1) reactants.push_back(spectonode[j.first]);
					else products.push_back(spectonode[j.first]);
				}
			}
			for(const auto &j:reactants)
			{
				for(const auto &k:products)
				{
					edges[j].push_back(k);
				}
			}
			expanded.at(i.first) = true;
		}
	}

	for(const auto &i:edges[pos])
	{
		if(nodes[i].index==-1) // if undefined
		{
			connect(nodes[i],i,index,stack,nodes,edges,spectonode,nrtos,expanded,nstor,STCs);
			n.lowlink = min(n.lowlink,nodes[i].lowlink);
		}
		else if (nodes[i].onStack)
		{
			n.lowlink = min(n.lowlink,nodes[i].index);
		}
	}

	if(n.lowlink == n.index)
	{
		int w;
		STCs[n.index];
		do
		{
			w = stack.back();
			STCs.at(n.index).insert(nodes[w].spec_num);
			nodes[w].onStack = false;
			stack.pop_back();
		} while(w!=pos);
	}
};

int find_BM (int target, set<int> &unnasr, set<int> &unnass,
			  pair<set<int>,set<int>> &BM,
			  const map<int,bool> &r_cons, 
			  const map<int,bool> &s_cons,
			  const map<int,map<int,pair<int,double>>> &rtos, 
			  const map<int,map<int,pair<int,double>>> &stor)
{
	if(!r_cons.at(target) && unnasr.find(target)!=unnasr.end()) // if not already added to a BM and dead
	{
		unnasr.erase(target);
		BM.first.insert(target);
		int count = 1;
		for(const auto &i:rtos.at(target)) // iterate over surrounding specs 
		{
			if(!s_cons.at(i.first) && unnass.find(i.first)!=unnass.end()) // if not already a part of a BM and dead 
			{
				unnass.erase(i.first);
				BM.second.insert(i.first);
				for(const auto &j:stor.at(i.first))
				{
					count+=find_BM(j.first,unnasr,unnass,BM,r_cons,s_cons,rtos,stor);
				}
			}
		}
		return count;
	}
	else return 0;
}

bool embedded(int target, set<int> &emr, const map<int,bool> &crit,
			  const map<int,map<int,pair<int,double>>> &rtos, 
			  const map<int,map<int,pair<int,double>>> &stor)
{
	bool found_all = true;
	for(const auto &i:stor.at(target))
	{
		bool found_none = true;
		bool r_o_p = rtos.at(i.first).at(target).first%2;
		for(const auto &j:rtos.at(i.first))
		{
			if(j.second.first%2!=r_o_p && !crit.at(j.first))
			{
				emr.insert(i.first);
				found_none = false; 
				break;
			}
		}
		if(found_none) found_all = false;
	}
	return found_all;
}

int find_LM (int target, set<int> &unnasr, set<int> &unnass,
			  pair<set<int>,set<int>> &BM,
			  const map<int,bool> &r_cons, 
			  const map<int,bool> &s_cons,
			  const map<int,map<int,pair<int,double>>> &rtos, 
			  const map<int,map<int,pair<int,double>>> &stor)
{
	if(!s_cons.at(target) && unnass.find(target)!=unnass.end()) // if not already added to a BM and dead
	{
		unnass.erase(target);
		BM.second.insert(target);
		int count = 1;
		for(const auto &i:stor.at(target)) // iterate over surrounding reacs 
		{
			if(!r_cons.at(i.first) && unnasr.find(i.first)!=unnasr.end()) // if not already a part of a BM and dead 
			{
				unnasr.erase(i.first);
				BM.first.insert(i.first);
				for(const auto &j:rtos.at(i.first))
				{
					count+=find_LM(j.first,unnasr,unnass,BM,r_cons,s_cons,rtos,stor);
				}
			}
		}
		return count;
	}
	else return 0;
}

map<int,map<int,pair<int,double>>> PCC::stor; 
map<int,map<int,pair<int,double>>> PCC::rtos;
int PCC::REV = 0;
int PCC::SOU = 0;
int PCC::STO = 0;

void ETracer::update_blocked()
{
	lm_count = 0;
	bm_count = 0;

	int sumreac = model->reactions.size();
	int sumspec = model->species.size();
	int sumtot = sumreac + sumspec;

	vector<string> index_to_id;
	map<string,int> r_id_to_index;
	map<string,int> s_id_to_index;

    /// Make links from indicies in the coordinates array to names in the species and reactions maps
    for(const auto &i:model->reactions) index_to_id.push_back(i.first);
    for(const auto &i:model->species) index_to_id.push_back(i.first);

    /// Make a map from id to index
    for(int i=0;i<sumreac;i++) r_id_to_index.emplace(index_to_id[i],i);
    for(int i=sumreac;i<sumtot;i++) s_id_to_index.emplace(index_to_id[i],i-sumreac);

    /// Make links from reactions to species
    Array<int> reactants(sumreac);
    Array<int> products(sumreac);
	Array<int> links;
    int iter = 0;
    for(const auto &i:model->reactions) 
    {
		for(const auto &j:i.second.reactants) reactants[iter].push_back(s_id_to_index.at(j.first)+sumreac); 
		iter++;
    }
    iter = 0;

    for(const auto &i:model->reactions) 
    {
		for(const auto &j:i.second.products) products[iter].push_back(s_id_to_index.at(j.first)+sumreac); 
		iter++;
    }
    iter = 0;

	vector<bool> reversible;
	/// Add the reversible array
    for(const auto &i:model->reactions) reversible.push_back(i.second.reversible);

    /// Find links between the species/reactions
    for(int i=0;i<sumreac;i++)
    {
        for(const int &j:reactants[i]) links.push_back({j,i,reversible[i]});
        for(const int &j:products[i]) links.push_back({i,j,reversible[i]});
    }

	/// Find all the disconnected subgraphs in the model
	vector<int> subgraphs(sumtot,0);
	vector<vector<int> > adjacency(sumtot);
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
	int num_subgraphs = curr_group;

	using namespace std;

	if (sumreac > 0 && sumspec > 0)
	{
		FBA_dead.clear();
		FBA_dead.resize(sumreac);
		fill(FBA_dead.begin(), FBA_dead.end(), false);

		/// Divide the problem into smaller problems if the number of subgraphs is > 1
		struct Submat
		{
			vector<vector<double> > MAT;
			vector<int> reaclinks;
			vector<int> speclinks;
		};


		vector<int> reac_backlinks(sumreac);
		vector<int> spec_backlinks(sumspec);
		vector<Submat> subproblems(num_subgraphs);
		for (int i = 0; i < sumreac; i++) 
		{
			reac_backlinks[i] = subproblems[subgraphs[i] - 1].reaclinks.size();
			subproblems[subgraphs[i] - 1].reaclinks.push_back(i);
		}
		for (int i = sumreac; i < sumtot; i++)
		{
			spec_backlinks[i-sumreac] = subproblems[subgraphs[i] - 1].speclinks.size();
			subproblems[subgraphs[i] - 1].speclinks.push_back(i);
		}

		for (auto &s:subproblems)
		{
			s.MAT.resize(s.speclinks.size());
			for (size_t i = 0; i < s.speclinks.size(); i++) { s.MAT[i].resize(s.reaclinks.size()); fill(s.MAT[i].begin(), s.MAT[i].end(), 0.); }
			for (size_t i = 0; i < s.reaclinks.size(); i++)
			{
				for (const auto &j : model->reactions.at(index_to_id[s.reaclinks[i]]).reactants) s.MAT[spec_backlinks[s_id_to_index.at(j.first)]][i] = -j.second;
				for (const auto &j : model->reactions.at(index_to_id[s.reaclinks[i]]).products) s.MAT[spec_backlinks[s_id_to_index.at(j.first)]][i] = j.second;
			}
		}

		CoinMessageHandler::silence_output(true);

		double tolerance = 1e-13;


		bool verbose = false;
		
		double tot_opt_time = 0;
		double tot_heur_time = 0;
		double tot_erfind_time = 0;

		// Pretend every reaction is bidirectional when the BIDIR round is performed
		//if(bidir_count==1) fill(reversible.begin(),reversible.end(),true);

		int fin_dead_u = 0;
		int fin_alive_u = 0;
		int fin_dead_b = 0;
		int fin_alive_b = 0;
		int cou = 0;
		int big_cou = 0;
		size_t biggest_clust = 0;

		// Clear the LM and BM storages

		LMtype.clear();
		LMs.clear();
		BMs.clear();
		erspecs.clear();
		reacs_to_erspecs.clear();


		for (auto &s : subproblems) // For all disconnected subgraphs
		{
			if (s.reaclinks.size() == 0 || s.speclinks.size() == 0) // if either a disconnected reactions or a disconnected species
			{

				for (size_t i = 0; i < s.reaclinks.size(); i++)
				{
					FBA_dead[s.reaclinks[i]] = true;
					if(reversible[s.reaclinks[i]]) fin_dead_b++;
					else fin_dead_u++;
				}

			}
			else
			{
				auto heur_time = get_time();
				auto opt_time = get_time();
				auto er_time = get_time();

				vector<int> new_state(s.reaclinks.size(), 0);

				map<int,map<int,pair<int,double>>> stor; // links from species to reactions and their direction: reacdir 0 consuming, 1 producing, 2 - consuming bidir, 3 - producing bidir .. and stoichiometry
				map<int,map<int,pair<int,double>>> rtos; // links from reactions to participating species and their relationship: 0 - reactant, 1 - product, 2 - bidir react, 3 - bidir prod  .. and stoichiometry
				map<int,bool> rev;			// reversible map
				int ite = 0;
				for(const auto &i:s.reaclinks) { rev.emplace(ite,reversible[i]); ite++; }
				for(size_t i=0;i<s.speclinks.size();i++)
				{
					for(size_t j=0;j<s.reaclinks.size();j++)
					{
						if(s.MAT[i][j]<0.0) 
						{
							if(rev.at(j)) 
							{
								stor[i].emplace(j,pair<int,double>(2,fabs(s.MAT[i][j])));
								rtos[j].emplace(i,pair<int,double>(2,fabs(s.MAT[i][j])));
							}
							else 
							{
								stor[i].emplace(j,pair<int,double>(0,fabs(s.MAT[i][j])));
								rtos[j].emplace(i,pair<int,double>(0,fabs(s.MAT[i][j])));
							}
						}
						else if(s.MAT[i][j]>0.0) 
						{
							if(rev.at(j)) 
							{
								stor[i].emplace(j,pair<int,double>(3,fabs(s.MAT[i][j])));
								rtos[j].emplace(i,pair<int,double>(3,fabs(s.MAT[i][j])));
							}
							else 
							{
								stor[i].emplace(j,pair<int,double>(1,fabs(s.MAT[i][j])));
								rtos[j].emplace(i,pair<int,double>(1,fabs(s.MAT[i][j])));
							}
						}
					}
				}

				PCC::set_stor(stor);
				PCC::set_rtos(rtos);

				map<int,PCC> PCCs; // reaction grouped by component number
				
				int it = 0;
				for(const auto &i:rtos) 
				{ 
					int temp = i.first;
					PCCs.emplace(it,PCC(temp,rev.at(temp))); 
					it++;
				}

				set<int> final_revival; // reactions to be unconditionally revived in the end (upon them no expansion of the living can be undertaken)

				// Temporary conversions from PCC to species and back
				map<int,map<int,pair<int,double>>> nstor;
				map<int,map<int,pair<int,double>>> nrtos;

				new_strtos(PCCs,nstor,nrtos);
				set<int> alive;
				find_basic_inconsistencies(PCCs,final_revival,nrtos,nstor,verbose,alive);

				map<int,PCC::incons> incs;
				map<int,map<int,PCC::incons>> incon_reacs; 

				//printf("\nNumber of inconsistent reactions after simple reduction: %i/%i (sou: %i(%i), sto: %i(%i), rev: %i(%i))\n",num_incons_reacs,(int)rtos.size(),sou,PCC::SOU,sto,PCC::STO,reve,PCC::REV);

				int num_iterations = 0;
				int dead_b, dead_u, alive_b, alive_u, dead_prod, dead_cons;

				map<int,pair<set<int>,set<int>>> bms;
				map<int,pair<set<int>,set<int>>> spec_toLMBM; // links fom specs to BMs and the LM
				map<int,set<int>> BM_to_perspec;

				tot_heur_time += to_seconds(get_time() - heur_time);

				if(nstor.size()!=0 && nrtos.size()!=0)
				{
					opt_time = get_time();
					
					map<int,bool> fast_results_FBA = FBA_find_incons(nrtos,nstor,verbose,dead_b,dead_u,alive_b,alive_u,num_iterations,alive);

					int dead_count = 0;
					for(const auto &i:fast_results_FBA) if(!i.second) dead_count++;

					tot_opt_time += to_seconds(get_time() - opt_time);

					er_time = get_time();
					if(dead_count>0)
					{
						// Find all Blocked Modules
						// First find dead specs (iff all reactions around are dead)
						map<int,bool> spec_consistent;
						for(const auto &i:nstor)
						{
							// test if the spec has only dead neighbours
							bool all_dead = true;
							for(const auto &j:i.second) if(fast_results_FBA.at(j.first)) { all_dead = false ; break; }
							if(all_dead) spec_consistent.emplace(i.first,false);
							else spec_consistent.emplace(i.first,true);
						}

						set<int> unnas_reacs; // dead reactions to be assigned to blocked modules
						set<int> unnas_specs; // dead species to be assigned to blocked modules
						for(const auto &i:fast_results_FBA) if(!i.second) unnas_reacs.insert(i.first);
						for(const auto &i:spec_consistent) if(!i.second) unnas_specs.insert(i.first);
						// recursive function for finding BMs

						while(!unnas_reacs.empty())
						{
							bm_count--;
							bms[bm_count];
							BMs[bm_count]; // initialize but do not fill the container
							int count = find_BM(*unnas_reacs.begin(),unnas_reacs,unnas_specs,bms.at(bm_count),fast_results_FBA,spec_consistent,nrtos,nstor);
							if(verbose) printf("Found BM %i big - (r:%i,s:%i)!\n",count,(int)bms.at(bm_count).first.size(),(int)bms.at(bm_count).second.size());
						}

						// Find the external specs of these BMs
						map<int,set<int>> per_specs; 
						for(const auto &i:bms)
						{
							per_specs[i.first];
							for(const auto &j:i.second.first) // iterate over reactions
							{
								for(const auto &k:nrtos.at(j)) // iterate over specs
								{
									if(spec_consistent.at(k.first)) // if not a dead spec
									{
										per_specs.at(i.first).insert(k.first);
									}
								}
							}
						}

						if(verbose) printf("\nThe numbers of peripheral species in each BM are:\n");
						if(verbose) for(const auto &i:per_specs) printf("%i\n",(int)i.second.size());

						// Remove the blocked reactions, giving them the order numbers of the BMs 
						for(const auto &i:bms)
						{
							for(const auto &j:i.second.first)
							{
								PCCs.at(j).addIncon(i.first,PCC::sto);
							}
						}
						new_strtos(PCCs,nstor,nrtos);

						// Test the cleared network with sinks and sources
						int numcd = 0;
						int numpd = 0;
						map<int,bool> CONS;
						map<int,bool> PROD;

						PROD = SINKS_find_incons(nrtos,nstor,1.0,numpd);
						CONS = SINKS_find_incons(nrtos,nstor,-1.0,numcd);



						map<int,bool> only_cons;
						map<int,bool> only_prod;
						map<int,bool> prod_cons;
						for(const auto &i:nstor)
						{
							only_cons.emplace(i.first,true); 
							only_prod.emplace(i.first,true);
							prod_cons.emplace(i.first,true);
							int co = !CONS.at(i.first);
							int pr = !PROD.at(i.first);
							if(co && !pr) only_cons.at(i.first)=false;
							else if(pr && !co) only_prod.at(i.first)=false;
							else if(pr && co) prod_cons.at(i.first)=false;
						}

						// Find spec and reac groups according to si-so status
						set<int> sis; // 0
						set<int> sos; // 1
						set<int> sisos; // 2
						set<int> sir;
						set<int> sor;
						set<int> sisor;
						for(const auto &i:nstor)
						{
							if(!only_cons.at(i.first) && embedded(i.first,sir,only_cons,nrtos,nstor)) sis.insert(i.first);
							else if(!only_prod.at(i.first) && embedded(i.first,sor,only_prod,nrtos,nstor)) sos.insert(i.first);
							else if(!prod_cons.at(i.first) && embedded(i.first,sisor,prod_cons,nrtos,nstor)) sisos.insert(i.first);
						}
						if(verbose) printf("\nThere are %i(%i)-si, %i(%i)-so and %i(%i)-siso specs(reacs)!\n",(int)sis.size(),(int)sir.size(),
																(int)sos.size(),(int)sor.size(),(int)sisos.size(),(int)sisor.size());

						// make some reaction consistency arrays
						map<int,bool> only_consR;
						map<int,bool> only_prodR;
						map<int,bool> prod_consR;
						for(const auto &i:nrtos) only_consR.emplace(i.first,(sir.find(i.first)==sir.end()));
						for(const auto &i:nrtos) only_prodR.emplace(i.first,(sor.find(i.first)==sor.end()));
						for(const auto &i:nrtos) prod_consR.emplace(i.first,(sisor.find(i.first)==sisor.end()));

						// Divide every si-so set into living modules (LMs) 
						// The rules are that every spec in every LM must have >0 consuming and >0 producing reactions 

						int lm_count = 1;
						map<int,pair<set<int>,set<int>>> lms;
						while(!sis.empty())
						{
							lm_count++;
							lms[lm_count];
							LMtype.emplace(lm_count,0);
							int count = find_LM(*sis.begin(),sir,sis,lms.at(lm_count),only_consR,only_cons,nrtos,nstor);
							if(verbose) printf("Found LM %i big - (r:%i,s:%i) - type: si!\n",count,(int)lms.at(lm_count).first.size(),(int)lms.at(lm_count).second.size());
						}
						while(!sos.empty())
						{
							lm_count++;
							lms[lm_count];
							LMtype.emplace(lm_count,1);
							int count = find_LM(*sos.begin(),sor,sos,lms.at(lm_count),only_prodR,only_prod,nrtos,nstor);
							if(verbose) printf("Found LM %i big - (r:%i,s:%i) - type: so!\n",count,(int)lms.at(lm_count).first.size(),(int)lms.at(lm_count).second.size());
						}
						while(!sisos.empty())
						{
							lm_count++;
							lms[lm_count];
							LMtype.emplace(lm_count,2);
							int count = find_LM(*sisos.begin(),sisor,sisos,lms.at(lm_count),prod_consR,prod_cons,nrtos,nstor);
							if(verbose) printf("Found LM %i big - (r:%i,s:%i) - type: siso!\n",count,(int)lms.at(lm_count).first.size(),(int)lms.at(lm_count).second.size());
						}

						// Use PCCs to convert the LMs to actual reactions (populate LMs)
						int coo = 1;
						for(const auto &i:lms)
						{
							set<int> new_reacs;
							for(const auto &j:i.second.first) for(const auto &k:PCCs.at(j).reacs) new_reacs.insert(k.first);
							set<int> new_specs;
							//for(const auto &i:new_reacs) for(const auto &j:rtos.at(i)) new_specs.insert(j.first);
							new_specs = i.second.second;

							// Now we need to clean the new reacs from imports and exports, which are obviously not a part of the loop
							// make a temporary rtos and stor
							map<int,map<int,pair<int,double>>> temp_rtos;
							map<int,map<int,pair<int,double>>> temp_stor;
							for(const auto &j:new_reacs) temp_rtos.emplace(j,rtos.at(j));
							for(const auto &j:temp_rtos) {for(const auto &k:j.second) {temp_stor[k.first][j.first] = k.second;}}

							// clean out external connections (dead end specs)
							set<int> sdelete;
							for(const auto &i:temp_stor)
							{
								int forw = 0;
								int backw = 0;
								int bidir = 0;
								for(const auto &j:i.second) 
								{
									if(j.second.first>1) bidir++;
									else if(j.second.first%2==0) forw++;
									else backw++;
								}
								if(((forw==0 || backw==0) && bidir==0) || (forw==0 && backw==0 && bidir==1))
								{
									sdelete.insert(i.first);
									for(const auto &j:i.second) temp_rtos.at(j.first).erase(i.first);
								}
							}
							for(const auto &i:sdelete) temp_stor.erase(i);
							set<int> rdelete;
							for(const auto &i:temp_rtos) if(i.second.size()==0) rdelete.insert(i.first);
							for(const auto &i:rdelete) temp_rtos.erase(i); 

							// Clean out anything exported/imported
							set<int> alive_reacs;
							set<int> alive_specs;
							for(const auto &j:temp_rtos) 
							{
								int numrs = 0;
								int numps = 0;
								for(const auto &k:j.second) {if(k.second.first%2==0) numrs++; else numps++;}
								if(numrs==0 || numps==0) { alive_reacs.insert(j.first); alive_specs.insert(j.second.begin()->first); }
							}

							int new_hits;
							do
							{
								new_hits = 0;
								for(auto &j:temp_rtos) 
								{
									if(alive_reacs.find(j.first)==alive_reacs.end())
									{
										bool all_rs_in = true;
										bool all_ps_in = true;
										for(auto &k:j.second) 
										{
											if(k.second.first%2==0 && alive_specs.find(k.first)==alive_specs.end()) all_rs_in = false;
											if(k.second.first%2==1 && alive_specs.find(k.first)==alive_specs.end()) all_ps_in = false;
										}
										if(all_rs_in || all_ps_in) 
										{
											alive_reacs.insert(j.first);
											for(auto &k:j.second) alive_specs.insert(k.first);
											new_hits++;
										} 
									}
								}

							} while(new_hits>0);

							for(const auto &j:alive_reacs) for(const auto &k:temp_rtos.at(j)) temp_stor.at(k.first).erase(j);
							for(const auto &j:alive_specs) for(const auto &k:temp_stor.at(j)) temp_rtos.at(k.first).erase(j);
							for(const auto &j:alive_reacs) temp_rtos.erase(j);
							for(const auto &j:alive_specs) temp_stor.erase(j);

							set<int> one; set<int> two;
							//printf("\nLM: one: %i two %i\n",(int)alive_reacs.size(),(int)alive_specs.size());
							for(const auto &i:temp_rtos) one.insert(s.reaclinks[i.first]);
							for(const auto &i:temp_stor) two.insert(s.speclinks[i.first]);

							LMs.emplace(i.first,make_pair(one,two));
						}

						// assign an LM to each peripheral spec (populate the erspecs and reacs_to_erspecs)

						for(const auto &i:per_specs) // find links to BMs and LMs
						{
							for(const auto &j:i.second) 
							{
								bool inserted = false;
								for(const auto &k:lms)
								{
									if(k.second.second.find(j)!=k.second.second.end())
									{
										spec_toLMBM[j].second.insert(k.first);
										inserted = true;
									}
								}
								if(inserted) spec_toLMBM[j].first.insert(i.first);
								else spec_toLMBM.erase(j);
							}
						}

						set<int> deciphered_BMs;
						for(const auto &i:spec_toLMBM) deciphered_BMs.insert(i.second.first.begin(),i.second.first.end());
						if(verbose) printf("\nNumber of BMs that have an LM: %i/%i!\n",(int)deciphered_BMs.size(),(int)bms.size());

						for(const auto &i:spec_toLMBM) 
						{
							if(verbose) printf("Spec: %i - sizes: [bm: %i, lm: %i]\n",i.first,(int)i.second.first.size(),(int)i.second.second.size());
						}

						for(const auto &i:spec_toLMBM) for(const auto &j:i.second.first) BM_to_perspec[j].insert(i.first);
						
						// Now error spreading needs to occur so that BMs, erspecs and reacs_to_erspecs can be filled
					}
					else 
					{
						// Remove the blocked reactions, giving them the order numbers of the BMs 
						for(const auto &i:fast_results_FBA) if(!i.second) PCCs.at(i.first).addIncon(-1,PCC::sto);
						new_strtos(PCCs,nstor,nrtos);
					}
					tot_erfind_time += to_seconds(get_time()-er_time);
				}

				heur_time = get_time();

				// Now transfer these inconsistencies to the original PCCs, in order to find the actually dead reactions
				incon_reacs = get_incon_reacs(PCCs,final_revival,incs);

				// First recreate the original PCCs
				it = 0;
				map<int,PCC> origPCCs;
				for(const auto &i:rtos) 
				{ 
					int temp = i.first;
					origPCCs.emplace(it,PCC(temp,rev.at(temp))); 
					it++;
				}

				// Now transfer the inconsistencies from the reduced array to the original
				for(const auto &i:incon_reacs) origPCCs.at(i.first).addIncons(i.second);
				// Now perform an inconsistency spreading (that does not fuse PCCs)
				nstor = stor;
				nrtos = rtos;
				final_revival.clear();
				spread_inconsistencies(origPCCs,final_revival,nrtos,nstor,verbose);
				// Get the inconsistent reactions 
				incon_reacs = get_incon_reacs(origPCCs,final_revival,incs);

				tot_heur_time += to_seconds(get_time() - heur_time);

				er_time = get_time();

				// If inconsistencies were found, do the error grouping stuff

				// Populate the BMs
				// first populate with reactions, then for every reaction set find the fully internal specs
				for(const auto &i:origPCCs)
				{
					//er_time = get_time();
					for(const auto &j:i.second.inc) // iterate over the inconsistencies, putting all reactions from the PCC to the respective BM
					{
						int type = j.first<0 ? 3 : j.second==PCC::rev ? 0 : j.second==PCC::sou ? 1 : 2;
						int BM_index = type==3 ? j.first : s.speclinks[j.first];
						for(const auto &k:i.second.reacs) 
						{
							int reacname = s.reaclinks[k.first];
							if(type==3)
							{
								if(BM_to_perspec.find(j.first)!=BM_to_perspec.end()) // if the reason for BM is known
								{
									for(const auto &l:BM_to_perspec.at(j.first)) 
									{
										int lm = *spec_toLMBM.at(l).second.begin();
										int spec_name = s.speclinks[l];
										erspecs[spec_name].second = 3;
										erspecs[spec_name].first.emplace(reacname,make_pair(j.first,lm));
									}
								}
							}
							else 
							{
								erspecs[BM_index].second = type;
								erspecs[BM_index].first.emplace(reacname,make_pair(BM_index,-1));
							}

							BMs[BM_index].first.insert(reacname);
							if(type!=3) 
							{
								//printf("\nReacname 1: %i\n",reacname);
								reacs_to_erspecs[reacname].emplace(BM_index,type);
							}
							else 
							{
								if(BM_to_perspec.find(j.first)!=BM_to_perspec.end()) // if the reason for BM is known
								{
									for(const auto &l:BM_to_perspec.at(j.first))
									{
										//printf("\nReacname 2: %i\n",reacname);
										reacs_to_erspecs[reacname].emplace(s.speclinks[l],type);
									} 
								}
							}
						}
					}
				}

				vector<int> scount (sumtot,0);
				for(auto &i:BMs)
				{
					for(const auto &j:i.second.first) 
					{
						for(const auto &k:neighbours[j]) 
						{
							scount[k]++;
						}
					}
					set<int> clean_specs;
					for(size_t j=sumreac;j<sumtot;j++) 
					{
						if(scount[j]>0) 
						{
							if(scount[j]>1 || neighbours[j].size()==1) clean_specs.insert(j);
							scount[j] = 0;
						}
					}
					i.second.second = clean_specs;
				}
							
				tot_erfind_time += to_seconds(get_time() - er_time);

				// count rev/irrev dead and alive
				for(const auto &i:rtos)
				{
					if(incon_reacs.find(i.first)!=incon_reacs.end()) // if inconsistent
					{
						if(rev.at(i.first)) fin_dead_b++;
						else fin_dead_u++;
					}
					else 
					{
						if(rev.at(i.first)) fin_alive_b++;
						else fin_alive_u++;
					}
				}
				cou+=num_iterations;
				if(s.reaclinks.size()>biggest_clust)
				{
					biggest_clust = s.reaclinks.size();
					big_cou = num_iterations;
				}

				// save to arrays 


				for (const auto &i:incon_reacs)
				{
					FBA_dead[s.reaclinks[i.first]] = true;
				}

			}
		}

		// Save stuff into the respective arrays and print the results

		printf(A_YEL "\n%i (uni: %i bidir: %i) reactions found blocked by the FBA ErrorTracer method in %i(%i) iterations!\n" A_RES, fin_dead_b+fin_dead_u, fin_dead_u, fin_dead_b, cou, big_cou);
		if((int)reacs_to_erspecs.size()!=(fin_dead_b+fin_dead_u))printf(A_RED "(NB! The inconsitency sources for %i of these reactions could not be found!)\n" A_RES,fin_dead_b+fin_dead_u-(int)reacs_to_erspecs.size());
		if(print_times) printf("Consistency checking took %f (heur: %f, opt: %f) sec, and the error finding took %f sec.\n",
			tot_heur_time+tot_opt_time, tot_heur_time, tot_opt_time, tot_erfind_time);

		FBA_dead_specs = get_dead_specs(FBA_dead,reactants,products);

		/// Store the deadness arrays inside the species / reactions in the model for later retreival
		int iter = 0;
		for (auto &i : model->species) { i.second.dead[0] = FBA_dead_specs[iter]; iter++; }
		iter = 0;
		for (auto &i : model->reactions) { i.second.dead[0] = FBA_dead[iter]; iter++; }
	}
}

vector<bool> ETracer::get_dead_specs(const vector<bool> &d_reacs,const Array<int> &reactants,const Array<int> &products)
{
    int sumreac = model->reactions.size();
    vector<bool> ret(model->species.size(),true);
    for(int i=0;i<sumreac;i++)
    {
        if(!d_reacs[i])
        {
            for(const int &j:reactants[i]) ret[j-sumreac]=false;
            for(const int &j:products[i]) ret[j-sumreac]=false;
        }
    }
    return ret;
}
