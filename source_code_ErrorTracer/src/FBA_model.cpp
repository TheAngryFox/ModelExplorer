#include <FBA_model.h>

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

bool FBA_model::empty()
{
	if ((species.size() + reactions.size()) == 0) return true;
	else return false;
}

int FBA_model::load(string file)
{
    string model_text;

    /// Load file
    ifstream model_file (file,ios::in);
    if(model_file.fail()) 
	{
		printf(A_RED "\n\n FBA_model::load: Could not load file with the given name!\n\n" A_RES);
		return 1;
	}
    model_file.seekg(0, ios::end);
    model_text.reserve(model_file.tellg());
    model_file.seekg(0, ios::beg);
    model_text.assign((istreambuf_iterator<char>(model_file)), istreambuf_iterator<char>());
    model_file.close();

    /// Make its tree
    pugi::xml_document model_tree;
    pugi::xml_parse_result result = model_tree.load_string(&model_text[0]);

    if (result)
    {
        /// Is the SBML and model there or not?
    
        if(!model_tree.child("sbml")) {printf(A_RED "\n\n FBA_model::load content error. No SBML branch found in the file!!!!\n\n" A_RES); return -2;}
        if(!model_tree.child("sbml").child("model")) {printf(A_RED "\n\n FBA_model::load content error. No model branch found in the file!!!!\n\n" A_RES); return -2;}

        pugi::xml_node model = model_tree.child("sbml").child("model");

        ///Find the model id

        if(!model.attribute("id")) model_id = "nan";
        else model_id = model.attribute("id").value();

        ///Find flux unit used

        if(!model.child("listOfUnitDefinitions").child("unitDefinition").attribute("id")) unit = "nan";
        else unit = model.child("listOfUnitDefinitions").child("unitDefinition").attribute("id").value();

        ///Find compartment list

        if(model.child("listOfCompartments"))
        {
            for (pugi::xml_node c = model.child("listOfCompartments").first_child(); c; c = c.next_sibling())
            {
                if((string)c.name()=="compartment")
                {
                    compartment comp;
                    string id;
                    if(c.attribute("id")) id = c.attribute("id").value();
                    else 
                    {
                        printf("\n\n" A_RED " Error!" A_RES " In FBA_model::load could not load compartment id!\n\n");
                        return -3;
                    }
                    if(c.attribute("name")) comp.name = c.attribute("name").value();
                    else printf("\n\n Warning! In FBA_model::load could not load compartment [%s] name!\n\n",id.c_str());
                    if(c.attribute("outside")) comp.outside = c.attribute("outside").value();

                    compartments.emplace(id,comp);
                }
            }
        }

        ///Find species list

        if(model.child("listOfSpecies"))
        {
            for (pugi::xml_node c = model.child("listOfSpecies").first_child(); c; c = c.next_sibling())
            {
                if((string)c.name()=="species")
                {

                    specium s;
                    string id;

                    if(c.attribute("id")) id = c.attribute("id").value();
                    else 
                    {
                        printf("\n\n " A_RED "Error!" A_RES " In FBA_model::load could not load species id!\n\n");
                        return -3;
                    }

                    if(c.attribute("name"))
                    {
                        string uncut_name = c.attribute("name").value();

                        size_t underscore = uncut_name.find("_");
                        if(underscore!=string::npos)
                        {
                            s.formula = uncut_name.substr(underscore+1);
                            s.name = uncut_name.substr(0,underscore);
                        }
                        else s.name = uncut_name;
                    }
                    else printf("\n\n Warning! In FBA_model::load could not load species name!\n\n");

                    if(c.attribute("kegg"))
                    {
                        string temp_kegg = c.attribute("kegg").value();
                        replace_mul(temp_kegg," ","");
                        if(!temp_kegg.empty()) s.kegg = c.attribute("kegg").value();
                    }

                    if(c.attribute("compartment"))
                    {
                        bool found = false;
                        s.compart = c.attribute("compartment").value();
                        if(compartments.find(s.compart)==compartments.end()) 
                        {
                            printf("\n " A_YEL "Warning!" A_RES " In FBA_model::load species " 
                                                A_YEL "%s" A_RES " belongs to an undefined compartment " 
                                                A_YEL "%s" A_RES " ! Skipping undefined compartment. ",id.c_str(),s.compart.c_str());
                            s.compart = "";
                        } 
                    }

                    if(c.attribute("boundaryCondition")) s.boundary_condition = ((string)c.attribute("boundaryCondition").value()=="true") ? true : false;

                    species.emplace(id,s);
                }
            }
        }
        else {printf(A_RED "\n\n FBA_model::load content error. No species are found in the SBML file!!!!\n\n" A_RES); return -3;}

        /// Find reactions list

        if(model.child("listOfReactions"))
        {
            for (pugi::xml_node c = model.child("listOfReactions").first_child(); c; c = c.next_sibling())
            {
                if((string)c.name()=="reaction")
                {
                    reaction r;
                    string id;

                    if(c.attribute("id")) id = c.attribute("id").value();
                    else 
                    {
                        printf("\n\n " A_RED "Error!" A_RES " In FBA_model::load could not load reaction id!\n\n");
                        return -3;
                    }

                    if(c.attribute("name")) r.name = c.attribute("name").value();
                    else printf("\n\n Warning! In FBA_model::load could not load reaction name!\n\n");

					if (c.attribute("kegg"))
					{
						string temp_kegg = c.attribute("kegg").value();
						replace_mul(temp_kegg, " ", "");
						if (!temp_kegg.empty()) r.kegg = c.attribute("kegg").value();
					}

                    if(c.attribute("reversible")) r.reversible = ((string)c.attribute("reversible").value()=="true") ? true : false;

                    /// Load the genetic association 

                    if(c.child("notes"))
                    {
                        string assoc = "GENE_ASSOCIATION:";
                        string notes = c.child("notes").child_value();
                        if(notes.find(assoc,0)!=string::npos)
                        {
                            smatch match;
                            replace_mul(notes,assoc,"");
                            regex t("([A-Z0-9-]+)");
                            while(regex_search(notes,match,t))
                            {
                                r.genes.insert(match[0]);
                                notes=match.suffix();
                            }
                        }
                    }

                    ///Add reactants

                    if(c.child("listOfReactants"))
                    {
                        for(pugi::xml_node s = c.child("listOfReactants").first_child(); s; s = s.next_sibling())
                        {
                            if((string)s.name()=="speciesReference")
                            {
                                if(s.attribute("species"))
                                {
                                    string spec = s.attribute("species").value();
                                    double stoich;
                                    if(species.find(spec)==species.end()) // if did not find species in the species list, give out a warning and skip species
                                    {
                                        printf("\n " A_YEL "Warning!" A_RES " In FBA_model::load could not find reactant " 
                                                A_YEL "%s" A_RES " from reaction " 
                                                A_YEL "%s" A_RES " in species list. Skipping empty reactant!",spec.c_str(),id.c_str());
                                    }
                                    else
                                    {
                                        if(s.attribute("stoichiometry")) stoich = atof(s.attribute("stoichiometry").value());
                                        else 
                                        {
                                            stoich = 1.0;
                                            printf("\n " A_YEL "Warning!" A_RES " In FBA_model::load could not load reactant stoichiometry for " A_YEL "%s" A_RES "! Assumed = 1.0.",id.c_str());
                                        }
                                        r.reactants.emplace(spec,stoich);
                                    }
                                }
                                else printf("\n\n Warning! In FBA_model::load could not load reactant species!\n\n");
                            }
                        }
                    }

                    ///Add products

                    if(c.child("listOfProducts"))
                    {
                        for(pugi::xml_node s = c.child("listOfProducts").first_child(); s; s = s.next_sibling())
                        {
                            if((string)s.name()=="speciesReference")
                            {
                                if(s.attribute("species"))
                                {
                                    string spec = s.attribute("species").value();
                                    double stoich;
                                    if(species.find(spec)==species.end()) // if did not find species in the species list, give out a warning and skip species
                                    {
                                        printf("\n " A_YEL "Warning!" A_RES " In FBA_model::load could not find product " 
                                                A_YEL "%s" A_RES " from reaction " 
                                                A_YEL "%s" A_RES " in species list. Skipping empty product!",spec.c_str(),id.c_str());
                                    }
                                    else
                                    {
                                        if(s.attribute("stoichiometry")) stoich = atof(s.attribute("stoichiometry").value());
                                        else 
                                        {
                                            stoich = 1.0;
                                            printf("\n " A_YEL "Warning!" A_RES " In FBA_model::load could not load product stoichiometry for " A_YEL "%s" A_RES "! Assumed = 1.0.",id.c_str());
                                        }
                                        r.products.emplace(spec,stoich);
                                    }
                                }
                                else printf("\n\n Warning! In FBA_model::load could not load reactant species!\n\n");
                            }
                        }
                    }

                    ///Add bounds and objective coefficients

                    if(c.child("kineticLaw"))
                    {
                        if(c.child("kineticLaw").child("listOfParameters"))
                        {
                            for (pugi::xml_node p = c.child("kineticLaw").child("listOfParameters").first_child(); p; p = p.next_sibling())
                            {
                                if((string)p.name()=="parameter")
                                {
                                    string id = p.attribute("id").value();
                                    if(id=="LOWER_BOUND") r.low_bound=atof(p.attribute("value").value());
                                    else if(id=="UPPER_BOUND") r.up_bound=atof(p.attribute("value").value());
                                    else if(id=="OBJECTIVE_COEFFICIENT") r.obj_coeff=atof(p.attribute("value").value());
                                }
                            }
                        }
                    }

                    reactions.emplace(id,r);
                }
            }

        }
        else {printf(A_RED "\n\n FBA_model::load content error. No reactions are found in the SBML file!!!!\n\n" A_RES); return -4;}

        printf(A_GRE "\n " A_RES);
        for(int i=0;i<95;i++) printf(A_GRE "/" A_RES);
        printf(A_GRE "\n%-94s//\n //   " A_CYA "%-23s%-23s%-23s%s" A_GRE "   //"," //","Opened Model:","Flux Unit:","Species Number:","Reaction Number:");
        printf("\n //   " A_YEL "%-23s%-23s%-23i%-19i" A_GRE "//\n%-94s//\n " A_RES,model_id.c_str(),unit.c_str(),(int)species.size(),(int)reactions.size()," //");
        for(int i=0;i<95;i++) printf(A_GRE "/" A_RES);
        printf("\n");

    }
    else
    {
        printf(A_RED "\n\n!!!!! FBA_model::load SBML parsing error: %s !!!!!\n\n",result.description());
        if((int)model_text.length()<(result.offset+100)) model_text.resize(result.offset+100);
        printf(A_YEL "Error in: \n %s \n\n" A_RES,&model_text[result.offset]);
        return -1;
    }

    return 0;
}

int FBA_model::save(string file)
{
    /// Open file
    ofstream model_file (file,ios::out | ios::trunc);

    /// Make tree
    pugi::xml_document model_tree;

	pugi::xml_node decl = model_tree.prepend_child(pugi::node_declaration);
	decl.append_attribute("version") = "1.0";
	decl.append_attribute("encoding") = "UTF-8";

    /// Add sbml and model nodes

    pugi::xml_node sbml = model_tree.append_child("sbml");
	pugi::xml_attribute xmlns = sbml.append_attribute("xmlns");
    pugi::xml_attribute sbml_level =  sbml.append_attribute("level");
    pugi::xml_attribute sbml_version =  sbml.append_attribute("version");
	xmlns.set_value("http://www.sbml.org/sbml/level2");
    sbml_level.set_value(2);
    sbml_version.set_value(1);

    pugi::xml_node model = sbml.append_child("model");
    pugi::xml_attribute mod_id = model.append_attribute("id");
    mod_id.set_value(&model_id[0]);

    ///Set flux unit used

    pugi::xml_node unit_defs = model.append_child("listOfUnitDefinitions");
    pugi::xml_node unit_def = unit_defs.append_child("unitDefinition");
    pugi::xml_attribute model_unit = unit_def.append_attribute("id");
    model_unit.set_value(&unit[0]);

    ///Make compartment list

    pugi::xml_node comp_list = model.append_child("listOfCompartments");
    for(const auto &c:compartments)
    {
        pugi::xml_node compart = comp_list.append_child("compartment");
        pugi::xml_attribute comp_id = compart.append_attribute("id");
        comp_id.set_value(&c.first[0]);
        pugi::xml_attribute comp_name = compart.append_attribute("name");
        comp_name.set_value(&c.second.name[0]);
        if(!c.second.outside.empty())
        {
            pugi::xml_attribute comp_out = compart.append_attribute("outside");
            comp_out.set_value(&c.second.outside[0]);
        }
    }

    ///Make species list

    pugi::xml_node spec_list = model.append_child("listOfSpecies");
    for(const auto &s:species)
    {
        pugi::xml_node spec = spec_list.append_child("species");
        pugi::xml_attribute spec_id = spec.append_attribute("id");
        spec_id.set_value(&s.first[0]);
        if(s.second.kegg.length()>0)
        {
            pugi::xml_attribute spec_kegg = spec.append_attribute("kegg");
            spec_kegg.set_value(&s.second.kegg[0]);
        }
        pugi::xml_attribute spec_name = spec.append_attribute("name");
        string aug_name = s.second.name + "_" + s.second.formula;
        replace_mul(aug_name,"'","&");
        spec_name.set_value(&aug_name[0]);
        if(!s.second.compart.empty())
        {
            pugi::xml_attribute spec_comp = spec.append_attribute("compartment");
            spec_comp.set_value(&s.second.compart[0]);
        }
    }

    /// Find reactions list

    pugi::xml_node reac_list = model.append_child("listOfReactions");
    for(const auto &r:reactions)
    {
        pugi::xml_node reac = reac_list.append_child("reaction");
        pugi::xml_attribute reac_id = reac.append_attribute("id");
        reac_id.set_value(&r.first[0]);
		if (r.second.kegg.length()>0)
		{
			pugi::xml_attribute spec_kegg = reac.append_attribute("kegg");
			spec_kegg.set_value(&r.second.kegg[0]);
		}
        pugi::xml_attribute reac_name = reac.append_attribute("name");
        string aug_name = r.second.name;
        replace_mul(aug_name,"'","&");
        reac_name.set_value(&aug_name[0]);
        pugi::xml_attribute reac_rev = reac.append_attribute("reversible");
        reac_rev.set_value((r.second.reversible) ? "true" : "false");

        /// add gene names
        pugi::xml_node reac_notes = reac.append_child("notes");
        string assoc = "GENE_ASSOCIATION:";
        for(const string &i:r.second.genes) assoc += " " + i;
        pugi::xml_node reac_genes = reac_notes.append_child(pugi::node_pcdata);
        reac_genes.set_value(&assoc[0]);

        /// add reactants

        if(r.second.reactants.size()>0)
        {
            pugi::xml_node reac_reac_list = reac.append_child("listOfReactants");
            for(const auto &s:r.second.reactants)
            {
                pugi::xml_node reac_reac = reac_reac_list.append_child("speciesReference");
                pugi::xml_attribute reac_reac_spec = reac_reac.append_attribute("species");
                reac_reac_spec.set_value(&s.first[0]);
                char val [256];
                sprintf(val,"%g",s.second);
                pugi::xml_attribute reac_reac_stoi = reac_reac.append_attribute("stoichiometry");
                reac_reac_stoi.set_value(val);
            }
        }

        /// add products
        if(r.second.products.size()>0)
        {
            pugi::xml_node reac_prod_list = reac.append_child("listOfProducts");
            for(const auto &s:r.second.products)
            {
                pugi::xml_node reac_prod = reac_prod_list.append_child("speciesReference");
                pugi::xml_attribute reac_prod_spec = reac_prod.append_attribute("species");
                reac_prod_spec.set_value(&s.first[0]);
                char val [256];
                sprintf(val,"%g",s.second);
                pugi::xml_attribute reac_prod_stoi = reac_prod.append_attribute("stoichiometry");
                reac_prod_stoi.set_value(val);
            }
        }

        ///Add bounds and objective coefficients

        pugi::xml_node reac_kin = reac.append_child("kineticLaw");
        pugi::xml_node reac_param_list = reac_kin.append_child("listOfParameters");
        if(r.second.low_bound!=0 || r.second.up_bound!=0)
        {
            pugi::xml_node reac_param_l = reac_param_list.append_child("parameter");
            pugi::xml_attribute param_id_l = reac_param_l.append_attribute("id");
            param_id_l.set_value("LOWER_BOUND");
            pugi::xml_attribute param_val_l = reac_param_l.append_attribute("value");
            param_val_l.set_value(r.second.low_bound);

            pugi::xml_node reac_param_u = reac_param_list.append_child("parameter");
            pugi::xml_attribute param_id_u = reac_param_u.append_attribute("id");
            param_id_u.set_value("UPPER_BOUND");
            pugi::xml_attribute param_val_u = reac_param_u.append_attribute("value");
            param_val_u.set_value(r.second.up_bound);
        }
        if(r.second.obj_coeff!=0)
        {
            pugi::xml_node reac_param = reac_param_list.append_child("parameter");
            pugi::xml_attribute param_id = reac_param.append_attribute("id");
            param_id.set_value("OBJECTIVE_COEFFICIENT");
            pugi::xml_attribute param_val = reac_param.append_attribute("value");
            param_val.set_value(r.second.obj_coeff);
        }
    }

    model_tree.save(model_file);
    model_file.close();

    printf("\n ");
    for(int i=0;i<95;i++) printf(A_MAG "/" A_RES);
    printf(A_MAG "\n%-94s//\n //   " A_BLU "%-23s%-23s%-23s%s" A_MAG "   //"," //","Saved Model:","Flux Unit:","Species Number:","Reaction Number:");
    printf("\n //   " A_MAG "%-23s%-23s%-23i%-19i" A_MAG "//\n%-94s//\n " A_RES,model_id.c_str(),unit.c_str(),(int)species.size(),(int)reactions.size()," //");
    for(int i=0;i<95;i++) printf(A_MAG "/" A_RES);
    printf("\n");

    return 0;
}

int replace_mul(string &str,string query,string replacement)
{
    int num = 0;
    size_t beg = str.find(query,0);
    while(beg!=string::npos)
    {
        num++;
        str.replace(beg,query.size(),replacement);
        beg = str.find(query,0);
    }
    return num;
}