#ifndef TREE_H_INCLUDED
#define TREE_H_INCLUDED

#include<string>
#include<cstdio>
#include<iostream>
#include<cmath>
#include<vector>
#include<set>
#include<algorithm>

using namespace std;

template<class T>
using Array = vector<vector<T> >;

struct int_int
{
    int x = -1;
    int y = -1;
    int_int(int a, int b): x(a), y(b) {};
};

struct ancestor
{
    bool reac_or_mol = false;
    bool on = false;
    bool killed = true;
    bool parsed = false;
    int num = -1;
    bool root = false;
    vector<int> loop_no;
    vector<int_int> parents;
    vector<int_int> children;
};

void make_ancestry_tree(const Array<int> &parents, 
						Array<ancestor> &tree, 
						const Array<int> &reac, 
						const Array<int> &prod, 
						const vector<bool> &rever, 
						const vector<bool> &dead, 
						int start, int sumreac, int max_depth);

int find_bmodule(set<int> &blocked_module, 
				 const Array<int> &links, 
				 const vector<bool> &dead, 
				 int hit,
				 vector<bool> &tested);


#endif // TREE_H_INCLUDED
