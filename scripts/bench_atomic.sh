g++ bench_atomic.cpp -g -lpthread -Wall -O0 -o bench_atomic 
echo "complete compiling..."
echo ""

for i in {1..31}
do
  ./bench_atomic $i "fetch"
  echo ""
done

echo "XXXXX\n"

for i in {1..31}
do
  ./bench_atomic $i "rdtscp"
  echo ""
done


