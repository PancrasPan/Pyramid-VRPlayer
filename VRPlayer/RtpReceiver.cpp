#include "RtpReceiver.h"
#include <iostream>

void checkerror(int rtperr)
{
	if (rtperr < 0)
	{
		std::cout << "ERROR: " << RTPGetErrorString(rtperr) << std::endl;
		exit(-1);
	}
}

RtpReciver::RtpReciver()
{
	m_buffer = new uint8_t[BUF_SIZE];
	m_outbuffer = new uint8_t[OUTBUF_SIZE];
	viewpoint = 9;
	//decoder.decoder_init();
	/*if (fopen_s(&fp, "out.h264", "wb")<0)
		std::cout << "open failed!" << std::endl;*/
}

RtpReciver::~RtpReciver()
{
	delete m_buffer;
	delete m_outbuffer;
	//fclose(fp);
}

void RtpReciver::SetParams(const char * destip, uint16_t destport, uint16_t baseport)
{
	int status;
	//RTP+RTCP���ʼ��SOCKET����
	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;

	in_addr dest_ip;
	inet_pton(AF_INET, destip, (void *)&dest_ip);
	RTPIPv4Address addr(ntohl(dest_ip.s_addr), 6666);

	sessparams.SetOwnTimestampUnit(1.0 / 9000.0); //ʱ�����λ
	sessparams.SetAcceptOwnPackets(true);	//�����Լ����͵����ݰ�
	sessparams.SetUsePredefinedSSRC(true);  //����ʹ��Ԥ�ȶ����SSRC
	sessparams.SetPredefinedSSRC(SSRC);     //����SSRC
	transparams.SetPortbase(baseport);

	status = this->Create(sessparams, &transparams);
	checkerror(status);
	status = this->AddDestination(addr);
	checkerror(status);
}

//void RtpReciver::OnPollThreadStep()
//{
//	BeginDataAccess();
//
//	//check incoming packets
//	if (GotoFirstSourceWithData())
//	{
//		do
//		{
//			RTPPacket *pack;
//			RTPSourceData *srcdat;
//
//			srcdat = GetCurrentSourceInfo();
//
//			while ((pack = GetNextPacket()) != NULL)
//			{
//				//ProcessRTPPacket(*srcdat, *pack);
//				ProcessCloudVRPacket(*this,*srcdat, *pack);
//
//				DeletePacket(pack);
//			}
//		} while (GotoNextSourceWithData());
//	}
//
//	EndDataAccess();
//}

void RtpReciver::ProcessRTPPacket(const RTPSourceData &srcdat, const RTPPacket &rtppack)
{
	// You can inspect the packet and the source's info here
	//std::cout << "Got packet " << rtppack.GetExtendedSequenceNumber() << " from SSRC " << srcdat.GetSSRC() << std::endl;

	if (rtppack.GetPayloadType() == H264)
	{
		//std::cout<<"Got H264 packet����o " << rtppack.GetExtendedSequenceNumber() << " from SSRC " << srcdat.GetSSRC() <<std::endl;
		if (rtppack.HasMarker())//��������һ����������
		{
			//m_pVideoData->m_lLength = m_current_size + rtppack.GetPayloadLength();//�õ����ݰ��ܵĳ���
			//memcpy(m_pVideoData->m_pBuffer, m_buffer, m_current_size);
			//(m_pVideoData->m_pBuffer + m_current_size, rtppack.GetPayloadData(), rtppack.GetPayloadLength());

			//m_ReceiveArray.Add(m_pVideoData);//��ӵ����ն���
			memcpy(m_buffer+m_current_size, rtppack.GetPayloadData(), rtppack.GetPayloadLength());
			m_current_size += rtppack.GetPayloadLength();
			std::cout << "receive h.264 nalu:" << m_current_size << std::endl;
			//fwrite(m_buffer, 1, m_current_size, fp);
			memset(m_buffer, 0, m_current_size);//��ջ��棬Ϊ�´���׼��
			m_current_size = 0;
		}
		else
		{
			unsigned char* p = rtppack.GetPayloadData();


			memcpy(m_buffer + m_current_size, rtppack.GetPayloadData(), rtppack.GetPayloadLength());
			m_current_size += rtppack.GetPayloadLength();
		}
	}
	else if (rtppack.GetPayloadType() == VIEWPOINT)
	{
		memcpy(&viewpoint, rtppack.GetPayloadData(), rtppack.GetPayloadLength());
	}

}



