#include "metro/Trie.h"

#include "metro/Sorting.h"
#include "metro/StringUtil.h"

#include <unordered_set>

namespace metro {

Trie::Trie() : root_(std::make_unique<Node>()) {}
Trie::~Trie() = default;

void Trie::insert(const std::string& name, int id) {
    const std::string key = str::normalizeWords(name);
    if (key.empty()) return;
    ++wordCount_;
    // Insert the whole name, then every suffix that starts at a word boundary.
    int wordIndex = 0;
    for (std::size_t i = 0; i < key.size(); ++i) {
        if (i == 0 || key[i - 1] == ' ') {
            insertKey(key.substr(i), Entry{id, wordIndex, name});
            ++wordIndex;
        }
    }
}

void Trie::insertKey(const std::string& key, const Entry& entry) {
    Node* node = root_.get();
    for (char c : key) {
        auto& child = node->children[c];
        if (!child) {
            child = std::make_unique<Node>();
            ++nodeCount_;
        }
        node = child.get();
    }
    node->entries.push_back(entry);
}

const Trie::Node* Trie::walk(const std::string& key) const {
    const Node* node = root_.get();
    for (char c : key) {
        auto it = node->children.find(c);
        if (it == node->children.end()) return nullptr;
        node = it->second.get();
    }
    return node;
}

void Trie::collect(const Node* node, std::vector<Entry>& out, std::size_t cap) {
    // Pre-order depth-first traversal; children are visited alphabetically.
    if (out.size() >= cap) return;
    for (const Entry& e : node->entries) out.push_back(e);
    for (const auto& [ch, child] : node->children) {
        (void)ch;
        if (out.size() >= cap) return;
        collect(child.get(), out, cap);
    }
}

std::vector<int> Trie::suggest(const std::string& prefix, std::size_t limit) const {
    std::vector<int> result;
    const std::string key = str::normalizeWords(prefix);
    if (key.empty() || limit == 0) return result;
    const Node* node = walk(key);
    if (!node) return result;

    std::vector<Entry> found;
    collect(node, found, 512);

    // Full-name matches first, then by word position, then alphabetically.
    mergeSort(found, [](const Entry& a, const Entry& b) {
        if ((a.rank == 0) != (b.rank == 0)) return a.rank == 0;
        if (a.rank != b.rank) return a.rank < b.rank;
        return a.display < b.display;
    });

    std::unordered_set<int> seen;
    for (const Entry& e : found) {
        if (seen.insert(e.id).second) {
            result.push_back(e.id);
            if (result.size() >= limit) break;
        }
    }
    return result;
}

bool Trie::contains(const std::string& name) const {
    const Node* node = walk(str::normalizeWords(name));
    if (!node) return false;
    for (const Entry& e : node->entries)
        if (e.rank == 0) return true;
    return false;
}

} // namespace metro
