#include<iostream>
#include<map>
#include<vector>
#include<string>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <string>
#include <map>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace std;


static std::vector<std::string>
split_util2( std::string const& original, char separator)
{
    std::vector<std::string> results;
    std::string::const_iterator start = original.begin();
    std::string::const_iterator end = original.end();
    std::string::const_iterator next = std::find( start, end, separator );
    while ( next != end ) {
        results.emplace_back( start, next );
        start = next + 1;
        next = std::find( start, end, separator );
    }
    results.emplace_back( start, next );
    return results;
}

bool fileToMap(const std::string &filename,std::map<std::string,long> &fileMap)  //Read Map
{
    ifstream ifile;
    ifile.open(filename.c_str());
    if(!ifile)
        return false;   //could not read the file.
    string line;
    string key;
    vector<string> v_str;
    while(ifile>>line)
    {
        v_str = split_util2(line,',');
        fileMap[v_str[0]] = stoul(v_str[1]);
    }
    return true;
}

int main(int argc,char *argv[]){
	std::map<std::string, long> table_map = {};
	string file_name=string(argv[1]);
	fileToMap(file_name,table_map);
	for(auto& a: table_map){
     		std::cout << "Table Name= " << a.first << " ID= " << a.second << std::endl;
   	}
}
