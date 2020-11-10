#ifndef SCENE_H
#define SCENE_H
#include <LWEGLTFParser.h>
#include "Mesh.h"
#include "Material.h"
#include <vector>

class Renderer;

class Animation;

struct Node {
	LWSMatrix4f m_Transform;
	std::vector<uint32_t> m_MaterialList;
	std::vector<uint32_t> m_ChildrenList;
	Mesh *m_Mesh = nullptr;
	Animation *m_Animation = nullptr;

	Node(Node &&O) noexcept;

	Node(const Node &O) = delete;

	Node(LWEGLTFParser &P, LWEGLTFNode &N, Renderer *R, LWAllocator &Allocator);

	Node() = default;

	~Node();
};

class Scene {
public:

	static bool LoadGLTFFile(Scene &S, const LWUTF8Iterator &Path, Renderer *R, LWAllocator &Allocator);

	bool PushImageTexID(uint32_t ID);

	bool PushMaterial(const Material &Mat);

	bool PushNode(Node &N, bool isRoot);

	void DrawScene(GFrame &F, Renderer *R, float Time, uint32_t PassBits, const LWSMatrix4f &Transform);

	//Calculates both the 2D tight screen bounding, and the 3D bounding box for the objects in the scene.
	//Returns the 2d tight screen bounding, writes into BoundsMin, and BoundsMax the 3d bounding box.
	LWVector4i CaclulateBounding(float Time, const LWSMatrix4f &Transform, const LWVector2f &WndSize, Camera &Cam, int32_t BorderSize, LWSVector4f &BoundsMin, LWSVector4f &BoundsMax);

	void Finalize(void);

	//Releases all internal resources(textures
	void Release(Renderer *R);

	uint32_t GetImageTexID(uint32_t Idx);

	Material &GetMaterial(uint32_t Idx);

	float GetTotalTime(void) const;

	Scene() = default;

	~Scene();
private:
	std::vector<uint32_t> m_ImageTexID;
	std::vector<uint32_t> m_RootNodes;
	std::vector<Node> m_NodeList;
	std::vector<Material> m_MaterialList;
	float m_TotalTime = 0.0f;
};

#endif