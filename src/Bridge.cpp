#include "Bridge.h"

using namespace BB;
static vector<int> S_LIGHTS
{
	1, 1, 2, 5, 7, 11,
	13, 14, 18, 21, 24, 27,
	31, 36, 42, 47, 53, 60,
	67, 72, 80, 87, 94, 102,
	110, 120, 129, 137, 147, 158,
	169, 179, 190, 202, 213, 226,

	226, 213, 202, 190, 179, 169,
	158, 147, 136, 129, 120, 110,
	102, 94, 87, 80, 72, 67,
	60, 53, 47, 42, 36, 31,
	27, 24, 21, 18, 14, 13,
	11, 7, 5, 2, 1, 1
};

static ivec2 S_DEPTH(480, 360);
static vec2 S_RANGE(500.0f, 1000.0f);
static int S_STRANDS = 72;
static int S_INTERVAL = 30;
static float S_X_BOUND = 300;

Strand::Strand(int pId, vec3 pPos, int pNumLights) : Position(pPos), StrandId(pId)
{
	for (int i = 0; i < pNumLights; ++i)
	{
		Lights.push_back(LED(vec3(pPos.x, pPos.y+(i + 1), pPos.z), vec2(0)));
	}
}

Strand::Strand(int pId, vec3 pPos, vector<LED> pLights)
{

}

void Bridge::Init(gl::GlslProgRef pShader)
{
	mStrandsShader = pShader;
	mCapsShader = pShader;
	mNumElements = 0;

	for (int i = 0; i < S_LIGHTS.size(); ++i)
	{
		float xPos = -1050 + (i*S_INTERVAL - (S_INTERVAL / 2.0f));
		Caps.push_back(LED(vec3(xPos, S_LIGHTS[i] + 10, -40), vec2(0), Color::white()));
		Caps.push_back(LED(vec3(xPos, S_LIGHTS[i] + 10, 40), vec2(0), Color::white()));

		Strands.push_back(Strand(i, vec3(xPos, 0, -36), S_LIGHTS[i]));
		mNumElements += S_LIGHTS[i];

		Strands.push_back(Strand(i, vec3(xPos, 0, 36), S_LIGHTS[i]));
		mNumElements += S_LIGHTS[i];
	}

	//now setup UVs
	for (auto s = begin(Strands); s != end(Strands); ++s)
	{
		for (auto t = begin(s->Lights); t != end(s->Lights); ++t)
		{
			float u = -1.0f;
			float v = -1.0f;
			if (t->Position.x >= -S_X_BOUND&&t->Position.x <= S_X_BOUND)
			{
				u = lmap<float>(t->Position.x, -S_X_BOUND, S_X_BOUND, 0.0f, 1.0f);
				v = lmap<float>(t->Position.y, 226.0f, 0.0f, 0.0f, 1.0f);
			}
			t->UV = vec2(u, v);
		}
	}

	auto capsMesh = gl::VboMesh::create(geom::Sphere().radius(1.0f));
	mCapsVbo = gl::Vbo::create(GL_ARRAY_BUFFER, Caps, GL_DYNAMIC_DRAW);
	geom::BufferLayout	capsAttribs;
	capsAttribs.append(geom::CUSTOM_0, 3, sizeof(LED), offsetof(LED, Position), 1);
	capsAttribs.append(geom::CUSTOM_1, 3, sizeof(LED), offsetof(LED, LedColor), 1);
	capsMesh->appendVbo(capsAttribs, mCapsVbo);
	mCapsBatch = gl::Batch::create(capsMesh, mCapsShader, { { geom::CUSTOM_0, "iPosition" }, { geom::CUSTOM_1, "iColor" } });

	BufferStrandData(false);
}

void Bridge::BufferStrandData(bool pReload)
{
	vector<LED> lights;
	for (auto s = begin(Strands); s != end(Strands); ++s)
	{
		for (auto t = begin(s->Lights); t != end(s->Lights); ++t)
		{
			lights.push_back(LED(*t));
		}
	}

	if (!pReload)
	{
		auto strandMesh = gl::VboMesh::create(geom::Sphere().radius(0.6f).subdivisions(8));
		mStrandsVbo = gl::Vbo::create(GL_ARRAY_BUFFER, lights, GL_DYNAMIC_DRAW);

		geom::BufferLayout	strandAttribs;
		strandAttribs.append(geom::CUSTOM_0, 3, sizeof(LED), offsetof(LED, Position), 1);
		strandAttribs.append(geom::CUSTOM_1, 3, sizeof(LED), offsetof(LED, LedColor), 1);
		strandMesh->appendVbo(strandAttribs, mStrandsVbo);
		mStrandsBatch = gl::Batch::create(strandMesh, mStrandsShader, { { geom::CUSTOM_0, "iPosition" }, { geom::CUSTOM_1, "iColor" } });
	}
	else
		mStrandsVbo->bufferData(lights.size()*sizeof(LED), lights.data(), GL_DYNAMIC_DRAW);
}

void Bridge::Update(Channel16u pDepth, bool pInvert, bool pClear)
{
	for (auto s = begin(Strands); s != end(Strands); ++s)
	{
		for (auto t = begin(s->Lights); t != end(s->Lights);++t)
		{
			if (t->Position.x >= -S_X_BOUND && t->Position.x <= S_X_BOUND)
			{
				int x = t->UV.x*S_DEPTH.x;
				int y = t->UV.y*S_DEPTH.y;

				float d = (float)pDepth.getValue(ivec2(x, y));
				if (d >= S_RANGE.x&&d <= S_RANGE.y)
				{
					if (!t->Active)
						t->Active = true;

					if (t->Active&&t->Age > 0)
					{
						t->Age--;
						float c = lmap<float>(t->Age, 0, t->Lifespan, 0.0f, 1.0f);
						float m = lmap<float>(d, S_RANGE.x, S_RANGE.y, 1.0f, 0.0f);
						t->LedColor = Color(c,c,c)*m;
					}
					else if (t->Active&&t->Age <= 0)
					{
						t->Active = false;
						t->Age = t->Lifespan = randFloat(60, 180);
						t->LedColor = Color::black();
					}
				}
				else
				{
					if (!t->Active)
					{
						float c = 65.0f / 265.0f;
						t->LedColor = Color(c, c, c);
					}
					else
					{
						if (t->Age > 0)
						{
							t->Age--;
							float c = lmap<float>(t->Age, 0, t->Lifespan, 0.0f, 1.0f);
							t->LedColor = Color(c, c, c);
						}
						else
						{
							t->Active = false;
							t->Age = t->Lifespan = randFloat(30, 90);
							t->LedColor = Color::black();
						}
					}
				}
			}
			else
			{
				if (t->Age > 0)
				{
					t->Age--;
					float n = t->Age / t->Lifespan;
					t->LedColor = Color(n, n, n);
				}
				else
				{
					t->Lifespan = randFloat(120, 240);
					t->Age = t->Lifespan;
					t->LedColor = Color::white();
				}
			}
		}
	}

	BufferStrandData(true);
}

void Bridge::Draw()
{
	mStrandsBatch->drawInstanced(mNumElements);
	mCapsBatch->drawInstanced(Caps.size());
}
