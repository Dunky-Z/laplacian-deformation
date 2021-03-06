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
	// L: 原始laplace矩阵（方阵），A: 加入了锚点（point_num + anchors, point_num）大小， T：转置
	Eigen::SparseMatrix<double> A, L, AT, ATA, b, ATb, vertice_new;
	std::vector<int>  fix_anchor_idx;					// 形变后移动锚点位置
	Eigen::SparseMatrix<double> AdjacentVertices, VerticesDegree;	//顶点邻接矩阵;顶点度数矩阵：顶点所相邻顶点的数量

	LaplaceDeformation();
	virtual ~LaplaceDeformation() = default;
	void InitializeMesh();
	void AllpyLaplaceDeformation(char ** argv);
	void BuildAdjacentMatrix(const Surface_mesh &mesh);
	void BuildATtimesAMatrix(const Surface_mesh & mesh);
	void BuildATtimesbMatrix(const Surface_mesh & mesh);
	void SetNewcord(Surface_mesh & mesh);
};