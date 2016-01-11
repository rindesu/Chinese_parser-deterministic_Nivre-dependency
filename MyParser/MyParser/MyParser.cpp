// MyParser.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"

#include "tinyxml.h"
#include "tinystr.h"
#include "svm.h"

#include "dict.h"
#include <malloc.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

//����
char pos_name[28][3] = {"a", "b", "c", "d", "e", "g", "h", "i", "j", "k", "m", "n", "nd", "nh", "ni", "nl", "ns", "nt", "nz", "o", "p", "q", "r", "u", "v", "wp", "ws", "x"};
//ջS��I�еĴʽڵ�
class wordnode{
public:
	char str[25];//��
	int position;//��ԭ���е�λ�á����е�һ���ʵ�λ��Ϊ0.
	char pos[6];//����
	int parent_position;//���ڵ��λ��
	const wordnode &operator = (const wordnode &); 
	wordnode() {parent_position = -1;}
};
const wordnode & wordnode::operator =(const wordnode & n) {
	strcpy_s(str, n.str);
	strcpy_s(pos, n.pos);
	position = n.position;
	parent_position = n.parent_position;
	return *this;
}
wordnode S[100], inputSeq[100], comp[100];//ջS��I(inputSeq)��comp��������ͳ�Ʋ���������ȷ��
int S_len, inputSeq_len;//ջS��I�ĳ���
int feature_vec[21];//����������������28*6+24729*2+30ά���з�0��ά
int feature_num;//���������з�0��ά��
fstream ftest[4];//�������������Ӧ����������
int data[4];//ÿ�������Ӧ������������
int total_parent;//�ܸ��ڵ����������о��ӵĴʵ�����֮��
int correct_parent;//���ڵ��ע��ȷ�Ĵʵ�����
int model_num;//��������Ŀ
int cur;//I��ǰ������λ��

//svm���������
struct svm_problem prob;
struct svm_parameter param;//����������
struct svm_model * model[6];//ѵ���������õ���ģ��
struct svm_node * x_space;
struct svm_node ** _node;
double * y;//����ѵ��������������Ӧ�����

int BS(char * arr, int width, int s, int e, char* str) {//���ַ�������arr�ж��ֲ����ַ���str����ֹ�±�ֱ�Ϊs��e
	if (s == e - 1 && strcmp(arr + s * width, str) != 0 && strcmp(arr + e * width, str) != 0)
		return -1;//û�ҵ�
	if (strcmp(arr + s * width, str) == 0)
		return s;
	if (strcmp(arr + e * width, str) == 0)
		return e;
	int mid = (s + e) / 2;
	if (strcmp(arr + mid * width, str) == 0)
		return mid;
	if (strcmp(arr + mid * width, str) < 0)
		return BS(arr, width, mid, e, str);
	return BS(arr, width, s, mid, str);
}

void calcu_feature_vec() {//������������
	int tmp, j;
	memset(feature_vec, 0, sizeof(feature_vec));
	feature_num = 0;
	for (j = 0; j < 3; j++) {
		if (S_len - 1 < j) {
			feature_vec[feature_num++] = j * 28;
		}
		else {
			tmp = BS(*pos_name, 3, 0, 27, S[S_len - 1 - j].pos) + 1;
			feature_vec[feature_num++] = j * 28 + tmp;
		}
		if (cur + j > inputSeq_len - 1) {
			feature_vec[feature_num++] = (3 + j) * 28;
		}
		else {
			tmp = BS(*pos_name, 3, 0, 27, inputSeq[cur + j].pos) + 1;
			feature_vec[feature_num++] = (3 + j) * 28 + tmp;
		}
	}
	
	if (S_len == 0)//ջI��ջ��Ԫ�غ�ջS��ջ��Ԫ����ԭ���еľ��루λ��֮�
		feature_vec[feature_num++] = 6 * 28;
	else {
		tmp = abs(S[S_len - 1].position - inputSeq[cur].position); 
		if (tmp > 29)
			tmp = 29;
		feature_vec[feature_num++] = 6 * 28 + tmp;
	}

	if (S_len == 0)//ջI��ջ��Ԫ�صĴʣ���dict.h�е�words�����в���
		feature_vec[feature_num++] = 198;
	else {
		tmp = BS(*words, 25, 0, 24727, S[S_len - 1].str) + 1;
		feature_vec[feature_num++] = 198 + tmp;
	}

	if (cur > inputSeq_len - 1)//ջS��ջ��Ԫ�صĴʣ���dict.h�е�words�����в���
		feature_vec[feature_num++] = 198 + 24729;
	else {
		tmp = BS(*words, 25, 0, 24727, inputSeq[cur].str) + 1;
		feature_vec[feature_num++] = 198 + 24729 + tmp;
	}
}
void train_sent_parse() {//ѵ�����ݣ����䣩parse
	cur = 0;
	int label, j;
	while (cur < inputSeq_len) {//��Iδ������
		if (strcmp(inputSeq[cur].pos, "wp") == 0) {//�����ŵĸ��ڵ㸳ֵΪ-2
			inputSeq[cur].parent_position = -2;
			cur++;
			continue;
		}
		if (S_len == 0)
			label = 4;
		else {
			if (S[S_len - 1].position == inputSeq[cur].parent_position)//left����
				label = 2;
			else if (S[S_len - 1].parent_position == inputSeq[cur].position)//right����
				label = 1;
			else {
				label = 4;//shift����
				if (S_len >= 2) {
					for (j = S_len - 2; j >= 0; j--) {
						//reduce����
						if (S[j].position == S[S_len - 1].parent_position && (S[j].parent_position == inputSeq[cur].position || S[j].position == inputSeq[cur].parent_position)) {
							label = 3;
							break;
						}
					}
				}
			}
		}
		calcu_feature_vec();//������������
		//�����������з�0��ά����άд���Ӧ���ļ�
		ftest[label - 1] << feature_num;
		for (j = 0; j < feature_num; j++) {
			ftest[label - 1] << " " << feature_vec[j];
		}
		ftest[label - 1] << endl;
		data[label - 1]++;//��Ӧ��������������+1

		//����labelִ����Ӧ����
		switch (label) {
			case 1://right
				S_len--;
				break;
			case 2://left
				S[S_len++] = inputSeq[cur++];
				break;
			case 3://reduce
				S_len--;
				break;
			case 4://shift
				S[S_len++] = inputSeq[cur++];
		}
	}
}

void SVM_train() {//ѵ��������	
	ftest[0].open("test1");
	ftest[1].open("test2");
	ftest[2].open("test3");
	ftest[3].open("test4");
	data[0] = 0;
	data[1] = 0;
	data[2] = 0;
	data[3] = 0;
	TiXmlDocument *tb = new TiXmlDocument("HITChineseDependencyTreeBank_train.xml");//ѵ������
    tb->LoadFile();
	TiXmlElement *para = tb->RootElement(), *sent, *word;
	int parent, i, j, m, n;
	while (strcmp(para->Value(), "para") != 0)
		para = para->FirstChildElement();
	//������ѵ�����ݲ�parse
	for (sent = para->FirstChildElement(); sent; sent = sent->NextSiblingElement()) {
		inputSeq_len = 0;
		S_len = 0;
		for (word = sent->FirstChildElement(); word; word = word->NextSiblingElement()) {
			strcpy_s(inputSeq[inputSeq_len].str, word->Attribute("cont"));
			strcpy_s(inputSeq[inputSeq_len].pos, word->Attribute("pos"));
			inputSeq[inputSeq_len++].parent_position = atoi(word->Attribute("parent"));
		}
		train_sent_parse();
	}
	//����svm����������svm����ѵ��
	param.svm_type = C_SVC;
	param.kernel_type = RBF;//�˺���
	param.degree = 3;
	param.coef0 = 0;
	param.gamma = 0.18;//�˺�������
	param.cache_size = 40;//������С
	param.C = 1;//�ͷ�����
	param.eps = 1e-4;//ֹͣ����
	param.shrinking = 1;//ѵ������ʹ��ѹ��
	param.probability = 0;
	param.nu = 1;
	param.p = 0.5;
	//C-SVCȨ�����
	param.nr_weight = 2;
	param.weight_label = (int *)malloc(2 * sizeof(int));
	param.weight_label[0] = +1;
	param.weight_label[1] = -1;
	param.weight = (double *)malloc(2 * sizeof(double));
	
	int train_data_num;//��ǰ��������ѵ��������Ŀ������ǰ���������Ӧ������������Ŀ֮��
	char name[15], tmp[10];
	const char * error_msg;//��ʾ��Ϣ

	//�ֱ�ѵ��6��������
	for (m = 0; m < 4; m++) {
		for (n = m + 1; n < 4; n++) {
			//����C_SVCȨ��
			if (data[n] > data[m]) {
				param.weight[0] = 1;
				param.weight[1] = data[m] * 1.0 / data[n];
			}
			else {
				param.weight[0] = data[n] * 1.0 / data[m];
				param.weight[1] = 1;
			}

			train_data_num = data[m] + data[n];
			//Ϊѵ�����ݣ����������������ڴ�
			_node = (struct svm_node **)malloc(train_data_num * sizeof(svm_node *));
			y = (double *)malloc(train_data_num * sizeof(double));
			ftest[m].seekp(0);
			ftest[n].seekp(0);
			for (i = 0; i < data[m]; i++) {
				y[i] = +1;//��Ӧ���
				ftest[m] >> feature_num;
				x_space = (svm_node *)malloc((feature_num + 1) * sizeof(svm_node));
				for (j = 0; j < feature_num; j++) {
					ftest[m] >> x_space[j].index;
					x_space[j].value = 1;
				}
				x_space[feature_num].index = -1;
				_node[i] = x_space;
			}
			for (i = data[m]; i < train_data_num; i++) {
				y[i] = -1;//��Ӧ���
				ftest[n] >> feature_num;
				x_space = (svm_node *)malloc((feature_num + 1) * sizeof(svm_node));
				for (j = 0; j < feature_num; j++) {
					ftest[n] >> x_space[j].index;
					x_space[j].value = 1;
				}
				x_space[feature_num].index = -1;
				_node[i] = x_space;
			}
			prob.l = train_data_num;
			prob.x = _node;
			prob.y = y;
			error_msg = svm_check_parameter(&prob, &param);//�������Ƿ����libsvm������Ҫ���ʽ
			if (error_msg) {
				cout << "error" << endl;
				exit(1);
			}
			strcpy_s(name, "train_model");
			itoa(++model_num, tmp, 10);
			strcat(name, tmp);
			svm_save_model(name, svm_train(&prob, &param));//ѵ��������ģ��
			for (i = 0; i < train_data_num; i++)
				free(_node[i]);
			free(_node);
			free(y);
		}
	}
	free(param.weight_label);
	free(param.weight);
	ftest[0].close();
	ftest[1].close();
	ftest[2].close();
	ftest[3].close();
}

void test_sent_parse() {//�������ݣ����䣩parse
	cur = 0;
	int label, l[6], vote[4], max_vote, j;
	while (cur < inputSeq_len) {//��Iδ������
		memset(vote, 0, sizeof(vote));
		max_vote = -1;
		if (strcmp(inputSeq[cur].pos, "wp") == 0) {//�����ŵĸ��ڵ㸳ֵΪ-2
			inputSeq[cur].parent_position = -2;
			cur++;
			continue;
		}
		if (S_len == 0)
			label = 4;
		else {
			calcu_feature_vec();//������������
			//Ϊѵ�����ݣ����������������ڴ�
			x_space = (svm_node *)malloc((feature_num + 1) * sizeof(svm_node));
			for (j = 0; j < feature_num; j++) {
				x_space[j].index = feature_vec[j];
				x_space[j].value = 1;
			}
			x_space[feature_num].index = -1;

			//��6���������ֱ���з��ࡢͶƱ
			for (j = 0; j < 6; j++) {
				l[j] = int(svm_predict(model[j], x_space));
			}
			for (j = 0; j < 3; j++) {
				if (l[j] > 0)
					vote[0]++;
				else
					vote[j + 1]++;
			}
			for (j = 3; j < 5; j++) {
				if (l[j] > 0)
					vote[1]++;
				else
					vote[j - 1]++;
			}
			if (l[5] > 0)
				vote[2]++;
			else
				vote[3]++;
			for (j = 0; j < 4; j++) {
				if (max_vote < vote[j]) {
					max_vote = vote[j];
					label = j + 1;
				}
			}
			free(x_space);
		}
		//����SVM������ѡ�����
		switch (label) {
			case 1://right
				for (j = cur - 1; j >= 0; j--) {
					if (inputSeq[j].position == S[S_len - 1].position) {
						inputSeq[j].parent_position = inputSeq[cur].position;
						break;
					}
				}
				S[S_len - 1].parent_position = inputSeq[cur].position;
				S_len--;
				break;
			case 2://left
				inputSeq[cur].parent_position = S[S_len - 1].position;
				S[S_len++] = inputSeq[cur++];
				break;
			case 3://reduce
				S_len--;
				break;
			case 4://shift
				S[S_len++] = inputSeq[cur++];
		}
	}
	int len, i;
	for (i = 0; i < inputSeq_len; i++) {//������I����ж��Ƿ񽫸��ڵ��ע��ȷ
		if (inputSeq[i].parent_position == comp[i].parent_position)
			correct_parent++;
	}
	total_parent += inputSeq_len;
}



void document_parse() {	//parseһ��xml�����ĵ�
	TiXmlDocument *tb = new TiXmlDocument("HITChineseDependencyTreeBank_test.xml");//�����ĵ�
    tb->LoadFile();
	TiXmlElement *para = tb->RootElement(), *sent, *word;
	int i, j;
	while (strcmp(para->Value(), "para") != 0)
		para = para->FirstChildElement();
	for (sent = para->FirstChildElement(); sent; sent = sent->NextSiblingElement()) {//�����벢parse
		inputSeq_len = 0;
		S_len = 0;
		for (i = 0; i < 100; i++)
			inputSeq[i].parent_position = -1;
		for (word = sent->FirstChildElement(); word; word = word->NextSiblingElement()) {
			strcpy_s(inputSeq[inputSeq_len].str, word->Attribute("cont"));
			strcpy_s(inputSeq[inputSeq_len].pos, word->Attribute("pos"));
			comp[inputSeq_len++].parent_position = atoi(word->Attribute("parent"));
		}
		test_sent_parse();
	}
	cout << "correct rate is: " << correct_parent * 1.0 / (total_parent * 1.0) << endl;
}

int main() {
	int flag, i, j;
	for (i = 0; i < 100; i++) {
		inputSeq[i].position = i;
		comp[i].position = i;
	}
	//SVM_train();
	//����ѵ��ģ��
	model[0] = svm_load_model("train_model/train_model1");
	model[1] = svm_load_model("train_model/train_model2");
	model[2] = svm_load_model("train_model/train_model3");
	model[3] = svm_load_model("train_model/train_model4");
	model[4] = svm_load_model("train_model/train_model5");
	model[5] = svm_load_model("train_model/train_model6");
	total_parent = 0;
	correct_parent = 0;
	model_num = 0;
	char t[10000];
	string tmp;
	int position, len;
	
	//document_parse();
	cout<<"��Ҫ�˳���������'Q'������������һ�仰��"<<endl;
	do{
		inputSeq_len = 0;
		S_len = 0;
		for (i = 0; i < 100; i++) {
			inputSeq[i].parent_position = -1;
		}
		cin.getline(t, 10000);
		if(!strcmp(t,"Q"))
			break;
		len = strlen(t);
		position = 0;
		while (position < len) {
			tmp = "";
			while (t[position] == ' ')
				position++;
			while (t[position] != ' ' && t[position] != '\0')
				tmp += t[position++];
			flag = tmp.find("/", 0);
			strcpy_s(inputSeq[inputSeq_len].str, (tmp.substr(0, flag)).c_str());
			strcpy_s(inputSeq[inputSeq_len].pos, (tmp.substr(flag + 1, tmp.length() - flag - 1).c_str()));
			inputSeq[inputSeq_len].position = inputSeq_len;
			inputSeq_len++;
		}	
		test_sent_parse();
		cout << endl << "�䷨���������" << endl;
		for (i = 0; i < inputSeq_len; i++) {
			for (j = 0; j < strlen(inputSeq[i].str); j++)
				cout << inputSeq[i].str[j];
			cout << "/";
			for (j = 0; j < strlen(inputSeq[i].pos); j++)
				cout << inputSeq[i].pos[j];
			cout << "/" << inputSeq[i].parent_position << " ";
		}
		cout << endl;
	} while(1);
	return 0;
}

