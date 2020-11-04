// MiniNVR.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <cstdint>
#include <tchar.h>
#include "NVRManager.h"
#include "HTTPServerRequestHandler.h"
#include <NVRLiveStreamer.h>

int _tmain(int32_t argc, TCHAR* argv[])
{
    NVRManager nvr;

    std::vector<std::string> options;
    options.push_back("document_root");
    options.push_back("html");
    options.push_back("enable_directory_listing");
    options.push_back("no");
    options.push_back("additional_header");
    options.push_back("X-Frame-Options: SAMEORIGIN");
    options.push_back("access_control_allow_origin");
    options.push_back("*");
    options.push_back("listening_ports");
    options.push_back("8080");
    options.push_back("enable_keep_alive");
    options.push_back("yes");
    options.push_back("keep_alive_timeout_ms");
    options.push_back("1000");

    try
    {
        std::map<std::string, HTTPServerRequestHandler::HTTPFunction> func = nvr.GetAPI();
        HTTPServerRequestHandler http_server(func, options);
        std::cout << "Start HTTP Sever :" << 8080 << std::endl;
        ::getchar();
    }
    catch (const CivetException& ex)
    {
        std::cout << "Cannot Initialize start HTTP sever exception:" << ex.what() << std::endl;
    }

    std::cout << "exit" << std::endl;
    return 0;
}
