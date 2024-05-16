#pragma once

struct Vertex {
	struct {
		float x;
		float y;
	} position;

	struct {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
	} color;
};