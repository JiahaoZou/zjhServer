#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include "../address.h"
#include "../tcp_server.h"
#include "http_session.h"
#include "servlet.h"
#include "../log.h"

namespace will {
namespace http {

class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;
    ~HttpServer() = default;
    HttpServer(bool keeplive = false
               ,will::IOManager* worker = will::IOManager::GetThis()
               ,will::IOManager* io_worker = will::IOManager::GetThis()
               ,will::IOManager* accept_worker = will::IOManager::GetThis());

    ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }

    void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v; }
    
    virtual void handleClient(Socket::ptr client) override;
private:
    bool m_isKeepLive;
    ServletDispatch::ptr m_dispatch;
};

}
}

#endif