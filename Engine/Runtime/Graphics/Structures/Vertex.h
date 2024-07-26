#pragma once

#include <DirectXMath.h>

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
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	Color color;
	DirectX::XMFLOAT2 uv;
};