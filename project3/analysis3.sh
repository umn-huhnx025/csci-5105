
make

for i in $(seq 1 6); do
  rm -rf AnalysisFolder/test$i
  mkdir AnalysisFolder/test$i
done

for i in $(seq 1 3); do
  cp -R test_files/lorem5000.txt AnalysisFolder/test$i
done

./tracker 9010 &
P1=$!
./peer 9001 127.0.0.1 9010 AnalysisFolder/test1 -t & 
P2=$!
./peer 9002 127.0.0.1 9010 AnalysisFolder/test2 -t &
P3=$!
./peer 9003 127.0.0.1 9010 AnalysisFolder/test3 -t & 
P4=$!
./peer 9005 127.0.0.1 9010 AnalysisFolder/test4 -r & 
P5=$!
./peer 9000 127.0.0.1 9010 AnalysisFolder/test5 -r & 
P6=$!
./peer 9006 127.0.0.1 9010 AnalysisFolder/test6 -r & 
P7=$!


wait $P1 $P2 $P3 $P4 $P5 $P6 $P7 
