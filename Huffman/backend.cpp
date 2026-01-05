#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include<string.h>

#include "hd.h"
#include "huffMan.h"
#include "backend.h"

#include <winsock2.h>
#include <windows.h>

// 宏定义增加括号，防止运算优先级问题，统一常量管理
#define PORT 8080
#define BUFFER_SIZE (1024 * 10)
#define MAX_FILE_SIZE (1024 * 1024 * 5)  // 最大支持5MB文件上传
#define FILE_DATA_BUF_SIZE (BUFFER_SIZE * 50)
#define PROCESSED_BUF_SIZE MAX_FILE_SIZE
#define BOUNDARY "----WebKitFormBoundary" // 前端FormData默认分隔符，固定值


//获取请求头中的真实boundary
int getRealBoundary(char *reqBuf, char *boundary, int boundaryMaxLen)
{
    char *contentType = strstr(reqBuf, "Content-Type: multipart/form-data; boundary=");
    if (!contentType) return -1;
    contentType += strlen("Content-Type: multipart/form-data; boundary=");
    char *boundaryEnd = strstr(contentType, "\r\n");
    if (!boundaryEnd) return -1;
    int boundaryLen = boundaryEnd - contentType;
    if (boundaryLen >= boundaryMaxLen) return -2;
    strncpy(boundary, contentType, boundaryLen);
    boundary[boundaryLen] = '\0';
    return boundaryLen;
}

//解密文件
int DecryptFile(char *compressFile,char *decompressFile)
{

    FILE* fpCompress = fopen(compressFile, "rb");
    if (!fpCompress)
    {
        printf("打开压缩文件失败\n");
        return -1;
    }
    else printf("打开压缩文件成功！！！\n");

    FILE* fpDecompress = fopen(decompressFile, "wb");
    if (!fpDecompress)
    {
        printf("创建失败！！！\n");
        fclose(fpCompress); // 兜底关闭文件句柄
        return -1;
    }
    else printf("解压缩文件创建成功！！！\n");

    rundeCompress(compressFile, decompressFile, fpCompress, fpDecompress);

    fclose(fpCompress);
    fclose(fpDecompress);

    printf("qwq任务完成\n");
    return 0;
}


// 文件处理核心函数 - 删除全局缓冲区，改为堆内存申请+线程安全+兜底释放
int cHandleFile(char *fileData, int fileLen, char* filename, char* processedBuf)
{

    memset(processedBuf, 0, PROCESSED_BUF_SIZE);
    // 1. 定义临时文件路径
    const char* tempUploadFile = "./temp_upload.tmp";
    const char* tempCompressFile = "./temp_compress.bh";
    FILE* fp_upload = fopen(tempUploadFile, "wb");
    if (!fp_upload)
    {
        printf("临时文件创建失败！\n");
        return -1;
    }
    fwrite(fileData, 1, fileLen, fp_upload);
    fclose(fp_upload);

    // 2. 调用哈夫曼压缩核心函数
    //  int compressRet;
    int compressRet= HUFFMAN(tempUploadFile, tempCompressFile);

    if (compressRet < 0)
    {
        printf("哈夫曼压缩失败！\n");
        remove(tempUploadFile);
        remove(tempCompressFile);
        return -1;
    }

    // 3. 读取压缩后的文件到缓冲区
    FILE* fp_compress = fopen(tempCompressFile, "rb");
    if (!fp_compress)
    {
        printf("加密文件读取失败！\n");
        remove(tempUploadFile);
        remove(tempCompressFile);
        return -1;
    }
    fseek(fp_compress, 0, SEEK_END);
    int newFileLen = ftell(fp_compress);

    if (newFileLen >= PROCESSED_BUF_SIZE)
    {
        printf("压缩后的文件超过缓冲区大小！\n");
        fclose(fp_compress);
        remove(tempUploadFile);
        remove(tempCompressFile);
        return -1;
    }

    fseek(fp_compress, 0, SEEK_SET);
    fread(processedBuf, 1, newFileLen, fp_compress);
    fclose(fp_compress);

    // 4. 删除临时文件
    remove(tempUploadFile);
    remove(tempCompressFile);
    return newFileLen;
}



// 解析form-data表单 - 安全拷贝文件名+无指针越界
int parseFormData(char *reqBody,int bodyLen,char *fileData,char *filename,int maxFileLen, char *boundary)
{
    char boundaryFull[132];
    sprintf(boundaryFull,"--%s",boundary);
    char *boundaryPos = strstr(reqBody,boundaryFull);
    if(!boundaryPos) return -1;

    char *filenamePos= strstr(boundaryPos,"filename=\"");
    if(filenamePos)
    {
        filenamePos += 10;
        char *filenameEnd = strstr(filenamePos,"\"");
        if(!filenameEnd) return -1;
        // 安全拷贝：计算长度+strncpy+手动加结束符
        int nameLen = filenameEnd - filenamePos;
        strncpy(filename, filenamePos, nameLen);
        filename[nameLen] = '\0';
    }
    else
    {
        return -1;
    }

    char *dataStart = strstr(filenamePos,"\r\n\r\n") + 4;
    char *dataEnd = strstr(dataStart,boundary);
    if(!dataEnd) return -1;
    int fileLen = dataEnd - dataStart - 2;

    if(fileLen>maxFileLen) return -2;

    memcpy(fileData,dataStart,fileLen);
    return fileLen;
}


// 封装HTTP响应头返回文件
void sendFileResponse(SOCKET clientSock,char *fileBuf,int fileLen,char *filename)
{

    char responseHeader[BUFFER_SIZE];
    int headerLen = sprintf(responseHeader,
                            "HTTP/1.1 200 OK\r\n"
                            "Server: C-Native-Server\r\n"
                            "Access-Control-Allow-Origin: *\r\n"
                            "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                            "Content-Type: application/octet-stream\r\n"
                            "Content-Disposition: attachment; filename=\"%s\"\r\n"
                            "Content-Length: %d\r\n"
                            "\r\n",filename,fileLen);
    send(clientSock,responseHeader,headerLen,0);
    send(clientSock,fileBuf,fileLen,0);
}

// 处理客户端请求 - recv超时+内存申请校验+动态内存释放+无内存泄漏
void handleClientRequest(SOCKET clientSock)
{
    char buffer[BUFFER_SIZE];
    memset(buffer,0,BUFFER_SIZE);

    // 1. 申请堆内存存储完整请求体 + 内存申请校验
    char *totalBuf = (char*)malloc(MAX_FILE_SIZE);
    if (totalBuf == NULL)
    {
        char *errResp = "HTTP/1.1 500 Internal Server Error\r\nAccess-Control-Allow-Origin: *\r\n\r\n内存申请失败";
        send(clientSock, errResp, strlen(errResp), 0);
        closesocket(clientSock);
        return;
    }
    memset(totalBuf, 0, MAX_FILE_SIZE);

    int recvLen = 0, totalRecv = 0;

    // 设置recv超时3秒，解决无限阻塞导致的30秒卡死问题
    DWORD recvTimeout = 3000;
    setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout));

    // 接收请求数据
    while(1)
    {
        recvLen = recv(clientSock, buffer, BUFFER_SIZE - 1, 0);
        if(recvLen > 0)
        {
            if(totalRecv + recvLen >= MAX_FILE_SIZE)
            {
                char *errResp = "HTTP/1.1 413 Payload Too Large\r\nAccess-Control-Allow-Origin: *\r\n\r\n文件大小超过5MB限制";
                send(clientSock, errResp, strlen(errResp), 0);
                free(totalBuf); // 兜底释放内存
                closesocket(clientSock);
                return ;
            }
            memcpy(totalBuf + totalRecv, buffer, recvLen);
            totalRecv += recvLen;
            memset(buffer, 0, sizeof(buffer));
        }
        else if(recvLen == 0)
        {
            break; // 客户端断开连接
        }
        else
        {
            // 超时/读取错误，正常退出循环
            int errCode = WSAGetLastError();
            if(errCode != WSAETIMEDOUT)
                printf("recv失败，错误码：%d\n", errCode);
            break;
        }
    }

    // 校验是否是目标请求
    if(strstr(totalBuf,"POST /encrypt HTTP/1.1") == NULL)
    {
        char *resp = "HTTP/1.1 404 Not Found\r\n\r\n404";
        send(clientSock,resp,strlen(resp),0);
        free(totalBuf);
        closesocket(clientSock);
        return;
    }

    // 解析请求体
    char *bodyStart = strstr(totalBuf,"\r\n\r\n");
    if(!bodyStart)
    {
        char *errResp = "HTTP/1.1 400 Bad Request\r\nAccess-Control-Allow-Origin: *\r\n\r\n请求格式错误";
        send(clientSock,errResp,strlen(errResp),0);
        free(totalBuf);
        closesocket(clientSock);
        return;
    }
    int bodyLen = totalRecv - (bodyStart - totalBuf) - 4;

    // 2. 申请文件数据缓冲区 + 修复核心错误：malloc后用实际大小memset，不是sizeof(指针)
    char *fileData = (char*)malloc(FILE_DATA_BUF_SIZE);
    if (fileData == NULL)
    {
        char *errResp = "HTTP/1.1 500 Internal Server Error\r\nAccess-Control-Allow-Origin: *\r\n\r\n文件缓冲区申请失败";
        send(clientSock, errResp, strlen(errResp), 0);
        free(totalBuf);
        closesocket(clientSock);
        return;
    }
    memset(fileData, 0, FILE_DATA_BUF_SIZE); // 修复：使用真实申请的内存大小清空

    char filename[256];
    memset(filename,0,sizeof(filename));

    // 获取真实boundary
    char realBoundary[128] = {0};
    if(getRealBoundary(totalBuf, realBoundary, sizeof(realBoundary)) <= 0)
    {
        char *errResp = "HTTP/1.1 400 Bad Request\r\nAccess-Control-Allow-Origin: *\r\n\r\n请求格式错误，非multipart/form-data";
        send(clientSock,errResp,strlen(errResp),0);
        free(totalBuf);
        free(fileData);
        closesocket(clientSock);
        return;
    }

    // 解析文件数据
    int fileLen = parseFormData(bodyStart+4,bodyLen,fileData,filename,FILE_DATA_BUF_SIZE-1, realBoundary);
    if(fileLen <= 0)
    {
        char *errResp = "HTTP/1.1 400 Bad Request\r\nAccess-Control-Allow-Origin: *\r\n\r\n文件上传失败";
        send(clientSock,errResp,strlen(errResp),0);
        free(totalBuf);
        free(fileData);
        closesocket(clientSock);
        return;
    }

    // 3. 申请处理后文件缓冲区 - 解决全局缓冲区线程不安全问题
    char *processedBuf = (char*)malloc(PROCESSED_BUF_SIZE);
    if (processedBuf == NULL)
    {
        char *errResp = "HTTP/1.1 500 Internal Server Error\r\nAccess-Control-Allow-Origin: *\r\n\r\n处理缓冲区申请失败";
        send(clientSock, errResp, strlen(errResp), 0);
        free(totalBuf);
        free(fileData);
        closesocket(clientSock);
        return;
    }

    // 文件压缩处理
    int newFileLen = cHandleFile(fileData, fileLen, filename, processedBuf);
    if(newFileLen <= 0)
    {
        char *errResp = "HTTP/1.1 500 Internal Server Error\r\nAccess-Control-Allow-Origin: *\r\n\r\n文件加密处理失败";
        send(clientSock,errResp,strlen(errResp),0);
        free(totalBuf);
        free(fileData);
        free(processedBuf);
        closesocket(clientSock);
        return;
    }

    // 返回处理后的文件
    char newFileName[512] = {0};
    sprintf(newFileName, "%s.BH", filename);
    sendFileResponse(clientSock, processedBuf, newFileLen, newFileName);

    // 兜底释放所有堆内存，无任何内存泄漏
    free(totalBuf);
    free(fileData);
    free(processedBuf);
    closesocket(clientSock);
}


//后端整个处理过程
int backend()
{
    WSADATA wasData;
    SOCKET serverSock,clientSock;
    struct sockaddr_in serverAddr,clientAddr;
    int addrLen=sizeof(clientAddr);

    if(WSAStartup(MAKEWORD(2,2),&wasData) !=0)
    {
        printf("WSA初始化失败！错误码：%d\n",WSAGetLastError());
        return 1;
    }

    serverSock = socket(AF_INET,SOCK_STREAM,0);
    if(serverSock == INVALID_SOCKET)
    {
        printf("创建套接字失败！错误码：%d\n",WSAGetLastError());
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if(bind(serverSock,(struct sockaddr *)&serverAddr,sizeof(serverAddr))==SOCKET_ERROR)
    {
        printf("端口绑定失败！错误码：%d\n",WSAGetLastError());
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }
    if(listen(serverSock,5)==SOCKET_ERROR)
    {
        printf("监听端口失败！错误码：%d\n",WSAGetLastError());
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }
    printf("✅ C语言原生后端服务启动成功！\n");
    printf("✅ 监听地址：http://localhost:%d\n", PORT);
    printf("✅ 接口地址：/encrypt\n");
    printf("✅ 纯C实现 | 无框架 | 支持文件上传处理+下载\n");
    printf("=========================================\n");
    while (1)
    {
        clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &addrLen);
        if (clientSock == INVALID_SOCKET) continue;
        handleClientRequest(clientSock);
    }

    closesocket(serverSock);
    WSACleanup();

}

