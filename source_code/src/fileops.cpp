#include <fileops.h>

int open_as_string(string &read_file,string file_path)
{
    read_file.clear();
    ifstream f (file_path,ios::in);
    if(f.fail()) return 1;
    f.seekg(0, ios::end);
    read_file.reserve(f.tellg());
    f.seekg(0, ios::beg);
    read_file.assign((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    f.close();
    return 0;
}

int string_to_file(string &write_file,string file_path)
{
    ofstream f (file_path,ios::out | ios::trunc);
    f << write_file;
    f.close();
    return 0;
}

vector<string> purge_empty(const vector<string> &v)
{
    vector<string> r;
    r.reserve(v.size());
    for(const string &s:v) if(!s.empty()) r.push_back(s);
    return r;
}

vector<string> purge_duplicates(vector<string> v)
{
    sort(v.begin(),v.end());
    vector<string> r;
    r.reserve(v.size());
    if(!v.empty()) r.push_back(v[0]);
    for(string &i:v) if(i!=r.back()) r.push_back(i);
    return r;
}


string rem_chars(const string &s,string x)
{
    string r;
    for(const char &i:s) if(find(x.begin(),x.end(),i)==x.end()) r.push_back(i);
    return r;
}

vector<string> remove_chars(const vector<string> &v,string x)
{
    vector<string> r (v.size());
    for(size_t i=0;i<v.size();i++) r[i]=rem_chars(v[i],x);
    return r;
}

vector<int> to_int(const vector<string> &v)
{
    vector<int> r(v.size());
    for(size_t i=0;i<v.size();i++)
    {
        string temp = rem_chars(v[i],"\" ");
        try
        {
            if(!temp.empty()) r[i]=stoi(temp);
            else r[i]=0;
        } catch (const invalid_argument& ia)
        {
            r[i]=0;
            cerr << "Invalid argument: " << ia.what() << "\t\t" << temp << '\n';
        }
    }
    return r;
}

vector<string> extr_col_by_title(string title,const string &text,char delim)
{
    vector<string> results;
    int col_num = 0;

    size_t n_pos = text.find("\n");
    vector<string> first_row;
    vector<size_t> fr_bgs;
    first_row.push_back("");
    for(size_t i=0;i<n_pos;i++)
    {
        if(text[i]!=delim) first_row.back().push_back(text[i]);
        else
        {
            first_row.push_back("");
            fr_bgs.push_back(i);
        }
    }
    vector<string>::iterator tit = find(first_row.begin(),first_row.end()+1,title);

    if(tit!=(first_row.end()+1))
    {
        for(size_t i=0;i<fr_bgs[tit-first_row.begin()];i++) if(text[i]==delim) col_num++;
        string::const_iterator pos = text.begin()+n_pos;

        enum state {sear,acc,skip};
        state s = sear;
        int acc_delim = 0;
        string word;
        while(pos<text.end())
        {
            /// Find the col_num'th delimiter on line
            if(s==sear)
            {
                if(*pos==delim) acc_delim++;
                if(acc_delim==col_num) {s=acc; acc_delim=0;}
            }
            else if(s==acc)
            {
                if(*pos==delim)
                {
                    s=skip;
                    results.push_back(word);
                    word.clear();
                }
                else word.push_back(*pos);
            }
            else if(s==skip)
            {
                if(*pos=='\n') s=sear;
            }
            pos++;
        }
    }
    else printf("\n\nERROR! Did not find a column with this title: %s\n\n",title.c_str());

    return results;
}
