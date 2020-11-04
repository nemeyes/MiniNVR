#pragma once

#include "HTTPServerRequestHandler.h"
#include <NVRLiveStreamer.h>

class NVRManager
	: public Base
{
public:
	NVRManager(void);
	~NVRManager(void);

	const std::map<std::string, HTTPServerRequestHandler::HTTPFunction> GetAPI(void);

	const Json::Value AddStream(const std::string& name, const std::string& url);
	const Json::Value RemoveStream(const std::string& name);
	const Json::Value StartStream(const std::string& name);
	const Json::Value StopStream(const std::string& name);


private:
	std::map<std::string, HTTPServerRequestHandler::HTTPFunction> _func;
	NVRLiveStreamer _live_streamer;
};