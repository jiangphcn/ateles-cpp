#ifndef ATELES_EXCEPTIONS_H
#define ATELES_EXCEPTIONS_H

#include <exception>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

namespace ateles
{
class AtelesExit : public std::exception {
};

class AtelesError : public std::exception {
  public:
    explicit AtelesError(const std::string& what) : _what(what) {}

    virtual ~AtelesError() throw() {}

    virtual grpc::StatusCode code() const throw()
    {
        return grpc::StatusCode::INTERNAL;
    }

    virtual const char* what() const throw() { return this->_what.c_str(); }

  protected:
    std::string _what;
};

class AtelesInvalidArgumentError : public AtelesError {
  public:
    explicit AtelesInvalidArgumentError(const std::string& what)
        : AtelesError(what)
    {
    }

    virtual grpc::StatusCode code() const throw()
    {
        return grpc::StatusCode::INVALID_ARGUMENT;
    }
};

class AtelesNotFoundError : public AtelesError {
  public:
    explicit AtelesNotFoundError(const std::string& what) : AtelesError(what) {}

    virtual grpc::StatusCode code() const throw()
    {
        return grpc::StatusCode::NOT_FOUND;
    }
};

class AtelesResourceExhaustedError : public AtelesError {
  public:
    explicit AtelesResourceExhaustedError(const std::string& what)
        : AtelesError(what)
    {
    }

    virtual grpc::StatusCode code() const throw()
    {
        return grpc::StatusCode::RESOURCE_EXHAUSTED;
    }
};

}  // namespace ateles

#endif  // included exceptions.h