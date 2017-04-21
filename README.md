### Co-occurrence-probability

This repository aims at providing a high performing and flexible Co-occurrence/conditional  probability, by employing  Stanford open source Glove. In one word, you get one item's probability Co-occurrence/conditional automatically with the following example code:

#### Install

``` c++
make
cd concurID2item
c++ -o concur.bin main.cpp -lglog -lgflags -std=c++11 -pthread -O3 -Wall -g
cd GloVe-1.2
make
cp build/* ../
```

#### Example

```c++
./itemfreq.bin build -i test -min-count 1 -max-vocab 10 -window-size 15 -topk 10 -o test.out
# -i:输入文件
# -min-count:最小词频
# -max-vocab:N
# -window-size:检索窗宽
# -topk:输出条件概率前K个，default：all
# -o:输出文件
./concur.bin -id2word -in ../test.out -out test.words
#上一步输出文件为ID，如需查看具体的item则运行该步
```

