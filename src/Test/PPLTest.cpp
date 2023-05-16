#include <ppl.h>
#include <iostream>
#include <format>
using namespace concurrency;

int main()
{
    task_group tg;
    while (true) {
        static int i = 0;
        i++;
        tg.run([]() {
            printf("Hello ");
        });
        tg.run([&]() {
            static bool isFirstFrame = true;
            if (isFirstFrame) {
                isFirstFrame = false;
                printf("First Frame\n");
                return;
            }
            static int u = i;
            static int v = 0;
            v = i;
            std::printf("Skip First Frame %d %d\n", u, v);
        });
        tg.wait();
        std::cout << " ================== " << i << " ================== \n";
        if (i == 10) {
            break;
        }
    }
    for (size_t i = 0; i < 4;i ++) {
        static int u = i;
        static int v = i;
        v = i;
        auto str = std::format("Static Test {} {}\n", u, v);
        std::cout << str;
    }
}