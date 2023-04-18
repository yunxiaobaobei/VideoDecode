
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdlHelper.h"

using namespace std;

extern "C" {
#include <./libavcodec/avcodec.h>
#include <./libavformat/avformat.h>
}


//切换播放和文件保存
//#define SAVE_YUV_FILE


#define INBUF_SIZE 4096


sdlHelper* sdl = NULL;


static void decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt,
	FILE* outfile)
{
	int ret;

	ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(dec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {
			fprintf(stderr, "Error during decoding\n");
			exit(1);
		}

		printf(" frame num %3d\n", dec_ctx->frame_num);
		fflush(stdout);

#ifdef SAVE_YUV_FILE
		//根据自己解码出来的数据格式进行处理
		//fwrite(frame->data[0], 1, frame->width * frame->height, outfile);//Y
		//fwrite(frame->data[1], 1, frame->width * frame->height / 4, outfile);//U
		//fwrite(frame->data[2], 1, frame->width * frame->height / 4, outfile);//V

		fwrite(frame->data[0], 1, frame->linesize[0] * frame->height, outfile);//Y
		fwrite(frame->data[1], 1, frame->linesize[1] * frame->height / 2, outfile);//U
		fwrite(frame->data[2], 1, frame->linesize[2] * frame->height / 2, outfile);//V

#else 
		//构建合适的纹理
		sdl->texture = SDL_CreateTexture(sdl->gRender, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, frame->width, frame->height);

		sdl->gRect.x = 0;
		sdl->gRect.y = 0;
		sdl->gRect.h = frame->height;
		sdl->gRect.w = frame->width;

		

		//将一帧数据复制到纹理平面上
		SDL_UpdateYUVTexture(sdl->texture, &sdl->gRect,
			frame->data[0], frame->linesize[0],
			frame->data[1], frame->linesize[1],
			frame->data[2], frame->linesize[2]);

		//清理渲染器缓冲区
		SDL_RenderClear(sdl->gRender);
		//将纹理拷贝到窗口渲染平面上
		SDL_RenderCopy(sdl->gRender, sdl->texture, NULL, &sdl->gRect);
		//翻转缓冲区，前台显示
		SDL_RenderPresent(sdl->gRender);

		SDL_Delay(40);
#endif
	}
}


int main(int argc, char** argv)
{
	const AVCodec* codec = nullptr;
	AVCodecParserContext* parser;
	AVCodecContext* codecContent;

	AVPacket* packet = nullptr;
	AVFrame* frame = nullptr;

	uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
	uint8_t* data;
	size_t data_size;
	int ret;

	//初始化一个AVFormatContent
	AVFormatContext* pFormatCtx;
	pFormatCtx = avformat_alloc_context();

	char name[] = "video.h264";
	const char* outName = "video.yuv";

	//打开输入和输出文件，用于读写文件
	FILE* inFile = fopen(name, "rb");
	//打开文件
	if (!inFile) {
		fprintf(stderr, "Could not open %s\n", name);
		exit(1);
	}

	FILE* outFile = fopen(outName, "wb");
	if (outFile == nullptr)
	{
		fprintf(stderr, "Could not open %s\n", outName);
		exit(1);
	}


	//打开文件初始化AVFormatContext
	if (avformat_open_input(&pFormatCtx, name, NULL, NULL) != 0)
	{
		printf("Couldn't open input stream.\n");

		avformat_free_context(pFormatCtx);
		return -1;
	}

	//查找流信息
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	//查找对应的流是否为视频流
	int videoindex = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	}
	if (videoindex == -1)
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}

	//根据编解码器id查询解码器并返回解码器结构
	codec = avcodec_find_decoder(pFormatCtx->streams[0]->codecpar->codec_id);

	//根据解码器id初始化响应的裸流的解析器
	parser = av_parser_init(codec->id);


	//分配解码器上下文
	codecContent = avcodec_alloc_context3(codec);


	//打开解码器和关联解码器上下文
	if (avcodec_open2(codecContent, codec, nullptr))
	{
		printf("could not open codec!\n");
		return -1;
	}

	//申请一个AVPacket 结构用来存放已编码的数据
	packet = av_packet_alloc();
	
	//申请一个AVFrame 结构用来存放解码后的数据
	frame = av_frame_alloc();

	//初始化SDL
	sdl = new sdlHelper();
	sdl->initSdl();

	//前期的准备工作完成
	//接下来开始解码数据

	//feof 判断流上的文件结束符，文件未结束返回0， 文件结束返回非0
	while (!feof(inFile))
	{
		//读取指定大小的数据到inbuff中
		data_size = fread(inbuf, 1, INBUF_SIZE, inFile);

		//读取完成
		if (data_size <= 0)
			break;

		//进行数据切片
		data = inbuf;
		while (data_size > 0)
		{
			ret = av_parser_parse2(parser, codecContent, &packet->data, &packet->size,
				data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

			if (ret <= 0)
				break;

			data += ret;
			data_size -= ret;

			//将packet 送去解码
			if (packet->size > 0)
				decode(codecContent, frame, packet, outFile);

		}

	}

	//解码器中可能有缓存，最后在解码一次
	/* 冲刷解码器 */
	packet->data = NULL;   // 让其进入drain mode
	packet->size = 0;
	decode(codecContent, frame, NULL, outFile);

	//执行清理操作
	av_parser_close(parser);
	avcodec_free_context(&codecContent);
	av_frame_free(&frame);
	av_packet_free(&packet);
	avformat_free_context(pFormatCtx);

	fclose(inFile);
	fclose(outFile);

	//清理sdl
	sdl->close();

	return 0;
}


