#pragma once
#ifndef RTP_RECIVER
#define RTP_RECIVER
#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtptimeutilities.h>
#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtcpapppacket.h>
#include <jrtplib3/rtpsourcedata.h>
#include <stdlib.h>


using namespace jrtplib;
#define BUF_SIZE 1024*1024*5
#define OUTBUF_SIZE 1280*1280*3/2
#define H264 96
#define VIEWPOINT 66

#define MAX_RTP_PKT_LENGTH 1360
#define TIMESTAMP_UNIT 90000
#define FPS 30
#define TIMESTAMP_INC TIMESTAMP_UNIT/FPS
#define SSRC 100

class RtpReciver : public RTPSession
{
public:
	//CVideoData* m_pVideoData;
	uint8_t *m_buffer;
	uint8_t *m_outbuffer;
	int m_current_size;
	FILE *fp;
	int viewpoint;
public:
	RtpReciver();
	~RtpReciver();
	void SetParams(const char * destip, uint16_t destport, uint16_t baseport);
protected:
	
	//void OnPollThreadStep();
	void ProcessRTPPacket(const RTPSourceData &srcdat, const RTPPacket &rtppack);
};

void checkerror(int rtperr);
extern void ProcessCloudVRPacket(RtpReciver &sess, const RTPSourceData &srcdat, const RTPPacket &rtppack);
#endif