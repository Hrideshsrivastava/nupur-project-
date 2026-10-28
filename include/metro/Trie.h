#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  Trie.h — prefix tree powering station-name autocomplete.
//
//  Every station is inserted once per word, so typing "sector 6" finds
//  "Noida Sector 62", and "chowk" finds "Rajiv Chowk", "IFFCO Chowk", …
//  Matches at the start of the full name are ranked before mid-name matches.
// ─────────────────────────────────────────────────────────────────────────────
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace metro {

class Trie {
public:
    Trie();
    ~Trie();
    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;

    /// Index `name` (for every word suffix) and associate it with `id`.
    void insert(const std::string& name, int id);

    /// Up to `limit` ids whose name (or one of its words) starts with `prefix`.
    std::vector<int> suggest(const std::string& prefix, std::size_t limit) const;

    /// True if `name` was inserted as a complete name.
    bool contains(const std::string& name) const;

    std::size_t nodeCount() const { return nodeCount_; }
    std::size_t wordCount() const { return wordCount_; }

private:
    struct Entry {
        int id;
        int rank; // 0 = matched from the first word, otherwise word position
        std::string display;
    };
    struct Node {
        std::map<char, std::unique_ptr<Node>> children; // ordered => alphabetical DFS
        std::vector<Entry> entries;                      // keys that end here
    };

    void insertKey(const std::string& key, const Entry& entry);
    const Node* walk(const std::string& key) const;
    static void collect(const Node* node, std::vector<Entry>& out, std::size_t cap);

    std::unique_ptr<Node> root_;
    std::size_t nodeCount_ = 1;
    std::size_t wordCount_ = 0;
};

} // namespace metro
