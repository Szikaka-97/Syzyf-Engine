#pragma once

#include <GameObject.h>

class Texture2D;

struct PostProcessParams {
	Texture2D* colorTexture;
	Texture2D* depthTexture;
};

class PostProcessor : public GameObject {
public:
	virtual void OnPostProcess(const PostProcessParams* params) = 0;
};