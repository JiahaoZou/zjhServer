#include "http_server.h"

namespace will {

namespace http {

HttpServer::HttpServer(bool keeplive
                   ,will::IOManager* worker
                   ,will::IOManager* io_worker
                   ,will::IOManager* accept_worker) 
    :TcpServer(io_worker,accept_worker)
    ,m_isKeepLive(keeplive){
    m_dispatch.reset(new ServletDispatch());
}

void HttpServer::handleClient(Socket::ptr client) {
    HttpSession::ptr session(new HttpSession(client));
    do {
        auto req = session->recvRequest();
        if(!req) {
            std::cout << "recv http request fail" << std::endl;
            break;
        }

        HttpResponse::ptr rsp(new HttpResponse(
           req->getVersion(), req->isClose() || !m_isKeepLive));
        
        rsp->setHeader("Server", getName());
        m_dispatch->handle(req, rsp, session);
        //rsp->setBody("hello, this is my reply!");
        session->sendResponse(rsp);
        
        if(!m_isKeepLive || req->isClose()) {
            break;
        }
    } while(true);
    session->close();
}

}
}