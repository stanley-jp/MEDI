//h264 filter for container file 

#include <stdio.h>
#include <stdlib.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#define FORMAT_TYPE_MP4 "mp4"
#define FORMAT_TYPE_FLV "flv"

void parser_format_type(const char *name, unsigned int len, char *output)
{
    if(name == NULL) {
        printf("%s input string is null!\n", __FUNCTION__);
        return ;
    }
    int i = 0;
    for(i=len; i>0; i--) {
        if(name[i] == '.') {
            printf("find!\n");
            memcpy(output, name+i+1, len-i);
        }
    }
    printf("output %s\n",output);
}

int main(int argc, char *argv[])
{
    AVCodecContext *pCodecContext;
    AVFormatContext *pFormatContext;
    AVCodec *pCodec;
    AVStream *pStream;
    AVPacket *pPacket;
    AVFrame *pFrame;
    
    int i = 0;
    int video_index = 0;
    int got_picture = 0;
    int pic_cnt = 0;


    char *file = argv[1];
    char file_type[8];
    parser_format_type(argv[1], strlen(argv[1]), file_type);
    printf("file_type %s\n", file_type);

    av_register_all();
    avformat_network_init();
    pFormatContext = avformat_alloc_context();

    if(avformat_open_input(&pFormatContext, file, NULL, NULL) != 0) {
        printf("avformat_open_input failed!\n");
        return -1;
    }

    if(avformat_find_stream_info(pFormatContext, NULL) != 0) {
        printf("av_find_stream_info failed!\n");
        return -1;
    }


    for(i = 0; i < pFormatContext->nb_streams; i++) {
        if(pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
            break;
        }
    }

    pCodecContext = pFormatContext->streams[video_index]->codec;
    pCodec = avcodec_find_decoder(pCodecContext->codec_id);
    if(pCodec == NULL) {
        printf("avcodec_find_decoder failed !\n");
        return -1;
    }

    if(avcodec_open2(pCodecContext, pCodec, NULL) != 0) {
        printf("avcodec_open2 failed !\n");
        return -1;
    }

    pPacket=(AVPacket *)av_malloc(sizeof(AVPacket));
    FILE * fp_h264 = fopen("output.h264","wb");
    if(strcmp(file_type, FORMAT_TYPE_MP4)==0 || strcmp(file_type, FORMAT_TYPE_FLV)==0) {
        AVBitStreamFilterContext* bsfc =  av_bitstream_filter_init("h264_mp4toannexb");  
        while(av_read_frame(pFormatContext, pPacket) >= 0) {
            if(pPacket->stream_index == video_index) {
                if(pPacket->flags &AV_PKT_FLAG_KEY) {
                    av_bitstream_filter_filter(bsfc, pCodecContext, NULL, &pPacket->data, &pPacket->size, pPacket->data, pPacket->size, 0);
                    fwrite(pPacket->data, pPacket->size, 1, fp_h264);  
                }
                else{
                    char nalu_start[] = {0,0,0,1};
                    memcpy(pPacket->data,nalu_start,4);
                    fwrite(pPacket->data, pPacket->size, 1, fp_h264);
                }
                printf("got a video packet %d\n", pic_cnt++);
                //av_bitstream_filter_filter(bsfc, pCodecContext, NULL, &pPacket->data, &pPacket->size, pPacket->data, pPacket->size, 0);  
                //fwrite(pPacket->data, pPacket->size, 1, fp_h264);
                
            }
            av_free_packet(pPacket);   
        }
        av_bitstream_filter_close(bsfc); 
    }

    av_free(pPacket);
    fclose(fp_h264);


    return 0;
}