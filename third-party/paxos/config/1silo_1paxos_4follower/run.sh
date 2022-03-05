git checkout .
sed -i "s/localhost: 127.0.0.1/localhost: 10.1.0.7/g" `grep "localhost: 127.0.0.1" -rl ./*.yml`
sed -i "s/p1: 127.0.0.1/p1: 10.1.0.8/g" `grep "p1: 127.0.0.1" -rl ./*.yml`
sed -i "s/p2: 127.0.0.1/p2: 10.1.0.9/g" `grep "p2: 127.0.0.1" -rl ./*.yml`
sed -i "s/p3: 127.0.0.1/p3: 10.1.0.72/g" `grep "p3: 127.0.0.1" -rl ./*.yml`
sed -i "s/p4: 127.0.0.1/p4: 10.1.0.73/g" `grep "p4: 127.0.0.1" -rl ./*.yml`
