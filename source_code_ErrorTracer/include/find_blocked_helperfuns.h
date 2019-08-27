#ifndef FIND_BLOCKED_HELPERFUNS_H_INCLUDED
#define FIND_BLOCKED_HELPERFUNS_H_INCLUDED

#include<find_blocked_optimizers.h>

template <typename T> 
int sgn(T val) 
{
    return (T(0) < val) - (val < T(0));
}

struct PCC
{
	enum incons {rev, sou, sto}; // inconsistency types: consistent, reversibility, source, stoichiometric
	struct pinfo 
	{
		int num; // the serial number of the PCC
		int s; // the number of the species around which it is calculated
		double val; // the flux into species s from that PCC
		bool rev;   // whether the PCC is reversible around that species
		bool unidir;  // whether all fluxes point inwards or all fluxes point outwards 
		pinfo (int num, int s, double val, bool rev, bool unidir) : num(num), s(s), unidir(unidir), rev(rev), val(val) {};
	}; 
	
	map<int,incons> inc; // list of internal insonsistencies around certain species (gives the errors that need to be corrected for the PCC to work)
	bool reversible;
	map<int,double> reacs; // reaction index, flux
	map<int,set<int>> specs_to_reacs; // specs with sets of reaction links
	
	static map<int,map<int,pair<int,double>>> stor; 
	static map<int,map<int,pair<int,double>>> rtos;
	static int REV;
	static int SOU;
	static int STO;

	PCC(int r, bool rev); // construct a PCC from a reaction index and its reversibility
	static void set_stor(map<int,map<int,pair<int,double>>> &s);
	static void set_rtos(map<int,map<int,pair<int,double>>> &r);
	static map<int,map<int,pair<int,double>>> get_stor();
	static map<int,map<int,pair<int,double>>> get_rtos();
	void addIncon(int s, incons i);
	void remIncon(int s);
	void addIncons(map<int,incons> i);
	bool isConsistent() const;
	bool getFlux(int s, double &val, bool &rev) const; // get the flux value of the PCC into a species s and whether it is reversible
	static bool get_rtos_elem(int num, PCC &p, map<int,map<int,pair<int,double>>> &nrtos);
	static bool fuse(pair<PCC*,pinfo> p1, pair<PCC*,pinfo> p2, map<int,PCC> &PCCs, map<int,map<int,pair<int,double>>> &nrtos, 
													 map<int,map<int,pair<int,double>>> &nstor);
	static set<int> fuse_degenerate(map<int,PCC> &PCCs, set<int> group, int corespec, 
											map<int,map<int,pair<int,double>>> &nrtos, 
											map<int,map<int,pair<int,double>>> &nstor); // Used to fuse PCCs which are spec-degenerate and proportional, the corespec i a non-zero flux species member of all pv
	static bool add(const int s, map<int,PCC> &PCCs, map<int,map<int,pair<int,double>>> &nrtos, 
													 map<int,map<int,pair<int,double>>> &nstor,
													 set<int> &final_revival); // returns whether it is necessary to run the command on the same species again
	static bool weak_add(const int s, map<int,PCC> &PCCs, map<int,map<int,pair<int,double>>> &nrtos, 
														  map<int,map<int,pair<int,double>>> &nstor);
	void multiply_fluxes(double m);
	void set_rev(bool rev);
	void flip();
};


bool rtos_elem(int num, const PCC &p, map<int,map<int,pair<int,double>>> &nrtos);

void new_strtos(const map<int,PCC> &PCCs, map<int,map<int,pair<int,double>>> &nstor, 
										  map<int,map<int,pair<int,double>>> &nrtos);

double prop(const map<int,pair<int,double>> &a, const map<int,pair<int,double>> &b);

bool find_specsim(map<int,PCC> &PCCs, 
					map<int,map<int,pair<int,double>>> &nrtos, 
					map<int,map<int,pair<int,double>>> &nstor,
					set<int> &final_revival, bool verbose);

void find_basic_inconsistencies(map<int,PCC> &PCCs, 
								set<int> &final_revival,
								map<int,map<int,pair<int,double>>> &nrtos, 
								map<int,map<int,pair<int,double>>> &nstor,
								bool verbose, set<int> &alive);

void spread_inconsistencies(map<int,PCC> &PCCs, 
							set<int> &final_revival,
							map<int,map<int,pair<int,double>>> &nrtos, 
							map<int,map<int,pair<int,double>>> &nstor,
							bool verbose);

map<int,map<int,PCC::incons>> get_incon_reacs(const map<int,PCC> &PCCs, const set<int> &final_revival, map<int,PCC::incons> incs);

#endif // FIND_BLOCKED_HELPERFUNS_H_INCLUDED