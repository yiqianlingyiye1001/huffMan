#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include<string.h>

#include <windows.h>
#include "hd.h"

using namespace std;

int weight[256] = {0};

//二进制读取原始文件
SqString readOriginalFile(const char *originalFile)
{

    SqString data;
    data.length=0;
    FILE* fp=fopen(originalFile,"rb");
    if(!fp)
    {
        printf("读取文件失败awa！！！\n");
        return data;
    }
    else printf("读取成功qwq！！！\n");
    //读取文件
    fseek(fp,0,SEEK_END);
    data.length = ftell(fp);
    fseek(fp,0,SEEK_SET);
    //data.s = (unsigned char*)malloc(1*data.length);
    data.s = new unsigned char[data.length];

    fread(data.s,1,data.length,fp);
    fclose(fp);
    return data;
}


//获取每个字符的权重值
int getWeight(SqString msg)
{
    memset(weight, 0, sizeof(weight));
    for(int i=0; i<msg.length; i++)//'\0'
    {
        weight[(unsigned char)msg.s[i]]++;
    }
    int n0=0;
    for(int i=0; i<256; i++)
    {
        if(weight[i]!=0)
            n0++;
    }
    return n0;
}
void createNode(HTNode ht[],SqString msg)
{
    for(int i=0,j=0; i<256; i++)
    {
        if(weight[i]!=0)
        {
            ht[j].data=(unsigned char)i;
            ht[j].weight = weight[i];
            j++;
        }
    }
}

//创建哈夫曼树
void  createHT(HTNode ht[],int n0)//HTNode *ht???
{
    int i;

    for(i=0; i<n0*2-1; i++)
        ht[i].parent=ht[i].lchild=ht[i].rchild=-1;

    int k,lnode,rnode;
    int min1,min2;

    for(i=n0; i<2*n0-1; i++)
    {
        min1=min2= 0x7FFFFFFF;
        lnode=rnode=-1;
        for(k=0; k<i; k++)
        {
            if(ht[k].parent==-1)
            {
                if(ht[k].weight<min1)
                {
                    min2=min1;
                    rnode=lnode;
                    min1=ht[k].weight;
                    lnode=k;
                }
                else if(ht[k].weight<min2)
                {
                    min2=ht[k].weight;
                    rnode=k;
                }
            }
        }
        ht[i].weight=ht[lnode].weight+ht[rnode].weight;
        ht[i].lchild=lnode;
        ht[i].rchild=rnode;
        ht[lnode].parent=ht[rnode].parent=i;
    }
}
//创建哈夫曼树节点
void createHCode(HTNode ht[],HCode hcd[],int n0)
{
    HCode hc;
    int c,f;
    for(int i=0; i<n0; i++)
    {
        c=i;
        hc.start=n0-1;
        f=ht[i].parent;
        //栈溢出?

        hc.cd = new char[n0];

        while(f!=-1)
        {
            if(ht[f].lchild==c)
                hc.cd[hc.start--]='0';
            else
                hc.cd[hc.start--]='1';
            c=f;
            f=ht[f].parent;
        }
        hc.start++;
        hcd[i]=hc;
    }
}

//获取哈夫曼数据
char* printHCode(SqString msg,HTNode ht[],HCode hcd[],int n0)
{
    //char* msgCd = (char*)malloc(MM*sizeof(char));
    //溢栈问题
    int totalLen=0;
    for(int i=0; i<msg.length; i++)
    {
        for(int j=0; j<n0; j++)
        {
            if(msg.s[i]==ht[j].data)
            {
                totalLen +=(n0-hcd[j].start);
                break;
            }
        }
    }
    char* msgCd = new char[totalLen+1];

    int q=0;

    for(int i=0; i<msg.length; i++)
    {
        for(int j=0; j<n0; j++)
        {
            if(msg.s[i]==ht[j].data)
            {
                for(int k = hcd[j].start; k < n0; k++)
                {
                    msgCd[q++] = hcd[j].cd[k];
                }
                break;
            }
        }
    }
    msgCd[q] = '\0';
    return msgCd;
}

//压缩文件
void runCompress(FILE* fp1,char* hufCode)
{
    int hufCodeLen = strlen(hufCode);
    unsigned char byte=0;
    int bitCount=0;
    for(int i=0; i<hufCodeLen; i++)
    {
        byte <<= 1;

        if(hufCode[i]=='1')
        {
            byte |= 1;
        }
        bitCount++;


        if(bitCount == 8)
        {
            fwrite(&byte,sizeof(unsigned char),1,fp1);
            byte=0;
            bitCount=0;
        }
    }
    if(bitCount>0)
    {
        byte <<= (8-bitCount);
        fwrite(&byte,sizeof(unsigned char),1,fp1);
    }
}

//解压文件
void rundeCompress(const char* compressFile,const char* decompressFile,FILE* fpCompress,FILE* fpDecompress)
{
    int N0=0;
    fread(&N0,sizeof(int),1,fpCompress);
    HTNode* decodeHT = new HTNode[2*N0-1];
    fread(decodeHT,sizeof(HTNode),2*N0-1,fpCompress);
    int originalLen;
    fread(&originalLen,sizeof(int),1,fpCompress);

    int bitPos=0;
    unsigned char byteBuf=0;
    int currentNode = 2*N0-2;
    int decodeBytes=0;

    while(decodeBytes<originalLen)
    {
        if(bitPos ==0)
        {
            if(fread(&byteBuf,sizeof(unsigned char),1,fpCompress)!=1)
                break;
            bitPos = 8;
        }
        bitPos -= 1;

        int bit = ((byteBuf)>>(bitPos)&1);

        if(bit==0)
        {
            currentNode = decodeHT[currentNode].lchild;
        }
        else
        {
            currentNode = decodeHT[currentNode].rchild;
        }

        if(decodeHT[currentNode].lchild==-1&&decodeHT[currentNode].rchild==-1)
        {
            fwrite(&decodeHT[currentNode].data,sizeof(unsigned char),1,fpDecompress);
            decodeBytes++;
            currentNode=2*N0-2;
        }
    }

    delete[] decodeHT;
}





