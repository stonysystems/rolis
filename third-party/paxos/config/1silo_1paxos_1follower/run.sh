git checkout .
sed -i "s/localhost: 127.0.0.1/localhost: 10.1.0.7/g" `grep "localhost: 127.0.0.1" -rl ./*.yml`
sed -i "s/p1: 127.0.0.1/p1: 10.1.0.8/g" `grep "p1: 127.0.0.1" -rl ./*.yml`
