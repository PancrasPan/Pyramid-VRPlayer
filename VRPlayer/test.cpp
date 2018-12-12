#include "hellovr_opengl_main.h"
Mat avframe_to_cvmat(AVFrame *frame)
{
	AVFrame dst;
	cv::Mat m;

	memset(&dst, 0, sizeof(dst));

	int w = frame->width, h = frame->height;
	m = cv::Mat(h, w, CV_8UC3);
	dst.data[0] = (uint8_t *)m.data;
	avpicture_fill((AVPicture *)&dst, dst.data[0], AV_PIX_FMT_BGR24, w, h);

	struct SwsContext *convert_ctx = NULL;
	enum AVPixelFormat src_pixfmt = (enum AVPixelFormat)frame->format;
	enum AVPixelFormat dst_pixfmt = AV_PIX_FMT_BGR24;
	convert_ctx = sws_getContext(w, h, src_pixfmt, w, h, dst_pixfmt,
		SWS_FAST_BILINEAR, NULL, NULL, NULL);
	sws_scale(convert_ctx, frame->data, frame->linesize, 0, h,
		dst.data, dst.linesize);
	sws_freeContext(convert_ctx);

	return m;
}
/*�����׽���Ϊ������ģʽ*/
int set_non_Block(SOCKET socket)
{
	/*��ʶ����0���������ģʽ*/
	int ret;
	unsigned long flag = 1;
	ret = ioctlsocket(socket, FIONBIO, (u_long*)&flag);
	if (ret)
		printf("set nonblock error: (errno: %d)\n", WSAGetLastError());
	return ret;
}
/*�������׽��ֵ�connect*/
int connect_non_Block(SOCKET socket, const struct sockaddr *address, int address_len)
{
	int sel;
	struct timeval tm;
	if (connect(socket, address, address_len) < 0)
	{
		fd_set wfd;
		FD_ZERO(&wfd);
		FD_SET(socket, &wfd);
		tm.tv_sec = 3;    //3��
		tm.tv_usec = 1;    //1u��

		sel = select(socket + 1, NULL, &wfd, NULL, NULL);
		if (sel <0)
		{
			printf("select socket error: (errno: %d)\n", WSAGetLastError());
			return -1;
		}
		else if (sel == 0) {
			printf("connect error: (errno: %d)\n", WSAGetLastError());
		}
		else {
			printf("Connet successfully!\n");
		}
	}
	return 0;
}
//��ʼ��socket����
int client_transfer_Init(WSAData *wsaData, SOCKET *sockfd)
{
	SOCKADDR_IN servaddr;
	//��ʼ��Window Socket
	if (WSAStartup(MAKEWORD(2, 2), wsaData)) {
		printf("Fail to initialize windows socket!\n");
		return -1;
	}
	//����һ���׽���
	if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("create socket error: (errno: %d)\n", WSAGetLastError());
		return 1;
	}

	/*�����׽���Ϊ������ģʽ*/
	if (set_non_Block(*sockfd)) {
		closesocket(*sockfd);
		return -1;
	}

	/*��ʼ���׽���*/
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT_NUMBER);
	/*client�������ַ*/
	if (inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr) == -1) {
		printf("inet_pton error for %s\n", SERVER_IP);
		return -1;
	}

	/*����server��*/
	if (connect_non_Block(*sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
		closesocket(*sockfd);
		return -1;
	}
	////////
	printf("Start transfer!\n");
	return 0;
}
/*�������׽��ֵ�recv*/
int recv_non_Block(SOCKET socket, char *buffer, int length, int flags)
{
	int recv_len, ret_val, sel;
	struct timeval tm;

	for (recv_len = 0; recv_len < length;)
	{
		/*�ö���*/
		fd_set read_fd;
		FD_ZERO(&read_fd);
		FD_SET(socket, &read_fd);
		//��1s�ղ������ݾͷ���
		tm.tv_sec = 0;    //��
		tm.tv_usec = 10000;    //1u��

		sel = select(socket + 1, &read_fd, NULL, NULL, &tm);  /*����select*/
		if (sel < 0) {   //����ʧ��
			printf("select socket error: (errno: %d)\n", WSAGetLastError());
			return -1;
		}
		else if (sel == 0) { //��ʱ���ؽ��յ���С����
			printf("Receive time out!(errno: %d) length=%d\n", WSAGetLastError(), recv_len);
			return recv_len;
		}
		else {
			if (FD_ISSET(socket, &read_fd)) { //��������ɶ�
				ret_val = recv(socket, buffer + recv_len, length - recv_len, flags);
				if (ret_val < 0) {
					printf("recv error\n");
					return ret_val;
				}
				else if (ret_val == 0) {
					printf("connection closed\n");
					return ret_val;
				}
				else
					recv_len += ret_val;
			}
		}
	}
	return recv_len;
}
/*�������׽��ֵ�send*/
int send_non_Block(SOCKET socket, char *buffer, int length, int flags)
{
	int send_len, ret_val, sel;
	struct timeval tm;

	for (send_len = 0; send_len < length;)
	{
		/*��д��*/
		fd_set write_fd;
		FD_ZERO(&write_fd);
		FD_SET(socket, &write_fd);
		//��1s���������ݾͷ���
		tm.tv_sec = 1;    //1��
		tm.tv_usec = 1;    //1u��

						   /*����select*/
		sel = select(socket + 1, NULL, &write_fd, NULL, &tm);
		if (sel < 0) {   //����ʧ��
			printf("select socket error: (errno: %d)\n", WSAGetLastError());
			return -1;
		}
		else if (sel == 0) {
			printf("Send time out! (errno: %d)\n", WSAGetLastError());
			return send_len;
		}
		else {
			if (FD_ISSET(socket, &write_fd)) { //���������д
				ret_val = send(socket, buffer + send_len, length - send_len, flags);
				if (ret_val < 0) {
					printf("send error\n");
					return -2;
				}
				else if (ret_val == 0) {
					printf("connection closed\n");
					return 0;
				}
				else
					send_len += ret_val;
			}
		}
	}
	return send_len;
}
//���ٿͻ��˴���socket
void client_transfer_Destroy(SOCKET *socket)
{
	closesocket(*socket);   //�ͻ��˹ر�����
	WSACleanup();
}
//��������ʼ��
int MDecode::decoder_init()
{

	const AVCodec *codec;

	//����x264�Ľ�����
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!codec) {
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}
	context = avcodec_alloc_context3(codec);
	if (!context) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}
	//���ձ��������ͳ�ʼ������������
	parser = av_parser_init(codec->id);
	if (!parser) {
		fprintf(stderr, "parser not found\n");
		exit(1);
	}

	//��msmpeg4�Լ�mpeg4���͵ı����������ڴ˴�������Ⱥ͸߶ȣ���Ϊ����bit���ﲻ���������ļ�
	//��avcodec_alloc_context3֮��ʹ�ã���ʼ��AVCodeContext�Ա�ʹ��AVCodeContext
	if (avcodec_open2(context, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	//������������ݰ��Լ�֡
	packet = av_packet_alloc();
	if (!packet)
		exit(1);
	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}


	return 1;
}
//flush������
void MDecode::decoder_flush(uint8_t *pic_outbuff)
{
	int ret;
	ret = avcodec_send_packet(context, NULL);
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		exit(1);
	}
	while (ret >= 0) {
		ret = avcodec_receive_frame(context, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)  //���뻹δ���
			return;
		else if (ret < 0) {
			fprintf(stderr, "Error during decoding\n"); //�������
			exit(1);
		}
#ifdef FRAME_INFO
		printf("saving frame %3d,width=%d,height=%d\nFlush", context->frame_number, context->width, context->height);
#endif // FRAME_INFO
		fflush(stdout);

		SDL_SemWait(Lock1);
		if ((Mbuff1 + 1) % MaxLoadFrameBuffSize != Mbuff2) {
			Mbuff[Mbuff1] = avframe_to_cvmat(frame);
			Mbuff1 = (Mbuff1 + 1) % MaxLoadFrameBuffSize;
		}
		SDL_SemPost(Lock1);


		//�����������ͼƬ�������ͷŸ�buffer 
	/*	memcpy(pic_outbuff, (char*)frame->data[0], PIC_SIZE);
		memcpy(pic_outbuff + PIC_SIZE, (char*)frame->data[1], PIC_SIZE / 4);
		memcpy(pic_outbuff + PIC_SIZE * 5 / 4, (char*)frame->data[2], PIC_SIZE / 4);*/
#ifdef DEBUG
		//fout.write((char*)pic_outbuff, PIC_SIZE);
#endif // DEBUG
	}
}
//�ͷŽ�����ռ���ڴ�
void MDecode::decoder_destroy()
{
#ifdef DEBUG
	fout.close();
#endif // DEBUG
	av_parser_close(parser);
	avcodec_free_context(&context);
	av_frame_free(&frame);
	av_packet_free(&packet);
}
//�������� ���֡parser ���������c �����pkt �������֡frame 
int MDecode::decoder_decode(uint8_t *pic_inbuff, uint8_t *pic_outbuff, int data_size)
{
	int ret;
	uint8_t *data;
	data = pic_inbuff;
	//ʹ��parser����������ݽ���Ϊ֡
	while (data_size> 0) {
		ret = av_parser_parse2(parser, context, &packet->data, &packet->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (ret < 0) {
			fprintf(stderr, "Error while parsing\n");
			exit(1);
		}
		data += ret;   //����ָ�����
		data_size -= ret; //��ǰʹ�õ�������

		//printf_s("ret=%d,data_size=%d,pkt->size=%d\n", ret, data_size, packet->size);
		if (packet->size) { //һ֡�ָ����,����һ֡
			ret = avcodec_send_packet(context, packet);
			if (ret < 0) {
				fprintf(stderr, "Error sending a packet for decoding\n");
				exit(1);
			}
			while (ret >= 0) {
				ret = avcodec_receive_frame(context, frame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)  //���뻹δ���
					break;
				else if (ret < 0) {
					fprintf(stderr, "Error during decoding\n"); //�������
					exit(1);
				}
#ifdef FRAME_INFO
				printf("saving frame %3d,colorspcae=%d,format=%d ,width=%d,height=%dDecode\n", context->frame_number, frame->colorspace, frame->format, frame->width, frame->height);
#endif // FRAME_INFO
				fflush(stdout);
				
				//test = avframe_to_cvmat(frame);
				
				SDL_SemWait(Lock1);
				if ((Mbuff1 + 1) % MaxLoadFrameBuffSize != Mbuff2) {					
					Mbuff[Mbuff1] = avframe_to_cvmat(frame);					
					Mbuff1 = (Mbuff1 + 1) % MaxLoadFrameBuffSize;
				}
				SDL_SemPost(Lock1);

				//�����������ͼƬ�������ͷŸ�buffer 
				/*memcpy(pic_outbuff, (char*)frame->data[0], PIC_SIZE);
				memcpy(pic_outbuff + PIC_SIZE, (char*)frame->data[1], PIC_SIZE / 4);
				memcpy(pic_outbuff + PIC_SIZE * 5 / 4, (char*)frame->data[2], PIC_SIZE / 4);*/
#ifdef DEBUG
				fout.write((char*)frame->data[0], PIC_SIZE);
				fout.write((char*)frame->data[1], PIC_SIZE / 4);
				fout.write((char*)frame->data[2], PIC_SIZE / 4);
#endif // DEBUG

			}
		}
	}
	return 0;
}
//
void ProcessCloudVRPacket(RtpReciver &sess, const RTPSourceData &srcdat, const RTPPacket &rtppack)
{

	if (rtppack.GetPayloadType() == H264)
	{
		if (rtppack.HasMarker())//��������һ����������
		{
			//m_ReceiveArray.Add(m_pVideoData);//��ӵ����ն���
			memcpy(sess.m_buffer + sess.m_current_size, rtppack.GetPayloadData(), rtppack.GetPayloadLength());
			sess.m_current_size += rtppack.GetPayloadLength();

			decoder.decoder_decode(sess.m_buffer, sess.m_outbuffer, sess.m_current_size);
			//decoder.decoder_flush((uint8_t*)sess.m_outbuffer);
			//std::cout << "receive h.264 nalu:" << sess.m_current_size << std::endl<<std::endl;
			memset(sess.m_buffer, 0, sess.m_current_size);//��ջ��棬Ϊ�´���׼��
			sess.m_current_size = 0;
		}
		else
		{
			unsigned char* p = rtppack.GetPayloadData();


			memcpy(sess.m_buffer + sess.m_current_size, rtppack.GetPayloadData(), rtppack.GetPayloadLength());
			sess.m_current_size += rtppack.GetPayloadLength();

			decoder.decoder_decode(sess.m_buffer, sess.m_outbuffer, sess.m_current_size);
			sess.m_current_size = 0;
		}
	}
	else if (rtppack.GetPayloadType() == VIEWPOINT)
	{
		memcpy(&(sess.viewpoint), rtppack.GetPayloadData(), rtppack.GetPayloadLength());
		printf("update viewpoint %d\n", sess.viewpoint);
	}

}

/**/
void conv_yuv420_to_mat(Mat &dst, unsigned char* pYUV420, int width, int height)
{
	if (!pYUV420) {
		return;
	}

	IplImage *yuvimage, *rgbimg, *yimg, *uimg, *vimg, *uuimg, *vvimg;

	int nWidth = width;
	int nHeight = height;
	rgbimg = cvCreateImage(cvSize(nWidth, nHeight), IPL_DEPTH_8U, 3);
	yuvimage = cvCreateImage(cvSize(nWidth, nHeight), IPL_DEPTH_8U, 3);

	yimg = cvCreateImageHeader(cvSize(nWidth, nHeight), IPL_DEPTH_8U, 1);
	uimg = cvCreateImageHeader(cvSize(nWidth / 2, nHeight / 2), IPL_DEPTH_8U, 1);
	vimg = cvCreateImageHeader(cvSize(nWidth / 2, nHeight / 2), IPL_DEPTH_8U, 1);

	uuimg = cvCreateImage(cvSize(nWidth, nHeight), IPL_DEPTH_8U, 1);
	vvimg = cvCreateImage(cvSize(nWidth, nHeight), IPL_DEPTH_8U, 1);

	cvSetData(yimg, pYUV420, nWidth);
	cvSetData(uimg, pYUV420 + nWidth * nHeight, nWidth / 2);
	cvSetData(vimg, pYUV420 + long(nWidth*nHeight*1.25), nWidth / 2);
	cvResize(uimg, uuimg, CV_INTER_LINEAR);
	cvResize(vimg, vvimg, CV_INTER_LINEAR);

	cvMerge(yimg, uuimg, vvimg, NULL, yuvimage);
	cvCvtColor(yuvimage, rgbimg, CV_YCrCb2RGB);

	cvReleaseImage(&uuimg);
	cvReleaseImage(&vvimg);
	cvReleaseImageHeader(&yimg);
	cvReleaseImageHeader(&uimg);
	cvReleaseImageHeader(&vimg);

	cvReleaseImage(&yuvimage);

	//dst = Mat(*rgbimg,int(1));
	dst = cvarrToMat(rgbimg, true);
	//rgbimg->
	cvReleaseImage(&rgbimg);
}
int DoubleToDegree(float d) {
	if (d < 0.0)
		return 360 + d * 180 / pi + 0.5f;
	else
		return d * 180 / pi;

}
int XYZToDegree(Vector3 v3) {//x:back y:right z:up
	int l, v;//0-360 ����
	float r;
	l = DoubleToDegree(atan2(v3.y, v3.x));//ˮƽx����н� level
	r = hypot(v3.x, v3.y);//ֱ��������б�߳�
	v = DoubleToDegree(atan2(r, v3.z));//��ֱz����н� vertl
	cout << "ˮƽ�нǺʹ�ֱ�нǣ�" << l << ' ' << v << endl;
	if (v <= 15) return 0;
	if (15 <= v&v< 45) {
		if (0 <= l < 180) return 1;
		else if (180 <= l&l<360)return 2;
	}
	if (45 <= v & v< 75) {
		if (60 <= l & l < 120)return 3;
		if (0 <= l & l< 60)return 4;
		if (300 <= l & l < 360)return 5;
		if (240 <= l & l< 300)return 6;
		if (180 <= l & l < 240)return 7;
		if (120 <= l & l < 180)return 8;
	}
	if (75 <= v & v < 105) {
		if (75 <= l & l < 105)return 9;
		if (45 <= l & l < 75)return 10;
		if (15 <= l & l < 45)return 11;
		if ((345 <= l & l< 360) || (0 <= l & l < 15))return 12;
		if (315 <= l & l< 345)return 13;
		if (285 <= l & l< 315)return 14;
		if (255 <= l & l< 285)return 15;
		if (225 <= l & l < 255)return 16;
		if (195 <= l & l < 225)return 17;
		if (165 <= l & l< 195)return 18;
		if (135 <= l & l< 165)return 19;
		if (105 <= l & l< 135)return 20;
	}
	if (105 <= v & v < 135) {
		if (60 <= l & l < 120)return 21;
		if (0 <= l & l < 60)return 22;
		if (300 <= l & l < 360)return 23;
		if (240 <= l & l < 300)return 24;
		if (180 <= l & l < 240)return 25;
		if (120 <= l & l < 180)return 26;
	}
	if (135 <= v & v < 165) {
		if (0 <= l&l < 180)return 27;
		else return 28;
	}
	if (165 <= v)return 29;
}
void ThreadSleep(unsigned long nMilliseconds)
{
#if defined(_WIN32)
	::Sleep(nMilliseconds);
#elif defined(POSIX)
	usleep(nMilliseconds * 1000);
#endif
}
void dprintf(const char *fmt, ...)
{
	va_list args;
	char buffer[2048];
	va_start(args, fmt);
	vsprintf_s(buffer, fmt, args);
	va_end(args);
	if (g_bPrintf)
		printf("%s", buffer);
	OutputDebugStringA(buffer);
}
Vector3 CMainApplication::GetForward() {
	Matrix4 x4 = m_mat4HMDPose;
	x4.transpose();
	//Matrix3 x3(0., 1., 0., 0., 0., 1., 1., 0., 0.);///����ϵת up:z right:y back:x
	Vector3 v3(0, 0, 1);
	v3 = (-x4)*v3;// ����ϵ�����ӽ�
				  //v3 =x3* ((-x4)*v3);
	return v3;
}
/**/
int jpeg2rgb(const unsigned char *jpeg_buffer,int jpeg_size,  unsigned char* rgb_buffer)
{
	tjhandle handle = NULL;    
	int width, height, subsample, colorspace;    
	int flags = 0;    
	int pixelfmt = TJPF_BGR;
	handle = tjInitDecompress();    
	tjDecompressHeader3(handle, jpeg_buffer, jpeg_size, &width, &height, &subsample, &colorspace);     
	flags |= 0;    
	tjDecompress2(handle, jpeg_buffer, jpeg_size, rgb_buffer, width, 0, height, pixelfmt, flags);     
	tjDestroy(handle);   
	return 0;
}
int GetFrame(void* xx) {	
	if (client_transfer_Init(&wsaData, &sockfd) == -1) {
		printf("Socket error!\n");
		exit(1);
	}
	SDL_SemWait(Lock2);
	while (1) {		
		pyramidRot.updateViewpoint(v3[0], v3[1], v3[2]);		
		viewPointIndex = ntohl(pyramidRot.getFrameIndex());
		send_non_Block(sockfd, (char*)&viewPointIndex, 4, 0);
		ThreadSleep(10);	
		recv_non_Block(sockfd, buffer, 2709504, 0);
		QueryPerformanceCounter(&nBeginTime);
		SDL_SemWait(Lock1);					
		//jpeg2rgb((const unsigned char *)buffer, f_len, rgb_buffer);		
		conv_yuv420_to_mat(test, (unsigned char*)&buffer, WID, WID);
		//transpose(test, test_0);
		//flip(test_0, test,1);
		
		//imshow("test", test);
		pyramidRot.getRotationMatrix(mModel);
		SDL_SemPost(Lock1);
		QueryPerformanceCounter(&nEndTime);
		//cout << (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart*1000 <<"ms"<< endl;				
	}
	
	//while (1)
	//{
	//	/*pyramidRot.updateViewpoint(v3[0], v3[1], v3[2]);
	//	viewPointIndex = pyramidRot.getFrameIndex();
	//	sess.SendPacket(&viewPointIndex, sizeof(int), VIEWPOINT, 1, 30000);*/
	//	SDL_SemWait(Lock2);
	//	sess.BeginDataAccess();
	//	//check incoming packets
	//	if (sess.GotoFirstSourceWithData())
	//	{
	//		do
	//		{
	//			RTPPacket *pack;
	//			RTPSourceData *srcdat;
	//			srcdat = sess.GetCurrentSourceInfo();
	//			while ((pack = sess.GetNextPacket()) != NULL)
	//			{
	//				//ProcessRTPPacket(*srcdat, *pack);
	//				ProcessCloudVRPacket(sess, *srcdat, *pack);
	//				sess.DeletePacket(pack);
	//			}
	//		} while (sess.GotoNextSourceWithData());
	//	}
	//	sess.EndDataAccess();
	//}
	

	return 0;
}
void CMainApplication::PrintInformation() {
	//HANDLE hout;
	//COORD coord;
	//coord.X = 0;
	//coord.Y = 2;
	//hout = GetStdHandle(STD_OUTPUT_HANDLE);
	//SetConsoleCursorPosition(hout, coord);

	system("cls");
	//printf("frame rate: %f \n", rate);
	cout << "eye coordinates(right-handed����" << v3 << endl;
	cout << "view point index:" << viewPointIndex << endl;
}
Matrix4 CMainApplication::GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye)
{///ͶӰ����*�ۺ�ͷת������*ͷ����
	Matrix4 matMVP;
	Matrix4 mModelRotation(mModel);
	//mModelRotation.transpose();
	if (nEye == vr::Eye_Left)
	{
		matMVP = m_mat4ProjectionLeft * m_mat4eyePosLeft * m_mat4HMDPose *mModelRotation;
	}
	else if (nEye == vr::Eye_Right)
	{
		matMVP = m_mat4ProjectionRight * m_mat4eyePosRight *  m_mat4HMDPose *mModelRotation;
	}
	return matMVP;
}
void CMainApplication::UpdateHMDMatrixPose()
{///ppp ÿ֡�����������
	if (!m_pHMD)
		return;
	vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);
	m_iValidPoseCount = 0;
	m_strPoseClasses = "";
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
	{
		if (m_rTrackedDevicePose[nDevice].bPoseIsValid)
		{
			m_iValidPoseCount++;
			m_rmat4DevicePose[nDevice] = ConvertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
			if (m_rDevClassChar[nDevice] == 0)
			{
				switch (m_pHMD->GetTrackedDeviceClass(nDevice))
				{
				case vr::TrackedDeviceClass_Controller:        m_rDevClassChar[nDevice] = 'C'; break;
				case vr::TrackedDeviceClass_HMD:               m_rDevClassChar[nDevice] = 'H'; break;
				case vr::TrackedDeviceClass_Invalid:           m_rDevClassChar[nDevice] = 'I'; break;
				case vr::TrackedDeviceClass_GenericTracker:    m_rDevClassChar[nDevice] = 'G'; break;
				case vr::TrackedDeviceClass_TrackingReference: m_rDevClassChar[nDevice] = 'T'; break;
				default:                                       m_rDevClassChar[nDevice] = '?'; break;
				}
			}
			m_strPoseClasses += m_rDevClassChar[nDevice];
		}
	}
	if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd];
		m_mat4HMDPose.setColumn(3, Vector4(0, 0, 0, 1));///rotate only ��ͼ����=��ת����*ƽ�ƾ���  ƽ�ƾ�����E
														//m_mat4HMDPose *= LookAt;///�����ӵ�
		m_mat4HMDPose.invert();
	}
}
void CMainApplication::RunMainLoop()
{///ppp
	bool bQuit = false;
	SDL_StartTextInput();
	SDL_ShowCursor(SDL_DISABLE);
	int ct1 = 0,ct2 = 10;
	start_time = GetTickCount();
	
	while (!bQuit)//bQuit = HandleInput();
	{
		/*if (ct2 == 10) {
			v3 = GetForward();
			pyramidRot.updateViewpoint(v3[0], v3[1], v3[2]);
			viewPointIndex =pyramidRot.getFrameIndex();
			sess.SendPacket(&viewPointIndex, sizeof(int), VIEWPOINT, 1, 30000);
			ct2 = 0;
			SDL_SemPost(Lock2);
			
		}	*/
		//pyramidRot.updateViewpoint(v3[0], v3[1], v3[2]);
		//viewPointIndex = pyramidRot.getFrameIndex();
		
		ThreadSleep(20);
		v3 = GetForward();
		//SDL_SemWait(Lock1);
		//if (Mbuff2 != Mbuff1) {			
			//test = Mbuff[Mbuff2];	
			//char file[16];
			//sprintf(file, "test_%d.jpg", ct1++);
			//imwrite(file,test);
			//Mbuff2 = (Mbuff2 + 1) % MaxLoadFrameBuffSize;
		//}
		//SDL_SemPost(Lock1);
		//test = imread("test_0.jpg");
		RenderFrame();
		glBindTexture(GL_TEXTURE_2D, m_iTexture);	
		SDL_SemWait(Lock1);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WID, WID, 0, GL_BGR, GL_UNSIGNED_BYTE, &rgb_buffer[0]);//jpegimage.data);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WID, WID, 0, GL_BGR, GL_UNSIGNED_BYTE, test.data);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WID, WID, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, test.data);		
		SDL_SemPost(Lock1);
		glGenerateMipmap(GL_TEXTURE_2D);	

		/*if (++ct1 == 30) {
			ct1 = 0;
			end_time = GetTickCount();
			printf("���Ŷ˲��֡�� %f \n", 30000.0 / (end_time - start_time));
			start_time = end_time;
			QueryPerformanceCounter(&nEndTime);
			printf("���֡��: %f\n", 30.0/((double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart));
			nBeginTime = nEndTime;
		}*/

		//ct2++;
	}
	SDL_StopTextInput();
}
bool CMainApplication::SetupTexturemaps()
{///ppp ��ʼ�� ��ȡͼƬ
	char file_dir[32];
	for (int i = 0; i < 30; i++)
	{
		sprintf(file_dir, "./small/%d.jpg", i);
		rgbImg[i] = imread(file_dir);
	}
	Mat rgbimg = rgbImg[9];
	/*capture = cvCreateFileCapture(VideoAddr);
	rate = (double)cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
	frame = cvQueryFrame(capture);
	cvarrToMat(frame).copyTo(rgbImg);*/
	if (rgbimg.cols != rgbimg.rows | rgbimg.rows % 4 != 0 | rgbimg.cols % 4 != 0) {
		cout << "�ֱ��������⣡" << endl;
		getchar();
		return false;
	}
	cout << "1" << endl;
	glGenTextures(1, &m_iTexture);
	glBindTexture(GL_TEXTURE_2D, m_iTexture);
	//
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rgbimg.cols, rgbimg.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, rgbimg.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
	glBindTexture(GL_TEXTURE_2D, 0);
	return (m_iTexture != 0);
}
void CMainApplication::AddPyramidToScene(Matrix4 mat, std::vector<float> &vertdata)
{///ppp ���Ƶ���άͼ�κͶ�άͼƬ�����
 //x�ұ� y���� z����      u�� v��     triangles instead of quads
	AddCubeVertex(0.f, 0.f, 3.16987f, 0.0f, 1.0f, vertdata);//O
	AddCubeVertex(-5.f, 0.f, -1.830127f, 0.0f, 0.5f, vertdata);//A
	AddCubeVertex(0.f, -5.f, -1.830127f, 0.5f, 1.0f, vertdata);//D
	AddCubeVertex(0.f, 0.f, 3.16987f, 1.0f, 1.0f, vertdata);//O
	AddCubeVertex(0.f, -5.f, -1.830127f, 0.5f, 1.0f, vertdata);//D
	AddCubeVertex(5.f, 0.f, -1.830127f, 1.0f, 0.5f, vertdata);//C
	AddCubeVertex(0.f, 0.f, 3.16987f, 1.0f, 0.0f, vertdata);//O
	AddCubeVertex(5.f, 0.f, -1.830127f, 1.0f, 0.5f, vertdata);//C
	AddCubeVertex(0.f, 5.f, -1.830127f, 0.5f, 0.0f, vertdata);//B
	AddCubeVertex(0.f, 0.f, 3.16987f, 0.0f, 0.0f, vertdata);//O
	AddCubeVertex(0.f, 5.f, -1.830127f, 0.5f, 0.0f, vertdata);//B
	AddCubeVertex(-5.f, 0.f, -1.830127f, 0.0f, 0.5f, vertdata);//A
	AddCubeVertex(0.f, 5.f, -1.830127f, 0.5f, 0.0f, vertdata);//B
	AddCubeVertex(-5.f, 0.f, -1.830127f, 0.0f, 0.5f, vertdata);//A
	AddCubeVertex(0.f, -5.f, -1.830127f, 0.5f, 1.0f, vertdata);//D
	AddCubeVertex(0.f, 5.f, -1.830127f, 0.5f, 0.0f, vertdata);//B
	AddCubeVertex(0.f, -5.f, -1.830127f, 0.5f, 1.0f, vertdata);//D
	AddCubeVertex(5.f, 0.f, -1.830127f, 1.0f, 0.5f, vertdata);//C
}
CMainApplication::CMainApplication(int argc, char *argv[])
	: m_pCompanionWindow(NULL)
	, m_pContext(NULL)
	, m_nCompanionWindowWidth(1200)
	, m_nCompanionWindowHeight(600)
	, m_unSceneProgramID(0)
	, m_unCompanionWindowProgramID(0)
	, m_unControllerTransformProgramID(0)
	, m_unRenderModelProgramID(0)
	, m_pHMD(NULL)
	, m_pRenderModels(NULL)
	, m_bDebugOpenGL(false)
	, m_bVerbose(false)
	, m_bPerf(false)
	, m_bVblank(false)
	, m_bGlFinishHack(true)
	, m_glControllerVertBuffer(0)
	, m_unControllerVAO(0)
	, m_unSceneVAO(0)
	, m_nSceneMatrixLocation(-1)
	, m_nControllerMatrixLocation(-1)
	, m_nRenderModelMatrixLocation(-1)
	, m_iTrackedControllerCount(0)
	, m_iTrackedControllerCount_Last(-1)
	, m_iValidPoseCount(0)
	, m_iValidPoseCount_Last(-1)
	, m_iSceneVolumeInit(1)///ppp ������ͼ��ʾ����  ���η���
	, m_strPoseClasses("cic474")
	, m_bShowCubes(true)
{
	for (int i = 1; i < argc; i++)
	{
		if (!stricmp(argv[i], "-gldebug"))
		{
			m_bDebugOpenGL = true;
		}
		else if (!stricmp(argv[i], "-verbose"))
		{
			m_bVerbose = true;
		}
		else if (!stricmp(argv[i], "-novblank"))
		{
			m_bVblank = false;
		}
		else if (!stricmp(argv[i], "-noglfinishhack"))
		{
			m_bGlFinishHack = false;
		}
		else if (!stricmp(argv[i], "-noprintf"))
		{
			g_bPrintf = false;
		}
		else if (!stricmp(argv[i], "-cubevolume") && (argc > i + 1) && (*argv[i + 1] != '-'))
		{
			m_iSceneVolumeInit = atoi(argv[i + 1]);
			i++;
		}
	}
	// other initialization tasks are done in BInit
	memset(m_rDevClassChar, 0, sizeof(m_rDevClassChar));
};
CMainApplication::~CMainApplication()
{
	// work is done in Shutdown
	dprintf("Shutdown");
}
std::string GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL)
{
	uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0)
		return "";
	char *pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}
bool CMainApplication::BInit()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
	{
		printf("%s - SDL could not initialize! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
		return false;
	}
	// Loading the SteamVR Runtime
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
	if (eError != vr::VRInitError_None)
	{
		m_pHMD = NULL;
		char buf[1024];
		sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "VR_Init Failed", buf, NULL);
		return false;
	}
	m_pRenderModels = (vr::IVRRenderModels *)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);
	if (!m_pRenderModels)
	{
		m_pHMD = NULL;
		vr::VR_Shutdown();
		char buf[1024];
		sprintf_s(buf, sizeof(buf), "Unable to get render model interface: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "VR_Init Failed", buf, NULL);
		return false;
	}
	int nWindowPosX = 500;
	int nWindowPosY = 100;
	Uint32 unWindowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN ;
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	if (m_bDebugOpenGL)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	m_pCompanionWindow = SDL_CreateWindow("CIC474-VR", nWindowPosX, nWindowPosY, m_nCompanionWindowWidth, m_nCompanionWindowHeight, unWindowFlags);
	if (m_pCompanionWindow == NULL)
	{
		printf("%s - Window could not be created! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
		return false;
	}
	m_pContext = SDL_GL_CreateContext(m_pCompanionWindow);
	if (m_pContext == NULL)
	{
		printf("%s - OpenGL context could not be created! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
		return false;
	}
	glewExperimental = GL_TRUE;
	GLenum nGlewError = glewInit();
	if (nGlewError != GLEW_OK)
	{
		printf("%s - Error initializing GLEW! %s\n", __FUNCTION__, glewGetErrorString(nGlewError));
		return false;
	}
	glGetError(); // to clear the error caused deep in GLEW
	if (SDL_GL_SetSwapInterval(m_bVblank ? 1 : 0) < 0)
	{
		printf("%s - Warning: Unable to set VSync! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
		return false;
	}
	m_strDriver = "No Driver";
	m_strDisplay = "No Display";
	m_strDriver = GetTrackedDeviceString(m_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
	m_strDisplay = GetTrackedDeviceString(m_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
	std::string strWindowTitle = "CIC474-VR";// "hellovr - " + m_strDriver + " " + m_strDisplay;
	SDL_SetWindowTitle(m_pCompanionWindow, strWindowTitle.c_str());
	// cube array
	m_iSceneVolumeWidth = m_iSceneVolumeInit;
	m_iSceneVolumeHeight = m_iSceneVolumeInit;
	m_iSceneVolumeDepth = m_iSceneVolumeInit;
	///ppp 
	m_fScale = 1.0f;//���о�����
	m_fScaleSpacing = 2.0f;//����ͼ��֮����
	m_fNearClip = 0.1f;//ͶӰԶ��ƽ�����
	m_fFarClip = 100.0f;
	m_iTexture = 0;
	m_uiVertcount = 0;
	// 		m_MillisecondsTimer.start(1, this);
	// 		m_SecondsTimer.start(1000, this);
	if (!BInitGL())
	{
		printf("%s - Unable to initialize OpenGL!\n", __FUNCTION__);
		return false;
	}
	if (!BInitCompositor())
	{
		printf("%s - Failed to initialize VR Compositor!\n", __FUNCTION__);
		return false;
	}
	return true;
}
void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	dprintf("GL Error: %s\n", message);
}
bool CMainApplication::BInitGL()
{
	if (m_bDebugOpenGL)
	{
		glDebugMessageCallback((GLDEBUGPROC)DebugCallback, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	}
	if (!CreateAllShaders())
		return false;
	SetupTexturemaps();
	SetupScene();
	SetupCameras();
	SetupStereoRenderTargets();
	SetupCompanionWindow();
	SetupRenderModels();
	return true;
}
bool CMainApplication::BInitCompositor()
{
	vr::EVRInitError peError = vr::VRInitError_None;
	if (!vr::VRCompositor())
	{
		printf("Compositor initialization failed. See log file for details\n");
		return false;
	}
	return true;
}
void CMainApplication::Shutdown()
{
	if (m_pHMD)
	{
		vr::VR_Shutdown();
		m_pHMD = NULL;
	}
	for (std::vector< CGLRenderModel * >::iterator i = m_vecRenderModels.begin(); i != m_vecRenderModels.end(); i++)
	{
		delete (*i);
	}
	m_vecRenderModels.clear();
	if (m_pContext)
	{
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
		glDebugMessageCallback(nullptr, nullptr);
		glDeleteBuffers(1, &m_glSceneVertBuffer);
		if (m_unSceneProgramID)
		{
			glDeleteProgram(m_unSceneProgramID);
		}
		if (m_unControllerTransformProgramID)
		{
			glDeleteProgram(m_unControllerTransformProgramID);
		}
		if (m_unRenderModelProgramID)
		{
			glDeleteProgram(m_unRenderModelProgramID);
		}
		if (m_unCompanionWindowProgramID)
		{
			glDeleteProgram(m_unCompanionWindowProgramID);
		}
		glDeleteRenderbuffers(1, &leftEyeDesc.m_nDepthBufferId);
		glDeleteTextures(1, &leftEyeDesc.m_nRenderTextureId);
		glDeleteFramebuffers(1, &leftEyeDesc.m_nRenderFramebufferId);
		glDeleteTextures(1, &leftEyeDesc.m_nResolveTextureId);
		glDeleteFramebuffers(1, &leftEyeDesc.m_nResolveFramebufferId);
		glDeleteRenderbuffers(1, &rightEyeDesc.m_nDepthBufferId);
		glDeleteTextures(1, &rightEyeDesc.m_nRenderTextureId);
		glDeleteFramebuffers(1, &rightEyeDesc.m_nRenderFramebufferId);
		glDeleteTextures(1, &rightEyeDesc.m_nResolveTextureId);
		glDeleteFramebuffers(1, &rightEyeDesc.m_nResolveFramebufferId);
		if (m_unCompanionWindowVAO != 0)
		{
			glDeleteVertexArrays(1, &m_unCompanionWindowVAO);
		}
		if (m_unSceneVAO != 0)
		{
			glDeleteVertexArrays(1, &m_unSceneVAO);
		}
		if (m_unControllerVAO != 0)
		{
			glDeleteVertexArrays(1, &m_unControllerVAO);
		}
	}
	if (m_pCompanionWindow)
	{
		SDL_DestroyWindow(m_pCompanionWindow);
		m_pCompanionWindow = NULL;
	}
	SDL_Quit();
}
bool CMainApplication::HandleInput()
{
	SDL_Event sdlEvent;
	bool bRet = false;
	while (SDL_PollEvent(&sdlEvent) != 0)
	{
		if (sdlEvent.type == SDL_QUIT)
		{
			bRet = true;
		}
		else if (sdlEvent.type == SDL_KEYDOWN)
		{
			if (sdlEvent.key.keysym.sym == SDLK_ESCAPE
				|| sdlEvent.key.keysym.sym == SDLK_q)
			{
				bRet = true;
			}
			if (sdlEvent.key.keysym.sym == SDLK_c)
			{
				m_bShowCubes = !m_bShowCubes;
			}
		}
	}
	// Process SteamVR events
	vr::VREvent_t event;
	while (m_pHMD->PollNextEvent(&event, sizeof(event)))
	{
		ProcessVREvent(event);
	}
	// Process SteamVR controller state
	for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++)
	{
		vr::VRControllerState_t state;
		if (m_pHMD->GetControllerState(unDevice, &state, sizeof(state)))
		{
			m_rbShowTrackedDevice[unDevice] = state.ulButtonPressed == 0;
		}
	}
	return bRet;
}
void CMainApplication::ProcessVREvent(const vr::VREvent_t & event)
{
	switch (event.eventType)
	{
	case vr::VREvent_TrackedDeviceActivated:
	{
		SetupRenderModelForTrackedDevice(event.trackedDeviceIndex);
		dprintf("Device %u attached. Setting up render model.\n", event.trackedDeviceIndex);
	}
	break;
	case vr::VREvent_TrackedDeviceDeactivated:
	{
		dprintf("Device %u detached.\n", event.trackedDeviceIndex);
	}
	break;
	case vr::VREvent_TrackedDeviceUpdated:
	{
		dprintf("Device %u updated.\n", event.trackedDeviceIndex);
	}
	break;
	}
}
void CMainApplication::RenderFrame()
{
	// for now as fast as possible
	if (m_pHMD)
	{
		RenderControllerAxes();
		RenderStereoTargets();
		RenderCompanionWindow();
		vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)leftEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
		vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)rightEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
	}
	if (m_bVblank && m_bGlFinishHack)
	{
		//$ HACKHACK. From gpuview profiling, it looks like there is a bug where two renders and a present
		// happen right before and after the vsync causing all kinds of jittering issues. This glFinish()
		// appears to clear that up. Temporary fix while I try to get nvidia to investigate this problem.
		// 1/29/2014 mikesart
		glFinish();
	}
	// SwapWindow
	{
		SDL_GL_SwapWindow(m_pCompanionWindow);
	}
	// Clear
	{
		// We want to make sure the glFinish waits for the entire present to complete, not just the submission
		// of the command. So, we do a clear here right here so the glFinish will wait fully for the swap.
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	// Flush and wait for swap.
	if (m_bVblank)
	{
		glFlush();//ǿ��ˢ�»��壬��֤��ͼ�����ִ��
		glFinish();
	}
	//Spew out the controller and pose count whenever they change.
	if (m_iTrackedControllerCount != m_iTrackedControllerCount_Last || m_iValidPoseCount != m_iValidPoseCount_Last)
	{
		m_iValidPoseCount_Last = m_iValidPoseCount;
		m_iTrackedControllerCount_Last = m_iTrackedControllerCount;
		//dprintf("PoseCount:%d(%s) Controllers:%d\n", m_iValidPoseCount, m_strPoseClasses.c_str(), m_iTrackedControllerCount);
	}
	UpdateHMDMatrixPose();
}
GLuint CMainApplication::CompileGLShader(const char *pchShaderName, const char *pchVertexShader, const char *pchFragmentShader)
{
	GLuint unProgramID = glCreateProgram();
	GLuint nSceneVertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(nSceneVertexShader, 1, &pchVertexShader, NULL);
	glCompileShader(nSceneVertexShader);
	GLint vShaderCompiled = GL_FALSE;
	glGetShaderiv(nSceneVertexShader, GL_COMPILE_STATUS, &vShaderCompiled);
	if (vShaderCompiled != GL_TRUE)
	{
		dprintf("%s - Unable to compile vertex shader %d!\n", pchShaderName, nSceneVertexShader);
		glDeleteProgram(unProgramID);
		glDeleteShader(nSceneVertexShader);
		return 0;
	}
	glAttachShader(unProgramID, nSceneVertexShader);
	glDeleteShader(nSceneVertexShader); // the program hangs onto this once it's attached
	GLuint  nSceneFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(nSceneFragmentShader, 1, &pchFragmentShader, NULL);
	glCompileShader(nSceneFragmentShader);
	GLint fShaderCompiled = GL_FALSE;
	glGetShaderiv(nSceneFragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
	if (fShaderCompiled != GL_TRUE)
	{
		dprintf("%s - Unable to compile fragment shader %d!\n", pchShaderName, nSceneFragmentShader);
		glDeleteProgram(unProgramID);
		glDeleteShader(nSceneFragmentShader);
		return 0;
	}
	glAttachShader(unProgramID, nSceneFragmentShader);
	glDeleteShader(nSceneFragmentShader); // the program hangs onto this once it's attached
	glLinkProgram(unProgramID);
	GLint programSuccess = GL_TRUE;
	glGetProgramiv(unProgramID, GL_LINK_STATUS, &programSuccess);
	if (programSuccess != GL_TRUE)
	{
		dprintf("%s - Error linking program %d!\n", pchShaderName, unProgramID);
		glDeleteProgram(unProgramID);
		return 0;
	}
	glUseProgram(unProgramID);
	glUseProgram(0);
	return unProgramID;
}
bool CMainApplication::CreateAllShaders()
{
	m_unSceneProgramID = CompileGLShader(
		"Scene",
		// Vertex Shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec2 v2UVcoordsIn;\n"
		"layout(location = 2) in vec3 v3NormalIn;\n"
		"out vec2 v2UVcoords;\n"
		"void main()\n"
		"{\n"
		"	v2UVcoords = v2UVcoordsIn;\n"
		"	gl_Position = matrix * position;\n"
		"}\n",
		// Fragment Shader
		"#version 410 core\n"
		"uniform sampler2D mytexture;\n"
		"in vec2 v2UVcoords;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = texture(mytexture, v2UVcoords);\n"
		"}\n"
		);
	m_nSceneMatrixLocation = glGetUniformLocation(m_unSceneProgramID, "matrix");
	if (m_nSceneMatrixLocation == -1)
	{
		dprintf("Unable to find matrix uniform in scene shader\n");
		return false;
	}
	m_unControllerTransformProgramID = CompileGLShader(
		"Controller",
		// vertex shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec3 v3ColorIn;\n"
		"out vec4 v4Color;\n"
		"void main()\n"
		"{\n"
		"	v4Color.xyz = v3ColorIn; v4Color.a = 1.0;\n"
		"	gl_Position = matrix * position;\n"
		"}\n",
		// fragment shader
		"#version 410\n"
		"in vec4 v4Color;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = v4Color;\n"
		"}\n"
		);
	m_nControllerMatrixLocation = glGetUniformLocation(m_unControllerTransformProgramID, "matrix");
	if (m_nControllerMatrixLocation == -1)
	{
		dprintf("Unable to find matrix uniform in controller shader\n");
		return false;
	}
	m_unRenderModelProgramID = CompileGLShader(
		"render model",
		// vertex shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec3 v3NormalIn;\n"
		"layout(location = 2) in vec2 v2TexCoordsIn;\n"
		"out vec2 v2TexCoord;\n"
		"void main()\n"
		"{\n"
		"	v2TexCoord = v2TexCoordsIn;\n"
		"	gl_Position = matrix * vec4(position.xyz, 1);\n"
		"}\n",
		//fragment shader
		"#version 410 core\n"
		"uniform sampler2D diffuse;\n"
		"in vec2 v2TexCoord;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = texture( diffuse, v2TexCoord);\n"
		"}\n"
		);
	m_nRenderModelMatrixLocation = glGetUniformLocation(m_unRenderModelProgramID, "matrix");
	if (m_nRenderModelMatrixLocation == -1)
	{
		dprintf("Unable to find matrix uniform in render model shader\n");
		return false;
	}
	m_unCompanionWindowProgramID = CompileGLShader(
		"CompanionWindow",
		// vertex shader
		"#version 410 core\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec2 v2UVIn;\n"
		"noperspective out vec2 v2UV;\n"
		"void main()\n"
		"{\n"
		"	v2UV = v2UVIn;\n"
		"	gl_Position = position;\n"
		"}\n",
		// fragment shader
		"#version 410 core\n"
		"uniform sampler2D mytexture;\n"
		"noperspective in vec2 v2UV;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"		outputColor = texture(mytexture, v2UV);\n"
		"}\n"
		);
	return m_unSceneProgramID != 0
		&& m_unControllerTransformProgramID != 0
		&& m_unRenderModelProgramID != 0
		&& m_unCompanionWindowProgramID != 0;
}
void CMainApplication::SetupScene()
{
	if (!m_pHMD)
		return;
	std::vector<float> vertdataarray;///ppp ���Ƶ���άͼ��
	Matrix4 matScale;
	matScale.scale(m_fScale, m_fScale, m_fScale);
	Matrix4 matTransform;
	matTransform.translate(
		-((float)m_iSceneVolumeWidth * m_fScaleSpacing) / 2.f,
		-((float)m_iSceneVolumeHeight * m_fScaleSpacing) / 2.f,
		-((float)m_iSceneVolumeDepth * m_fScaleSpacing) / 2.f);
	Matrix4 mat = matScale * matTransform;
	//cout << "mat:" <<endl<< mat << endl;
	for (int z = 0; z< m_iSceneVolumeDepth; z++)
	{
		for (int y = 0; y< m_iSceneVolumeHeight; y++)
		{
			for (int x = 0; x< m_iSceneVolumeWidth; x++)
			{
				AddPyramidToScene(mat, vertdataarray);///���Ƶ���άͼ�κͶ�άͼƬ��ͼ��
													  //AddCubeToScene(mat, vertdataarray);
				mat = mat * Matrix4().translate(m_fScaleSpacing, 0, 0);
			}
			mat = mat * Matrix4().translate(-((float)m_iSceneVolumeWidth) * m_fScaleSpacing, m_fScaleSpacing, 0);
		}
		mat = mat * Matrix4().translate(0, -((float)m_iSceneVolumeHeight) * m_fScaleSpacing, m_fScaleSpacing);
	}
	m_uiVertcount = vertdataarray.size() / 5;
	glGenVertexArrays(1, &m_unSceneVAO);
	glBindVertexArray(m_unSceneVAO);
	glGenBuffers(1, &m_glSceneVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glSceneVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertdataarray.size(), &vertdataarray[0], GL_STATIC_DRAW);
	GLsizei stride = sizeof(VertexDataScene);
	uintptr_t offset = 0;
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset);
	offset += sizeof(Vector3);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (const void *)offset);
	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}
void CMainApplication::AddCubeVertex(float fl0, float fl1, float fl2, float fl3, float fl4, std::vector<float> &vertdata)
{
	vertdata.push_back(fl0);
	vertdata.push_back(fl1);
	vertdata.push_back(fl2);
	vertdata.push_back(fl3);
	vertdata.push_back(fl4);
}
void CMainApplication::AddCubeToScene(Matrix4 mat, std::vector<float> &vertdata)
{
	//x�ұ� y���� z����      u�� v��     triangles instead of quads
	// Matrix4 mat( outermat.data() );
	Vector4 A = mat * Vector4(0, 0, 0, 1);
	Vector4 B = mat * Vector4(1, 0, 0, 1);
	Vector4 C = mat * Vector4(1, 1, 0, 1);
	Vector4 D = mat * Vector4(0, 1, 0, 1);
	Vector4 E = mat * Vector4(0, 0, 1, 1);
	Vector4 F = mat * Vector4(1, 0, 1, 1);
	Vector4 G = mat * Vector4(1, 1, 1, 1);
	Vector4 H = mat * Vector4(0, 1, 1, 1);
	// 
	AddCubeVertex(E.x, E.y, E.z, 0, 1, vertdata); //Front
	AddCubeVertex(F.x, F.y, F.z, 1, 1, vertdata);
	AddCubeVertex(G.x, G.y, G.z, 1, 0, vertdata);
	AddCubeVertex(G.x, G.y, G.z, 1, 0, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 0, 0, vertdata);
	AddCubeVertex(E.x, E.y, E.z, 0, 1, vertdata);
	AddCubeVertex(B.x, B.y, B.z, 0, 1, vertdata); //Back
	AddCubeVertex(A.x, A.y, A.z, 1, 1, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 1, 0, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 1, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 0, 0, vertdata);
	AddCubeVertex(B.x, B.y, B.z, 0, 1, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 0, 1, vertdata); //Top
	AddCubeVertex(G.x, G.y, G.z, 1, 1, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 0, 0, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 0, 1, vertdata);
	AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata); //Bottom
	AddCubeVertex(B.x, B.y, B.z, 1, 1, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 1, 0, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 1, 0, vertdata);
	AddCubeVertex(E.x, E.y, E.z, 0, 0, vertdata);
	AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata);
	AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata); //Left
	AddCubeVertex(E.x, E.y, E.z, 1, 1, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 1, 0, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 1, 0, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 0, 0, vertdata);
	AddCubeVertex(A.x, A.y, A.z, 0, 1, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 0, 1, vertdata); //Right
	AddCubeVertex(B.x, B.y, B.z, 1, 1, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, vertdata);
	AddCubeVertex(G.x, G.y, G.z, 0, 0, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 0, 1, vertdata);
}
void CMainApplication::RenderControllerAxes()
{
	// don't draw controllers if somebody else has input focus
	if (m_pHMD->IsInputFocusCapturedByAnotherProcess())
		return;
	std::vector<float> vertdataarray;
	m_uiControllerVertcount = 0;
	m_iTrackedControllerCount = 0;
	for (vr::TrackedDeviceIndex_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; ++unTrackedDevice)
	{
		if (!m_pHMD->IsTrackedDeviceConnected(unTrackedDevice))
			continue;
		if (m_pHMD->GetTrackedDeviceClass(unTrackedDevice) != vr::TrackedDeviceClass_Controller)
			continue;
		m_iTrackedControllerCount += 1;
		if (!m_rTrackedDevicePose[unTrackedDevice].bPoseIsValid)
			continue;
		const Matrix4 & mat = m_rmat4DevicePose[unTrackedDevice];
		Vector4 center = mat * Vector4(0, 0, 0, 1);
		for (int i = 0; i < 3; ++i)
		{
			Vector3 color(0, 0, 0);
			Vector4 point(0, 0, 0, 1);
			point[i] += 0.05f;  // offset in X, Y, Z
			color[i] = 1.0;  // R, G, B
			point = mat * point;
			vertdataarray.push_back(center.x);
			vertdataarray.push_back(center.y);
			vertdataarray.push_back(center.z);
			vertdataarray.push_back(color.x);
			vertdataarray.push_back(color.y);
			vertdataarray.push_back(color.z);
			vertdataarray.push_back(point.x);
			vertdataarray.push_back(point.y);
			vertdataarray.push_back(point.z);
			vertdataarray.push_back(color.x);
			vertdataarray.push_back(color.y);
			vertdataarray.push_back(color.z);
			m_uiControllerVertcount += 2;
		}
		Vector4 start = mat * Vector4(0, 0, -0.02f, 1);
		Vector4 end = mat * Vector4(0, 0, -39.f, 1);
		Vector3 color(.92f, .92f, .71f);
		vertdataarray.push_back(start.x); vertdataarray.push_back(start.y); vertdataarray.push_back(start.z);
		vertdataarray.push_back(color.x); vertdataarray.push_back(color.y); vertdataarray.push_back(color.z);
		vertdataarray.push_back(end.x); vertdataarray.push_back(end.y); vertdataarray.push_back(end.z);
		vertdataarray.push_back(color.x); vertdataarray.push_back(color.y); vertdataarray.push_back(color.z);
		m_uiControllerVertcount += 2;
	}
	// Setup the VAO the first time through.
	if (m_unControllerVAO == 0)
	{
		glGenVertexArrays(1, &m_unControllerVAO);
		glBindVertexArray(m_unControllerVAO);
		glGenBuffers(1, &m_glControllerVertBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_glControllerVertBuffer);
		GLuint stride = 2 * 3 * sizeof(float);
		uintptr_t offset = 0;
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset);
		offset += sizeof(Vector3);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset);
		glBindVertexArray(0);
	}
	glBindBuffer(GL_ARRAY_BUFFER, m_glControllerVertBuffer);
	// set vertex data if we have some
	if (vertdataarray.size() > 0)
	{
		//$ TODO: Use glBufferSubData for this...
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertdataarray.size(), &vertdataarray[0], GL_STREAM_DRAW);
	}
}
void CMainApplication::SetupCameras()
{
	m_mat4ProjectionLeft = GetHMDMatrixProjectionEye(vr::Eye_Left);
	m_mat4ProjectionRight = GetHMDMatrixProjectionEye(vr::Eye_Right);
	m_mat4eyePosLeft = GetHMDMatrixPoseEye(vr::Eye_Left);
	m_mat4eyePosRight = GetHMDMatrixPoseEye(vr::Eye_Right);
}
bool CMainApplication::CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc)
{
	glGenFramebuffers(1, &framebufferDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId);
	glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
	glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, nWidth, nHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);
	glGenTextures(1, &framebufferDesc.m_nRenderTextureId);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, nWidth, nHeight, true);
	//glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_BGR, nWidth, nHeight, true);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId, 0);
	glGenFramebuffers(1, &framebufferDesc.m_nResolveFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId);
	glGenTextures(1, &framebufferDesc.m_nResolveTextureId);
	glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_BGR, nWidth, nHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId, 0);
	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		return false;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}
bool CMainApplication::SetupStereoRenderTargets()
{
	if (!m_pHMD)
		return false;
	m_pHMD->GetRecommendedRenderTargetSize(&m_nRenderWidth, &m_nRenderHeight);
	CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, leftEyeDesc);
	CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, rightEyeDesc);
	return true;
}
void CMainApplication::SetupCompanionWindow()
{
	if (!m_pHMD)
		return;
	std::vector<VertexDataWindow> vVerts;
	//��Ļ������
	// left eye verts
	vVerts.push_back(VertexDataWindow(Vector2(-1, -1), Vector2(0, 0)));
	vVerts.push_back(VertexDataWindow(Vector2(0, -1), Vector2(1, 0)));
	vVerts.push_back(VertexDataWindow(Vector2(-1, 1), Vector2(0, 1)));
	vVerts.push_back(VertexDataWindow(Vector2(0, 1), Vector2(1, 1)));
	// right eye verts
	vVerts.push_back(VertexDataWindow(Vector2(0, -1), Vector2(0, 0)));
	vVerts.push_back(VertexDataWindow(Vector2(1, -1), Vector2(1, 0)));
	vVerts.push_back(VertexDataWindow(Vector2(0, 1), Vector2(0, 1)));
	vVerts.push_back(VertexDataWindow(Vector2(1, 1), Vector2(1, 1)));
	GLushort vIndices[] = { 0, 1, 3,   0, 3, 2,   4, 5, 7,   4, 7, 6 };
	m_uiCompanionWindowIndexSize = _countof(vIndices);
	glGenVertexArrays(1, &m_unCompanionWindowVAO);
	glBindVertexArray(m_unCompanionWindowVAO);
	glGenBuffers(1, &m_glCompanionWindowIDVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glCompanionWindowIDVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, vVerts.size() * sizeof(VertexDataWindow), &vVerts[0], GL_STATIC_DRAW);
	glGenBuffers(1, &m_glCompanionWindowIDIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glCompanionWindowIDIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_uiCompanionWindowIndexSize * sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void *)offsetof(VertexDataWindow, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void *)offsetof(VertexDataWindow, texCoord));
	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
void CMainApplication::RenderStereoTargets()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_MULTISAMPLE);
	// Left Eye
	glBindFramebuffer(GL_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
	RenderScene(vr::Eye_Left);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_MULTISAMPLE);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, leftEyeDesc.m_nResolveFramebufferId);
	glBlitFramebuffer(0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glEnable(GL_MULTISAMPLE);
	// Right Eye
	glBindFramebuffer(GL_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId);
	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
	RenderScene(vr::Eye_Right);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_MULTISAMPLE);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rightEyeDesc.m_nResolveFramebufferId);
	glBlitFramebuffer(0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}
void CMainApplication::RenderScene(vr::Hmd_Eye nEye)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	if (m_bShowCubes)
	{
		glUseProgram(m_unSceneProgramID);
		glUniformMatrix4fv(m_nSceneMatrixLocation, 1, GL_FALSE, GetCurrentViewProjectionMatrix(nEye).get());
		glBindVertexArray(m_unSceneVAO);
		glBindTexture(GL_TEXTURE_2D, m_iTexture);
		glDrawArrays(GL_TRIANGLES, 0, m_uiVertcount);
		glBindVertexArray(0);
	}
	bool bIsInputCapturedByAnotherProcess = m_pHMD->IsInputFocusCapturedByAnotherProcess();
	if (!bIsInputCapturedByAnotherProcess)
	{
		// draw the controller axis lines
		glUseProgram(m_unControllerTransformProgramID);
		glUniformMatrix4fv(m_nControllerMatrixLocation, 1, GL_FALSE, GetCurrentViewProjectionMatrix(nEye).get());
		glBindVertexArray(m_unControllerVAO);
		glDrawArrays(GL_LINES, 0, m_uiControllerVertcount);
		glBindVertexArray(0);
	}
	// ----- Render Model rendering -----
	glUseProgram(m_unRenderModelProgramID);
	for (uint32_t unTrackedDevice = 0; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++)
	{
		if (!m_rTrackedDeviceToRenderModel[unTrackedDevice] || !m_rbShowTrackedDevice[unTrackedDevice])
			continue;
		const vr::TrackedDevicePose_t & pose = m_rTrackedDevicePose[unTrackedDevice];
		if (!pose.bPoseIsValid)
			continue;
		if (bIsInputCapturedByAnotherProcess && m_pHMD->GetTrackedDeviceClass(unTrackedDevice) == vr::TrackedDeviceClass_Controller)
			continue;
		const Matrix4 & matDeviceToTracking = m_rmat4DevicePose[unTrackedDevice];
		Matrix4 matMVP = GetCurrentViewProjectionMatrix(nEye) * matDeviceToTracking;
		glUniformMatrix4fv(m_nRenderModelMatrixLocation, 1, GL_FALSE, matMVP.get());
		m_rTrackedDeviceToRenderModel[unTrackedDevice]->Draw();
	}
	glUseProgram(0);
}
void CMainApplication::RenderCompanionWindow()
{
	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, m_nCompanionWindowWidth, m_nCompanionWindowHeight);
	glBindVertexArray(m_unCompanionWindowVAO);
	glUseProgram(m_unCompanionWindowProgramID);
	// render left eye (first half of index array )
	glBindTexture(GL_TEXTURE_2D, leftEyeDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDrawElements(GL_TRIANGLES, m_uiCompanionWindowIndexSize / 2, GL_UNSIGNED_SHORT, (const void *)(uintptr_t)(m_uiCompanionWindowIndexSize));
	//glDrawElements(GL_TRIANGLES, m_uiCompanionWindowIndexSize / 2, GL_UNSIGNED_SHORT, 0);
	// render right eye (second half of index array )
	glBindTexture(GL_TEXTURE_2D, rightEyeDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDrawElements(GL_TRIANGLES, m_uiCompanionWindowIndexSize / 2, GL_UNSIGNED_SHORT, 0);
	//glDrawElements(GL_TRIANGLES, m_uiCompanionWindowIndexSize / 2, GL_UNSIGNED_SHORT, (const void *)(uintptr_t)(m_uiCompanionWindowIndexSize));
	glBindVertexArray(0);
	glUseProgram(0);
}
Matrix4 CMainApplication::GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye)
{
	if (!m_pHMD)
		return Matrix4();
	vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, m_fNearClip, m_fFarClip);
	return Matrix4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
		);
}
Matrix4 CMainApplication::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{///�ۺ�ͷת������
	if (!m_pHMD)
		return Matrix4();
	vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);
	Matrix4 matrixObj(
		matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
		matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
		matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
		matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f
		);
	return matrixObj.invert();
}
CGLRenderModel *CMainApplication::FindOrLoadRenderModel(const char *pchRenderModelName)
{
	CGLRenderModel *pRenderModel = NULL;
	for (std::vector< CGLRenderModel * >::iterator i = m_vecRenderModels.begin(); i != m_vecRenderModels.end(); i++)
	{
		if (!stricmp((*i)->GetName().c_str(), pchRenderModelName))
		{
			pRenderModel = *i;
			break;
		}
	}
	// load the model if we didn't find one
	if (!pRenderModel)
	{
		vr::RenderModel_t *pModel;
		vr::EVRRenderModelError error;
		while (1)
		{
			error = vr::VRRenderModels()->LoadRenderModel_Async(pchRenderModelName, &pModel);
			if (error != vr::VRRenderModelError_Loading)
				break;
			ThreadSleep(1);
		}
		if (error != vr::VRRenderModelError_None)
		{
			dprintf("Unable to load render model %s - %s\n", pchRenderModelName, vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error));
			return NULL; // move on to the next tracked device
		}
		vr::RenderModel_TextureMap_t *pTexture;
		while (1)
		{
			error = vr::VRRenderModels()->LoadTexture_Async(pModel->diffuseTextureId, &pTexture);
			if (error != vr::VRRenderModelError_Loading)
				break;
			ThreadSleep(1);
		}
		if (error != vr::VRRenderModelError_None)
		{
			dprintf("Unable to load render texture id:%d for render model %s\n", pModel->diffuseTextureId, pchRenderModelName);
			vr::VRRenderModels()->FreeRenderModel(pModel);
			return NULL; // move on to the next tracked device
		}
		pRenderModel = new CGLRenderModel(pchRenderModelName);
		if (!pRenderModel->BInit(*pModel, *pTexture))
		{
			dprintf("Unable to create GL model from render model %s\n", pchRenderModelName);
			delete pRenderModel;
			pRenderModel = NULL;
		}
		else
		{
			m_vecRenderModels.push_back(pRenderModel);
		}
		vr::VRRenderModels()->FreeRenderModel(pModel);
		vr::VRRenderModels()->FreeTexture(pTexture);
	}
	return pRenderModel;
}
void CMainApplication::SetupRenderModelForTrackedDevice(vr::TrackedDeviceIndex_t unTrackedDeviceIndex)
{
	if (unTrackedDeviceIndex >= vr::k_unMaxTrackedDeviceCount)
		return;
	// try to find a model we've already set up
	std::string sRenderModelName = GetTrackedDeviceString(m_pHMD, unTrackedDeviceIndex, vr::Prop_RenderModelName_String);
	CGLRenderModel *pRenderModel = FindOrLoadRenderModel(sRenderModelName.c_str());
	if (!pRenderModel)
	{
		std::string sTrackingSystemName = GetTrackedDeviceString(m_pHMD, unTrackedDeviceIndex, vr::Prop_TrackingSystemName_String);
		dprintf("Unable to load render model for tracked device %d (%s.%s)", unTrackedDeviceIndex, sTrackingSystemName.c_str(), sRenderModelName.c_str());
	}
	else
	{
		m_rTrackedDeviceToRenderModel[unTrackedDeviceIndex] = pRenderModel;
		m_rbShowTrackedDevice[unTrackedDeviceIndex] = true;
	}
}
void CMainApplication::SetupRenderModels()
{
	memset(m_rTrackedDeviceToRenderModel, 0, sizeof(m_rTrackedDeviceToRenderModel));
	if (!m_pHMD)
		return;
	for (uint32_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++)
	{
		if (!m_pHMD->IsTrackedDeviceConnected(unTrackedDevice))
			continue;
		SetupRenderModelForTrackedDevice(unTrackedDevice);
	}
}
Matrix4 CMainApplication::ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose)
{
	Matrix4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
		);
	return matrixObj;
}
CGLRenderModel::CGLRenderModel(const std::string & sRenderModelName)
	: m_sModelName(sRenderModelName)
{
	m_glIndexBuffer = 0;
	m_glVertArray = 0;
	m_glVertBuffer = 0;
	m_glTexture = 0;
}
CGLRenderModel::~CGLRenderModel()
{
	Cleanup();
}
bool CGLRenderModel::BInit(const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture)
{
	// create and bind a VAO to hold state for this model
	glGenVertexArrays(1, &m_glVertArray);
	glBindVertexArray(m_glVertArray);
	// Populate a vertex buffer
	glGenBuffers(1, &m_glVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vr::RenderModel_Vertex_t) * vrModel.unVertexCount, vrModel.rVertexData, GL_STATIC_DRAW);
	// Identify the components in the vertex buffer
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, vPosition));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, vNormal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, rfTextureCoord));
	// Create and populate the index buffer
	glGenBuffers(1, &m_glIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * vrModel.unTriangleCount * 3, vrModel.rIndexData, GL_STATIC_DRAW);
	glBindVertexArray(0);
	// create and populate the texture
	glGenTextures(1, &m_glTexture);
	glBindTexture(GL_TEXTURE_2D, m_glTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vrDiffuseTexture.unWidth, vrDiffuseTexture.unHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, vrDiffuseTexture.rubTextureMapData);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_BGR, vrDiffuseTexture.unWidth, vrDiffuseTexture.unHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, vrDiffuseTexture.rubTextureMapData);
	// If this renders black ask McJohn what's wrong.
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
	glBindTexture(GL_TEXTURE_2D, 0);
	m_unVertexCount = vrModel.unTriangleCount * 3;
	return true;
}
void CGLRenderModel::Cleanup()
{
	if (m_glVertBuffer)
	{
		glDeleteBuffers(1, &m_glIndexBuffer);
		glDeleteVertexArrays(1, &m_glVertArray);
		glDeleteBuffers(1, &m_glVertBuffer);
		m_glIndexBuffer = 0;
		m_glVertArray = 0;
		m_glVertBuffer = 0;
	}
}
void CGLRenderModel::Draw()
{
	glBindVertexArray(m_glVertArray);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_glTexture);
	glDrawElements(GL_TRIANGLES, m_unVertexCount, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}
int main(int argc, char *argv[])
{
	//namedWindow("test");
	//#ifdef RTP_SOCKETTYPE_WINSOCK
	//WSADATA dat;
	//WSAStartup(MAKEWORD(2, 2), &dat);
	//#endif // RTP_SOCKETTYPE_WINSOCK
	//decoder.decoder_init();
	//sess.SetParams("192.168.0.121", portdest, portbase);	

	SDL_Init(SDL_INIT_EVERYTHING);
	rgb_buffer = new unsigned char[WID*WID*3];
	LoadFrameThread = SDL_CreateThread(GetFrame, "GetFrame", &xx);
	jpegimage.create(WID, WID, CV_8UC3);
	QueryPerformanceFrequency(&nFreq);
	CMainApplication *pMainApplication = new CMainApplication(argc, argv);
	if (!pMainApplication->BInit())	{
		pMainApplication->Shutdown();
		return 1;
	}
	pMainApplication->RunMainLoop();
	pMainApplication->Shutdown();
	delete rgb_buffer;

	//decoder.decoder_flush((uint8_t*)sess.m_outbuffer);
	//decoder.decoder_destroy();
	//sess.BYEDestroy(RTPTime(10, 0), 0, 0);
	//#ifdef RTP_SOCKETTYPE_WINSOCK
	//WSACleanup();
	//#endif // RTP_SOCKETTYPE_WINSOCK

	return 0;
}