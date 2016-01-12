# Chinese_parser-deterministic_Nivre-dependency
deterministic Nivre algorithm based on long-distance dependency and SVM

【实验原理】</br>
在确定性Nivre算法中，解析器可以表示成一个三元组<S,I,A>。S和I是堆栈，I中是待解析的输入序列。A是一个集合，存放在解析过程中确定下来的依存关系项。假设给定一个输入序列W，解析器首先被初始化成<nil,W,∅>。解析器解析S的栈顶元素t和I的栈顶元素n的依存关系，然后采取相应的动作，操作栈中的元素移动。算法迭代进行直到I为空，此时输出保存在集合A中的依存关系序列。</br>
</br>
Nivre算法一共定义了四种操作，采用哪种操作是由SVM分类器决定的。</br>
>Right：在当前三元组<t|S,n|I,A>中，假如存在依存关系t→n，即t依存于n，则在A中添加项t→n，同时弹出S中的栈顶元素t，于是三元组变成<S,n|I,A∪{t→n}>。</br>
>Left：在当前三元组<t|S,n|I,A>中，假如存在依存关系n→t，即n依存于t，则在A中添加项n→t，同时把n压入栈S中，于是<n|t|S,I,A∪{n→t}>。</br>
>Reduce：假如n和t相互间没有依存关系，且t有父节点在其左侧，则弹出S中的栈顶元素t，于是三元组变成<S,n|I,A>。</br>
>Shift：上述三种情况都不满足时，把n压入栈S中，于是三元组变成<n|t|S,I,A>。</br>

考虑远距离依存关系的Nivre算法是基于依存关系不交叉的依存公理：如果A成分直接依存于B成分，而C成分在句子中位于A和B之间，那么C或者直接依存于A，或者直接依存于B，或者直接依存于A和B之间的某一成分。假如I的栈顶元素n不依存于t，并且t的左侧的父亲节点与n存在依存关系，则I中一定不存在任何元素依存于t，从栈中弹出t，不会影响其后的解析。</br>
考虑远距离依存关系的Nivre算法对Reduce和shift操作作了进一步明确的划分。</br>
>Reduce：假如n和t相互间没有依存关系，且t有父节点在其左侧，且父节点与n存在依存关系，则弹出S中的栈顶元素t，于是三元组变成<S,n|I,A>。</br>
>Shift：上述三种情况都不满足时，把n压入栈S中，于是三元组变成<n|t|S,I,A>。</br>

句长为N时，最多只需2N个动作就可以完成解析，时间复杂度为O(N)。</br>
</br>
基于SVM的Nivre算法是根据SVM分类器分类的结果，在Right,left,reduce,shift四个操作中选择相应的操作。对四个操作两两之间训练一个分类器，一共得到6个分类器。用训练模型对测试数据进行句法分析时，根据6个分类器投票的结果选择相应的操作。</br>
![](https://github.com/rindesu/Chinese_parser-deterministic_Nivre-dependency/tree/master/img/nivre.png)
 
【使用说明】</br>
train_model文件夹中的训练模型是用本项目利用哈工大树库中的前8000个句子作为训练数据得到的训练模型。直接利用该模型进行句法分析。</br>

输入：词性标注序列。</br>
输入格式为：词0/词性 词1/词性 词2/词性 ……如：抢险/v 救援/v 责无旁贷/i</br>
test_data.txt提供了哈工大树库最后2000个句子的词性标注结果。</br>

输出：依存句法分析结果。</br>
输出格式为：词0/词性/父节点位置 词1/词性/父节点位置 ……（句子第一个词的位置为0）如：抢险/v/2 救援/v/0 责无旁贷/i/-1</br>
输出结果中，一个句子可能有多个节点的父节点为-1，这是因为程序在初始化时将各个节点的父节点都赋值为-1。</br>
 
【项目实现】</br>
（1）选取特征，调用libsvm工具包对树库语料进行SVM训练（svm.h,svm.cpp）。读取树库时使用tinyxml解析器（tinystr.h,tinystr.cpp,tinyxml.h,tinyxml.cpp,
tinyxmlerror.cpp,tinyxmlparser.cpp）。训练模型保存在trainx_model中（x=1,2,3,
4,5,6）。
（2）用（1）中得到的训练模型对给定的词性标注序列进行句法分析，并输出结果。
![](https://github.com/rindesu/Chinese_parser-deterministic_Nivre-dependency/tree/master/img/project_frame.png)
	int BS(char * arr, int width, int s, int e, char* str)
在字符串数组arr中二分查找字符串str，起止下标分别为s和e
	void calcu_feature_vec()
计算特征向量
	void train_sent_parse()
训练数据（单句）parse
	void SVM_train()
训练分类器
	void test_sent_parse()
测试数据（单句）parse
	void document_parse()
parse一个xml测试文档

【特征选取与数字化】
	特征选取
选取如下9个特征组成特征向量，其中t-1,t-2分别指S栈顶第二和第三个元素，n+1,n+2分别指I栈顶第二和第三个元素。
节点t,t-1,t-2的词性
节点n,n+1,n+2的词性
节点t,n的词
节点t与n在句中的距离
	特征数字化
	根据哈工大树库所用标准，词性一共有28种情况。
	本项目将哈工大树库中的词存为一个词典，大小为24728。考虑到有些词不能在这个词典中找到，那么一个词的在词典中的位置一共有24729种情况。
	若栈S为空，则令节点t和n的距离为0，若t和n的距离大于29，则令其距离为29。那么t和n的距离一共有30种情况。
	将单个特征的可能的取值按大小排列（如果是非数字特征，则按字典序排列，并与数组下标做对应的数字转换）。然后将所有特征可能的取值按特征出现的顺序进行排列，得到一个28*6+24729*2+30维的特征向量。解析器的每一次操作，都对应这样一个特征向量。第i类操作的特征向量保存在testi.txt中（i=1,2,3,4）。

【训练及测试数据】
哈工大HIT树库中共有10000句中文句子。本项目将其中前8000句作为训练数据，后2000句作为测试数据。
采用以下评价标准：
Dependency Accuracy(DA,考察父节点的标注准确率)
本项目中，DA=0.769 。



【参考文献】
[1] 高玲玲, 基于SVM的确定性中文依存关系解析, 硕士学位论文
[2] 杨洋, 基于依存语法的汉语句法分析研究, 硕士学位论文

