#pragma once

#include <array>
#include <string>
#include <vector>

struct StudioMeshProcessor
{
	struct Vec2
	{
		Vec2(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

		float x = 0.0f;
		float y = 0.0f;
	};

	struct Vec3
	{
		Vec3(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}

		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
	};

	struct Mat4
	{
		std::array<float, 16> Values = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};
	};

	struct Vertex
	{
		Vec3 position;
		Vec3 normal;
	};

	struct Triangle
	{
		std::array<int, 3> indices = { 0, 0, 0 };
	};

	struct Texture
	{
		std::array<float, 16> values = {};
		float minU = 0.0f;
		float minV = 0.0f;
		float diffU = 1.0f;
		float diffV = 1.0f;
		std::string name;
	};

	struct Input
	{
		std::vector<Vertex> vertices;
		std::vector<Triangle> triangles;
		Texture texture;
		Mat4 parentMatrix;
	};

	struct Result
	{
		std::vector<int> selectedTriangles;
		std::vector<Vec2> uv;
		std::vector<bool> uvAssigned;
	};

	static Result assign(const Input& input);
};
