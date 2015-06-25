#ifndef _BRIDGE_H_
#define _BRIDGE_H_
#include <vector>
#include "cinder/Color.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Rand.h"

using namespace ci;
using namespace std;

namespace BB
{
	struct LED
	{
		vec3	Position;
		vec2	UV;
		Color	LedColor;
		bool	Active;
		float	Age,
				Lifespan;
		LED(){}
		LED(vec3 pPos, vec2 pUV) : Position(pPos), UV(pUV), LedColor(Color(0.25f,0.25f,0.25f)), Active(false)
		{
			Age = Lifespan = randFloat(30, 90);
		}

		LED(vec3 pPos, vec2 pUV, Color pColor) : Position(pPos), UV(pUV), LedColor(pColor), Active(false)
		{
			Age = Lifespan = randFloat(30, 90);
		}

		LED(const LED &source)
		{
			Position = source.Position;
			UV = source.UV;
			LedColor = source.LedColor;
			Active = source.Active;
			Lifespan = source.Lifespan;
			Age = Lifespan;
		}

		~LED(){}
	};

	struct Strand
	{
		bool IsMapped;
		bool IsActive;
		int StrandId;
		vec3 Position;
		vector<LED> Lights;

		Strand(){};
		Strand(int pId, vec3 pPos, int pNumLights);
		Strand(int pId, vec3 pPos, vector<LED> pLights);
	};

	class Bridge
	{
	public:
		Bridge(){}
		~Bridge(){}

		void Init(gl::GlslProgRef pShader);
		void Update(Channel16u pDepth, bool pInvert, bool pClear);
		void Draw();
		void BufferStrandData(bool pReload);

		vector<Strand>	Strands;
		vector<LED>		Caps;

	private:
		int				mNumElements;

		gl::VboRef		mStrandsVbo;
		gl::BatchRef	mStrandsBatch;
		gl::GlslProgRef	mStrandsShader;

		gl::VboRef		mCapsVbo;
		gl::BatchRef	mCapsBatch;
		gl::GlslProgRef	mCapsShader;
	};
}
#endif