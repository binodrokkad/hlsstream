#include "HTTPHandler.h"
#include <iostream>
#include <curl/curl.h>

using namespace HLS;

size_t writeCB(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t dataLen = size * nmemb;
	std::function<void(void *, size_t)> *retFunction = (std::function<void(void *, size_t)> *)userdata;
	(*retFunction)(ptr, dataLen);
	return dataLen;
}

HTTPHandler::HTTPHandler() : m_curl(nullptr)
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
	m_curl = curl_easy_init();
}

HTTPHandler::~HTTPHandler()
{
	curl_easy_cleanup(m_curl);
	curl_global_cleanup();
	m_curl = nullptr;
}

int HTTPHandler::getRequest(std::string &url,
							std::vector<std::pair<std::string, std::string>> &args,
							std::function<void(void *, size_t)> response)
{
	// append args;
	std::string mainUrl = url + "?";
	for (auto &arg : args)
	{
		mainUrl += arg.first + "=" + arg.second + "&";
	}

	curl_easy_setopt(m_curl, CURLOPT_URL, mainUrl.c_str());
	curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeCB);
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);

	CURLcode res = curl_easy_perform(m_curl);
	if (res != CURLE_OK)
	{
		std::cout << "curl_easy_perform() failed" << curl_easy_strerror(res);
	}

	return res;
}
