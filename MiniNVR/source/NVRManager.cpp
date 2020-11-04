#pragma once

#include "NVRManager.h"

NVRManager::NVRManager(void)
{
	_func["/api/AddStream"] = [this](const struct mg_request_info* req_info, const Json::Value& in) -> Json::Value {
		std::string name;
		std::string url;
		if (req_info->query_string)
		{
			CivetServer::getParam(req_info->query_string, "name", name);
			CivetServer::getParam(req_info->query_string, "url", url);
		}
		return this->AddStream(name, url);
	};

	_func["/api/RemoveStream"] = [this](const struct mg_request_info* req_info, const Json::Value& in) -> Json::Value {
		std::string name;
		if (req_info->query_string)
		{
			CivetServer::getParam(req_info->query_string, "name", name);
		}
		return this->RemoveStream(name);
	};

	_func["/api/StartStream"] = [this](const struct mg_request_info* req_info, const Json::Value& in) -> Json::Value {
		std::string name;
		if (req_info->query_string)
		{
			CivetServer::getParam(req_info->query_string, "name", name);
		}
		return this->StartStream(name);
	};

	_func["/api/StopStream"] = [this](const struct mg_request_info* req_info, const Json::Value& in) -> Json::Value {
		std::string name;
		if (req_info->query_string)
		{
			CivetServer::getParam(req_info->query_string, "name", name);
		}
		return this->StopStream(name);
	};
}

NVRManager::~NVRManager(void)
{

}

const std::map<std::string, HTTPServerRequestHandler::HTTPFunction> NVRManager::GetAPI(void)
{
	return _func;
}

const Json::Value NVRManager::AddStream(const std::string& name, const std::string& url)
{
	int32_t status = _live_streamer.AddStream(name.c_str(), url.c_str());

	Json::Value answer;
	if (status == NVRManager::ERR_CODE_T::SUCCESS)
		answer = true;
	else
		answer = false;
	return answer;
}

const Json::Value NVRManager::RemoveStream(const std::string& name)
{
	int32_t status = _live_streamer.RemoveStream(name.c_str());

	Json::Value answer;
	if (status == NVRManager::ERR_CODE_T::SUCCESS)
		answer = true;
	else
		answer = false;
	return answer;
}

const Json::Value NVRManager::StartStream(const std::string& name)
{
	int32_t status = _live_streamer.StartStream(name.c_str());

	Json::Value answer;
	if (status == NVRManager::ERR_CODE_T::SUCCESS)
		answer = true;
	else
		answer = false;
	return answer;
}

const Json::Value NVRManager::StopStream(const std::string& name)
{
	int32_t status = _live_streamer.StopStream(name.c_str());

	Json::Value answer;
	if (status == NVRManager::ERR_CODE_T::SUCCESS)
		answer = true;
	else
		answer = false;
	return answer;
}