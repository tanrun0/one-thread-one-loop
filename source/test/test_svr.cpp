#include "../server.hpp"
#include <thread>

class TcpServer {
private:
    Socket _sock;
    Epoller _epoller;
    std::vector<Channel *> _active_channels;
    
public:
    TcpServer(const std::string &ip, uint16_t port) {
        _sock.CreateServer(ip, port);
        _sock.NonBlock();
    }
    
    ~TcpServer() {
        _sock.Close();
    }
    
    void Start() {
        Channel *serv_channel = new Channel(&_epoller, _sock.Fd());
        serv_channel->SetReadCallback(std::bind(&TcpServer::HandleNewConnection, this));
        serv_channel->EnableRead();
        
        while (true) {
            _active_channels.clear();
            _epoller.Poll(&_active_channels);
            
            for (auto &channel : _active_channels) {
                channel->HandleEvent();
            }
        }
    }
    
private:
    void HandleNewConnection() {
        int newfd = _sock.Accept();
        if (newfd < 0) {
            ERR_LOG("Accept error");
            return;
        }
        
        INF_LOG("New connection fd: %d", newfd);
        
        Channel *cli_channel = new Channel(&_epoller, newfd);
        cli_channel->SetReadCallback(std::bind(&TcpServer::HandleMessage, this, cli_channel));
        cli_channel->SetCloseCallback(std::bind(&TcpServer::HandleClose, this, cli_channel));
        cli_channel->EnableRead();
    }
    
    void HandleMessage(Channel *channel) {
        int fd = channel->Fd();
        char buf[1024] = {0};
        ssize_t ret = recv(fd, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) {
            if (ret < 0 && (errno == EAGAIN || errno == EINTR)) {
                return;
            }
            return HandleClose(channel);
        }
        
        buf[ret] = '\0';
        INF_LOG("Recv from fd %d: %s", fd, buf);
        
        // Echo back
        std::string response = "Server response: ";
        response += buf;
        send(fd, response.c_str(), response.size(), 0);
    }
    
    void HandleClose(Channel *channel) {
        int fd = channel->Fd();
        INF_LOG("Connection closed fd: %d", fd);
        close(fd);
        delete channel;
    }
};

int main() {
    TcpServer server("0.0.0.0", 8500);
    server.Start();
    return 0;
}