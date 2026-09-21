#include "studio_mesh_processor.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace
{
using Vec2 = StudioMeshProcessor::Vec2;
using Vec3 = StudioMeshProcessor::Vec3;
using Mat4 = StudioMeshProcessor::Mat4;

constexpr float kUVNormalEpsilon = 0.001f;

Vec3 Subtract(const Vec3& A, const Vec3& B)
{
	return { A.x - B.x, A.y - B.y, A.z - B.z };
}

Vec3 Multiply(const Vec3& V, float Scale)
{
	return { V.x * Scale, V.y * Scale, V.z * Scale };
}

float Dot(const Vec3& A, const Vec3& B)
{
	return A.x * B.x + A.y * B.y + A.z * B.z;
}

Vec3 Cross(const Vec3& A, const Vec3& B)
{
	return {
		A.y * B.z - A.z * B.y,
		A.z * B.x - A.x * B.z,
		A.x * B.y - A.y * B.x
	};
}

float Length(const Vec3& V)
{
	return std::sqrt(Dot(V, V));
}

Mat4 Multiply(const Mat4& A, const Mat4& B)
{
	Mat4 Result;

	for (int Row = 0; Row < 4; Row++)
	{
		for (int Column = 0; Column < 4; Column++)
		{
			float Value = 0.0f;
			for (int Index = 0; Index < 4; Index++)
				Value += A.Values[Row * 4 + Index] * B.Values[Index * 4 + Column];
			Result.Values[Row * 4 + Column] = Value;
		}
	}

	return Result;
}

Vec3 TransformPoint(const Mat4& Matrix, const Vec3& Point)
{
	const float X = Matrix.Values[0] * Point.x + Matrix.Values[1] * Point.y + Matrix.Values[2] * Point.z + Matrix.Values[3];
	const float Y = Matrix.Values[4] * Point.x + Matrix.Values[5] * Point.y + Matrix.Values[6] * Point.z + Matrix.Values[7];
	const float Z = Matrix.Values[8] * Point.x + Matrix.Values[9] * Point.y + Matrix.Values[10] * Point.z + Matrix.Values[11];
	const float W = Matrix.Values[12] * Point.x + Matrix.Values[13] * Point.y + Matrix.Values[14] * Point.z + Matrix.Values[15];

	if (std::fabs(W) > 0.0f && W != 1.0f)
		return { X / W, Y / W, Z / W };

	return { X, Y, Z };
}

Mat4 TextureMatrix(const std::array<float, 16>& Values)
{
	Mat4 Matrix;
	Matrix.Values = {
		Values[3], Values[4], -Values[5], Values[0],
		Values[6], Values[7], -Values[8], Values[1],
		-Values[9], -Values[10], Values[11], -Values[2],
		0.0f, 0.0f, 0.0f, 1.0f
	};
	return Matrix;
}

float Determinant3(const Mat4& Matrix)
{
	return Matrix.Values[0] * (Matrix.Values[5] * Matrix.Values[10] - Matrix.Values[6] * Matrix.Values[9]) -
	       Matrix.Values[1] * (Matrix.Values[4] * Matrix.Values[10] - Matrix.Values[6] * Matrix.Values[8]) +
	       Matrix.Values[2] * (Matrix.Values[4] * Matrix.Values[9] - Matrix.Values[5] * Matrix.Values[8]);
}

Mat4 InverseLinearTransform(const Mat4& Matrix)
{
	const float Determinant = Determinant3(Matrix);
	if (std::fabs(Determinant) < 1.0e-12f)
		return Mat4();

	Mat4 Inverse;
	Inverse.Values[0] = (Matrix.Values[5] * Matrix.Values[10] - Matrix.Values[6] * Matrix.Values[9]) / Determinant;
	Inverse.Values[1] = (Matrix.Values[2] * Matrix.Values[9] - Matrix.Values[1] * Matrix.Values[10]) / Determinant;
	Inverse.Values[2] = (Matrix.Values[1] * Matrix.Values[6] - Matrix.Values[2] * Matrix.Values[5]) / Determinant;
	Inverse.Values[4] = (Matrix.Values[6] * Matrix.Values[8] - Matrix.Values[4] * Matrix.Values[10]) / Determinant;
	Inverse.Values[5] = (Matrix.Values[0] * Matrix.Values[10] - Matrix.Values[2] * Matrix.Values[8]) / Determinant;
	Inverse.Values[6] = (Matrix.Values[2] * Matrix.Values[4] - Matrix.Values[0] * Matrix.Values[6]) / Determinant;
	Inverse.Values[8] = (Matrix.Values[4] * Matrix.Values[9] - Matrix.Values[5] * Matrix.Values[8]) / Determinant;
	Inverse.Values[9] = (Matrix.Values[1] * Matrix.Values[8] - Matrix.Values[0] * Matrix.Values[9]) / Determinant;
	Inverse.Values[10] = (Matrix.Values[0] * Matrix.Values[5] - Matrix.Values[1] * Matrix.Values[4]) / Determinant;

	const Vec3 Translation(Matrix.Values[3], Matrix.Values[7], Matrix.Values[11]);
	const Vec3 InverseTranslation = TransformPoint(Inverse, Translation);
	Inverse.Values[3] = -InverseTranslation.x;
	Inverse.Values[7] = -InverseTranslation.y;
	Inverse.Values[11] = -InverseTranslation.z;
	Inverse.Values[12] = 0.0f;
	Inverse.Values[13] = 0.0f;
	Inverse.Values[14] = 0.0f;
	Inverse.Values[15] = 1.0f;

	return Inverse;
}

struct ProjectionTransform
{
	Mat4 InverseMatrix;
	Vec3 BoxExtents;
};

ProjectionTransform CalculateProjectionTransform(const StudioMeshProcessor::Texture& Texture, const Mat4& ParentMatrix)
{
	const Mat4 Combined = Multiply(ParentMatrix, TextureMatrix(Texture.values));

	const Vec3 Column0(Combined.Values[0], Combined.Values[4], Combined.Values[8]);
	const Vec3 Column1(Combined.Values[1], Combined.Values[5], Combined.Values[9]);
	const Vec3 Column2(Combined.Values[2], Combined.Values[6], Combined.Values[10]);
	const Vec3 Columns[3] = { Column0, Column1, Column2 };

	Mat4 BoxTransform;
	Vec3 Extents;
	const float Scales[3] = { Length(Column0), Length(Column1), Length(Column2) };

	for (int Axis = 0; Axis < 3; Axis++)
	{
		if (Scales[Axis] < 1.0e-8f)
			return { Mat4(), { 0.0f, 0.0f, 0.0f } };

		const Vec3 Normalized = Multiply(Columns[Axis], 1.0f / Scales[Axis]);
		BoxTransform.Values[0 + Axis] = Normalized.x;
		BoxTransform.Values[4 + Axis] = Normalized.y;
		BoxTransform.Values[8 + Axis] = Normalized.z;
		Extents = Axis == 0 ? Vec3 { 0.5f * Scales[0], 0.0f, 0.0f } :
		          Axis == 1 ? Vec3 { Extents.x, 0.5f * Scales[1], Extents.z } :
		                      Vec3 { Extents.x, Extents.y, 0.5f * Scales[2] };
	}

	BoxTransform.Values[3] = Combined.Values[3];
	BoxTransform.Values[7] = Combined.Values[7];
	BoxTransform.Values[11] = Combined.Values[11];
	BoxTransform.Values[12] = 0.0f;
	BoxTransform.Values[13] = 0.0f;
	BoxTransform.Values[14] = 0.0f;
	BoxTransform.Values[15] = 1.0f;

	Extents.y = std::numeric_limits<float>::max();
	return { InverseLinearTransform(BoxTransform), Extents };
}

bool TriangleIntersectsBox(const Vec3& V0, const Vec3& V1, const Vec3& V2, const Vec3& Extents)
{
	const double Ex = Extents.x;
	const double Ey = Extents.y;
	const double Ez = Extents.z;
	const double Vertices[3][3] = {
		{ V0.x, V0.y, V0.z },
		{ V1.x, V1.y, V1.z },
		{ V2.x, V2.y, V2.z }
	};

	for (int Axis = 0; Axis < 3; Axis++)
	{
		const double Minimum = std::min({ Vertices[0][Axis], Vertices[1][Axis], Vertices[2][Axis] });
		const double Maximum = std::max({ Vertices[0][Axis], Vertices[1][Axis], Vertices[2][Axis] });
		const double Extent = Axis == 0 ? Ex : Axis == 1 ? Ey : Ez;

		if (Minimum > Extent || Maximum < -Extent)
			return false;
	}

	const Vec3 Edges[3] = { Subtract(V1, V0), Subtract(V2, V1), Subtract(V0, V2) };
	const Vec3 Axes[3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };

	for (const Vec3& Edge : Edges)
	{
		for (const Vec3& Axis : Axes)
		{
			const Vec3 SeparatingAxis = Cross(Edge, Axis);
			const double P0 = Dot(SeparatingAxis, V0);
			const double P1 = Dot(SeparatingAxis, V1);
			const double P2 = Dot(SeparatingAxis, V2);
			const double Minimum = std::min({ P0, P1, P2 });
			const double Maximum = std::max({ P0, P1, P2 });
			const double Radius = Ex * std::fabs(SeparatingAxis.x) + Ey * std::fabs(SeparatingAxis.y) + Ez * std::fabs(SeparatingAxis.z);

			if (Minimum > Radius || Maximum < -Radius)
				return false;
		}
	}

	const Vec3 Normal = Cross(Edges[0], Edges[1]);
	const double Distance = Dot(Normal, V0);
	const double Radius = Ex * std::fabs(Normal.x) + Ey * std::fabs(Normal.y) + Ez * std::fabs(Normal.z);
	return std::fabs(Distance) <= Radius;
}
} // namespace

StudioMeshProcessor::Result StudioMeshProcessor::assign(const Input& input)
{
	Result ResultData;
	ResultData.uv.resize(input.vertices.size());
	ResultData.uvAssigned.assign(input.vertices.size(), false);

	const ProjectionTransform Projection = CalculateProjectionTransform(input.texture, input.parentMatrix);
	std::unordered_set<int> TrianglesInBox;

	// TEMP DIAGNOSTIC: which filter rejects the decal triangles?
	size_t SkipAssigned = 0, SkipNormal = 0, SkipBox = 0;

	for (size_t TriangleIndex = 0; TriangleIndex < input.triangles.size(); TriangleIndex++)
	{
		const Triangle& TriangleData = input.triangles[TriangleIndex];
		const int V0 = TriangleData.indices[0];
		const int V1 = TriangleData.indices[1];
		const int V2 = TriangleData.indices[2];

		if (ResultData.uv[V0].x > 0.0f || ResultData.uv[V1].x > 0.0f || ResultData.uv[V2].x > 0.0f)
		{
			SkipAssigned++;
			continue;
		}

		const Vec3 P0 = TransformPoint(Projection.InverseMatrix, input.vertices[V0].position);
		const Vec3 P1 = TransformPoint(Projection.InverseMatrix, input.vertices[V1].position);
		const Vec3 P2 = TransformPoint(Projection.InverseMatrix, input.vertices[V2].position);
		const Vec3 Normal = Cross(Subtract(P1, P0), Subtract(P2, P0));

		if (Dot(Normal, { 0.0f, -1.0f, 0.0f }) < kUVNormalEpsilon)
		{
			SkipNormal++;
			continue;
		}

		if (!TriangleIntersectsBox(P0, P1, P2, Projection.BoxExtents))
		{
			SkipBox++;
			continue;
		}

		TrianglesInBox.insert(static_cast<int>(TriangleIndex));
	}

	fprintf(stderr, "STUDIOTEXDBG assign: total=%zu skipAssigned=%zu skipNormal=%zu skipBox=%zu inBox=%zu tex='%s' minU=%.3f minV=%.3f diffU=%.3f diffV=%.3f box=(%.2f,%.2f,%.2f)\n",
		input.triangles.size(), SkipAssigned, SkipNormal, SkipBox, TrianglesInBox.size(),
		input.texture.name.c_str(), input.texture.minU, input.texture.minV, input.texture.diffU, input.texture.diffV,
		Projection.BoxExtents.x, Projection.BoxExtents.y, Projection.BoxExtents.z);

	std::unordered_map<int, std::vector<int>> VertexToTriangles;
	for (size_t TriangleIndex = 0; TriangleIndex < input.triangles.size(); TriangleIndex++)
	{
		for (const int VertexIndex : input.triangles[TriangleIndex].indices)
			VertexToTriangles[VertexIndex].push_back(static_cast<int>(TriangleIndex));
	}

	std::unordered_set<int> Visited;
	std::queue<int> Pending;
	for (const int TriangleIndex : TrianglesInBox)
		Pending.push(TriangleIndex);

	while (!Pending.empty())
	{
		const int TriangleIndex = Pending.front();
		Pending.pop();

		if (!Visited.insert(TriangleIndex).second)
			continue;

		for (const int VertexIndex : input.triangles[TriangleIndex].indices)
		{
			const auto MapIt = VertexToTriangles.find(VertexIndex);
			if (MapIt == VertexToTriangles.end())
				continue;

			for (const int ConnectedTriangle : MapIt->second)
			{
				if (Visited.find(ConnectedTriangle) == Visited.end())
				{
					Pending.push(ConnectedTriangle);
					TrianglesInBox.insert(ConnectedTriangle);
				}
			}
		}
	}

	for (const int TriangleIndex : TrianglesInBox)
	{
		for (const int VertexIndex : input.triangles[TriangleIndex].indices)
		{
			const Vec3 Point = TransformPoint(Projection.InverseMatrix, input.vertices[VertexIndex].position);
			const Vec2 UV = {
				(Point.x - input.texture.minU) / input.texture.diffU,
				(Point.z - input.texture.minV) / input.texture.diffV
			};

			ResultData.uv[VertexIndex] = UV;
			ResultData.uvAssigned[VertexIndex] = true;
		}
	}

	ResultData.selectedTriangles.assign(TrianglesInBox.begin(), TrianglesInBox.end());
	std::sort(ResultData.selectedTriangles.begin(), ResultData.selectedTriangles.end());
	fprintf(stderr, "STUDIOTEXDBG assign: FINAL selected=%zu (verts=%zu)\n",
		ResultData.selectedTriangles.size(), input.vertices.size());
	return ResultData;
}
