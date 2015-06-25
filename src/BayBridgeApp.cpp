#ifdef _DEBUG
#pragma comment(lib, "DSAPI.dbg.lib")
#else
#pragma comment(lib, "DSAPI.lib")
#endif

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Camera.h"
#include "cinder/CameraUi.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/params/Params.h"
#include "Bridge.h"
#include "CiDSAPI.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace BB;
using namespace CinderDS;

class BayBridgeApp : public App
{
public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
	void cleanup() override;

private:
	void drawBridge(vec3 pColor, float pAmbient);
	void drawFbo();
	void updateBridge(Channel16u pDepth);
	void drawStrands();

	gl::BatchRef	mBridgeBatch;
	gl::GlslProgRef	mBridgeShader;

	CameraPersp		mCamera;
	CameraUi		mCamUi;

	Bridge			mBridge;

	CinderDSRef		mDS;

	gl::GlslProgRef	mShaderHiPass,
					mShaderBlur,
					mShaderSkybox,
					mShaderDiffuse;

	gl::TextureCubeMapRef	mTexSkybox;
	gl::BatchRef			mBatchSkybox;

	gl::FboRef	mFboRaw,
				mFboHiPass,
				mFboBlurU,
				mFboBlurV;

	params::InterfaceGlRef	mGUI;
	bool					mParamInvert,
							mParamClear;
};

void BayBridgeApp::setup()
{
	mGUI = params::InterfaceGl::create("Params", ivec2(300, 50));
	mParamInvert = false;
	mParamClear = true;
	mGUI->addParam<bool>("paramInvert", &mParamInvert).optionsStr("label='Inverted'");
	mGUI->addParam<bool>("paramClear", &mParamClear).optionsStr("label='Clear Frame'");

	getWindow()->setSize(1280, 720);
	getWindow()->setPos(ivec2(20));
	setFrameRate(60);
	mBridgeShader = gl::GlslProg::create(loadAsset("shaders/light_vert.glsl"), loadAsset("shaders/light_frag.glsl"));
	
	mBridge.Init(mBridgeShader);

	mCamera.setPerspective(45.0f, getWindowAspectRatio(), 10.0f, 10000.0f);
	mCamera.lookAt(vec3(0, 50, -525), vec3(0,50,0), vec3(0, 1, 0));
	mCamUi = CameraUi(&mCamera, getWindow());

	mDS = CinderDSAPI::create();
	mDS->init();
	mDS->initDepth(FrameSize::DEPTHSD, 60);
	mDS->start();

	ImageSourceRef skybox[6]
	{
		loadImage(loadAsset("textures/px04.png")), loadImage(loadAsset("textures/nx04.png")),
		loadImage(loadAsset("textures/py04.png")), loadImage(loadAsset("textures/ny04.png")),
		loadImage(loadAsset("textures/pz04.png")), loadImage(loadAsset("textures/nz04.png"))
	};

	mTexSkybox = gl::TextureCubeMap::create(skybox);
	mShaderSkybox = gl::GlslProg::create(loadAsset("shaders/skybox_vert.glsl"), loadAsset("shaders/skybox_frag.glsl"));
	mBatchSkybox = gl::Batch::create(geom::Cube(), mShaderSkybox);
	mBatchSkybox->getGlslProg()->uniform("uCubemapSampler", 0);

	mShaderDiffuse = gl::GlslProg::create(loadAsset("shaders/diffuse_vert.glsl"), loadAsset("shaders/diffuse_frag.glsl"));

	//Bloom
	mFboRaw = gl::Fbo::create(1280, 720, gl::Fbo::Format().colorTexture(gl::Texture2d::Format().dataType(GL_FLOAT).internalFormat(GL_RGBA32F)));
	mFboHiPass = gl::Fbo::create(1280, 720);
	mFboBlurU = gl::Fbo::create(1280, 720);
	mFboBlurV = gl::Fbo::create(1280, 720);

	mShaderHiPass = gl::GlslProg::create(loadAsset("shaders/passthru_vert.glsl"), loadAsset("shaders/highpass_frag.glsl"));
	mShaderHiPass->uniform("uTextureSampler", 0);

	mShaderBlur = gl::GlslProg::create(loadAsset("shaders/blur_vert.glsl"), loadAsset("shaders/blur_frag.glsl"));
	mShaderBlur->uniform("uTextureSampler", 0);
}

void BayBridgeApp::mouseDown( MouseEvent event )
{
}

void BayBridgeApp::update()
{
	mDS->update();
	mBridge.Update(mDS->getDepthFrame(), mParamInvert, mParamClear);
	drawFbo();
}

void BayBridgeApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );

	gl::setMatrices(mCamera);
	
	gl::disableDepthRead();
	mTexSkybox->bind(0);
	mBatchSkybox->draw();
	mTexSkybox->unbind(0);
	gl::enableDepthRead();

	drawBridge(vec3(0.25f), 0.15f);
	mBridge.Draw();

	gl::setMatricesWindow(getWindowSize());
	gl::enableAdditiveBlending();
	gl::disableDepthRead();
	gl::draw(mFboBlurV->getColorTexture(), vec2(0));
	gl::disableAlphaBlending();

	mGUI->draw();
}

void BayBridgeApp::drawFbo()
{
	mFboRaw->bindFramebuffer();
	gl::clear(Color::black());
	gl::setMatrices(mCamera);
	drawBridge(vec3(0), 0.0f);
	mBridge.Draw();
	mFboRaw->unbindFramebuffer();

	////////////////////////////////////////////////////
	mFboHiPass->bindFramebuffer();
	mFboRaw->bindTexture(0);
	mShaderHiPass->bind();
	gl::enableAlphaBlending();
	gl::clear(ColorA::zero());
	gl::setMatricesWindow(getWindowSize());
	gl::drawSolidRect(Rectf({vec2(0), getWindowSize()}));
	gl::disableAlphaBlending();
	mFboRaw->unbindTexture(0);
	mFboHiPass->unbindFramebuffer();

	////////////////////////////////////////////////////
	mFboBlurU->bindFramebuffer();
	mFboHiPass->bindTexture(0);
	mShaderBlur->bind();
	mShaderBlur->uniform("uBlurAxis", vec2(1.0, 0.0));
	mShaderBlur->uniform("uBlurStrength", 2.0f);
	mShaderBlur->uniform("uBlurSize", 1.0f);
	gl::clear(ColorA::zero());
	gl::setMatricesWindow(getWindowSize());
	gl::drawSolidRect(Rectf({ vec2(0), getWindowSize() }));
	mFboHiPass->unbindTexture(0);
	mFboBlurU->unbindFramebuffer();

	////////////////////////////////////////////////////
	mFboBlurV->bindFramebuffer();
	mFboBlurU->bindTexture(0);
	mShaderBlur->bind();
	mShaderBlur->uniform("uBlurAxis", vec2(0.0, 1.0));
	mShaderBlur->uniform("uBlurStrength", 2.0f);
	mShaderBlur->uniform("uBlurSize", 2.0f);
	gl::clear(ColorA::zero());
	gl::setMatricesWindow(getWindowSize());
	gl::drawSolidRect(Rectf({ vec2(0), getWindowSize() }));
	mFboBlurU->unbindTexture(0);
	mFboBlurV->unbindFramebuffer();
}

void BayBridgeApp::drawBridge(vec3 pColor, float pAmbient)
{
	mShaderDiffuse->bind();
	mShaderDiffuse->uniform("uColor", pColor);
	mShaderDiffuse->uniform("uAmbient", pAmbient);
	gl::drawCube(vec3(0, -2, 0), vec3(1050, 5, 30));
	gl::drawCube(vec3(500, -25, 0), vec3(30,50,30));
	gl::drawCube(vec3(-500, -25, 0), vec3(30, 50, 30));
	gl::drawCube(vec3(0, 30, 0), vec3(5, 180, 50));
}

void BayBridgeApp::cleanup()
{
	mDS->stop();
}


CINDER_APP( BayBridgeApp, RendererGl( RendererGl::Options().msaa(8) ) )
