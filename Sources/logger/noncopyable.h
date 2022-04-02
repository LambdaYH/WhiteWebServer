#ifndef WHITEWEBSERVER_LOGGER_NONCOPYABLE_H_
#define WHITEWEBSERVER_LOGGER_NONCOPYABLE_H_

namespace white {

class Noncopyable
{
protected:
    Noncopyable(){}
    ~Noncopyable(){}

private:
    Noncopyable &operator=(const Noncopyable &rhs) = delete;
    Noncopyable(const Noncopyable &ori) = delete;

};

} // namespace  white

#endif