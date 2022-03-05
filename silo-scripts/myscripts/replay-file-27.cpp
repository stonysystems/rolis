#include <iostream>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <string.h>
#include <chrono>
#include <vector>
#include <map>
#include <cstring>
#include <algorithm>
#include <stdlib.h> // https://stackoverflow.com/questions/1646031/strtoull-and-long-long-arithmetic

using namespace std;

std::map<long, std::string> table_map = {};
int OFFSET_CID = 0; // sizeof = 8
int OFFSET_K = 8 ; // sizeof = 8
int OFFSET_V = 16 ; // sizeof = 8
int OFFSET_TABLE_ID = 24; // sizeof = 2
int OFFSET_DELETE_TRUE = 26 ; // sizeof = 1

long get_file_size(std::string filename)
{
    struct stat *stat_buf=new struct stat();
    int rc = stat(filename.c_str(), stat_buf);
    auto ans = rc == 0 ? stat_buf->st_size : -1;
    delete stat_buf;
    return ans;
}

inline std::vector<std::string>
split_util2( std::string const& original, char separator)
{
    std::vector<std::string> results;
    results.reserve(32);
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

bool fileToMap(const std::string &filename,std::map<long, std::string> &fileMap)  //Read Map
{
    ifstream ifile;
    ifile.open(filename.c_str());
    if(!ifile) {
        std::cout << "File " << filename << " Can't be read successfully " << '\n';
        return false;   //could not read the file.
    }
    string line;
    string key;
    vector<string> v_str;
    while(ifile>>line)
    {
        v_str = split_util2(line,',');
        fileMap[stoul(v_str[1])] = v_str[0];
    }
    return true;
}

namespace hopman_fast
{
    struct itostr_helper
    {
        static unsigned out[10000];

        itostr_helper()
        {
            for (int i = 0; i < 10000; i++)
            {
                unsigned v = i;
                char * o = (char*)(out + i);
                o[3] = v % 10 + '0';
                o[2] = (v % 100) / 10 + '0';
                o[1] = static_cast<char>((v % 1000) / 100) + '0';
                o[0] = static_cast<char>((v % 10000) / 1000);
                if (o[0])        o[0] |= 0x30;
                else if (o[1] != '0') o[0] |= 0x20;
                else if (o[2] != '0') o[0] |= 0x10;
                else                  o[0] |= 0x00;
            }
        }
    };

    unsigned itostr_helper::out[10000];

    itostr_helper hlp_init;

    template <typename T>
    void itostr(T o, std::string& out)
    {
        typedef itostr_helper hlp;

        unsigned blocks[3], *b = blocks + 2;
        blocks[0] = o < 0 ? ~o + 1 : o;
        blocks[2] = blocks[0] % 10000; blocks[0] /= 10000;
        blocks[2] = hlp::out[blocks[2]];

        if (blocks[0])
        {
            blocks[1]  = blocks[0] % 10000; blocks[0] /= 10000;
            blocks[1]  = hlp::out[blocks[1]];
            blocks[2] |= 0x30303030;
            b--;
        }

        if (blocks[0])
        {
            blocks[0]  = hlp::out[blocks[0] % 10000];
            blocks[1] |= 0x30303030;
            b--;
        }

        char* f = ((char*)b);
        f += 3 - (*f >> 4);

        char* str = (char*)blocks;
        if (o < 0) *--f = '-';
        out.assign(f, (str + 12) - f);
    }

    template <typename T>
    std::string itostr(T o)
    {
        std::string result;
        itostr(o,result);
        return result;
    }
}

void readOneFile(std::string file_name) {
    size_t sz = get_file_size(file_name);
    int fd = open (file_name.c_str (), O_RDONLY);
    size_t ret = 0;
    void *ptr = NULL;
    char *buffer;

    ptr = (void *) malloc (sz);
    buffer = (char *) ptr;
    ret = read (fd, buffer, sz);
    if (ret == -1 || ret == 0) {
        std::cout << "[mymain] file is empty " << ret << std::endl;
    }

    unsigned long long int *cid = 0;
    unsigned long long int *k = 0;
    unsigned long long int *v = 0;
    unsigned short int *table_id = 0;
    unsigned char *delete_true = 0;
    const unsigned char true_flag = 0x1;

    std::ofstream file(file_name + ".log");

    size_t chunk = ret / 27 ;
    for (size_t i=0; i < chunk; ++i) {

        cid = reinterpret_cast<unsigned long long int *>(buffer + i * 27 + OFFSET_CID);
        k = reinterpret_cast<unsigned long long int *>(buffer + i * 27 + OFFSET_K);
        v = reinterpret_cast<unsigned long long int *>(buffer + i * 27 + OFFSET_V);
        table_id = reinterpret_cast<unsigned short int *>(buffer + i * 27 + OFFSET_TABLE_ID);
        delete_true = reinterpret_cast<unsigned char *>(buffer + i * 27 + OFFSET_DELETE_TRUE);
        bool isDeleted = false;
        if(*delete_true & true_flag) {
            isDeleted = true ;
        }

        if (*table_id == 0 || *table_id > 20000) {
            std::cout << "table_id " << *table_id << ":" << i << ":" << ret << std::endl;
            exit(1);
        }
        auto str_key = hopman_fast::itostr(*k);
        std::string row = to_string(*cid) + "," + str_key + "," +  to_string(*v) + "," + to_string(*table_id) + "," + table_map[*table_id] + "," + to_string(isDeleted) + "\n";
        file << row ;
    }
}

namespace so
{
    std::string& itostr(int n, std::string& s)
    {
        if (n == 0)
        {
            s = "0";
            return s;
        }

        int sign = -(n < 0);
        unsigned int val = (n ^ sign) - sign;

        int size;
        if (val >= 10000)
        {
            if (val >= 10000000)
            {
                if (val >= 1000000000)
                    size = 10;
                else if (val >= 100000000)
                    size = 9;
                else
                    size = 8;
            }
            else
            {
                if (val >= 1000000)
                    size = 7;
                else if (val >= 100000)
                    size = 6;
                else
                    size = 5;
            }
        }
        else
        {
            if (val >= 100)
            {
                if (val >= 1000)
                    size = 4;
                else
                    size = 3;
            }
            else
            {
                if (val>=10)
                    size = 2;
                else
                    size = 1;
            }
        }

        s.resize(-sign + size);
        char* c = &s[0];
        if (sign)
            *c++='-';

        char* d = c + size - 1;
        while(val > 0)
        {
            *d-- = '0' + (val % 10);
            val /= 10;
        }
        return s;
    }

    template <typename T>
    std::string itostr(T o)
    {
        std::string result;
        itostr(o,result);
        return result;
    }
}

namespace cppx {
    using std::numeric_limits;
    using std::reverse;

    typedef numeric_limits<long>    Long_info;
    int const long_digits   = Long_info::max_digits10;
    int const long_bufsize  = long_digits + 2;

    inline void unsigned_to_decimal( unsigned long number, char* buffer )
    {
        if( number == 0 )
        {
            *buffer++ = '0';
        }
        else
        {
            char* p_first = buffer;
            while( number != 0 )
            {
                *buffer++ = '0' + number % 10;
                number /= 10;
            }
            reverse( p_first, buffer );
        }
        *buffer = '\0';
    }

    inline auto decimal_from_unsigned( unsigned long number, char* buffer )
    -> char const*
    {
        unsigned_to_decimal( number, buffer );
        return buffer;
    }

    inline void to_decimal( long number, char* buffer )
    {
        if( number < 0 )
        {
            buffer[0] = '-';
            unsigned_to_decimal( -number, buffer + 1 );
        }
        else
        {
            unsigned_to_decimal( number, buffer );
        }
    }

    inline auto decimal_from( long number, char* buffer )
    -> char const*
    {
        to_decimal( number, buffer );
        return buffer;
    }
}  // namespace cppx

int main()
{
//    size_t file_count = 83;
//    fileToMap("/home/weihshen/Desktop/GenLogThd20.Time.20/info/Log-ThreadID:9999.txt",table_map);
//    for(int fid=0;fid<file_count;fid++) {
//        std::string file_name = "/home/weihshen/Desktop/GenLogThd20.Time.20/Log-ThreadID:"+ to_string(fid) +".txt" ;
//        readOneFile(file_name) ;
//    }

    unsigned long long int k = 10232314212323;
    std::string s = hopman_fast::itostr(k) ;
    std::cout << s << " : " << std::to_string(k) << " : " << so::itostr(k) << std::endl;

    std::cout << sizeof(unsigned long long int) << " : " << sizeof(unsigned long) << " : " << sizeof(uint64_t) << " : " << sizeof(unsigned)<< std::endl;

}