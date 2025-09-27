#include "../server.hpp"
#include <thread>
#include <atomic>

// 用于统计连接数
std::atomic<int> g_conn_count{0};

void echo_session(int cli_fd) {
	Socket cli_sock;
	// 直接用 cli_fd 替换 Socket 的 _sockfd
	*(int*)((char*)&cli_sock) = cli_fd;
	Buffer buffer;
	g_conn_count++;
	INF_LOG("new connection, total: %d", g_conn_count.load());
	int msg_count = 0;
	while (true) {
		char buf[1024] = {0};
		ssize_t n = cli_sock.Recv(buf, 1023);
		if (n <= 0) break;
		buffer.Clear();
		buffer.WriteAndPush(buf, n);
		std::string msg = buffer.ReadAsString(buffer.ReadAbleSize());
		DBG_LOG("[echo] recv: %s", msg.c_str());
		cli_sock.Send(msg.c_str(), msg.size());
		msg_count++;
		// 每收到3条消息，演示日志等级和Buffer的GetLine功能
		if (msg_count % 3 == 0) {
			ERR_LOG("[echo] 3 messages received from fd=%d", cli_fd);
			buffer.WriteStringAndPush("test\nline\n");
			std::string line = buffer.GetLineAndPop();
			INF_LOG("Buffer GetLine: %s", line.c_str());
		}
	}
	cli_sock.Close();
	g_conn_count--;
	INF_LOG("connection closed, total: %d", g_conn_count.load());
}

// 演示定时器功能
void test_timer() {
	EventLoop loop;
	loop.TimerAdd(1, 3, [](){ INF_LOG("定时器触发: 3秒后"); });
	std::thread([&loop](){ loop.Start(); }).detach();
	sleep(4); // 等待定时器触发
}

// 如果只想测试 echo 服务，直接注释掉 test_timer() 部分即可
int main() {
    Socket svr_sock;
    svr_sock.CreateServer("0.0.0.0", 8500);

    while (true) {
        int cli_fd = svr_sock.Accept();
        if (cli_fd < 0) continue;
        std::thread(echo_session, cli_fd).detach();
    }
    return 0;
}
