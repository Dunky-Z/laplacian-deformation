#include <iostream>
#include <meshlab/glarea.h>
#include "edit_LaplaceDeformation.h"
#include <wrap/qt/gl_label.h>
#include <eigenlib/Eigen/Cholesky>
#include <fstream>

using namespace std;	// MatrixXd ������ std::cout << ��ֱ�� cout << m << endl;����
using namespace vcg;
using Eigen::MatrixXf;
using Eigen::VectorXf;

EditLaplaceDeformationPlugin::EditLaplaceDeformationPlugin(): gla(nullptr)
{
	qFont.setFamily("Helvetica");
	qFont.setPixelSize(12);

	haveToPick = false;
	pickmode = 0; // 0 face 1 vertex
	curFacePtr = 0;
	curVertPtr = 0;
	pIndex = 0;
}

const QString EditLaplaceDeformationPlugin::Info()
{
	return tr("Laplace Deformation on a model!");
}

/// ����һ������������˳ʱ����ת�ĺ�����ֻ�����ڲ��ԣ�ʵ��ʹ��Laplace Deformation�ò���
void EditLaplaceDeformationPlugin::ChimneyRotate(MeshModel &m)
{
	fixed_anchor_idx.clear();
	move_anchor_idx.clear();
	Vertices.clear();
	Faces.clear();
	AdjacentVertices.clear();
	tri::UpdateSelection<CMeshO>::VertexClear(m.cm);
	tri::UpdateSelection<CMeshO>::FaceClear(m.cm);

	int tmp;
	ifstream move_anchor;
	move_anchor.open(R"(C:\Users\Administrator\Desktop\top_anchor.txt)");	// "C:\Users\Administrator\Desktop\top_anchor.txt" Reshaper C++ �ĵ�
	while (move_anchor >> tmp)
		move_anchor_idx.push_back(tmp);
	move_anchor.close();

	ifstream fix_anchor;
	fix_anchor.open(R"(C:\Users\Administrator\Desktop\bot_anchor.txt)");
	while (fix_anchor >> tmp)
		fixed_anchor_idx.push_back(tmp);
	fix_anchor.close();

	// move_anchor��Ϊ��ɫ��move_anchor��Ϊ��ɫ
	//for (CMeshO::VertexIterator vi = m.cm.vert.begin(); vi != m.cm.vert.end(); ++vi)
	//	if (!vi->IsD())
	//	{
	//		int tmp = vi - m.cm.vert.begin();
	//		if (find(move_anchor_idx.begin(), move_anchor_idx.end(), tmp) != move_anchor_idx.end())
	//			vi->C() = vcg::Color4b(vcg::Color4b::Red);
	//		else if (find(fixed_anchor_idx.begin(), fixed_anchor_idx.end(), tmp) != fixed_anchor_idx.end())
	//			vi->C() = vcg::Color4b(vcg::Color4b::Blue);
	//	}
	
//// test ������ת����д�öԲ���
	//float angle = (45 * M_PI) / 180;	// ÿ����ת10��
	//cout << "sin = " << sin(angle) << endl;
	//cout << "cos = " << cos(angle) << endl << endl;
	//for (CMeshO::VertexIterator vi = m.cm.vert.begin(); vi != m.cm.vert.end(); ++vi)
	//{
	//	int idx = vi - m.cm.vert.begin();
	//	if (find(move_anchor_idx.begin(), move_anchor_idx.end(), idx) != move_anchor_idx.end())
	//	{
	//		float tx = m.cm.vert[idx].P().X(), &ty = m.cm.vert[idx].P().Y();
	//		printf("Before Rotate, (%f, %f)\n", tx, ty);
	//		m.cm.vert[idx].P().X() = tx * cos(angle) + ty * sin(angle);
	//		m.cm.vert[idx].P().Y() = -tx * sin(angle) + ty * cos(angle);
	//		printf("After Rotate, (%f, %f)\n\n", m.cm.vert[idx].P().X(), m.cm.vert[idx].P().Y());
	//	}
	//}
//// test ������ת����д�öԲ���

//// Laplace Deformation - Rotate
	toCaculateAdjacentVertices(&m.cm);
	printf("\nVertices = %d\n", static_cast<int>(Vertices.size()));
	printf("Faces = %d\n\n", static_cast<int>(Faces.size()));

	get_LsTLs_Matrix();
	printf("LsTLs ����������\n");

	const int fix_anchors = fixed_anchor_idx.size(), move_anchors = move_anchor_idx.size(), points_num = Vertices.size();
	VectorXf vx(points_num), vy(points_num);
	for (auto i = 0; i < points_num; i++)
	{
		vx[i] = Vertices[i].X();
		vy[i] = Vertices[i].Y();
	}
	bx = L * vx;	// ����laplace�����������е�ĵ�laplace����
	by = L * vy;	// ����laplace�����������е�ĵ�laplace����

	bx.conservativeResize(points_num + fix_anchors + move_anchors);
	by.conservativeResize(points_num + fix_anchors + move_anchors);

	for (auto i = 0; i < fix_anchors; i++)
	{
		bx[i + points_num] = Vertices[fixed_anchor_idx[i]].X();
		by[i + points_num] = Vertices[fixed_anchor_idx[i]].Y();
	}

	const float angle = (15.0 * M_PI) / 180.0;	// ÿ����ת10�ȣ�����д 90 / 180 * pi��90/180 = 0.��

	// x_new = x * cos + y * sin
	// y_new = -x * sin + y * cos
	for (auto i = 0; i < move_anchors; i++)
	{
		const auto tx = Vertices[move_anchor_idx[i]].X(), ty = Vertices[move_anchor_idx[i]].Y();
		bx[i + points_num + fix_anchors] = tx * cos(angle) + ty * sin(angle);
		by[i + points_num + fix_anchors] = -tx * sin(angle) + ty * cos(angle);
	}
	LsTbx = AT * bx;
	LsTby = AT * by;
	printf("LsTb ����������, ͨ��cholesky�ֽ⣬�����Է����� LsTLs * x = LsTb\n\n\n");

	vx_new = LsTLs.llt().solve(LsTbx);
	vy_new = LsTLs.llt().solve(LsTby);

	for (auto i = 0; i < m.cm.vert.size(); i++)
	{
		if (find(move_anchor_idx.begin(), move_anchor_idx.end(), i) != move_anchor_idx.end())
		{
			const auto tx = Vertices[i].X(), ty = Vertices[i].Y();
			m.cm.vert[i].P().X() = tx * cos(angle) + ty * sin(angle);
			m.cm.vert[i].P().Y() = -tx * sin(angle) + ty * cos(angle);
		}
		else if (find(fixed_anchor_idx.begin(), fixed_anchor_idx.end(), i) == fixed_anchor_idx.end())	// �Ȳ���move�㣬�ֲ��ǹ̶���
		{
			m.cm.vert[i].P().X() = vx_new[i];
			m.cm.vert[i].P().Y() = vy_new[i];
		}
	}
	//// Laplace Deformation - Rotate

	m.updateDataMask(MeshModel::MM_POLYGONAL);
	tri::UpdateBounding<CMeshO>::Box(m.cm);
	tri::UpdateNormal<CMeshO>::PerVertexNormalizedPerFaceNormalized(m.cm);

	////// ����update����
	gla->mvc()->sharedDataContext()->meshInserted(m.id());
	MLRenderingData dt;
	gla->mvc()->sharedDataContext()->getRenderInfoPerMeshView(m.id(), gla->context(), dt);
	suggestedRenderingData(m, dt);
	MLPoliciesStandAloneFunctions::disableRedundatRenderingDataAccordingToPriorities(dt);
	gla->mvc()->sharedDataContext()->setRenderingDataPerMeshView(m.id(), gla->context(), dt);
	gla->update();
	////// ����update����
}

// ������ǰѡ��ģ��
/// ʹ��resharper C++ �Ż�������α���룬�Դ���� MeshModel �����α� - GY 2018.12.17
bool EditLaplaceDeformationPlugin::StartEdit(MeshModel & m, GLArea * _gla, MLSceneGLSharedDataContext* ctx)
{
	gla = _gla;

//// ���̴���תʵ�� 2018.8.31�ɹ������ǲ��������������ת������Ƭ�������ཻ
	//ChimneyRotate(m);
	//return true;
//// ���̴���תʵ��

	// Ϊʲôģ����������ѡ�еĵ㣿��������ClearS
	/*for (CMeshO::VertexIterator vi = m.cm.vert.begin(); vi != m.cm.vert.end(); ++vi)
	if (!vi->IsD() && vi->IsS())
	{
	(*vi).C() = vcg::Color4b(vcg::Color4b::Red);
	cout << vi - m.cm.vert.begin() << endl;
	}*/

//// Iniatial ����
	fixed_anchor_idx.clear();
	move_anchor_idx.clear();
	tri::UpdateSelection<CMeshO>::VertexClear(m.cm);
	tri::UpdateSelection<CMeshO>::FaceClear(m.cm);
	//tri::UpdateFlags<CMeshO>::FaceClearS(m.cm);		// ����
	//tri::UpdateFlags<CMeshO>::VertexClearS(m.cm);		// ����

	move_anchor_coord.resize(m.cm.vert.size());
	for (auto& mac : move_anchor_coord)
	{
		mac.X() = -1;
		mac.Y() = -1;
		mac.Z() = -1;
	}
//// Iniatial ����
	m.updateDataMask(MeshModel::MM_ALL);

	// ���� fixed_anchor.txt �ļ�����Ϊ�̶�ê��
	int tmp;
	ifstream fix_a;
	QString fixed_anchor_txt = QFileDialog::getOpenFileName(this->gla, tr("Open fixed_anchor_txt File..."), ".", tr("Setting (*.txt)"));
	fix_a.open(fixed_anchor_txt.toStdString());

	while (fix_a >> tmp)
		fixed_anchor_idx.push_back(tmp);	// ���ظ���Ҳû�н����жϣ�1 2 2 2���3��2
	fix_a.close();

	// ���� move_anchor_coord.txt �ļ�����Ϊ�ƶ�ê��������� idx x y z
	ifstream move_coord;
	QString move_anchor_txt = QFileDialog::getOpenFileName(this->gla, tr("Open move_anchor_txt File..."), ".", tr("Setting (*.txt)"));
	move_coord.open(move_anchor_txt.toStdString());
	while (move_coord >> tmp)
	{
		move_anchor_idx.push_back(tmp);
		move_coord >> move_anchor_coord[tmp].X();
		move_coord >> move_anchor_coord[tmp].Y();
		move_coord >> move_anchor_coord[tmp].Z();
	}
	move_coord.close();
	
	//for (auto vi = m.cm.vert.begin(); vi != m.cm.vert.end(); ++vi) // CMeshO::VertexIterator
	//{
	//	if (vi->IsD())
	//		printf("���Ѿ���ɾ��\n");
	//	else
	//	{
	//		if (find(fixed_anchor_idx.begin(), fixed_anchor_idx.end(), static_cast<int>(vi - m.cm.vert.begin())) != fixed_anchor_idx.end())
	//			vi->C() = vcg::Color4b(vcg::Color4b::Blue);
	//		else if (find(move_anchor_idx.begin(), move_anchor_idx.end(), static_cast<int>(vi - m.cm.vert.begin())) != move_anchor_idx.end())
	//			vi->C() = vcg::Color4b(vcg::Color4b::Red);
	//	}
	//}
	printf("�� %d ���㱻ѡΪ�̶�ê�㣨��ɫ��ʾ��!\n", static_cast<int>(fixed_anchor_idx.size()));
	printf("�� %d ���㱻ѡΪ�ƶ�ê�㣨��ɫ��ʾ��!\n", static_cast<int>(move_anchor_idx.size()));

	if(fixed_anchor_idx.empty() || move_anchor_idx.empty())
		throw std::exception("Wrong input");
	LaplaceDeformation(m); // ����Laplace�α�

	// �����û�� nan
	//for (CMeshO::VertexIterator vi = m.cm.vert.begin(); vi != m.cm.vert.end(); vi++)
	//	printf("%d - (%f, %f, %f)\n", vi - m.cm.vert.begin(), vi->P().X(), vi->P().Y(), vi->P().Z());

////// ����update����
	gla->mvc()->sharedDataContext()->meshInserted(m.id());
	MLRenderingData dt;
	gla->mvc()->sharedDataContext()->getRenderInfoPerMeshView(m.id(), gla->context(), dt);
	suggestedRenderingData(m, dt);
	MLPoliciesStandAloneFunctions::disableRedundatRenderingDataAccordingToPriorities(dt);
	gla->mvc()->sharedDataContext()->setRenderingDataPerMeshView(m.id(), gla->context(), dt);
	gla->update();
////// ����update����

///////////// ��������ָ�벻��ʹ����������// 2018.8.17���ϱߵĴ�����Ը���ģ�ͣ������±ߵ���β���
	//ctx->meshInserted(m.id());
	//MLRenderingData dt;
	//ctx->getRenderInfoPerMeshView(m.id(), gla->context(), dt);
	//// ��Ҫ�Լ�ʵ�ֵĴ��麯��
	//suggestedRenderingData(m, dt);
	//MLPoliciesStandAloneFunctions::disableRedundatRenderingDataAccordingToPriorities(dt);
	//ctx->setRenderingDataPerMeshView(m.id(), gla->context(), dt);
	//gla->update();
///////////// ��������ָ�벻��ʹ����������

	return true;
}

void EditLaplaceDeformationPlugin::EndEdit(MeshModel &/*m*/, GLArea * /*parent*/, MLSceneGLSharedDataContext* /*cont*/)
{
	haveToPick = false;
	pickmode = 0; // 0 face 1 vertex
	curFacePtr = 0;
	curVertPtr = 0;
	pIndex = 0;
}

void EditLaplaceDeformationPlugin::LaplaceDeformation(MeshModel& m)
{
	toCaculateAdjacentVertices(&m.cm);
	printf("\nVertices = %d\n", static_cast<int>(Vertices.size()));
	printf("Faces = %d\n\n", static_cast<int>(Faces.size()));

	get_LsTLs_Matrix();
	printf("LsTLs ����������\n");
	get_LsTb_Matrix();
	printf("LsTb ����������, ͨ��cholesky�ֽ⣬�����Է����� LsTLs * x = LsTb\n");

	// ͨ��cholesky�ֽ⣬�����Է����� Ax = b���� LsTLs * x = LsTb
	// Cholesky�ֽ��ǰ�һ���Գ�����(symmetric, positive definite)�ľ����ʾ��һ�������Ǿ��� L ����ת�� LT �ĳ˻��ķֽ�
	// ��Ҫͷ�ļ� #include <Eigen/Cholesky>
	vx_new = LsTLs.ldlt().solve(LsTbx);	// ��֪��llt().solve()Ϊʲô�����nan
	vy_new = LsTLs.ldlt().solve(LsTby);
	vz_new = LsTLs.ldlt().solve(LsTbz);
	printf("�µ�����������\n");

	setNewCoord(m);
	printf("ģ������������\n");

	tri::UpdateBounding<CMeshO>::Box(m.cm);
	tri::UpdateNormal<CMeshO>::PerVertexNormalizedPerFaceNormalized(m.cm);

}

void EditLaplaceDeformationPlugin::toCaculateAdjacentVertices(CMeshO* cm)
{
	for (auto& vi : cm->vert) // CMeshO::VertexIterator
		Vertices.push_back(vi.P());



	for (auto fi = cm->face.begin(); fi != cm->face.end(); ++fi) // CMeshO::FaceIterator
		if (!(*fi).IsD())
		{
			std::vector<int> tri;
			for (auto k = 0; k < (*fi).VN(); k++)
			{
				const auto vInd = static_cast<int>(fi->V(k) - &*(cm->vert.begin()));
				//if (cm->vert[vInd].IsD())
				//	printf("���Ѿ���ɾ��");
				tri.push_back(vInd);
			}
			Faces.push_back(tri);
		}

	CalculateAdjacentVertices(cm);

	// ������е�Ľ�������
	/*for (int i = 0; i < AdjacentVertices.size(); i++)
		printf("�� %d ������ %d ������\n", i, AdjacentVertices[i].size());*/

	// ������е���Χ�������
	//for (int i = 0; i < AdjacentVertices.size(); i++)
	//	for (int j = 0; j < AdjacentVertices[i].size(); j++)
	//		printf("AdjacentVertices[%d][%d] = %d\n", i, j, AdjacentVertices[i][j]);
}

void EditLaplaceDeformationPlugin::CalculateAdjacentVertices(CMeshO* cm)
{
	AdjacentVertices.resize(Vertices.size());
	for (auto& face : Faces)
	{
		//printf("t0 = %d, t[1] = %, t[2] = %d\n", t[0], t[1], t[2]);
		auto& p0list = AdjacentVertices[face[0]];
		auto& p1list = AdjacentVertices[face[1]];
		auto& p2list = AdjacentVertices[face[2]];

		auto vi0 = cm->vert.begin() + face[0];
		auto vi1 = cm->vert.begin() + face[1];
		auto vi2 = cm->vert.begin() + face[2];
		if (!vi1->IsD() && std::find(p0list.begin(), p0list.end(), face[1]) == p0list.end())
			p0list.push_back(face[1]);
		if (!vi2->IsD() && std::find(p0list.begin(), p0list.end(), face[2]) == p0list.end())
			p0list.push_back(face[2]);
		if (!vi0->IsD() && std::find(p1list.begin(), p1list.end(), face[0]) == p1list.end())
			p1list.push_back(face[0]);
		if (!vi2->IsD() && std::find(p1list.begin(), p1list.end(), face[2]) == p1list.end())
			p1list.push_back(face[2]);
		if (!vi0->IsD() && std::find(p2list.begin(), p2list.end(), face[0]) == p2list.end())
			p2list.push_back(face[0]);
		if (!vi1->IsD() && std::find(p2list.begin(), p2list.end(), face[1]) == p2list.end())
			p2list.push_back(face[1]);
	}
}

void EditLaplaceDeformationPlugin::get_LsTLs_Matrix()
{
	const int fix_anchors = fixed_anchor_idx.size(), move_anchors = move_anchor_idx.size(), points_num = Vertices.size();

	// points_num ����̫��ʵ�� 12000 ��ʱ������bad_alloc
	// 30000+�����ʱ���ڴ�ص�12+GB��10000�ǿ��Եģ�һ���Ӳ������α������
	L = MatrixXf(points_num, points_num);
	Ls = MatrixXf(points_num + fix_anchors + move_anchors, points_num);

	for (auto i = 0; i < points_num; i++)
	{
		for (auto j = 0; j < points_num; j++)
		{
			if (i == j)
			{
				//printf("�� %d ������ %d ������\n", i, AdjacentVertices[i].size());
				L(i, j) = AdjacentVertices[i].size();
				Ls(i, j) = AdjacentVertices[i].size();
				continue;
			}
			if (std::find(AdjacentVertices[i].begin(), AdjacentVertices[i].end(), j) != AdjacentVertices[i].end())
			{
				L(i, j) = -1;
				Ls(i, j) = -1;
			}
			else
			{
				L(i, j) = 0;
				Ls(i, j) = 0;
			}
		}
	}
	// �̶�ê��
	for (auto i = 0; i < fix_anchors; i++)
		for (auto j = 0; j < points_num; j++)
		{
			if (j == fixed_anchor_idx[i])
				Ls(points_num + i, j) = 1;
			else
				Ls(points_num + i, j) = 0;
		}
	// �ƶ�ê��
	for (auto i = 0; i < move_anchor_idx.size(); i++)
		for (auto j = 0; j < points_num; j++)
		{
			if (j == move_anchor_idx[i]) Ls(points_num + fix_anchors + i, j) = 1;
			else            			 Ls(points_num + fix_anchors + i, j) = 0;
		}

	//printf("Laplace���� - L\n");
	//cout << L << endl << endl;

	//printf("������ê����Ϣ��Laplace���� - Ls\n");
	//cout << Ls << endl << endl;

	printf("Laplace ����������\n");
	// ����̫���˴�ӡ������������ʲô����
	// ���һ��
	//for (int i = 0; i < points_num; i++)
	//{
	//	int cnt = 0;
	//	int tmp = 0;
	//	for (int j = 0; j < points_num; j++)
	//	{
	//		if (Ls(i, j) == -1)
	//			++cnt;
	//		else if (Ls(i, j) != 0)
	//			tmp = Ls(i, j);
	//	}
	//	if (cnt != tmp)	printf("Error!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	//}

	AT = Ls.transpose();
	LsTLs = AT * Ls;

	// �鿴������Ϣ
	//printf("\nLsTLs\n");
	//cout << LsTLs.rows() << "  " << LsTLs.cols() << endl;
	//cout << LsTLs << endl;
	//printf("\n");
}

void EditLaplaceDeformationPlugin::get_LsTb_Matrix()
{
	const int fix_anchors = fixed_anchor_idx.size(), move_anchors = move_anchor_idx.size(), points_num = Vertices.size();
	VectorXf vx(points_num), vy(points_num), vz(points_num);

	for (auto i = 0; i < points_num; i++)
	{
		vx[i] = Vertices[i].X();
		vy[i] = Vertices[i].Y();
		vz[i] = Vertices[i].Z();
	}

	// ����laplace�����������е�ĵ�laplace����
	bx = L * vx;	// �˼����Ǽ���
	by = L * vy;
	bz = L * vz;

	bx.conservativeResize(points_num + fix_anchors + move_anchors);
	by.conservativeResize(points_num + fix_anchors + move_anchors);
	bz.conservativeResize(points_num + fix_anchors + move_anchors);

	// ���α�ǰ����Թ̶�ê��������и�ֵ
	for (auto i = 0; i < fix_anchors; i++)
	{
		bx[i + points_num] = Vertices[fixed_anchor_idx[i]].X();
		by[i + points_num] = Vertices[fixed_anchor_idx[i]].Y();
		bz[i + points_num] = Vertices[fixed_anchor_idx[i]].Z();
	}

	// ���α��������ƶ�ê��������и�ֵ
	for (auto i = 0; i < move_anchors; i++)
	{
		bx[i + points_num + fix_anchors] = move_anchor_coord[move_anchor_idx[i]].X();
		by[i + points_num + fix_anchors] = move_anchor_coord[move_anchor_idx[i]].Y();
		bz[i + points_num + fix_anchors] = move_anchor_coord[move_anchor_idx[i]].Z();
	}

	// �����������ϵ� LsTb ����
	LsTbx = AT * bx;
	LsTby = AT * by;
	LsTbz = AT * bz;
	//cout << LsTbx << endl << endl;
}

void EditLaplaceDeformationPlugin::setNewCoord(MeshModel& m)
{
	for (auto i = 0; i < m.cm.vert.size(); i++)
	{
		if (find(move_anchor_idx.begin(), move_anchor_idx.end(), i) != move_anchor_idx.end())
		{
			m.cm.vert[i].P().X() = move_anchor_coord[i].X();
			m.cm.vert[i].P().Y() = move_anchor_coord[i].Y();
			m.cm.vert[i].P().Z() = move_anchor_coord[i].Z();
		}
		else if (find(fixed_anchor_idx.begin(), fixed_anchor_idx.end(), i) == fixed_anchor_idx.end())
		{
			m.cm.vert[i].P().X() = vx_new[i];
			m.cm.vert[i].P().Y() = vy_new[i];
			m.cm.vert[i].P().Z() = vz_new[i];
		}
	}
}
//
//void EditLaplaceDeformationPlugin::suggestedRenderingData(MeshModel &m, MLRenderingData& dt)
//{
//	if (m.cm.VN() == 0)
//		return;
//	MLRenderingData::PRIMITIVE_MODALITY pr = MLRenderingData::PR_SOLID;
//	if (m.cm.FN() > 0)
//		pr = MLRenderingData::PR_SOLID;
//
//	MLRenderingData::RendAtts atts;
//	atts[MLRenderingData::ATT_NAMES::ATT_VERTPOSITION] = true;
//	atts[MLRenderingData::ATT_NAMES::ATT_VERTCOLOR] = true;
//	atts[MLRenderingData::ATT_NAMES::ATT_VERTNORMAL] = true;
//	atts[MLRenderingData::ATT_NAMES::ATT_FACENORMAL] = true;
//	atts[MLRenderingData::ATT_NAMES::ATT_FACECOLOR] = true;
//
//	MLPerViewGLOptions opts;
//	dt.get(opts);
//	opts._sel_enabled = true;
//	opts._face_sel = true;
//	opts._vertex_sel = true;
//	dt.set(opts);
//
//	dt.set(pr, atts);
//}