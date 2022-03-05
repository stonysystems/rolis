//
// Created by mrityunjay kumar on 1/15/21.
//
#include <iostream>
#include <cstring>
#include <random>
#include <memory>

using namespace std;

#define PAXOS_LIB_ENABLED 1
#define MAX_ARRAY_SIZE_IN_BYTES  size_t(40980)*size_t(4098)*sizeof(char)

size_t ul_len = sizeof (unsigned long long int);

struct PUnit{
  unsigned long long int cid;
  int length;
  int tsize;
  const unsigned char *start;
};

int getRandom(int start=1,int endNumber=100)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist6(start,endNumber); // distribution in range [1, 6]

    int k =  dist6(rng);
  	return k;
}

void generate(unsigned char** data,unsigned long long int& cid, int &len,size_t& w, int iter=20)
{
  auto *array = *data;
  cid = getRandom(10000,99999);
  unsigned long long int k, v;
  unsigned short int table_id=0; // 2 bytes
  
  len = iter*(2*ul_len+sizeof(unsigned short));  // iter * 18
  
  for(int i=0;i<iter;i++){
    // copy 3 data per iter
    // key
    k = getRandom (100,3000);
	memcpy (array + w, (char *) &k, sizeof (k));
	w += sizeof (k);
	// value
	v = getRandom (3000,9000);
	memcpy (array + w, (char *) &v, sizeof (v));
	w += sizeof (v);
	// table_id
	table_id = getRandom (10001,10070);
//	std::cout << "Iter ::: " << i << " :=> ";
//	std::cout << "Key= " << k << " Val= " << v << " cid= " << cid;;
//	std::cout << " Table ID= " << table_id;
//    std::cout << " Del " ;
	if (getRandom (0,1)==0) {
		// 1 << ((sizeof(unsigned short)*8)-1) = 1 << 15
		table_id = table_id | (1 << 15);
//		std::cout << "True ";
	}else{
//	  	std::cout << "False ";
	}
//	std::cout << "\n";
	memcpy (array + w, (char *) &table_id, sizeof(table_id));
	w += sizeof(table_id);
  }
//  std::cout << "==============================================\n";
}

void printstats(const unsigned char *arr, unsigned long long int cid, int len, size_t& w, int iter)
{
  	char *buffer = (char *)arr;
  	unsigned long long int k,v;
	unsigned short int table_id=0;
	bool delete_true;
	size_t CW=0;
	int jjj=0;
	for(int i=0;i<iter;i++){
	   jjj++;
	   delete_true = false;
	   memcpy (&k, buffer+CW, sizeof (k));
	   w += sizeof (k);
	   CW += sizeof (k);

	   memcpy (&v, buffer+CW, sizeof (v));
	   w += sizeof (v);
	   CW += sizeof (v);

	   memcpy (&table_id, buffer+CW, sizeof (table_id));
	   w += sizeof (table_id);
	   CW += sizeof (table_id);
	   
//	   std::cout << "Iter ::: " << i << " :=> ";
//	   std::cout << "Key= " << k << " Val= " << v << " cid= " << cid;
	   if((table_id) & (1 << 15)){
          // check if MSB is set
          delete_true = true;
        // clear the bit
          table_id = (table_id) ^ (1 << 15);
	   }
//	   std::cout << " Table ID= " << table_id;
//	   std::cout << " Del " ;
	   if(delete_true){
//	     std::cout << "True ";
	   }else{
//	     std::cout << "False ";
	   }
//	   std::cout << "\n";
	}
}
void decode(unsigned char* buffer,size_t len)
{
//  	std::cout << "==============================================\n";
//  	std::cout << "==============================================\n";
//  	std::cout << "==============================================\n";
 
	int vector_count=0;
	size_t w=0;
	size_t dataLen;
	std::vector<PUnit> _vector;
	
	memcpy (&vector_count, buffer+w, sizeof (int));
	w += sizeof (int);
	_vector.resize (vector_count);
	
	memcpy (&dataLen, buffer+w, sizeof (dataLen));
	w += sizeof (dataLen);
	
	memcpy (_vector.data (), buffer+w, vector_count* sizeof (PUnit));
	w += vector_count* sizeof (PUnit);
	
	for(auto & _t : _vector){
//	  std::cout << "==============================================\n";
	  printstats(_t.start,_t.cid, _t.length, w, _t.tsize);
//	  std::cout << "==============================================\n";
	}

	
	std::cout << "==============================================\n";
	if(w==len){
	  std::cout << "Safe check [Pass]" << std::endl;
	}else{
	  std::cout << "Safe check [Fail]" << std::endl;
	}
	std::cout << "==============================================\n";
//  	std::cout << "==============================================\n";
//  	std::cout << "==============================================\n";
}

class SiloBatching{
 private:
  unsigned char *LOG;
  size_t kSizeLimit;
  std::vector<PUnit> _prequalVector;
  size_t entries;
  size_t curr_pos;
  
 public:
  explicit SiloBatching(size_t batchLimitSize){
  	LOG = (unsigned char *) malloc (MAX_ARRAY_SIZE_IN_BYTES);
    memset(LOG,'\0',MAX_ARRAY_SIZE_IN_BYTES);
    curr_pos = 0;
    entries = 0;
    kSizeLimit = batchLimitSize;
  }
  
  bool checkPushRequired(){
	return entries>=kSizeLimit;
  }
  
  void pushIfNeeded(bool force=false)
  {
    if(this->checkPushRequired() || force) {
	  // drain and push
	  size_t pos = 0;
	  unsigned char *queueLog = this->getBatchedLog (pos);
	  #if defined(PAXOS_LIB_ENABLED)
	  if(pos!=0) {
	      std::cout << "==============================================\n";

		std::cout << "New buffer with len= " << pos << " pushed to paxos\n" << std::endl;
		decode(queueLog, pos);
	  }
      #endif
      free(queueLog) ;  // FIXME
	  this->resetMemory();
	}
  }
  
  void resetMemory(){
    curr_pos = 0;
    entries = 0;
  	_prequalVector.clear ();
  	memset(LOG,'\0',MAX_ARRAY_SIZE_IN_BYTES);
  }
  
  unsigned char *getBatchedLog(size_t &pos){
    // 1. overhead
    // 2. more memcpy
    // 3. more complicated to do replay loading logs from local files
    pos=0;
    int vector_count = this->_prequalVector.size ();
    if(vector_count==0)
	  return nullptr;
    int prequalPayloadLen = sizeof (PUnit)*vector_count;
    // FIXME
    unsigned char *ptr = (unsigned char *)malloc(sizeof (vector_count)+sizeof (curr_pos) +prequalPayloadLen+curr_pos);

    // copy number of commit IDS
	memcpy ((void *)(ptr+pos), &vector_count, sizeof (vector_count));
	pos+= sizeof (vector_count);
	
	// copy curr_pos
	memcpy ((void *)(ptr+pos), &curr_pos, sizeof (curr_pos));
	pos+= sizeof (curr_pos);
	
	// copy all data from vector
    memcpy ((void *)(ptr+pos), this->_prequalVector.data(), prequalPayloadLen);
    pos+= prequalPayloadLen;
    
    memcpy ((void *)(ptr+pos), LOG, curr_pos);
    pos+= curr_pos;


    return ptr;
  }
  
  unsigned char * getLogOnly(size_t& pos){
    pos = curr_pos;
	return LOG;
  }
  
  bool checkLimits(size_t newLogLen){
  	return (curr_pos + newLogLen) < MAX_ARRAY_SIZE_IN_BYTES;
  }

  void update_ptr(const unsigned long long int& _cid,
  	              int _len,const unsigned char *startAddr,
  	              const size_t& w,int _tsize)
  {
    // FIXME
    _prequalVector.emplace_back (PUnit{.cid=_cid, .length=_len, .tsize=_tsize, .start=startAddr});
    entries++;
    curr_pos=w;
    pushIfNeeded();
  }
};

void encode(){
    std::shared_ptr<SiloBatching> instance = std::shared_ptr<SiloBatching>(new SiloBatching(10));
	size_t w=0;
	unsigned char *array;
	for(int k=0;k< 1200;k++){
		// int iter = getRandom (50,70);
        int iter = 100;
		unsigned long long int _cid;
		unsigned char *newBuffer = instance->getLogOnly (w);
		int _len;
		const unsigned char* refCopyBuffer=(newBuffer+w);
		generate (&newBuffer, _cid, _len, w, iter);
		instance->update_ptr(_cid, _len, refCopyBuffer, w, iter);
		
		instance->pushIfNeeded();
	}
	instance->pushIfNeeded(true);
}

int main(int argc, char** argv)
{
	encode();
	return 0;
}