#include <find_blocked_helperfuns.h>

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

std::chrono::steady_clock::time_point get_time()
{
	 return std::chrono::steady_clock::now();
}

double to_seconds(std::chrono::steady_clock::duration t)
{
	return std::chrono::duration_cast<std::chrono::duration<double>>(t).count();
}

PCC::PCC(int r, bool rev) // construct a PCC from a reaction index and its reversibility
{
	if(rev) reversible = true; 
	else reversible = false;
	reacs.emplace(r,1.0);
	for(const auto &i:rtos.at(r)) specs_to_reacs[i.first].insert(r);
};

void PCC::set_stor(map<int,map<int,pair<int,double>>> &s) { stor = s; };
void PCC::set_rtos(map<int,map<int,pair<int,double>>> &r) { rtos = r; };
map<int,map<int,pair<int,double>>> PCC::get_stor() {return stor;};
map<int,map<int,pair<int,double>>> PCC::get_rtos() {return rtos;};
void PCC::addIncon(int s, incons i) { inc.emplace(s,i); };
void PCC::addIncons(map<int,incons> i) { inc.insert(i.begin(),i.end()); };
bool PCC::isConsistent() const { return (inc.empty()) ? true : false; };
bool PCC::getFlux(int s, double &val, bool &rev) const // get the flux value of the PCC into a species s and whether it is reversible
{
	rev = reversible;
	val = 0;
	bool unidir = true;
	int sign = 0;
	for(const auto &i:specs_to_reacs.at(s)) // iterate over the reactions in this PCC which are around this species
	{
		double a = reacs.at(i) * (stor.at(s).at(i).first%2==1 ? 1.0 : -1.0) * stor.at(s).at(i).second; // flux direction - positive into, negative out
		if((a>0 && sign==-1) || (a<0 && sign==1)) // if sign-inconsitent
		{
			unidir = false;
		}
		sign = (a>0) ? 1 : -1;
		val += a;
	}
	if(fabs(val)<1e-10) val = 0.0; // Try slashing down on numerical uncertainty
	return unidir;
};
bool PCC::get_rtos_elem(int num, PCC &p, map<int,map<int,pair<int,double>>> &nrtos)
{
  	bool added_elem = false;
	vector<int> storemove;
	for(const auto &i:p.specs_to_reacs) 
	{
		double val; // 0 - reactant, 1 - product, 2 - bidir react, 3 - bidir prod 
		bool rev;
		p.getFlux(i.first,val,rev);
		if(val!=0) 
		{
			nrtos[num].emplace(i.first,pair<int,double>((val>0.0 ? 1 : 0) + 2*(rev ? 1 : 0),fabs(val)));
			added_elem = true;
		}
		else storemove.push_back(i.first);
	}
	for(const auto &i:storemove) p.specs_to_reacs.erase(i);
	return added_elem;  
};
bool PCC::fuse(pair<PCC*,pinfo> p1, pair<PCC*,pinfo> p2, map<int,PCC> &PCCs, map<int,map<int,pair<int,double>>> &nrtos, 
													map<int,map<int,pair<int,double>>> &nstor)
{
	if(p1.second.rev || p2.second.rev || sgn(p1.second.val)!=sgn(p2.second.val)) // if able to feed flux into one another
	{
		// revert the PCC that can be reverted or a random if both can

		if(!p1.second.rev) 
		{
			p2.first->multiply_fluxes(-1.0*p1.second.val/p2.second.val); 
			p2.first->set_rev(false);
		}
		else
		{
			p1.first->multiply_fluxes(-1.0*p2.second.val/p1.second.val);
			if(!p2.second.rev) p1.first->set_rev(false);
		}

		// now physically add one to the other (its reactions and its inconsistencies) the second to the first
		
		PCC * a = p1.first;
		PCC * b = p2.first;
		a->reacs.insert(b->reacs.begin(),b->reacs.end());
		for(auto &i : b->specs_to_reacs) a->specs_to_reacs[i.first].insert(i.second.begin(),i.second.end());
		a->addIncons(b->inc);

		// tell to modify the original array, by deleting the PCC which was removed (remove the second reference)
		pair<int,int> fusion = make_pair(p1.second.num,p2.second.num);

		// remove the mentions of the dead PCC in all specs previously neighbouring it
		for(const auto &j:nrtos.at(fusion.second)) 
		{
			nstor.at(j.first).erase(fusion.second); 
		}
		// remove all other mentions
		PCCs.erase(fusion.second);
		nrtos.erase(fusion.second);

		// Change the info about the expanded PCC
		// First remove mentiones of it in nstor
		for(const auto &j:nrtos.at(fusion.first)) nstor.at(j.first).erase(fusion.first);
		nrtos.erase(fusion.first);
		// Now recreate its nrtos element
		bool added = get_rtos_elem(fusion.first,PCCs.at(fusion.first),nrtos);
		// Now recreate its nstor elements
		if(added) for(const auto &j:nrtos.at(fusion.first)) nstor[j.first].emplace(fusion.first,j.second); 

		return true;
	}
	else return false;
};
set<int> PCC::fuse_degenerate(map<int,PCC> &PCCs, set<int> group, int corespec, 
										map<int,map<int,pair<int,double>>> &nrtos, 
										map<int,map<int,pair<int,double>>> &nstor) // Used to fuse PCCs which are spec-degenerate and proportional, the corespec i a non-zero flux species member of all pv
{
	// assumes the degeneracy/proportionality has already been checked
	// reversibility inheritance: if the reactions point in opposite directions, or either is reversible, the result is reversible
	// flux inheritance: use the flux of the first fusant

	// find if reversible
	bool reversible = false;
	double prev_val = 0;
	for(const auto &i:group)
	{
		double val;
		bool rev;
		PCCs.at(i).getFlux(corespec,val,rev);
		if(prev_val==0) prev_val = val;
		if(sgn(val)!=sgn(prev_val) || rev) 
		{
			reversible = true; 
			break;
		}
		val = prev_val; 
	}
	PCCs.at(*group.begin()).set_rev(reversible);

	// now add the reactions from these other PCCs into the first one, bearing zero flux

	int c = 0;
	for(const auto &i:group) 
	{
		if(c!=0) for(const auto &j:PCCs.at(i).reacs) PCCs.at(*group.begin()).reacs.emplace(j.first,0.0);
		c++;
	}

	// now remove all the other pccs
	// if the overall PCC is reversible, return the set of all reactions in all involved PCCss
	// so that they could be moved into a permanent storage for later unconditional revival

	set<int> ret;
	if(reversible && group.size()>1) 
	{
		for(const auto &i:group) 
		{
			for(const auto &j:PCCs.at(i).reacs) ret.insert(j.first);
		}
	}

	// remove all PCCs except first from nrtos, and remove mentions of these from nstor
	c = 0;
	for(const int &i:group) 
	{ 
		for(const auto &j:nrtos.at(i)) nstor.at(j.first).erase(i);
		nrtos.erase(i);
		if(c!=0) 
		{
			PCCs.erase(i);
		}
		c++;
	}

	// Now recreate the rtos element of the new remaining PCC
	int e = *(group.begin());
	nrtos.erase(e);
	bool added = get_rtos_elem(e,PCCs.at(e),nrtos);
	if(added) for(const auto &i:nrtos.at(e)) nstor.at(i.first).emplace(e,i.second); 

	return ret;
};
bool PCC::add(const int s, map<int,PCC> &PCCs, map<int,map<int,pair<int,double>>> &nrtos, 
												map<int,map<int,pair<int,double>>> &nstor,
												set<int> &final_revival) // returns whether it is necessary to run the command on the same species again
{
	map<int,PCC*> p; // get the surrounding PCCs around the given species
	for(const auto &j:nstor.at(s)) p.emplace(j.first,&(PCCs.at(j.first)));

	// Check how many non-zero outputting PCCs there are around the species

	vector<pair<PCC*,pinfo>> unfrp;
	vector<pair<PCC*,pinfo>> incon_unfrp;
	for(auto &i:p)
	{
		double temp_val;
		bool temp_rev;
		bool unidir = i.second->getFlux(s,temp_val,temp_rev);
		// if the total flux of the PCC into the species is nonzero and the PCC is not dead
		if(temp_val!=0)
		{
			if(i.second->isConsistent()) unfrp.push_back(pair<PCC*,pinfo>(i.second,pinfo(i.first,s,temp_val,temp_rev,unidir))); 
			else incon_unfrp.push_back(pair<PCC*,pinfo>(i.second,pinfo(i.first,s,temp_val,temp_rev,unidir))); 
		}
	}

	if(unfrp.size()==0) // if no PCCs project flux into or out of the species
	{
		if(incon_unfrp.size()==1)
		{
			SOU++;
			incon_unfrp[0].first->addIncon(s,sou);
		}
		else if(incon_unfrp.size()==2) // if only two inconsistent PCCs, add them together into one PCC
		{
			fuse(incon_unfrp[0],incon_unfrp[1],PCCs,nrtos,nstor);
		} 
		else if(incon_unfrp.size()>2)
		{
			vector<int> forward;
			vector<int> backward;
			for(const auto &i:incon_unfrp) 
			{
				if(i.second.rev) 
				{
					forward.push_back(i.second.num);
					backward.push_back(i.second.num);
				}
				else if(i.second.val>0.0) forward.push_back(i.second.num);
				else if (i.second.val<0.0) backward.push_back(i.second.num);
			}
			for(const auto &i:forward)
			{
				for(const auto &j:backward)	
				{
					PCCs.at(i).addIncons(PCCs.at(j).inc);
					PCCs.at(j).addIncons(PCCs.at(i).inc);
				}
			}

		}
		return false;
	}
	else if(unfrp.size()==1) // a source inconsistency or a stoichiometric inconsistency depending on the case (only one PCC wants to consume or produce the spec)
	{
		double dir = unfrp[0].second.val;
		bool spread_error = false;
		for(const auto &i:incon_unfrp) 
		{
			if(i.second.rev || sgn(i.second.val)!=sgn(dir)) 
			{
				unfrp[0].first->addIncons(i.first->inc);
				spread_error = true;
			}
		}

		if(!spread_error)
		{
			if(unfrp[0].second.unidir) // if all reactions are in the same direction -> source/sink inconsistency
			{
				SOU++;
				unfrp[0].first->addIncon(s,sou);
			}
			else // if reactions in different directions -> stoichiometric inconsistency
			{
				STO++;
				unfrp[0].first->addIncon(s,sto);
			}
		}

		if(incon_unfrp.size()==1) // if only one inconsistent PCC, add them together into one PCC
		{
			fuse(unfrp[0],incon_unfrp[0],PCCs,nrtos,nstor);
		}

		return true;
	}
	else // may be either reversibility-inconsistent or consistent
	{
		// find the number of PCCs that have their fluxes fixed dir into and out of the species
		size_t plus_fixed = 0;
		size_t minus_fixed = 0;
		for(const auto &i:unfrp)
		{
			if(!i.second.rev)
			{
				if(i.second.val>0) plus_fixed++;
				else minus_fixed++;
			}
		}

		int dir = (plus_fixed==unfrp.size()) ? 1.0 : (minus_fixed==unfrp.size()) ? -1.0 : 2.0;

		if(plus_fixed==unfrp.size() || minus_fixed==unfrp.size()) // if all fluxes point in the same dir and are irreversible -> reversibility error
		{
			bool spread_error = false;
			for(const auto &i:incon_unfrp) 
			{
				if(i.second.rev || sgn(i.second.val)!=sgn(dir))
				{
					for(auto &j:unfrp)
					{
						j.first->addIncons(i.first->inc); 
					}
					spread_error = true;
				}
			}
			if(!spread_error)
			{
				REV++;
				// set every PCC around this species to be dead
				for(auto &i:unfrp) i.first->addIncon(s,rev);
			}
			return false;
		}
		if(unfrp.size()==2) // if the number of active PCCs is just 2, then a proper linkange can be established
		{
			fuse(unfrp[0],unfrp[1],PCCs,nrtos,nstor);
			return true;
		}
		else if (plus_fixed==(unfrp.size()-1) || minus_fixed==(unfrp.size()-1)) 
		{
			// If all but one reaction point into or out of the species and are irreversible, 
			// then if the remaining reaction is reversible, it should get the opposite direction and become irreversible

			// find the one reversible spec if it exists
			int rev = -1;
			double val = 0;
			for(const auto &i:unfrp) if(i.second.rev) {rev = i.second.num; val = i.second.val; break;}

			if(rev!=-1)
			{
				// if all other reactions are plus, set this one to minus, and the other way around
				if((plus_fixed==(unfrp.size()-1) && val>0.0) || (minus_fixed==(unfrp.size()-1) && val<0.0)) 
				{
					PCCs.at(rev).multiply_fluxes(-1.0);
				}
				PCCs.at(rev).set_rev(false);
			}
			// we do not return anything from this clause, because there may still be simplifiable impex reactions around this species (see below)
		}
 		else
		{
			// Now if no other case is appliccable , test if we can impex - simplify this species
			// This means we first test if there are import/export reactions around this species 
			// If there are, we simplify (or not, in certain cases) the other reactions around, and remove the import/export one

			set<int> impexes; // importing/exporting reactions around the current species 
			map<int,pair<double,bool>> others; // non-impex reactions
			for(const auto &i:unfrp) // we iterate over the living neighbouring reactions, since the dead ones cannot possibly be valid imports
			{
				map<int,pair<double,bool>> ac_specs; // active specs of the currently addressed PCC, bool true if reversible
				for(auto &j:i.first->specs_to_reacs) 
				{
					double val; bool rev;
					bool unidir = i.first->getFlux(j.first,val,rev);
					if(val!=0) ac_specs.emplace(j.first,make_pair(val,rev));
				}
				if(ac_specs.size()==1) // if import or export
				{
					impexes.insert(i.second.num);
				}
				else others.emplace(i.second.num,(*ac_specs.begin()).second);
			} 

			// fuse the impexes together 
			if(!impexes.empty())
			{
				set<int> revived = fuse_degenerate(PCCs,impexes,s,nrtos,nstor);
				final_revival.insert(revived.begin(),revived.end());

				// Now consider whether the resulting impex 
				int imp = *impexes.begin();
				double val; bool rev;
				bool unidir = PCCs.at(imp).getFlux(s,val,rev);

				bool all_opposite = true;
				for(const auto &i:others) if(i.second.second || sgn(val/i.second.first)!=-1) all_opposite = false;

				if(all_opposite) // if reversible, unconditionally reduce this PCC, otherwise check if all are opposite
				{

					// Destroy the current species in nstor and all mentions of it in nrtos and in specs_to_reacs of all PCCs
					for(const auto &i:nstor.at(s))
					{
						nrtos.at(i.first).erase(s);
						PCCs.at(i.first).specs_to_reacs.erase(s);
					}
					nstor.erase(s);

					// Destroy the reactions from imp 
					// Destroy the impex in PCCs and nrtos and all mentions thereof in nstor

					for(const auto &i:nrtos.at(imp)) nstor.at(i.first).erase(imp);
					PCCs.erase(imp);
					nrtos.erase(imp);
				}
			}
		}
	}
	return true;
};
bool PCC::weak_add(const int s, map<int,PCC> &PCCs, map<int,map<int,pair<int,double>>> &nrtos, 
													map<int,map<int,pair<int,double>>> &nstor)
{
	map<int,PCC*> p; // get the surrounding PCCs around the given species
	for(const auto &j:nstor.at(s)) p.emplace(j.first,&(PCCs.at(j.first)));

	// Check how many non-zero outputting PCCs there are around the species

	vector<pair<PCC*,pinfo>> unfrp;
	vector<pair<PCC*,pinfo>> incon_unfrp;
	for(auto &i:p)
	{
		double temp_val;
		bool temp_rev;
		bool unidir = i.second->getFlux(s,temp_val,temp_rev);
		// if the total flux of the PCC into the species is nonzero and the PCC is not dead
		if(temp_val!=0)
		{
			if(i.second->isConsistent()) unfrp.push_back(pair<PCC*,pinfo>(i.second,pinfo(i.first,s,temp_val,temp_rev,unidir))); 
			else incon_unfrp.push_back(pair<PCC*,pinfo>(i.second,pinfo(i.first,s,temp_val,temp_rev,unidir))); 
		}
	}

	if(unfrp.size()==0) // if no PCCs project flux into or out of the species
	{
		if(incon_unfrp.size()==1)
		{
			SOU++;
			incon_unfrp[0].first->addIncon(s,sou);
		}
		return false;
	}
	else if(unfrp.size()==1) // a source inconsistency or a stoichiometric inconsistency depending on the case (only one PCC wants to consume or produce the spec)
	{
		double dir = unfrp[0].second.val;
		bool spread_error = false;
		for(const auto &i:incon_unfrp) 
		{
			if(i.second.rev || sgn(i.second.val)!=sgn(dir)) 
			{
				unfrp[0].first->addIncons(i.first->inc);
				spread_error = true;
			}
		}

		if(!spread_error)
		{
			if(unfrp[0].second.unidir) // if all reactions are in the same direction -> source/sink inconsistency
			{
				SOU++;
				unfrp[0].first->addIncon(s,sou);
			}
			else // if reactions in different directions -> stoichiometric inconsistency
			{
				STO++;
				unfrp[0].first->addIncon(s,sto);
			}
		}

		if(incon_unfrp.size()==1) // if only one inconsistent PCC, add them together into one PCC
		{
			fuse(unfrp[0],incon_unfrp[0],PCCs,nrtos,nstor);
		}

		return true;
	}
	else // may be either reversibility-inconsistent or consistent
	{
		// find the number of PCCs that have their fluxes fixed dir into and out of the species
		size_t plus_fixed = 0;
		size_t minus_fixed = 0;
		for(const auto &i:unfrp)
		{
			if(!i.second.rev)
			{
				if(i.second.val>0) plus_fixed++;
				else minus_fixed++;
			}
		}

		int dir = (plus_fixed==unfrp.size()) ? 1.0 : (minus_fixed==unfrp.size()) ? -1.0 : 2.0;

		if(plus_fixed==unfrp.size() || minus_fixed==unfrp.size()) // if all fluxes point in the same dir and are irreversible -> reversibility error
		{
			bool spread_error = false;
			for(const auto &i:incon_unfrp) 
			{
				if(i.second.rev || sgn(i.second.val)!=sgn(dir))
				{
					for(auto &j:unfrp)
					{
						j.first->addIncons(i.first->inc); 
					}
					spread_error = true;
				}
			}
			if(!spread_error)
			{
				REV++;
				// set every PCC around this species to be dead
				for(auto &i:unfrp) i.first->addIncon(s,rev);
			}
			return false;
		}
		if(unfrp.size()==2) // if the number of active PCCs is just 2, then a proper linkange can be established
		{
			fuse(unfrp[0],unfrp[1],PCCs,nrtos,nstor);
			return true;
		}
		else if (plus_fixed==(unfrp.size()-1) || minus_fixed==(unfrp.size()-1)) 
		{
			// If all but one reaction point into or out of the species and are irreversible, 
			// then if the remaining reaction is reversible, it should get the opposite direction and become irreversible

			// find the one reversible reac if it exists
			int rev = -1;
			double val = 0;
			for(const auto &i:unfrp) if(i.second.rev) {rev = i.second.num; val = i.second.val; break;}

			if(rev!=-1)
			{
				// if all other reactions are plus, set this one to minus, and the other way around
				if((plus_fixed==(unfrp.size()-1) && val>0.0) || (minus_fixed==(unfrp.size()-1) && val<0.0)) 
				{
					PCCs.at(rev).multiply_fluxes(-1.0);
				}
				PCCs.at(rev).set_rev(false);
			}
		}
	}

	return true;
};
void PCC::multiply_fluxes(double m) { for(auto &i:reacs) i.second*=m; };
void PCC::set_rev(bool rev) { reversible = rev; };
void PCC::flip() { multiply_fluxes(-1.0); };

bool rtos_elem(int num, const PCC &p, map<int,map<int,pair<int,double>>> &nrtos)
{
	bool added_elem = false;
	for(const auto &i:p.specs_to_reacs) 
	{
		double val; // 0 - reactant, 1 - product, 2 - bidir react, 3 - bidir prod 
		bool rev;
		p.getFlux(i.first,val,rev);
		if(val!=0) 
		{
			nrtos[num].emplace(i.first,pair<int,double>((val>0.0 ? 1 : 0) + 2*(rev ? 1 : 0),fabs(val)));
			added_elem = true;
		}
	}
	return added_elem;
}

void new_strtos(const map<int,PCC> &PCCs, map<int,map<int,pair<int,double>>> &nstor, 
										  map<int,map<int,pair<int,double>>> &nrtos)
{
    // Convert the living PCCs to new stor / rtos, check how many reactions and species are left
	nstor.clear();
	nrtos.clear();

	for(const auto &i:PCCs) 
	{
		if(i.second.isConsistent()) 
		{
			rtos_elem(i.first,i.second,nrtos);
		}
	}

	for(const auto &i:nrtos)
	{
		for(const auto &j:i.second)
		{
			nstor[j.first][i.first] = j.second;
		}
	}
}

double prop(const map<int,pair<int,double>> &a, const map<int,pair<int,double>> &b)
{
	double prev_rat = 0;
	for(auto ita = a.begin(), itb = b.begin(); ita!=a.end() && itb!=b.end(); ita++, itb++)
	{
		double temp = (ita->second.second / itb->second.second) 
						* ((ita->second.first%2==0) ? -1.0 : 1.0) 
						* ((itb->second.first%2==0) ? -1.0 : 1.0);
		if(ita!=a.begin() && temp!=prev_rat) return 0.0;
		prev_rat = temp;
	}
	return prev_rat;
}

bool find_specsim(map<int,PCC> &PCCs, 
					map<int,map<int,pair<int,double>>> &nrtos, 
					map<int,map<int,pair<int,double>>> &nstor,
					set<int> &final_revival, bool verbose)
{
	// Now test for the existence of spec-similar reactions (species the same, but stoichiometry may differ)

	map<set<int>,set<int>> spec_sim; // set of participating specs, set of reacs with this set of participating specs 
	for(const auto &i:nrtos)
	{
		set<int> temp; 
		for(const auto &j:i.second) temp.insert(j.first);
		spec_sim[temp].insert(i.first);
	} 

	if(verbose)
	{
		map<int,int> num_ntuples; // size of ntuple, number of reactions in the ntuple
		for(const auto &i:spec_sim) 
		{
			if(num_ntuples.find(i.second.size())==num_ntuples.end()) num_ntuples.emplace(i.second.size(),1);
			else num_ntuples.at(i.second.size())++;
		}
		printf("\nNumber of spec-diffirent reactions: %i \n",(int)spec_sim.size());
		for(const auto &i:num_ntuples) printf("%i - %i\n",i.first,i.second);
	}

	// Now test whether the reaction groups are multiples of each other

	// First remove every 1st order ntuple (reactions that do not have duplicates)
	map<set<int>,set<int>> temp_spec_sim; 
	for(const auto &i:spec_sim) if(i.second.size()>1) temp_spec_sim.insert(i);
	spec_sim = temp_spec_sim;

	if(verbose) 
	{
		printf("Number of spec-similar reaction groups: %i\n",(int)spec_sim.size());
		// Show a historgram of how many species there are in each reaction
		map<int,int> spec_hist;
		for(const auto &i:spec_sim)
		{
			if(spec_hist.find(i.first.size())==spec_hist.end()) spec_hist.emplace(i.first.size(),1);
			else spec_hist.at(i.first.size())++;
		}
		printf("\nHistogram of spec num in spec-similar reaction groups:\n");
		for(const auto &i:spec_hist) printf("%i - %i\n",i.first,i.second);
	}

	// Now test every other ntuple
	int m_self = 0;
	int m_self_r = 0;
	int m_not_self = 0;
	int m_not_self_r = 0;
	int not_m = 0;
	int solitary = 0;
	int non_solitary = 0;
	vector<vector<pair<set<int>,bool>>> propgroups; // storage for proportionality groups
	for(const auto &i:spec_sim) // iterate over the groups
	{
		vector<pair<set<int>,bool>> propg; // proportionality groups (bool is true if the group is self-feeding)
		vector<int> stack; // the set of reaction indicies in the current group
		for(const auto &j:i.second) stack.push_back(j);
		while(!stack.empty()) // test the first item in stack against every other item, adding to the last meember of propg if proportional
		{ 
			vector<int> reststack;
			propg.resize(propg.size()+1);
			propg.back().first.insert(stack[0]);
			propg.back().second = (nrtos.at(stack[0]).begin()->second.first>1) ? true : false;
			for(size_t j = 1; j<stack.size(); j++)
			{
				double p = prop(nrtos.at(stack[0]),nrtos.at(stack[j]));
				if(p!=0) // if proportional add to propg
				{
					if(p<0 || nrtos.at(stack[j]).begin()->second.first>1) propg.back().second = true; // if at least one proportionality is negative or at least one reaction is reversible, the group is self-feeding
					propg.back().first.insert(stack[j]);
				} 
				else reststack.push_back(stack[j]); // if unproportional add to living list
			}
			stack = reststack;
		}
		if(propg.size()==1)
		{
			if(propg[0].second) { m_self++; m_self_r+=propg[0].first.size(); }
			else { m_not_self++; m_not_self_r+=propg[0].first.size(); }
		}
		else 
		{
			not_m++;
			for(const auto &i:propg)
			{
				if(i.first.size()==1) solitary++;
				else non_solitary+=i.first.size();
			}
		}

		// add the groups which are non-solitary to the proprgoups
		int count = 0;
		for(const auto &i:propg) if(i.first.size()>1) count++;
		if(count>0)
		{
			propgroups.resize(propgroups.size()+1);
			for(const auto &i:propg) if(i.first.size()>1) propgroups.back().push_back(i);
		}
	}

	if(verbose)
	{
		printf("\nSpec-similar reaction groups can be split into:"
			"\n- monolithic self-feeding: %i (reacs: %i)"
			"\n- monolithic unidirectional: %i (reacs: %i)"  
			"\n- non-monolithic: %i (of which %i reacs solitary and %i non-solitary)"
			"\nTotal group number: %i\n",m_self,m_self_r,m_not_self,m_not_self_r,not_m,solitary,non_solitary,m_self+m_not_self+not_m);
	}

	// Fuse the multiples, both the monolithic and the non-monolithic if present
	// If negative multiple, save these reactions as active for later reactivation

	// Reactions to be revived in the end due to self-feeding behaviour
	for(const auto &i:propgroups)
	{
		// fuse. save if self-feeding
		for(const auto &j:i) 
		{
			// clean the PCC
			set<int> revived = PCC::fuse_degenerate(PCCs,j.first,nrtos.at(*j.first.begin()).begin()->first,nrtos,nstor);
			// save reactions to revival set if self-feeding
			final_revival.insert(revived.begin(),revived.end());
		}
	}

	if(verbose)
	{
		printf("\nThe number of PCCs after degerate removal is: %i\n",(int)PCCs.size());
		printf("\nNumber of reactions to be revived is: %i\n",(int)final_revival.size());
	}

	return propgroups.empty() ? false : true; 
}

void find_basic_inconsistencies(map<int,PCC> &PCCs, 
								set<int> &final_revival,
								map<int,map<int,pair<int,double>>> &nrtos, 
								map<int,map<int,pair<int,double>>> &nstor,
								bool verbose, set<int> &alive)
{
	alive.clear();

	new_strtos(PCCs,nstor,nrtos);

	bool extended_engulfed = false;
	int prev_irrev = -1;

	set<int> iterable;
	for(const auto &i:nstor) iterable.insert(i.first);

	auto first_time = get_time();

	do
	{
		extended_engulfed = false;

		// fuse duplicates together
		if(find_specsim(PCCs,nrtos,nstor,final_revival,false)) extended_engulfed=true;

		size_t prevPCCsize = PCCs.size();

		// perform PCC fusion, dead expansion and import/export simplification
		set<int> to_remove;
		for(auto &i:iterable)
		{
			// If changes are no longer possible around this spec, remove it from iterable list
			if(!PCC::add(i,PCCs,nrtos,nstor,final_revival)) to_remove.insert(i);
		}

		if(PCCs.size()!=prevPCCsize || to_remove.size()!=0) extended_engulfed = true;

		// if the number any of the PCCs have changed reversibility, continue looping 
		int new_irrev = 0;
		for(const auto &j:PCCs) if (!j.second.reversible) new_irrev++;
		if(new_irrev!=prev_irrev)
		{
			prev_irrev = new_irrev;
			extended_engulfed = true;
		}

		// now clean the nstor off empty elements, and do the same with iterable elements

		for(const auto &i:iterable) if(nstor.find(i)==nstor.end()) to_remove.insert(i);
		for(const auto &i:to_remove) iterable.erase(i);

	} while(extended_engulfed);

	new_strtos(PCCs,nstor,nrtos);

	if(verbose) printf(A_GRE "\nIt took %f seconds to find basic inconsistencies!\n" A_RES,to_seconds(get_time()-first_time));
}

void spread_inconsistencies(map<int,PCC> &PCCs, 
							set<int> &final_revival,
							map<int,map<int,pair<int,double>>> &nrtos, 
							map<int,map<int,pair<int,double>>> &nstor,
							bool verbose)
{

	bool extended_engulfed = false;
	int prev_irrev = -1;

	set<int> iterable;
	for(const auto &i:nstor) iterable.insert(i.first);

	auto first_time = get_time();

	do
	{
		extended_engulfed = false;

		size_t prevPCCsize = PCCs.size();

		// perform PCC fusion, dead expansion and import/export simplification
		set<int> to_remove;
		for(auto &i:iterable)
		{
			// If changes are no longer possible around this spec, remove it from iterable list
			if(!PCC::weak_add(i,PCCs,nrtos,nstor)) to_remove.insert(i);
		}

		if(PCCs.size()!=prevPCCsize || to_remove.size()!=0) extended_engulfed = true;

		// if the number any of the PCCs have changed reversibility, continue looping 
		int new_irrev = 0;
		for(const auto &j:PCCs) if (!j.second.reversible) new_irrev++;
		if(new_irrev!=prev_irrev)
		{
			prev_irrev = new_irrev;
			extended_engulfed = true;
		}
		
		// now clean the nstor off empty elements, and do the same with iterable elements

		for(const auto &i:iterable) if(nstor.find(i)==nstor.end()) to_remove.insert(i);
		for(const auto &i:to_remove) iterable.erase(i);

	} while(extended_engulfed);

	new_strtos(PCCs,nstor,nrtos);

	if(verbose) printf(A_GRE "\nIt took %f seconds to spread inconsistencies!\n" A_RES,to_seconds(get_time()-first_time));
}

map<int,map<int,PCC::incons>> get_incon_reacs(const map<int,PCC> &PCCs, const set<int> &final_revival, map<int,PCC::incons> incs)
{
	map<int,map<int,PCC::incons>> incon_reacs;
	for(auto &i:PCCs) 
	{
		if(!i.second.isConsistent()) 
		{
			for(const auto j:i.second.reacs) if(final_revival.find(j.first)==final_revival.end()) incon_reacs.emplace(j.first,i.second.inc);
			incs.insert(i.second.inc.begin(),i.second.inc.end());
		}
	}
	return incon_reacs;
}