#ifndef FBA_MODEL_H_INCLUDED
#define FBA_MODEL_H_INCLUDED

#include<map>
#include<set>
#include<regex>
#include<string>
#include<cstdio>
#include<iostream>
#include<fstream>
#include <pugixml.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

using namespace std;

struct compartment
{
    string name;
    string outside;

    template<class Archive>
    void serialize(Archive & ar,const unsigned int version)
    {
        ar & name;
        ar & outside;
    }
	void clear() { name = ""; outside = ""; }
};

class specium
{
	friend class FBA_model;

	private:
	bool cm = false;
	bool cbm = false;
    vector<double> coor = {0,0};
	vector<double> coor_back = {0,0};

	public:
    string name;
    string formula;
    string kegg;
    string compart;
    bool boundary_condition;

    vector<bool> dead = {false, false, false};

	void set_backx(const double &x) {coor_back[0]=x;};
	void set_backy(const double &y) {coor_back[1]=y;};
	void setx(const double &x) {coor[0]=x;};
	void sety(const double &y) {coor[1]=y;};
	double get_backx() const {return coor_back[0];};
	double get_backy() const {return coor_back[1];};
	double getx() const {return coor[0];};
	double gety() const {return coor[1];};
	void mask() {cm = true;};
	void unmask() {cm = false;};
	bool ismasked() const {return cm;};
	void flip() {swap(coor,coor_back); swap(cm,cbm);}

    template<class Archive>
    void serialize(Archive & ar,const unsigned int version)
    {
        ar & name;
        ar & formula;
        ar & kegg;
        ar & compart;
        ar & boundary_condition;
        ar & coor;
		ar & cm;
		ar & cbm;
		ar & coor_back;
        ar & dead;
    }
	void clear() { name = ""; formula = ""; kegg = ""; compart = "", boundary_condition = false; cm = false; cbm = false;};
};

class reaction
{
	friend class FBA_model;

	private:
	bool cm = false;
	bool cbm = false;
    vector<double> coor = {0,0};
	vector<double> coor_back = {0,0};

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

	void setx(const double &x) {coor[0]=x;};
	void sety(const double &y) {coor[1]=y;};
	void set_backx(const double &x) {coor_back[0]=x;};
	void set_backy(const double &y) {coor_back[1]=y;};
	double getx() const {return coor[0];};
	double gety() const {return coor[1];};
	double get_backx() const {return coor_back[0];};
	double get_backy() const {return coor_back[1];};
	void mask() {cm = true;};
	void unmask() {cm = false;};
	bool ismasked() const {return cm;};
	void flip() {swap(coor,coor_back); swap(cm,cbm);}

    template<class Archive>
    void serialize(Archive & ar,const unsigned int version)
    {
        ar & name;
        ar & reversible;
        ar & low_bound;
        ar & up_bound;
        ar & obj_coeff;
        ar & K_eq;
        ar & reactants;
        ar & products;
        ar & genes;
        ar & coor;
		ar & cm;
		ar & cbm;
		ar & coor_back;
        ar & dead;
    }
	void clear() { name = ""; kegg = ""; reversible = false; low_bound = 0; up_bound - 0; obj_coeff = 0; K_eq = 1; genes.clear(); reactants.clear(); products.clear(); cm = false; cbm = false;}
};

class FBA_model
{
public:
    enum search_pos {AT_END,ANYWHERE};
private:
    /// Used for saving changes after reaction/species/compartment addition or removal 
    enum action {ADDED,REMOVED,CHANGED,NAN_ACTION};
    struct Save 
    {
        int num;
        action a = NAN_ACTION;
        map<string,compartment> c;
        map<string,specium> s;
        map<string,reaction> r;
    };
    vector<Save> saves;
    int current_pos = -1; 
    /// /////////////////////////////////////////////////////////////////////////////  

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
    int purge_reactions (const vector<string> &rem_reacs, bool truncate = true, bool add_to_previous=false);
    int purge_species(const vector<string> &rem_specs, bool truncate = true, bool add_to_previous=false);
	int purge_species_weak(const vector<string> &rem_specs, bool truncate = true);
    int purge_disconnected_species(bool truncate = true);
    vector<string> find_species_by_tag(string tag, int pos);
    int purge_species_by_tag(string tag, int pos, bool truncate = true);
	bool purge_compartment(string id, bool truncate = true);
    int purge_species_by_compartment(string name, bool truncate = true);
    int purge_disconnected_reactions(bool truncate = true);
    int purge_disconnected_clusters(bool truncate = true);
	int purge_dead(int dead_type, bool truncate = true);
	bool add_species(string old_id, string id, const specium &s);
	bool add_reaction(string old_id, string id, const reaction &r);
	bool add_compartment(string old_id, string id, const compartment &c);
	int get_unmasked_rnum() { int rnum = 0; for(auto &i:reactions) if(!i.second.ismasked()) rnum++; return rnum;};
	int get_unmasked_snum() { int snum = 0; for(auto &i:species) if(!i.second.ismasked()) snum++; return snum;};
	template <class T>
	void copy_rmasked(vector<T> &dest,const vector<T> &source) 
	{
		int iterD = 0; int iterS = 0;
		for(const auto &i:reactions)
		{
			if(!i.second.ismasked())
			{
				dest[iterD]=source[iterS];
				iterD++; 
			}
			iterS++;
		}
	}
	template <class T>
	void copy_smasked(vector<T> &dest,const vector<T> &source) 
	{
		int iterD = 0; int iterS = 0;
		for(const auto &i:species)
		{
			if(!i.second.ismasked())
			{
				dest[iterD]=source[iterS];
				iterD++; 
			}
			iterS++;
		}
	}
	bool empty();
    void undo();
    void redo();
    void truncate_saves();

    template<class Archive>
    void serialize(Archive & ar,const unsigned int version)
    {
        ar & saves;
        ar & compartments;
        ar & species;
        ar & reactions;
        ar & model_id;
        ar & unit;
    }
};

int replace_mul(string &str,string query,string replacement);

#endif // FBA_MODEL_H_INCLUDED
