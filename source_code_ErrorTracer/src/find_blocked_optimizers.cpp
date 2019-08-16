#include<find_blocked_optimizers.h>

void find_neighbour_power (vector<double> &rpower, 
							const map<int,int> &reac_intoor,
							const vector<int> &reac_ortoin,
							const map<int,map<int,pair<int,double>>> &stor, 
							const map<int,map<int,pair<int,double>>> &rtos, 
							int target, double power) // target reaction
{
	bool any_undefined = true;
	do // this is done in case the reaction network consists of several disconnected subnetworks
	{
		rpower[reac_intoor.at(target)] = power;
		for(const auto &i:rtos.at(target))
		{
			double stoic1 = i.second.second;
			for(const auto &j:stor.at(i.first))
			{
				double stoic2 = j.second.second;
				if(rpower[reac_intoor.at(j.first)]==-1) find_neighbour_power(rpower,reac_intoor,reac_ortoin,stor,rtos,j.first,stoic2/stoic1);
			}
		}
		any_undefined = false;
		for(size_t i=0;i<rpower.size();i++) if(rpower[i]==-1) {any_undefined=true; target = reac_ortoin[i]; break;}
	} while(any_undefined);
}

void solve (ClpSimplex &solver, CoinModel &linear_model)
{
	solver.setLogLevel(0);
	solver.loadProblem(linear_model);
	//solver.setZeroTolerance(1e-15);
	//solver.setPrimalTolerance(1e-15);
	//solver.setDualTolerance(1e-16);
	//solver.dual(0,2);
	solver.reducedGradient();
	//solver.dual();
 	//solver.barrier(false);
	//solver.dual(); 
}

vector<int> check(ClpSimplex &solver,
					const vector<int> &new_state,
					const vector<bool> reversible, 
					int &truly_alive_u, int &truly_alive_b, int &truly_dead_u)
{
	const double * solution = solver.primalColumnSolution();
	vector<int> n_state = new_state;
	for (size_t i = 0; i < n_state.size(); i++)
	{
		if (n_state[i] == 0) // check only the reactions whose fate is still unknown
		{
			if (!reversible[i]) // if unidirectional
			{
				if (solution[i] > 0.99)
				{
					truly_alive_u++;
					n_state[i] = 1;
				}
				else
				{
					truly_dead_u++;
					n_state[i] = -1;
				}
			}
			else
			{
				if (fabs(solution[i]) > 0.99) // if |x|>1, definitely alive
				{
					truly_alive_b++;
					n_state[i] = 1;
				}
			}
		}
	}
	return n_state;
}

void expand_dead (const vector<bool> &reversible, 
					const map<int,int> &reac_intoor, 
					const map<int,int> &spec_intoor, 
					const vector<int> &reac_ortoin, 
					const map<int,map<int,pair<int,double>>> &stor, 
					const map<int,map<int,pair<int,double>>> &rtos,
					vector<int> &new_state, int &truly_dead_b, bool verbose)
{
	vector<pair<int,int>> crit_specs; // species index, reaction index
	auto is_critical = [&] (map<int,pair<int,double>> i)
	{
		int num_undef_bi = 0;
		int num_dead = 0;
		int ind_undef_bi = -1;
		for(const auto &j:i) // iterate over reactions producing and consuming the species
		{
			if(reversible[reac_intoor.at(j.first)] && new_state[reac_intoor.at(j.first)]==0) // if bidirectional and unknown, add to the list
			{
				num_undef_bi++;
				ind_undef_bi = reac_intoor.at(j.first);
			}
			else if(new_state[reac_intoor.at(j.first)]==-1) num_dead++;
		}
		if(num_undef_bi==1 && (num_dead+1)==(int)i.size()) // if number of bidir uknowns is 1 and the rest are dead
		{
			return ind_undef_bi;
		}
		else return -1;
	};

	for(const auto &i:stor)
	{
		int crit_r = is_critical(i.second);
		if(crit_r!=-1) crit_specs.push_back(pair<int,int>(spec_intoor.at(i.first),crit_r));
	}
	set<int> kill_them;
	for(const auto &i:crit_specs) kill_them.insert(i.second);

	// Then expand the set until everything is gone
	while(crit_specs.size()>0)
	{
		vector<pair<int,int>> new_crits; // species index, reaction index
		kill_them.clear();
		for(const auto &i:crit_specs) kill_them.insert(i.second);
		for(const auto &i:kill_them)
		{
			new_state[i] = -1;
			truly_dead_b++;
		}
		for(const auto &i:kill_them)
		{
			for(const auto &j:rtos.at(reac_ortoin[i])) // iterate over all species involved in the critical reaction
			{
				int crit_r = is_critical(stor.at(j.first));
				if(crit_r!=-1)
				{
					new_crits.push_back(pair<int,int>(spec_intoor.at(j.first),crit_r));
				}
			}
		}
		crit_specs = new_crits;
	}
	if(verbose) printf("\nThe number of dead bidirectionals is now: %i\n",truly_dead_b);
};

void retighten (CoinModel &linear_model,vector<int> &new_state,
				const map<int,map<int,pair<int,double>>> &stor, 
				const map<int,map<int,pair<int,double>>> &rtos) // Retighten auxiliaries around all unknown reactions
{
	for (size_t i = 0; i < rtos.size(); i++) 
	{
		if(new_state[i]==0) // undetermined
		{
			linear_model.setRowLower(i+stor.size(),0.0);
			linear_model.setColObjective(rtos.size()+i,1.0);
		}
		else // determined
		{
			linear_model.setRowLower(i+stor.size(),-COIN_DBL_MAX);
			linear_model.setColObjective(rtos.size()+i,0.0);
		}
	}
};

int retighten_powerful (CoinModel &linear_model, vector<int> &new_state, 
						const vector<double> rpower,
						const map<int,map<int,pair<int,double>>> &stor, 
						const map<int,map<int,pair<int,double>>> &rtos) // Retighten auxiliaries around the most powerful reaction
{
	int highest_pow_in = -1;
	double highest_power = -1.0;
	for(size_t i=0;i<new_state.size();i++) 
	{
		if(new_state[i]==0 && rpower[i]>highest_power)
		{
			highest_power = rpower[i];
			highest_pow_in = i;
		}
	}
	if(highest_pow_in!=-1)
	{
		for (size_t i = 0; i < rtos.size(); i++) 
		{
			if(i==highest_pow_in) // undetermined
			{
				linear_model.setRowLower(i+stor.size(),0.0);
				linear_model.setColObjective(rtos.size()+i,1.0);
			}
			else // determined
			{
				linear_model.setRowLower(i+stor.size(),-COIN_DBL_MAX);
				linear_model.setColObjective(rtos.size()+i,0.0);
			}
		}
	}
	return highest_pow_in;
};

void flip_reacs (vector<int> &flippable, 
				const map<int,map<int,pair<int,double>>> &rtos, 
				const vector<int> &reac_ortoin, 
				const map<int,int> &spec_intoor, CoinModel &linear_model)
{
	for(const int &i:flippable)
	{
		for(const auto &j:rtos.at(reac_ortoin[i]))
		{
			linear_model.setElement(spec_intoor.at(j.first),i,-linear_model.getElement(spec_intoor.at(j.first),i));
		}
	}
};


map<int,bool> FBA_find_incons(const map<int,map<int,pair<int,double>>> &rtos,
					  	 		const map<int,map<int,pair<int,double>>> &stor,
					   			bool verbose,
								int &dead_b, int &dead_u, int &alive_b, int &alive_u, int &num_iterations, set<int> alive_default)
{
	vector<bool> reversible;
	for(const auto &i:rtos) reversible.push_back(i.second.begin()->second.first>1 ? true : false);

	map<int,int> spec_intoor; // conversion from spec index to order number in stor
	map<int,int> reac_intoor; // conversion from reac index to order number in rtos
	for(const auto &i:stor) spec_intoor.emplace(i.first,spec_intoor.size());
	for(const auto &i:rtos) reac_intoor.emplace(i.first,reac_intoor.size());

	vector<int> reac_ortoin;
	vector<int> spec_ortoin;
	for(const auto &i:rtos) reac_ortoin.push_back(i.first);
	for(const auto &i:stor) spec_ortoin.push_back(i.first);

	int truly_dead_u = 0;
	int truly_alive_u = 0;
	int truly_dead_b = 0;
	int truly_alive_b = 0;

	// initialize the vector of reaction consistency 
	vector<int> new_state(rtos.size(),0);
	for(const auto &i:alive_default) new_state[reac_intoor.at(i)] = 1;
	for(const auto &i:alive_default) 
	{
		if(reversible[reac_intoor.at(i)]) truly_alive_b++;
		else truly_alive_u++;
	}

	// Make an array of reaction powers based on relative stoichiometries (the most powerful needs the lowert relative flux)
	vector<double> rpower (rtos.size(),-1); // An array of reaction powers
	find_neighbour_power(rpower,reac_intoor,reac_ortoin,stor,rtos,rtos.begin()->first,1.0);

	ClpSimplex solver;

	CoinModel linear_model;
	ClpModel intermediate;
	linear_model.setOptimizationDirection(-1.);

	int place = 0;

	// Initialize species
	for (size_t i = 0; i < stor.size(); i++) linear_model.setRowBounds(i, 0.0, 0.0);

	// Initialize auxiliary constraints
	for (size_t i = stor.size(); i < (stor.size() + rtos.size()); i++) 
		linear_model.setRowBounds(i,(reversible[i-stor.size()]) ? -COIN_DBL_MAX : 0.0, COIN_DBL_MAX);

	// Initialize reactions
	int it = 0;
	for (const auto &i:rtos)
	{
		vector<int> columnIndex;
		vector<double> columnValue;
		for(const auto &j:i.second)
		{
			columnIndex.push_back(spec_intoor.at(j.first));
			columnValue.push_back(j.second.second * (j.second.first%2==0 ? -1.0 : 1.0));
		}
		// Auxiliary constraint
		columnIndex.push_back(stor.size() + it);
		columnValue.push_back(1.0);
		linear_model.addColumn((int)columnIndex.size(), &columnIndex[0], &columnValue[0], (reversible[it]) ? -COIN_DBL_MAX : 0.0, COIN_DBL_MAX, 0.0);
		it++;
	}

	/// Initialize auxiliary reactions
	for (size_t i = 0; i < rtos.size(); i++)
	{
		vector<int> columnIndex;
		vector<double> columnValue;
		columnIndex.push_back(stor.size() + i);
		columnValue.push_back(-1.0);
		// set objective to 1 only if irreversible (for the first round)
		linear_model.addColumn((int)columnIndex.size(), &columnIndex[0], &columnValue[0], 0.0, 1.0, reversible[i] ? 0.0 : 1.0); 
	}

	// Add reactions and optimize
	solve(solver,linear_model);

	/// After this optimization we decisively know which unidirectional reactions are dead and which are alive
	/// Check how many of the monodirectional reactions are dead and alive

	int num_uni = 0, num_bi = 0;
	for(const auto &i:reversible) 
	{
		if(i) num_bi++;
		else num_uni++;
	}
	new_state = check(solver,new_state,reversible,truly_alive_u,truly_alive_b,truly_dead_u);
	if(verbose)
	{
		printf("\nUnidirectional rs DEAD after 1st round: %i / %i\n",truly_dead_u,(int)rtos.size());
		printf("Unidirectional rs ALIVE after 1st round: %i / %i\n",truly_alive_u,(int)rtos.size());
		printf("Unidirectional rs UNDEF after 1st round: %i / %i\n",num_uni - truly_dead_u - truly_alive_u,(int)rtos.size());
		printf("Bidirectional rs DEAD after 1st round: %i / %i\n",truly_dead_b,(int)rtos.size());
		printf("Bidirectional rs ALIVE after 1st round: %i / %i\n",truly_alive_b,(int)rtos.size());
		printf("Bidirectional rs UNDEF after 1st round: %i / %i\n",num_bi - truly_dead_b - truly_alive_b,(int)rtos.size());
	}

	// Expand the dead 
	// Core rule: if all reactions are dead around a metabolite except one bidirectional reaction, then that one is also dead
	// Before we go, find obvious dead (bidirectional reactions that have a reactant or product only connected to that reaction)

	int obvious_bi_dead = 0;
	for(const auto &i:stor) // iterate over all species that only have one producing/consimung reaction
	{
		if(i.second.size()==1 && new_state[reac_intoor.at(i.second.begin()->first)]==0) // if only one participating reaction and that reaction is undefined
		{
			new_state[reac_intoor.at(i.second.begin()->first)] = -1;
			truly_dead_b++;	
			obvious_bi_dead++;			
		}
	}
	if(verbose) printf("\nThe number of obviously dead bidirectional reactions is: %i\n",obvious_bi_dead);

	// Now expand dead
	

	expand_dead(reversible,reac_intoor,spec_intoor,reac_ortoin,stor,rtos,new_state,truly_dead_b,verbose);

	// Relax the auxiliaries on all determined reactions (all unidirectional and some bidirectional)
	// and tighten the auxiliaries on all undetermined reactions, 

	retighten(linear_model,new_state,stor,rtos);

	// Optimize
	place = 1;
	solve(solver,linear_model);

	new_state = check(solver,new_state,reversible,truly_alive_u,truly_alive_b,truly_dead_u);
	if(verbose)
	{
		printf("\nUnidirectional rs DEAD after 2nd round: %i / %i\n",truly_dead_u,(int)rtos.size());
		printf("Unidirectional rs ALIVE after 2nd round: %i / %i\n",truly_alive_u,(int)rtos.size());
		printf("Unidirectional rs UNDEF after 2nd round: %i / %i\n",num_uni - truly_dead_u - truly_alive_u,(int)rtos.size());
		printf("Bidirectional rs DEAD after 2nd round: %i / %i\n",truly_dead_b,(int)rtos.size());
		printf("Bidirectional rs ALIVE after 2nd round: %i / %i\n",truly_alive_b,(int)rtos.size());
		printf("Bidirectional rs UNDEF after 2nd round: %i / %i\n",num_bi - truly_dead_b - truly_alive_b,(int)rtos.size());
	}
	// Retighten the auxiliaries around the unknown

	retighten(linear_model,new_state,stor,rtos);

	// Now flip all the unknown bidirectional reactions and optimize again

	vector<int> fl;
	for(size_t i=0;i<new_state.size();i++) if(new_state[i]==0) fl.push_back(i);
	flip_reacs(fl,rtos,reac_ortoin,spec_intoor,linear_model);

	// Optimize
	place = 3;
	solve(solver,linear_model);

	new_state = check(solver,new_state,reversible,truly_alive_u,truly_alive_b,truly_dead_u);
	if(verbose)
	{
		printf("\nUnidirectional rs DEAD after 3rd round: %i / %i\n",truly_dead_u,(int)rtos.size());
		printf("Unidirectional rs ALIVE after 3rd round: %i / %i\n",truly_alive_u,(int)rtos.size());
		printf("Unidirectional rs UNDEF after 3rd round: %i / %i\n",num_uni - truly_dead_u - truly_alive_u,(int)rtos.size());
		printf("Bidirectional rs DEAD after 3rd round: %i / %i\n",truly_dead_b,(int)rtos.size());
		printf("Bidirectional rs ALIVE after 3rd round: %i / %i\n",truly_alive_b,(int)rtos.size());
		printf("Bidirectional rs UNDEF after 3rd round: %i / %i\n",num_bi - truly_dead_b - truly_alive_b,(int)rtos.size());
	}

	// Now flip them one by one until all are gone

	int num_extra_iter = 0;
	while((truly_dead_b+truly_alive_b+truly_dead_u+truly_alive_u)<(int)rtos.size())
	{
		num_extra_iter++;
		// retighten on the most powerful reaction
		int pindex = retighten_powerful(linear_model,new_state,rpower,stor,rtos);
		if(pindex!=-1) // if the set of undefs is not empty yet
		{
			// Optimize and check if the reaction itself is living
			place = 4;
			solve(solver,linear_model);
			new_state = check(solver,new_state,reversible,truly_alive_u,truly_alive_b,truly_dead_u);

			// If dead, reverse its direction, optimize and check
			if(new_state[pindex]==0)
			{
				vector<int> f = {pindex};
				flip_reacs(f,rtos,reac_ortoin,spec_intoor,linear_model);
				place = 5;
				solve(solver,linear_model);
				new_state = check(solver,new_state,reversible,truly_alive_u,truly_alive_b,truly_dead_u);
				// If still dead, expand the dead
				if(new_state[pindex]==0) 
				{
					new_state[pindex]=-1;
					truly_dead_b++;
					expand_dead(reversible,reac_intoor,spec_intoor,reac_ortoin,stor,rtos,new_state,truly_dead_b,verbose);
				}
			}
		}
	}

	if(verbose) 
	{
		printf("\nThe number of dead is now: %i",truly_dead_b+truly_dead_u);
		printf("\nNumber of extra iterations: %i\n",num_extra_iter);
	}

	dead_b = truly_dead_b;
	dead_u = truly_dead_u;
	alive_b = truly_alive_b;
	alive_u = truly_alive_u;

	num_iterations=3+num_extra_iter;

	map<int,bool> ret; // true means consistent
	for(size_t i=0;i<new_state.size();i++) ret.emplace(reac_ortoin[i],(new_state[i] == 1) ? true : false);

	return ret;
}


map<int,bool> SINKS_find_incons(map<int,map<int,pair<int,double>>> rtos,
					  	 		map<int,map<int,pair<int,double>>> stor,
					   			int dir, int &num_dead)
{
	dir = (dir<0) ? -1 : (dir>1) ? 1 : 0;

	if(dir==1)
	{
		for(auto &i:rtos)
		{
			for(auto &j:i.second)
			{
				if(j.second.first==1) j.second.first=0;
				else if (j.second.first==0) j.second.first=1;
				else if (j.second.first==2) j.second.first=3;
				else if (j.second.first==3) j.second.first=2;
			}
		}
		for(auto &i:stor)
		{
			for(auto &j:i.second)
			{
				if(j.second.first==1) j.second.first=0;
				else if (j.second.first==0) j.second.first=1;
				else if (j.second.first==2) j.second.first=3;
				else if (j.second.first==3) j.second.first=2;
			}
		}
	}

	vector<bool> reversible;
	for(const auto &i:rtos) reversible.push_back(i.second.begin()->second.first>1 ? true : false);

	map<int,int> spec_intoor; // conversion from spec index to order number in stor
	for(const auto &i:stor) spec_intoor.emplace(i.first,spec_intoor.size());

	vector<int> spec_ortoin;
	for(const auto &i:stor) spec_ortoin.push_back(i.first);

	ClpSimplex solver;
	CoinModel linear_model;
	linear_model.setOptimizationDirection(-1.0);

	// Initialize species
	for (size_t i = 0; i < stor.size(); i++)
	{
		linear_model.setRowBounds(i, 0.0, 0.0);
	}

	/// Initialize reactions

	int it = 0;
	for (const auto &i:rtos)
	{
		vector<int> columnIndex;
		vector<double> columnValue;
		for (const auto &j:i.second)
		{
			columnIndex.push_back(spec_intoor.at(j.first));
			columnValue.push_back(j.second.second * (j.second.first%2==0 ? -1.0 : 1.0));
		}
		linear_model.addColumn((int)columnIndex.size(), &columnIndex[0], &columnValue[0], (reversible[it]) ? -COIN_DBL_MAX : 0.0, COIN_DBL_MAX, 0.0);
		it++;
	}

	/// Initialize sinks
	for (size_t i = 0; i < stor.size(); i++)
	{
		vector<int> columnIndex;
		vector<double> columnValue;
		columnIndex.push_back(i);
		columnValue.push_back(-1.0);
		linear_model.addColumn((int)columnIndex.size(), &columnIndex[0], &columnValue[0], 0.0, 1.0, 1.0);
	}

	solver.setLogLevel(0);
	solver.loadProblem(linear_model);
	solver.barrier();

	const double * solution = solver.primalColumnSolution();
	map<int,bool> ret; // true means consistent
	for (size_t i = 0; i < stor.size(); i++)  
	{
		ret.emplace(spec_ortoin[i],(solution[i + rtos.size()] < 0.5) ? false : true);
	}

	num_dead = 0;
	for(const auto &i:ret) if(!i.second) num_dead++;

	return ret;
}