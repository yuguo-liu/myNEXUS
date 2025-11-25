#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include <cstdlib>

// #include "tools.h"

class Channel {
private:
    int sockfd;
    std::string my_ip;
    int my_port;
    bool initialized;
    size_t max_udp_payload;
    int max_rcvbuf_size;

    struct FragmentBuffer {
        std::vector<std::string> chunks;
        time_t last_update;
        int total_chunks;
    };

    std::unordered_map<uint32_t, FragmentBuffer> fragment_map;

public:
    // Constructor
    Channel(const std::string& ip, int port, int seed, int recvbuf_size = 4*1024*1024)
        : my_ip(ip), my_port(port), initialized(false), max_udp_payload(1400), max_rcvbuf_size(recvbuf_size) {
        srand(seed);
        initialize();
    }

    // Destructor
    ~Channel() {
        if (initialized) {
            close(sockfd);
        }
    }

private:
    // Initialize socket
    void initialize() {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        // Set large receive buffer
        setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &max_rcvbuf_size, sizeof(max_rcvbuf_size));

        struct sockaddr_in my_addr;
        memset(&my_addr, 0, sizeof(my_addr));
        my_addr.sin_family = AF_INET;
        my_addr.sin_addr.s_addr = inet_addr(my_ip.c_str());
        my_addr.sin_port = htons(my_port);

        if (bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0) {
            close(sockfd);
            throw std::runtime_error("Failed to bind socket");
        }

        initialized = true;
        // mlog(MSG_INFO, "Communication initialized successfully. Listening on %s:%d", my_ip.c_str(), my_port);
    }

    uint32_t generate_msg_id() {
        return static_cast<uint32_t>(rand());
    }

public:
    // Send message (auto fragment if > max_udp_payload)
    bool send(const std::string& message, const std::string& dest_ip, int dest_port) {
        if (!initialized) {
            // mlog(MSG_ERROR, "Communication not properly initialized");
            return false;
        }

        struct sockaddr_in dest_addr;
        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_addr.s_addr = inet_addr(dest_ip.c_str());
        dest_addr.sin_port = htons(dest_port);

        size_t max_payload = max_udp_payload - (sizeof(uint32_t) + sizeof(uint16_t)*2);
        size_t total_chunks = (message.size() + max_payload - 1) / max_payload;
        uint32_t msg_id = generate_msg_id();

        for (size_t i = 0; i < total_chunks; ++i) {
            size_t offset = i * max_payload;
            size_t chunk_size = std::min(max_payload, message.size() - offset);

            std::string packet;
            packet.resize(sizeof(uint32_t) + sizeof(uint16_t)*2 + chunk_size);

            // Fill header
            memcpy(&packet[0], &msg_id, sizeof(uint32_t));
            uint16_t total = total_chunks;
            memcpy(&packet[4], &total, sizeof(uint16_t));
            uint16_t index = i;
            memcpy(&packet[6], &index, sizeof(uint16_t));

            // Fill payload
            memcpy(&packet[8], message.data() + offset, chunk_size);

            ssize_t sent_bytes = sendto(sockfd, packet.data(), packet.size(), 0,
                                        (struct sockaddr*)&dest_addr, sizeof(dest_addr));
            if (sent_bytes < 0) {
                // mlog(MSG_ERROR, "Failed to send chunk %zu/%zu", i+1, total_chunks);
                return false;
            }
        }

        // mlog(MSG_INFO, "Sent message of %zu bytes in %zu chunks to %s:%d", 
            //  message.size(), total_chunks, dest_ip.c_str(), dest_port);
        return true;
    }

    // Receive message (auto reassemble)
    bool recv(std::string& message, std::string& src_ip, int& src_port, int timeout_ms = 0) {
        loop_again:
        if (!initialized) {
            // mlog(MSG_ERROR, "Communication not properly initialized");
            return false;
        }

        if (timeout_ms > 0) {
            struct timeval tv;
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        }

        char buffer[65536]; // full UDP max size
        struct sockaddr_in src_addr;
        socklen_t addr_len = sizeof(src_addr);

        ssize_t recv_bytes = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                     (struct sockaddr*)&src_addr, &addr_len);
        if (recv_bytes < 0) {
            if (timeout_ms > 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                // mlog(MSG_ERROR, "Timeout occurred");
                return false;
            }
            // mlog(MSG_ERROR, "Failed to receive message");
            return false;
        }

        src_ip = inet_ntoa(src_addr.sin_addr);
        src_port = ntohs(src_addr.sin_port);

        if (recv_bytes < 8) {
            // mlog(MSG_ERROR, "Received packet too small");
            return false;
        }

        // Parse header
        uint32_t msg_id;
        memcpy(&msg_id, &buffer[0], sizeof(uint32_t));
        uint16_t total_chunks;
        memcpy(&total_chunks, &buffer[4], sizeof(uint16_t));
        uint16_t chunk_index;
        memcpy(&chunk_index, &buffer[6], sizeof(uint16_t));

        // Store chunk
        std::string payload(buffer + 8, buffer + recv_bytes);
        auto& frag = fragment_map[msg_id];
        if (frag.chunks.empty()) {
            frag.chunks.resize(total_chunks);
            frag.total_chunks = total_chunks;
        }
        frag.chunks[chunk_index] = std::move(payload);
        frag.last_update = time(nullptr);

        // Check if complete
        bool complete = true;
        for (auto& chunk : frag.chunks) {
            if (chunk.empty()) {
                complete = false;
                break;
            }
        }

        if (complete) {
            message.clear();
            for (auto& chunk : frag.chunks) {
                message += chunk;
            }
            fragment_map.erase(msg_id);
            // mlog(MSG_INFO, "Reassembled complete message of %zu bytes from %s:%d", 
                //  message.size(), src_ip.c_str(), src_port);
            return true;
        }

        // Not complete yet
        // return false;
        goto loop_again;
    }
};

#endif
