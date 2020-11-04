#pragma once

#include <list>
#include <map>
#include <functional>

#include "json/json.h"
#include "CivetServer.h"


class HTTPServerRequestHandler : public CivetServer
{
public:
	typedef std::function<Json::Value(const struct mg_request_info* req_info, const Json::Value&)> HTTPFunction;
	HTTPServerRequestHandler(std::map<std::string, HTTPFunction>& func, const std::vector<std::string>& options);
};