#ifndef __HTTP_SESSION_H__
#define __HTTP_SESSION_H__

#include "../socket_stream.h"
#include "http.h"
#include "../log.h"
/*session和connection类都是对连接后产生的socket进行的封装
  区别在于：
  session用于封装服务端accept产生的socket
  connection用于封装客户端connect产生的socket
  */
namespace will {
namespace http{
class HttpSession : public SocketStream {
public:
    typedef std::shared_ptr<HttpSession> ptr;
    HttpSession(Socket::ptr sock, bool owner = true);
    HttpRequest::ptr recvRequest();
    int sendResponse(HttpResponse::ptr rsp);

};


}
}

#endif

