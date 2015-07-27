
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


	//---------------------------------------------------
	//cv test
	//char* imageName = "aaaa.bmp";

	//Mat image;
	//image = imread("aaaa.bmp", 1);


	//Mat gray_image;
	//cvtColor(image, gray_image, CV_RGB2GRAY);

	////imwrite("maple girl gray.jpg", gray_image);

	//namedWindow(imageName, CV_WINDOW_AUTOSIZE);
	//namedWindow("Gray image", CV_WINDOW_AUTOSIZE);

	//imshow(imageName, image);
	//imshow("Gray image", gray_image);
	//---------------------------------------------------

	const char *pstrImageName = "aaaa.bmp";
	const char *pstrWindowsTitle = "NONONO2";
	IplImage *pImage = cvLoadImage(pstrImageName, CV_LOAD_IMAGE_COLOR);
	//cvNamedWindow(pstrWindowsTitle, CV_WINDOW_AUTOSIZE);
	
	cvShowImage(pstrWindowsTitle, pImage);

	cvWaitKey();

	cvDestroyWindow(pstrWindowsTitle);
	cvReleaseImage(&pImage);
	//-------------------------------

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
	ClientSocket = accept(ListenSocket, NULL, NULL); //同步
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

	// 對照c#端 - 系統時間
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

		//先接收資料長度與傳送端時間
		for (int i = 0; i < 4; i++)
		{
			data_Len_Buf[i] = 0;
			data_SendTime_Buf[i] = 0;
		}
		iResult = recv(ClientSocket, data_Len_Buf, 4, 0);
		iResult = recv(ClientSocket, data_SendTime_Buf, 4, 0);


		//dataLen = (int) (data_Len_Buf[0]) << 24 | (int) (data_Len_Buf[1]) << 16 | (int) (data_Len_Buf[2]) << 8 | data_Len_Buf[3];

		//dataLen = data_Len_Buf[0] << 24 | data_Len_Buf[1] << 16 | data_Len_Buf[2] << 8 | data_Len_Buf[3];
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

		//如果接收不完全
		if (get != dataLen)
		{
			int StartIndex = get;
			int EndIndex = dataLen - get;
			//不斷等待進行接收
			while (StartIndex != dataLen)
			{
				char* tmpAcceptData = new char[EndIndex];

				iResult = recv(ClientSocket, tmpAcceptData, EndIndex, 0);
				get = iResult;
				if (get == -1) get = 0;
				// data << tmpAcceptData

				//Array.ConstrainedCopy(tmpAcceptData, 0, data, StartIndex, get); 
				StartIndex += get;
				EndIndex -= get;
				//if (get == 0)
				//{
				//	++TryCount;
				//	if (TryCount > 100)
				//	{
				//		//msg += "Client斷線，或接收逾時";
				//		shutdown(ClientSocket, 2);
				//		closesocket(ClientSocket);
				//		WSACleanup();
				//		printf("Client斷線，或接收逾時\n");
				//		system("pause");
				//		return 0;
				//	}
				//}
			}
		}



		//回傳接收成功 要求下一張影像
		send(ClientSocket, new char[]{3}, 1, 0);

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