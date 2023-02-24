#ifndef __WILL_TCP_SERVER_H__
#define __WILL_TCP_SERVER_H__

#include <memory>
#include <functional>
#include "address.h"
#include "iomanager.h"
#include "socket.h"
#include "noncopyable.h"

namespace will {

class TcpServer : public std::enable_shared_from_this<TcpServer>
                    , Noncopyable {
public:
    typedef std::shared_ptr<TcpServer> ptr;

    TcpServer(will::IOManager* io_woker = will::IOManager::GetThis()
              ,will::IOManager* accept_worker = will::IOManager::GetThis());

    virtual ~TcpServer();

    virtual bool bind(will::Address::ptr addr);

    virtual bool bind(const std::vector<Address::ptr>& addrs
                        ,std::vector<Address::ptr>& fails);

    virtual bool start();

    virtual void stop();

    uint64_t getRecvTimeout() const { return m_recvTimeout;}

    std::string getName() const { return m_name;}

    void setRecvTimeout(uint64_t v) { m_recvTimeout = v;}

    virtual void setName(const std::string& v) { m_name = v;}

    bool isStop() const { return m_isStop;}

    virtual std::string toString(const std::string& prefix = "");

protected:
    virtual void handleClient(Socket::ptr client);

    virtual void startAccept(Socket::ptr sock);

protected:
    // 监听Socket数组
    std::vector<Socket::ptr> m_socks;
    // 新连接的Socket工作的调度器
    IOManager* m_ioWorker;
    // 服务器Socket接收连接的调度器
    IOManager* m_acceptWorker;
    // 接收超时时间(毫秒)
    uint64_t m_recvTimeout;
    // 服务器名称
    std::string m_name;
    // 服务器类型
    std::string m_type;
    // 服务是否停止
    bool m_isStop;
};

}

#endif
