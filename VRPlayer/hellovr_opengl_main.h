#pragma once
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <WinSock2.h>
#include <windows.h>
#include<time.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <string.h> 
#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <string>
#include <cstdlib>
#include "SDL_thread.h"
#include "openvr.h"
#include "lodepng.h"
#include "Matrices.h"
#include "pathtools.h"
#include "pyramid_rotation.h"
#include <windows.h>
#include <GLTools.h>	
#include <GLMatrixStack.h>
#include <GLFrame.h>
#include <GLFrustum.h>
#include <GLGeometryTransform.h>
#include <StopWatch.h>
#include <opencv2/cudawarping.hpp>
#include <math.h>
#include <stdlib.h>
#include <opencv2\core\opengl.hpp>
#include <GL/glut.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2\core\opengl.hpp>
#include <GL/glut.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "SDL_thread.h"
#include <fstream>
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include<time.h>
#include <cstdio>
#include <cstdlib>
#include <WS2tcpip.h>
#include<time.h>
#include <cstdio>
#include <cstdlib>
#include "RtpReceiver.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include "libavutil/audio_fifo.h"
#include <libswscale/swscale.h>
#include "libswresample/swresample.h"
#include <turbojpeg.h>
}
using namespace cv;
using namespace std;
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"ws2_32.lib")
//#define DEBUG    //读写文件
//#define FRAME_INFO //每帧输出信息
//#define RTP
#define TCP
//#define YUV
#define WID 1280 //YUV1344 1488 RTPTCP1280
#define BUFF_SIZE 1024
#define PIC_SIZE WID*WID
#define YUV_BUF WID*WID*12/8    //w*h*1.5
#define PORT_NUMBER 6666
//#define SERVER_IP "100.64.187.136"
#define SERVER_IP "127.0.0.1"
#define MaxLoadFrameBuffSize 100
class MDecode
{
protected:
	AVCodecParserContext *parser;
	AVCodecContext *context;
	AVFrame *frame;
	AVPacket *packet;
public:
	int decoder_init();
	void decoder_flush(uint8_t *pic_outbuff);
	//void decoder_flush(uint8_t *pic_inbuff, uint8_t *pic_outbuff, int data_size);
	void decoder_destroy();
	int decoder_decode(uint8_t *pic_inbuff, uint8_t *pic_outbuff, int data_size);
};
class TClient
{
public:
	/*Windows Socket的定义*/
	WSAData wsaData;
	SOCKET sockfd;
	char *pic_inbuff, *pic_outbuff;
	MDecode dec;
public:
	TClient();
	~TClient();
	int client_transfer_init(); //初始化socket操作
	int set_non_block(SOCKET socket);		//设置非阻塞方式
	int send_non_block(SOCKET socket, int length, int flags);//非阻塞方式发送数据
	int recv_non_block(SOCKET socket, int length, int flags);//非阻塞方式接收数据
	int connect_non_block(SOCKET socket, const struct sockaddr *address, int address_len); //非阻塞方式连接
	void client_transfer_destroy();//销毁客户端传输socket
	void client_excute();
};
struct Frame
{
	int viewpoint;
	cv::Mat Mbuff;
};
DWORD start_time, end_time;//帧率计时
char buffer[WID*WID*12/8];//480*480*1.5
WSAData wsaData;
SOCKET sockfd;
char *socketdata;
int f_len;
unsigned int seq_number;
Mat jpegimage;
unsigned char *rgb_buffer;
LARGE_INTEGER nFreq, nBeginTime, nEndTime, nBeginTime_1, nEndTime_1;
static bool g_bPrintf = true;
#define pi 3.141592653
double rate;//帧率
CvCapture * capture = NULL;
IplImage * frame;
Mat rgbImg[30];//读取图片
uint32_t viewPointIndex;
Vector3 v3;
M3DMatrix44f mModel;
SDL_Thread *LoadFrameThread;
int xx = 0;
pyramid_rotation pyramidRot;
ofstream fout("out.yuv", ios::out | ios::binary);
Mat test,test_0;
SDL_sem *Lock1 = SDL_CreateSemaphore(1);
SDL_sem *Lock2 = SDL_CreateSemaphore(0);//信号量
MDecode decoder;
uint16_t portbase = 8888, portdest = 6666;
std::string ipstr;
int status, num = 10;
RtpReciver sess;
TClient client;
Frame frame_fifo[MaxLoadFrameBuffSize];
int Mbuff1 = 2;//预存队首loadframe
int Mbuff2 = 2;//队尾loadframe
int pre_viewpoint = 9;
int cur_viewpoint = 9;
int temp_viewpoint = 0;
bool first_flag = true;
class CGLRenderModel
{
public:
	CGLRenderModel(const std::string & sRenderModelName);
	~CGLRenderModel();
	bool BInit(const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture);
	void Cleanup();
	void Draw();
	const std::string & GetName() const { return m_sModelName; }
private:
	GLuint m_glVertBuffer;
	GLuint m_glIndexBuffer;
	GLuint m_glVertArray;
	GLuint m_glTexture;
	GLsizei m_unVertexCount;
	std::string m_sModelName;
};
class CMainApplication
{
public:
	CMainApplication(int argc, char *argv[]);
	virtual ~CMainApplication();
	bool BInit();
	bool BInitGL();
	bool BInitCompositor();
	void SetupRenderModels();
	void Shutdown();
	void RunMainLoop();
	bool HandleInput();
	void ProcessVREvent(const vr::VREvent_t & event);
	void RenderFrame();
	bool SetupTexturemaps();
	void SetupScene();
	void AddCubeToScene(Matrix4 mat, std::vector<float> &vertdata);
	void AddPyramidToScene(Matrix4 mat, std::vector<float> &vertdata);///添加四棱锥贴图
	void AddCubeVertex(float fl0, float fl1, float fl2, float fl3, float fl4, std::vector<float> &vertdata);
	void RenderControllerAxes();
	bool SetupStereoRenderTargets();
	void SetupCompanionWindow();
	void SetupCameras();
	void RenderStereoTargets();
	void RenderCompanionWindow();
	void RenderScene(vr::Hmd_Eye nEye);
	Vector3 GetForward();///
	void PrintInformation();///
	void LoadFrame();
	Matrix4 GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
	Matrix4 GetHMDMatrixPoseEye(vr::Hmd_Eye nEye);
	Matrix4 GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye);
	void UpdateHMDMatrixPose();
	Matrix4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose);
	GLuint CompileGLShader(const char *pchShaderName, const char *pchVertexShader, const char *pchFragmentShader);
	bool CreateAllShaders();
	void SetupRenderModelForTrackedDevice(vr::TrackedDeviceIndex_t unTrackedDeviceIndex);
	CGLRenderModel *FindOrLoadRenderModel(const char *pchRenderModelName);
private:
	bool m_bDebugOpenGL;
	bool m_bVerbose;
	bool m_bPerf;
	bool m_bVblank;
	bool m_bGlFinishHack;
	vr::IVRSystem *m_pHMD;
	vr::IVRRenderModels *m_pRenderModels;
	std::string m_strDriver;
	std::string m_strDisplay;
	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	Matrix4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];
	bool m_rbShowTrackedDevice[vr::k_unMaxTrackedDeviceCount];
private: // SDL bookkeeping
	SDL_Window * m_pCompanionWindow;
	uint32_t m_nCompanionWindowWidth;
	uint32_t m_nCompanionWindowHeight;
	SDL_GLContext m_pContext;
private: // OpenGL bookkeeping
	int m_iTrackedControllerCount;
	int m_iTrackedControllerCount_Last;
	int m_iValidPoseCount;
	int m_iValidPoseCount_Last;
	bool m_bShowCubes;
	std::string m_strPoseClasses;                            // what classes we saw poses for this frame
	char m_rDevClassChar[vr::k_unMaxTrackedDeviceCount];   // for each device, a character representing its class
	int m_iSceneVolumeWidth;
	int m_iSceneVolumeHeight;
	int m_iSceneVolumeDepth;
	float m_fScaleSpacing;
	float m_fScale;
	int m_iSceneVolumeInit;                                  // if you want something other than the default 20x20x20
	float m_fNearClip;
	float m_fFarClip;
	GLuint m_iTexture;
	unsigned int m_uiVertcount;
	GLuint m_glSceneVertBuffer;
	GLuint m_unSceneVAO;
	GLuint m_unCompanionWindowVAO;
	GLuint m_glCompanionWindowIDVertBuffer;
	GLuint m_glCompanionWindowIDIndexBuffer;
	unsigned int m_uiCompanionWindowIndexSize;
	GLuint m_glControllerVertBuffer;
	GLuint m_unControllerVAO;
	unsigned int m_uiControllerVertcount;
	Matrix4 m_mat4HMDPose;
	Matrix4 m_mat4eyePosLeft;
	Matrix4 m_mat4eyePosRight;
	Matrix4 m_mat4ProjectionCenter;
	Matrix4 m_mat4ProjectionLeft;
	Matrix4 m_mat4ProjectionRight;
	struct VertexDataScene
	{
		Vector3 position;
		Vector2 texCoord;
	};
	struct VertexDataWindow
	{
		Vector2 position;
		Vector2 texCoord;
		VertexDataWindow(const Vector2 & pos, const Vector2 tex) : position(pos), texCoord(tex) {	}
	};
	GLuint m_unSceneProgramID;
	GLuint m_unCompanionWindowProgramID;
	GLuint m_unControllerTransformProgramID;
	GLuint m_unRenderModelProgramID;
	GLint m_nSceneMatrixLocation;
	GLint m_nControllerMatrixLocation;
	GLint m_nRenderModelMatrixLocation;
	struct FramebufferDesc
	{
		GLuint m_nDepthBufferId;
		GLuint m_nRenderTextureId;
		GLuint m_nRenderFramebufferId;
		GLuint m_nResolveTextureId;
		GLuint m_nResolveFramebufferId;
	};
	FramebufferDesc leftEyeDesc;
	FramebufferDesc rightEyeDesc;
	bool CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc);
	uint32_t m_nRenderWidth;
	uint32_t m_nRenderHeight;
	std::vector< CGLRenderModel * > m_vecRenderModels;
	CGLRenderModel *m_rTrackedDeviceToRenderModel[vr::k_unMaxTrackedDeviceCount];
};


