docker stop m1 m2 r1 r2
docker rm m1 m2 r1 r2

docker run -d -p 2001:27017 --name m1 --cpuset-cpus="0" --memory-reservation="2g" --memory="2g" --memory-swap="4g" kvmprashanth/mongodb-enterprise --dbpath /data/db
docker run -d -p 2002:27017 --name m2 --cpuset-cpus="1-2" --memory-reservation="4g" --memory="4g" --memory-swap="8g" kvmprashanth/mongodb-enterprise --dbpath /data/db

docker run --name r1 --cpuset-cpus="3" --memory-reservation="2g" --memory="2g" --memory-swap="4g" -p 3001:6379 -d redis
docker run --name r2 --cpuset-cpus="4-5" --memory-reservation="4g" --memory="4g" --memory-swap="8g" -p 3002:6379 -d redis

gnome-terminal -x bash -c './ycsb/bin/ycsb load redis -s -P ycsb/workloads/workload1 -p "redis.host=127.0.0.1" -p "redis.port=3001" -threads 3'
gnome-terminal -x bash -c './ycsb/bin/ycsb load redis -s -P ycsb/workloads/workload2 -p "redis.host=127.0.0.1" -p "redis.port=3002" -threads 5'

gnome-terminal -x bash -c './ycsb/bin/ycsb load mongodb -s -P ycsb/workloads/workload1 -p "mongodb.url=mongodb://localhost:2001/ycsb?w=1" -threads 3'

gnome-terminal -x bash -c './ycsb/bin/ycsb load mongodb -s -P ycsb/workloads/workload2 -p "mongodb.url=mongodb://localhost:2002/ycsb?w=1" -threads 5'

