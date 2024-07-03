#include <iostream>
#include <thread>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
using namespace std;
std::mutex button_mutex;

struct status {
    bool floor_1_idle;
    bool floor_2_idle;
    bool floor_1_open;
    bool floor_2_open;
    bool lift;
    bool down;
} stat;

class elevator {
private:
    int timer;
    unsigned short button;

public:
    elevator() : timer(0), button(0) {
        stat.floor_1_idle = true;
        stat.floor_2_idle = false;
        stat.floor_1_open = false;
        stat.floor_2_open = false;
        stat.lift = false;
        stat.down = false;
    }

    void statChanging(unsigned short button);
};

void elevator::statChanging(unsigned short button) {
    if (stat.floor_1_idle) {
        if (button & (1 << 0) || button & (1 << 2)) {
            stat.floor_1_open = true;
            stat.floor_1_idle = false;
            button_mutex.lock();
            button |= 0;
            button_mutex.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(2));
        } else if (button & (1 << 1)|| button & (1 << 3)) {
            stat.lift = true;
            stat.floor_1_idle = false;
            button_mutex.lock();
            button = 0;
            button_mutex.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    else if(stat.floor_1_open){
        if (button & (1 << 0) || button & (1 << 2)) {
            stat.floor_1_open = true;
            stat.floor_1_idle = false;
            button_mutex.lock();
            button = 0;
            button_mutex.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(2));
        } else if (button & (1 << 1)|| button & (1 << 3)) {
            stat.lift = true;
            stat.floor_1_idle = false;
            button_mutex.lock();
            button = 0;
            button_mutex.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        else{
            stat.floor_1_idle = true;
            stat.floor_1_open = false;
        }
    }
    else if(stat.floor_2_idle){
        if (button & (1 << 1)|| button & (1 << 3)) {
            stat.floor_2_open = true;
            stat.floor_2_idle = false;
            button_mutex.lock();
            button = 0;
            button_mutex.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(2));
        } 
        else if (button & (1 << 0) || button & (1 << 2)) {
            stat.down = true;
            stat.floor_2_idle = false;
            button_mutex.lock();
            button = 0;
            button_mutex.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    else if(stat.floor_2_open){
        if (button & (1 << 1)|| button & (1 << 3)) {
            stat.floor_2_open = true;
            stat.floor_2_idle = false;
            button_mutex.lock();
            button = 0;
            button_mutex.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(2));
        } else if (button & (1 << 0) || button & (1 << 2)) {
            stat.down = true;
            stat.floor_2_idle = false;
            button_mutex.lock();
            button = 0;
            button_mutex.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        else{
            stat.floor_2_idle = true;
            stat.floor_2_open = false;
        }
    }
    else if(stat.lift){
        stat.lift = false;
        stat.floor_2_open = true;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    else if(stat.down){
        stat.down = false;
        stat.floor_1_open = true;
    }
}

void sendState(int clientSockfd) {
    char message[100];
    snprintf(message, sizeof(message), "Elevator state: floor_1_idle=%d, floor_2_idle=%d, floor_1_open=%d, floor_2_open=%d, lift=%d, down=%d", 
             stat.floor_1_idle, stat.floor_2_idle, stat.floor_1_open, stat.floor_2_open, stat.lift, stat.down);
    send(clientSockfd, message, sizeof(message), 0);
}

void clientHandler(int clientSockfd) {
    char buffer[10] = {0};
    elevator e;
    while (1) {
        recv(clientSockfd, buffer, sizeof(buffer), 0);
        unsigned short button = atoi(buffer);
        //printf("Received button input: %hu\n", button);
        e.statChanging(button);
        sendState(clientSockfd);
    }
}


void printState() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::lock_guard<std::mutex> lock(button_mutex);
        if(stat.floor_1_idle) cout << "floor_1_idle\n";
        else if(stat.floor_2_idle) cout << "floor_2_idle\n";
        else if(stat.floor_1_open) cout << "floor_1_open\n";
        else if(stat.floor_2_open) cout << "floor_2_open\n";
        else if(stat.lift) cout << "lift\n";
        else if(stat.down) cout << "down\n";
        else cout << "error\n";
    }
}

int main() {
    // 創建 socket
    int serverSockfd, clientSockfd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    serverSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSockfd == -1) {
        perror("Fail to create a socket");
        return -1;
    }

    // 綁定地址和端口
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8700);

    if (bind(serverSockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        return -1;
    }

    // 監聽連線
    if (listen(serverSockfd, 3) < 0) {
        perror("Listen failed");
        return -1;
    }

    std::cout << "Server listening on port 8700...\n";

    // 接受客戶端連線
    clientSockfd = accept(serverSockfd, (struct sockaddr *)&clientAddr, &addrLen);
    if (clientSockfd < 0) {
        perror("Accept failed");
        return -1;
    }

    std::cout << "Client connected\n";
    std::thread printThread(printState);

    // 處理客戶端請求
    clientHandler(clientSockfd);

    // 等待print結束
    printThread.join();

    // 處理客戶端請求
    clientHandler(clientSockfd);

    close(clientSockfd);
    close(serverSockfd);

    return 0;
}
