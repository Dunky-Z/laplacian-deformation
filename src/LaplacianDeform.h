#pragma once

#include <vector>
#include <iostream>
#include <surface_mesh/Surface_mesh.h>
#include <fstream>
#include <Eigen/Cholesky>

#include <Eigen/Dense>
#include <Eigen/Sparse>

using namespace std;
using namespace surface_mesh;
using namespace Eigen;

class LaplaceDeformation
{
public:
	// L: ԭʼlaplace���󣨷��󣩣�A: ������ê�㣨point_num + anchors, point_num����С�� T��ת��
	Eigen::SparseMatrix<double> A, L;
	Eigen::SparseMatrix<double> AT, ATA, ATbx, ATby, ATbz, tem;
	Eigen::SparseVector<double>  bx, by, bz;
	Eigen::SparseVector<double> vx_new, vy_new, vz_new;

	std::vector<int>  move_anchor_idx, fix_anchor_idx;	
	std::vector<Point> move_anchor_coord;					// �α���ƶ�ê��λ��

	Eigen::SparseMatrix<double> AdjacentVertices, VerticesDegree;	//�����ڽӾ���;����������󣺶��������ڶ��������

	LaplaceDeformation();
	virtual ~LaplaceDeformation() = default;

	void InitializeMesh(const Surface_mesh&mesh);
	void AllpyLaplaceDeformation(char ** argv);	
	void BuildAdjacentMatrix(const Surface_mesh &mesh);
	void BuildATtimesAMatrix(const Surface_mesh & mesh);
	void BuildATtimesbMatrix(const Surface_mesh & mesh);
	void SetNewcord(Surface_mesh & mesh);
};