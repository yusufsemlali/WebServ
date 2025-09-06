#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <map>
#include "HttpResponse.hpp"

class CgiHandler 
{
public:
    CgiHandler();
    ~CgiHandler();

    CgiHandler(HttpResponse &res) : response(res) {}
    void ExecuteCgi (const std::string& scriptName, std::string pathCgi);
    


private:
    HttpResponse &response;

};




#endif // CGIHANDLER_HPP