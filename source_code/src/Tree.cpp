#include <Tree.h>

void make_ancestry_tree(const Array<int> &parents,
						Array<ancestor> &tree,
						const Array<int> &reac,
						const Array<int> &prod,
						const vector<bool> &rever,
						const vector<bool> &dead,
						int start, int sumreac, int max_depth)
{
    // see if new coordinate is initialized or not
    // if initialized make futile loop
    // else initialize and start a loop through the parents (create the parents, add basic info to them, add the child)
    // if cannot find any parents do nothing

    if (dead[start])
    {
        tree.clear();
        tree.resize(1);
        tree[0].resize(1);
        if(start<sumreac) tree[0][0].reac_or_mol = true;
        tree[0][0].num = start;

        size_t prev_size = 0;
        while(tree.size()>prev_size)
        {
            prev_size=tree.size();
            if((int)tree.size()<=max_depth)
            {
                int temp = 0;
                for(ancestor& i:tree[prev_size-1])
                {
                    for(int j:parents[i.num])
                    {
                        if(dead[j])
                        {
                            /// check if already member
                            int coor [2] = {-1,-1};
                            for(size_t k=0;k<tree.size();k++) for(size_t l=0;l<tree[k].size();l++) if(j==tree[k][l].num) {coor[0] = k; coor[1] = l;}

                            if(coor[0]==-1) // is not a member
                            {
                                if(tree.size()==prev_size) tree.resize(tree.size()+1);
                                tree.back().resize(tree.back().size()+1);
                                i.parents.push_back(int_int(tree.size()-1,tree.back().size()-1));
                                tree.back().back().reac_or_mol=!i.reac_or_mol;
                                tree.back().back().num=j;
                                tree.back().back().children.push_back(int_int(prev_size-1,temp));
                            }
                            else  // is a member
                            {
                                i.parents.push_back(int_int(coor[0],coor[1]));
                                tree[coor[0]][coor[1]].children.push_back(int_int(prev_size-1,temp));
                            }
                        }
                    }
                    temp++;
                }
            }
        }

        for(auto& i:tree) for(ancestor& j:i) j.killed=false;
    }
    else
    {
        tree.clear();
        tree.resize(1);
        tree[0].resize(1);
        if(start<sumreac) tree[0][0].reac_or_mol = true;
        tree[0][0].num = start;
        tree[0][0].root = true;
        bool found_whole_tree = true;
		size_t prev_size = 0;
        while(tree.size()>prev_size)
        {
            prev_size=tree.size();
            if((int)tree.size()<=max_depth)
            {
                int temp = 0;
                for(ancestor& i:tree[prev_size-1])
                {
                    for(int j:parents[i.num])
                    {
                        if(!dead[j])
                        {
                            /// check if already member
                            int coor [2] = {-1,-1};
                            for(size_t k=0;k<tree.size();k++) for(size_t l=0;l<tree[k].size();l++) if(j==tree[k][l].num) {coor[0] = k; coor[1] = l;}

                            if(coor[0]==-1) // is not a member
                            {
                                if(tree.size()==prev_size) tree.resize(tree.size()+1);
                                tree.back().resize(tree.back().size()+1);
                                i.parents.push_back(int_int(tree.size()-1,tree.back().size()-1));
                                tree.back().back().reac_or_mol=!i.reac_or_mol;
                                tree.back().back().num=j;
                                tree.back().back().children.push_back(int_int(prev_size-1,temp));
                            }
                            else  // is a member
                            {
                                i.parents.push_back(int_int(coor[0],coor[1]));
                                tree[coor[0]][coor[1]].children.push_back(int_int(prev_size-1,temp));
                            }
                        }
                    }
                    temp++;
                }
            }
            else found_whole_tree=false;
        }
        if(found_whole_tree)
        {
            /// Clean the tree
            /// Check if this is a reactions level

            /// 1) Check the upper level for living species (only if this level is reactions).
            for(auto& i:tree) for(ancestor &j:i) if(j.reac_or_mol && reac[j.num].size()==0) j.on=true;
            /// 2) See if the remaining levels have any (on) parents (all for reactions and at
            ///    least one for molecules) and delete all parent (off)-reactions for each (on) molecule.

            while(!tree[0][0].on)
            {
                /// 3) If anything is deleted like this, repeat through all levels until nothing more disappears.
                /// 4) Break if tree[0][0] is (on).
                for(auto& i:tree)
                {
                    for(ancestor& j:i)
                    {
                        if(!j.on)
                        {
                            if(j.reac_or_mol)
                            {
                                bool reversible = rever[j.num];
                                vector<int> reactants = reac[j.num];
                                vector<int> products = prod[j.num];

                                vector<int> on_parents;
                                for(int_int &k:j.parents) if(tree[k.x][k.y].on) on_parents.push_back(tree[k.x][k.y].num);

                                bool found_all_r = true;
                                bool found_all_p = true;
                                for(int &k:reactants) if(find(on_parents.begin(),on_parents.end(),k)==on_parents.end()) found_all_r=false;
                                for(int &k:products) if(find(on_parents.begin(),on_parents.end(),k)==on_parents.end()) found_all_p=false;

                                if((reversible && (found_all_p!=found_all_r)) || (!reversible && found_all_r)) j.on=true;

                                if(j.root && reversible && found_all_p && found_all_r) found_all_p = false;
                                if(reversible && found_all_p)
                                {
                                    /// Clear all the reactants out of the parents
                                    int iter = 0;
                                    while(iter<(int)j.parents.size())
                                    {
                                        int sear = tree[j.parents[iter].x][j.parents[iter].y].num;
                                        if(find(reactants.begin(),reactants.end(),sear)!=reactants.end()) j.parents.erase(j.parents.begin()+iter);
                                        else iter++;
                                    }
                                }
                                if(reversible && found_all_r)
                                {
                                    /// Clear all the products out of the parents
                                    int iter = 0;
                                    while(iter<(int)j.parents.size())
                                    {
                                        int sear = tree[j.parents[iter].x][j.parents[iter].y].num;
                                        if(find(products.begin(),products.end(),sear)!=products.end()) j.parents.erase(j.parents.begin()+iter);
                                        else iter++;
                                    }
                                }
                            }
                            else
                            {
                                int found = -1;
                                for(int k=0;k<(int)j.parents.size();k++) if(tree[j.parents[k].x][j.parents[k].y].on) {found=k; break;}
                                if(found!=-1)
                                {
                                    j.on=true;
                                    int_int only = j.parents[found];
                                    j.parents.clear();
                                    j.parents.push_back(only);
                                }
                            }
                        }
                    }
                }
            }

            tree[0][0].killed = false;

            /// Revive everything that is connected
            bool added_more = true;
            while(added_more)
            {
                added_more=false;
                for(auto& i:tree)
                {
                    for(ancestor& j:i)
                    {
                        if(!j.killed && !j.parsed)
                        {
                            j.parsed=true;
                            for(int_int &k:j.parents)
                            {
                                tree[k.x][k.y].killed=false;
                                added_more=true;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            for(auto& i:tree) for(ancestor& j:i) j.killed=false;
        }
    }
}

int find_bmodule(set<int> &blocked_module, 
				 const Array<int> &links, 
				 const vector<bool> &dead, 
				 int hit,
				 vector<bool> &tested) 
{
	if(dead[hit] && !tested[hit])
	{
		tested[hit] = true;
		blocked_module.insert(hit);
		int count = 1;
		for(const auto &i:links[hit]) count+=find_bmodule(blocked_module,links,dead,i,tested);
		return count;
		
	}
	else return 0;
}