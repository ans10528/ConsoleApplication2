
#include <time.h>
#include <stdio.h>
#include <stdlib.h>   // Needed for _wtoi


// link with Ws2_32.lib
#pragma comment(lib,"Ws2_32.lib")
#include <WinSock2.h>
#include <ws2tcpip.h>


#include <opencv\cv.h>
#include <opencv\highgui.h>
using namespace cv;


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "3145"

int main(int argc, wchar_t **argv)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;


	const char *pstrImageName = "aaaa.bmp";
	const char *pstrWindowsTitle = "NONONO2";

	IplImage *pImage1;
	pImage1 = cvCreateImage(cvSize(320,240),8,3);

	cvShowImage(pstrWindowsTitle, pImage1);
	
	//cvNamedWindow(pstrWindowsTitle, CV_WINDOW_AUTOSIZE);

	//cvShowImage(pstrWindowsTitle, pImage1);

	//cvWaitKey();

	//cvDestroyWindow(pstrWindowsTitle);
	//cvReleaseImage(&pImage);
	//-------------------------------

	std::cout << "Winsock" << std::endl;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int) result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL); //�P�B
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// No longer need server socket
	closesocket(ListenSocket);


	char *data_Len_Buf = new char[4];
	char *data_SendTime_Buf = new char[4];
	int dataLen = 0;
	char *data;

	// ���c#�� - �t�ήɶ�
	//time_t t = time(0);
	//printf("%d", t);
	//system("pause");
	//return 0;

	// C# ---------------------------------------------------
	iSendResult = send(ClientSocket, new char[1]{0}, 1, 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		shutdown(ClientSocket, 2);
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	iResult = recv(ClientSocket, data_SendTime_Buf, 4, 0);


	// Receive until the peer shuts down the connection
	do {
		int TryCount = 0;

		//��������ƪ��׻P�ǰe�ݮɶ�
		for (int i = 0; i < 4; i++)
		{
			data_Len_Buf[i] = 0;
			data_SendTime_Buf[i] = 0;
		}
		iResult = recv(ClientSocket, data_Len_Buf, 4, 0);
		iResult = recv(ClientSocket, data_SendTime_Buf, 4, 0);

		dataLen = (unsigned char) data_Len_Buf[0] << 24 | (unsigned char) data_Len_Buf[1] << 16 | (unsigned char) data_Len_Buf[2] << 8 | (unsigned char) data_Len_Buf[3];

		printf("datalen = %d\n", dataLen);

		if (dataLen > 1024567 || dataLen < 0) //if  > 1MB 
		{
			printf("ErrorDetect: len=%d\n", dataLen);
			shutdown(ClientSocket, 2);
			closesocket(ClientSocket);
			WSACleanup();
			system("pause");
			return 0;
		}
		data = new char[dataLen];

		int get = 0;

		int curBufLen;// = dataLen - get > recvbuflen ? recvbuflen : dataLen - get;
		iResult = recv(ClientSocket, data, recvbuflen, 0);
		get = iResult;
		if (get == -1) get = 0;

		//�p�G����������
		if (get != dataLen)
		{
			int StartIndex = get;
			int EndIndex = dataLen - get;
			//���_���ݶi�汵��
			while (StartIndex != dataLen)
			{
				char* tmpAcceptData = new char[EndIndex];

				iResult = recv(ClientSocket, tmpAcceptData, EndIndex, 0);
				get = iResult;
				if (get == -1) get = 0;

				memcpy(data + StartIndex, tmpAcceptData, iResult*sizeof(char));

				StartIndex += get;
				EndIndex -= get;

				delete []tmpAcceptData;
			}
		}
		//���ռv�������O�_���`
		Mat cppImg;
		CvMat tmpMat = cvMat(320, 240, CV_8UC1, data);
		pImage1 = cvDecodeImage(&tmpMat,1);
		cvShowImage(pstrWindowsTitle, pImage1);
		cvWaitKey(33);


		//�^�Ǳ������\ �n�D�U�@�i�v��
		send(ClientSocket, new char[]{3}, 1, 0);

		delete []data;
	} while (iResult > 0);

	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		shutdown(ClientSocket, 2);
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	shutdown(ClientSocket, 2);
	closesocket(ClientSocket);
	WSACleanup();


	system("PAUSE");
	return 0;
}