#include "servlet.h"
#include <fnmatch.h>

namespace will {
namespace http{

int32_t FunctionServlet::handle(will::http::HttpRequest::ptr request,
            will::http::HttpResponse::ptr response,
            will::http::HttpSession::ptr session) {
    return m_cb(request, response, session);
}

FunctionServlet::FunctionServlet(callback cb)
    :Servlet("FunctionServlet")
    ,m_cb(cb) {
}

ServletDispatch::ServletDispatch()
    :Servlet("ServletDispatch"){
    m_default.reset(new NotFoundServlet());
}

int32_t ServletDispatch::handle(will::http::HttpRequest::ptr request,
            will::http::HttpResponse::ptr response,
            will::http::HttpSession::ptr session) {
    auto slt = getMatchedServlet(request->getPath());
    if(slt) {
        slt->handle(request, response, session);
    }
    return 0;
}
    
void ServletDispatch::addServlet(const std::string& uri, Servlet::ptr slt) {
    RWMutexType::WriteLock lock(m_mutex);
    m_datas[uri] = slt;
}

void ServletDispatch::addServlet(const std::string& uri, FunctionServlet::callback cb) {
    RWMutexType::WriteLock lock(m_mutex);
    m_datas[uri].reset(new FunctionServlet(cb));
}

void ServletDispatch::addGlobServlet(const std::string& uri, Servlet::ptr slt) {
    RWMutexType::WriteLock lock(m_mutex);
    for(auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
    m_globs.push_back(std::make_pair(uri, slt));
}

void ServletDispatch::addGlobServlet(const std::string& uri, FunctionServlet::callback cb) {
    return addGlobServlet(uri, FunctionServlet::ptr(new FunctionServlet(cb)));
}

void ServletDispatch::delServlet(const std::string& uri) {
    RWMutexType::WriteLock lock(m_mutex);
    m_datas.erase(uri);
}

void ServletDispatch::delGlobServlet(const std::string& uri) {
    RWMutexType::WriteLock lock(m_mutex);
    for(auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
}

Servlet::ptr ServletDispatch::getServlet(const std::string& uri) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_datas.find(uri);
    return it == m_datas.end() ? nullptr : it->second;
}

Servlet::ptr ServletDispatch::getGlobServlet(const std::string& uri) {
    RWMutexType::ReadLock lock(m_mutex);
    for(auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        if(it->first == uri) {
            return it->second;
        }
    }
    return nullptr;
}

Servlet::ptr ServletDispatch::getMatchedServlet(const std::string& uri) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_datas.find(uri);
    if(it != m_datas.end()) {
        return it->second;
    }
    for(auto it2 = m_globs.begin(); it2 != m_globs.end(); ++it2) {
        if(!fnmatch(it2->first.c_str(), uri.c_str(), 0)) {
            return it2->second;
        }
    }
    return m_default;
}

NotFoundServlet::NotFoundServlet()
            : Servlet("NotFoundServlet"){

}
int32_t NotFoundServlet::handle(will::http::HttpRequest::ptr request,
            will::http::HttpResponse::ptr response,
            will::http::HttpSession::ptr session) {
    static const std::string& RSP_BODY = "<html><head><title>404 Not Found"
        "</title></head><body><center><h1>404 Not Found</h1></center>"
        "<hr><center>Hello, This is My Server!" 
        "</center></body></html>";
    response->setStatus(will::http::HttpStatus::NOT_FOUND);
    response->setHeader("Content-Type", "text/html");
    response->setBody(RSP_BODY);
    return 0;
}

}
}