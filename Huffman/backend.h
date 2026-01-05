#ifndef BACKEND_H_INCLUDED
#define BACKEND_H_INCLUDED


#include "hd.h"//Í·ÎÄ¼þ
#include "huffMan.h"


#include <winsock2.h>


int getRealBoundary(char *reqBuf, char *boundary, int boundaryMaxLen);

int cHandleFile(char *fileData, int fileLen, char* filename, char* processedBuf);

int DecryptFile(char *compressFile,char *decompressFile);

int cHandleFile(char *fileData, int fileLen, char* filename, char* processedBuf);

int parseFormData(char *reqBody,int bodyLen,char *fileData,char *filename,int maxFileLen, char *boundary);

void sendFileResponse(SOCKET clientSock,char *fileBuf,int fileLen,char *filename);

void handleClientRequest(SOCKET clientSock);

int backend();




#endif // BACKEND_H_INCLUDED
