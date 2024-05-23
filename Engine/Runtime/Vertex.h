#pragma once

struct Position {
	float x;
	float y;
};

struct Color {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};

struct UV {
	float u;
	float v;
};

struct Vertex {
	Position position;
	Color color;
	UV uv;

	void SetUV(float u, float v)
	{
		uv.u = u;
		uv.v = v;
	}
};