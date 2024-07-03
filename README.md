## elevator simulator

程式分成兩個部分，兩個程式透過 socket 溝通，第一個部分是 `client.cpp` ，也是負責處理輸入的程式，會負責檢查使用者有沒有按按鍵並將指令傳送給 server

電梯的按鍵 `__ __ __ __` 由左到右代表 在電梯內按一樓、在電梯內按二樓、在電梯外按一樓、在電梯外按二樓，取得使用者的輸入 `input value` 之後再用 `1 << (input_value - 1)` 的方式轉成上述的按鍵表示方式

在這部分我使用了一個另外的thread去看使用者有沒有輸入，如果沒有輸入的話，就傳送0給server，有輸入的話才會傳送使用者輸入的結果

第二個部分是 `server.cpp` ，負責模擬並展示一個兩層樓電梯的運行，接收 client 傳來的按鈕並做出對應的反應，如果 client 一直沒有按按鈕，電梯就會保持 idel，電梯的狀態包含以下六個

1. floor one idle
2. floor two idlw
3. floor one open
4. floor two open
5. lift
6. down

在這個部分中我使用了另外一個 thread 去持續印出電梯目前的狀態，為了模擬電梯在開門的時候會持續兩秒鐘以及上升、下降時需要五秒鐘的運行時間，目前我是使用 `this_thread::sleep_for(std::chrono::seconds(2))`來讓特定 thread 休眠指定的時間


#### How to run

`g++ server.cpp -o server -pthread`

`g++ client.cpp -o client -pthread`

`./server`

`./client`

#### To do

1. 如果使用者同時按下四個按鈕
2. 如何加入timer

#### Ref

1. http://zake7749.github.io/2015/03/17/SocketProgramming/
2. chatgpt
