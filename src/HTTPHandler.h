#ifndef HTTPHandler_H
#define HTTPHandler_H

#include <string>
#include <vector>
#include <functional>

namespace HLS
{
	class HTTPHandler
	{
	public:
		HTTPHandler();
		~HTTPHandler();
		int getRequest(std::string &url,
					   std::vector<std::pair<std::string, std::string>> &args,
					   std::function<void(void *, size_t)> response);

	private:
		void *m_curl;
	};
}
#endif // !
