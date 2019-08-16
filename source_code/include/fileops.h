#ifndef FILEOPS_H_INCLUDED
#define FILEOPS_H_INCLUDED

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <fstream>

using namespace std;

int open_as_string(string &read_file,string file_path);
int string_to_file(string &write_file,string file_path);
vector<string> purge_empty(const vector<string> &v);
vector<string> purge_duplicates(vector<string> v);
vector<string> remove_chars(const vector<string> &v,string x);
vector<int> to_int(const vector<string> &v);
vector<string> extr_col_by_title(string title,const string &text,char delim);

#endif // FILEOPS_H_INCLUDED