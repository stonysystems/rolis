//
// Created by mrityunjay kumar on 1/15/21.
//
#include <iostream>
#include <random>
#include <cstring>
#include <cassert>
#include <memory>
#include <x86intrin.h>

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
/*
 *
 #include <intrin.h>
#include <stdio.h>
int main()
{
    unsigned __int64 i;
    unsigned int ui;
    i = __rdtscp(&ui);
    printf_s("%I64d ticks\n", i);
    printf_s("TSC_AUX was %x\n", ui);
}
 */
unsigned char* generate(const unsigned char* array,unsigned long long int& cid, int &len,size_t& w, int iter=20)
{
//  cid = getRandom(10000,99999);
  unsigned int ui;
  cid = __rdtscp(&ui);
  unsigned long long int k, v;
  unsigned short int table_id=0; // 2 bytes
  
  std::cout << "==============================================\n";
  std::cout << "==============================================\n";
  std::cout << "Generate Phase\n";
//  len = iter*(2*ul_len+sizeof(unsigned short));
  
  for(int i=0;i<iter;i++){
    // copy 3 data per iter
    // key
    k = getRandom (100,3000);
	memcpy ((void *)(array + w), (char *) &k, sizeof (k));
	w += sizeof (k);
	len += sizeof (k);
	// value
	v = getRandom (3000,9000);
	memcpy ((void *)(array + w), (char *) &v, sizeof (v));
	w += sizeof (v);
	len += sizeof (v);
	// table_id
	table_id = getRandom (10001,10070);
	std::cout << "Iter ::: " << i << " :=> ";
	std::cout << "Key= " << k << " Val= " << v << " cid= " << cid;;
	std::cout << " Table ID= " << table_id;
    std::cout << " Del " ;
	if (getRandom (0,1)==0) {
		// 1 << ((sizeof(unsigned short)*8)-1) = 1 << 15
		table_id = table_id | (1 << 15);
		std::cout << "True ";
	}else{
	  	std::cout << "False ";
	}
	std::cout << "\n";
	memcpy ((void *)(array + w), (char *) &table_id, sizeof(table_id));
	w += sizeof(table_id);
	len += sizeof (table_id);
  }
  std::cout << "==============================================\n";
  return (unsigned char*)array;
}


void printstats(const unsigned char *arr, unsigned long long int cid, size_t len, int iter)
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
	   memcpy (&k, (void *)(buffer+CW), sizeof (k));
	   CW += sizeof (k);

	   memcpy (&v, (void *)(buffer+CW), sizeof (v));
	   CW += sizeof (v);

	   memcpy (&table_id, (void *)(buffer+CW), sizeof (table_id));
	   CW += sizeof (table_id);
	   
	   std::cout << "Iter ::: " << i << " :=> ";
	   std::cout << "Key= " << k << " Val= " << v << " cid= " << cid;
	   if((table_id) & (1 << 15)){
          // check if MSB is set
          delete_true = true;
        // clear the bit
          table_id = (table_id) ^ (1 << 15);
	   }
	   std::cout << " Table ID= " << table_id;
	   std::cout << " Del " ;
	   if(delete_true){
	     std::cout << "True ";
	   }else{
	     std::cout << "False ";
	   }
	   std::cout << "\n";
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
	
	// read the vector count
	memcpy (&vector_count, (void *)(buffer+w), sizeof (int));
	w += sizeof (int);
	_vector.resize (vector_count);
	
	// read the actual data payload length after vector
	memcpy (&dataLen, (void *)(buffer+w), sizeof (dataLen));
	w += sizeof (dataLen);
	
	// read the vectors
	memcpy (_vector.data (), (void *)(buffer+w), vector_count* sizeof (PUnit));
	w += (vector_count* sizeof (PUnit));
	
	
	for(auto &_t : _vector){
	  printstats(_t.start,_t.cid, _t.length, _t.tsize);
	  w += _t.length;
	}
	if(w==len){
	  std::cout << "Safe check [Pass]" << std::endl;
	}else{
	  std::cout << "Safe check [Fail]" << std::endl;
	}
}

class SiloBatching{
 private:
  unsigned char *LOG;
  size_t total_alloc_size;
  size_t _offset;
  size_t actual_len;
  
  size_t kSizeLimit;
  std::vector<PUnit> _prequalVector;
  size_t entries;
  size_t curr_pos;
  
 public:
  explicit SiloBatching(int batchLimitSize){
    int prequalPayloadLen = sizeof (PUnit)*batchLimitSize;
    _offset = sizeof (batchLimitSize) + sizeof (curr_pos) + prequalPayloadLen;
    total_alloc_size = _offset + MAX_ARRAY_SIZE_IN_BYTES;
    LOG = (unsigned char *) malloc (total_alloc_size);
    memset(LOG,'\0',total_alloc_size);
    curr_pos = _offset;
    entries = 0;
    actual_len = 0;
    kSizeLimit = batchLimitSize;
  }
  
  ~SiloBatching(){
    if(LOG){
      free (LOG);
      LOG = nullptr;
    }
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
	  this->resetMemory();
	}
  }
  
  void resetMemory(){
    curr_pos = _offset;
    entries = 0;
    actual_len = 0;
  	_prequalVector.clear ();
  	memset(LOG,'\0',total_alloc_size);
  }
  
  unsigned char *getBatchedLog(size_t &lpos){
    size_t pos = 0;
    lpos = 0;
	int vector_count = this->_prequalVector.size ();
	if (vector_count == 0)
	  return nullptr;
	int lag =0;
	int prequalPayloadLen = sizeof (PUnit) * vector_count;
	if(kSizeLimit != this->_prequalVector.size ())
	{
	  lag = _offset-prequalPayloadLen-sizeof (vector_count)-sizeof (actual_len);
	  pos = lag;
	}
	
	unsigned char *ptr = LOG;

	// current ptr is at 0th location
	// copy number of commit IDS->int
	memcpy ((void *) (ptr + pos), &vector_count, sizeof (vector_count));
	pos += sizeof (vector_count);
	lpos += sizeof (vector_count);

	// copy curr_pos->size_t
	memcpy ((void *) (ptr + pos), &actual_len, sizeof (actual_len));
	pos += sizeof (actual_len);
	lpos += sizeof (actual_len);

	// copy all data from vector
	memcpy ((void *) (ptr + pos), this->_prequalVector.data (), prequalPayloadLen);
	pos += prequalPayloadLen;
	lpos += prequalPayloadLen;
	assert (pos == _offset);

	memcpy ((void *) (ptr + pos), LOG + _offset, actual_len);
	pos += actual_len;
	lpos += actual_len;
	
	assert(pos == curr_pos);
	return ptr+lag;
  }
  
  // always return start of the address
  unsigned char * getLogOnly(size_t& pos){
    pos = curr_pos;
	return LOG;
  }
  
  bool checkIfOutofMemLimits(size_t newLogLen){
  	return (curr_pos + newLogLen) >= total_alloc_size;
  }

  void update_ptr(const unsigned long long int& _cid,
  	              int _len,const unsigned char **startAddr, int _tsize)
  {
    bool force = this->checkIfOutofMemLimits(_len);
    pushIfNeeded(force);
    _prequalVector.emplace_back (PUnit{.cid=_cid, .length=_len,.start=*startAddr,.tsize=_tsize});
    entries++;
    curr_pos += _len;
    actual_len += _len;
  }
};

void test_encode(){
    std::shared_ptr<SiloBatching> instance = std::shared_ptr<SiloBatching>(new SiloBatching(10));
	size_t w=0;
	unsigned char *array;
	for(size_t k=0;k< 12;k++){
		int iter = getRandom (50,70);
		unsigned long long int _cid;
		unsigned char *orgBuffer = instance->getLogOnly (w);
		int _len=0;
		const unsigned char* refCopyBuffer=orgBuffer+w;
		orgBuffer = generate (orgBuffer, _cid, _len, w, iter);
		assert (refCopyBuffer == orgBuffer+w-_len);
		instance->update_ptr(_cid, _len, &refCopyBuffer, iter);
//		std::cout << ".";
		instance->pushIfNeeded();
	}
	instance->pushIfNeeded(true);
}

void getDecoded(std::vector<PUnit>& _vector,
	int& vector_count, unsigned char *buffer,size_t &w,size_t& dataLen)
{
  
  memcpy (&vector_count, buffer+w, sizeof (int));
  w += sizeof (int);
  _vector.resize (vector_count);
  
  memcpy (&dataLen, buffer+w, sizeof (dataLen));
  w += sizeof (dataLen);
  
  memcpy (_vector.data (), buffer+w, (vector_count* sizeof (PUnit)));
  w += (vector_count* sizeof (PUnit));
}

unsigned char* merge(int& total_len,
	       unsigned char* buffer1, int len_1,
	       unsigned char* buffer2, int len_2)
{
  // PART 1
  int vector_count1=0;
  size_t w1=0;
  size_t dataLen1;
  std::vector<PUnit> _vectors1;
  getDecoded(_vectors1, vector_count1, buffer1, w1, dataLen1);
  
  // PART 2
  int vector_count2=0;
  size_t w2=0;
  size_t dataLen2;
  std::vector<PUnit> _vectors2;
  getDecoded(_vectors2, vector_count2, buffer2, w2, dataLen2);
  
  // merge
  int fVectorCount = vector_count1 + vector_count2;
  //a.insert(a.end(), b.begin(), b.end());
  _vectors1.insert( _vectors1.end(), _vectors2.begin(), _vectors2.end() );
  assert (fVectorCount == _vectors1.size());
  
  // start copying
  // alloc the memory and load
  size_t pos=0;
  int prequalPayloadLen = sizeof (PUnit)*fVectorCount;
  size_t payload_start_offset = sizeof (fVectorCount)+prequalPayloadLen+len_1+len_2;
  unsigned char *ptr = (unsigned char *)malloc(sizeof (int) + prequalPayloadLen + len_1+len_2 + 1);
  
  size_t fw = sizeof (fVectorCount)+ sizeof (payload_start_offset) + prequalPayloadLen;
  for(auto &each: _vectors1){
//    printstats(each.start,each.cid, each.length, each.tsize);
    each.start = ptr+fw;
    
    fw += each.length;
  }
  
  // read the vector count
  memcpy ((void *)(ptr+pos), &fVectorCount, sizeof (fVectorCount));
  pos+= sizeof (fVectorCount);
  
  // total data len except vector and vector count
  memcpy ((void *)(ptr+pos), &payload_start_offset, sizeof (payload_start_offset));
  pos+= sizeof (payload_start_offset);
  
  // copy all data from vector
  memcpy ((void *)(ptr+pos), _vectors1.data(), prequalPayloadLen);
  pos+= prequalPayloadLen;
  
  size_t offset = pos;
  
  memcpy ((void *)(ptr+pos), buffer1+w1, len_1);
//  printstats(ptr+pos,_vectors1[0].cid, _vectors1[0].length, _vectors1[0].tsize);
  
  pos+= len_1;
  offset += len_1;
  
  memcpy ((void *)(ptr+pos), buffer2+w2, len_2);
//  printstats(ptr+pos,_vectors1[1].cid, _vectors1[1].length, _vectors1[1].tsize);
  pos+= len_2;
  
  total_len = pos;
  return ptr;
}


void test_merge(){
  // part 1
  auto instance1 = new SiloBatching(1);
  size_t w1=0;
  int iter1 = getRandom (1,1);
  int _len1=0;
  unsigned long long int _cid1;
  unsigned char *orgBuffer1 = instance1->getLogOnly (w1);
  const unsigned char* refCopyBuffer=orgBuffer1+w1;
  orgBuffer1 = generate (orgBuffer1, _cid1, _len1, w1, iter1);
  assert (refCopyBuffer == orgBuffer1+w1-_len1);
  instance1->update_ptr(_cid1, _len1, &refCopyBuffer, iter1);
  
  
  size_t _dlen1=0;
  unsigned char *array_1 = instance1->getBatchedLog (_dlen1);
//  decode (array_1, _dlen1);
  
  // part 2
  auto instance2 = new SiloBatching(1);
  size_t w2=0;
  int iter2 = getRandom (1,1);
  int _len2=0;
  unsigned long long int _cid2;
  unsigned char *orgBuffer2 = instance2->getLogOnly (w2);
  const unsigned char* refCopyBuffer2=orgBuffer2+w2;
  orgBuffer2 = generate (orgBuffer2, _cid2, _len2, w2, iter2);
  assert (refCopyBuffer2 == orgBuffer2+w2-_len2);
  instance2->update_ptr(_cid2, _len2, &refCopyBuffer2, iter2);
  
  
  size_t _dlen2=0;
  unsigned char *array_2 = instance2->getBatchedLog (_dlen2);
//  decode (array_2, _dlen2);
  
  int mArrLen=0;
  unsigned char *mArr = merge (mArrLen, array_1, _len1, array_2, _len2);
  delete (instance1);
  delete (instance2);
  
  decode (mArr, mArrLen);
}

// 1. modify qmain => main
// 2. g++ EncDecode.cc -g -lpthread -Wall -O0 -o run
// 3. ./run
int main(int argc, char** argv)
{
	test_encode();
//	test_merge();
	return 0;
}