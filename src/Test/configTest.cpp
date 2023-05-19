#include <yaml-cpp/yaml.h>

#include <iostream>
#include <format>

int main()
{
    auto config = YAML::LoadFile("./config.yaml");
    std::cout << config["Width"].as<int>() << std::endl;
    std::cout << config["Height"].as<int>() << std::endl;
    std::cout << config["LightPassLoop"].as<uint32_t>() << std::endl;
    std::cout << config["PostProcessLoop"].as<int>() << std::endl;
}
