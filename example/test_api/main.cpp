#include <iostream>
#include <iomanip>
#include <TopocodingApi.hpp>

int main()
{
        std::cout << "Hello world!" << std::endl;
        TopocodingApi api;
        std::cout << api.getAltitudes(60.032698,30.2555) << std::endl;
        while(1);
        return 0;
}

