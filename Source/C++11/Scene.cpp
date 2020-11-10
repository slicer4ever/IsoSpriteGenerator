#include "Scene.h"
#include <LWVideo/LWImage.h>
#include "Renderer.h"
#include "Camera.h"
#include "Logger.h"
#include "Animation.h"

//Node
Node::Node(LWEGLTFParser &P, LWEGLTFNode &N, Renderer *R, LWAllocator &Allocator) : m_Transform(N.m_TransformMatrix) {
	LWEGLTFMesh *GMsh = nullptr;
	LWEGLTFSkin *GSkn = nullptr;
	if (N.m_MeshID != -1) GMsh = P.GetMesh(N.m_MeshID);
	if (N.m_SkinID != -1) GSkn = P.GetSkin(N.m_SkinID);
	if (GMsh) {
		m_Mesh = Allocator.Create<Mesh>();
		m_Mesh->MakeGLTFMesh(P, &N, GMsh, Allocator);
		if (GSkn) {
			m_Mesh->MakeGLTFSkin(P, &N, GSkn, Allocator);
			m_Animation = Allocator.Create<Animation>();
			m_Animation->MakeGLTFSkin(P, GSkn);
		}
		
		m_Mesh->GetVertices().UploadData(R, Allocator, true);
		m_Mesh->GetIndices().UploadData(R, Allocator, true);
		m_Mesh->BuildAABB(LWSMatrix4f(), nullptr);
	}
}

Node::Node(Node &&O) noexcept : m_Transform(O.m_Transform), m_MaterialList(std::move(O.m_MaterialList)), m_ChildrenList(std::move(O.m_ChildrenList)), m_Mesh(O.m_Mesh), m_Animation(O.m_Animation){
	O.m_Mesh = nullptr;
	O.m_Animation = nullptr;
}

Node::~Node() {
	LWAllocator::Destroy(m_Mesh);
	LWAllocator::Destroy(m_Animation);
}

//Scene
bool Scene::LoadGLTFFile(Scene &S, const LWUTF8Iterator &Path, Renderer *R, LWAllocator &Allocator) {
	LWEGLTFParser P;

	std::vector<uint32_t> NodeList;
	std::vector<uint32_t> MeshList;
	std::vector<uint32_t> SkinList;
	std::vector<uint32_t> LightList;
	std::vector<uint32_t> MaterialList;
	std::vector<uint32_t> TextureList;
	std::vector<uint32_t> ImageList;
	if (!LWEGLTFParser::LoadFile(P, Path, Allocator)) {
		LogCritical(LWUTF8I::Fmt<256>("Error: failed to load file: '{}'", Path));
		return false;
	}

	auto MapIDToListIndex = [](std::vector<uint32_t> &List, uint32_t ID)->uint32_t {
		for (uint32_t i = 0; i < List.size(); i++) {
			if (ID == List[i]) return i;
		}
		return -1;
	};

	std::function<void(uint32_t, bool)> ParseNode = [&S, &P, &R, &NodeList, &MaterialList, &Allocator, &MapIDToListIndex, &ParseNode](uint32_t NodeID, bool isRoot) {
		LWEGLTFNode *GN = P.GetNode(NodeID); 
		Node N(P, *GN, R, Allocator);
		if (N.m_Mesh) {
			//Add meterials.
			LWEGLTFMesh *GMsh = P.GetMesh(GN->m_MeshID);
			for (auto &&Prim : GMsh->m_Primitives) {
				uint32_t ID = MapIDToListIndex(MaterialList, Prim.m_MaterialID);
				if(ID==-1) continue;
				N.m_MaterialList.push_back(MapIDToListIndex(MaterialList, Prim.m_MaterialID));
			}
		}
		//Add children:
		for (auto &&C : GN->m_Children) N.m_ChildrenList.push_back(MapIDToListIndex(NodeList, C));
		S.PushNode(N, isRoot);
		for (auto &&C : GN->m_Children) ParseNode(C, false);
		return;
	};

	LWEGLTFScene *GS = P.BuildSceneOnlyList(P.GetDefaultSceneID(), NodeList, MeshList, SkinList, LightList, MaterialList, TextureList, ImageList);
	if (!GS) {
		LogCritical("Error: No default scene provided.");
		return false;
	}

	for (uint32_t i = 0; i < ImageList.size(); i++) {
		LWImage *Img = Allocator.Create<LWImage>();
		if (!P.LoadImage(*Img, ImageList[i], Allocator)) {
			LogCritical(LWUTF8I::Fmt<256>("Error: Failed to load image '{}'.", P.GetImage(ImageList[i])->GetName()));
			LWAllocator::Destroy(Img);
		} else {
			S.PushImageTexID(R->PushPendingTexture(0, Img));
		}
	}
	for (uint32_t i = 0; i < MaterialList.size(); i++) {
		Material Mat;
		Mat.MakeGLTFMaterial(P, P.GetMaterial(MaterialList[i]));
		//Fix texture id's.
		uint32_t TexCnt = Mat.GetTextureCount();
		for (uint32_t n = 0; n < TexCnt; n++) {
			MaterialTexture &MT = Mat.GetTexture(n);
			LWEGLTFTexture *Tex = P.GetTexture(MT.m_TextureID);
			if (!Tex) {
				MT.m_TextureID = 0;
				continue;
			}
			MT.m_TextureID = S.GetImageTexID(MapIDToListIndex(ImageList, Tex->m_ImageID));
		}
		S.PushMaterial(Mat);
	}
	for (auto &&N : GS->m_NodeList) ParseNode(N, true);
	S.Finalize();
	return true;
}

bool Scene::PushImageTexID(uint32_t ID) {
	m_ImageTexID.push_back(ID);
	return true;
}

bool Scene::PushMaterial(const Material &Mat) {
	m_MaterialList.push_back(Mat);
	return true;
}

bool Scene::PushNode(Node &N, bool isRoot) {
	uint32_t ID = (uint32_t)m_NodeList.size();
	m_NodeList.push_back(std::move(N));
	if (isRoot) m_RootNodes.push_back(ID);
	return true;
}

void Scene::DrawScene(GFrame &F, Renderer *R, float Time, uint32_t PassBits, const LWSMatrix4f &Transform) {
	std::function<void(uint32_t, const LWSMatrix4f &)> DrawNode = [this, &Time, &PassBits, &F, &R, &DrawNode](uint32_t NodeID, const LWSMatrix4f &Transform) {
		LWSMatrix4f BoneTransforms[Mesh::MaxBones];
		Node &N = m_NodeList[NodeID];
		LWSMatrix4f Trans = N.m_Transform * Transform;
		Material DefMaterial;
		if (N.m_Mesh) {
			uint32_t PrimCount = N.m_Mesh->GetPrimitiveCount();
			uint32_t VertID = N.m_Mesh->GetVertices().m_ID;
			uint32_t IndicesID = N.m_Mesh->GetIndices().m_ID;
			uint32_t AnimID = 0;
			if (N.m_Animation) {
				uint32_t BoneCount = N.m_Animation->MakeBoneTransforms(Time, false, N.m_Mesh, BoneTransforms);
				if (BoneCount) {
					AnimID = F.NextAnimation();
					N.m_Mesh->BuildRenderMatrixs(BoneTransforms, F.GetAnimDataAt(AnimID)->BoneMatrixs);
				}
			}
			for (uint32_t i = 0; i < PrimCount; i++) {
				Primitive &P = N.m_Mesh->GetPrimitive(i);
				Material *Mat = &DefMaterial;
				if (i < N.m_MaterialList.size()) Mat = &m_MaterialList[N.m_MaterialList[i]];
				Mat->SetTime(Time, true);
				R->WriteGeometry(F, VertID, IndicesID, AnimID, PassBits, Trans, *Mat, 0, P.m_Offset, P.m_Count);
			}
		}
		for (auto &&C : N.m_ChildrenList) DrawNode(C, Trans);
		return;
	};

	for (auto &&C : m_RootNodes) DrawNode(C, Transform);
	return;
}

LWVector4i Scene::CaclulateBounding(float Time, const LWSMatrix4f &Transform, const LWVector2f &WndSize, Camera &Cam, int32_t BorderSize, LWSVector4f &BoundsMin, LWSVector4f &BoundsMax){
	LWSMatrix4f ProjViewMatrix = Cam.GetProjViewMatrix();
	std::function<bool(uint32_t, const LWSMatrix4f &, LWVector4i &, LWSVector4f &, LWSVector4f &, bool)> BoundNode = [this, &Time, &WndSize, &Cam, &ProjViewMatrix, &BoundNode](uint32_t NodeID, const LWSMatrix4f &Transform, LWVector4i &ParentsBound, LWSVector4f &ParentsMinBounds, LWSVector4f &ParentsMaxBounds, bool ParentHadBounds)->bool {
		LWSMatrix4f BoneTransforms[Mesh::MaxBones];
		Node &N = m_NodeList[NodeID];
		LWSMatrix4f Trans = N.m_Transform * Transform;
		bool HasBounds = N.m_Mesh != nullptr;
		LWVector4i Bounds = LWVector4i();
		LWSVector4f MinBounds, MaxBounds;
		if (N.m_Mesh) {
			if (N.m_Animation) N.m_Animation->MakeBoneTransforms(Time, false, N.m_Mesh, BoneTransforms);
			Bounds = N.m_Mesh->BuildBounds(Trans, BoneTransforms, WndSize, ProjViewMatrix, MinBounds, MaxBounds);
		}
		for (auto &&C : N.m_ChildrenList) HasBounds = BoundNode(C, Trans, Bounds, MinBounds, MaxBounds, HasBounds) || HasBounds;
		if (HasBounds) {
			if (ParentHadBounds) {
				ParentsBound = LWVector4i(ParentsBound.xy().Min(Bounds.xy()), ParentsBound.zw().Max(Bounds.zw()));
				ParentsMinBounds = ParentsMinBounds.Min(MinBounds);
				ParentsMaxBounds = ParentsMaxBounds.Max(MaxBounds);
			} else {
				ParentsBound = Bounds;
				ParentsMinBounds = MinBounds;
				ParentsMaxBounds = MaxBounds;
			}
		}
		return HasBounds;
	};
	LWVector4i Bounds = LWVector4i(0);
	bool HasBounds = false;
	for (auto &&C : m_RootNodes) HasBounds = BoundNode(C, Transform, Bounds, BoundsMin, BoundsMax, HasBounds) || HasBounds;
	if (HasBounds) Bounds = LWVector4i(Bounds.xy() - BorderSize, Bounds.zw() + BorderSize);
	return Bounds;
}

void Scene::Finalize(void) {
	std::function<void(uint32_t)> EvaluateNode = [this, &EvaluateNode](uint32_t NodeID) {
		Node &N = m_NodeList[NodeID];
		if (N.m_Animation) m_TotalTime = std::max<float>(m_TotalTime, N.m_Animation->GetTotalTime());
		for (auto &&C : N.m_ChildrenList) EvaluateNode(C);
		return;
	};
	m_TotalTime = 0.0f;
	for (auto &&C : m_RootNodes) EvaluateNode(C);
	return;
}

uint32_t Scene::GetImageTexID(uint32_t Idx) {
	return m_ImageTexID[Idx];
}

Material &Scene::GetMaterial(uint32_t Idx) {
	return m_MaterialList[Idx];
}

float Scene::GetTotalTime(void) const {
	return m_TotalTime;
}

Scene::~Scene() {
}