#include <animation/SkeletonComponent.h>

#include <MeshRenderer.h>

void SkeletonComponent::Awake() {
	MeshRenderer* renderer = nullptr;

	if (TryGetObject<MeshRenderer>(renderer)) {
		renderer->SetSkeleton(this);
	}
}