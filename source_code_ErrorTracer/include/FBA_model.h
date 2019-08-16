#ifndef FBA_MODEL_H_INCLUDED
#define FBA_MODEL_H_INCLUDED

#include<map>
#include<set>
#include<regex>
#include<string>
#include<cstdio>
#include<iostream>
#include<fstream>
#include<pugixml.hpp>

using namespace std;

struct compartment
{
    string name;
    string outside;
	void clear() { name = ""; outside = ""; }
};

class specium
{
	friend class FBA_model;

	public:
    string name;
    string formula;
    string kegg;
    string compart;
    bool boundary_condition;

    vector<bool> dead = {false, false, false};

	void clear() { name = ""; formula = ""; kegg = ""; compart = "", boundary_condition = false; };
};

class reaction
{
	friend class FBA_model;

	public:
    string name;
    bool reversible;
    double low_bound = 0;
    double up_bound = 0;
    double obj_coeff = 0;
    double K_eq = 1;
	string kegg;

    set<string> genes; /// Gene association
    map<string,double> reactants;
    map<string,double> products;

    vector<bool> dead = {false, false, false};

	void clear() { name = ""; kegg = ""; reversible = false; low_bound = 0; up_bound - 0; obj_coeff = 0; K_eq = 1; genes.clear(); reactants.clear(); products.clear(); }
};

class FBA_model
{

public:
    map<string,compartment> compartments;
    map<string,specium> species;
    map<string,reaction> reactions;
    string model_id;
    string unit;

    int load(string file);
    int save(string file);
    FBA_model () {};
    FBA_model (string file) {load(file);}
    ~FBA_model() {}
	bool empty();
};

int replace_mul(string &str,string query,string replacement);

#endif // FBA_MODEL_H_INCLUDED
