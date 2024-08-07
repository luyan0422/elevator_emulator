#include <iostream>
#include <thread>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


std::mutex input_mutex;
std::mutex button_mutex;
unsigned short input_value = 0;
bool button_pressed = false;

void getInput() {
    char buffer[10];
    while (true) {
        std::cout << "Enter button input (1: in elevator press floor 1)\n \
        (2: in elevator press floor 2)\n \
        (3: outside elevator press floor 1)\n \
        (4: outside elevator press floor 2): ";
        std::cin.getline(buffer, sizeof(buffer));

        std::lock_guard<std::mutex> lock(input_mutex);
        input_value = atoi(buffer); // 記錄最新的輸入

        std::lock_guard<std::mutex> button_lock(button_mutex);
        button_pressed = true;

    }
}

void startClient() {
    int sockfd;
    struct sockaddr_in servaddr;

    // 創建 socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Fail to create a socket");
        exit(EXIT_FAILURE);
    }

    // 設定伺服器地址和端口
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(8700);

    // 連接到伺服器
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Connected to server\n";

    char buffer[10];
    char defaultBuffer[10] = "0";
    char recvMessage[100];


    std::thread inputThread(getInput);

    while (1) {
        {
            std::lock_guard<std::mutex> lock(input_mutex);
            snprintf(buffer, sizeof(buffer), "%hu", 1 << (input_value - 1));
        }

        if(button_pressed) {
            send(sockfd, buffer, sizeof(buffer), 0);
            std::lock_guard<std::mutex> button_lock(button_mutex);
            button_pressed = false;
        }
        else{
            send(sockfd, defaultBuffer, sizeof(defaultBuffer), 0);
        }
        
        recv(sockfd, recvMessage, sizeof(recvMessage), 0);

        
        //std::cout << "Received elevator state: " << recvMessage << std::endl;

        //std::this_thread::sleep_for(std::chrono::seconds(1)); // 每秒鐘檢查一次
    }

    inputThread.join();
    close(sockfd);
}

int main() {
    startClient();
    return 0;
}
