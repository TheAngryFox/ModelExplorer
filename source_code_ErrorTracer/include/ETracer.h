#ifndef EXPLORER_H_INCLUDED
#define EXPLORER_H_INCLUDED

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

#include <boost/filesystem.hpp>

#include <FBA_model.h>
#include <fileops.h>

using namespace std;

template<class T>
using Array = vector<vector<T> >;

class ETracer
{

private:

	bool print_times = false;

	int sumreac = 0;
	int sumspec = 0;
	int sumtot = 0;

    FBA_model * model = NULL;
    vector<string> flags;           /// User defined flags 
	string odir; 					/// Output directory
	string ifname;					/// The name of the file opened by user
	string ofname; 					/// Name of the output file
    vector<string> index_to_id;     /// get species/reaction name from its index 
    map<string,int> r_id_to_index;  /// inverse of index_to_id but only for the reactions
    map<string,int> s_id_to_index;  /// inverse of index_to_id but only for the species
    Array<int> links;               /// links between nodes

	Array<int> neighbours;			/// Nearest neighbours of each node
    Array<int> reactants;           /// Reactants of each reaction
    Array<int> products;            /// Products of each reaction
    vector<bool> reversible;        /// Reaction reversible or not?
	vector<int> subgraphs;			/// Reactions and species divided into subgraphs (number indicates subgraph index)
	int num_subgraphs;

    vector<bool> FBA_dead;               /// Dead or living REACTIONS
    vector<bool> FBA_dead_specs;         /// Dead or living SPECIES 

	// Error - finding structures ////////

	map<int,int> LMtype; // the type of lm 0 - si, 1 - so, 2 - siso
	map<int,pair<set<int>,set<int>>> LMs; // LMs with their own indexing, and reac and spec ids 
	map<int,pair<set<int>,set<int>>> BMs; // Same as above but for BMs (indices are negative)

	int lm_count = 0;
	int bm_count = 0;

 	// specs with first - associated errors by querying reaction - first - BM, second - LM
	map<int,pair<map<int,pair<int,int>>,int>> erspecs; // second - type of error 0 - rev, 1 - sou, 2 - sto. If LM-based - 3
	map<int,map<int,int>> reacs_to_erspecs; // links from blocked reactions to their respective error specs 
 
	// ///////////////////////////////////

	void results_to_XML();
    int load_FBA_model(string filename);
    void update_arrays();
    void update_blocked();
    vector<bool> get_dead_specs(const vector<bool> &d_reacs,const Array<int> &reactants,const Array<int> &products);
	void find_subgraph(const int &n, const int &curr_group,const vector<vector<int> > &adjacency,vector<int> &subgraphs);
	

public:
    ETracer(string filename, const vector<string> &flags, int &result);
    ~ETracer();
};

#endif // EXPLORER_H_INCLUDED
