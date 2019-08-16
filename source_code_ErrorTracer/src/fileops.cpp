#include <fileops.h>

int string_to_file(string &write_file,string file_path)
{
    ofstream f (file_path,ios::out | ios::trunc);
    f << write_file;
    f.close();
    return 0;
}


