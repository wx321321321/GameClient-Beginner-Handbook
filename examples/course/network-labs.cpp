#ifdef NDEBUG
#error "Teaching labs require assertions; compile without NDEBUG."
#endif

#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// Teaching format: unsigned 16-bit big-endian length, followed by payload bytes.
// Feed limits each accumulated batch; real servers should use bounded rings
// and explicit backpressure rather than repeatedly erasing vector prefixes.
class FrameDecoder {
    std::vector<std::uint8_t> pending_;
public:
    std::vector<std::string> Feed(const std::vector<std::uint8_t>& bytes) {
        if (bytes.size() > 4096 - pending_.size()) {
            throw std::length_error("input buffer limit");
        }
        pending_.insert(pending_.end(), bytes.begin(), bytes.end());
        std::vector<std::string> messages;
        while (pending_.size() >= 2) {
            const std::size_t length =
                (static_cast<std::size_t>(pending_[0]) << 8) | pending_[1];
            if (length > 1024) throw std::length_error("payload limit");
            if (pending_.size() - 2 < length) break;
            messages.emplace_back(pending_.begin() + 2,
                                  pending_.begin() + 2 + length);
            pending_.erase(pending_.begin(), pending_.begin() + 2 + length);
        }
        return messages;
    }
};

struct SnapshotReceiver {
    bool received{false};
    std::uint64_t lastSequence{0};
    int position{0};
    bool Accept(std::uint64_t sequence, int newPosition) {
        if (received && sequence <= lastSequence) return false;
        received = true;
        lastSequence = sequence;
        position = newPosition;
        return true;
    }
};

// A simplified authoritative request handler. The caller already resolved
// the player from the authenticated connection. Reconnection has a new session.
struct ServerPlayer {
    std::uint64_t lastRequest{0};
    int ammo{2};
    bool Fire(std::uint64_t request) {
        if (request == 0 || request <= lastRequest) return false;
        lastRequest = request; // consumed, including a rejected empty-mag request
        if (ammo == 0) return false;
        --ammo;
        return true;
    }
};

int main() {
    FrameDecoder decoder;
    assert(decoder.Feed({0}).empty());
    assert(decoder.Feed({3, 'a'}).empty());
    const auto messages = decoder.Feed({'b', 'c', 0, 2, 'd', 'e'});
    assert((messages == std::vector<std::string>{"abc", "de"}));
    assert(decoder.Feed({0, 0}) == std::vector<std::string>{""});
    FrameDecoder invalid;
    bool rejected = false;
    try { invalid.Feed({4, 1}); } // declared 1025 bytes, rejected before allocation
    catch (const std::length_error&) { rejected = true; }
    assert(rejected);

    SnapshotReceiver receiver;
    assert(receiver.Accept(101, 1));
    assert(receiver.Accept(103, 3));
    assert(!receiver.Accept(102, 2));
    assert(receiver.position == 3);
    ServerPlayer player;
    assert(player.Fire(1));
    assert(!player.Fire(1));
    assert(player.ammo == 1);
    assert(player.Fire(2));
    assert(!player.Fire(3));
    std::cout << "frames=abc,de position=3 ammo=0\n";
}
