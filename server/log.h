#ifndef __WILL_LOG_H__
#define __WILL_LOG_H__

#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cstdarg>
#include <list>
#include <map>
#include "util.h"
#include "mutex.h"
#include "singleton.h"

#define WILL_LOG_ROOT() will::LoggerMgr::GetInstance()->getRoot()

#define WILL_LOG_NAME(name) will::LoggerMgr::GetInstance()->getLogger(name)

// 使用流式方式将日志级别level的日志写入到logger
// 构造一个LogEventWrap对象，包裹包含日志器和日志事件，在对象析构时调用日志器写日志事件
// 协程id未实现，暂时写0

#define WILL_LOG_LEVEL(logger , level) \
    if(level <= logger->getLevel()) \
        will::LogEventWrap(logger, will::LogEvent::ptr(new will::LogEvent(logger->getName(), \
            level, __FILE__, __LINE__, will::GetElapsedMS() - logger->getCreateTime(), \
            will::GetThreadId(), will::GetFiberId(), time(0), will::GetThreadName()))).getLogEvent()->getSS()

#define WILL_LOG_FATAL(logger) WILL_LOG_LEVEL(logger, will::LogLevel::FATAL)

#define WILL_LOG_ALERT(logger) WILL_LOG_LEVEL(logger, will::LogLevel::ALERT)

#define WILL_LOG_CRIT(logger) WILL_LOG_LEVEL(logger, will::LogLevel::CRIT)

#define WILL_LOG_ERROR(logger) WILL_LOG_LEVEL(logger, will::LogLevel::ERROR)

#define WILL_LOG_WARN(logger) WILL_LOG_LEVEL(logger, will::LogLevel::WARN)

#define WILL_LOG_NOTICE(logger) WILL_LOG_LEVEL(logger, will::LogLevel::NOTICE)

#define WILL_LOG_INFO(logger) WILL_LOG_LEVEL(logger, will::LogLevel::INFO)

#define WILL_LOG_DEBUG(logger) WILL_LOG_LEVEL(logger, will::LogLevel::DEBUG)

namespace will {

class LogLevel {
public:
    // 日志级别枚举，参考log4cpp
    enum Level { 
        // 致命情况，系统不可用
        FATAL  = 0,
        // 高优先级情况，例如数据库系统崩溃
        ALERT  = 100,
        // 严重错误，例如硬盘错误
        CRIT   = 200,
        // 错误
        ERROR  = 300,
        // 警告
        WARN   = 400,
        // 正常但值得注意
        NOTICE = 500,
        // 一般信息
        INFO   = 600,
        // 调试信息
        DEBUG  = 700,
        // 未设置
        NOTSET = 800,
    };

    // 日志级别转字符串
    // level 日志级别 
    // 字符串形式的日志级别
    static const char *ToString(LogLevel::Level level);

    // 字符串转日志级别
    // str 字符串 
    // 日志级别
    // 不区分大小写
    static LogLevel::Level FromString(const std::string &str);
};

class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;

    // logger_name 日志器名称
    // level 日志级别
    // file 文件名
    // line 行号
    // elapse 从日志器创建开始到当前的累计运行毫秒
    // thead_id 线程id
    // fiber_id 协程id
    // time UTC时间
    // thread_name 线程名称
    LogEvent(const std::string &logger_name, 
             LogLevel::Level level, const char *file, int32_t line,
             int64_t elapse, uint32_t thread_id, uint64_t fiber_id, 
             time_t time, const std::string &thread_name);

    LogLevel::Level getLevel() const { return m_level; }

    std::string getContent() const { return m_ss.str(); }

    std::string getFile() const { return m_file; }

    int32_t getLine() const { return m_line; }

    int64_t getElapse() const { return m_elapse; }

    uint32_t getThreadId() const { return m_threadId; }

    uint64_t getFiberId() const { return m_fiberId; }

    time_t getTime() const { return m_time; }

    const std::string &getThreadName() const { return m_threadName; }

    std::stringstream &getSS() { return m_ss; }

    const std::string &getLoggerName() const { return m_loggerName; }

private:
    // 日志级别
    LogLevel::Level m_level;
    // 日志内容，使用stringstream存储，便于流式写入日志
    std::stringstream m_ss;
    // 文件名
    const char *m_file = nullptr;
    // 行号
    int32_t m_line = 0;
    // 从日志器创建开始到当前的耗时
    int64_t m_elapse = 0;
    // 线程id
    uint32_t m_threadId = 0;
    // 协程id
    uint64_t m_fiberId = 0;
    // 时间戳
    time_t m_time;
    // 线程名称
    std::string m_threadName;
    // 日志器名称
    std::string m_loggerName;
};

class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;

    // %%m 消息
    // %%p 日志级别
    // %%c 日志器名称
    // %%d 日期时间，后面可跟一对括号指定时间格式，比如%%d{%%Y-%%m-%%d %%H:%%M:%%S}，这里的格式字符与C语言strftime一致
    // %%r 该日志器创建后的累计运行毫秒数
    // %%f 文件名
    // %%l 行号
    // %%t 线程id
    // %%F 协程id
    // %%N 线程名称
    // %%% 百分号
    // %%T 制表符
    // %%n 换行
    // 默认格式：%%d{%%Y-%%m-%%d %%H:%%M:%%S}%%T%%t%%T%%N%%T%%F%%T[%%p]%%T[%%c]%%T%%f:%%l%%T%%m%%n
    // 
    // 默认格式描述：年-月-日 时:分:秒 [累计运行毫秒数] \\t 线程id \\t 线程名称 \\t 协程id \\t [日志级别] \\t [日志器名称] \\t 文件名:行号 \\t 日志消息 换行符
    
    LogFormatter(const std::string &pattern = "%d{%Y-%m-%d %H:%M:%S} [%rms]%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n");

    void init();

    bool isError() const { return m_error; }

    std::string format(LogEvent::ptr event);

    std::ostream &format(std::ostream &os, LogEvent::ptr event);

    std::string getPattern() const { return m_pattern; }

public:

    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;

        virtual ~FormatItem() {}

        virtual void format(std::ostream &os, LogEvent::ptr event) = 0;
    };

private:
    // 日志格式模板
    std::string m_pattern;
    // 解析后的格式模板数组
    std::vector<FormatItem::ptr> m_items;
    // 是否出错
    bool m_error = false;
};

// 日志输出地，虚基类，用于派生出不同的LogAppender
class LogAppender {
public:
    typedef std::shared_ptr<LogAppender> ptr;
    typedef Spinlock MutexType;

    // default_formatter 默认日志格式器
    LogAppender(LogFormatter::ptr default_formatter);
    
    virtual ~LogAppender() {}

    void setFormatter(LogFormatter::ptr val);

    LogFormatter::ptr getFormatter();

    virtual void log(LogEvent::ptr event) = 0;

protected:
    // Mutex
    MutexType m_mutex;
    // 日志格式器
    LogFormatter::ptr m_formatter;
    // 默认日志格式器
    LogFormatter::ptr m_defaultFormatter;
};

class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;

    StdoutLogAppender();

    void log(LogEvent::ptr event) override;
};

class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;

    FileLogAppender(const std::string &file);

    void log(LogEvent::ptr event) override;

    bool reopen();

private:
    // 文件路径
    std::string m_filename;
    // 文件流
    std::ofstream m_filestream;
    // 上次重打打开时间
    uint64_t m_lastTime = 0;
    // 文件打开错误标识
    bool m_reopenError = false;
};

class Logger{
public:
    typedef std::shared_ptr<Logger> ptr;
    typedef Spinlock MutexType;

    Logger(const std::string &name="default");

    const std::string &getName() const { return m_name; }

    const uint64_t &getCreateTime() const { return m_createTime; }

    void setLevel(LogLevel::Level level) { m_level = level; }

    LogLevel::Level getLevel() const { return m_level; }

    void addAppender(LogAppender::ptr appender);

    void delAppender(LogAppender::ptr appender);

    void clearAppenders();

    void log(LogEvent::ptr event);

private:
    // Mutex
    MutexType m_mutex;
    // 日志器名称
    std::string m_name;
    // 日志器等级
    LogLevel::Level m_level;
    // LogAppender集合
    std::list<LogAppender::ptr> m_appenders;
    // 创建时间（毫秒）
    uint64_t m_createTime;
};

class LogEventWrap{
public:
    LogEventWrap(Logger::ptr logger, LogEvent::ptr event);

    ~LogEventWrap();

    LogEvent::ptr getLogEvent() const { return m_event; }

private:
    // 日志器
    Logger::ptr m_logger;
    // 日志事件
    LogEvent::ptr m_event;
};

class LoggerManager{
public:
    typedef Spinlock MutexType;

    LoggerManager();

    void init();

    Logger::ptr getLogger(const std::string &name);

    Logger::ptr getRoot() { return m_root; }

private:
    // Mutex
    MutexType m_mutex;
    // 日志器集合
    std::map<std::string, Logger::ptr> m_loggers;
    // root日志器
    Logger::ptr m_root;
};

// 日志器管理类单例
typedef will::Singleton<LoggerManager> LoggerMgr;

} // end namespace will

#endif // __WILL_LOG_H__
