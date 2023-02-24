#ifndef __HTTP_SERVLET_H__
#define __HTTP_SERVLET_H__

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include "http.h"
#include "http_session.h"
#include "../thread.h"
#include "../log.h"

namespace will {
namespace http {

/**
 * servlet是仿照java实现的
 * 可以实现动态网页
 **/
class Servlet {
public:
    typedef std::shared_ptr<Servlet> ptr;

    Servlet(const std::string& name)
    :m_name(name) {}
    virtual ~Servlet(){}
    virtual int32_t handle(will::http::HttpRequest::ptr request,
                        will::http::HttpResponse::ptr response,
                        will::http::HttpSession::ptr session) = 0;
    const std::string& getName() const { return m_name; }
private:
    std::string m_name;
};

class FunctionServlet : public Servlet {
public:
    typedef std::shared_ptr<FunctionServlet> ptr;
    typedef std::function<int32_t(will::http::HttpRequest::ptr request,
                                will::http::HttpResponse::ptr response,
                                will::http::HttpSession::ptr session)> callback;
    FunctionServlet(callback cb);
    virtual int32_t handle(will::http::HttpRequest::ptr request,
                        will::http::HttpResponse::ptr response,
                        will::http::HttpSession::ptr session) override;
private:
    callback m_cb;
};

// 管理维护servlet之间的关系
class ServletDispatch : public Servlet {
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef RWMutex RWMutexType; 
    virtual int32_t handle(will::http::HttpRequest::ptr request,
                will::http::HttpResponse::ptr response,
                will::http::HttpSession::ptr session) override;
    ServletDispatch();

    void addServlet(const std::string& uri, Servlet::ptr slt);
    void addServlet(const std::string& uri, FunctionServlet::callback cb);
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    Servlet::ptr getDefault() const { return m_default; }
    void setDefault(Servlet::ptr v) { m_default = v; }

    Servlet::ptr getServlet(const std::string& uri);
    Servlet::ptr getGlobServlet(const std::string& uri);

    Servlet::ptr getMatchedServlet(const std::string& uri);
private:
    RWMutexType m_mutex;
    // 精准uri分配 /will/xxx -> servlet
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    // 模糊uri分配 /will/* -> servlet
    std::vector<std::pair<std::string, Servlet::ptr> > m_globs;
    // 默认servlet
    Servlet::ptr m_default;
};

class NotFoundServlet : public Servlet {
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;
    NotFoundServlet();
    virtual int32_t handle(will::http::HttpRequest::ptr request,
                will::http::HttpResponse::ptr response,
                will::http::HttpSession::ptr session) override;
};

}
}

#endif