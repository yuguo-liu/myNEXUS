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
#include <thread> // Requires C++11
#include <chrono>

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
        uint32_t total_chunks;    // Changed to uint32_t
        uint32_t received_count;  // Track count to avoid iterating vector every time
    };

    std::unordered_map<uint32_t, FragmentBuffer> fragment_map;

public:
    // Constructor
    Channel(const std::string& ip, int port, int seed, int recvbuf_size = 128*1024*1024) // Default expanded to 128MB
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
    void initialize() {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        // Set large receive buffer
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &max_rcvbuf_size, sizeof(max_rcvbuf_size)) < 0) {
            // If system limits cause setting to fail, try setting a smaller value or ignore
            // std::cerr << "Warning: Could not set requested SO_RCVBUF" << std::endl;
        }

        // Set large send buffer as well
        int sndbuf_size = max_rcvbuf_size;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf_size, sizeof(sndbuf_size));

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
    }

    uint32_t generate_msg_id() {
        return static_cast<uint32_t>(rand());
    }

public:
    // Send message (auto fragment)
    bool send(const std::string& message, const std::string& dest_ip, int dest_port) {
        if (!initialized) return false;

        struct sockaddr_in dest_addr;
        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_addr.s_addr = inet_addr(dest_ip.c_str());
        dest_addr.sin_port = htons(dest_port);

        // Use 12-byte header: msg_id(4) + total(4) + index(4)
        const size_t header_len = sizeof(uint32_t) * 3;
        size_t max_payload = max_udp_payload - header_len;
        
        size_t total_chunks = (message.size() + max_payload - 1) / max_payload;
        uint32_t msg_id = generate_msg_id();

        // Packet buffer reuse to reduce allocation
        std::string packet; 
        packet.reserve(max_udp_payload);

        for (size_t i = 0; i < total_chunks; ++i) {
            size_t offset = i * max_payload;
            size_t chunk_size = std::min(max_payload, message.size() - offset);

            packet.resize(header_len + chunk_size);

            // Fill header (Use pointer manipulation to avoid endian/alignment issues, safe under the same architecture)
            uint32_t* header_ptr = (uint32_t*)&packet[0];
            header_ptr[0] = msg_id;
            header_ptr[1] = (uint32_t)total_chunks; // Cast to uint32
            header_ptr[2] = (uint32_t)i;            // Cast to uint32

            // Fill payload
            memcpy(&packet[header_len], message.data() + offset, chunk_size);

            ssize_t sent_bytes = sendto(sockfd, packet.data(), packet.size(), 0,
                                      (struct sockaddr*)&dest_addr, sizeof(dest_addr));
            
            if (sent_bytes < 0) {
                // If buffer is full, simply retry
                if (errno == ENOBUFS || errno == EAGAIN) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    i--; // Retry this chunk
                    continue;
                }
                return false;
            }

            // === CRITICAL FIX for 100MB ===
            // Flow Control: Simple sleep every N packets
            // Sleep 500 microseconds every 20 packets sent (~28KB).
            // This will limit the sending rate, giving the receiver and router a chance to breathe.
            if (i % 20 == 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(500));
            }
        }

        return true;
    }

    // Receive message
    bool recv(std::string& message, std::string& src_ip, int& src_port, int timeout_ms = 0) {
        loop_again:
        if (!initialized) return false;

        if (timeout_ms > 0) {
            struct timeval tv;
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        }

        char buffer[65536];
        struct sockaddr_in src_addr;
        socklen_t addr_len = sizeof(src_addr);

        ssize_t recv_bytes = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                     (struct sockaddr*)&src_addr, &addr_len);
        
        if (recv_bytes < 0) {
            return false; // Error or Timeout
        }

        src_ip = inet_ntoa(src_addr.sin_addr);
        src_port = ntohs(src_addr.sin_port);

        const size_t header_len = sizeof(uint32_t) * 3;
        if (recv_bytes < (ssize_t)header_len) {
            goto loop_again; // Packet too small
        }

        // Parse header (uint32_t)
        uint32_t* header_ptr = (uint32_t*)buffer;
        uint32_t msg_id = header_ptr[0];
        uint32_t total_chunks = header_ptr[1];
        uint32_t chunk_index = header_ptr[2];

        // Store chunk
        std::string payload(buffer + header_len, buffer + recv_bytes);
        
        auto& frag = fragment_map[msg_id];
        if (frag.chunks.empty()) {
            frag.chunks.resize(total_chunks);
            frag.total_chunks = total_chunks;
            frag.received_count = 0;
            frag.last_update = time(nullptr);
        }

        // Check index validity
        if (chunk_index >= total_chunks) goto loop_again;

        // Only add if not received yet (avoid count errors caused by duplicate packets)
        if (frag.chunks[chunk_index].empty()) {
            frag.chunks[chunk_index] = std::move(payload);
            frag.received_count++;
            frag.last_update = time(nullptr);
        }

        // Check if complete
        if (frag.received_count == total_chunks) {
            // Reassemble
            // Pre-calculate total size to reduce memory reallocation
            size_t total_size = 0;
            for (const auto& chunk : frag.chunks) total_size += chunk.size();
            
            message.clear();
            message.reserve(total_size);
            
            for (auto& chunk : frag.chunks) {
                message += chunk;
            }
            
            fragment_map.erase(msg_id);
            return true;
        }

        // Cleanup stale partial messages (optional, prevents memory leak)
        // In actual use, stale map entries can be cleaned up periodically
        
        goto loop_again;
    }
};

#endif