#include "stdafx.h"
#include "Laplacian1.h"

int main(int argc, char **argv)
{
	double start = GetTickCount();  //��ʼʱ��
	LaplaceDeformation Deform;
	Deform.AllpyLaplaceDeformation(argv);
	double finish = GetTickCount();   //����ʱ��
	double t = finish - start;
	cout << t << endl; //���ʱ��
	return 0;
}