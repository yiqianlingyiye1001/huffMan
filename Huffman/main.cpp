#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include<string.h>

#include "hd.h"
#include "huffMan.h"
#include "backend.h"


using namespace std;


// 主函数 - 服务启动入口
int main()
{
    SetConsoleOutputCP(CP_UTF8); //调整控制台编码格式

    int a = pureHUFFMAN();

    backend();

    return 0;
}
