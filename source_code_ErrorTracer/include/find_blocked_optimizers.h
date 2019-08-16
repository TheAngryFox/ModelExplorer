#ifndef FIND_BLOCKED_OPTIMIZERS_H_INCLUDED
#define FIND_BLOCKED_OPTIMIZERS_H_INCLUDED


#include<vector>
#include<map>
#include<set>
#include<string>
#include<cstdio>
#include<chrono>

#include "ClpInterior.hpp"
#include "ClpSimplex.hpp"
#include "ClpPrimalColumnSteepest.hpp"
#include "ClpDualRowDantzig.hpp"
#include "ClpDualRowSteepest.hpp"
#include "ClpPresolve.hpp"
#include "CoinTime.hpp"
#include "CoinBuild.hpp"
#include "CoinModel.hpp"

using namespace std;

map<int,bool> FBA_find_incons(const map<int,map<int,pair<int,double>>> &rtos,
					  	 	  const map<int,map<int,pair<int,double>>> &stor,
					   		  bool verbose,
							  int &dead_b, int &dead_u, int &alive_b, int &alive_u, int &num_iterations, set<int> alive = {});

map<int,bool> SINKS_find_incons(map<int,map<int,pair<int,double>>> rtos,
					  	 		map<int,map<int,pair<int,double>>> stor,
					   			int dir, int &num_dead);

std::chrono::steady_clock::time_point get_time();
double to_seconds(std::chrono::steady_clock::duration t);


#endif // FIND_BLOCKED_OPTIMIZERS_H_INCLUDED