#include "../will/http/http_server.h"

static will::Logger::ptr g_logger = WILL_LOG_ROOT();

#define XX(...) #__VA_ARGS__

will::IOManager::ptr worker;

void run() {
    g_logger->setLevel(will::LogLevel::INFO);
    
    will::http::HttpServer::ptr server(new will::http::HttpServer(true));
    will::Address::ptr addr = will::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while (!server->bind(addr)) {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/will/xx", [](will::http::HttpRequest::ptr req, will::http::HttpResponse::ptr rsp, will::http::HttpSession::ptr session) {
        rsp->setBody(req->toString());
        return 0;
    });

    sd->addGlobServlet("/will/*", [](will::http::HttpRequest::ptr req, will::http::HttpResponse::ptr rsp, will::http::HttpSession::ptr session) {
        rsp->setBody("Glob:\r\n" + req->toString());
        return 0;
    });

    sd->addGlobServlet("/willx/*", [](will::http::HttpRequest::ptr req, will::http::HttpResponse::ptr rsp, will::http::HttpSession::ptr session) {
        rsp->setBody(XX(<html>
                                <head><title> 404 Not Found</ title></ head>
                                <body>
                                <center><h1> 404 Not Found</ h1></ center>
                                <hr><center>
                                    nginx /
                                1.16.0 <
                            / center >
                            </ body>
                            </ html> < !--a padding to disable MSIE and
                        Chrome friendly error page-- >
                            < !--a padding to disable MSIE and
                        Chrome friendly error page-- >
                            < !--a padding to disable MSIE and
                        Chrome friendly error page-- >
                            < !--a padding to disable MSIE and
                        Chrome friendly error page-- >
                            < !--a padding to disable MSIE and
                        Chrome friendly error page-- >
                            < !--a padding to disable MSIE and
                        Chrome friendly error page-- >));
        return 0;
    });

    server->start();
}

int main(int argc, char **argv) {
    will::IOManager iom(1, true, "main");
    worker.reset(new will::IOManager(3, false, "worker"));
    iom.schedule(run);
    return 0;
}
