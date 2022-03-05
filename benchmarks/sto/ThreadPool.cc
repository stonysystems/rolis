#include <utility>

//
// Created by mrityunjay kumar on 2019-10-25.
//
#include "ThreadPool.h"
#include <stdexcept>
#include <algorithm>
#include<string_view>
#include <unordered_set>

#define MAX_VALUE 10000
#define MAX_READ_LENGTH 1000
#define MAX_CHUNK_SIZE_ALLOWED 6*MB
//512*kB
using namespace std;

str_arena arena;
void *buf = NULL ;

//bool finished = false;
class quit_worker_exception : public std::exception {};

xaw_directs ThreadDBWrapper::replay_thread_wrapper_db;

int OFFSET_CID = 0; // sizeof = 8
int OFFSET_K = 8 ; // sizeof = 8
int OFFSET_V = 16 ; // sizeof = 8
int OFFSET_TABLE_ID = 24; // sizeof = 2
int OFFSET_DELETE_TRUE = 26 ; // sizeof = 1

#define STD_OP2(f) \
  try { \
    f; \
  } catch (Transaction::Abort E) { \
    throw std::invalid_argument("Error"); \
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


int GetNextThreadID(){
  std::atomic<int> thread_id{0};
  thread_id+=1;
  return thread_id;
}

//mutable std::shared_mutex mpublisher_lock;

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

std::vector<std::string> GetDecoded(std::string input) {
  std::vector<std::string> results = split_util2 (input, ':');
  if(results.size() !=2){
    results.emplace_back("0");
    results.emplace_back("0");
  }
  return results;
}

inline std::string Encode(const std::string& input1,const std::string& input2){
    return input1 + std::string(":") + input2;
}

void fix_start_end_pos(FILE *fp,size_t &start,size_t &endp,size_t chunk_size)
{
    size_t ul_len = sizeof (unsigned long long int);
    size_t one_block = 3*ul_len;
    endp += chunk_size - (chunk_size % one_block);
	fseeko (fp, endp, SEEK_SET);
	endp = ftello (fp);
}

bool sortbysecasc(const pair<container_hello*,int> &a,
              const pair<container_hello*,int> &b)
{
    return (a.second < b.second);
}

bool sortbysecdsc(const pair<container_hello*,int> &a,
              const pair<container_hello*,int> &b)
{
    return (a.second < b.second);
}

long get_file_size(std::string filename)
{
    struct stat *stat_buf=new struct stat();
    int rc = stat(filename.c_str(), stat_buf);
    auto ans = rc == 0 ? stat_buf->st_size : -1;
    delete stat_buf;
  	return ans;
}

bool fileToMap(const std::string &filename,std::map<std::string,long> &fileMap)  //Read Map
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
        fileMap[v_str[0]] = stoul(v_str[1]);
    }
    return true;
}

size_t getTotalTransInfo(const std::string &filename) {
    ifstream ifile;
    ifile.open(filename.c_str());
    if(!ifile)
        return -1;   // could not read the file.
    string line;
    string key;
    vector<string> v_str;
    while(ifile>>line)
    {
        if (line.find("total_number,") != string::npos) {
            v_str = split_util2(line,',');
            return stoul(v_str[1]);
        }
    }
    return -1;
}

template <typename T>
std::vector<T> GetSlice(std::vector<T> &input, int start,int end)
{
  std::vector<T> slice(input.cbegin()+start,input.cbegin()+end);
  return slice;
}
size_t load_objects_from_buffer_no_table(char *buffer, int length,int wthreads,int nthreads, const std::map<std::string,
	long>& table_map,std::vector<one_container*>& all_containers){
  size_t ULL_LEN = sizeof (unsigned long long int);
//  std::string dummy_file_name = "demo_file";
  std::unordered_set<long> all_files_names_temp={};
  
  int tid = 0;
  std::unordered_set<unsigned long long int> _count={};
  for(auto &table : table_map)
  {
//    std::cout << "F " << table.first << " S:" << table.second << std::endl;
    all_files_names_temp.insert(table.second);
  }
  size_t total_entries_global = 0;
//  size_t sz = length;
  int covered = 0;
  size_t rw=0;
  size_t ret=length;
  
  
  for (size_t k = 0; rw < ret; k++) {
    if(covered >= length)
	  break;
	auto *entry = new one_container (buffer, rw);
	total_entries_global++;
	auto table_id = entry->table_id;
	_count.insert(entry->cid);
	if (all_files_names_temp.find (table_id) != all_files_names_temp.end ()) {
	  all_containers.push_back(entry);
	} else {
	  std::cout << "Table id= " << table_id << " Massive error here Covered= " << covered  << " total : " << length << std::endl;
	  exit(1);
	}
	covered += 27;
  }
  return _count.size();
}

void load_objects_from_buffer(char *buffer, int length,int wthreads,int nthreads, const std::map<std::string,
	long>& table_map,std::vector<container_hello*>& all_files_names){
  size_t ULL_LEN = sizeof (unsigned long long int);
//  std::string dummy_file_name = "demo_file";
  std::map<long, container_hello*> all_files_names_temp={};
  std::map<long,int> simple_tid_mapping={};
  std::map<long, std::string> table_map_rev={};
  
  int tid = 0;
  for(auto &table : table_map)
  {
//    std::cout << "F " << table.first << " S:" << table.second << std::endl;
    table_map_rev[table.second] = table.first;
    all_files_names_temp[table.second] = new container_hello (0, 0, table.first, nullptr, wthreads,
    	tid++);
  }
  size_t total_entries_global = 0;
//  size_t sz = length;
  int covered = 0;
  size_t rw=0;
  size_t ret=length;
  std::set<unsigned long long int> unique_keys;
  
  
  for (size_t k = 0; rw < ret; k++) {
    if(covered >= length)
	  break;
	auto *entry = new one_container (buffer, rw);
	total_entries_global++;
	unique_keys.insert(entry->k);
	auto table_id = entry->table_id;
	if (all_files_names_temp.find (table_id) != all_files_names_temp.end ()) {
	  all_files_names_temp[table_id]->add_one_container (entry);
	} else {
	  std::cout << "Table id= " << table_id << " Massive error here Covered= " << covered  << " total : " << length << std::endl;
	  exit(1);
	}
	covered += (ULL_LEN * 6);
  }
  
//  std::cout << "-------========-------==========----------" << endl;
//  std::cout << "Micro Processing done " << std::endl;
  static int tid_start=0;
  for(auto &table:all_files_names_temp) {
    if(table.second->GetItems ().empty())
	  continue;
    simple_tid_mapping[table.first] = (tid_start%32);
   	tid_start++;
  }
  
//  std::cout << "\n[Loader Stats] " << total_entries_global << std::endl;
  
  for(auto &table:all_files_names_temp) {
	size_t exhausted = 0;
  	auto all_here = table.second->GetItems ();
//  	std::cout << "Table ID : " << table.first << " Table Name : " << table_map_rev[table.first] << " Entries in It : " << table.second->GetContainerCounts ();
  	size_t rest_mod_slice_size=0;
	size_t one_slice_size = (all_here.size () / size_t (nthreads));
	rest_mod_slice_size = all_here.size ()%nthreads;
 
	auto nthreadsc = nthreads;
	bool exit_now=false;
	size_t processed = 0;
	while (nthreadsc--) {
	  size_t slice_size =
		  exhausted + one_slice_size < total_entries_global ? one_slice_size : total_entries_global - exhausted;
	  if((nthreadsc==0 || slice_size==0) && rest_mod_slice_size!=0){
	    slice_size+=rest_mod_slice_size;
	    exit_now = true;
	  }
	  if (slice_size <= 0)
		break;
	  std::vector<one_container *> new_arr = GetSlice (all_here, exhausted, exhausted + slice_size);
	  if(new_arr.empty()){
	    break;
	  }
	  container_hello *c1;
	  if(simple_tid_mapping[table.first]==0){
	    c1 = new container_hello (0, 0, table_map_rev[table.first], nullptr, simple_tid_mapping[table.first], table.first);
	  }else{
	    wthreads = (wthreads+1) % nthreads;
	    if(wthreads==0){
	      wthreads++;
	    }
	    c1 = new container_hello (0, 0, table_map_rev[table.first], nullptr, simple_tid_mapping[table.first], table.first);
	  }
	  
	  c1->bulk_add_containers (new_arr);
	  processed+=new_arr.size();
	  all_files_names.emplace_back (c1);
	  exhausted += slice_size;
	  if(exit_now)
		break;
	}
//	cout << " Added : " << processed << endl;
  }
//  cout << "Finished Loading" << std::endl;
}




void get_all_files(unsigned long count,int wthreads,int nthreads, const std::map<std::string, long>& table_map,
	std::vector<container_hello*>& all_files_names)
{
  size_t ULL_LEN = sizeof (unsigned long long int);
  std::string dummy_file_name = "demo_file";
  std::map<long, container_hello*> all_files_names_temp={};
  std::map<long,int> simple_tid_mapping={};
  std::map<long, std::string> table_map_rev={};
  
  int tid = 0;
  for(auto &table : table_map)
  {
    table_map_rev[table.second] = table.first;
    all_files_names_temp[table.second] = new container_hello (0, 0, table.first, nullptr, wthreads,
    	tid++);
  }
  size_t total_entries_global = 0;
  
  for (unsigned long i = 0; i < count ; i++) {
		  std::string file_name = std::string (LOG_FOLDER_DIR) + \
										std::string ("Log-ThreadID:") + \
										std::to_string (i) +std::string(".txt" );
//		  std::cout << "Start processing file : " << file_name << std::endl;
		  size_t sz = get_file_size(file_name);
		  size_t original_sz = sz;
		  size_t covered = 0;
		  void *ptr=NULL;
		  char *buffer;
		  size_t rw=0;
		  size_t ret=0;
		  size_t total_entries_local = 0;
		  std::set<unsigned long long int> unique_keys;
		  int fd = open(file_name.c_str(), O_RDONLY);
		  
		  if(fd<=0){
			return;
		  }
		  
		  for(size_t times=0; times <= sz/MAX_CHUNK_SIZE_ALLOWED;times++) {
		    if (covered >= original_sz) {
				break;
			}
		    rw = 0;
			ptr = (void *) malloc (MAX_CHUNK_SIZE_ALLOWED);
			buffer = (char *) ptr;
			ret = read (fd, buffer, MAX_CHUNK_SIZE_ALLOWED);
			if (ret <= 0) {
			  std::cout << "ERROR ret=1" << std::endl;
			  return;
			}
			for (size_t k = 0; rw < ret; k++) {
			  auto *entry = new one_container (buffer, rw);
			  total_entries_global++;
			  total_entries_local+=1;
			  unique_keys.insert(entry->k);
			  auto table_id = entry->table_id;
			  if (all_files_names_temp.find (table_id) != all_files_names_temp.end ()) {
				all_files_names_temp[table_id]->add_one_container (entry);
			  } else {
				std::cout << "Table id= " << table_id << " Massive error here " << std::endl;
				return;
			  }
			  covered += (ULL_LEN * 6);
			}
			free (ptr);
		  }
//		  std::cout << "Original Size " << original_sz << " covered size : " << covered << " Entries Retrived " << total_entries_local << " Unique Entries : " << unique_keys.size() << std::endl;
		  close (fd);
  }
//  std::cout << "-------========-------==========----------" << endl;
//  std::cout << "Micro Processing done " << std::endl;
  int tid_start=0;
  for(auto &table:all_files_names_temp) {
    if(table.second->GetItems ().empty())
	  continue;
    simple_tid_mapping[table.first] = tid_start;
   	tid_start++;
  }
  
  for(auto &table:all_files_names_temp) {
	size_t exhausted = 0;
  	auto all_here = table.second->GetItems ();
//  	std::cout << "Table ID : " << table.first << " Table Name : " << table_map_rev[table.first] << " Entries in It : " << table.second->GetContainerCounts ();
  	size_t rest_mod_slice_size=0;
	size_t one_slice_size = (all_here.size () / size_t (nthreads));
	rest_mod_slice_size = all_here.size ()%nthreads;
 
	auto nthreadsc = nthreads;
	bool exit_now=false;
	size_t processed = 0;
	while (nthreadsc--) {
	  size_t slice_size =
		  exhausted + one_slice_size < total_entries_global ? one_slice_size : total_entries_global - exhausted;
	  if((nthreadsc==0 || slice_size==0) && rest_mod_slice_size!=0){
	    slice_size+=rest_mod_slice_size;
	    exit_now = true;
	  }
	  if (slice_size <= 0)
		break;
	  std::vector<one_container *> new_arr = GetSlice (all_here, exhausted, exhausted + slice_size);
	  if(new_arr.empty()){
	    break;
	  }
	  container_hello *c1;
	  if(simple_tid_mapping[table.first]==0){
	    c1 = new container_hello (0, 0, table_map_rev[table.first], nullptr, simple_tid_mapping[table.first], table.first);
	  }else{
	    wthreads = (wthreads+1) % nthreads;
	    if(wthreads==0){
	      wthreads++;
	    }
	    c1 = new container_hello (0, 0, table_map_rev[table.first], nullptr, simple_tid_mapping[table.first], table.first);
	  }
	  
	  c1->bulk_add_containers (new_arr);
	  processed+=new_arr.size();
	  all_files_names.emplace_back (c1);
	  exhausted += slice_size;
	  if(exit_now)
		break;
	}
//	cout << " Added : " << processed << endl;
  }
  
}

#if 0
std::vector<container_hello*> get_all_files_no_need(unsigned long count,int nthreads){
    std::vector<container_hello*> all_files_names;
    std::vector<std::pair<container_hello*,int>> temp;
    {
        for (unsigned long i = 0; i < count ; ++i) {
          	std::string file_name = std::string (LOG_FOLDER_DIR) + \
                                          std::string ("Log-ThreadID:") + \
                                          std::to_string (i) +std::string(".txt" );
          	size_t file_size = get_file_size(file_name);
          	size_t start_pos=0;
          	size_t end_pos=0;
          	FILE *fp = fopen(file_name.c_str(),"rb");

          	size_t j = 0;
          	if((file_size/(MAX_CHUNK_SIZE_ALLOWED)) >1) {
			  for (; j < (file_size / (MAX_CHUNK_SIZE_ALLOWED)) - 1; ++j) {

				fix_start_end_pos (fp, start_pos, end_pos, MAX_CHUNK_SIZE_ALLOWED);
				container_hello *c1 = new container_hello (start_pos, end_pos, file_name, nullptr,nthreads, 0);
				temp.emplace_back (c1,j);
				start_pos = end_pos;
			  }
			}
			container_hello* c2 = new container_hello(start_pos,file_size,file_name, nullptr,nthreads, 0);
			temp.emplace_back (std::make_pair (c2,j));
			fclose(fp);
        }
    }
    sort (temp.begin(),temp.end());
    sort (temp.begin(),temp.end(),sortbysecdsc);
  	for(auto &each:temp){
  	  all_files_names.emplace_back(each.first);
  	}
    std::cout <<"Loading all file meta info <<Done>>" << std::endl;
    return all_files_names;
}
#endif

uint64_t get_hex_key(const std::string input) {
//    if(!input || strlen (input)==0){
//        assert (false);
//    }
//    if(strstr (input, "?"))
//        return 0;
//    if((strlen (input)<=2) || !strstr (input, "0x"))
//        return std::stoul (input,nullptr,0);

    std::istringstream ss(input);
    uint64_t a=0;
    try {
        ss >> std::hex >> a;
    }
    catch (...){
        std::cout << "Parse failed\n";
        assert (false);
    }
//    assert(to_hex_string(a) == std::string(input));
    return a;
}

struct keystore{
	unsigned long long int k1; /* commit id */
	unsigned long long int k2; /* value */
};

static void *data_store = (void *)malloc (sizeof (char)* sizeof (struct keystore));

void keystore_encode2(std::string& s, unsigned long long int x, unsigned long long int y){
	auto *encoded_pair = (struct keystore *) malloc(sizeof (struct keystore));
	encoded_pair->k1=x;
	encoded_pair->k2=y;
	memcpy ((void *)s.data(), (void *)encoded_pair, sizeof (struct keystore));
}
size_t LEN_ULL = sizeof (unsigned long long int );

void keystore_encode3(std::string& s, unsigned long long int x, unsigned long long int y){
	memcpy ((void *)s.data(), &x, LEN_ULL);
	memcpy ((void *)(s.data()+LEN_ULL), &y, LEN_ULL);
}


inline unsigned long long int keystore_decode3(const std::string& s, bool first= false){
	unsigned long long int x=0;
	if(first) {
	  memcpy (&x, (void *) (s.data ()), LEN_ULL);
	  return x;
	}else {
	  memcpy (&x, (void *) (s.data () + LEN_ULL), sizeof (unsigned long long int));
	  return x;
	}
}

std::string keystore_encode(unsigned long long int x, unsigned long long int y){
	struct keystore encoded_pair{.k1=x, .k2=y};
	memcpy (data_store, &encoded_pair, sizeof (struct keystore));
	std::string s = std::string((char*)data_store, sizeof (struct keystore));
	return s;
}

struct keystore decode(const string& s){
	struct keystore obj{};
	memcpy (&obj, s.data(),sizeof (struct keystore));
	return obj;
}

#ifndef MASS_DIRECT
void nontrans_get(new_ds &h, std::string& k,unsigned long long int& table_id,std::string &v, bool &exists) {
        {
            TransactionGuard guard;
            exists = h[table_id].transGet(k, v);
        }
}
#endif

bool cmpFunc(const std::string& newValue,const std::string& oldValue)
{
  auto retrieved_value_1 = decode(newValue);
  auto retrieved_value_2 = decode(oldValue);
  
  return retrieved_value_1.k1 > retrieved_value_2.k1;
}

bool cmpFunc2(const std::string& newValue,const std::string& oldValue)
{
  unsigned long long int commit_id_new = keystore_decode3(newValue, true);
  unsigned long long int commit_id_old = keystore_decode3(oldValue, true);
  
  return commit_id_new > commit_id_old;
}

void insertEntry(one_container* entry, new_directs &h) {
    std::string value = "";
    value.resize(2* sizeof (unsigned long long int));
    entry->str_key=hopman_fast::itostr(entry->k);
    if(entry->delete_true & 0x1) // delete true
    {
        // if delete flag is tru, its okay to loose the value being inserted
        // we will push the sentinel instead of original value to enable garbage collection
        keystore_encode3 (value, entry->cid, 999);
        h[entry->table_id].put (entry->str_key, value, cmpFunc2);
    } else{
        // first time and update
        keystore_encode3 (value, entry->cid, entry->v);
        h[entry->table_id].put (entry->str_key, value, cmpFunc2);
    }
}

void insertEntry_pointer(unsigned long long int* cid,
                         unsigned long long int* k,
                         unsigned long long int* v,
                         unsigned short int* table_id,
                         unsigned char* delete_true,
                         new_directs &h) {
    std::string value = "";
    value.resize(2* sizeof (unsigned long long int));
    auto str_key=hopman_fast::itostr(*k);
    if(*delete_true & 0x1) // delete true
    {
        // if delete flag is tru, its okay to loose the value being inserted
        // we will push the sentinel instead of original value to enable garbage collection
        keystore_encode3 (value, *cid, 999);
        h[*table_id].put (str_key, value, cmpFunc2);
    } else{
        // first time and update
        keystore_encode3 (value, *cid, *v);
        h[*table_id].put (str_key, value, cmpFunc2);
    }
}

size_t getFileContentNew_optimized_pointer(char *init, size_t chunk, new_directs &h)
{
    size_t put_ops = 0;
    string value="";
    unsigned long long int *cid = 0;
    unsigned long long int *k = 0;
    unsigned long long int *v = 0;
    unsigned short int *table_id = 0;
    unsigned char *delete_true = 0;

    value.resize(2* sizeof (unsigned long long int));
    const unsigned char true_flag = 0x1;
	#if 0
    struct EncStruct{
      unsigned long long int key;
      unsigned long long int value;
      unsigned long long int commitID;
    };
    std::vector<EncStruct> vectors = {EncStruct{.key=10021,.value=97768,.commitID=100},
									  EncStruct{.key=10021,.value=45552,.commitID=110},
									  EncStruct{.key=10021,.value=42331,.commitID=101}};
  	#endif
    for (size_t i=0; i<chunk; i++) {

        cid = reinterpret_cast<unsigned long long int*>(init + i * 27 + OFFSET_CID);
        k = reinterpret_cast<unsigned long long int*>(init + i * 27 + OFFSET_K);
        v = reinterpret_cast<unsigned long long int*>(init + i * 27 + OFFSET_V);
        table_id = reinterpret_cast<unsigned short int*>(init + i * 27 + OFFSET_TABLE_ID);
        delete_true = reinterpret_cast<unsigned char*>(init + i * 27 + OFFSET_DELETE_TRUE);

        // FIXME: be more accurate
        if (*table_id == 0 || *table_id > 20000) {
            std::cout << "error: table_id " << *table_id << std::endl;
            exit(1);
	} else {
            //std::cout << "cid " << *cid << ", table_id " << *table_id << ", k " << *k << ", v " << *v << std::endl;
        }


        try {
            auto str_key = hopman_fast::itostr(*k); // XXX, hopman_fast can't support uint_64
            if(*delete_true & true_flag) // delete true
            {
                // if delete flag is tru, its okay to loose the value being inserted
                // we will push the sentinel instead of original value to enable garbage collection
                keystore_encode3 (value, *cid, 999);
                h[*table_id].put (str_key, value, cmpFunc2);
                put_ops ++;
            } else{
                // first time and update
                keystore_encode3 (value, *cid, *v);
                h[*table_id].put (str_key, value, cmpFunc2);
                put_ops ++;
            }
        } catch (...) {
            std::cout << "exception" << std::endl;
        }
    }
    return put_ops;
}

size_t getFileContentOz(xew_directs *db,
	char *buffer,
	size_t chunk,
	unsigned long long int cid,
	const std::unordered_set<long>& all_files_names_temp)
{
	size_t put_ops = 1;
    string value="";
    unsigned long long int *k = 0;
    unsigned long long int *v = 0;
    unsigned short *table_id = 0;
	unsigned short int *_fake_tid = 0;
    bool delete_true = false;

    value.resize(2* sizeof (unsigned long long int));
    const unsigned char true_flag = 0x1;

    for (size_t i=0; i<chunk; i++) {

        k = reinterpret_cast<unsigned long long int*>(buffer + i * DECL_OPTIMAL_SIZE + DECL_OFFSET_K);
        v = reinterpret_cast<unsigned long long int*>(buffer + i * DECL_OPTIMAL_SIZE + DECL_OFFSET_V);
        table_id = reinterpret_cast<unsigned short*>(buffer + i * DECL_OPTIMAL_SIZE + DECL_OFFSET_TABLE_ID);
        if((*table_id) & (1 << 15)){
          // check if MSB is set
          delete_true = true;
        // clear the bit
          *table_id = (*table_id) ^ (1 << 15);
        }
        
        if (all_files_names_temp.find (*table_id) == all_files_names_temp.end ()) {
            std::cout << "Table id= " << *table_id << " doesn't exist" << std::endl;
            continue;
        }
        try {
            auto str_key = hopman_fast::itostr(*k);
            if(delete_true != 0) // delete true
            {
                // if delete flag is tru, its okay to loose the value being inserted
                // we will push the sentinel instead of original value to enable garbage collection
                keystore_encode3 (value, cid, 999);
                db->insert(*table_id, str_key, value, cmpFunc2);
                put_ops ++;
            } else{
                // first time and update
                keystore_encode3 (value, cid, *v);
                db->insert(*table_id, str_key, value, cmpFunc2);
                put_ops ++;
            }
        } catch (...) {
            std::cout << "exception" << std::endl;
        }
    }
    return put_ops;
}

unsigned long long int resolve64(std::string s) {
    unsigned long long int *k  ;
    k = reinterpret_cast<unsigned long long int*>((char *)s.data()) ;
    return *k ;
}

// derived from getFileContentNew_OneLogOptimized
size_t getFileContentNew_OneLogOptimized_mbta(char *buffer,
                                              unsigned long long int cid, size_t chunk, abstract_db* db, const std::unordered_set<long>& all_files_names_temp) {
    size_t put_ops = 1;
    string value="";
    std::string myKey ;
    myKey.resize(sizeof (unsigned long long int)) ;

    unsigned long long int *v = 0;
    unsigned short *table_id = 0;
    unsigned short int *_fake_tid = 0;
    bool delete_true = false;

    value.resize(2* sizeof (unsigned long long int));
    const unsigned char true_flag = 0x1;
    for (size_t i=0; i<chunk; i++) {
        myKey.assign(buffer + i * DECL_OPTIMAL_SIZE + DECL_OFFSET_K, LEN_ULL);
        v = reinterpret_cast<unsigned long long int*>(buffer + i * DECL_OPTIMAL_SIZE + DECL_OFFSET_V);
        table_id = reinterpret_cast<unsigned short*>(buffer + i * DECL_OPTIMAL_SIZE + DECL_OFFSET_TABLE_ID);
        if((*table_id) & (1 << 15)){
            // check if MSB is set
            delete_true = true;
            // clear the bit
            *table_id = (*table_id) ^ (1 << 15);
        }

        if (*table_id == 0 || *table_id > 20000) {
            std::cout << "table_id " << *table_id << std::endl;
            exit(1);
        }

        if(delete_true != 0) // delete true
        {
            // if delete flag is true, its okay to loose the value being inserted
            // we will push the sentinel instead of original value to enable garbage collection
            keystore_encode3 (value, cid, 999);
        } else{
            // first time and update
            keystore_encode3 (value, cid, *v);
        }

        // TODO, optimization: combine key-value in same chunk into a transaction and execute them together
        int try_cnt = 1 ;
        while (1) {
            try {
                void *txn = db->new_txn(0, arena, buf, abstract_db::HINT_DEFAULT);
                abstract_ordered_index *table_index = db->open_index(*table_id) ;
                table_index->put_mbta(txn, myKey, cmpFunc2, value);
                auto ret = db->commit_txn_no_paxos(txn);
                //printf ("%d_%p\n", *table_id, &h[*table_id]);
                if (try_cnt > 1) {
                    std::cout << "succeed at retry#" << try_cnt << std::endl;
                }
                break ;
            } catch (...) {   // if abort happens, replay it until it succeeds
                std::cout << "exception, retry#" << try_cnt << std::endl;
                try_cnt += 1 ;
            }
        }
        put_ops ++;
    }
    return put_ops;
}

size_t getFileContentNew_OneLogOptimized(char *buffer,
	unsigned long long int cid, size_t chunk, new_directs &h,const std::unordered_set<long>& all_files_names_temp)
{
	size_t put_ops = 1;
    string value="";
    std::string myKey ;
    myKey.resize(sizeof (unsigned long long int)) ;

    unsigned long long int *v = 0;
    unsigned short *table_id = 0;
	unsigned short int *_fake_tid = 0;
    bool delete_true = false;

    value.resize(2* sizeof (unsigned long long int));
    const unsigned char true_flag = 0x1;

    for (size_t i=0; i<chunk; i++) {

        myKey.assign(buffer + i * DECL_OPTIMAL_SIZE + DECL_OFFSET_K, LEN_ULL);

        v = reinterpret_cast<unsigned long long int*>(buffer + i * DECL_OPTIMAL_SIZE + DECL_OFFSET_V);
        table_id = reinterpret_cast<unsigned short*>(buffer + i * DECL_OPTIMAL_SIZE + DECL_OFFSET_TABLE_ID);
        if((*table_id) & (1 << 15)){
          // check if MSB is set
          delete_true = true;
        // clear the bit
          *table_id = (*table_id) ^ (1 << 15);
        }

        if (*table_id == 0 || *table_id > 20000) {
            std::cout << "table_id " << *table_id << std::endl;
            exit(1);
	    }
        
        try {
            if(delete_true != 0) // delete true
            {
                // if delete flag is tru, its okay to loose the value being inserted
                // we will push the sentinel instead of original value to enable garbage collection
                keystore_encode3 (value, cid, 999);
                h[*table_id].put (myKey, value, cmpFunc2);
                //printf ("del_%d_%p\n", *table_id, &h[*table_id]);
                put_ops ++;
            } else{
                // first time and update
                keystore_encode3 (value, cid, *v);
                h[*table_id].put (myKey, value, cmpFunc2);
                //printf ("%d_%p\n", *table_id, &h[*table_id]);
                put_ops ++;
            }
        } catch (...) {
            std::cout << "exception" << std::endl;
        }
    }
    return put_ops;
}

size_t getFileContentNew_optimized(const std::vector<one_container*>& arg, new_directs &h)
{
	size_t put_ops = 1;
//	std::cout << "start push to tables " << " Txns here " << arg.size() << " :::"<<std::endl;
	string value="";
	value.resize(2* sizeof (unsigned long long int));
 	int i=0;
 	const unsigned char true_flag = 0x1;
 	for (auto &item: arg){
 	  item->str_key = hopman_fast::itostr(item->k);
 	}
	for (auto &item: arg) {
//      std::cout << "count = " << i << std::endl;
//      std::cout << "DEBUG cid= " << item->cid << " key= "<<item->str_key << " value= "<<item->v << " table_id= "<< item->table_id << std::endl;
      i++;
	  try {
	    if(item->delete_true & true_flag) // delete true
	    {
	      // if delete flag is tru, its okay to loose the value being inserted
	      // we will push the sentinel instead of original value to enable garbage collection
	      keystore_encode3 (value, item->cid, 999);
	      h[item->table_id].put (item->str_key, value, cmpFunc2);
	      put_ops ++;
	    } else{
	      // first time and update
	      keystore_encode3 (value, item->cid, item->v);
	      h[item->table_id].put (item->str_key, value, cmpFunc2);
	      put_ops ++;
	    }
	  } catch (...) {
	    std::cout << "excpetion" << std::endl;
	  }
//	  std::cout << "----------------------------" << std::endl;
	}
//	std::cout << "----------------------------" << std::endl;
//	std::cout << "--------- FINISHED ---------" << std::endl;
//	std::cout << "----------------------------" << std::endl;
	return put_ops;
}

bool getFileContentNew_direct(const std::vector<one_container*>& arg, new_directs &h, std::atomic<int> &success_counts,
	std::atomic<int> &insert_stats_count, std::atomic<int> &delete_stats_count,std::atomic<int> &failed,
	std::atomic<size_t >& txns_count_this)
{
//	std::cout << "start push to tables " << " Txns here " << arg.size() << " :::"<<std::endl;
    std::string value = keystore_encode(0, 0);
 	int i=0;
 	for (auto &item: arg){
 	  item->str_key = hopman_fast::itostr(item->k);
 	}
	for (auto &item: arg) {
//      std::cout << "count = " << i << std::endl;
//      std::cout << "DEBUG cid= " << item->cid << " key= "<<item->str_key << " value= "<<item->v << " table_id= "<< item->table_id << std::endl;
      i++;
	  try {
	    if(item->delete_true){
	      // if delete flag is tru, its okay to loose the value being inserted
	      // we will push the sentinel instead of original value to enable garbage collection
	      std::string encoded = keystore_encode(item->cid, 999);
	      h[item->table_id].put (item->str_key, encoded, cmpFunc);
	    } else{
	      // first time and update
	      std::string encoded = keystore_encode (item->cid, item->v);
	      h[item->table_id].put (item->str_key, encoded, cmpFunc);
	    }
	    txns_count_this = item->insert_true;
	  } catch (...) {
	    std::cout << "excpetion" << std::endl;
	  }
//	  std::cout << "----------------------------" << std::endl;
	}
//	std::cout << "----------------------------" << std::endl;
//	std::cout << "--------- FINISHED ---------" << std::endl;
//	std::cout << "----------------------------" << std::endl;
	return true;
}

void keystore_encode3_v2(std::string& s, unsigned long long int x) {
    memcpy ((void *)(s.data() + s.length() - LEN_ULL), &x, LEN_ULL);
}

inline unsigned long long int keystore_decode3_v2(const std::string& s){
    unsigned long long int cid=0;
    memcpy (&cid, (void *) (s.data () + s.length() - LEN_ULL), sizeof (unsigned long long int));
    return cid;
}

bool cmpFunc2_v2(const std::string& newValue,const std::string& oldValue)
{
    unsigned long long int commit_id_new = keystore_decode3_v2(newValue);
    unsigned long long int commit_id_old = keystore_decode3_v2(oldValue);

    return commit_id_new > commit_id_old;
}

// 05/05/2021 weihshen
size_t getFileContentNew_OneLogOptimized_mbta_v2(char *buffer, unsigned long long int cid,
                                                 unsigned short int count,
                                                 unsigned int len,
                                                 abstract_db* db) {
    size_t put_ops = 0;

    unsigned short int *len_of_K=0, *len_of_V=0, *table_id=0;
    size_t offset=0;
    bool delete_true = false;
    for(int i=0;i<count;i++) {
        delete_true = false ;
        // 1. len of K
        len_of_K = reinterpret_cast<unsigned short int*>(buffer + offset);
        offset += sizeof(unsigned short int) ;

        // 2. content of K
        std::string key = std::string(buffer + offset, *len_of_K) ;
        offset += *len_of_K;

        // 3. len of V
        len_of_V = reinterpret_cast<unsigned short int*>(buffer + offset);
        offset += sizeof(unsigned short int) ;

        // 4. content of V, add an extra 8 bytes
        std::string value = std::string(buffer + offset, *len_of_V + 8) ;
        offset += *len_of_V;

        // 5. table id
        table_id = reinterpret_cast<unsigned short*>(buffer + offset);
        offset += sizeof(unsigned short int) ;

        if((*table_id) & (1 << 15)) {
            delete_true = true;
            *table_id = (*table_id) ^ (1 << 15);
        }

        //printf("K: %d, V: %d\n", *len_of_K, *len_of_V);
        if (*table_id == 0 || *table_id > 20000 || *table_id < 10000) {
            std::cout << "table_id[ThreadPool.cc]: " << *table_id << std::endl;
            exit(1);
        }

        if (delete_true) {  // FLAGXXX,
            value = "01234567" ;  // DELETE + 8 bytes for cid
        }

        // 6. encode value + cid
        keystore_encode3_v2 (value, cid);

        // 7. put it into actual tables
        int try_cnt = 1 ;
        while (1) {
            try {
                void *txn = db->new_txn(0, arena, buf, abstract_db::HINT_DEFAULT);
                abstract_ordered_index *table_index = db->open_index(*table_id) ;
                table_index->put_mbta(txn, key, cmpFunc2_v2, value);
                auto ret = db->commit_txn_no_paxos(txn);
                if (try_cnt > 1) {
                    std::cout << "succeed at retry#:" << try_cnt << std::endl;
                }
                break ;
            } catch (...) {   // if abort happens, replay it until it succeeds
                std::cout << "exception, retry#:" << try_cnt << std::endl;
                try_cnt += 1 ;
            }
        }
        put_ops ++;
    }

    return put_ops;
}

#ifndef MASS_DIRECT
bool getFileContentNew_t(const std::vector<one_container*>& arg, new_ds &h, std::atomic<int> &success_counts,
	std::atomic<int> &insert_stats_count, std::atomic<int> &delete_stats_count,std::atomic<int> &failed)
{
//	std::cout << "start push to tables"<<std::endl;

    std::string value = keystore_encode(0, 0);
//	#ifdef TEST_REPLAY_SPEED_FEAT
//  		auto logger = new OutputDataSerializer(GetNextThreadID(),
//  		REPLAY_SAVER_PATH +std::to_string(arg->run_thread_count())+"/",
//  		true);
//	#endif
// 	std::cout << "Processing for Table Chunk ID: " << arg->GetTableID() << " Item Count : "<< arg->GetContainerCounts() << std::endl;
 	int i=0;
 	for (auto &item: arg){
 	  item->str_key = hopman_fast::itostr(item->k);
	  nontrans_get(h,item->str_key,item->table_id,item->str_value,item->exists);
 	}
	for (auto &item: arg) {
//	  #ifdef TEST_REPLAY_SPEED_FEAT
//	  logger->BloopLog ();
//	  #endif
//      std::cout << "Processing ID= " << i << std::endl;
      i++;
//	  item->str_key = hopman_fast::itostr(item->k);
//	  nontrans_get(h,item->str_key,item->table_id,item->str_value,item->exists);
	  auto retrieved_value = decode(item->str_value);

	  try {
		std::string encoded = keystore_encode(item->cid, item->v);
				NO_PAXOS_TRANSACTION
				{
				  if(item->insert_true) {
//				    value = item->str_value;
					if (item->exists) {
//					  auto retrieved_value = decode(value);
//					  auto retrieved_value_aux = decode(item->str_value);
//
//					  if(retrieved_value.k2 == retrieved_value_aux.k2){
//					    std::cout << "=> "<<retrieved_value.k2<<" : "<< retrieved_value_aux.k2 << std::endl;
//					  }

					  unsigned long old_cid = retrieved_value.k1;
					  unsigned long old_value = retrieved_value.k2;
					  if(old_value == 999){
					    // delete this
//					    h[table_id].transDelete (std::to_string (k));
//					    cout << "@,";
					  }else if (old_cid <= item->cid) {
						h[item->table_id].transPut (item->str_key, encoded);
					  }
					} else {
//					  std::cout << "before transPut" << std::endl;
					  h[item->table_id].transPut (item->str_key, encoded);
//					  insert_stats_count++;
//					  std::cout << "after transPut" << std::endl;
					} // insert end
				  }else if(item->delete_true){
//				    delete_stats_count++;
				    if (item->exists) {
//				      auto retrieved_value = decode(value);
					  unsigned long old_cid = retrieved_value.k1;
					  if (old_cid <= item->cid) {
						h[item->table_id].transDelete (item->str_key);
					  }
				    }else{
				      // TODO delete comes first <-- write the concrete steps for delayed delete here
				      encoded = keystore_encode(item->cid, 999);
				      h[item->table_id].transPut (item->str_key, encoded);
				    }
				  } // delete end
//					success_counts++;
				}
				NO_PAXOS_RETRY(true);
	  } catch (...) {
		// don't count this
	  }
//	  std::cout << "gone" << std::endl;
	}
//	#ifdef TEST_REPLAY_SPEED_FEAT
//	logger->flush (true);
//	#endif
        return true;
}

bool getFileContentNew(const container_hello* arg, new_ds &h, std::atomic<int> &success_counts,
	std::atomic<int> &insert_stats_count, std::atomic<int> &delete_stats_count,std::atomic<int> &failed)
{
  	unsigned long long int k =0;
    unsigned long long int v = 0;
    unsigned long long int table_id=0;
    unsigned long long int insert_true=0;
    unsigned long long int delete_true=0;
    unsigned long long int cid=0;
//	std::cout << "start push to tables"<<std::endl;

    std::string value = "0:0";
//	#ifdef TEST_REPLAY_SPEED_FEAT
//  		auto logger = new OutputDataSerializer(GetNextThreadID(),
//  		REPLAY_SAVER_PATH +std::to_string(arg->run_thread_count())+"/",
//  		true);
//	#endif
// 	std::cout << "Processing for Table Chunk ID: " << arg->GetTableID() << " Item Count : "<< arg->GetContainerCounts() << std::endl;
 	int i=0;
	for (auto &item: arg->GetItems()) {
//	  #ifdef TEST_REPLAY_SPEED_FEAT
//	  logger->BloopLog ();
//	  #endif
//      std::cout << "Processing ID= " << i << std::endl;
      i++;
	  cid = item->cid;
	  k = item->k;
	  v = item->v;
	  table_id = item->table_id;
	  delete_true=item->delete_true;
	  insert_true=item->insert_true;
	  try {
		std::string encoded = Encode (std::to_string (cid), std::to_string (v));
		try {
		  NO_PAXOS_TRANSACTION
				{
				  if(insert_true) {
				    value = "0:0";
					if (h[table_id].transGet (std::to_string(k), value)) {
					  // already exists
					  auto retrieved_value = GetDecoded(value);
					  unsigned long old_cid = std::stoul (retrieved_value[0]);
					  unsigned long old_value = std::stoul (retrieved_value[1]);
					  if(old_value == 999){
					    // SKIP
					  }else if (old_cid <= cid) {
					    // UPDATE
						h[table_id].transPut (std::to_string(k), encoded);
					  }
					} else {
					  // first insert
					  h[table_id].transPut (std::to_string(k), encoded);
					} // insert end
				  }else if(delete_true){
//				    delete_stats_count++;
				    value = "0:0";
				    encoded = Encode (std::to_string (cid), std::to_string (999));
				    h[table_id].transPut (std::to_string (k), encoded);
				  } // delete end
//					success_counts++;
				}
		  NO_PAXOS_RETRY(true);
		} catch (Transaction::Abort E) {
//			failed++;
		}
	  } catch (...) {
		// don't count this
	  }
//	  std::cout << "gone" << std::endl;
	}
//	#ifdef TEST_REPLAY_SPEED_FEAT
//	logger->flush (true);
//	#endif
	std::cout << "finished replay job count:" << i <<std::endl;
	return true;
}

bool getFileContent(const container_hello* arg, ds &h, std::atomic<int> &success_counts,
	std::atomic<int> &insert_stats_count, std::atomic<int> &delete_stats_count,std::atomic<int> &failed)
{
  	unsigned long long int k =0;
    unsigned long long int v = 0;
    unsigned long long int table_id=0;
    unsigned long long int insert_true=0;
    unsigned long long int delete_true=0;
    unsigned long long int cid=0;
//	std::cout << "start push to tables"<<std::endl;

    std::string value = "0:0";
//	#ifdef TEST_REPLAY_SPEED_FEAT
//  		auto logger = new OutputDataSerializer(GetNextThreadID(),
//  		REPLAY_SAVER_PATH +std::to_string(arg->run_thread_count())+"/",
//  		true);
//	#endif
// 	std::cout << "Processing for Table Chunk ID: " << arg->GetTableID() << " Item Count : "<< arg->GetContainerCounts() << std::endl;
 	int i=1;
	for (auto &item: arg->GetItems()) {
//	  #ifdef TEST_REPLAY_SPEED_FEAT
//	  logger->BloopLog ();
//	  #endif
//      std::cout << "Processing ID= " << i << std::endl;
      i++;
	  cid = item->cid;
	  k = item->k;
	  v = item->v;
	  table_id = item->table_id;
	  delete_true=item->delete_true;
	  insert_true=item->insert_true;
	  try {
		std::string encoded = Encode (std::to_string (cid), std::to_string (v));
		try {
		  NO_PAXOS_TRANSACTION
				{
				  if(insert_true) {
				    value = "0:0";
					if (h[table_id]->transGet (std::to_string(k), value)) {
					  auto retrieved_value = GetDecoded(value);
					  unsigned long old_cid = std::stoul (retrieved_value[0]);
					  unsigned long old_value = std::stoul (retrieved_value[1]);
					  if(old_value == 999){
					    // delete this
//					    h[table_id].transDelete (std::to_string (k));
//					    cout << "@,";
					  }else if (old_cid <= cid) {
						h[table_id]->transPut (std::to_string(k), encoded);
					  }
					} else {
//					  std::cout << "before transPut" << std::endl;
					  h[table_id]->transPut (std::to_string(k), encoded);
//					  insert_stats_count++;
//					  std::cout << "after transPut" << std::endl;
					} // insert end
				  }else if(delete_true){
//				    delete_stats_count++;
				    value = "0:0";
				    if (h[table_id]->transGet (std::to_string (k), value)) {
				      auto retrieved_value = GetDecoded(value);
					  unsigned long old_cid = std::stoul (retrieved_value[0]);
					  if (old_cid <= cid) {
						h[table_id]->transDelete (std::to_string(k));
					  }
				    }else{
				      // TODO delete comes first <-- write the concrete steps for delayed delete here
				      encoded = Encode (std::to_string (cid), std::to_string (999));
				      h[table_id]->transPut (std::to_string (k), encoded);
				    }
				  } // delete end
//					success_counts++;
				}
		  NO_PAXOS_RETRY(true);
		} catch (Transaction::Abort E) {
//			failed++;
		}
	  } catch (...) {
		// don't count this
	  }
//	  std::cout << "gone" << std::endl;
	}
//	#ifdef TEST_REPLAY_SPEED_FEAT
//	logger->flush (true);
//	#endif
//	std::cout << "finished push to tables"<<std::endl;
	return true;
}

bool MassTreeWrapper::do_insert_op(const container_hello* arg, std::atomic<int> &success_counts,
  std::atomic<int> &insert_stats_count, std::atomic<int> &delete_stats_count,std::atomic<int> &failed) {
	  unsigned long long int k =0;
  unsigned long long int v = 0;
//  unsigned long long int table_id=0;
  unsigned long long int insert_true=0;
  unsigned long long int delete_true=0;
  unsigned long long int cid=0;
//	std::cout << "start push to tables"<<std::endl;

  std::string value = "0:0";
//	#ifdef TEST_REPLAY_SPEED_FEAT
//  		auto logger = new OutputDataSerializer(GetNextThreadID(),
//  		REPLAY_SAVER_PATH +std::to_string(arg->run_thread_count())+"/",
//  		true);
//	#endif
//  std::cout << "Processing for Table Chunk ID: " << arg->GetTableID() << " Item Count : "<< arg->GetContainerCounts() << std::endl;
  for (auto &item: arg->GetItems()) {
//	  #ifdef TEST_REPLAY_SPEED_FEAT
//	  logger->BloopLog ();
//	  #endif
//      std::cout << "Processing ID= " << i << std::endl;
	cid = item->cid;
	k = item->k;
	v = item->v;
//	table_id = item->table_id;
	delete_true=item->delete_true;
	insert_true=item->insert_true;
//	std::cout << "[DB-DEBUG] " << cid << ":" << k << ":" << v << ":" << table_id << ":" << delete_true << ":" << insert_true << ":" << std::endl;
	try {
	  std::string encoded = Encode (std::to_string (cid), std::to_string (v));
	  try {
		NO_PAXOS_TRANSACTION
			  {
				if(insert_true) {
				  value = "0:0";
				  if (h.transGet (std::to_string(k), value)) {
					auto retrieved_value = GetDecoded(value);
					unsigned long old_cid = std::stoul (retrieved_value[0]);
					unsigned long old_value = std::stoul (retrieved_value[1]);
					if(old_value == 999){
					  // delete this
//					    h[table_id].transDelete (std::to_string (k));
//					    cout << "@,";
					}else if (old_cid <= cid) {
					  h.transPut (std::to_string(k), encoded);
					}
				  } else {
//					  std::cout << "before transPut" << std::endl;
					h.transPut (std::to_string(k), encoded);
					insert_stats_count++;
//					  std::cout << "after transPut" << std::endl;
				  } // insert end
				}else if(delete_true){
				  delete_stats_count++;
				  value = "0:0";
				  if (h.transGet (std::to_string (k), value)) {
					auto retrieved_value = GetDecoded(value);
					unsigned long old_cid = std::stoul (retrieved_value[0]);
					if (old_cid <= cid) {
					  h.transDelete (std::to_string(k));
					}
				  }else{
					// TODO delete comes first <-- write the concrete steps for delayed delete here
					encoded = Encode (std::to_string (cid), std::to_string (999));
					h.transPut (std::to_string (k), encoded);
				  }
				} // delete end
				  success_counts++;
			  }
		NO_PAXOS_RETRY(true);
	  } catch (Transaction::Abort E) {
		  failed++;
	  }
	} catch (...) {
	  // don't count this
	}
//	  std::cout << "gone" << std::endl;
  }
//	#ifdef TEST_REPLAY_SPEED_FEAT
//	logger->flush (true);
//	#endif
//  std::cout << "finished push to tables"<<std::endl;
  return true;
}

void MassTreeWrapper::insert(const container_hello *container, int wrapper_id){
  //TODO micro stats per job
  std::atomic<int> fail_counts{0};
  std::atomic<int> success_counts{0};
  std::atomic<int> inserts{0};
  std::atomic<int> deletes{0};
  std::atomic<int> failed{0};
  
  TThread::set_id(container->GetThreadID ());
  Sto::update_threadid();
  actual_ds::thread_init();
	
  do_insert_op(container, success_counts, inserts, deletes, failed);
}
MassTreeWrapper::MassTreeWrapper (actual_ds& ref_, int ID) {
  this->wrapperID = ID;
  this->h = ref_;
  once_init = false;
}
#endif

std::queue<std::tuple<const container_hello*,int>> Q;
#ifndef MASS_DIRECT
void IndependentThread::independentWork(void* data, int id)
  {
    while(!finish_all) {
      std::lock_guard<std::mutex> lock(m_);
      if(Q.empty ()){
//        std::this_thread::sleep_for (std::chrono::nanoseconds (1));
		continue;
      }
      auto d = Q.front ();
      Q.pop ();
	  std::map<int,MassTreeWrapper*> ds = static_cast<IndependentThread *> (data)->masstree_objs;
	  int masstree_id = std::get<1> (d);
	  ds[masstree_id]->insert ( std::get<0> (d), masstree_id);
//	  int random_sleep = 1;//rand()%(1200-1 + 1) + 1;
//	  std::cout << "[id=" << id <<"," << ds[masstree_id]->getID() << "] " << " Key= " << std::get<0> (d) << "  Val= " << std::get<1> (d) << " || " << std::endl;
//      std::cout << id << ":"<< masstree_id << ",";
//	  std::this_thread::sleep_for (std::chrono::nanoseconds (random_sleep));
//	  std::cout << "[id= " << id << "] Exiting concurrent thread.\n";
	}
  }

void IndependentThread::add_work(const container_hello* container,int mass_id){
    std::lock_guard<std::mutex> lock(m_);
    Q.push (std::make_tuple(container,mass_id));
}

void* IndependentThread::wait_for_finish (void *) {
  	while (!Q.empty () || !finish_all){
        std::this_thread::sleep_for (std::chrono::nanoseconds (1));
  	}
  	std::cout << "wait_for_finish [done]" << std::endl;
    return NULL;
}

void IndependentThread::start_all(const std::map<int,MassTreeWrapper*>& objs){
  this->masstree_objs = objs;
  for (int i = 0; i < thread_count; ++i) {
	this->threadCaller(i);
  }
}

void IndependentThread::threadCaller(int t_id)
{
	worker_threads[t_id] = std::thread(IndependentThread::independentWork, (void *) this, t_id);
	worker_threads[t_id].detach();
}

IndependentThread::IndependentThread (int t_count) {
	thread_count = t_count;
	masstree_objs = {};
	worker_threads.resize(thread_count);
}

std::mutex IndependentThread::m_;
bool IndependentThread::finish_all = false;

void add_new_work(IndependentThread* tt,const container_hello* con,int mass_id){
  	tt->add_work(con,mass_id);
}
#endif
int getRandomNumber(int TOTAL_MASS_TREE_COUNT){
	int max_ = TOTAL_MASS_TREE_COUNT;
	int min_ = 1;
	return rand() % (max_- min_+1) + min_;
}

