#ifndef ANIMATION_H
#define ANIMATION_H
#include "Mesh.h"
#include <LWEGLTFParser.h>
#include <LWETween.h>

class Animation;

struct AnimTween {
	static bool Deserialize(AnimTween &Tween, LWByteBuffer &Buffer);

	LWSMatrix4f GetFrame(float Time, bool Loop = false);

	uint32_t Serialize(LWByteBuffer &Buf);

	float GetTotalTime(void);

	AnimTween() = default;

	AnimTween(const LWEGLTFAnimTween &GFTLAnimTween);

	LWETween<LWSVector4f> m_Translation;
	LWETween<LWSQuaternionf> m_Rotation;
	LWETween<LWSVector4f> m_Scale;
};

//Instance of animation.
struct AnimationInstance {
	static const uint32_t Playing = 0x1;
	static const uint32_t Loop = 0x2;
	static const uint32_t Finished = 0x4;

	//Transforms a point by the transform matrix's.
	static LWSVector4f TransformPoint(const LWSVector4f &Pos, const LWSMatrix4f *Transforms, uint32_t Idx);

	//Transforms a point by a weighted 4 index's.
	static LWSVector4f TransformPoint(const LWSVector4f &Pos, const LWSMatrix4f *Transforms, const LWVector4i &Indexs, const LWVector4f &Weights);

	AnimationInstance &Update(float dTime);

	AnimationInstance &SetTime(float Time);

	AnimationInstance &Play(void);

	AnimationInstance &Pause(void);

	AnimationInstance &SetLooping(bool isLooping);

	bool isLooping(void) const;

	bool isPlaying(void) const;

	bool isFinished(void) const;

	AnimationInstance(Animation *Anim, uint32_t Flags);

	AnimationInstance() = default;

	LWSMatrix4f m_TransformMatrixs[Mesh::MaxBones];
	Animation *m_Animation = nullptr;
	float m_Time = 0.0f;
	uint32_t m_Flag = 0;
};


class Animation {
public:
	
	static bool Deserialize(Animation &Anim, LWByteBuffer &Buf);

	void MakeGLTFSkin(LWEGLTFParser &P, LWEGLTFSkin *Skin);

	uint32_t Serialize(LWByteBuffer &Buf);

	Animation &Finished(void);

	Animation &SetNameHash(uint32_t NameHash);

	AnimTween &GetAnimation(uint32_t ID);

	//Constructs all animations linearly.
	uint32_t MakeAnimationTransform(float Time, bool Loop, LWSMatrix4f *TransformMatrixs, uint32_t TransformMatrixCount);

	//Constructs transform matrix's in order of mesh's bone ordering.
	uint32_t MakeBoneTransforms(float Time, bool Loop, Mesh *Msh, LWSMatrix4f *TransformMatrixs);

	uint32_t GetCount(void) const;

	float GetTotalTime(void) const;

	uint32_t GetNameHash(void) const;

	Animation(uint32_t Count);

	Animation() = default;
private:
	AnimTween m_AnimationList[Mesh::MaxBones];
	uint32_t m_Count = 0;
	uint32_t m_NameHash = 0;
	float m_TotalTime = 0;
};

#endif