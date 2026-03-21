#pragma once

#include <DirectXMath.h>

struct Position {
    /// Horizontal position.
	float x;

    /// Vertical position.
	float y;
};

struct Color {
    /// Red channel.
	unsigned char r;

    /// Green channel.
	unsigned char g;

    /// Blue channel.
	unsigned char b;

    /// Alpha channel.
	unsigned char a;
};

struct UV {
    /// Horizontal texture coordinate.
	float u;

    /// Vertical texture coordinate.
	float v;
};

struct Vertex {
    /// Object-space vertex position.
	DirectX::XMFLOAT3 position;

    /// Vertex color.
	DirectX::XMFLOAT4 color;

    /// Object-space vertex normal.
	DirectX::XMFLOAT3 normal;

    /// Texture coordinates.
	DirectX::XMFLOAT2 uv;
};
