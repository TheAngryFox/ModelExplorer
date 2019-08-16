#ifndef FIND_BLOCKED_OPTIMIZERS_H_INCLUDED
#define FIND_BLOCKED_OPTIMIZERS_H_INCLUDED

#include<Explorer.h>

map<int,bool> FBA_find_incons(const map<int,map<int,pair<int,double>>> &rtos,
					  	 	  const map<int,map<int,pair<int,double>>> &stor,
					   		  bool verbose,
							  int &dead_b, int &dead_u, int &alive_b, int &alive_u, int &num_iterations, set<int> alive = {});

map<int,bool> FBA_find_incons_old(const map<int,map<int,pair<int,double>>> &rtos,
					  	 		const map<int,map<int,pair<int,double>>> &stor,
					   			bool verbose,
								int &dead_b, int &dead_u, int &alive_b, int &alive_u, int &num_iterations);

map<int,bool> SINKS_find_incons(map<int,map<int,pair<int,double>>> rtos,
					  	 		map<int,map<int,pair<int,double>>> stor,
					   			int dir, int &num_dead);



struct SINKS_obo
{
	ClpSimplex solver;
	CoinModel linear_model;

	vector<bool> reversible;
	map<int,int> spec_intoor; // conversion from spec index to order number in stor
	vector<int> spec_ortoin;
	map<int,map<int,pair<int,double>>> stor;
	map<int,map<int,pair<int,double>>> rtos;

	SINKS_obo(map<int,map<int,pair<int,double>>> nrtos, map<int,map<int,pair<int,double>>> nstor);
	bool get_sink(int num, int dir);
};
map<int,pair<bool,bool>> SINKS_one_by_one(map<int,map<int,pair<int,double>>> rtos,
					  	 				  map<int,map<int,pair<int,double>>> stor);

#endif // FIND_BLOCKED_OPTIMIZERS_H_INCLUDED