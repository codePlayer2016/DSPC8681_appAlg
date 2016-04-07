#ifndef HTTP_NET_H
#define HTTP_NET_H

#ifndef DOWNLOAD_STAUTS
#define DOWNLOAD_STAUTS
#define NOSTART 0
#define DOWNLOADING 1
#define FINISHED 2
#define ERROR_DOWNLOADING 3
#define PAUSE 4



#define URL_NULL_ERROR -1
#define URL_PROTOCAL_ERROR -2
#define URL_HOST_ERROR -3
#define URL_PORT_ERROR -4
#define  URL_FILE_PATH_ERROR -11

#define REQUEST_RANGE_ERROR -12

#define HTTP_RESPONSE_ERROR -5
#define HTTP_CON_LEN_ERROR -6
#define HTTP_PCON_STA_ERROR -7


#define NET_CONNECT_ERROR -8
#define NET_SEND_ERROR - 9
#define NET_SOCKET_ERROR -10
#endif
//This struct is used to get the information of the url.
typedef struct  http_downloadInfo
{
	char *p_input_url;
	char *p_host_ip;
	char *p_fileDir;
	char *p_fileName;
	char *p_first_content;
	int n_port;
	int n_first_length;
} http_downloadInfo;

/************************************
*	p_input_ulr is the input url.
*	p_picture_data is the picture data.
*	n_picture_length is the picture length.
*	flag_downloadPicSuccessful is the flag for the picture downloading successful or not.
*	n_error_code is the error code ,if download the picture failed.
*************************************/
typedef struct pic_status
{
	char *p_input_ulr;
	char *p_picture_data;
	int n_picture_length;
	int flag_downloadPicSuccessful;
	int n_error_code;
} pic_status,*p_pic_status;
typedef struct __tagPicInfor
{
	uint8_t *picAddr[100];
	uint32_t picLength[100];
	uint8_t picUrls[100][102];
	uint8_t picNums;
} PicInfor;
//typedef struct __tagPicOutInfor
//{
//	unsigned char g_outBuffer[0x00400000];
//} PicOutInfor;
int http_parseURL(char *p_url,http_downloadInfo *p_info);
int http_sendRequest(SOCKET *socket_sendHandle,char *p_urlInput,http_downloadInfo *p_info);


#endif
