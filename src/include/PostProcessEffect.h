#pragma once

#include <GameObject.h>

class Texture2D;

struct PostProcessParams {
	Texture2D* inputTexture;
	Texture2D* outputTexture;
	Texture2D* depthTexture;
};

class PostProcessEffect : public GameObject {
public:
	virtual void OnPostProcess(const PostProcessParams* params) = 0;
};
